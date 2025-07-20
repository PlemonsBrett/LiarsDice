#include "liarsdice/core/dice.hpp"
#include "liarsdice/core/player.hpp"
#include <iostream>

int main() {
  std::cout << "LiarsDice Library Example\n";
  std::cout << "=========================\n\n";
  
  // Create some dice
  Dice die1;
  Dice die2;
  die1.roll();
  die2.roll();
  
  std::cout << "Die 1 value: " << die1.get_face_value() << '\n';
  std::cout << "Die 2 value: " << die2.get_face_value() << '\n';
  
  // Create a player
  Player player(1);  // Player with ID 1
  
  std::cout << "\nPlayer ID: " << player.get_player_id() << '\n';
  
  // Display player's dice (they start with some dice by default)
  std::cout << "Player's dice count: " << player.get_dice().size() << '\n';
  
  std::cout << "Player's dice values: ";
  const auto& dice = player.get_dice();
  for (const auto& die : dice) {
    std::cout << die.get_face_value() << " ";
  }
  std::cout << '\n';
  
  // Roll the player's dice
  player.roll_dice();
  std::cout << "After rolling - dice values: ";
  for (const auto& die : player.get_dice()) {
    std::cout << die.get_face_value() << " ";
  }
  std::cout << '\n';
  
  std::cout << "\nThis is a basic example showing how to use the LiarsDice library.\n";
  std::cout << "For a full interactive game, run the liarsdice-cli executable.\n";
  
  return 0;
}