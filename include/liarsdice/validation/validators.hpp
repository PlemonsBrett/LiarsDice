#pragma once

/**
 * @file validators.hpp
 * @brief Composable validators for input validation
 */

#include "validation_concepts.hpp"
#include <algorithm>
#include <charconv>
#include <liarsdice/utils/format_helper.hpp>
#include <numeric>
#include <regex>
#include <sstream>

namespace liarsdice::validation {

/**
 * @brief Base validator template for composable validation
 */
template <typename T> class ValidatorBase {
public:
  using ValueType = T;
  using ValidatorFunc = std::function<std::optional<ValidationError>(const T &)>;

  explicit ValidatorBase(ValidatorFunc func, std::string name = "")
      : validator_(std::move(func)), name_(std::move(name)) {}

  /**
   * @brief Validate a value
   */
  [[nodiscard]] std::optional<ValidationError> operator()(const T &value) const {
    return validator_(value);
  }

  /**
   * @brief Validate and return result
   */
  [[nodiscard]] ValidationResult<T> validate(const T &value) const {
    if (auto error = validator_(value)) {
      return std::unexpected(ValidationErrors{*error});
    }
    return value;
  }

  /**
   * @brief AND composition
   */
  [[nodiscard]] ValidatorBase<T> operator&&(const ValidatorBase<T> &other) const {
    return ValidatorBase<T>(
        [*this, other](const T &value) -> std::optional<ValidationError> {
          if (auto error = (*this)(value))
            return error;
          return other(value);
        },
        liarsdice::utils::format_string("{} && {}", name_, other.name_));
  }

  /**
   * @brief OR composition
   */
  [[nodiscard]] ValidatorBase<T> operator||(const ValidatorBase<T> &other) const {
    return ValidatorBase<T>(
        [*this, other](const T &value) -> std::optional<ValidationError> {
          auto error1 = (*this)(value);
          auto error2 = other(value);
          if (!error1 || !error2)
            return std::nullopt;
          return ValidationError{
              .field_name = liarsdice::utils::format_string("{} || {}", name_, other.name_),
              .error_message = "Both validations failed: " + error1->error_message + " and " +
                               error2->error_message};
        },
        liarsdice::utils::format_string("{} || {}", name_, other.name_));
  }

  /**
   * @brief NOT composition
   */
  [[nodiscard]] ValidatorBase<T> operator!() const {
    return ValidatorBase<T>(
        [*this](const T &value) -> std::optional<ValidationError> {
          if (!(*this)(value)) {
            return ValidationError{.field_name = liarsdice::utils::format_string("!{}", name_),
                                   .error_message = "Validation should have failed but passed"};
          }
          return std::nullopt;
        },
        liarsdice::utils::format_string("!{}", name_));
  }

  [[nodiscard]] const std::string &name() const { return name_; }

private:
  ValidatorFunc validator_;
  std::string name_;
};

/**
 * @brief Factory functions for common validators
 */
namespace validators {

/**
 * @brief Range validator
 */
template <typename T>
requires std::totally_ordered<T>
[[nodiscard]] auto range(T min_val, T max_val, std::string field_name = "value") {
  return ValidatorBase<T>(
      [=](const T &value) -> std::optional<ValidationError> {
        if (value < min_val || value > max_val) {
          return ValidationError{.field_name = field_name,
                                 .error_message = "Must be between " + std::to_string(min_val) +
                                                  " and " + std::to_string(max_val)};
        }
        return std::nullopt;
      },
      "range(" + std::to_string(min_val) + ", " + std::to_string(max_val) + ")");
}

/**
 * @brief Minimum value validator
 */
template <typename T>
requires std::totally_ordered<T>
[[nodiscard]] auto min(T min_val, std::string field_name = "value") {
  return ValidatorBase<T>(
      [=](const T &value) -> std::optional<ValidationError> {
        if (value < min_val) {
          return ValidationError{.field_name = field_name,
                                 .error_message = "Must be at least " + std::to_string(min_val)};
        }
        return std::nullopt;
      },
      "min(" + std::to_string(min_val) + ")");
}

/**
 * @brief Maximum value validator
 */
template <typename T>
requires std::totally_ordered<T>
[[nodiscard]] auto max(T max_val, std::string field_name = "value") {
  return ValidatorBase<T>(
      [=](const T &value) -> std::optional<ValidationError> {
        if (value > max_val) {
          return ValidationError{.field_name = field_name,
                                 .error_message = "Must be at most " + std::to_string(max_val)};
        }
        return std::nullopt;
      },
      "max(" + std::to_string(max_val) + ")");
}

/**
 * @brief String length validator
 */
[[nodiscard]] inline auto length(std::size_t min_len, std::size_t max_len,
                                 std::string field_name = "string") {
  return ValidatorBase<std::string>(
      [=](const std::string &value) -> std::optional<ValidationError> {
        if (value.length() < min_len || value.length() > max_len) {
          return ValidationError{.field_name = field_name,
                                 .error_message = "Length must be between " +
                                                  std::to_string(min_len) + " and " +
                                                  std::to_string(max_len) + " characters"};
        }
        return std::nullopt;
      },
      "length(" + std::to_string(min_len) + ", " + std::to_string(max_len) + ")");
}

/**
 * @brief Non-empty string validator
 */
[[nodiscard]] inline auto non_empty(std::string field_name = "string") {
  return ValidatorBase<std::string>(
      [=](const std::string &value) -> std::optional<ValidationError> {
        if (value.empty()) {
          return ValidationError{.field_name = field_name, .error_message = "Cannot be empty"};
        }
        return std::nullopt;
      },
      "non_empty");
}

/**
 * @brief Pattern/regex validator
 */
[[nodiscard]] inline auto pattern(const std::string &regex_pattern,
                                  std::string field_name = "string") {
  return ValidatorBase<std::string>(
      [=](const std::string &value) -> std::optional<ValidationError> {
        std::regex re(regex_pattern);
        if (!std::regex_match(value, re)) {
          return ValidationError{.field_name = field_name,
                                 .error_message = "Must match pattern: " + regex_pattern};
        }
        return std::nullopt;
      },
      "pattern(" + regex_pattern + ")");
}

/**
 * @brief One-of validator
 */
template <typename T>
[[nodiscard]] auto one_of(std::initializer_list<T> valid_values, std::string field_name = "value") {
  std::vector<T> values(valid_values);
  return ValidatorBase<T>(
      [=](const T &value) -> std::optional<ValidationError> {
        if (std::ranges::find(values, value) == values.end()) {
          std::string valid_list;
          for (const auto &v : values) {
            if (!valid_list.empty())
              valid_list += ", ";
            if constexpr (std::is_same_v<T, std::string>) {
              valid_list += v;
            } else if constexpr (std::is_arithmetic_v<T>) {
              valid_list += std::to_string(v);
            } else {
              std::ostringstream oss;
              oss << v;
              valid_list += oss.str();
            }
          }
          return ValidationError{.field_name = field_name,
                                 .error_message = "Must be one of: " + valid_list};
        }
        return std::nullopt;
      },
      "one_of");
}

/**
 * @brief Custom predicate validator
 */
template <typename T>
[[nodiscard]] auto predicate(std::function<bool(const T &)> pred, std::string error_msg,
                             std::string field_name = "value") {
  return ValidatorBase<T>(
      [=](const T &value) -> std::optional<ValidationError> {
        if (!pred(value)) {
          return ValidationError{.field_name = field_name, .error_message = error_msg};
        }
        return std::nullopt;
      },
      "predicate");
}

/**
 * @brief Numeric string validator
 */
[[nodiscard]] inline auto numeric(std::string field_name = "string") {
  return ValidatorBase<std::string>(
      [=](const std::string &value) -> std::optional<ValidationError> {
        if (value.empty() || !std::ranges::all_of(value, [](char c) { return std::isdigit(c); })) {
          return ValidationError{.field_name = field_name,
                                 .error_message = "Must contain only numeric characters"};
        }
        return std::nullopt;
      },
      "numeric");
}

/**
 * @brief Alpha string validator
 */
[[nodiscard]] inline auto alpha(std::string field_name = "string") {
  return ValidatorBase<std::string>(
      [=](const std::string &value) -> std::optional<ValidationError> {
        if (value.empty() || !std::ranges::all_of(value, [](char c) { return std::isalpha(c); })) {
          return ValidationError{.field_name = field_name,
                                 .error_message = "Must contain only alphabetic characters"};
        }
        return std::nullopt;
      },
      "alpha");
}

/**
 * @brief Alphanumeric string validator
 */
[[nodiscard]] inline auto alphanumeric(std::string field_name = "string") {
  return ValidatorBase<std::string>(
      [=](const std::string &value) -> std::optional<ValidationError> {
        if (value.empty() || !std::ranges::all_of(value, [](char c) { return std::isalnum(c); })) {
          return ValidationError{.field_name = field_name,
                                 .error_message = "Must contain only alphanumeric characters"};
        }
        return std::nullopt;
      },
      "alphanumeric");
}

} // namespace validators

/**
 * @brief Validate a range of values
 */
template <ValidatableRange<ValidationErrors> R, typename V>
requires Validator<V, std::ranges::range_value_t<R>>
[[nodiscard]] ValidationErrors validate_all(R &&range, const V &validator) {
  ValidationErrors errors;

  for (const auto &value : range) {
    if (auto error = validator(value)) {
      errors.push_back(*error);
    }
  }

  return errors;
}

} // namespace liarsdice::validation