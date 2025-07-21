#include "liarsdice/logging/logger_manager.hpp"
#include <fstream>
#include <iostream>
#include <thread>

#ifdef LIARSDICE_ENABLE_LOGGING
#include <nlohmann/json.hpp>
#include <spdlog/details/os.h>
#include <spdlog/pattern_formatter.h>
#endif

namespace liarsdice::logging {

#ifdef LIARSDICE_ENABLE_LOGGING
using json = nlohmann::json;
#endif

// Thread-local storage for correlation IDs
thread_local CorrelationId SpdlogLogger::thread_correlation_id_ = 0;

LoggerManager::~LoggerManager() {
  if (initialized_.load()) {
    shutdown();
  }
}

void LoggerManager::initialize(const LoggerConfig &config) {
  if (initialized_.exchange(true)) {
    return; // Already initialized
  }

  config_ = config;

  try {
    create_log_directories();

    if (!config_.config_file_path.empty() && std::filesystem::exists(config_.config_file_path)) {
      initialize_from_config(config_.config_file_path);
    } else {
      initialize_default_config();
    }

    setup_async_logging();
    setup_default_loggers();

    auto logger = get_logger();
    logger->info("Logging system initialized successfully");
    logger->info("Environment: {}, Config: {}", config_.environment,
                 config_.config_file_path.empty() ? "default" : config_.config_file_path);

  } catch (const std::exception &e) {
    initialized_.store(false);
    std::cerr << "Failed to initialize logging system: " << e.what() << std::endl;
    throw;
  }
}

std::shared_ptr<ILogger> LoggerManager::get_logger(std::string_view name) {
  std::string logger_name{name};

  // Try to find existing logger first
  {
    std::shared_lock lock(loggers_mutex_);
    auto it = loggers_.find(logger_name);
    if (it != loggers_.end()) {
      return it->second;
    }
  }

  // Create new logger
  std::unique_lock lock(loggers_mutex_);

  // Double-check pattern
  auto it = loggers_.find(logger_name);
  if (it != loggers_.end()) {
    return it->second;
  }

#ifdef LIARSDICE_ENABLE_LOGGING
  try {
    std::shared_ptr<spdlog::logger> spdlog_logger;

    if (logger_name == "default") {
      spdlog_logger = spdlog::default_logger();
    } else {
      spdlog_logger = spdlog::get(logger_name);
      if (!spdlog_logger) {
        // Create a new logger based on default configuration
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            config_.log_directory + "/" + logger_name + ".log", 1024 * 1024 * 5, 3);

        spdlog_logger = std::make_shared<spdlog::logger>(
            logger_name, spdlog::sinks_init_list{console_sink, file_sink});

        spdlog::register_logger(spdlog_logger);
      }
    }

    auto logger = std::make_shared<SpdlogLogger>(spdlog_logger);
    loggers_[logger_name] = logger;
    return logger;

  } catch (const std::exception &e) {
    std::cerr << "Failed to create logger '" << logger_name << "': " << e.what() << std::endl;
    // Return a basic logger as fallback
    auto logger = std::make_shared<SpdlogLogger>(spdlog::default_logger());
    loggers_[logger_name] = logger;
    return logger;
  }
#else
  auto logger = std::make_shared<SpdlogLogger>(logger_name);
  loggers_[logger_name] = logger;
  return logger;
#endif
}

std::shared_ptr<ILogger> LoggerManager::get_component_logger(std::string_view component) {
  return get_logger(std::string("component.") + std::string(component));
}

void LoggerManager::shutdown() {
  if (!initialized_.exchange(false)) {
    return; // Already shutdown
  }

  shutdown_requested_.store(true);

  {
    std::unique_lock lock(loggers_mutex_);
    loggers_.clear();
  }

#ifdef LIARSDICE_ENABLE_LOGGING
  if (thread_pool_) {
    spdlog::shutdown();
    thread_pool_.reset();
  }
#endif
}

CorrelationId LoggerManager::generate_correlation_id() {
  return next_correlation_id_.fetch_add(1, std::memory_order_relaxed);
}

void LoggerManager::set_global_level(spdlog::level::level_enum level) {
#ifdef LIARSDICE_ENABLE_LOGGING
  spdlog::set_level(level);
#endif
}

