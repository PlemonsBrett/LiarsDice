#pragma once

/**
 * @file interactive_prompt.hpp
 * @brief Interactive prompts with ANSI color support (cross-platform)
 */

#include "../validation/sanitizers.hpp"
#include "../validation/validation_concepts.hpp"
#include "../validation/validators.hpp"
#include "fuzzy_match.hpp"
#include "input_history.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <liarsdice/utils/format_helper.hpp>
#include <optional>
#include <string>
#include <thread>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace liarsdice::ui {

/**
 * @brief ANSI color codes
 */
namespace colors {
constexpr auto reset = "\033[0m";
constexpr auto bold = "\033[1m";
constexpr auto dim = "\033[2m";
constexpr auto underline = "\033[4m";
constexpr auto blink = "\033[5m";
constexpr auto reverse = "\033[7m";

// Foreground colors
constexpr auto black = "\033[30m";
constexpr auto red = "\033[31m";
constexpr auto green = "\033[32m";
constexpr auto yellow = "\033[33m";
constexpr auto blue = "\033[34m";
constexpr auto magenta = "\033[35m";
constexpr auto cyan = "\033[36m";
constexpr auto white = "\033[37m";

// Bright foreground colors
constexpr auto bright_black = "\033[90m";
constexpr auto bright_red = "\033[91m";
constexpr auto bright_green = "\033[92m";
constexpr auto bright_yellow = "\033[93m";
constexpr auto bright_blue = "\033[94m";
constexpr auto bright_magenta = "\033[95m";
constexpr auto bright_cyan = "\033[96m";
constexpr auto bright_white = "\033[97m";

// Background colors
constexpr auto bg_black = "\033[40m";
constexpr auto bg_red = "\033[41m";
constexpr auto bg_green = "\033[42m";
constexpr auto bg_yellow = "\033[43m";
constexpr auto bg_blue = "\033[44m";
constexpr auto bg_magenta = "\033[45m";
constexpr auto bg_cyan = "\033[46m";
constexpr auto bg_white = "\033[47m";
} // namespace colors

/**
 * @brief Terminal utilities
 */
class Terminal {
public:
  /**
   * @brief Enable ANSI colors on Windows
   */
  static void enable_ansi_colors() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
  }

  /**
   * @brief Clear screen
   */
  static void clear() {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[1;1H" << std::flush;
#endif
  }

  /**
   * @brief Move cursor to position
   */
  static void move_cursor(int row, int col) {
    std::cout << "\033[" << row << ";" << col << "H" << std::flush;
  }

  /**
   * @brief Get terminal width
   */
  static int get_width() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
#endif
  }

  /**
   * @brief Hide cursor
   */
  static void hide_cursor() { std::cout << "\033[?25l" << std::flush; }

  /**
   * @brief Show cursor
   */
  static void show_cursor() { std::cout << "\033[?25h" << std::flush; }
};

/**
 * @brief Prompt style configuration
 */
struct PromptStyle {
  std::string prompt_color = colors::bright_green;
  std::string input_color = colors::white;
  std::string error_color = colors::bright_red;
  std::string warning_color = colors::bright_yellow;
  std::string success_color = colors::bright_green;
  std::string hint_color = colors::bright_black;
  std::string border_char = "─";
  bool show_hints = true;
  bool show_validation_inline = true;
};

/**
 * @brief Interactive prompt manager
 */
class InteractivePrompt {
public:
  explicit InteractivePrompt(PromptStyle style = {}) : style_(std::move(style)) {
    Terminal::enable_ansi_colors();
  }

  /**
   * @brief Basic string prompt
   */
  [[nodiscard]] std::string prompt(const std::string &message,
                                   const std::string &default_value = "") {
    std::cout << style_.prompt_color << message;

    if (!default_value.empty()) {
      std::cout << " [" << default_value << "]";
    }

    std::cout << ": " << colors::reset << style_.input_color;

    std::string input;
    std::getline(std::cin, input);

    std::cout << colors::reset;

    if (input.empty() && !default_value.empty()) {
      return default_value;
    }

    history_.add(input, "prompt");
    return input;
  }

