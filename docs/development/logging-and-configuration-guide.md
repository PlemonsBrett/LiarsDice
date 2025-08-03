# Development Guide: Logging and Configuration Systems

## Quick Start

### Enable Logging and Configuration

Add to your CMake configuration:

```cmake
# Enable both systems (default: ON)
set(LIARSDICE_ENABLE_LOGGING ON)
set(LIARSDICE_ENABLE_CONFIG ON)
```

### Basic Setup

```cpp
#include "liarsdice/logging/logging.hpp"
#include "liarsdice/config/config_manager.hpp"

int main() {
    // Initialize logging
    LoggingSystem logging("development");
    
    // Initialize configuration
    ConfigManager config;
    config.add_source(std::make_unique<DefaultsSource>());
    
    // Start using both systems
    GAME_LOG_INFO("Application started");
    auto max_players = config.get_value_or<uint32_t>(
        ConfigPath{"game.max_players"}, 6);
}
```

## Logging Development Guide

### Adding Logging to Your Components

#### 1. Include the Logging Header

```cpp
#include "liarsdice/logging/logging.hpp"
```

#### 2. Use Component-Specific Macros

```cpp
class DiceManager {
public:
    void roll_dice() {
        DICE_LOG_DEBUG("Starting dice roll operation");
        
        auto result = perform_roll();
        
        DICE_LOG_INFO("Dice rolled: {}", result);
        
        if (result < 1 || result > 6) {
            DICE_LOG_ERROR("Invalid dice result: {}", result);
        }
    }
};
```

#### 3. Add Performance Monitoring

```cpp
void expensive_operation() {
    PERF_TIMER("expensive_operation");
    
    // Your code here
    // Timer automatically logs duration when scope ends
}
```

#### 4. Use Correlation IDs for Request Tracking

```cpp
void handle_game_action(const GameAction& action) {
    auto correlation_id = NEW_CORRELATION_ID();
    
    {
        WITH_CORRELATION_ID(*get_game_logger(), correlation_id);
        
        GAME_LOG_INFO("Processing action: {}", action.type);
        
        // All logs in this scope will include the correlation ID
        process_action(action);
        
        GAME_LOG_INFO("Action completed successfully");
    }
}
```

### Advanced Logging Patterns

#### Structured Logging

```cpp
void log_player_action(const Player& player, const Action& action) {
    auto logger = get_game_logger();
    
    LogContext context;
    context.component = "game_engine";
    context.user_id = player.get_id();
    context.session_id = player.get_session_id();
    
    logger->log_structured(
        spdlog::level::info,
        context,
        "player_action",
        std::format("Player {} performed {}", player.get_name(), action.type)
    );
}
```

#### Conditional Logging

```cpp
void debug_dice_state(const DiceCollection& dice) {
#ifdef LIARSDICE_ENABLE_LOGGING
    if (get_dice_logger()->should_log(spdlog::level::debug)) {
        std::string dice_state;
        for (const auto& die : dice) {
            dice_state += std::format("{} ", die.value);
        }
        DICE_LOG_DEBUG("Current dice state: {}", dice_state);
    }
#endif
}
```

### Logging Best Practices

#### 1. Choose Appropriate Log Levels

```cpp
// ERROR: System errors, exceptions, critical failures
GAME_LOG_ERROR("Failed to initialize game: {}", error.what());

// WARN: Unusual conditions, recoverable errors
GAME_LOG_WARN("Player connection timeout, attempting reconnect");

// INFO: Important business logic events
GAME_LOG_INFO("Game started with {} players", player_count);

// DEBUG: Detailed flow information
GAME_LOG_DEBUG("Validating player move: {}", move.to_string());
```

#### 2. Include Relevant Context

```cpp
// Bad: Vague message
GAME_LOG_ERROR("Operation failed");

// Good: Specific context
GAME_LOG_ERROR("Failed to process bid from player {}: invalid dice count {}",
               player.get_id(), bid.dice_count);
```

#### 3. Use Format Strings Properly

```cpp
// Good: Type-safe formatting
GAME_LOG_INFO("Player {} rolled {} dice", player.name, dice.size());

// Avoid: String concatenation
GAME_LOG_INFO("Player " + player.name + " rolled " + std::to_string(dice.size()) + " dice");
```

#### 4. Handle Logging Errors Gracefully

```cpp
class SafeLogger {
public:
    template<typename... Args>
    void safe_log(spdlog::level::level_enum level, 
                  const std::string& format, Args&&... args) {
        try {
            get_game_logger()->log(level, format, std::forward<Args>(args)...);
        } catch (const std::exception& e) {
            // Fallback to stderr, never throw from logging
            std::cerr << "Logging error: " << e.what() << std::endl;
        }
    }
};
```

## Configuration Development Guide

### Adding Configuration to Your Components

#### 1. Define Configuration Structure