LoggerManager::Statistics LoggerManager::get_statistics() const {
  std::shared_lock lock(loggers_mutex_);
  return Statistics{.total_loggers = loggers_.size(),
                    .messages_logged = messages_logged_.load(),
                    .errors_logged = errors_logged_.load(),
                    .last_correlation_id = next_correlation_id_.load() - 1};
}

void LoggerManager::create_log_directories() {
  std::filesystem::path log_dir(config_.log_directory);
  if (!std::filesystem::exists(log_dir)) {
    std::filesystem::create_directories(log_dir);
  }

  // Create subdirectories for different log types
  std::filesystem::create_directories(log_dir / "archive");
  std::filesystem::create_directories(log_dir / "performance");
  std::filesystem::create_directories(log_dir / "audit");
}

void LoggerManager::initialize_from_config(const std::filesystem::path &config_path) {
#ifdef LIARSDICE_ENABLE_LOGGING
  try {
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
      throw std::runtime_error("Cannot open config file: " + config_path.string());
    }

    json config;
    config_file >> config;

    // Parse global settings
    if (config.contains("global")) {
      const auto &global = config["global"];
      if (global.contains("level")) {
        auto level_str = global["level"].get<std::string>();
        auto level = spdlog::level::from_str(level_str);
        spdlog::set_level(level);
      }
      if (global.contains("flush_on")) {
        auto flush_level_str = global["flush_on"].get<std::string>();
        auto flush_level = spdlog::level::from_str(flush_level_str);
        spdlog::flush_on(flush_level);
      }
    }

    // Setup async logging if configured
    if (config.contains("async") && config["async"]["enabled"].get<bool>()) {
      const auto &async_config = config["async"];
      std::size_t queue_size = async_config.value("queue_size", 8192);
      std::size_t thread_count = async_config.value("thread_count", 1);

      thread_pool_ = std::make_shared<spdlog::details::thread_pool>(queue_size, thread_count);
      spdlog::set_default_logger(
          spdlog::create_async<spdlog::sinks::stdout_color_sink_mt>("async_default", thread_pool_));
    }

  } catch (const std::exception &e) {
    std::cerr << "Failed to parse config file: " << e.what() << std::endl;
    initialize_default_config();
  }
#else
  initialize_default_config();
#endif
}

void LoggerManager::initialize_default_config() {
#ifdef LIARSDICE_ENABLE_LOGGING
  // Create default console sink
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::debug);
  console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

  // Create default file sink
  auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(config_.log_directory +
                                                                              "/liarsdice.log",
                                                                          1024 * 1024 * 10, // 10MB
                                                                          5); // 5 files
  file_sink->set_level(spdlog::level::trace);
  file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] %v");

  // Create default logger
  auto default_logger =
      std::make_shared<spdlog::logger>("default", spdlog::sinks_init_list{console_sink, file_sink});

  default_logger->set_level(spdlog::level::debug);
  spdlog::set_default_logger(default_logger);
  spdlog::flush_on(spdlog::level::err);
#endif
}

void LoggerManager::setup_async_logging() {
  if (config_.force_sync) {
    return;
  }

#ifdef LIARSDICE_ENABLE_LOGGING
  if (!thread_pool_) {
    // Create default async setup
    thread_pool_ = std::make_shared<spdlog::details::thread_pool>(8192, 1);
  }
#endif
}

void LoggerManager::setup_default_loggers() {
  // Pre-create common loggers
  get_logger("default");
  get_component_logger("game");
  get_component_logger("dice");
  get_component_logger("player");
  get_logger("performance");
  get_logger("audit");
}

// SpdlogLogger implementation
#ifdef LIARSDICE_ENABLE_LOGGING
SpdlogLogger::SpdlogLogger(std::shared_ptr<spdlog::logger> logger)
    : spdlog_logger_(std::move(logger)) {}
#else
SpdlogLogger::SpdlogLogger(std::string name) : logger_name_(std::move(name)) {}
#endif

void SpdlogLogger::log_structured(spdlog::level::level_enum level, const LogContext &context,
                                  const std::string &event_type, const std::string &message) const {
#ifdef LIARSDICE_ENABLE_LOGGING
  if (!spdlog_logger_ || !spdlog_logger_->should_log(level)) {
    return;
  }

  // Create structured JSON log entry
  json log_entry = {
      {     "timestamp", spdlog::details::os::utc_minutes_offset()},
      {         "level",      spdlog::level::to_string_view(level)},
      {    "event_type",                                event_type},
      {       "message",                                   message},
      {"correlation_id",                    context.correlation_id},
      {     "component",                         context.component},
      {     "operation",                         context.operation}
  };

  if (!context.user_id.empty()) {
    log_entry["user_id"] = context.user_id;
  }
  if (!context.session_id.empty()) {
    log_entry["session_id"] = context.session_id;
  }

  spdlog_logger_->log(level, log_entry.dump());
#endif
}

