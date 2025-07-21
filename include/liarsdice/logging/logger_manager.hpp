#pragma once

#include "i_logger.hpp"
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>
#include <filesystem>
#include <string_view>

#ifdef LIARSDICE_ENABLE_LOGGING
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#ifdef __linux__
#include <spdlog/sinks/syslog_sink.h>
#endif
#endif

namespace liarsdice::logging {

/**
 * @brief Configuration for logger initialization
 */
struct LoggerConfig {
    std::string config_file_path;
    std::string environment = "development"; // development, production, testing
    bool force_sync = false;
    bool enable_console = true;
    std::string log_directory = "logs";
};

/**
 * @brief Thread-safe singleton for managing loggers with modern C++23 features
 */
class LoggerManager {
public:
    /**
     * @brief Get the singleton instance with thread-safe initialization
     */
    static LoggerManager& instance() {
        static LoggerManager instance_;
        return instance_;
    }

    /**
     * @brief Initialize the logging system with configuration
     */
    void initialize(const LoggerConfig& config = {});

    /**
     * @brief Get or create a logger by name
     */
    std::shared_ptr<ILogger> get_logger(std::string_view name = "default");

    /**
     * @brief Create a specialized logger for a component
     */
    std::shared_ptr<ILogger> get_component_logger(std::string_view component);

    /**
     * @brief Shutdown the logging system gracefully
     */
    void shutdown();

    /**
     * @brief Generate a new correlation ID
     */
    CorrelationId generate_correlation_id();

    /**
     * @brief Set global log level
     */
    void set_global_level(spdlog::level::level_enum level);

    /**
     * @brief Check if logging system is initialized
     */
    bool is_initialized() const { return initialized_.load(); }

    /**
     * @brief Get logging statistics
     */
    struct Statistics {
        std::size_t total_loggers;
        std::size_t messages_logged;
        std::size_t errors_logged;
        CorrelationId last_correlation_id;
    };
    Statistics get_statistics() const;

    // Non-copyable, non-movable
    LoggerManager(const LoggerManager&) = delete;
    LoggerManager& operator=(const LoggerManager&) = delete;
    LoggerManager(LoggerManager&&) = delete;
    LoggerManager& operator=(LoggerManager&&) = delete;

private:
    LoggerManager() = default;
    ~LoggerManager();

    void initialize_from_config(const std::filesystem::path& config_path);
    void initialize_default_config();
    void create_log_directories();
    void setup_async_logging();
    void setup_default_loggers();

#ifdef LIARSDICE_ENABLE_LOGGING
    void setup_sinks_from_config();
    std::shared_ptr<spdlog::sinks::sink> create_sink_from_config(
        const std::string& sink_type,
        const std::string& config);
#endif

    mutable std::shared_mutex loggers_mutex_;
    std::unordered_map<std::string, std::shared_ptr<ILogger>> loggers_;
    
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_requested_{false};
    std::atomic<CorrelationId> next_correlation_id_{1};
    std::atomic<std::size_t> messages_logged_{0};
    std::atomic<std::size_t> errors_logged_{0};
    
    LoggerConfig config_;
    
#ifdef LIARSDICE_ENABLE_LOGGING
    std::shared_ptr<spdlog::details::thread_pool> thread_pool_;
#endif
};

/**
 * @brief Concrete implementation of ILogger using spdlog
 */
class SpdlogLogger : public ILogger {
public:
#ifdef LIARSDICE_ENABLE_LOGGING
    explicit SpdlogLogger(std::shared_ptr<spdlog::logger> logger);
#else
    explicit SpdlogLogger(std::string name);
#endif

    void log_structured(spdlog::level::level_enum level,
                       const LogContext& context,
                       const std::string& event_type,
                       const std::string& message) const override;

    void log_performance(const std::string& operation,
                        std::chrono::nanoseconds duration,
                        const LogContext& context = {}) const override;

    void set_correlation_id(CorrelationId id) override;
    CorrelationId get_correlation_id() const override;
    bool should_log(spdlog::level::level_enum level) const override;

protected:
    void log_impl(spdlog::level::level_enum level,
                 const std::string& message,
                 fmt::format_args args) const override;

    void log_with_context_impl(spdlog::level::level_enum level,
                              const LogContext& context,
                              const std::string& message,
                              fmt::format_args args) const override;

    void log_source_impl(spdlog::level::level_enum level,
                        const std::string& message,
                        const std::source_location& location,
                        fmt::format_args args) const override;

private:
    std::string format_context(const LogContext& context) const;
    std::string format_performance_message(const std::string& operation,
                                          std::chrono::nanoseconds duration) const;

#ifdef LIARSDICE_ENABLE_LOGGING
    std::shared_ptr<spdlog::logger> spdlog_logger_;
#else
    std::string logger_name_;
#endif
    
    static thread_local CorrelationId thread_correlation_id_;
};

/**
 * @brief RAII helper for setting correlation ID scope
 */
class CorrelationScope {
private:
    ILogger& logger_;
    CorrelationId previous_id_;
    
public:
    explicit CorrelationScope(ILogger& logger, CorrelationId id)
        : logger_(logger), previous_id_(logger_.get_correlation_id()) {
        logger_.set_correlation_id(id);
    }
    
    ~CorrelationScope() {
        logger_.set_correlation_id(previous_id_);
    }
    
    // Non-copyable, non-movable
    CorrelationScope(const CorrelationScope&) = delete;
    CorrelationScope& operator=(const CorrelationScope&) = delete;
    CorrelationScope(CorrelationScope&&) = delete;
    CorrelationScope& operator=(CorrelationScope&&) = delete;
};

} // namespace liarsdice::logging

// Convenience macros for common logging patterns
#ifdef LIARSDICE_ENABLE_LOGGING
#define GET_LOGGER(name) liarsdice::logging::LoggerManager::instance().get_logger(name)
#define GET_COMPONENT_LOGGER(component) liarsdice::logging::LoggerManager::instance().get_component_logger(component)
#define LOG_CORRELATION_SCOPE(logger, id) liarsdice::logging::CorrelationScope scope_##__LINE__(logger, id)
#define GENERATE_CORRELATION_ID() liarsdice::logging::LoggerManager::instance().generate_correlation_id()
#else
#define GET_LOGGER(name) nullptr
#define GET_COMPONENT_LOGGER(component) nullptr
#define LOG_CORRELATION_SCOPE(logger, id) do {} while(0)
#define GENERATE_CORRELATION_ID() 0ULL
#endif