#include "liarsdice/core/game.hpp"
#include <iostream>
#include <limits>

const std::string kPlayAgainYes = "yes";
const std::string kWelcomeMessage = "Welcome to Liar's Dice!\n";
const std::string kGoodbyeMessage = "Thank you for playing Liar's Dice!\n";
const std::string kPlayAgainPrompt = "Do you want to play again? (yes/no): ";

int main() {
  std::string play_again;
  bool should_continue = true;

  // Display the welcome message
  std::cout << kWelcomeMessage;

  // Initialize the game
  Game game;

  while (should_continue) {
    // Start the game
    game.init();

    // Prompt the user to play again
    std::cout << kPlayAgainPrompt;
    std::cin >> play_again;

    // Clear the input buffer to ensure proper functioning of cin in the next iteration
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    should_continue = (play_again == kPlayAgainYes);
  }

  // Display the goodbye message
  std::cout << kGoodbyeMessage;

  return 0;
}
