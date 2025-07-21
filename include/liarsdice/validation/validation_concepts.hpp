#pragma once

/**
 * @file validation_concepts.hpp
 * @brief Modern C++23 concepts for input validation framework
 */

#include <concepts>
#include <expected>
#include <functional>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
// Note: std::format and std::source_location require proper C++23 support

namespace liarsdice::validation {

/**
 * @brief Error type for validation failures
 */
struct ValidationError {
  std::string field_name;
  std::string error_message;
  // std::source_location location{std::source_location::current()};

  [[nodiscard]] std::string to_string() const { return field_name + ": " + error_message; }
};

/**
 * @brief Collection of validation errors
 */
using ValidationErrors = std::vector<ValidationError>;

/**
 * @brief Result type for validation operations
 */
template <typename T> using ValidationResult = std::expected<T, ValidationErrors>;

/**
 * @brief Concept for types that can be validated
 */
template <typename T>
concept Validatable = requires {
  std::default_initializable<T>;
  std::copyable<T>;
};

/**
 * @brief Concept for validator functions
 */
template <typename V, typename T>
concept Validator = requires(const V &validator, const T &value) {
  { validator(value) } -> std::convertible_to<std::optional<ValidationError>>;
};

/**
 * @brief Concept for types that can be parsed from strings
 */
template <typename T>
concept StringParseable = requires(std::string_view str) {
  { T::parse(str) } -> std::same_as<std::optional<T>>;
};

/**
 * @brief Concept for input sanitizers
 */
template <typename S>
concept Sanitizer = requires(const S &sanitizer, std::string_view input) {
  { sanitizer(input) } -> std::convertible_to<std::string>;
};

/**
 * @brief Concept for types that support fuzzy matching
 */
template <typename T>
concept FuzzyMatchable = requires(const T &value, std::string_view pattern) {
  { value.fuzzy_match(pattern) } -> std::convertible_to<double>; // Returns similarity score [0, 1]
};

/**
 * @brief Concept for parser combinators
 */
template <typename P, typename T>
concept Parser = requires(const P &parser, std::string_view input) {
  typename P::value_type;
  {
    parser.parse(input)
  } -> std::same_as<std::expected<std::pair<T, std::string_view>, std::string>>;
};

/**
 * @brief Concept for composable validators
 */
template <typename V>
concept ComposableValidator = requires(const V &v1, const V &v2) {
  { v1 && v2 } -> std::same_as<V>; // AND composition
  { v1 || v2 } -> std::same_as<V>; // OR composition
  { !v1 } -> std::same_as<V>;      // NOT composition
};

/**
 * @brief Concept for range-based validation
 */
template <typename R, typename T>
concept ValidatableRange = std::ranges::input_range<R> && requires(R &&range) {
  typename std::ranges::range_value_t<R>;
  requires Validatable<std::ranges::range_value_t<R>>;
};

/**
 * @brief Concept for error aggregators
 */
template <typename A>
concept ErrorAggregator = requires(A &aggregator, const ValidationError &error) {
  { aggregator.add(error) } -> std::same_as<void>;
  { aggregator.get_errors() } -> std::convertible_to<ValidationErrors>;
  { aggregator.has_errors() } -> std::same_as<bool>;
  { aggregator.clear() } -> std::same_as<void>;
};

} // namespace liarsdice::validation