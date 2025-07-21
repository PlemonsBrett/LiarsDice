#pragma once

/**
 * @file config.hpp
 * @brief Main configuration system header - includes all configuration functionality
 */

#include "config_concepts.hpp"
#include "config_value.hpp"
#include "config_manager.hpp"
#include "config_sources.hpp"
#include "game_config.hpp"

/**
 * @brief Configuration system for LiarsDice using modern C++23 features
 * 
 * This configuration system provides:
 * - Type-safe configuration values using std::variant and concepts
 * - Hierarchical configuration with dot notation (e.g., "game.rules.max_players")
 * - Multiple configuration sources (JSON files, environment variables, command line)
 * - Hot-reloading with filesystem monitoring
 * - Compile-time validation using C++23 concepts
 * - Game-specific configuration classes with type-safe enums
 * 
 * @example Basic Usage:
 * ```cpp
 * using namespace liarsdice::config;
 * 
 * // Initialize configuration manager
 * auto& config = global_config();
 * 
 * // Add configuration sources (in priority order - higher priority overrides lower)
 * config.add_source(std::make_unique<DefaultsSource>(10));           // Lowest priority
 * config.add_source(std::make_unique<JsonFileSource>("config.json", 100));
 * config.add_source(std::make_unique<EnvironmentSource>("GAME_", 150));
 * config.add_source(std::make_unique<CommandLineSource>(argc, argv, 200)); // Highest priority
 * 
 * // Access configuration values
 * auto max_players = get_config<uint32_t>(ConfigPath{"game.rules.max_players"});
 * auto timeout = get_config_or<int64_t>(ConfigPath{"game.rules.turn_timeout"}, 60);
 * 
 * // Use game-specific configuration
 * GameConfig game_config;
 * game_config.load();
 * std::cout << "Max players: " << game_config.rules.max_players << std::endl;
 * ```
 * 
 * @example Configuration Sources:
 * 
 * JSON File (config.json):
 * ```json
 * {
 *   "game": {
 *     "rules": {
 *       "max_players": 6,
 *       "dice_per_player": 5,
 *       "variant": "classic"
 *     }
 *   },
 *   "ui": {
 *     "theme": "dark",
 *     "show_animations": true
 *   }
 * }
 * ```
 * 
 * Environment Variables:
 * ```bash
 * export GAME_GAME_RULES_MAX_PLAYERS=8
 * export GAME_UI_THEME=light
 * ```
 * 
 * Command Line:
 * ```bash
 * ./liarsdice-cli --game.rules.max-players=4 --ui.theme=dark
 * ```
 * 
 * @example Hot-Reloading:
 * ```cpp
 * // Enable hot-reloading
 * config.set_hot_reload_enabled(true);
 * 
 * // Add change listener
 * config.add_change_listener("ui_updater", [](const ConfigChangeEvent& event) {
 *     if (event.path.to_string().starts_with("ui.")) {
 *         // Update UI when UI configuration changes
 *         update_ui_settings();
 *     }
 * });
 * ```
 * 
 * @example Validation:
 * ```cpp
 * // Validate all configuration
 * auto errors = config.validate();
 * if (!errors.empty()) {
 *     for (const auto& error : errors) {
 *         std::cerr << "Config error: " << error << std::endl;
 *     }
 * }
 * 
 * // Validate game-specific configuration
 * GameConfig game_config;
 * game_config.load();
 * auto game_errors = game_config.validate_all();
 * ```
 * 
 * @example Custom Validators:
 * ```cpp
 * using namespace liarsdice::config::validation;
 * 
 * // Built-in validators
 * auto percent_validator = percentage_validator(); // 0-100
 * auto port_validator = port_validator();          // 1024-65535
 * auto player_validator = player_count_validator(2, 8);
 * 
 * // Custom validator
 * auto custom_validator = ConfigValidator<std::string>(
 *     [](const std::string& value) { return value.length() >= 3; },
 *     []() { return "Value must be at least 3 characters"; }
 * );
 * ```
 */