void SpdlogLogger::log_performance(const std::string &operation, std::chrono::nanoseconds duration,
                                   const LogContext &context) const {
#ifdef LIARSDICE_ENABLE_LOGGING
  if (!spdlog_logger_)
    return;

  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();

  LoggerManager::instance().messages_logged_.fetch_add(1);

  std::string message = format_performance_message(operation, duration);
  std::string ctx_str = format_context(context);

  spdlog_logger_->info("[PERF] {} {} ({}ms / {}μs)", message, ctx_str, duration_ms, duration_us);
#endif
}

void SpdlogLogger::set_correlation_id(CorrelationId id) { thread_correlation_id_ = id; }

CorrelationId SpdlogLogger::get_correlation_id() const { return thread_correlation_id_; }

bool SpdlogLogger::should_log(spdlog::level::level_enum level) const {
#ifdef LIARSDICE_ENABLE_LOGGING
  return spdlog_logger_ && spdlog_logger_->should_log(level);
#else
  return false;
#endif
}

void SpdlogLogger::log_impl(spdlog::level::level_enum level, const std::string &message,
                            fmt::format_args args) const {
#ifdef LIARSDICE_ENABLE_LOGGING
  if (!spdlog_logger_ || !spdlog_logger_->should_log(level)) {
    return;
  }

  LoggerManager::instance().messages_logged_.fetch_add(1);
  if (level >= spdlog::level::err) {
    LoggerManager::instance().errors_logged_.fetch_add(1);
  }

  if (thread_correlation_id_ != 0) {
    std::string formatted_message = fmt::vformat(message, args);
    spdlog_logger_->log(level, "[CID:{}] {}", thread_correlation_id_, formatted_message);
  } else {
    spdlog_logger_->log(level, fmt::runtime(message), args);
  }
#endif
}

void SpdlogLogger::log_with_context_impl(spdlog::level::level_enum level, const LogContext &context,
                                         const std::string &message, fmt::format_args args) const {
#ifdef LIARSDICE_ENABLE_LOGGING
  if (!spdlog_logger_ || !spdlog_logger_->should_log(level)) {
    return;
  }

  LoggerManager::instance().messages_logged_.fetch_add(1);
  std::string formatted_message = fmt::vformat(message, args);
  std::string ctx_str = format_context(context);

  spdlog_logger_->log(level, "{} {}", formatted_message, ctx_str);
#endif
}

void SpdlogLogger::log_source_impl(spdlog::level::level_enum level, const std::string &message,
                                   const std::source_location &location,
                                   fmt::format_args args) const {
#ifdef LIARSDICE_ENABLE_LOGGING
  if (!spdlog_logger_ || !spdlog_logger_->should_log(level)) {
    return;
  }

  LoggerManager::instance().messages_logged_.fetch_add(1);
  std::string formatted_message = fmt::vformat(message, args);

  std::string source_info =
      fmt::format("{}:{}:{}", location.file_name(), location.line(), location.function_name());

  if (thread_correlation_id_ != 0) {
    spdlog_logger_->log(level, "[CID:{}] [{}] {}", thread_correlation_id_, source_info,
                        formatted_message);
  } else {
    spdlog_logger_->log(level, "[{}] {}", source_info, formatted_message);
  }
#endif
}

std::string SpdlogLogger::format_context(const LogContext &context) const {
  std::string result;

  if (context.correlation_id != 0) {
    result += fmt::format("[CID:{}]", context.correlation_id);
  }
  if (!context.component.empty()) {
    result += fmt::format("[{}]", context.component);
  }
  if (!context.operation.empty()) {
    result += fmt::format("[{}]", context.operation);
  }
  if (!context.user_id.empty()) {
    result += fmt::format("[User:{}]", context.user_id);
  }
  if (!context.session_id.empty()) {
    result += fmt::format("[Session:{}]", context.session_id);
  }

  return result;
}

std::string SpdlogLogger::format_performance_message(const std::string &operation,
                                                     std::chrono::nanoseconds duration) const {
  return fmt::format("Operation '{}' completed", operation);
}

} // namespace liarsdice::logging