  /**
   * @brief Validated prompt
   */
  template <typename T, typename Validator>
  requires validation::Validator<Validator, T>
  [[nodiscard]] validation::ValidationResult<T>
  prompt_validated(const std::string &message, const Validator &validator,
                   std::optional<T> default_value = std::nullopt, int max_retries = 3) {

    for (int attempt = 0; attempt < max_retries; ++attempt) {
      std::string prompt_msg = message;

      if (attempt > 0) {
        prompt_msg = message + " (attempt " + std::to_string(attempt + 1) + "/" +
                     std::to_string(max_retries) + ")";
      }

      std::string default_str;
      if (default_value) {
        if constexpr (std::is_same_v<T, std::string>) {
          default_str = *default_value;
        } else {
          if constexpr (std::is_same_v<T, std::string>) {
            default_str = *default_value;
          } else {
            default_str = std::to_string(*default_value);
          }
        }
      }

      std::string input = prompt(prompt_msg, default_str);

      // Parse input to type T
      T value;
      if constexpr (std::is_same_v<T, std::string>) {
        value = input;
      } else if constexpr (std::is_integral_v<T>) {
        try {
          if constexpr (std::is_unsigned_v<T>) {
            value = std::stoull(input);
          } else {
            value = std::stoll(input);
          }
        } catch (...) {
          show_error("Invalid number format");
          continue;
        }
      } else {
        // Requires StringParseable concept
        static_assert(validation::StringParseable<T>, "Type must be string parseable");
        auto parsed = T::parse(input);
        if (!parsed) {
          show_error("Invalid format");
          continue;
        }
        value = *parsed;
      }

      // Validate
      auto result = validator.validate(value);
      if (result) {
        return result;
      }

      // Show validation errors
      show_validation_errors(result.error());
    }

    return std::unexpected(validation::ValidationErrors{{validation::ValidationError{
        .field_name = "input", .error_message = "Maximum retry attempts exceeded"}}});
  }

  /**
   * @brief Yes/No prompt
   */
  [[nodiscard]] bool prompt_yes_no(const std::string &message,
                                   std::optional<bool> default_value = std::nullopt) {
    std::string prompt_msg = message;
    std::string default_str;

    if (default_value) {
      prompt_msg += *default_value ? " [Y/n]" : " [y/N]";
      default_str = *default_value ? "y" : "n";
    } else {
      prompt_msg += " [y/n]";
    }

    while (true) {
      std::string input = prompt(prompt_msg, default_str);

      using namespace validation::sanitizers;
      std::string clean = chain(trim(), lowercase())(input);

      if (clean == "y" || clean == "yes")
        return true;
      if (clean == "n" || clean == "no")
        return false;

      show_error("Please answer 'yes' or 'no' (or y/n)");
    }
  }

  /**
   * @brief Menu selection
   */
  [[nodiscard]] std::size_t prompt_menu(const std::string &title,
                                        const std::vector<std::string> &options,
                                        std::size_t default_index = 0) {
    draw_box(title);

    for (std::size_t i = 0; i < options.size(); ++i) {
      std::cout << style_.prompt_color << "  " << (i + 1) << ". " << colors::reset << options[i]
                << "\n";
    }

    std::cout << "\n";

    auto validator = validation::validators::range(std::size_t(1), options.size(), "selection");

    while (true) {
      std::string prompt_msg = "Select option";
      if (default_index < options.size()) {
        prompt_msg += " [" + std::to_string(default_index + 1) + "]";
      }

      std::string input = prompt(
          prompt_msg, default_index < options.size() ? std::to_string(default_index + 1) : "");

      try {
        std::size_t selection = std::stoull(input);
        auto result = validator.validate(selection);

        if (result) {
          return selection - 1; // Convert to 0-based index
        }

        show_validation_errors(result.error());
      } catch (...) {
        show_error("Please enter a valid number");
      }
    }
  }