namespace liarsdice::config {

/**
 * @brief Initialize the global configuration system with common sources
 * 
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @param config_file_path Path to JSON configuration file (optional)
 * @param env_prefix Prefix for environment variables (default: "LIARSDICE_")
 */
inline void initialize_config_system(int argc = 0, 
                                    char* argv[] = nullptr,
                                    const std::string& config_file_path = "config.json",
                                    const std::string& env_prefix = "LIARSDICE_") {
    auto& config = global_config();
    
    // Add default configuration source (lowest priority)
    auto defaults = std::make_unique<DefaultsSource>(10);
    defaults->add_defaults({
        {ConfigPath{"game.rules.min_players"}, "2"},
        {ConfigPath{"game.rules.max_players"}, "6"},
        {ConfigPath{"game.rules.dice_per_player"}, "5"},
        {ConfigPath{"game.rules.dice_faces"}, "6"},
        {ConfigPath{"game.rules.variant"}, "classic"},
        {ConfigPath{"game.rules.allow_spectators"}, "true"},
        {ConfigPath{"game.rules.turn_timeout"}, "60"},
        {ConfigPath{"game.rules.timeout_action"}, "extend"},
        {ConfigPath{"game.rules.show_dice_count"}, "false"},
        {ConfigPath{"game.rules.enable_undo"}, "false"},
        
        {ConfigPath{"ui.theme"}, "auto"},
        {ConfigPath{"ui.show_animations"}, "true"},
        {ConfigPath{"ui.show_tooltips"}, "true"},
        {ConfigPath{"ui.confirm_actions"}, "true"},
        {ConfigPath{"ui.animation_speed"}, "100"},
        {ConfigPath{"ui.language"}, "en"},
        {ConfigPath{"ui.accessibility_mode"}, "false"},
        {ConfigPath{"ui.font_scale"}, "100"},
        
        {ConfigPath{"sound.mode"}, "full"},
        {ConfigPath{"sound.master_volume"}, "70"},
        {ConfigPath{"sound.effects_volume"}, "80"},
        {ConfigPath{"sound.ambient_volume"}, "50"},
        {ConfigPath{"sound.mute_when_unfocused"}, "true"},
        
        {ConfigPath{"ai.default_difficulty"}, "normal"},
        {ConfigPath{"ai.enable_learning"}, "false"},
        {ConfigPath{"ai.show_thinking"}, "true"},
        {ConfigPath{"ai.delay_min_ms"}, "500"},
        {ConfigPath{"ai.delay_max_ms"}, "2000"},
        {ConfigPath{"ai.bluff_frequency"}, "0.15"},
        {ConfigPath{"ai.conservative_factor"}, "0.5"},
        
        {ConfigPath{"network.default_port"}, "7777"},
        {ConfigPath{"network.connection_timeout"}, "30"},
        {ConfigPath{"network.max_reconnect_attempts"}, "3"},
        {ConfigPath{"network.enable_lan_discovery"}, "true"},
        {ConfigPath{"network.server_region"}, "auto"}
    });
    config.add_source(std::move(defaults));
    
    // Add JSON file source (medium priority)
    if (std::filesystem::exists(config_file_path)) {
        config.add_source(std::make_unique<JsonFileSource>(config_file_path, 100));
    }
    
    // Add environment variables source (high priority)
    config.add_source(std::make_unique<EnvironmentSource>(env_prefix, 150));
    
    // Add command line arguments source (highest priority)
    if (argc > 0 && argv != nullptr) {
        config.add_source(std::make_unique<CommandLineSource>(argc, argv, 200));
    }
    
    // Enable hot-reloading by default
    config.set_hot_reload_enabled(true);
}

/**
 * @brief Get a fully configured GameConfig instance
 */
inline GameConfig get_game_config() {
    GameConfig config;
    config.load();
    return config;
}

/**
 * @brief Validate the entire configuration system
 */
inline std::vector<std::string> validate_all_config() {
    std::vector<std::string> errors;
    
    // Validate configuration manager
    auto manager_errors = global_config().validate();
    errors.insert(errors.end(), manager_errors.begin(), manager_errors.end());
    
    // Validate game configuration
    auto game_config = get_game_config();
    auto game_errors = game_config.validate_all();
    errors.insert(errors.end(), game_errors.begin(), game_errors.end());
    
    return errors;
}

/**
 * @brief Print configuration summary to output stream
 */
inline void print_config_summary(std::ostream& os = std::cout) {
    auto game_config = get_game_config();
    
    os << "=== LiarsDice Configuration Summary ===\n";
    os << game_config.rules.describe() << "\n\n";
    
    os << "UI Preferences:\n";
    os << "  Theme: " << to_string(game_config.ui.theme) << "\n";
    os << "  Language: " << game_config.ui.language << "\n";
    os << "  Animations: " << (game_config.ui.show_animations ? "Enabled" : "Disabled") << "\n";
    os << "  Animation Speed: " << game_config.ui.animation_speed << "%\n\n";
    
    os << "Sound Configuration:\n";
    os << "  Mode: " << to_string(game_config.sound.mode) << "\n";
    os << "  Master Volume: " << game_config.sound.master_volume << "%\n";
    os << "  Effects Volume: " << game_config.sound.effects_volume << "%\n\n";
    
    os << "AI Configuration:\n";
    os << "  Default Difficulty: " << to_string(game_config.ai.default_difficulty) << "\n";
    os << "  Learning: " << (game_config.ai.enable_ai_learning ? "Enabled" : "Disabled") << "\n";
    os << "  Bluff Frequency: " << (game_config.ai.bluff_frequency * 100) << "%\n\n";
    
    os << "Network Configuration:\n";
    os << "  Default Port: " << game_config.network.default_port << "\n";
    os << "  Connection Timeout: " << game_config.network.connection_timeout.count() << "s\n";
    os << "  LAN Discovery: " << (game_config.network.enable_lan_discovery ? "Enabled" : "Disabled") << "\n";
    
    os << "======================================\n";
}

} // namespace liarsdice::config