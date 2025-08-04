#ifndef LIARSDICE_DATABASE_ERROR_HPP
#define LIARSDICE_DATABASE_ERROR_HPP

#include <string>
#include <variant>
#include <optional>
#include <boost/system/error_code.hpp>

namespace liarsdice::database {

/**
 * @brief Database error types
 */
enum class DatabaseErrorType {
    ConnectionFailed,
    QueryFailed,
    TransactionFailed,
    PreparedStatementFailed,
    ConstraintViolation,
    Timeout,
    InvalidParameter,
    InternalError
};

/**
 * @brief Database error information
 */
class DatabaseError {
public:
    DatabaseError(DatabaseErrorType type, const std::string& message, 
                  boost::system::error_code ec = {})
        : type_(type), message_(message), error_code_(ec) {}
    
    DatabaseErrorType type() const { return type_; }
    const std::string& message() const { return message_; }
    const boost::system::error_code& error_code() const { return error_code_; }
    
    std::string full_message() const {
        std::string result = message_;
        if (error_code_) {
            result += " (";
            result += error_code_.message();
            result += ")";
        }
        return result;
    }
    
private:
    DatabaseErrorType type_;
    std::string message_;
    boost::system::error_code error_code_;
};

/**
 * @brief Result type for database operations (similar to boost::expected)
 * 
 * Since boost::expected is not available in older Boost versions,
 * we use std::variant to create a similar type.
 */
template<typename T>
class DatabaseResult {
public:
    // Constructors
    DatabaseResult(T value) : result_(std::move(value)) {}
    DatabaseResult(DatabaseError error) : result_(std::move(error)) {}
    
    // Check if result contains a value
    bool has_value() const {
        return std::holds_alternative<T>(result_);
    }
    
    // Check if result contains an error
    bool has_error() const {
        return std::holds_alternative<DatabaseError>(result_);
    }
    
    // Get value (throws if error)
    T& value() {
        if (auto* val = std::get_if<T>(&result_)) {
            return *val;
        }
        throw std::runtime_error("DatabaseResult contains error: " + error().full_message());
    }
    
    const T& value() const {
        if (auto* val = std::get_if<T>(&result_)) {
            return *val;
        }
        throw std::runtime_error("DatabaseResult contains error: " + error().full_message());
    }
    
    // Get error (throws if value)
    DatabaseError& error() {
        if (auto* err = std::get_if<DatabaseError>(&result_)) {
            return *err;
        }
        throw std::runtime_error("DatabaseResult contains value, not error");
    }
    
    const DatabaseError& error() const {
        if (auto* err = std::get_if<DatabaseError>(&result_)) {
            return *err;
        }
        throw std::runtime_error("DatabaseResult contains value, not error");
    }
    
    // Convenience operators
    explicit operator bool() const { return has_value(); }
    T& operator*() { return value(); }
    const T& operator*() const { return value(); }
    T* operator->() { return &value(); }
    const T* operator->() const { return &value(); }
    
private:
    std::variant<T, DatabaseError> result_;
};

// Specialization for void operations
template<>
class DatabaseResult<void> {
public:
    DatabaseResult() : error_(std::nullopt) {}
    DatabaseResult(DatabaseError error) : error_(std::move(error)) {}
    
    bool has_value() const { return !error_.has_value(); }
    bool has_error() const { return error_.has_value(); }
    
    const DatabaseError& error() const {
        if (!error_) {
            throw std::runtime_error("DatabaseResult contains success, not error");
        }
        return *error_;
    }
    
    explicit operator bool() const { return has_value(); }
    
private:
    std::optional<DatabaseError> error_;
};

} // namespace liarsdice::database

#endif // LIARSDICE_DATABASE_ERROR_HPP