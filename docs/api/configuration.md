# Configuration System API

## Overview

The LiarsDice configuration system uses Boost.PropertyTree to provide hierarchical configuration management for UI elements, game settings, and theme customization.

## Core Components

### UIConfig Class

**Location**: `include/liarsdice/ui/ui_config.hpp`

The `UIConfig` class manages all UI-related configuration including menus, prompts, messages, and theming.

{% raw %}
```cpp
namespace liarsdice::ui {
    class UIConfig {
    public:
        struct MenuItem {
            std::string id;
            std::string label;
            std::string shortcut;
            std::string description;
        };
        
        struct Menu {
            std::string id;
            std::string title;
            std::string prompt;
            std::vector<MenuItem> items;
            std::optional<std::string> back_option;
        };
        
        struct Prompt {
            std::string id;
            std::string text;
            std::string default_value;
            std::string validation_pattern;
            std::string error_message;
        };
        
        struct Message {
            std::string id;
            std::string template_text;
            std::vector<std::string> placeholders;
        };
        
        // Configuration loading
        void load_from_file(const std::string& filepath);
        void load_from_info(const boost::property_tree::ptree& pt);
        
        // Accessors
        [[nodiscard]] const Menu* get_menu(const std::string& menu_id) const;
        [[nodiscard]] const Prompt* get_prompt(const std::string& prompt_id) const;
        [[nodiscard]] std::string get_message(const std::string& message_id, 
                                             const std::unordered_map<std::string, std::string>& params = {}) const;
        [[nodiscard]] std::string get_text(const std::string& text_id) const;
        
        // Theme and styling
        [[nodiscard]] std::string get_color(const std::string& element) const;
        [[nodiscard]] std::string get_style(const std::string& element) const;
    };
}
```
{% endraw %}

## Configuration File Format

The configuration uses Boost.PropertyTree's INFO format, which is a simple hierarchical text format:

{% raw %}
```
; UI Configuration for Liar's Dice

menus {
    main_menu {
        title "Liar's Dice - Main Menu"
        prompt "Select an option:"
        
        items {
            new_game {
                label "New Game"
                shortcut "N"
                description "Start a new game of Liar's Dice"
            }
            
            settings {
                label "Settings"
                shortcut "S"
                description "Configure game settings"
            }
            
            rules {
                label "Game Rules"
                shortcut "R"
                description "View the rules of Liar's Dice"
            }
            
            quit {
                label "Quit"
                shortcut "Q"
                description "Exit the game"
            }
        }
    }
}

prompts {
    player_name {
        text "Enter your name:"
        default_value "Player"
        validation_pattern "[A-Za-z0-9 ]+"
        error_message "Name must contain only letters, numbers, and spaces"
    }
    
    num_players {
        text "How many players? (2-8):"
        default_value "2"
        validation_pattern "[2-8]"
        error_message "Number of players must be between 2 and 8"
    }
}

messages {
    welcome {
        template_text "Welcome to Liar's Dice, {player_name}!"
        placeholders "player_name"
    }
    
    round_start {
        template_text "Round {round_number} begins!"
        placeholders "round_number"
    }
}

theme {
    colors {
        primary "#3498db"
        secondary "#2ecc71"
        danger "#e74c3c"
        warning "#f39c12"
        info "#34495e"
    }
    
    styles {
        header "bold"
        menu_item "normal"
        prompt "bold cyan"
        error "bold red"
        success "bold green"
    }
}
```
{% endraw %}

## Usage Examples

### Loading Configuration

{% raw %}
```cpp
#include <liarsdice/ui/ui_config.hpp>

// Load from file
liarsdice::ui::UIConfig config;
config.load_from_file("assets/ui_config.info");

// Or load from property tree
boost::property_tree::ptree pt;
boost::property_tree::read_info("config.info", pt);
config.load_from_info(pt);
```
{% endraw %}

### Accessing Menu Configuration

{% raw %}
```cpp
// Get main menu
auto* main_menu = config.get_menu("main_menu");
if (main_menu) {
    std::cout << main_menu->title << "\n";
    std::cout << main_menu->prompt << "\n\n";
    
    for (const auto& item : main_menu->items) {
        std::cout << "[" << item.shortcut << "] " 
                  << item.label << " - " 
                  << item.description << "\n";
    }
}
```
{% endraw %}

### Using Prompts

{% raw %}
```cpp
// Get player name prompt
auto* name_prompt = config.get_prompt("player_name");
if (name_prompt) {
    std::cout << name_prompt->text << " ";
    
    std::string input;
    std::getline(std::cin, input);
    
    // Validate input
    std::regex pattern(name_prompt->validation_pattern);
    if (!std::regex_match(input, pattern)) {
        std::cout << name_prompt->error_message << "\n";
    }
}
```
{% endraw %}

### Formatted Messages

{% raw %}
```cpp
// Display welcome message
std::unordered_map<std::string, std::string> params = {
    {"player_name", "Alice"}
};

std::string welcome = config.get_message("welcome", params);
std::cout << welcome << "\n"; // Output: "Welcome to Liar's Dice, Alice!"

// Display round start message
params = {{"round_number", "3"}};
std::string round_msg = config.get_message("round_start", params);
std::cout << round_msg << "\n"; // Output: "Round 3 begins!"
```
{% endraw %}

### Theme Access

{% raw %}
```cpp
// Get colors
std::string primary_color = config.get_color("primary");
std::string error_color = config.get_color("danger");

// Get styles
std::string header_style = config.get_style("header");
std::string error_style = config.get_style("error");

// Apply styling (implementation depends on terminal library)
apply_style(header_style);
std::cout << "LIAR'S DICE\n";
reset_style();
```
{% endraw %}

## Integration with Game

The configuration system integrates with the menu system and game UI:

{% raw %}
```cpp
class MenuSystem {
private:
    UIConfig config_;
    
public:
    MenuSystem(const std::string& config_file) {
        config_.load_from_file(config_file);
    }
    
    void display_menu(const std::string& menu_id) {
        auto* menu = config_.get_menu(menu_id);
        if (!menu) {
            throw std::runtime_error("Menu not found: " + menu_id);
        }
        
        // Display menu using configuration
        clear_screen();
        display_header(menu->title);
        std::cout << "\n" << menu->prompt << "\n\n";
        
        for (const auto& item : menu->items) {
            display_menu_item(item);
        }
    }
};
```
{% endraw %}

## Boost.PropertyTree Features

The configuration system leverages several Boost.PropertyTree features:

- **Hierarchical Structure**: Nested configuration sections
- **Multiple Formats**: Support for INFO, XML, JSON, and INI formats
- **Type Safety**: Automatic type conversion with error handling
- **Default Values**: Fallback values for missing configuration
- **Comments**: Configuration files can include comments for documentation

## Error Handling

{% raw %}
```cpp
try {
    config.load_from_file("config.info");
} catch (const boost::property_tree::info_parser_error& e) {
    std::cerr << "Failed to parse configuration: " << e.what() << "\n";
    std::cerr << "File: " << e.filename() << ", Line: " << e.line() << "\n";
} catch (const std::exception& e) {
    std::cerr << "Configuration error: " << e.what() << "\n";
}
```
{% endraw %}

## Performance Considerations

- Configuration is loaded once at startup
- All lookups are O(1) using hash maps
- Message formatting uses simple string replacement
- No dynamic allocation during normal operation

## See Also

- [UI Menu System](../architecture/ui-system.md) - Menu system architecture
- [Core Game API](core.md) - Game configuration integration
- [Boost.PropertyTree Documentation](https://www.boost.org/doc/libs/release/doc/html/property_tree.html)