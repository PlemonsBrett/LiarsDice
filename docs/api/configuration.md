# Configuration System API Reference

## Overview

The LiarsDice configuration system provides type-safe, hierarchical configuration management with multiple sources, hot-reloading, and modern C++23 features including concepts and std::variant.

## Core Components

### ConfigValue

Type-safe configuration value wrapper using std::variant.

```cpp
class ConfigValue {
public:
    constexpr ConfigValue() noexcept = default;
    
    template<ConfigValueType T>
    constexpr ConfigValue(T&& value) noexcept;
    
    template<typename T>
    constexpr std::optional<T> get() const noexcept;
    
    template<typename T>
    constexpr T get_or(const T& default_val) const noexcept;
    
    template<typename T>
    constexpr T get_required() const;
    
    constexpr bool is_set() const noexcept;
    
    template<ConfigValueType T>
    constexpr void set(T&& new_value) noexcept;
    
    std::string to_string() const;
    static std::optional<ConfigValue> from_string(const std::string& str, std::size_t target_type_index);
};
```

#### Supported Types

```cpp
using ConfigVariant = std::variant<
    std::monostate,     // Null/unset
    bool,
    int32_t,
    int64_t,
    uint32_t,
    uint64_t,
    double,
    std::string,
    std::vector<std::string>
>;
```

### ConfigPath

Hierarchical path for configuration keys.

```cpp
class ConfigPath {
public:
    explicit ConfigPath(std::string_view path = "");
    explicit ConfigPath(std::vector<std::string> segments);
    
    std::string to_string() const;
    std::optional<ConfigPath> parent() const;
    ConfigPath append(std::string_view segment) const;
    bool is_root() const noexcept;
    const std::vector<std::string>& segments() const noexcept;
};
```

### ConfigManager

Central configuration management with multiple sources.

```cpp
class ConfigManager {
public:
    void add_source(std::unique_ptr<IConfigSource> source);
    void remove_source(const std::string& name);
    
    template<typename T>
    std::optional<T> get_value(const ConfigPath& path) const;
    
    template<typename T>
    T get_value_or(const ConfigPath& path, const T& default_value) const;
    
    template<typename T>
    void set_value(const ConfigPath& path, const T& value);
    
    bool has_value(const ConfigPath& path) const;
    void reload_sources();
    std::vector<ConfigPath> get_all_paths() const;
};
```

## Configuration Sources

### IConfigSource Interface

```cpp
class IConfigSource {
public:
    virtual std::string get_name() const = 0;
    virtual int get_priority() const = 0;
    virtual bool has_value(const ConfigPath& path) const = 0;
    virtual std::string get_raw_value(const ConfigPath& path) const = 0;
    virtual std::vector<ConfigPath> get_all_paths() const = 0;
    virtual bool is_valid() const = 0;
    virtual void reload() = 0;
};
```

### DefaultsSource

Provides default configuration values.

```cpp
class DefaultsSource : public IConfigSource {
public:
    explicit DefaultsSource(int priority = 0);
    
    void add_default(const ConfigPath& path, const std::string& value);
    void add_defaults(const std::map<ConfigPath, std::string>& defaults);
};
```

### EnvironmentSource

Reads configuration from environment variables.

```cpp
class EnvironmentSource : public IConfigSource {
public:
    explicit EnvironmentSource(const std::string& prefix = "", int priority = 50);
    
    // Environment variable TEST_GAME_MAX_PLAYERS maps to path "game.max.players"
};
```

### CommandLineSource

Parses command line arguments.

```cpp
class CommandLineSource : public IConfigSource {
public:
    explicit CommandLineSource(const std::vector<std::string_view>& args, int priority = 200);
    
    // --game.max-players=6 maps to path "game.max.players"
    // --verbose becomes boolean flag
};
```

### JsonFileSource

Reads configuration from JSON files.

```cpp
class JsonFileSource : public IConfigSource {
public:
    explicit JsonFileSource(const std::string& file_path, int priority = 100);
    
    // Hierarchical JSON structure maps to dot-notation paths
};
```

## Game-Specific Configuration

### GameConfig

Complete game configuration structure.

```cpp
class GameConfig {
public:
    GameRules rules;
    UIPreferences ui;
    SoundConfig sound;
    AIConfig ai;
    NetworkConfig network;
    
    std::vector<std::string> validate_all() const;
    void reset_to_defaults();
    uint32_t get_version() const;
};
```

### Game Configuration Structures

```cpp
// Game difficulty levels
enum class Difficulty : uint8_t {
    Beginner, Easy, Normal, Hard, Expert
};

// Dice face options  
enum class DiceFaces : uint8_t {
    Four = 4, Six = 6, Eight = 8, Ten = 10, Twelve = 12, Twenty = 20
};

// Game variants
enum class GameVariant : uint8_t {
    Classic, Perudo, Dudo, Challenge
};

// UI themes
enum class UITheme : uint8_t {
    Auto, Light, Dark, HighContrast
};

// Sound modes
enum class SoundMode : uint8_t {
    Off, Effects, Full
};
```

### GameRules

```cpp
struct GameRules {
    uint32_t min_players{2};
    uint32_t max_players{6};
    uint32_t dice_per_player{5};
    DiceFaces dice_faces{DiceFaces::Six};
    GameVariant variant{GameVariant::Classic};
    Difficulty difficulty{Difficulty::Normal};
    std::chrono::seconds turn_timeout{std::chrono::seconds(60)};
    bool enable_wild_dice{true};
    bool allow_spectators{true};
    
    std::vector<std::string> validate() const;
    std::string describe() const;
};
```

