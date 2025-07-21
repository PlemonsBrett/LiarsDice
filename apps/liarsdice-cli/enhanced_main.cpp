#include "liarsdice/core/game.hpp"
#include "liarsdice/ui/game_input.hpp"
#include "liarsdice/ui/input_history.hpp"
#include "liarsdice/ui/interactive_prompt.hpp"
#include <filesystem>
#include <iostream>
#include <liarsdice/utils/format_helper.hpp>
#include <memory>

#ifdef LIARSDICE_ENABLE_LOGGING
#include "liarsdice/logging/logging.hpp"
#endif

#ifdef LIARSDICE_ENABLE_CONFIG
#include "liarsdice/config/config_manager.hpp"
#include "liarsdice/config/game_config.hpp"
#endif

using namespace liarsdice;
using namespace liarsdice::ui;

class EnhancedLiarsDiceApp {
public:
  EnhancedLiarsDiceApp() : prompt_(create_default_style()) {
#ifdef LIARSDICE_ENABLE_LOGGING
    logging_system_ = std::make_unique<logging::LoggingSystem>("production");
    logger_ = logging::get_default_logger();
    LOG_INFO(*logger_, "=== Enhanced LiarsDice Application Started ===");
#endif

#ifdef LIARSDICE_ENABLE_CONFIG
    setup_configuration();
#endif

    // Load history
    auto history_path = get_history_path();
    if (std::filesystem::exists(history_path)) {
      prompt_.history().load_from_file(history_path);
    }
  }

  ~EnhancedLiarsDiceApp() {
    // Save history
    auto history_path = get_history_path();
    prompt_.history().save_to_file(history_path);

#ifdef LIARSDICE_ENABLE_LOGGING
    LOG_INFO(*logger_, "=== Enhanced LiarsDice Application Ended ===");
#endif
  }

  void run() {
    show_welcome();

    bool should_continue = true;
    while (should_continue) {
      if (show_main_menu()) {
        play_game();
      } else {
        should_continue = false;
      }
    }

    show_goodbye();
  }

private:
  InteractivePrompt prompt_;
  std::unique_ptr<Game> game_;

#ifdef LIARSDICE_ENABLE_LOGGING
  std::unique_ptr<logging::LoggingSystem> logging_system_;
  std::shared_ptr<logging::ILogger> logger_;
#endif

#ifdef LIARSDICE_ENABLE_CONFIG
  std::unique_ptr<config::ConfigManager> config_manager_;
  config::GameConfig game_config_;
#endif

  static PromptStyle create_default_style() {
    PromptStyle style;
    style.prompt_color = colors::bright_cyan;
    style.input_color = colors::bright_white;
    style.error_color = colors::bright_red;
    style.success_color = colors::bright_green;
    style.hint_color = colors::bright_black;
    return style;
  }

  [[nodiscard]] static std::filesystem::path get_history_path() {
    std::filesystem::path home_dir;

#ifdef _WIN32
    const char *userprofile = std::getenv("USERPROFILE");
    if (userprofile) {
      home_dir = userprofile;
    }
#else
    const char *home = std::getenv("HOME");
    if (home) {
      home_dir = home;
    }
#endif

    if (!home_dir.empty()) {
      auto config_dir = home_dir / ".liarsdice";
      std::filesystem::create_directories(config_dir);
      return config_dir / "history.txt";
    }

    return "liarsdice_history.txt";
  }

#ifdef LIARSDICE_ENABLE_CONFIG
  void setup_configuration() {
    config_manager_ = std::make_unique<config::ConfigManager>();

    // Add configuration sources
    config_manager_->add_source(std::make_unique<config::DefaultsSource>(0));

    auto config_path = get_config_path();
    if (std::filesystem::exists(config_path)) {
      config_manager_->add_source(
          std::make_unique<config::JsonFileSource>(config_path.string(), 100));
    }

    config_manager_->add_source(std::make_unique<config::EnvironmentSource>("LIARSDICE_", 150));

    // Load game configuration
    load_game_config();
  }

  [[nodiscard]] static std::filesystem::path get_config_path() {
    auto history_path = get_history_path();
    return history_path.parent_path() / "config.json";
  }

  void load_game_config() {
    // Load configuration values
    game_config_.rules.max_players = config_manager_->get_value_or<uint32_t>(
        config::ConfigPath{"game.rules.max_players"}, game_config_.rules.max_players);

    game_config_.rules.dice_per_player = config_manager_->get_value_or<uint32_t>(
        config::ConfigPath{"game.rules.dice_per_player"}, game_config_.rules.dice_per_player);

    auto theme_str =
        config_manager_->get_value_or<std::string>(config::ConfigPath{"ui.theme"}, "auto");

    if (auto theme = config::parse_ui_theme(theme_str)) {
      game_config_.ui.theme = *theme;
    }
  }
#endif

