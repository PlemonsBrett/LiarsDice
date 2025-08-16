#pragma once

#include <boost/signals2.hpp>
#include <functional>
#include <iostream>
#include <liarsdice/ui/ui_config.hpp>
#include <memory>
#include <regex>

namespace liarsdice::ui {

  class MenuSystem {
  public:
    using MenuAction = std::function<void()>;
    using InputValidator = std::function<bool(const std::string&)>;

    explicit MenuSystem(std::shared_ptr<UIConfig> config);

    // Display functions
    void display_menu(const std::string& menu_id);
    void display_message(const std::string& message_id,
                         const std::unordered_map<std::string, std::string>& params = {});
    void display_text(const std::string& text_id);
    void display_error(const std::string& error);
    void clear_screen();

    // Input functions
    std::string get_input(const std::string& prompt_id);
    std::string get_input_with_validation(const std::string& prompt_id);
    bool confirm(const std::string& prompt_id);

    // Menu handling
    void register_action(const std::string& menu_id, const std::string& item_id, MenuAction action);
    void run_menu(const std::string& menu_id);
    void set_exit_requested(bool exit) { exit_requested_ = exit; }
    [[nodiscard]] bool is_exit_requested() const { return exit_requested_; }

    // Theme support
    void apply_color(const std::string& color_name);
    void reset_color();

  private:
    std::shared_ptr<UIConfig> config_;
    std::unordered_map<std::string, std::unordered_map<std::string, MenuAction>> menu_actions_;
    bool exit_requested_ = false;

    std::string apply_style(const std::string& text, const std::string& style);
    bool validate_input(const std::string& input, const std::string& pattern);
  };

}  // namespace liarsdice::ui