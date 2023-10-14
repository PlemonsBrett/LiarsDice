//
// Created by Brett on 9/4/2023.
//

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <vector>
#include "Dice.hpp"

class Player {
public:
  explicit Player(int id);

  void RollDice();
  void DisplayDice();
  std::pair<int, int> MakeGuess();
  bool CallLiar();
  [[nodiscard]] const std::vector<Dice>& GetDice() const { return dice; } // Return const reference to avoid copying

private:
  int id;
  std::vector<Dice> dice;
};

#endif //PLAYER_HPP
