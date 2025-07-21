#pragma once

/**
 * @file config_manager.hpp
 * @brief Hierarchical configuration management system with hot-reloading
 */

#include "config_value.hpp"
#include "config_concepts.hpp"
#include <map>
#include <memory>
#include <functional>
#include <filesystem>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

namespace liarsdice::config {

/**
 * @brief Configuration change event
 */
struct ConfigChangeEvent {
    ConfigPath path;
    ConfigValue old_value;
    ConfigValue new_value;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Configuration source interface
 */
class IConfigSource {
public:
    virtual ~IConfigSource() = default;
    
    /**
     * @brief Check if source has a value for the given path
     */
    virtual bool has_value(const ConfigPath& path) const = 0;
    
    /**
     * @brief Get raw string value from source
     */
    virtual std::optional<std::string> get_raw_value(const ConfigPath& path) const = 0;
    
    /**
     * @brief Get all available keys from source
     */
    virtual std::vector<ConfigPath> get_all_paths() const = 0;
    
    /**
     * @brief Get source priority (higher = more important)
     */
    virtual int get_priority() const noexcept = 0;
    
    /**
     * @brief Get source name for debugging
     */
    virtual std::string get_name() const = 0;
    
    /**
     * @brief Check if source supports watching for changes
     */
    virtual bool supports_watching() const noexcept { return false; }
    
    /**
     * @brief Start watching for changes (if supported)
     */
    virtual void start_watching() {}
    
    /**
     * @brief Stop watching for changes
     */
    virtual void stop_watching() {}
};

/**
 * @brief Configuration manager with hierarchical structure and hot-reloading
 */
class ConfigManager {
public:
    using ChangeListener = std::function<void(const ConfigChangeEvent&)>;
    
    /**
     * @brief Constructor
     */
    ConfigManager();
    
    /**
     * @brief Destructor
     */
    ~ConfigManager();
    
    // Non-copyable, non-movable
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;
    
    /**
     * @brief Add configuration source
     */
    void add_source(std::unique_ptr<IConfigSource> source);
    
    /**
     * @brief Remove configuration source by name
     */
    void remove_source(const std::string& name);
    
    /**
     * @brief Get configuration value with type safety
     */
    template<typename T>
    std::optional<T> get(const ConfigPath& path) const {
        std::shared_lock lock(mutex_);
        return get_internal<T>(path);
    }
    
    /**
     * @brief Get configuration value with default
     */
    template<typename T>
    T get_or(const ConfigPath& path, const T& default_value) const {
        if (auto value = get<T>(path)) {
            return *value;
        }
        return default_value;
    }
    
    /**
     * @brief Get required configuration value (throws if not found)
     */
    template<typename T>
    T get_required(const ConfigPath& path) const {
        if (auto value = get<T>(path)) {
            return *value;
        }
        throw ConfigException(std::format("Required configuration '{}' not found", path.to_string()));
    }
    
    /**
     * @brief Set configuration value (runtime override)
     */
    template<typename T>
    void set(const ConfigPath& path, const T& value) {
        std::unique_lock lock(mutex_);
        set_internal(path, ConfigValue{value});
    }
    
    /**
     * @brief Check if configuration exists
     */
    bool has(const ConfigPath& path) const;
    
    /**
     * @brief Get all configuration paths
     */
    std::vector<ConfigPath> get_all_paths() const;
    
    /**
     * @brief Add change listener
     */
    void add_change_listener(const std::string& name, ChangeListener listener);
    
    /**
     * @brief Remove change listener
     */
    void remove_change_listener(const std::string& name);
    
    /**
     * @brief Enable/disable hot-reloading
     */
    void set_hot_reload_enabled(bool enabled);
    
    /**
     * @brief Manually reload configuration from sources
     */
    void reload();
    
    /**
     * @brief Get configuration as hierarchical map
     */
    std::map<std::string, ConfigValue> get_section(const ConfigPath& section_path) const;
    
    /**
     * @brief Validate all configuration values
     */
    std::vector<std::string> validate() const;

private:
    template<typename T>
    std::optional<T> get_internal(const ConfigPath& path) const {
        // Check runtime overrides first
        if (auto it = runtime_overrides_.find(path); it != runtime_overrides_.end()) {
            return it->second.get<T>();
        }
        
        // Check sources in priority order
        for (const auto& source : sources_) {
            if (source->has_value(path)) {
                if (auto raw_value = source->get_raw_value(path)) {
                    // Try to parse as target type
                    if (auto config_value = ConfigValue::from_string(*raw_value, 
                            get_type_index<T>())) {
                        return config_value->get<T>();
                    }
                }
            }
        }
        
        return std::nullopt;
    }
    
    template<typename T>
    static constexpr std::size_t get_type_index() {
        if constexpr (std::same_as<T, bool>) return 1;
        else if constexpr (std::same_as<T, int32_t>) return 2;
        else if constexpr (std::same_as<T, int64_t>) return 3;
        else if constexpr (std::same_as<T, uint32_t>) return 4;
        else if constexpr (std::same_as<T, uint64_t>) return 5;
        else if constexpr (std::same_as<T, double>) return 6;
        else if constexpr (std::same_as<T, std::string>) return 7;
        else if constexpr (std::same_as<T, std::vector<std::string>>) return 8;
        else return 0; // monostate
    }
    
    void set_internal(const ConfigPath& path, ConfigValue value);
    void notify_change(const ConfigPath& path, const ConfigValue& old_value, const ConfigValue& new_value);
    void watch_sources();
    
    mutable std::shared_mutex mutex_;
    std::vector<std::unique_ptr<IConfigSource>> sources_;
    std::map<ConfigPath, ConfigValue> runtime_overrides_;
    std::map<std::string, ChangeListener> change_listeners_;
    
    std::atomic<bool> hot_reload_enabled_{true};
    std::unique_ptr<std::thread> watcher_thread_;
    std::atomic<bool> stop_watching_{false};
};

/**
 * @brief Global configuration manager instance
 */
ConfigManager& global_config();

/**
 * @brief Convenience functions for global configuration access
 */
template<typename T>
std::optional<T> get_config(const ConfigPath& path) {
    return global_config().get<T>(path);
}

template<typename T>
T get_config_or(const ConfigPath& path, const T& default_value) {
    return global_config().get_or<T>(path, default_value);
}

template<typename T>
T get_required_config(const ConfigPath& path) {
    return global_config().get_required<T>(path);
}

template<typename T>
void set_config(const ConfigPath& path, const T& value) {
    global_config().set<T>(path, value);
}

} // namespace liarsdice::config