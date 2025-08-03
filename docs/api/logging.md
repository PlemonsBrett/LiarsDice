# Logging System API

## Overview

The LiarsDice logging system uses Boost.Log to provide comprehensive logging with severity levels, file rotation, and
component-specific loggers. The system supports both console and file output with configurable formatting.

## Core Components

### Logger Interface

**Location**: `include/liarsdice/logging/logger.hpp`

The `ILogger` interface provides the core logging functionality:

{% raw %}
```cpp
namespace liarsdice::logging {
    
    // Severity levels from Boost.Log
    using Severity = boost::log::trivial::severity_level;
    
    class ILogger {
    public:
        virtual ~ILogger() = default;
        
        virtual void trace(const std::string& message) = 0;
        virtual void debug(const std::string& message) = 0;
        virtual void info(const std::string& message) = 0;
        virtual void warning(const std::string& message) = 0;
        virtual void error(const std::string& message) = 0;
        virtual void fatal(const std::string& message) = 0;
        
        // Formatted logging
        template<typename... Args>
        void debug_fmt(const std::string& format, Args&&... args);
        
        template<typename... Args>
        void info_fmt(const std::string& format, Args&&... args);
        
        template<typename... Args>
        void error_fmt(const std::string& format, Args&&... args);
    };
}
```

{% endraw %}

### Logger Configuration

{% raw %}
```cpp
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
```

{% endraw %}

### Logger Manager

The `LoggerManager` class provides centralized logger management:

{% raw %}
```cpp
class LoggerManager {
public:
    static LoggerManager& instance();
    
    void initialize(const LoggerConfig& config = {});
    void shutdown();
    
    std::shared_ptr<ILogger> get_logger(const std::string& name = "default");
    void set_global_level(Severity level);
    void set_console_level(Severity level);
    void set_file_level(Severity level);
    
private:
    LoggerManager() = default;
    void setup_console_sink(const LoggerConfig& config);
    void setup_file_sink(const LoggerConfig& config);
    void setup_formatter();
};
```

{% endraw %}

## Usage Examples

### Basic Logging

{% raw %}
```cpp
#include <liarsdice/logging/logger.hpp>

// Get logger instance
auto logger = liarsdice::logging::LoggerManager::instance().get_logger("game");

// Log messages at different severity levels
logger->trace("Detailed trace information");
logger->debug("Debug information");
logger->info("Game started");
logger->warning("Low dice count");
logger->error("Invalid move attempted");
logger->fatal("Critical game error");
```

{% endraw %}

### Formatted Logging

{% raw %}
```cpp
// Use formatted logging methods
logger->info_fmt("Player {} joined the game", player_name);
logger->debug_fmt("Dice roll: {}, {}, {}, {}, {}", d1, d2, d3, d4, d5);
logger->error_fmt("Invalid bid: {} x {}", quantity, face_value);

// With boost::format
logger->info(boost::str(boost::format("Round %1% started with %2% players") 
    % round_number % player_count));
```

{% endraw %}

### Component-Specific Loggers

{% raw %}
```cpp
// Create loggers for different components
auto game_logger = LoggerManager::instance().get_logger("game");
auto ai_logger = LoggerManager::instance().get_logger("ai");
auto ui_logger = LoggerManager::instance().get_logger("ui");

// Each logger can be used independently
game_logger->info("Game initialized");
ai_logger->debug("AI strategy selected: Medium");
ui_logger->trace("Menu displayed");
```

{% endraw %}

### Configuration

{% raw %}
```cpp
// Custom configuration
liarsdice::logging::LoggerConfig config;
config.console_level = liarsdice::logging::Severity::debug;
config.file_level = liarsdice::logging::Severity::trace;
config.log_directory = "./game_logs";
config.log_filename = "game_%Y%m%d.log";
config.max_file_size = 50 * 1024 * 1024; // 50MB
config.enable_file = true;

// Initialize logging system
liarsdice::logging::LoggerManager::instance().initialize(config);
```

{% endraw %}

### Scoped Attributes

