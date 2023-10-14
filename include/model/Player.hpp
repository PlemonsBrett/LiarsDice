//
// Created by Brett on 9/4/2023.
// This class represents a player in the game of Liar's Dice.
//

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <vector>
#include "Dice.hpp"
#include "controller/Game.hpp"

class Player {
public:
  // Constructor initializes the player with an ID
  explicit Player(int id);

  // Rolls all the dice for the player
  void RollDice();

  // Displays the face values of the player's dice
  void DisplayDice();

  // Allows the player to make a guess
  Guess MakeGuess();

  // Allows the player to call "Liar" on another player's guess
  bool CallLiar();

  // Returns a const reference to the player's dice to avoid copying
  [[nodiscard]] const std::vector<Dice>& GetDice() const { return dice; }

private:
  int id;  // Player ID
  std::vector<Dice> dice;  // Player's dice
};

#endif //PLAYER_HPP
