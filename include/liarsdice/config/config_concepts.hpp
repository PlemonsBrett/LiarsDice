#pragma once

/**
 * @file config_concepts.hpp
 * @brief Modern C++23 concepts for type-safe configuration system
 */

#include <concepts>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace liarsdice::config {

/**
 * @brief Concept for types that can be used as configuration values
 */
template <typename T>
concept ConfigValueType =
    requires {
      // Must be default constructible
      std::default_initializable<std::remove_cvref_t<T>>;
      // Must be copyable
      std::copyable<std::remove_cvref_t<T>>;
      // Must be equality comparable
      std::equality_comparable<std::remove_cvref_t<T>>;
    } &&
    (
        // Allow fundamental types
        std::is_fundamental_v<std::remove_cvref_t<T>> ||
        // Allow string types
        std::same_as<std::remove_cvref_t<T>, std::string> ||
        std::same_as<std::remove_cvref_t<T>, std::string_view> ||
        // Allow enums
        std::is_enum_v<std::remove_cvref_t<T>>);

/**
 * @brief Concept for types that can be converted from string
 */
template <typename T>
concept StringConvertible = ConfigValueType<T> && requires(const std::string &str) {
  { std::declval<T>() } -> std::convertible_to<T>;
};

/**
 * @brief Concept for configuration key types
 */
template <typename T>
concept ConfigKeyType = requires(const T &key) {
  { key } -> std::convertible_to<std::string_view>;
  std::totally_ordered<T>;
};

/**
 * @brief Concept for configuration source types
 */
template <typename T>
concept ConfigSource = requires(T &source, const std::string &key) {
  { source.has_value(key) } -> std::same_as<bool>;
  { source.get_raw_value(key) } -> std::same_as<std::optional<std::string>>;
  { source.get_all_keys() } -> std::ranges::range;
};

/**
 * @brief Concept for configuration validator types
 */
template <typename T, typename ValueType>
concept ValidatorType = requires(const T &validator, const ValueType &value) {
  { validator.validate(value) } -> std::same_as<bool>;
  { validator.get_error_message() } -> std::convertible_to<std::string>;
};

/**
 * @brief Concept for configuration serializer types
 */
template <typename T, typename ValueType>
concept ConfigSerializer =
    requires(const T &serializer, const ValueType &value, const std::string &str) {
      { serializer.serialize(value) } -> std::same_as<std::string>;
      { serializer.deserialize(str) } -> std::same_as<std::optional<ValueType>>;
    };

/**
 * @brief Concept for configuration change listener types
 */
template <typename T, typename ValueType>
concept ConfigChangeListener = requires(T &listener, const std::string &key,
                                        const ValueType &old_val, const ValueType &new_val) {
  { listener.on_config_changed(key, old_val, new_val) } -> std::same_as<void>;
};

/**
 * @brief Concept for hierarchical configuration path types
 */
template <typename T>
concept PathType = requires(const T &path) {
  { path.to_string() } -> std::same_as<std::string>;
  { path.parent() } -> std::same_as<std::optional<T>>;
  { path.append(std::string_view{}) } -> std::same_as<T>;
  { path.is_root() } -> std::same_as<bool>;
};

} // namespace liarsdice::config