```cpp
// In your component header
struct MyComponentConfig {
    uint32_t max_connections{100};
    std::chrono::seconds timeout{std::chrono::seconds(30)};
    std::string server_address{"localhost"};
    bool enable_compression{true};
    
    std::vector<std::string> validate() const {
        std::vector<std::string> errors;
        
        if (max_connections == 0 || max_connections > 10000) {
            errors.emplace_back("max_connections must be between 1 and 10000");
        }
        
        if (timeout.count() < 1 || timeout.count() > 300) {
            errors.emplace_back("timeout must be between 1 and 300 seconds");
        }
        
        if (server_address.empty()) {
            errors.emplace_back("server_address cannot be empty");
        }
        
        return errors;
    }
};
```

#### 2. Initialize Configuration

```cpp
class MyComponent {
private:
    MyComponentConfig config_;
    ConfigManager& config_manager_;
    
public:
    explicit MyComponent(ConfigManager& config_manager) 
        : config_manager_(config_manager) {
        load_configuration();
    }
    
private:
    void load_configuration() {
        // Load with defaults
        config_.max_connections = config_manager_.get_value_or<uint32_t>(
            ConfigPath{"mycomponent.max_connections"}, config_.max_connections);
            
        config_.timeout = std::chrono::seconds(
            config_manager_.get_value_or<int64_t>(
                ConfigPath{"mycomponent.timeout"}, config_.timeout.count()));
                
        config_.server_address = config_manager_.get_value_or<std::string>(
            ConfigPath{"mycomponent.server_address"}, config_.server_address);
            
        config_.enable_compression = config_manager_.get_value_or<bool>(
            ConfigPath{"mycomponent.enable_compression"}, config_.enable_compression);
        
        // Validate configuration
        auto errors = config_.validate();
        if (!errors.empty()) {
            for (const auto& error : errors) {
                CONFIG_LOG_ERROR("Configuration error: {}", error);
            }
            throw std::runtime_error("Invalid configuration");
        }
        
        CONFIG_LOG_INFO("MyComponent configured with {} max connections", 
                       config_.max_connections);
    }
};
```

#### 3. Support Hot Reloading

```cpp
class MyComponent {
public:
    void reload_configuration() {
        auto old_config = config_;
        
        try {
            load_configuration();
            CONFIG_LOG_INFO("Configuration reloaded successfully");
            
            // Handle specific changes
            if (old_config.max_connections != config_.max_connections) {
                resize_connection_pool(config_.max_connections);
            }
            
        } catch (const std::exception& e) {
            config_ = old_config; // Restore previous config
            CONFIG_LOG_ERROR("Failed to reload configuration: {}", e.what());
            throw;
        }
    }
};
```

### Configuration Sources Setup

#### 1. Development Environment

```cpp
void setup_development_config(ConfigManager& config) {
    // Add defaults first (lowest priority)
    auto defaults = std::make_unique<DefaultsSource>(0);
    defaults->add_defaults({
        {ConfigPath{"game.rules.max_players"}, "6"},
        {ConfigPath{"game.rules.dice_per_player"}, "5"},
        {ConfigPath{"ui.theme"}, "auto"},
        {ConfigPath{"logging.level"}, "debug"}
    });
    config.add_source(std::move(defaults));
    
    // Add local config file
    config.add_source(std::make_unique<JsonFileSource>("config/development.json", 100));
    
    // Add environment variables (highest priority)
    config.add_source(std::make_unique<EnvironmentSource>("LIARSDICE_", 150));
}
```

#### 2. Production Environment

```cpp
void setup_production_config(ConfigManager& config) {
    // Production defaults
    auto defaults = std::make_unique<DefaultsSource>(0);
    defaults->add_defaults({
        {ConfigPath{"logging.level"}, "warn"},
        {ConfigPath{"logging.async"}, "true"},
        {ConfigPath{"logging.file"}, "/var/log/liarsdice.log"}
    });
    config.add_source(std::move(defaults));
    
    // Production config file
    config.add_source(std::make_unique<JsonFileSource>("/etc/liarsdice/config.json", 100));
    
    // Environment variables for container deployment
    config.add_source(std::make_unique<EnvironmentSource>("LIARSDICE_", 150));
    
    // Command line overrides
    config.add_source(std::make_unique<CommandLineSource>(argv, 200));
}
```

### Configuration File Formats

#### JSON Configuration Example

```json
{
  "game": {
    "rules": {
      "max_players": 8,
      "dice_per_player": 5,
      "dice_faces": "6",
      "turn_timeout": 60,
      "difficulty": "normal"
    }
  },
  "ui": {
    "theme": "dark",
    "show_animations": true,
    "animation_speed": 100,
    "language": "en"
  },
  "sound": {
    "mode": "full",
    "master_volume": 80,
    "effects_volume": 90,
    "ambient_volume": 60
  },
  "logging": {
    "level": "info",
    "async": true,
    "file": "./logs/game.log",
    "json_format": false
  }
}
```

