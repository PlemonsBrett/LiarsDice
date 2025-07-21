/**
 * @file config_manager.cpp
 * @brief Implementation of hierarchical configuration management system
 */

#include "liarsdice/config/config_manager.hpp"
#include <algorithm>
#include <ranges>

namespace liarsdice::config {

ConfigManager::ConfigManager() = default;

ConfigManager::~ConfigManager() {
    stop_watching_ = true;
    if (watcher_thread_ && watcher_thread_->joinable()) {
        watcher_thread_->join();
    }
}

void ConfigManager::add_source(std::unique_ptr<IConfigSource> source) {
    std::unique_lock lock(mutex_);
    
    // Insert source in priority order (highest priority first)
    auto insert_pos = std::ranges::upper_bound(sources_, source->get_priority(),
        [](int priority, const auto& src) { return priority > src->get_priority(); });
    
    sources_.insert(insert_pos, std::move(source));
    
    // Start watching if enabled and supported
    if (hot_reload_enabled_ && sources_.back()->supports_watching()) {
        sources_.back()->start_watching();
    }
}

void ConfigManager::remove_source(const std::string& name) {
    std::unique_lock lock(mutex_);
    
    auto it = std::ranges::find_if(sources_, 
        [&name](const auto& src) { return src->get_name() == name; });
    
    if (it != sources_.end()) {
        (*it)->stop_watching();
        sources_.erase(it);
    }
}

bool ConfigManager::has(const ConfigPath& path) const {
    std::shared_lock lock(mutex_);
    
    // Check runtime overrides first
    if (runtime_overrides_.contains(path)) {
        return true;
    }
    
    // Check sources
    return std::ranges::any_of(sources_, 
        [&path](const auto& source) { return source->has_value(path); });
}

std::vector<ConfigPath> ConfigManager::get_all_paths() const {
    std::shared_lock lock(mutex_);
    
    std::set<ConfigPath> unique_paths;
    
    // Add runtime override paths
    for (const auto& [path, _] : runtime_overrides_) {
        unique_paths.insert(path);
    }
    
    // Add paths from all sources
    for (const auto& source : sources_) {
        auto source_paths = source->get_all_paths();
        unique_paths.insert(source_paths.begin(), source_paths.end());
    }
    
    return std::vector<ConfigPath>(unique_paths.begin(), unique_paths.end());
}

void ConfigManager::add_change_listener(const std::string& name, ChangeListener listener) {
    std::unique_lock lock(mutex_);
    change_listeners_[name] = std::move(listener);
}

void ConfigManager::remove_change_listener(const std::string& name) {
    std::unique_lock lock(mutex_);
    change_listeners_.erase(name);
}

void ConfigManager::set_hot_reload_enabled(bool enabled) {
    hot_reload_enabled_ = enabled;
    
    if (enabled && !watcher_thread_) {
        stop_watching_ = false;
        watcher_thread_ = std::make_unique<std::thread>(&ConfigManager::watch_sources, this);
    } else if (!enabled && watcher_thread_) {
        stop_watching_ = true;
        if (watcher_thread_->joinable()) {
            watcher_thread_->join();
        }
        watcher_thread_.reset();
    }
}

void ConfigManager::reload() {
    std::unique_lock lock(mutex_);
    
    // Store old values for change detection
    std::map<ConfigPath, ConfigValue> old_values;
    for (const auto& path : get_all_paths()) {
        if (auto value = get_internal<ConfigValue>(path)) {
            old_values[path] = *value;
        }
    }
    
    // Reload would typically involve re-reading sources
    // For now, just notify that reload happened
    
    // Check for changes and notify listeners
    for (const auto& path : get_all_paths()) {
        if (auto new_value = get_internal<ConfigValue>(path)) {
            auto old_it = old_values.find(path);
            ConfigValue old_value = (old_it != old_values.end()) ? old_it->second : ConfigValue{};
            
            if (!(*new_value == old_value)) {
                notify_change(path, old_value, *new_value);
            }
        }
    }
}

std::map<std::string, ConfigValue> ConfigManager::get_section(const ConfigPath& section_path) const {
    std::shared_lock lock(mutex_);
    
    std::map<std::string, ConfigValue> result;
    
    for (const auto& path : get_all_paths()) {
        // Check if path is under the section
        if (path.segments().size() > section_path.segments().size()) {
            bool is_under_section = true;
            for (size_t i = 0; i < section_path.segments().size(); ++i) {
                if (path.segments()[i] != section_path.segments()[i]) {
                    is_under_section = false;
                    break;
                }
            }
            
            if (is_under_section) {
                // Get relative path from section
                std::string relative_key = path.segments()[section_path.segments().size()];
                
                // Try to get value as different types (prioritize string)
                if (auto value = get_internal<std::string>(path)) {
                    result[relative_key] = ConfigValue{*value};
                } else if (auto value = get_internal<int64_t>(path)) {
                    result[relative_key] = ConfigValue{*value};
                } else if (auto value = get_internal<double>(path)) {
                    result[relative_key] = ConfigValue{*value};
                } else if (auto value = get_internal<bool>(path)) {
                    result[relative_key] = ConfigValue{*value};
                }
            }
        }
    }
    
    return result;
}

std::vector<std::string> ConfigManager::validate() const {
    std::shared_lock lock(mutex_);
    
    std::vector<std::string> errors;
    
    // Basic validation - check that all sources are accessible
    for (const auto& source : sources_) {
        try {
            source->get_all_paths();
        } catch (const std::exception& e) {
            errors.push_back(std::format("Source '{}' validation failed: {}", 
                                       source->get_name(), e.what()));
        }
    }
    
    return errors;
}

void ConfigManager::set_internal(const ConfigPath& path, ConfigValue value) {
    ConfigValue old_value;
    if (auto it = runtime_overrides_.find(path); it != runtime_overrides_.end()) {
        old_value = it->second;
    }
    
    runtime_overrides_[path] = value;
    notify_change(path, old_value, value);
}

void ConfigManager::notify_change(const ConfigPath& path, const ConfigValue& old_value, const ConfigValue& new_value) {
    ConfigChangeEvent event{
        .path = path,
        .old_value = old_value,
        .new_value = new_value,
        .timestamp = std::chrono::system_clock::now()
    };
    
    // Notify all listeners (without holding the main lock)
    std::vector<ChangeListener> listeners_copy;
    {
        std::shared_lock lock(mutex_);
        for (const auto& [name, listener] : change_listeners_) {
            listeners_copy.push_back(listener);
        }
    }
    
    for (const auto& listener : listeners_copy) {
        try {
            listener(event);
        } catch (const std::exception& e) {
            // Log error but don't let one bad listener break others
            // In a real implementation, we'd use proper logging here
        }
    }
}

void ConfigManager::watch_sources() {
    while (!stop_watching_) {
        {
            std::shared_lock lock(mutex_);
            for (const auto& source : sources_) {
                if (source->supports_watching()) {
                    // In a real implementation, we'd check for file changes here
                }
            }
        }
        
        // Check every 100ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

ConfigManager& global_config() {
    static ConfigManager instance;
    return instance;
}

} // namespace liarsdice::config