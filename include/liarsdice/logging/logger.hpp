#pragma once

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/core/null_deleter.hpp>
#include <string>
#include <memory>
#include <iostream>

namespace liarsdice::logging {

// Severity levels
using Severity = boost::log::trivial::severity_level;

// Forward declarations
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;

// Logger configuration
struct LoggerConfig {
    Severity console_level = Severity::info;
    Severity file_level = Severity::debug;
    std::string log_directory = "logs";
    std::string log_filename = "liarsdice_%Y%m%d_%H%M%S.log";
    size_t max_file_size = 10 * 1024 * 1024; // 10MB
    size_t max_files = 5;
    bool enable_console = true;
    bool enable_file = true;
    bool auto_flush = true;
};

// Logger interface
class ILogger {
public:
    virtual ~ILogger() = default;
    
    virtual void trace(const std::string& message) = 0;
    virtual void debug(const std::string& message) = 0;
    virtual void info(const std::string& message) = 0;
    virtual void warning(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
    virtual void fatal(const std::string& message) = 0;
    
    virtual void set_component(const std::string& component) = 0;
    virtual std::string get_component() const = 0;
};

// Boost.Log implementation
class BoostLogger : public ILogger {
public:
    explicit BoostLogger(const std::string& component = "");
    
    void trace(const std::string& message) override;
    void debug(const std::string& message) override;
    void info(const std::string& message) override;
    void warning(const std::string& message) override;
    void error(const std::string& message) override;
    void fatal(const std::string& message) override;
    
    void set_component(const std::string& component) override { component_ = component; }
    std::string get_component() const override { return component_; }

private:
    void log(Severity level, const std::string& message);
    
    src::severity_logger<Severity> logger_;
    std::string component_;
};

// Logger factory and manager
class LoggerManager {
public:
    static LoggerManager& instance();
    
    void initialize(const LoggerConfig& config = {});
    void shutdown();
    
    std::shared_ptr<ILogger> get_logger(const std::string& component = "");
    
    // Component-specific loggers
    std::shared_ptr<ILogger> get_game_logger() { return get_logger("GAME"); }
    std::shared_ptr<ILogger> get_player_logger() { return get_logger("PLAYER"); }
    std::shared_ptr<ILogger> get_ai_logger() { return get_logger("AI"); }
    std::shared_ptr<ILogger> get_config_logger() { return get_logger("CONFIG"); }
    std::shared_ptr<ILogger> get_validation_logger() { return get_logger("VALIDATION"); }
    
private:
    LoggerManager() = default;
    
    void setup_console_sink(const LoggerConfig& config);
    void setup_file_sink(const LoggerConfig& config);
    
    bool initialized_ = false;
    LoggerConfig config_;
};

// Convenience macros
#define LOG_TRACE(component, message) \
    liarsdice::logging::LoggerManager::instance().get_logger(component)->trace(message)

#define LOG_DEBUG(component, message) \
    liarsdice::logging::LoggerManager::instance().get_logger(component)->debug(message)

#define LOG_INFO(component, message) \
    liarsdice::logging::LoggerManager::instance().get_logger(component)->info(message)

#define LOG_WARNING(component, message) \
    liarsdice::logging::LoggerManager::instance().get_logger(component)->warning(message)

#define LOG_ERROR(component, message) \
    liarsdice::logging::LoggerManager::instance().get_logger(component)->error(message)

#define LOG_FATAL(component, message) \
    liarsdice::logging::LoggerManager::instance().get_logger(component)->fatal(message)

// Component-specific macros
#define GAME_LOG_DEBUG(message) LOG_DEBUG("GAME", message)
#define GAME_LOG_INFO(message) LOG_INFO("GAME", message)
#define GAME_LOG_WARNING(message) LOG_WARNING("GAME", message)
#define GAME_LOG_ERROR(message) LOG_ERROR("GAME", message)

#define PLAYER_LOG_DEBUG(message) LOG_DEBUG("PLAYER", message)
#define PLAYER_LOG_INFO(message) LOG_INFO("PLAYER", message)
#define PLAYER_LOG_WARNING(message) LOG_WARNING("PLAYER", message)
#define PLAYER_LOG_ERROR(message) LOG_ERROR("PLAYER", message)

#define AI_LOG_DEBUG(message) LOG_DEBUG("AI", message)
#define AI_LOG_INFO(message) LOG_INFO("AI", message)
#define AI_LOG_WARNING(message) LOG_WARNING("AI", message)
#define AI_LOG_ERROR(message) LOG_ERROR("AI", message)

} // namespace liarsdice::logging