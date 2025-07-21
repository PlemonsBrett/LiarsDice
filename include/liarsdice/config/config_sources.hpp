#pragma once

/**
 * @file config_sources.hpp
 * @brief Configuration source implementations for various input types
 */

#include "config_manager.hpp"
#include <filesystem>
#include <span>
#include <cstdlib>
#include <unordered_map>

// Forward declare nlohmann::json to avoid including the header here
namespace nlohmann {
    class json;
}

namespace liarsdice::config {

/**
 * @brief JSON file configuration source with hot-reloading
 */
class JsonFileSource : public IConfigSource {
public:
    /**
     * @brief Constructor
     */
    explicit JsonFileSource(std::filesystem::path file_path, int priority = 100);
    
    /**
     * @brief Destructor
     */
    ~JsonFileSource() override;
    
    // IConfigSource implementation
    bool has_value(const ConfigPath& path) const override;
    std::optional<std::string> get_raw_value(const ConfigPath& path) const override;
    std::vector<ConfigPath> get_all_paths() const override;
    int get_priority() const noexcept override { return priority_; }
    std::string get_name() const override { return std::format("JsonFile({})", file_path_.string()); }
    
    bool supports_watching() const noexcept override { return true; }
    void start_watching() override;
    void stop_watching() override;
    
    /**
     * @brief Manually reload the file
     */
    void reload();
    
    /**
     * @brief Check if file exists and is readable
     */
    bool is_valid() const;

private:
    void load_file();
    void parse_json_recursive(const nlohmann::json& json_obj, const ConfigPath& base_path);
    
    std::filesystem::path file_path_;
    int priority_;
    std::filesystem::file_time_type last_write_time_;
    mutable std::shared_mutex data_mutex_;
    std::unordered_map<std::string, std::string> data_;
    bool watching_{false};
};

/**
 * @brief Environment variables configuration source
 */
class EnvironmentSource : public IConfigSource {
public:
    /**
     * @brief Constructor with optional prefix
     */
    explicit EnvironmentSource(std::string prefix = "LIARSDICE_", int priority = 50);
    
    // IConfigSource implementation
    bool has_value(const ConfigPath& path) const override;
    std::optional<std::string> get_raw_value(const ConfigPath& path) const override;
    std::vector<ConfigPath> get_all_paths() const override;
    int get_priority() const noexcept override { return priority_; }
    std::string get_name() const override { return std::format("Environment({}*)", prefix_); }
    
private:
    std::string to_env_var_name(const ConfigPath& path) const;
    ConfigPath from_env_var_name(const std::string& env_var) const;
    
    std::string prefix_;
    int priority_;
};

/**
 * @brief Command-line arguments configuration source
 */
class CommandLineSource : public IConfigSource {
public:
    /**
     * @brief Constructor from argc/argv
     */
    CommandLineSource(int argc, char* argv[], int priority = 200);
    
    /**
     * @brief Constructor from span
     */
    explicit CommandLineSource(std::span<const std::string_view> args, int priority = 200);
    
    // IConfigSource implementation
    bool has_value(const ConfigPath& path) const override;
    std::optional<std::string> get_raw_value(const ConfigPath& path) const override;
    std::vector<ConfigPath> get_all_paths() const override;
    int get_priority() const noexcept override { return priority_; }
    std::string get_name() const override { return "CommandLine"; }

private:
    void parse_arguments(std::span<const std::string_view> args);
    
    std::unordered_map<std::string, std::string> data_;
    int priority_;
};

/**
 * @brief In-memory configuration source for defaults
 */
class DefaultsSource : public IConfigSource {
public:
    /**
     * @brief Constructor
     */
    explicit DefaultsSource(int priority = 10);
    
    /**
     * @brief Add default value
     */
    template<typename T>
    void add_default(const ConfigPath& path, const T& value) {
        std::unique_lock lock(data_mutex_);
        if constexpr (std::same_as<T, std::string>) {
            data_[path.to_string()] = value;
        } else {
            data_[path.to_string()] = std::to_string(value);
        }
    }
    
    /**
     * @brief Add multiple defaults from initializer list
     */
    void add_defaults(std::initializer_list<std::pair<ConfigPath, std::string>> defaults);
    
    // IConfigSource implementation
    bool has_value(const ConfigPath& path) const override;
    std::optional<std::string> get_raw_value(const ConfigPath& path) const override;
    std::vector<ConfigPath> get_all_paths() const override;
    int get_priority() const noexcept override { return priority_; }
    std::string get_name() const override { return "Defaults"; }

private:
    mutable std::shared_mutex data_mutex_;
    std::unordered_map<std::string, std::string> data_;
    int priority_;
};

/**
 * @brief Environment variable wrapper with modern C++ interface
 */
class EnvironmentWrapper {
public:
    /**
     * @brief Get environment variable value
     */
    static std::optional<std::string> get(const std::string& name);
    
    /**
     * @brief Get environment variable with default value
     */
    static std::string get_or(const std::string& name, const std::string& default_value);
    
    /**
     * @brief Set environment variable (process-local)
     */
    static bool set(const std::string& name, const std::string& value);
    
    /**
     * @brief Check if environment variable exists
     */
    static bool exists(const std::string& name);
    
    /**
     * @brief Get all environment variables with given prefix
     */
    static std::vector<std::pair<std::string, std::string>> get_with_prefix(const std::string& prefix);
};

/**
 * @brief Command-line argument parser using std::span
 */
class ArgumentParser {
public:
    struct ParsedArgument {
        std::string key;
        std::optional<std::string> value;
        bool is_flag{false};
    };
    
    /**
     * @brief Parse command-line arguments
     */
    static std::vector<ParsedArgument> parse(std::span<const std::string_view> args);
    
    /**
     * @brief Parse single argument
     */
    static std::optional<ParsedArgument> parse_single(std::string_view arg, 
                                                     std::optional<std::string_view> next_arg = std::nullopt);

private:
    static bool is_long_option(std::string_view arg);
    static bool is_short_option(std::string_view arg);
    static std::string normalize_key(std::string_view key);
};

} // namespace liarsdice::config