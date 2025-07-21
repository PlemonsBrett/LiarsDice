#include "liarsdice/core/game.hpp"
#include <iostream>
#include <limits>

#ifdef LIARSDICE_ENABLE_LOGGING
#include "liarsdice/logging/logging.hpp"
#endif

const std::string kPlayAgainYes = "yes";
const std::string kWelcomeMessage = "Welcome to Liar's Dice!\n";
const std::string kGoodbyeMessage = "Thank you for playing Liar's Dice!\n";
const std::string kPlayAgainPrompt = "Do you want to play again? (yes/no): ";

int main() {
#ifdef LIARSDICE_ENABLE_LOGGING
  // Initialize logging system for development environment
  liarsdice::logging::LoggingSystem logging_system("development");
  auto logger = liarsdice::logging::get_default_logger();
  
  LOG_INFO(*logger, "=== LiarsDice CLI Application Started ===");
  LOG_INFO(*logger, "Logging system initialized successfully");
#endif

  std::string play_again;
  bool should_continue = true;

  // Display the welcome message
  std::cout << kWelcomeMessage;
#ifdef LIARSDICE_ENABLE_LOGGING
  LOG_INFO(*logger, "Welcome message displayed to user");
#endif

  // Initialize the game
  Game game;

  while (should_continue) {
#ifdef LIARSDICE_ENABLE_LOGGING
    LOG_INFO(*logger, "Starting new game session");
#endif
    // Start the game
    game.init();

    // Prompt the user to play again
    std::cout << kPlayAgainPrompt;
    std::cin >> play_again;

    // Clear the input buffer to ensure proper functioning of cin in the next iteration
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    should_continue = (play_again == kPlayAgainYes);
#ifdef LIARSDICE_ENABLE_LOGGING
    LOG_INFO(*logger, "User play again response: '{}', continuing: {}", play_again, should_continue);
#endif
  }

  // Display the goodbye message
  std::cout << kGoodbyeMessage;
#ifdef LIARSDICE_ENABLE_LOGGING
  LOG_INFO(*logger, "=== LiarsDice CLI Application Ended ===");
#endif

  return 0;
}
