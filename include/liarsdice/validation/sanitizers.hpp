#pragma once

/**
 * @file sanitizers.hpp
 * @brief Input sanitization using std::string_view and ranges algorithms
 */

#include "validation_concepts.hpp"
#include <algorithm>
#include <cctype>
#include <liarsdice/utils/format_helper.hpp>
#include <locale>
#include <ranges>
#include <string>
#include <string_view>
// #include <unicode/unistr.h>  // Optional: for Unicode normalization

namespace liarsdice::validation {

/**
 * @brief Base sanitizer class
 */
class SanitizerBase {
public:
  using sanitizer_func = std::function<std::string(std::string_view)>;

  explicit SanitizerBase(sanitizer_func func, std::string name = "sanitizer")
      : sanitizer_(std::move(func)), name_(std::move(name)) {}

  /**
   * @brief Apply sanitization
   */
  [[nodiscard]] std::string operator()(std::string_view input) const { return sanitizer_(input); }

  /**
   * @brief Chain sanitizers
   */
  [[nodiscard]] SanitizerBase then(const SanitizerBase &next) const {
    return SanitizerBase([*this, next](std::string_view input) { return next((*this)(input)); },
                         liarsdice::utils::format_string("{} >> {}", name_, next.name_));
  }

  [[nodiscard]] const std::string &name() const { return name_; }

private:
  sanitizer_func sanitizer_;
  std::string name_;
};

/**
 * @brief Sanitizer factory functions
 */
namespace sanitizers {

/**
 * @brief Trim whitespace from both ends
 */
[[nodiscard]] inline auto trim() {
  return SanitizerBase(
      [](std::string_view input) -> std::string {
        // Find first non-whitespace
        auto start = std::ranges::find_if_not(
            input, [](char c) { return std::isspace(static_cast<unsigned char>(c)); });

        // Find last non-whitespace
        auto rstart = std::ranges::find_if_not(input | std::views::reverse, [](char c) {
          return std::isspace(static_cast<unsigned char>(c));
        });

        if (start == input.end()) {
          return ""; // All whitespace
        }

        auto end = rstart.base();
        return std::string(start, end);
      },
      "trim");
}

/**
 * @brief Trim specific characters from both ends
 */
[[nodiscard]] inline auto trim_chars(std::string_view chars_to_trim) {
  std::string chars(chars_to_trim);
  return SanitizerBase(
      [chars](std::string_view input) -> std::string {
        // Find first character not in trim set
        auto start = std::ranges::find_if_not(
            input, [&chars](char c) { return chars.find(c) != std::string::npos; });

        // Find last character not in trim set
        auto rstart = std::ranges::find_if_not(input | std::views::reverse, [&chars](char c) {
          return chars.find(c) != std::string::npos;
        });

        if (start == input.end()) {
          return ""; // All characters should be trimmed
        }

        auto end = rstart.base();
        return std::string(start, end);
      },
      std::string("trim_chars(\"") + std::string(chars_to_trim) + "\")");
}

/**
 * @brief Convert to lowercase
 */
[[nodiscard]] inline auto lowercase() {
  return SanitizerBase(
      [](std::string_view input) -> std::string {
        std::string result;
        result.reserve(input.size());

        std::ranges::transform(input, std::back_inserter(result),
                               [](char c) { return std::tolower(static_cast<unsigned char>(c)); });

        return result;
      },
      "lowercase");
}

/**
 * @brief Convert to uppercase
 */
[[nodiscard]] inline auto uppercase() {
  return SanitizerBase(
      [](std::string_view input) -> std::string {
        std::string result;
        result.reserve(input.size());

        std::ranges::transform(input, std::back_inserter(result),
                               [](char c) { return std::toupper(static_cast<unsigned char>(c)); });

        return result;
      },
      "uppercase");
}

/**
 * @brief Remove all whitespace
 */
[[nodiscard]] inline auto remove_whitespace() {
  return SanitizerBase(
      [](std::string_view input) -> std::string {
        std::string result;
        result.reserve(input.size());

        std::ranges::copy_if(input, std::back_inserter(result),
                             [](char c) { return !std::isspace(static_cast<unsigned char>(c)); });

        return result;
      },
      "remove_whitespace");
}

/**
 * @brief Collapse multiple whitespaces to single space
 */
[[nodiscard]] inline auto collapse_whitespace() {
  return SanitizerBase(
      [](std::string_view input) -> std::string {
        std::string result;
        result.reserve(input.size());

        bool prev_was_space = false;
        for (char c : input) {
          if (std::isspace(static_cast<unsigned char>(c))) {
            if (!prev_was_space && !result.empty()) {
              result.push_back(' ');
            }
            prev_was_space = true;
          } else {
            result.push_back(c);
            prev_was_space = false;
          }
        }

        // Trim trailing space if any
        if (!result.empty() && result.back() == ' ') {
          result.pop_back();
        }

        return result;
      },
      "collapse_whitespace");
}

/**
 * @brief Keep only specified characters
 */
[[nodiscard]] inline auto keep_chars(std::string_view allowed_chars) {
  std::string allowed(allowed_chars);
  return SanitizerBase(
      [allowed](std::string_view input) -> std::string {
        std::string result;
        result.reserve(input.size());

        std::ranges::copy_if(input, std::back_inserter(result),
                             [&allowed](char c) { return allowed.find(c) != std::string::npos; });

        return result;
      },
      std::string("keep_chars(\"") + std::string(allowed_chars) + "\")");
}

/**
 * @brief Remove specified characters
 */
[[nodiscard]] inline auto remove_chars(std::string_view chars_to_remove) {
  std::string remove(chars_to_remove);
  return SanitizerBase(
      [remove](std::string_view input) -> std::string {
        std::string result;
        result.reserve(input.size());

        std::ranges::copy_if(input, std::back_inserter(result),
                             [&remove](char c) { return remove.find(c) == std::string::npos; });

        return result;
      },
      std::string("remove_chars(\"") + std::string(chars_to_remove) + "\")");
}

/**
 * @brief Keep only alphanumeric characters
 */
[[nodiscard]] inline auto alphanumeric_only() {
  return SanitizerBase(
      [](std::string_view input) -> std::string {
        std::string result;
        result.reserve(input.size());

        std::ranges::copy_if(input, std::back_inserter(result),
                             [](char c) { return std::isalnum(static_cast<unsigned char>(c)); });

        return result;
      },
      "alphanumeric_only");
}

/**
 * @brief Replace characters
 */
[[nodiscard]] inline auto replace_chars(std::string_view from, std::string_view to) {
  if (from.size() != to.size()) {
    throw std::invalid_argument("replace_chars: 'from' and 'to' must have same length");
  }

  std::string from_str(from);
  std::string to_str(to);

  return SanitizerBase(
      [from_str, to_str](std::string_view input) -> std::string {
        std::string result;
        result.reserve(input.size());

        for (char c : input) {
          auto pos = from_str.find(c);
          if (pos != std::string::npos) {
            result.push_back(to_str[pos]);
          } else {
            result.push_back(c);
          }
        }

        return result;
      },
      std::string("replace_chars(\"") + std::string(from) + "\", \"" + std::string(to) + "\")");
}

/**
 * @brief Escape HTML special characters
 */
[[nodiscard]] inline auto escape_html() {
  return SanitizerBase(
      [](std::string_view input) -> std::string {
        std::string result;
        result.reserve(static_cast<std::size_t>(input.size() * 1.5)); // Estimate

        for (char c : input) {
          switch (c) {
          case '&':
            result.append("&amp;");
            break;
          case '<':
            result.append("&lt;");
            break;
          case '>':
            result.append("&gt;");
            break;
          case '"':
            result.append("&quot;");
            break;
          case '\'':
            result.append("&#39;");
            break;
          default:
            result.push_back(c);
            break;
          }
        }

        return result;
      },
      "escape_html");
}

/**
 * @brief Limit string length
 */
[[nodiscard]] inline auto truncate(std::size_t max_length, std::string_view suffix = "...") {
  std::string suffix_str(suffix);
  return SanitizerBase(
      [max_length, suffix_str](std::string_view input) -> std::string {
        if (input.size() <= max_length) {
          return std::string(input);
        }

        if (max_length < suffix_str.size()) {
          return std::string(input.substr(0, max_length));
        }

        return std::string(input.substr(0, max_length - suffix_str.size())) + suffix_str;
      },
      "truncate(" + std::to_string(max_length) + ")");
}

/**
 * @brief Normalize Unicode (requires ICU library)
 */
#ifdef HAS_ICU
[[nodiscard]] inline auto normalize_unicode() {
  return SanitizerBase(
      [](std::string_view input) -> std::string {
        icu::UnicodeString ustr =
            icu::UnicodeString::fromUTF8(icu::StringPiece(input.data(), input.size()));
        icu::UnicodeString normalized;

        UErrorCode status = U_ZERO_ERROR;
        icu::Normalizer::normalize(ustr, UNORM_NFC, 0, normalized, status);

        if (U_FAILURE(status)) {
          return std::string(input); // Return original on error
        }

        std::string result;
        normalized.toUTF8String(result);
        return result;
      },
      "normalize_unicode");
}
#endif

/**
 * @brief Remove control characters
 */
[[nodiscard]] inline auto remove_control_chars() {
  return SanitizerBase(
      [](std::string_view input) -> std::string {
        std::string result;
        result.reserve(input.size());

        std::ranges::copy_if(input, std::back_inserter(result), [](char c) {
          unsigned char uc = static_cast<unsigned char>(c);
          return !std::iscntrl(uc) || uc == '\t' || uc == '\n' || uc == '\r';
        });

        return result;
      },
      "remove_control_chars");
}

/**
 * @brief Sanitize for filename use
 */
[[nodiscard]] inline auto filename_safe() {
  return SanitizerBase(
      [](std::string_view input) -> std::string {
        std::string result;
        result.reserve(input.size());

        const std::string_view invalid_chars{R"(<>:"/\|?*)"};

        for (char c : input) {
          if (invalid_chars.find(c) != std::string_view::npos) {
            result.push_back('_');
          } else if (std::iscntrl(static_cast<unsigned char>(c))) {
            result.push_back('_');
          } else {
            result.push_back(c);
          }
        }

        // Remove leading/trailing dots and spaces
        while (!result.empty() && (result.front() == '.' || result.front() == ' ')) {
          result.erase(0, 1);
        }
        while (!result.empty() && (result.back() == '.' || result.back() == ' ')) {
          result.pop_back();
        }

        return result.empty() ? "unnamed" : result;
      },
      "filename_safe");
}

} // namespace sanitizers

/**
 * @brief Chain multiple sanitizers
 */
template <typename First, typename... Rest>
requires Sanitizer<First> && (Sanitizer<Rest> && ...)
[[nodiscard]] auto chain(First first, Rest... rest) {
  if constexpr (sizeof...(rest) == 0) {
    return first;
  } else {
    return first.then(chain(rest...));
  }
}

} // namespace liarsdice::validation