{% raw %}
```cpp
// Add context to log messages using scoped attributes
{
    BOOST_LOG_SCOPED_THREAD_TAG("GameID", game_id);
    BOOST_LOG_SCOPED_THREAD_TAG("PlayerID", player_id);
    
    logger->info("Player made a move");
    // Log output includes GameID and PlayerID
}
```

{% endraw %}

## Logging Macros

For convenience, the system provides logging macros:

{% raw %}
```cpp
// Direct logging macros
LOG_TRACE(message)
LOG_DEBUG(message)
LOG_INFO(message)
LOG_WARNING(message)
LOG_ERROR(message)
LOG_FATAL(message)

// Component-specific macros
GAME_LOG_INFO(message)
GAME_LOG_DEBUG(message)
GAME_LOG_ERROR(message)

AI_LOG_INFO(message)
AI_LOG_DEBUG(message)

UI_LOG_INFO(message)
UI_LOG_TRACE(message)
```

{% endraw %}

## Log Format

The default log format includes:

{% raw %}
```
[Timestamp] [Severity] [Thread] [Component] Message

Example:
[2025-01-15 14:32:15.123] [INFO] [0x7fff8b3a5380] [game] New game started with 4 players
[2025-01-15 14:32:15.456] [DEBUG] [0x7fff8b3a5380] [ai] AI player analyzing game state
[2025-01-15 14:32:15.789] [ERROR] [0x7fff8b3a5380] [game] Invalid bid: quantity exceeds total dice
```

{% endraw %}

## File Rotation

The logging system supports automatic file rotation:

- **Size-based rotation**: Files rotate when reaching `max_file_size`
- **Count-based cleanup**: Keeps only the most recent `max_files` log files
- **Time-based naming**: Log files include timestamp in filename

## Performance Considerations

- **Asynchronous logging**: Available through Boost.Log async frontend
- **Severity filtering**: Messages below threshold are not processed
- **Auto-flush**: Configurable for debugging vs. performance
- **Thread-safe**: All logging operations are thread-safe

## Integration with Game Components

### Game Class Integration

{% raw %}
```cpp
class Game {
private:
    std::shared_ptr<ILogger> logger_;
    
public:
    Game() : logger_(LoggerManager::instance().get_logger("game")) {
        logger_->info("Game instance created");
    }
    
    void start_round() {
        logger_->info_fmt("Starting round {}", current_round_);
        // Game logic...
    }
};
```

{% endraw %}

### AI Integration

{% raw %}
```cpp
class AIPlayer : public Player {
private:
    std::shared_ptr<ILogger> logger_;
    
public:
    AIPlayer(unsigned int id) 
        : Player(id, "AI"), 
          logger_(LoggerManager::instance().get_logger("ai")) {
        logger_->debug_fmt("AI player {} created", id);
    }
    
    Guess make_guess(const std::optional<Guess>& last_guess) override {
        logger_->debug("AI analyzing game state");
        // AI logic...
        logger_->info_fmt("AI decided to bid {} x {}", 
                         guess.count, guess.face);
        return guess;
    }
};
```

{% endraw %}

## Error Handling

{% raw %}

```cpp
try {
    // Game operation
} catch (const std::exception& e) {
    logger->error_fmt("Game error: {}", e.what());
    // Additional error handling
}
```

{% endraw %}

## Best Practices

1. **Use appropriate severity levels**:
    - `TRACE`: Very detailed information, typically disabled
    - `DEBUG`: Detailed information for debugging
    - `INFO`: General informational messages
    - `WARNING`: Warning messages for recoverable issues
    - `ERROR`: Error messages for serious problems
    - `FATAL`: Critical errors requiring immediate attention

2. **Component-specific loggers**: Use separate loggers for different subsystems

3. **Structured logging**: Include relevant context in log messages

4. **Performance**: Disable verbose logging in production builds

5. **Security**: Never log sensitive information (passwords, personal data)

## See Also

- [Boost.Log Documentation](https://www.boost.org/doc/libs/release/libs/log/doc/html/index.html)
- [Core Game API](core.md) - Game component integration
- [AI System](ai.md) - AI logging integration