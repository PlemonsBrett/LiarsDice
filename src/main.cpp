#include "Game.hpp"
#include <iostream>

int main() {
  std::string playAgain;

  // Show Welcome Message
  std::cout << "Welcome to Liar's Dice!\n";

  do {
    // Initialize the Game
    Game game;
    game.Init();

    // Ask the user if they want to play again
    std:: cout << "Do you want to play again? (yes/no): ";
    std::cin >> playAgain;

    // Clear the input buffer to make sure cin works correctly in the next iteration
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  } while (playAgain == "yes");

  // Goodbye Message;
  std::cout << "Thank you for playing Liar's Dice!\n";

  return 0;
}
