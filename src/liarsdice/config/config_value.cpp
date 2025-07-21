/**
 * @file config_value.cpp
 * @brief Implementation of type-safe configuration value system
 */

#include "liarsdice/config/config_value.hpp"
#include <algorithm>
#include <charconv>
#include <sstream>

namespace liarsdice::config {

std::string ConfigValue::to_string() const {
  if (!is_set_) {
    return "<unset>";
  }

  return std::visit(
      [](const auto &value) -> std::string {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::same_as<T, std::monostate>) {
          return "<null>";
        } else if constexpr (std::same_as<T, bool>) {
          return value ? "true" : "false";
        } else if constexpr (std::same_as<T, std::string>) {
          return value;
        } else if constexpr (std::same_as<T, std::vector<std::string>>) {
          std::ostringstream oss;
          oss << "[";
          for (size_t i = 0; i < value.size(); ++i) {
            if (i > 0)
              oss << ", ";
            oss << "\"" << value[i] << "\"";
          }
          oss << "]";
          return oss.str();
        } else {
          return std::to_string(value);
        }
      },
      value_);
}

std::optional<ConfigValue> ConfigValue::from_string(const std::string &str,
                                                    std::size_t target_type_index) {
  if (str.empty() || str == "<unset>" || str == "<null>") {
    return ConfigValue{};
  }

  try {
    switch (target_type_index) {
    case 0: // std::monostate
      return ConfigValue{};

    case 1: { // bool
      std::string lower_str = str;
      std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
      if (lower_str == "true" || lower_str == "1" || lower_str == "yes" || lower_str == "on") {
        return ConfigValue{true};
      } else if (lower_str == "false" || lower_str == "0" || lower_str == "no" ||
                 lower_str == "off") {
        return ConfigValue{false};
      }
      return std::nullopt;
    }

    case 2: { // int32_t
      int32_t value;
      auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      if (ec == std::errc{} && ptr == str.data() + str.size()) {
        return ConfigValue{value};
      }
      return std::nullopt;
    }

    case 3: { // int64_t
      int64_t value;
      auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      if (ec == std::errc{} && ptr == str.data() + str.size()) {
        return ConfigValue{value};
      }
      return std::nullopt;
    }

    case 4: { // uint32_t
      uint32_t value;
      auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      if (ec == std::errc{} && ptr == str.data() + str.size()) {
        return ConfigValue{value};
      }
      return std::nullopt;
    }

    case 5: { // uint64_t
      uint64_t value;
      auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      if (ec == std::errc{} && ptr == str.data() + str.size()) {
        return ConfigValue{value};
      }
      return std::nullopt;
    }

    case 6: { // double
      double value;
      auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      if (ec == std::errc{} && ptr == str.data() + str.size()) {
        return ConfigValue{value};
      }
      return std::nullopt;
    }

    case 7: { // std::string
      return ConfigValue{str};
    }

    case 8: { // std::vector<std::string>
      std::vector<std::string> result;
      if (str.front() == '[' && str.back() == ']') {
        std::string content = str.substr(1, str.length() - 2);
        std::istringstream iss(content);
        std::string item;

        while (std::getline(iss, item, ',')) {
          // Trim whitespace
          item.erase(0, item.find_first_not_of(" \t"));
          item.erase(item.find_last_not_of(" \t") + 1);

          // Remove quotes if present
          if (item.front() == '"' && item.back() == '"') {
            item = item.substr(1, item.length() - 2);
          }

          result.push_back(item);
        }
      }
      return ConfigValue{result};
    }

    default:
      return std::nullopt;
    }
  } catch (...) {
    return std::nullopt;
  }
}

ConfigPath::ConfigPath(std::string_view path) {
  if (path.empty()) {
    return; // Root path
  }

  std::string path_str{path};
  std::istringstream iss(path_str);
  std::string segment;

  while (std::getline(iss, segment, separator_)) {
    if (!segment.empty()) {
      segments_.push_back(segment);
    }
  }
}

ConfigPath::ConfigPath(std::vector<std::string> segments) : segments_(std::move(segments)) {}

std::string ConfigPath::to_string() const {
  if (segments_.empty()) {
    return "";
  }

  std::ostringstream oss;
  for (size_t i = 0; i < segments_.size(); ++i) {
    if (i > 0)
      oss << separator_;
    oss << segments_[i];
  }
  return oss.str();
}

std::optional<ConfigPath> ConfigPath::parent() const {
  if (segments_.empty()) {
    return std::nullopt; // Root has no parent
  }

  std::vector<std::string> parent_segments(segments_.begin(), segments_.end() - 1);
  return ConfigPath{std::move(parent_segments)};
}

ConfigPath ConfigPath::append(std::string_view segment) const {
  std::vector<std::string> new_segments = segments_;
  new_segments.emplace_back(segment);
  return ConfigPath{std::move(new_segments)};
}

} // namespace liarsdice::config