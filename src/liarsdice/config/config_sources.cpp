/**
 * @file config_sources.cpp
 * @brief Implementation of configuration source classes
 */

#include "liarsdice/config/config_sources.hpp"
#include <algorithm>
#include <fstream>
#include <ranges>
#include <sstream>

// Only include nlohmann/json if available (will be handled by CMake)
#ifdef LIARSDICE_HAS_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

namespace liarsdice::config {

// JsonFileSource implementation
JsonFileSource::JsonFileSource(std::filesystem::path file_path, int priority)
    : file_path_(std::move(file_path)), priority_(priority) {
  if (is_valid()) {
    load_file();
  }
}

JsonFileSource::~JsonFileSource() { stop_watching(); }

bool JsonFileSource::has_value(const ConfigPath &path) const {
  std::shared_lock lock(data_mutex_);
  return data_.contains(path.to_string());
}

std::optional<std::string> JsonFileSource::get_raw_value(const ConfigPath &path) const {
  std::shared_lock lock(data_mutex_);
  auto it = data_.find(path.to_string());
  return (it != data_.end()) ? std::optional<std::string>{it->second} : std::nullopt;
}

std::vector<ConfigPath> JsonFileSource::get_all_paths() const {
  std::shared_lock lock(data_mutex_);
  std::vector<ConfigPath> paths;
  paths.reserve(data_.size());

  for (const auto &[key, _] : data_) {
    paths.emplace_back(key);
  }

  return paths;
}

void JsonFileSource::start_watching() {
  watching_ = true;
  if (std::filesystem::exists(file_path_)) {
    last_write_time_ = std::filesystem::last_write_time(file_path_);
  }
}

void JsonFileSource::stop_watching() { watching_ = false; }

void JsonFileSource::reload() {
  if (is_valid()) {
    load_file();
  }
}

bool JsonFileSource::is_valid() const {
  return std::filesystem::exists(file_path_) && std::filesystem::is_regular_file(file_path_);
}

void JsonFileSource::load_file() {
#ifdef LIARSDICE_HAS_NLOHMANN_JSON
  try {
    std::ifstream file(file_path_);
    if (!file.is_open()) {
      return;
    }

    nlohmann::json json_data;
    file >> json_data;

    std::unique_lock lock(data_mutex_);
    data_.clear();
    parse_json_recursive(json_data, ConfigPath{});

    if (watching_ && std::filesystem::exists(file_path_)) {
      last_write_time_ = std::filesystem::last_write_time(file_path_);
    }
  } catch (const std::exception &e) {
    // In a real implementation, we'd log this error
    // For now, just clear the data
    std::unique_lock lock(data_mutex_);
    data_.clear();
  }
#else
  // Fallback: simple key=value parsing
  try {
    std::ifstream file(file_path_);
    if (!file.is_open()) {
      return;
    }

    std::unique_lock lock(data_mutex_);
    data_.clear();

    std::string line;
    while (std::getline(file, line)) {
      // Skip empty lines and comments
      if (line.empty() || line[0] == '#' || line[0] == ';') {
        continue;
      }

      auto pos = line.find('=');
      if (pos != std::string::npos) {
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        data_[key] = value;
      }
    }
  } catch (const std::exception &e) {
    std::unique_lock lock(data_mutex_);
    data_.clear();
  }
#endif
}

#ifdef LIARSDICE_HAS_NLOHMANN_JSON
void JsonFileSource::parse_json_recursive(const nlohmann::json &json_obj,
                                          const ConfigPath &base_path) {
  if (json_obj.is_object()) {
    for (auto it = json_obj.begin(); it != json_obj.end(); ++it) {
      ConfigPath new_path = base_path.append(it.key());
      parse_json_recursive(it.value(), new_path);
    }
  } else if (json_obj.is_array()) {
    for (size_t i = 0; i < json_obj.size(); ++i) {
      ConfigPath new_path = base_path.append(std::to_string(i));
      parse_json_recursive(json_obj[i], new_path);
    }
  } else {
    // Leaf value - convert to string
    std::string value;
    if (json_obj.is_string()) {
      value = json_obj.get<std::string>();
    } else if (json_obj.is_boolean()) {
      value = json_obj.get<bool>() ? "true" : "false";
    } else if (json_obj.is_number()) {
      value = json_obj.dump();
    } else if (json_obj.is_null()) {
      value = "";
    }

    data_[base_path.to_string()] = value;
  }
}
#endif

// EnvironmentSource implementation
EnvironmentSource::EnvironmentSource(std::string prefix, int priority)
    : prefix_(std::move(prefix)), priority_(priority) {}

bool EnvironmentSource::has_value(const ConfigPath &path) const {
  std::string env_var = to_env_var_name(path);
  return EnvironmentWrapper::exists(env_var);
}

std::optional<std::string> EnvironmentSource::get_raw_value(const ConfigPath &path) const {
  std::string env_var = to_env_var_name(path);
  return EnvironmentWrapper::get(env_var);
}

std::vector<ConfigPath> EnvironmentSource::get_all_paths() const {
  std::vector<ConfigPath> paths;
  auto env_vars = EnvironmentWrapper::get_with_prefix(prefix_);

  for (const auto &[name, _] : env_vars) {
    paths.push_back(from_env_var_name(name));
  }

  return paths;
}

std::string EnvironmentSource::to_env_var_name(const ConfigPath &path) const {
  std::string result = prefix_;

  for (const auto &segment : path.segments()) {
    std::string upper_segment = segment;
    std::transform(upper_segment.begin(), upper_segment.end(), upper_segment.begin(), ::toupper);
    result += upper_segment + "_";
  }

  // Remove trailing underscore
  if (!result.empty() && result.back() == '_') {
    result.pop_back();
  }

  return result;
}

ConfigPath EnvironmentSource::from_env_var_name(const std::string &env_var) const {
  if (!env_var.starts_with(prefix_)) {
    return ConfigPath{};
  }

  std::string remaining = env_var.substr(prefix_.length());
  std::vector<std::string> segments;

  std::istringstream iss(remaining);
  std::string segment;

  while (std::getline(iss, segment, '_')) {
    if (!segment.empty()) {
      std::transform(segment.begin(), segment.end(), segment.begin(), ::tolower);
      segments.push_back(segment);
    }
  }

  return ConfigPath{std::move(segments)};
}

// CommandLineSource implementation
CommandLineSource::CommandLineSource(int argc, char *argv[], int priority) : priority_(priority) {
  std::vector<std::string_view> args;
  args.reserve(argc);

  for (int i = 1; i < argc; ++i) { // Skip program name
    args.emplace_back(argv[i]);
  }

  parse_arguments(args);
}

CommandLineSource::CommandLineSource(std::span<const std::string_view> args, int priority)
    : priority_(priority) {
  parse_arguments(args);
}

bool CommandLineSource::has_value(const ConfigPath &path) const {
  return data_.contains(path.to_string());
}

std::optional<std::string> CommandLineSource::get_raw_value(const ConfigPath &path) const {
  auto it = data_.find(path.to_string());
  return (it != data_.end()) ? std::optional<std::string>{it->second} : std::nullopt;
}

std::vector<ConfigPath> CommandLineSource::get_all_paths() const {
  std::vector<ConfigPath> paths;
  paths.reserve(data_.size());

  for (const auto &[key, _] : data_) {
    paths.emplace_back(key);
  }

  return paths;
}

void CommandLineSource::parse_arguments(std::span<const std::string_view> args) {
  auto parsed_args = ArgumentParser::parse(args);

  for (const auto &arg : parsed_args) {
    if (arg.is_flag) {
      data_[arg.key] = "true";
    } else if (arg.value) {
      data_[arg.key] = *arg.value;
    }
  }
}

// DefaultsSource implementation
DefaultsSource::DefaultsSource(int priority) : priority_(priority) {}

void DefaultsSource::add_defaults(
    std::initializer_list<std::pair<ConfigPath, std::string>> defaults) {
  std::unique_lock lock(data_mutex_);
  for (const auto &[path, value] : defaults) {
    data_[path.to_string()] = value;
  }
}

bool DefaultsSource::has_value(const ConfigPath &path) const {
  std::shared_lock lock(data_mutex_);
  return data_.contains(path.to_string());
}

std::optional<std::string> DefaultsSource::get_raw_value(const ConfigPath &path) const {
  std::shared_lock lock(data_mutex_);
  auto it = data_.find(path.to_string());
  return (it != data_.end()) ? std::optional<std::string>{it->second} : std::nullopt;
}

std::vector<ConfigPath> DefaultsSource::get_all_paths() const {
  std::shared_lock lock(data_mutex_);
  std::vector<ConfigPath> paths;
  paths.reserve(data_.size());

  for (const auto &[key, _] : data_) {
    paths.emplace_back(key);
  }

  return paths;
}

// EnvironmentWrapper implementation
std::optional<std::string> EnvironmentWrapper::get(const std::string &name) {
  const char *value = std::getenv(name.c_str());
  return value ? std::optional<std::string>{value} : std::nullopt;
}

std::string EnvironmentWrapper::get_or(const std::string &name, const std::string &default_value) {
  return get(name).value_or(default_value);
}

bool EnvironmentWrapper::set(const std::string &name, const std::string &value) {
#ifdef _WIN32
  return _putenv_s(name.c_str(), value.c_str()) == 0;
#else
  return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

bool EnvironmentWrapper::exists(const std::string &name) {
  return std::getenv(name.c_str()) != nullptr;
}

std::vector<std::pair<std::string, std::string>>
EnvironmentWrapper::get_with_prefix(const std::string &prefix) {
  std::vector<std::pair<std::string, std::string>> result;

#ifdef _WIN32
  // Windows implementation would need GetEnvironmentStrings()
  // For simplicity, just return empty vector
#else
  extern char **environ;
  for (char **env = environ; *env != nullptr; ++env) {
    std::string env_str(*env);
    auto pos = env_str.find('=');
    if (pos != std::string::npos) {
      std::string name = env_str.substr(0, pos);
      std::string value = env_str.substr(pos + 1);

      if (name.starts_with(prefix)) {
        result.emplace_back(std::move(name), std::move(value));
      }
    }
  }
#endif

  return result;
}

// ArgumentParser implementation
std::vector<ArgumentParser::ParsedArgument>
ArgumentParser::parse(std::span<const std::string_view> args) {
  std::vector<ParsedArgument> result;

  for (size_t i = 0; i < args.size(); ++i) {
    std::optional<std::string_view> next_arg =
        (i + 1 < args.size()) ? std::optional<std::string_view>{args[i + 1]} : std::nullopt;

    if (auto parsed = parse_single(args[i], next_arg)) {
      result.push_back(*parsed);

      // Skip next argument if it was consumed as a value
      if (parsed->value && next_arg && *next_arg == *parsed->value) {
        ++i;
      }
    }
  }

  return result;
}

std::optional<ArgumentParser::ParsedArgument>
ArgumentParser::parse_single(std::string_view arg, std::optional<std::string_view> next_arg) {
  if (is_long_option(arg)) {
    // Handle --key=value or --key value
    arg.remove_prefix(2); // Remove "--"

    auto pos = arg.find('=');
    if (pos != std::string_view::npos) {
      // --key=value format
      return ParsedArgument{.key = normalize_key(arg.substr(0, pos)),
                            .value = std::string(arg.substr(pos + 1)),
                            .is_flag = false};
    } else {
      // --key value format or flag
      std::string key = normalize_key(arg);

      if (next_arg && !is_long_option(*next_arg) && !is_short_option(*next_arg)) {
        // Next argument is the value
        return ParsedArgument{.key = key, .value = std::string(*next_arg), .is_flag = false};
      } else {
        // It's a flag
        return ParsedArgument{.key = key, .value = std::nullopt, .is_flag = true};
      }
    }
  } else if (is_short_option(arg)) {
    // Handle -k value
    arg.remove_prefix(1); // Remove "-"
    std::string key = normalize_key(arg);

    if (next_arg && !is_long_option(*next_arg) && !is_short_option(*next_arg)) {
      return ParsedArgument{.key = key, .value = std::string(*next_arg), .is_flag = false};
    } else {
      return ParsedArgument{.key = key, .value = std::nullopt, .is_flag = true};
    }
  }

  return std::nullopt;
}

bool ArgumentParser::is_long_option(std::string_view arg) {
  return arg.starts_with("--") && arg.length() > 2;
}

bool ArgumentParser::is_short_option(std::string_view arg) {
  return arg.starts_with("-") && arg.length() > 1 && !arg.starts_with("--");
}

std::string ArgumentParser::normalize_key(std::string_view key) {
  std::string result(key);
  std::replace(result.begin(), result.end(), '-', '.');
  return result;
}

} // namespace liarsdice::config