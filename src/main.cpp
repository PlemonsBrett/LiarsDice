#include "controller/Game.hpp"
#include <iostream>
#include <limits>



const std::string PLAY_AGAIN_YES = "yes";
const std::string WELCOME_MESSAGE = "Welcome to Liar's Dice!\n";
const std::string GOODBYE_MESSAGE = "Thank you for playing Liar's Dice!\n";
const std::string PLAY_AGAIN_PROMPT = "Do you want to play again? (yes/no): ";

int main() {
  std::string playAgain;

  // Display the welcome message
  std::cout << WELCOME_MESSAGE;

  // Initialize the game
  Game game;

  do {
    // Start the game
    game.Init();

    // Prompt the user to play again
    std::cout << PLAY_AGAIN_PROMPT;
    std::cin >> playAgain;

    // Clear the input buffer to ensure proper functioning of cin in the next iteration
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  } while (playAgain == PLAY_AGAIN_YES);

  // Display the goodbye message
  std::cout << GOODBYE_MESSAGE;

  return 0;
}

