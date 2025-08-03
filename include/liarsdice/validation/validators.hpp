#pragma once

#include <string>
#include <vector>
#include <functional>
#include <regex>
#include <stdexcept>
#include <concepts>
#include <type_traits>

namespace liarsdice::validation {

// Validation result
struct ValidationResult {
    bool is_valid = true;
    std::vector<std::string> errors;
    
    explicit operator bool() const { return is_valid; }
    
    void add_error(const std::string& error) {
        is_valid = false;
        errors.push_back(error);
    }
    
    ValidationResult& operator+=(const ValidationResult& other) {
        if (!other.is_valid) {
            is_valid = false;
            errors.insert(errors.end(), other.errors.begin(), other.errors.end());
        }
        return *this;
    }
};

// Base validator interface
template<typename T>
class IValidator {
public:
    virtual ~IValidator() = default;
    virtual ValidationResult validate(const T& value) const = 0;
    virtual std::string get_error_message() const = 0;
};

// Range validator for numeric types
template<typename T>
requires std::is_arithmetic_v<T>
class RangeValidator : public IValidator<T> {
public:
    RangeValidator(T min, T max, const std::string& error_msg = "")
        : min_(min), max_(max), error_message_(error_msg) {
        if (error_message_.empty()) {
            error_message_ = "Value must be between " + std::to_string(min) + 
                            " and " + std::to_string(max);
        }
    }
    
    ValidationResult validate(const T& value) const override {
        ValidationResult result;
        if (value < min_ || value > max_) {
            result.add_error(error_message_);
        }
        return result;
    }
    
    std::string get_error_message() const override { return error_message_; }

private:
    T min_, max_;
    std::string error_message_;
};

// String length validator
class StringLengthValidator : public IValidator<std::string> {
public:
    StringLengthValidator(size_t min_length, size_t max_length, 
                         const std::string& error_msg = "")
        : min_length_(min_length), max_length_(max_length), error_message_(error_msg) {
        if (error_message_.empty()) {
            error_message_ = "String length must be between " + std::to_string(min_length) + 
                            " and " + std::to_string(max_length) + " characters";
        }
    }
    
    ValidationResult validate(const std::string& value) const override {
        ValidationResult result;
        if (value.length() < min_length_ || value.length() > max_length_) {
            result.add_error(error_message_);
        }
        return result;
    }
    
    std::string get_error_message() const override { return error_message_; }

private:
    size_t min_length_, max_length_;
    std::string error_message_;
};

// Regex validator
class RegexValidator : public IValidator<std::string> {
public:
    RegexValidator(const std::string& pattern, const std::string& error_msg = "")
        : regex_(pattern), error_message_(error_msg) {
        if (error_message_.empty()) {
            error_message_ = "Value does not match required pattern";
        }
    }
    
    ValidationResult validate(const std::string& value) const override {
        ValidationResult result;
        if (!std::regex_match(value, regex_)) {
            result.add_error(error_message_);
        }
        return result;
    }
    
    std::string get_error_message() const override { return error_message_; }

private:
    std::regex regex_;
    std::string error_message_;
};

// Custom function validator
template<typename T>
class FunctionValidator : public IValidator<T> {
public:
    using ValidatorFunction = std::function<bool(const T&)>;
    
    FunctionValidator(ValidatorFunction func, const std::string& error_msg)
        : validator_func_(std::move(func)), error_message_(error_msg) {}
    
    ValidationResult validate(const T& value) const override {
        ValidationResult result;
        if (!validator_func_(value)) {
            result.add_error(error_message_);
        }
        return result;
    }
    
    std::string get_error_message() const override { return error_message_; }

private:
    ValidatorFunction validator_func_;
    std::string error_message_;
};

// Composite validator
template<typename T>
class CompositeValidator : public IValidator<T> {
public:
    void add_validator(std::unique_ptr<IValidator<T>> validator) {
        validators_.push_back(std::move(validator));
    }
    
    ValidationResult validate(const T& value) const override {
        ValidationResult result;
        for (const auto& validator : validators_) {
            result += validator->validate(value);
        }
        return result;
    }
    
    std::string get_error_message() const override {
        return "Composite validation failed";
    }

private:
    std::vector<std::unique_ptr<IValidator<T>>> validators_;
};

// Factory functions for common validators
namespace factory {

    // Dice-specific validators
    inline auto dice_value_validator() {
        return std::make_unique<RangeValidator<unsigned int>>(
            1, 6, "Dice value must be between 1 and 6");
    }
    
    inline auto dice_count_validator(unsigned int max_dice = 50) {
        return std::make_unique<RangeValidator<unsigned int>>(
            1, max_dice, "Dice count must be between 1 and " + std::to_string(max_dice));
    }
    
    inline auto player_count_validator() {
        return std::make_unique<RangeValidator<unsigned int>>(
            2, 8, "Player count must be between 2 and 8");
    }
    
    // String validators
    inline auto player_name_validator() {
        auto composite = std::make_unique<CompositeValidator<std::string>>();
        composite->add_validator(std::make_unique<StringLengthValidator>(
            1, 20, "Player name must be 1-20 characters"));
        composite->add_validator(std::make_unique<RegexValidator>(
            R"(^[A-Za-z0-9 ]+$)", "Player name can only contain letters, numbers, and spaces"));
        return composite;
    }
    
    inline auto file_path_validator() {
        return std::make_unique<RegexValidator>(
            R"(^[^<>:"|?*]+$)", "Invalid file path characters");
    }
    
    // Game-specific validators
    inline auto guess_validator(unsigned int total_dice) {
        return std::make_unique<FunctionValidator<unsigned int>>(
            [total_dice](unsigned int guess) { return guess <= total_dice; },
            "Guess cannot exceed total number of dice in play");
    }

} // namespace factory

// Validation exception
class ValidationException : public std::runtime_error {
public:
    ValidationException(const ValidationResult& result)
        : std::runtime_error(format_errors(result)), result_(result) {}
    
    const ValidationResult& get_result() const { return result_; }

private:
    ValidationResult result_;
    
    static std::string format_errors(const ValidationResult& result) {
        std::string message = "Validation failed:";
        for (const auto& error : result.errors) {
            message += "\n  - " + error;
        }
        return message;
    }
};

// Validation helper functions
template<typename T>
void validate_and_throw(const T& value, const IValidator<T>& validator) {
    auto result = validator.validate(value);
    if (!result) {
        throw ValidationException(result);
    }
}

template<typename T>
bool is_valid(const T& value, const IValidator<T>& validator) {
    return validator.validate(value).is_valid;
}

} // namespace liarsdice::validation