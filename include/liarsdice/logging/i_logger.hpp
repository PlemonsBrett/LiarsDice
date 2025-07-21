#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <source_location>

#ifdef LIARSDICE_ENABLE_LOGGING
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#else
// Provide minimal interface when logging is disabled
namespace spdlog {
    enum class level : int {
        trace = 0,
        debug = 1,
        info = 2,
        warn = 3,
        err = 4,
        critical = 5,
        off = 6
    };
}
#endif

namespace liarsdice::logging {

/**
 * @brief Correlation ID for tracing related log messages
 */
using CorrelationId = std::uint64_t;

/**
 * @brief Structured logging context information
 */
struct LogContext {
    CorrelationId correlation_id{0};
    std::string component;
    std::string operation;
    std::string user_id;
    std::string session_id;
    
    LogContext() = default;
    LogContext(std::string_view comp, std::string_view op = "")
        : component(comp), operation(op) {}
};

/**
 * @brief Interface for structured logging with modern C++23 features
 */
class ILogger {
public:
    virtual ~ILogger() = default;

    /**
     * @brief Log a message with specified level
     */
    template<typename... Args>
    void log(spdlog::level::level_enum level, 
             const std::string& message, 
             Args&&... args) const {
        log_impl(level, message, std::forward<Args>(args)...);
    }

    /**
     * @brief Log with context information
     */
    template<typename... Args>
    void log_with_context(spdlog::level::level_enum level,
                         const LogContext& context,
                         const std::string& message,
                         Args&&... args) const {
        log_with_context_impl(level, context, message, std::forward<Args>(args)...);
    }

    /**
     * @brief Convenience methods for different log levels
     */
    template<typename... Args>
    void trace(const std::string& message, Args&&... args) const {
        log(spdlog::level::trace, message, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(const std::string& message, Args&&... args) const {
        log(spdlog::level::debug, message, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(const std::string& message, Args&&... args) const {
        log(spdlog::level::info, message, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(const std::string& message, Args&&... args) const {
        log(spdlog::level::warn, message, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(const std::string& message, Args&&... args) const {
        log(spdlog::level::err, message, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void critical(const std::string& message, Args&&... args) const {
        log(spdlog::level::critical, message, std::forward<Args>(args)...);
    }

    /**
     * @brief Log with source location (C++20 feature)
     */
    template<typename... Args>
    void log_source(spdlog::level::level_enum level,
                   const std::string& message,
                   const std::source_location& location = std::source_location::current(),
                   Args&&... args) const {
        log_source_impl(level, message, location, std::forward<Args>(args)...);
    }

    /**
     * @brief Structured logging with JSON output
     */
    virtual void log_structured(spdlog::level::level_enum level,
                               const LogContext& context,
                               const std::string& event_type,
                               const std::string& message) const = 0;

    /**
     * @brief Performance logging with timing information
     */
    virtual void log_performance(const std::string& operation,
                                std::chrono::nanoseconds duration,
                                const LogContext& context = {}) const = 0;

    /**
     * @brief Set correlation ID for current thread
     */
    virtual void set_correlation_id(CorrelationId id) = 0;

    /**
     * @brief Get correlation ID for current thread
     */
    virtual CorrelationId get_correlation_id() const = 0;

    /**
     * @brief Check if logging level is enabled
     */
    virtual bool should_log(spdlog::level::level_enum level) const = 0;

protected:
    virtual void log_impl(spdlog::level::level_enum level,
                         const std::string& message,
                         fmt::format_args args) const = 0;

    virtual void log_with_context_impl(spdlog::level::level_enum level,
                                      const LogContext& context,
                                      const std::string& message,
                                      fmt::format_args args) const = 0;

    virtual void log_source_impl(spdlog::level::level_enum level,
                                const std::string& message,
                                const std::source_location& location,
                                fmt::format_args args) const = 0;

private:
    // Template implementation for variadic arguments
    template<typename... Args>
    void log_impl(spdlog::level::level_enum level,
                 const std::string& message,
                 Args&&... args) const {
        if constexpr (sizeof...(args) > 0) {
            log_impl(level, message, fmt::make_format_args(args...));
        } else {
            log_impl(level, message, fmt::format_args{});
        }
    }

    template<typename... Args>
    void log_with_context_impl(spdlog::level::level_enum level,
                              const LogContext& context,
                              const std::string& message,
                              Args&&... args) const {
        if constexpr (sizeof...(args) > 0) {
            log_with_context_impl(level, context, message, fmt::make_format_args(args...));
        } else {
            log_with_context_impl(level, context, message, fmt::format_args{});
        }
    }

    template<typename... Args>
    void log_source_impl(spdlog::level::level_enum level,
                        const std::string& message,
                        const std::source_location& location,
                        Args&&... args) const {
        if constexpr (sizeof...(args) > 0) {
            log_source_impl(level, message, location, fmt::make_format_args(args...));
        } else {
            log_source_impl(level, message, location, fmt::format_args{});
        }
    }
};

/**
 * @brief RAII helper for automatic performance timing
 */
class ScopedTimer {
private:
    const ILogger& logger_;
    std::string operation_;
    LogContext context_;
    std::chrono::high_resolution_clock::time_point start_time_;

public:
    ScopedTimer(const ILogger& logger, 
               std::string operation, 
               LogContext context = {})
        : logger_(logger)
        , operation_(std::move(operation))
        , context_(std::move(context))
        , start_time_(std::chrono::high_resolution_clock::now()) {}

    ~ScopedTimer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time_);
        logger_.log_performance(operation_, duration, context_);
    }

    // Non-copyable, non-movable
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer& operator=(ScopedTimer&&) = delete;
};

} // namespace liarsdice::logging

// Convenience macros for logging with source location
#ifdef LIARSDICE_ENABLE_LOGGING
#define LOG_TRACE(logger, msg, ...) (logger).log_source(spdlog::level::trace, msg, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)
#define LOG_DEBUG(logger, msg, ...) (logger).log_source(spdlog::level::debug, msg, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)
#define LOG_INFO(logger, msg, ...) (logger).log_source(spdlog::level::info, msg, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARN(logger, msg, ...) (logger).log_source(spdlog::level::warn, msg, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)
#define LOG_ERROR(logger, msg, ...) (logger).log_source(spdlog::level::err, msg, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)
#define LOG_CRITICAL(logger, msg, ...) (logger).log_source(spdlog::level::critical, msg, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)

#define LOG_SCOPED_TIMER(logger, operation) \
    liarsdice::logging::ScopedTimer timer_##__LINE__(logger, operation)

#define LOG_SCOPED_TIMER_CTX(logger, operation, context) \
    liarsdice::logging::ScopedTimer timer_##__LINE__(logger, operation, context)
#else
// No-op macros when logging is disabled
#define LOG_TRACE(logger, msg, ...) do {} while(0)
#define LOG_DEBUG(logger, msg, ...) do {} while(0)
#define LOG_INFO(logger, msg, ...) do {} while(0)
#define LOG_WARN(logger, msg, ...) do {} while(0)
#define LOG_ERROR(logger, msg, ...) do {} while(0)
#define LOG_CRITICAL(logger, msg, ...) do {} while(0)
#define LOG_SCOPED_TIMER(logger, operation) do {} while(0)
#define LOG_SCOPED_TIMER_CTX(logger, operation, context) do {} while(0)
#endif