### UIPreferences

```cpp
struct UIPreferences {
    UITheme theme{UITheme::Auto};
    bool show_animations{true};
    uint32_t animation_speed{100}; // 50-150%
    uint32_t font_scale{100};      // 75-125%
    std::string language{"en"};
    bool show_dice_history{true};
    bool show_player_stats{false};
    bool highlight_current_player{true};
    
    std::vector<std::string> validate() const;
};
```

## Validators

### ConfigValidator

Type-safe validation system.

```cpp
template<typename T>
class ConfigValidator {
public:
    using ValidatorFunc = std::function<bool(const T&)>;
    using ErrorMessageFunc = std::function<std::string()>;
    
    ConfigValidator(ValidatorFunc validator, ErrorMessageFunc error_message);
    bool validate(const T& value) const;
    std::string get_error_message() const;
};
```

### Predefined Validators

```cpp
namespace validation {
    auto percentage_validator();
    auto port_validator();
    auto player_count_validator(uint32_t min, uint32_t max);
    auto positive_validator();
    auto non_empty_string_validator();
}
```

### Range Validators

```cpp
template<typename T>
constexpr auto make_range_validator(T min_val, T max_val);

template<typename E>
constexpr auto make_enum_validator(std::initializer_list<E> valid_values);
```

## C++23 Concepts

### Configuration Concepts

```cpp
template<typename T>
concept ConfigValueType = requires {
    std::default_initializable<std::remove_cvref_t<T>>;
    std::copyable<std::remove_cvref_t<T>>;
    std::equality_comparable<std::remove_cvref_t<T>>;
};

template<typename T>
concept StringConvertible = ConfigValueType<T> && requires(const std::string& str) {
    { std::declval<T>() } -> std::convertible_to<T>;
};

template<typename T>
concept ConfigSource = requires(T& source, const std::string& key) {
    { source.has_value(key) } -> std::same_as<bool>;
    { source.get_raw_value(key) } -> std::same_as<std::optional<std::string>>;
    { source.get_all_keys() } -> std::ranges::range;
};
```

## Usage Examples

### Basic Configuration

```cpp
#include "liarsdice/config/config_manager.hpp"
#include "liarsdice/config/game_config.hpp"

// Initialize configuration manager
ConfigManager config;

// Add sources (priority order: command line > env > file > defaults)
config.add_source(std::make_unique<DefaultsSource>(0));
config.add_source(std::make_unique<JsonFileSource>("config.json", 100));
config.add_source(std::make_unique<EnvironmentSource>("LIARSDICE_", 150));
config.add_source(std::make_unique<CommandLineSource>(argv, 200));

// Get values with type safety
auto max_players = config.get_value<uint32_t>(ConfigPath{"game.rules.max_players"});
auto theme = config.get_value<std::string>(ConfigPath{"ui.theme"});
auto enable_sound = config.get_value_or<bool>(ConfigPath{"sound.enabled"}, true);
```

### Game Configuration

```cpp
GameConfig game_config;

// Validate configuration
auto errors = game_config.validate_all();
if (!errors.empty()) {
    for (const auto& error : errors) {
        std::cerr << "Config error: " << error << std::endl;
    }
}

// Access typed configuration
std::cout << "Max players: " << game_config.rules.max_players << std::endl;
std::cout << "Theme: " << to_string(game_config.ui.theme) << std::endl;
```

### Custom Validators

```cpp
auto player_validator = make_range_validator<uint32_t>(2, 8);
auto difficulty_validator = make_enum_validator({
    Difficulty::Easy, Difficulty::Normal, Difficulty::Hard
});

if (!player_validator.validate(player_count)) {
    std::cerr << player_validator.get_error_message() << std::endl;
}
```

### Configuration Sources Priority

1. **Command Line** (priority 200): `--game.max-players=6`
2. **Environment** (priority 150): `LIARSDICE_GAME_MAX_PLAYERS=6`
3. **JSON File** (priority 100): `{"game": {"max_players": 6}}`
4. **Defaults** (priority 0): Fallback values

### Hot Reloading

```cpp
// Reload all sources
config.reload_sources();

// Check for changes
if (config.has_value(ConfigPath{"game.rules.max_players"})) {
    auto new_value = config.get_value<uint32_t>(ConfigPath{"game.rules.max_players"});
    // Handle configuration change
}
```

### Type Conversion

```cpp
ConfigValue value{"42"};

// Type-safe access
auto as_int = value.get<int32_t>();           // std::optional<int32_t>
auto as_string = value.get<std::string>();     // std::optional<std::string>

// With defaults
auto number = value.get_or<int32_t>(0);       // Returns 42 or 0
auto text = value.get_required<std::string>(); // Throws if not available
```

## Error Handling

```cpp
// Configuration exceptions
class ConfigException : public std::runtime_error {
    // Includes source location information
};

// Validation errors
std::vector<std::string> errors = game_config.validate_all();
```

## Conditional Compilation

Configuration can be disabled at compile-time:

```cmake
# Disable configuration system
set(LIARSDICE_ENABLE_CONFIG OFF)
```

When disabled, all configuration operations become no-ops or return defaults.

## Thread Safety

- ConfigManager is thread-safe for read operations
- Write operations should be synchronized externally
- Configuration sources are read-only after initialization
- Hot reloading requires external synchronization