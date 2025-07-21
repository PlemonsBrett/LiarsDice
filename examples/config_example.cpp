/**
 * @file config_example.cpp
 * @brief Example demonstrating the configuration system
 */

#include <iostream>

#ifdef LIARSDICE_ENABLE_CONFIG
#include "liarsdice/config/config.hpp"

using namespace liarsdice::config;

int main() {
    std::cout << "=== LiarsDice Configuration System Example ===\n\n";
    
    // Initialize configuration system with defaults
    initialize_config_system();
    
    // Create a game configuration instance
    GameConfig game_config;
    game_config.load();
    
    // Display current configuration
    std::cout << "Current Configuration:\n";
    print_config_summary();
    
    // Demonstrate individual value access
    std::cout << "\n=== Individual Value Access ===\n";
    
    auto max_players = get_config<uint32_t>(ConfigPath{"game.rules.max_players"});
    if (max_players) {
        std::cout << "Max players: " << *max_players << "\n";
    }
    
    auto theme = get_config_or<std::string>(ConfigPath{"ui.theme"}, "auto");
    std::cout << "UI Theme: " << theme << "\n";
    
    auto port = get_config_or<uint32_t>(ConfigPath{"network.default_port"}, 7777);
    std::cout << "Network Port: " << port << "\n";
    
    // Demonstrate runtime configuration changes
    std::cout << "\n=== Runtime Configuration Changes ===\n";
    
    set_config<uint32_t>(ConfigPath{"game.rules.max_players"}, 8);
    std::cout << "Changed max players to: " << get_config<uint32_t>(ConfigPath{"game.rules.max_players"}).value_or(0) << "\n";
    
    set_config<std::string>(ConfigPath{"ui.theme"}, "dark");
    std::cout << "Changed theme to: " << get_config<std::string>(ConfigPath{"ui.theme"}).value_or("unknown") << "\n";
    
    // Demonstrate validation
    std::cout << "\n=== Configuration Validation ===\n";
    
    auto validation_errors = validate_all_config();
    if (validation_errors.empty()) {
        std::cout << "✓ All configuration is valid\n";
    } else {
        std::cout << "✗ Configuration errors found:\n";
        for (const auto& error : validation_errors) {
            std::cout << "  - " << error << "\n";
        }
    }
    
    // Demonstrate game-specific configuration access
    std::cout << "\n=== Game-Specific Configuration ===\n";
    
    game_config.load(); // Reload to get updated values
    std::cout << "Game variant: " << to_string(game_config.rules.variant) << "\n";
    std::cout << "Dice per player: " << game_config.rules.dice_per_player << "\n";
    std::cout << "Turn timeout: " << game_config.rules.turn_timeout.count() << " seconds\n";
    std::cout << "AI difficulty: " << to_string(game_config.ai.default_difficulty) << "\n";
    
    // Demonstrate configuration sections
    std::cout << "\n=== Configuration Sections ===\n";
    
    auto ui_section = global_config().get_section(ConfigPath{"ui"});
    std::cout << "UI configuration section:\n";
    for (const auto& [key, value] : ui_section) {
        std::cout << "  " << key << " = " << value.to_string() << "\n";
    }
    
    auto sound_section = global_config().get_section(ConfigPath{"sound"});
    std::cout << "Sound configuration section:\n";
    for (const auto& [key, value] : sound_section) {
        std::cout << "  " << key << " = " << value.to_string() << "\n";
    }
    
    std::cout << "\n=== Configuration System Demo Complete ===\n";
    
    return 0;
}

#else

int main() {
    std::cout << "Configuration system is disabled. Enable with -DLIARSDICE_ENABLE_CONFIG=ON\n";
    return 0;
}

#endif // LIARSDICE_ENABLE_CONFIG