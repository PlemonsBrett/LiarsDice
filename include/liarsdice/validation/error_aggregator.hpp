#pragma once

/**
 * @file error_aggregator.hpp
 * @brief Validation error aggregation with custom error types
 */

#include "validation_concepts.hpp"
#include <algorithm>
#include <iomanip>
#include <set>
#include <sstream>
#include <unordered_map>

namespace liarsdice::validation {

/**
 * @brief Error severity levels
 */
enum class ErrorSeverity { Warning, Error, Critical };

/**
 * @brief Extended validation error with severity
 */
struct ExtendedValidationError : ValidationError {
  ErrorSeverity severity{ErrorSeverity::Error};
  std::optional<std::string> suggestion;
  std::optional<std::string> code; // Error code for i18n

  [[nodiscard]] std::string to_string() const {
    std::ostringstream oss;

    switch (severity) {
    case ErrorSeverity::Warning:
      oss << "[WARNING] ";
      break;
    case ErrorSeverity::Error:
      oss << "[ERROR] ";
      break;
    case ErrorSeverity::Critical:
      oss << "[CRITICAL] ";
      break;
    }

    oss << field_name << ": " << error_message;

    if (code) {
      oss << " (code: " << *code << ")";
    }

    if (suggestion) {
      oss << "\n  Suggestion: " << *suggestion;
    }

    return oss.str();
  }
};

/**
 * @brief Validation error aggregator
 */
class ValidationErrorAggregator {
public:
  /**
   * @brief Add a validation error
   */
  void add(const ValidationError &error) {
    errors_.emplace_back(ExtendedValidationError{.field_name = error.field_name,
                                                 .error_message = error.error_message,
                                                 // .location = error.location,
                                                 .severity = ErrorSeverity::Error});

    field_errors_[error.field_name].push_back(errors_.size() - 1);
  }

  /**
   * @brief Add an extended validation error
   */
  void add(const ExtendedValidationError &error) {
    errors_.push_back(error);
    field_errors_[error.field_name].push_back(errors_.size() - 1);
  }

  /**
   * @brief Add a warning
   */
  void add_warning(const std::string &field_name, const std::string &message,
                   const std::optional<std::string> &suggestion = std::nullopt) {
    add(ExtendedValidationError{.field_name = field_name,
                                .error_message = message,
                                .severity = ErrorSeverity::Warning,
                                .suggestion = suggestion});
  }

  /**
   * @brief Add a critical error
   */
  void add_critical(const std::string &field_name, const std::string &message,
                    const std::optional<std::string> &code = std::nullopt) {
    add(ExtendedValidationError{.field_name = field_name,
                                .error_message = message,
                                .severity = ErrorSeverity::Critical,
                                .code = code});
  }

  /**
   * @brief Merge errors from another aggregator
   */
  void merge(const ValidationErrorAggregator &other) {
    for (const auto &error : other.errors_) {
      add(error);
    }
  }

  /**
   * @brief Get all errors
   */
  [[nodiscard]] ValidationErrors get_errors() const {
    ValidationErrors result;
    result.reserve(errors_.size());

    std::ranges::transform(errors_, std::back_inserter(result),
                           [](const ExtendedValidationError &e) -> ValidationError {
                             return ValidationError{.field_name = e.field_name,
                                                    .error_message = e.error_message,
                                                    .location = e.location};
                           });

    return result;
  }

  /**
   * @brief Get extended errors
   */
  [[nodiscard]] const std::vector<ExtendedValidationError> &get_extended_errors() const {
    return errors_;
  }

  /**
   * @brief Get errors for a specific field
   */
  [[nodiscard]] std::vector<ExtendedValidationError>
  get_field_errors(const std::string &field_name) const {
    std::vector<ExtendedValidationError> result;

    if (auto it = field_errors_.find(field_name); it != field_errors_.end()) {
      for (size_t idx : it->second) {
        result.push_back(errors_[idx]);
      }
    }

    return result;
  }

  /**
   * @brief Check if there are any errors
   */
  [[nodiscard]] bool has_errors() const {
    return std::ranges::any_of(errors_,
                               [](const auto &e) { return e.severity != ErrorSeverity::Warning; });
  }

  /**
   * @brief Check if there are any warnings
   */
  [[nodiscard]] bool has_warnings() const {
    return std::ranges::any_of(errors_,
                               [](const auto &e) { return e.severity == ErrorSeverity::Warning; });
  }

  /**
   * @brief Check if there are any critical errors
   */
  [[nodiscard]] bool has_critical_errors() const {
    return std::ranges::any_of(errors_,
                               [](const auto &e) { return e.severity == ErrorSeverity::Critical; });
  }

  /**
   * @brief Get error count by severity
   */
  [[nodiscard]] std::size_t count_by_severity(ErrorSeverity severity) const {
    return std::ranges::count_if(errors_,
                                 [severity](const auto &e) { return e.severity == severity; });
  }

