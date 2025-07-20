#include "liarsdice/core/game.hpp"
#include <iostream>

int main() {
  std::cout << "LiarsDice Library Example\n";
  std::cout << "=========================\n\n";
  
  // Create a simple game example
  Game game;
  
  // Add some players
  game.addPlayer("Alice");
  game.addPlayer("Bob");
  
  std::cout << "Created a game with " << game.getPlayerCount() << " players.\n";
  
  // Display players
  auto players = game.getPlayers();
  for (const auto& player : players) {
    std::cout << "Player: " << player.getName() << std::endl;
  }
  
  std::cout << "\nThis is a basic example showing how to use the LiarsDice library.\n";
  std::cout << "For a full interactive game, run the liarsdice-cli executable.\n";
  
  return 0;
}