  void show_welcome() {
    Terminal::clear();
    prompt_.draw_box("Welcome to Liar's Dice!");

    std::cout << R"(
      🎲 🎲 🎲 🎲 🎲
    
    A game of bluffing and probability
    
    Rules:
    • Each player starts with 5 dice
    • Players make bids about all dice in play
    • Call "liar" to challenge the previous bid
    • Lose a die if you're wrong
    • Last player with dice wins!
        )" << "\n\n";

    prompt_.show_hint("Type 'help' at any time for assistance");
  }

  bool show_main_menu() {
    const std::vector<std::string> options = {"Start New Game", "View Statistics", "Settings",
                                              "Help", "Quit"};

    auto selection = prompt_.prompt_menu("Main Menu", options);

    switch (selection) {
    case 0: // Start New Game
      return true;

    case 1: // View Statistics
      show_statistics();
      return show_main_menu();

    case 2: // Settings
      show_settings();
      return show_main_menu();

    case 3: // Help
      show_help();
      return show_main_menu();

    case 4: // Quit
    default:
      return false;
    }
  }

  void play_game() {
    // Get number of players
    auto player_count_validator = create_player_count_validator(2, 6);
    auto player_count_result =
        prompt_.prompt_validated<uint32_t>("How many players", player_count_validator, 2u);

    if (!player_count_result) {
      prompt_.show_error("Failed to get player count");
      return;
    }

    uint32_t num_players = *player_count_result;

    // Get player names
    std::vector<std::string> player_names;
    auto name_validator = create_player_name_validator();

    for (uint32_t i = 0; i < num_players; ++i) {
      auto name_result =
          prompt_.prompt_validated<std::string>("Enter name for Player " + std::to_string(i + 1),
                                                name_validator, "Player" + std::to_string(i + 1));

      if (!name_result) {
        prompt_.show_error("Failed to get player name");
        return;
      }

      player_names.push_back(*name_result);
    }

    // Initialize enhanced game
    prompt_.show_success("Starting game with " + std::to_string(num_players) + " players");

    // TODO: Create enhanced game class that uses the new input system
    game_ = std::make_unique<Game>();
    game_->init();

    // After game ends
    if (prompt_.prompt_yes_no("Would you like to play again?", false)) {
      play_game();
    }
  }

  void show_statistics() {
    prompt_.draw_box("Game Statistics");

    // Analyze command frequency
    CommandFrequencyAnalyzer analyzer;
    auto most_common = analyzer.get_most_common(prompt_.history(), 10);

    if (!most_common.empty()) {
      std::cout << "\nMost Common Commands:\n";
      for (const auto &cmd : most_common) {
        std::cout << "  • " << cmd << "\n";
      }
    } else {
      std::cout << "\nNo statistics available yet.\n";
    }

    std::cout << "\nPress Enter to continue...";
    std::cin.get();
  }

  void show_settings() {
    prompt_.draw_box("Settings");

#ifdef LIARSDICE_ENABLE_CONFIG
    std::cout << "\nCurrent Settings:\n";
    std::cout << "  Max Players: " << game_config_.rules.max_players << "\n";
    std::cout << "  Dice per Player: " << game_config_.rules.dice_per_player << "\n";
    std::cout << "  UI Theme: " << config::to_string(game_config_.ui.theme) << "\n";
    std::cout << "  Sound: " << (game_config_.sound.enabled ? "Enabled" : "Disabled") << "\n";
#else
    std::cout << "\nConfiguration system not enabled.\n";
#endif

    std::cout << "\nPress Enter to continue...";
    std::cin.get();
  }

  void show_help() {
    prompt_.draw_box("Help");

    std::cout << R"(
Game Commands:
  • Enter bid as "3 5" (3 dice showing 5)
  • Or use dice notation: "3d5"
  • Type "liar" to challenge the previous bid
  • Type "help" to see this message
  • Type "history" to see previous bids
  • Type "quit" to exit the game

Tips:
  • Pay attention to the total number of dice
  • Remember that 1s are wild (count as any value)
  • Bluff strategically but don't get caught!
        )" << "\n";

    std::cout << "\nPress Enter to continue...";
    std::cin.get();
  }

  void show_goodbye() {
    Terminal::clear();

    std::cout << colors::bright_cyan;
    std::cout << R"(
    Thanks for playing Liar's Dice!
    
         🎲 See you next time! 🎲
        )" << colors::reset
              << "\n\n";

    // Show a spinner while saving
    {
      Spinner spinner("Saving game data");
      spinner.start();
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    prompt_.show_success("Game data saved successfully!");
  }
};

int main() {
  try {
    EnhancedLiarsDiceApp app;
    app.run();
  } catch (const std::exception &e) {
    std::cerr << colors::bright_red << "Fatal error: " << e.what() << colors::reset << std::endl;
    return 1;
  }

  return 0;
}