#include "liarsdice/core/dice.hpp"
#include "liarsdice/core/player.hpp"
#include <iostream>

int main() {
  std::cout << "LiarsDice Library Example\n";
  std::cout << "=========================\n\n";
  
  // Create some dice
  Dice die1;
  Dice die2;
  die1.Roll();
  die2.Roll();
  
  std::cout << "Die 1 value: " << die1.GetFaceValue() << std::endl;
  std::cout << "Die 2 value: " << die2.GetFaceValue() << std::endl;
  
  // Create a player
  Player player(1);  // Player with ID 1
  
  std::cout << "\nPlayer ID: " << player.GetPlayerId() << std::endl;
  
  // Display player's dice (they start with some dice by default)
  std::cout << "Player's dice count: " << player.GetDice().size() << std::endl;
  
  std::cout << "Player's dice values: ";
  const auto& dice = player.GetDice();
  for (const auto& die : dice) {
    std::cout << die.GetFaceValue() << " ";
  }
  std::cout << std::endl;
  
  // Roll the player's dice
  player.RollDice();
  std::cout << "After rolling - dice values: ";
  for (const auto& die : player.GetDice()) {
    std::cout << die.GetFaceValue() << " ";
  }
  std::cout << std::endl;
  
  std::cout << "\nThis is a basic example showing how to use the LiarsDice library.\n";
  std::cout << "For a full interactive game, run the liarsdice-cli executable.\n";
  
  return 0;
}