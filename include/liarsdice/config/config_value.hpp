#pragma once

/**
 * @file config_value.hpp
 * @brief Type-safe configuration value system using std::variant and std::optional
 */

#include "config_concepts.hpp"
#include <variant>
#include <optional>
#include <string>
#include <vector>
#include <map>
#include <source_location>
#include <stdexcept>
#include <format>
#include <functional>

namespace liarsdice::config {

/**
 * @brief Exception thrown when configuration operations fail
 */
class ConfigException : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message, 
                           std::source_location location = std::source_location::current())
        : std::runtime_error(std::format("Config error at {}:{}: {}", 
                                        location.file_name(), 
                                        location.line(), 
                                        message)) {}
};

/**
 * @brief Type-safe configuration value using std::variant
 */
using ConfigVariant = std::variant<
    std::monostate,     // Represents null/unset value
    bool,
    int32_t,
    int64_t,
    uint32_t,
    uint64_t,
    double,
    std::string,
    std::vector<std::string>
>;

/**
 * @brief Wrapper for configuration values with metadata
 */
class ConfigValue {
public:
    /**
     * @brief Default constructor creates unset value
     */
    constexpr ConfigValue() noexcept = default;

    /**
     * @brief Constructor from any supported type
     */
    template<ConfigValueType T>
    constexpr ConfigValue(T&& value) noexcept
        : value_(std::forward<T>(value)), is_set_(true) {}

    /**
     * @brief Constructor with default value
     */
    template<ConfigValueType T>
    constexpr ConfigValue(T&& value, T&& default_value) noexcept
        : value_(std::forward<T>(value)), default_value_(std::forward<T>(default_value)), is_set_(true) {}

    /**
     * @brief Get the value with type checking
     */
    template<typename T>
    constexpr std::optional<T> get() const noexcept {
        if (!is_set_) {
            return std::nullopt;
        }
        
        if (auto* ptr = std::get_if<T>(&value_)) {
            return *ptr;
        }
        return std::nullopt;
    }

    /**
     * @brief Get the value or default
     */
    template<typename T>
    constexpr T get_or(const T& default_val) const noexcept {
        if (auto val = get<T>()) {
            return *val;
        }
        return default_val;
    }

    /**
     * @brief Get the value or throw exception
     */
    template<typename T>
    constexpr T get_required() const {
        if (auto val = get<T>()) {
            return *val;
        }
        throw ConfigException(std::format("Required configuration value of type '{}' not found", 
                                        typeid(T).name()));
    }

    /**
     * @brief Check if value is set
     */
    constexpr bool is_set() const noexcept { return is_set_; }

    /**
     * @brief Check if value is of specific type
     */
    template<typename T>
    constexpr bool is_type() const noexcept {
        return std::holds_alternative<T>(value_);
    }

    /**
     * @brief Get the type index of stored value
     */
    constexpr std::size_t type_index() const noexcept {
        return value_.index();
    }

    /**
     * @brief Set new value
     */
    template<ConfigValueType T>
    constexpr void set(T&& new_value) noexcept {
        value_ = std::forward<T>(new_value);
        is_set_ = true;
    }

    /**
     * @brief Reset to unset state
     */
    constexpr void reset() noexcept {
        value_ = std::monostate{};
        is_set_ = false;
    }

    /**
     * @brief Get string representation
     */
    std::string to_string() const;

    /**
     * @brief Parse from string representation
     */
    static std::optional<ConfigValue> from_string(const std::string& str, std::size_t target_type_index);

    /**
     * @brief Equality comparison
     */
    constexpr bool operator==(const ConfigValue& other) const noexcept {
        return value_ == other.value_ && is_set_ == other.is_set_;
    }

private:
    ConfigVariant value_{};
    std::optional<ConfigVariant> default_value_{};
    bool is_set_{false};
};

/**
 * @brief Configuration path for hierarchical access
 */
class ConfigPath {
public:
    /**
     * @brief Constructor from string path
     */
    explicit ConfigPath(std::string_view path = "");

    /**
     * @brief Constructor from vector of segments
     */
    explicit ConfigPath(std::vector<std::string> segments);

    /**
     * @brief Get string representation
     */
    std::string to_string() const;

    /**
     * @brief Get parent path
     */
    std::optional<ConfigPath> parent() const;

    /**
     * @brief Append segment to path
     */
    ConfigPath append(std::string_view segment) const;

    /**
     * @brief Check if this is root path
     */
    bool is_root() const noexcept { return segments_.empty(); }

    /**
     * @brief Get path segments
     */
    const std::vector<std::string>& segments() const noexcept { return segments_; }

    /**
     * @brief Comparison operators
     */
    auto operator<=>(const ConfigPath& other) const = default;

private:
    std::vector<std::string> segments_;
    static constexpr char separator_ = '.';
};

/**
 * @brief Type-safe configuration validator
 */
template<typename T>
class ConfigValidator {
public:
    using ValidatorFunc = std::function<bool(const T&)>;
    using ErrorMessageFunc = std::function<std::string()>;

    /**
     * @brief Constructor with validation function
     */
    ConfigValidator(ValidatorFunc validator, ErrorMessageFunc error_message)
        : validator_(std::move(validator)), error_message_(std::move(error_message)) {}

    /**
     * @brief Validate value
     */
    bool validate(const T& value) const {
        last_valid_ = validator_(value);
        return last_valid_;
    }

    /**
     * @brief Get error message for last validation
     */
    std::string get_error_message() const {
        return last_valid_ ? "" : error_message_();
    }

private:
    ValidatorFunc validator_;
    ErrorMessageFunc error_message_;
    mutable bool last_valid_{true};
};

/**
 * @brief Range validator for numeric types
 */
template<typename T>
    requires std::totally_ordered<T>
constexpr auto make_range_validator(T min_val, T max_val) {
    return ConfigValidator<T>(
        [min_val, max_val](const T& value) { return value >= min_val && value <= max_val; },
        [min_val, max_val]() { return std::format("Value must be between {} and {}", min_val, max_val); }
    );
}

/**
 * @brief Enum validator
 */
template<typename E>
    requires std::is_enum_v<E>
constexpr auto make_enum_validator(std::initializer_list<E> valid_values) {
    return ConfigValidator<E>(
        [valid_values](const E& value) {
            return std::find(valid_values.begin(), valid_values.end(), value) != valid_values.end();
        },
        []() { return "Invalid enum value"; }
    );
}

} // namespace liarsdice::config