# Logging System API Reference

## Overview

The LiarsDice logging system provides structured, type-safe logging with correlation tracking and performance monitoring. Built on spdlog with modern C++23 features.

## Core Components

### LoggerManager

Thread-safe singleton managing all logger instances.

```cpp
class LoggerManager {
public:
    static LoggerManager& instance();
    void initialize(const LoggerConfig& config);
    std::shared_ptr<ILogger> get_logger();
    std::shared_ptr<ILogger> get_component_logger(const std::string& component);
    void shutdown();
};
```

#### Methods

- **`instance()`**: Get singleton instance
- **`initialize(config)`**: Initialize with configuration
- **`get_logger()`**: Get default logger
- **`get_component_logger(component)`**: Get component-specific logger
- **`shutdown()`**: Clean shutdown

### ILogger Interface

Core logging interface with structured logging support.

```cpp
class ILogger {
public:
    virtual void log_structured(spdlog::level::level_enum level,
                               const LogContext& context,
                               const std::string& event_type,
                               const std::string& message) const = 0;
    
    template<typename... Args>
    void info(const std::string& format, Args&&... args) const;
    
    template<typename... Args>
    void debug(const std::string& format, Args&&... args) const;
    
    template<typename... Args>
    void warn(const std::string& format, Args&&... args) const;
    
    template<typename... Args>
    void error(const std::string& format, Args&&... args) const;
};
```

### LogContext

Structured logging context for correlation and metadata.

```cpp
struct LogContext {
    std::string component;
    uint64_t correlation_id{0};
    std::optional<std::string> user_id;
    std::optional<std::string> session_id;
    std::optional<std::string> request_id;
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
    std::source_location location{std::source_location::current()};
};
```

### LoggerConfig

Configuration structure for logger initialization.

```cpp
struct LoggerConfig {
    std::string environment{"development"};
    spdlog::level::level_enum log_level{spdlog::level::info};
    bool async_logging{true};
    bool json_format{false};
    bool console_output{true};
    std::optional<std::string> file_path;
    std::size_t async_queue_size{8192};
    std::size_t worker_threads{1};
    std::chrono::milliseconds flush_interval{std::chrono::milliseconds(100)};
};
```

## Logging Macros

### Component-Specific Macros

```cpp
// Game logging
GAME_LOG_INFO(message, ...);
GAME_LOG_DEBUG(message, ...);
GAME_LOG_WARN(message, ...);
GAME_LOG_ERROR(message, ...);

// Dice logging
DICE_LOG_INFO(message, ...);
DICE_LOG_DEBUG(message, ...);
DICE_LOG_WARN(message, ...);
DICE_LOG_ERROR(message, ...);

// Player logging
PLAYER_LOG_INFO(message, ...);
PLAYER_LOG_DEBUG(message, ...);
PLAYER_LOG_WARN(message, ...);
PLAYER_LOG_ERROR(message, ...);

// Performance logging
PERF_LOG_INFO(message, ...);
PERF_LOG_DEBUG(message, ...);
PERF_TIMER(operation_name);
```

### Correlation Tracking

```cpp
// Generate new correlation ID
auto id = NEW_CORRELATION_ID();

// Set correlation scope (RAII)
WITH_CORRELATION_ID(logger, correlation_id);
```

## Convenience Functions

```cpp
// Get pre-configured loggers
std::shared_ptr<ILogger> get_default_logger();
std::shared_ptr<ILogger> get_game_logger();
std::shared_ptr<ILogger> get_dice_logger();
std::shared_ptr<ILogger> get_player_logger();
std::shared_ptr<ILogger> get_performance_logger();
```

## RAII System Management

```cpp
class LoggingSystem {
public:
    explicit LoggingSystem(const std::string& environment);
    ~LoggingSystem(); // Automatic cleanup
};
```

## Usage Examples

### Basic Logging

```cpp
#include "liarsdice/logging/logging.hpp"

// Initialize logging system
LoggingSystem logging("production");

// Simple logging
GAME_LOG_INFO("Game started with {} players", player_count);
DICE_LOG_DEBUG("Rolling dice: result = {}", result);
```

### Structured Logging

```cpp
auto logger = get_game_logger();
LogContext context;
context.component = "game_engine";
context.user_id = "player123";
context.session_id = "session456";

logger->log_structured(spdlog::level::info, context, 
                      "game_event", "Player joined game");
```

### Performance Monitoring

```cpp
PERF_TIMER("dice_roll_operation");
// ... perform dice roll ...
// Timer automatically logs duration when scope ends
```

### Correlation Tracking

```cpp
auto correlation_id = NEW_CORRELATION_ID();
{
    WITH_CORRELATION_ID(*get_game_logger(), correlation_id);
    GAME_LOG_INFO("Starting game operation");
    // All logs in this scope will include correlation_id
}
```

### Configuration

```cpp
LoggerConfig config;
config.environment = "production";
config.log_level = spdlog::level::warn;
config.async_logging = true;
config.json_format = true;
config.file_path = "/var/log/liarsdice.log";

LoggerManager::instance().initialize(config);
```

## Conditional Compilation

Logging can be disabled at compile-time:

```cmake
# Disable logging
set(LIARSDICE_ENABLE_LOGGING OFF)
```

When disabled:
- All macros become no-ops
- `NEW_CORRELATION_ID()` returns 0
- No runtime overhead

## Thread Safety

- All logger instances are thread-safe
- LoggerManager is thread-safe singleton
- Async logging uses lock-free queues
- Correlation IDs are thread-local

## Error Handling

- Invalid configurations log warnings and use defaults
- Failed file operations fall back to console output
- Exceptions in user code don't affect logging system
- Graceful degradation when spdlog unavailable

## Performance Considerations

- Async logging minimizes blocking
- Format strings evaluated only when needed
- Correlation IDs use atomic counters
- RAII objects have minimal overhead
- Compile-time log level optimization available