  /**
   * @brief Get affected fields
   */
  [[nodiscard]] std::set<std::string> get_affected_fields() const {
    std::set<std::string> fields;
    for (const auto &[field, _] : field_errors_) {
      fields.insert(field);
    }
    return fields;
  }

  /**
   * @brief Clear all errors
   */
  void clear() {
    errors_.clear();
    field_errors_.clear();
  }

  /**
   * @brief Get formatted error report
   */
  [[nodiscard]] std::string format_report(bool include_warnings = true,
                                          bool group_by_field = true) const {
    std::ostringstream report;

    if (errors_.empty()) {
      report << "No validation errors found.\n";
      return report.str();
    }

    // Summary
    auto error_count = count_by_severity(ErrorSeverity::Error);
    auto critical_count = count_by_severity(ErrorSeverity::Critical);
    auto warning_count = count_by_severity(ErrorSeverity::Warning);

    report << "Validation Report:\n";
    report << std::string(50, '-') << "\n";

    if (critical_count > 0) {
      report << "Critical Errors: " << critical_count << "\n";
    }
    if (error_count > 0) {
      report << "Errors: " << error_count << "\n";
    }
    if (warning_count > 0 && include_warnings) {
      report << "Warnings: " << warning_count << "\n";
    }
    report << std::string(50, '-') << "\n\n";

    if (group_by_field) {
      // Group by field
      for (const auto &field : get_affected_fields()) {
        report << "Field: " << field << "\n";

        for (const auto &error : get_field_errors(field)) {
          if (error.severity == ErrorSeverity::Warning && !include_warnings) {
            continue;
          }

          report << "  " << error.to_string() << "\n";
        }
        report << "\n";
      }
    } else {
      // List all errors
      for (const auto &error : errors_) {
        if (error.severity == ErrorSeverity::Warning && !include_warnings) {
          continue;
        }

        report << error.to_string() << "\n\n";
      }
    }

    return report.str();
  }

  /**
   * @brief Get JSON representation
   */
  [[nodiscard]] std::string to_json() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"summary\": {\n";
    json << "    \"total\": " << errors_.size() << ",\n";
    json << "    \"critical\": " << count_by_severity(ErrorSeverity::Critical) << ",\n";
    json << "    \"errors\": " << count_by_severity(ErrorSeverity::Error) << ",\n";
    json << "    \"warnings\": " << count_by_severity(ErrorSeverity::Warning) << "\n";
    json << "  },\n";
    json << "  \"errors\": [\n";

    bool first = true;
    for (const auto &error : errors_) {
      if (!first)
        json << ",\n";
      first = false;

      json << "    {\n";
      json << "      \"field\": \"" << error.field_name << "\",\n";
      json << "      \"message\": \"" << error.error_message << "\",\n";
      json << "      \"severity\": \"";

      switch (error.severity) {
      case ErrorSeverity::Warning:
        json << "warning";
        break;
      case ErrorSeverity::Error:
        json << "error";
        break;
      case ErrorSeverity::Critical:
        json << "critical";
        break;
      }
      json << "\"";

      if (error.code) {
        json << ",\n      \"code\": \"" << *error.code << "\"";
      }
      if (error.suggestion) {
        json << ",\n      \"suggestion\": \"" << *error.suggestion << "\"";
      }

      json << "\n    }";
    }

    json << "\n  ]\n";
    json << "}";

    return json.str();
  }

private:
  std::vector<ExtendedValidationError> errors_;
  std::unordered_map<std::string, std::vector<std::size_t>> field_errors_;
};

/**
 * @brief Result type with error aggregator
 */
template <typename T> using AggregatedResult = std::expected<T, ValidationErrorAggregator>;

/**
 * @brief Validate multiple fields and aggregate errors
 */
template <typename... Fields> class MultiFieldValidator {
public:
  template <typename... Validators> explicit MultiFieldValidator(Validators &&...validators) {
    (add_validator(std::forward<Validators>(validators)), ...);
  }

  template <typename T>
  void add_validator(const std::string &field_name, const ValidatorBase<T> &validator) {
    validators_.emplace_back(
        [field_name, validator](const void *data) -> std::optional<ValidationError> {
          const T *value = static_cast<const T *>(data);
          auto result = validator(*value);
          if (result) {
            result->field_name = field_name;
          }
          return result;
        });
  }

  [[nodiscard]] ValidationErrorAggregator validate_all() const {
    ValidationErrorAggregator aggregator;

    for (const auto &validator : validators_) {
      if (auto error = validator(nullptr)) { // Simplified for example
        aggregator.add(*error);
      }
    }

    return aggregator;
  }

private:
  std::vector<std::function<std::optional<ValidationError>(const void *)>> validators_;
};

} // namespace liarsdice::validation