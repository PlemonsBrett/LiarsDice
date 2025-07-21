//
// Created by Brett on 9/4/2023.
// This class represents a player in the game of Liar's Dice.
//

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "liarsdice/core/dice.hpp"
#include <vector>

class Player {
public:
  // Constructor initializes the player with an ID
  explicit Player(int id);

  // Rolls all the dice for the player
  void roll_dice();

  // Displays the face values of the player's dice
  void display_dice();

  // Allows the player to make a guess
  std::pair<int, int> make_guess();

  // Allows the player to call "Liar" on another player's guess
  bool call_liar();

  // Returns a const reference to the player's dice to avoid copying
  [[nodiscard]] const std::vector<Dice> &get_dice() const { return dice_; }

  [[nodiscard]] int get_player_id() const { return id_; };

private:
  int id_;                 // Player ID
  std::vector<Dice> dice_; // Player's dice
};

#endif // PLAYER_HPP
