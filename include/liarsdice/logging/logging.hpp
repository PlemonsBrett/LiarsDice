#pragma once

/**
 * @file logging.hpp
 * @brief Main header for the comprehensive logging system
 *
 * This file provides the complete logging interface for the LiarsDice project.
 * It includes structured logging, performance monitoring, and correlation tracking.
 */

#include "i_logger.hpp"
#include "logger_manager.hpp"

// Convenience includes for common functionality
#include <memory>
#include <string_view>

namespace liarsdice::logging {

/**
 * @brief Initialize the logging system with environment-specific configuration
 *
 * @param environment Environment name (development, production, testing)
 * @param config_path Optional path to logging configuration file
 */
inline void initialize_logging(std::string_view environment = "development",
                               std::string_view config_path = "") {
  LoggerConfig config;
  config.environment = std::string(environment);
  if (!config_path.empty()) {
    config.config_file_path = std::string(config_path);
  }

  LoggerManager::instance().initialize(config);
}

/**
 * @brief Get the default logger for general application logging
 */
inline std::shared_ptr<ILogger> get_default_logger() {
  return LoggerManager::instance().get_logger();
}

/**
 * @brief Get a logger for a specific game component
 */
inline std::shared_ptr<ILogger> get_game_logger() {
  return LoggerManager::instance().get_component_logger("game");
}

/**
 * @brief Get a logger for dice-related operations
 */
inline std::shared_ptr<ILogger> get_dice_logger() {
  return LoggerManager::instance().get_component_logger("dice");
}

/**
 * @brief Get a logger for player-related operations
 */
inline std::shared_ptr<ILogger> get_player_logger() {
  return LoggerManager::instance().get_component_logger("player");
}

/**
 * @brief Get a logger for performance monitoring
 */
inline std::shared_ptr<ILogger> get_performance_logger() {
  return LoggerManager::instance().get_logger("performance");
}

/**
 * @brief Shutdown the logging system gracefully
 */
inline void shutdown_logging() { LoggerManager::instance().shutdown(); }

/**
 * @brief RAII helper for automatic logging system management
 */
class LoggingSystem {
public:
  explicit LoggingSystem(std::string_view environment = "development",
                         std::string_view config_path = "") {
    initialize_logging(environment, config_path);
  }

  ~LoggingSystem() { shutdown_logging(); }

  // Non-copyable, non-movable
  LoggingSystem(const LoggingSystem &) = delete;
  LoggingSystem &operator=(const LoggingSystem &) = delete;
  LoggingSystem(LoggingSystem &&) = delete;
  LoggingSystem &operator=(LoggingSystem &&) = delete;
};

} // namespace liarsdice::logging

// Global convenience macros for common logging patterns
#ifdef LIARSDICE_ENABLE_LOGGING

// Game-specific logging macros
#define GAME_LOG_TRACE(msg, ...)                                                                   \
  LOG_TRACE(*liarsdice::logging::get_game_logger(), msg __VA_OPT__(, ) __VA_ARGS__)
#define GAME_LOG_DEBUG(msg, ...)                                                                   \
  LOG_DEBUG(*liarsdice::logging::get_game_logger(), msg __VA_OPT__(, ) __VA_ARGS__)
#define GAME_LOG_INFO(msg, ...)                                                                    \
  LOG_INFO(*liarsdice::logging::get_game_logger(), msg __VA_OPT__(, ) __VA_ARGS__)
#define GAME_LOG_WARN(msg, ...)                                                                    \
  LOG_WARN(*liarsdice::logging::get_game_logger(), msg __VA_OPT__(, ) __VA_ARGS__)
#define GAME_LOG_ERROR(msg, ...)                                                                   \
  LOG_ERROR(*liarsdice::logging::get_game_logger(), msg __VA_OPT__(, ) __VA_ARGS__)

// Dice-specific logging macros
#define DICE_LOG_TRACE(msg, ...)                                                                   \
  LOG_TRACE(*liarsdice::logging::get_dice_logger(), msg __VA_OPT__(, ) __VA_ARGS__)
#define DICE_LOG_DEBUG(msg, ...)                                                                   \
  LOG_DEBUG(*liarsdice::logging::get_dice_logger(), msg __VA_OPT__(, ) __VA_ARGS__)
#define DICE_LOG_INFO(msg, ...)                                                                    \
  LOG_INFO(*liarsdice::logging::get_dice_logger(), msg __VA_OPT__(, ) __VA_ARGS__)

// Player-specific logging macros
#define PLAYER_LOG_TRACE(msg, ...)                                                                 \
  LOG_TRACE(*liarsdice::logging::get_player_logger(), msg __VA_OPT__(, ) __VA_ARGS__)
#define PLAYER_LOG_DEBUG(msg, ...)                                                                 \
  LOG_DEBUG(*liarsdice::logging::get_player_logger(), msg __VA_OPT__(, ) __VA_ARGS__)
#define PLAYER_LOG_INFO(msg, ...)                                                                  \
  LOG_INFO(*liarsdice::logging::get_player_logger(), msg __VA_OPT__(, ) __VA_ARGS__)

// Performance logging macros
#define PERF_LOG_INFO(msg, ...)                                                                    \
  LOG_INFO(*liarsdice::logging::get_performance_logger(), msg __VA_OPT__(, ) __VA_ARGS__)
#define PERF_TIMER(operation)                                                                      \
  LOG_SCOPED_TIMER(*liarsdice::logging::get_performance_logger(), operation)

// Correlation tracking macros
#define WITH_CORRELATION_ID(logger, id) LOG_CORRELATION_SCOPE(*logger, id)
#define NEW_CORRELATION_ID() GENERATE_CORRELATION_ID()

#else

// No-op macros when logging is disabled
#define GAME_LOG_TRACE(msg, ...)                                                                   \
  do {                                                                                             \
  } while (0)
#define GAME_LOG_DEBUG(msg, ...)                                                                   \
  do {                                                                                             \
  } while (0)
#define GAME_LOG_INFO(msg, ...)                                                                    \
  do {                                                                                             \
  } while (0)
#define GAME_LOG_WARN(msg, ...)                                                                    \
  do {                                                                                             \
  } while (0)
#define GAME_LOG_ERROR(msg, ...)                                                                   \
  do {                                                                                             \
  } while (0)

#define DICE_LOG_TRACE(msg, ...)                                                                   \
  do {                                                                                             \
  } while (0)
#define DICE_LOG_DEBUG(msg, ...)                                                                   \
  do {                                                                                             \
  } while (0)
#define DICE_LOG_INFO(msg, ...)                                                                    \
  do {                                                                                             \
  } while (0)

#define PLAYER_LOG_TRACE(msg, ...)                                                                 \
  do {                                                                                             \
  } while (0)
#define PLAYER_LOG_DEBUG(msg, ...)                                                                 \
  do {                                                                                             \
  } while (0)
#define PLAYER_LOG_INFO(msg, ...)                                                                  \
  do {                                                                                             \
  } while (0)

#define PERF_LOG_INFO(msg, ...)                                                                    \
  do {                                                                                             \
  } while (0)
#define PERF_TIMER(operation)                                                                      \
  do {                                                                                             \
  } while (0)

#define WITH_CORRELATION_ID(logger, id)                                                            \
  do {                                                                                             \
  } while (0)
#define NEW_CORRELATION_ID() 0ULL

#endif // LIARSDICE_ENABLE_LOGGING