  /**
   * @brief Progress bar
   */
  void show_progress(const std::string &message, double progress, int width = 40) {
    std::cout << "\r" << style_.prompt_color << message << ": " << colors::reset;

    int filled = static_cast<int>(progress * width);
    std::cout << "[";

    for (int i = 0; i < width; ++i) {
      if (i < filled) {
        std::cout << style_.success_color << "█" << colors::reset;
      } else {
        std::cout << style_.hint_color << "░" << colors::reset;
      }
    }

    std::cout << "] " << std::fixed << std::setprecision(0) << std::setw(3) << (progress * 100)
              << "%" << std::flush;

    if (progress >= 1.0) {
      std::cout << "\n";
    }
  }

  /**
   * @brief Show error message
   */
  void show_error(const std::string &message) {
    std::cout << style_.error_color << "✗ Error: " << message << colors::reset << "\n";
  }

  /**
   * @brief Show warning message
   */
  void show_warning(const std::string &message) {
    std::cout << style_.warning_color << "⚠ Warning: " << message << colors::reset << "\n";
  }

  /**
   * @brief Show success message
   */
  void show_success(const std::string &message) {
    std::cout << style_.success_color << "✓ " << message << colors::reset << "\n";
  }

  /**
   * @brief Show hint
   */
  void show_hint(const std::string &hint) {
    if (style_.show_hints) {
      std::cout << style_.hint_color << "💡 Hint: " << hint << colors::reset << "\n";
    }
  }

  /**
   * @brief Draw a box
   */
  void draw_box(const std::string &title, int width = 0) {
    if (width == 0) {
      width = Terminal::get_width() - 2;
    }

    // Top border
    std::cout << style_.prompt_color << "┌";

    if (!title.empty()) {
      std::string padded_title = " " + title + " ";
      int border_width = (width - padded_title.length()) / 2;

      for (int i = 0; i < border_width; ++i) {
        std::cout << style_.border_char;
      }

      std::cout << colors::bold << padded_title << colors::reset << style_.prompt_color;

      for (int i = 0; i < width - border_width - padded_title.length(); ++i) {
        std::cout << style_.border_char;
      }
    } else {
      for (int i = 0; i < width; ++i) {
        std::cout << style_.border_char;
      }
    }

    std::cout << "┐" << colors::reset << "\n";
  }

  /**
   * @brief Get input history
   */
  [[nodiscard]] InputHistory &history() { return history_; }
  [[nodiscard]] const InputHistory &history() const { return history_; }

private:
  PromptStyle style_;
  InputHistory history_;
  CommandSuggester suggester_{create_game_command_suggester()};

  void show_validation_errors(const validation::ValidationErrors &errors) {
    for (const auto &error : errors) {
      show_error(error.to_string());
    }

    // Show suggestions if available
    if (!errors.empty() && style_.show_hints) {
      auto suggestions = suggester_.format_suggestions(errors[0].field_name);
      if (!suggestions.empty()) {
        show_hint(suggestions);
      }
    }
  }
};

/**
 * @brief Spinner animation
 */
class Spinner {
public:
  explicit Spinner(const std::string &message = "Loading") : message_(message), running_(false) {}

  void start() {
    running_ = true;
    thread_ = std::thread([this]() {
      const std::vector<std::string> frames = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
      std::size_t frame = 0;

      Terminal::hide_cursor();

      while (running_) {
        std::cout << "\r" << colors::bright_cyan << frames[frame] << colors::reset << " "
                  << message_ << std::flush;

        frame = (frame + 1) % frames.size();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      std::cout << "\r" << std::string(message_.length() + 3, ' ') << "\r" << std::flush;
      Terminal::show_cursor();
    });
  }

  void stop() {
    running_ = false;
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  ~Spinner() { stop(); }

private:
  std::string message_;
  std::atomic<bool> running_;
  std::thread thread_;
};

} // namespace liarsdice::ui