#### Environment Variables

```bash
# Game configuration
export LIARSDICE_GAME_RULES_MAX_PLAYERS=8
export LIARSDICE_GAME_RULES_DIFFICULTY=hard

# UI configuration
export LIARSDICE_UI_THEME=dark
export LIARSDICE_UI_LANGUAGE=es

# Logging configuration
export LIARSDICE_LOGGING_LEVEL=debug
export LIARSDICE_LOGGING_FILE=/var/log/liarsdice.log
```

#### Command Line Arguments

```bash
# Start with custom configuration
./liarsdice-cli \
  --game.rules.max-players=4 \
  --ui.theme=light \
  --logging.level=debug \
  --config=/path/to/custom.json
```

### Custom Validators

#### Create Type-Safe Validators

```cpp
// Percentage validator
auto create_percentage_validator() {
    return ConfigValidator<uint32_t>(
        [](uint32_t value) { return value <= 100; },
        []() { return "Value must be between 0 and 100 percent"; }
    );
}

// Network port validator
auto create_port_validator() {
    return ConfigValidator<uint32_t>(
        [](uint32_t port) { return port >= 1024 && port <= 65535; },
        []() { return "Port must be between 1024 and 65535"; }
    );
}

// File path validator
auto create_file_path_validator() {
    return ConfigValidator<std::string>(
        [](const std::string& path) { 
            return !path.empty() && std::filesystem::exists(std::filesystem::path(path).parent_path());
        },
        []() { return "File path must exist and be accessible"; }
    );
}
```

### Testing Configuration

#### Unit Test Configuration Components

```cpp
TEST_CASE("MyComponent configuration loading", "[config]") {
    ConfigManager config;
    auto defaults = std::make_unique<DefaultsSource>(0);
    defaults->add_default(ConfigPath{"mycomponent.max_connections"}, "50");
    config.add_source(std::move(defaults));
    
    MyComponent component(config);
    
    REQUIRE(component.get_max_connections() == 50);
}

TEST_CASE("Configuration validation", "[config]") {
    MyComponentConfig config;
    config.max_connections = 0; // Invalid
    
    auto errors = config.validate();
    REQUIRE_FALSE(errors.empty());
    REQUIRE_THAT(errors[0], Catch::Matchers::ContainsSubstring("max_connections"));
}
```

#### Integration Testing

```cpp
TEST_CASE("Configuration hot reload", "[config][integration]") {
    ConfigManager config;
    // ... setup config sources ...
    
    MyComponent component(config);
    auto initial_connections = component.get_max_connections();
    
    // Simulate configuration change
    config.set_value(ConfigPath{"mycomponent.max_connections"}, 200u);
    component.reload_configuration();
    
    REQUIRE(component.get_max_connections() == 200);
    REQUIRE(component.get_max_connections() != initial_connections);
}
```

## Common Patterns and Examples

### Application Initialization Pattern

```cpp
class Application {
public:
    int run(int argc, char* argv[]) {
        try {
            // 1. Initialize logging first
            setup_logging();
            
            // 2. Initialize configuration
            setup_configuration(argc, argv);
            
            // 3. Initialize application components
            setup_components();
            
            // 4. Start main application loop
            return main_loop();
            
        } catch (const std::exception& e) {
            LOG_ERROR("Application startup failed: {}", e.what());
            return 1;
        }
    }
    
private:
    void setup_logging() {
        logging_system_ = std::make_unique<LoggingSystem>("production");
        LOG_INFO("Logging system initialized");
    }
    
    void setup_configuration(int argc, char* argv[]) {
        config_manager_ = std::make_unique<ConfigManager>();
        
        // Add configuration sources in priority order
        config_manager_->add_source(std::make_unique<DefaultsSource>(0));
        config_manager_->add_source(std::make_unique<JsonFileSource>("config.json", 100));
        config_manager_->add_source(std::make_unique<EnvironmentSource>("APP_", 150));
        config_manager_->add_source(std::make_unique<CommandLineSource>(
            std::vector<std::string_view>(argv + 1, argv + argc), 200));
        
        LOG_INFO("Configuration system initialized");
    }
};
```

### Component Factory Pattern

```cpp
template<typename T>
class ConfigurableComponentFactory {
public:
    static std::unique_ptr<T> create(ConfigManager& config, const std::string& component_name) {
        try {
            auto component = std::make_unique<T>(config, component_name);
            
            LOG_INFO("Created component '{}' of type '{}'", 
                    component_name, typeid(T).name());
            
            return component;
            
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to create component '{}': {}", component_name, e.what());
            throw;
        }
    }
};

// Usage
auto dice_manager = ConfigurableComponentFactory<DiceManager>::create(config, "dice_manager");
auto player_manager = ConfigurableComponentFactory<PlayerManager>::create(config, "player_manager");
```

This development guide provides practical patterns and examples for effectively using the logging and configuration systems in your LiarsDice application development.