#pragma once

/**
 * @file format_helper.hpp
 * @brief Helper for string formatting until std::format is properly supported
 */

#include <sstream>
#include <string>

namespace liarsdice::utils {

template <typename... Args>
std::string format_string(const std::string &format, Args &&.../* args */) {
  // Simple implementation for now
  std::ostringstream oss;
  oss << format;
  // This is a placeholder - in production you'd want a proper implementation
  return oss.str();
}

// Specialized versions for common cases
inline std::string format_string(const std::string &format, const std::string &arg1) {
  auto pos = format.find("{}");
  if (pos != std::string::npos) {
    return format.substr(0, pos) + arg1 + format.substr(pos + 2);
  }
  return format;
}

inline std::string format_string(const std::string &format, int arg1) {
  return format_string(format, std::to_string(arg1));
}

inline std::string format_string(const std::string &format, const std::string &arg1,
                                 const std::string &arg2) {
  auto pos1 = format.find("{}");
  if (pos1 != std::string::npos) {
    auto temp = format.substr(0, pos1) + arg1 + format.substr(pos1 + 2);
    auto pos2 = temp.find("{}");
    if (pos2 != std::string::npos) {
      return temp.substr(0, pos2) + arg2 + temp.substr(pos2 + 2);
    }
    return temp;
  }
  return format;
}

} // namespace liarsdice::utils