#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace liarsdice::ui {

// UI text configuration loaded from assets
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
    
    // Load configuration from file
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
    
private:
    std::unordered_map<std::string, Menu> menus_;
    std::unordered_map<std::string, Prompt> prompts_;
    std::unordered_map<std::string, Message> messages_;
    std::unordered_map<std::string, std::string> texts_;
    boost::property_tree::ptree theme_;
    
    std::string replace_placeholders(const std::string& template_text, 
                                   const std::unordered_map<std::string, std::string>& params) const;
};

} // namespace liarsdice::ui