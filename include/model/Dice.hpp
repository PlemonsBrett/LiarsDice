//
// Created by Brett on 9/4/2023.
// This class represents a die in the game of Liar's Dice.
//

#ifndef DICE_HPP
#define DICE_HPP

#include <random>

class Dice {
public:
  // Constructor initializes the random number generator
  Dice();

  // Rolls the dice and updates the face value
  void Roll();

  // Returns the current face value of the dice
  [[nodiscard]] unsigned int GetFaceValue() const;

private:
  unsigned int face_value{};  // Holds the face value of the dice
  std::random_device rd;  // Random device engine
  std::mt19937 gen;  // Mersenne Twister random number generator
  std::uniform_int_distribution<> dis;  // Uniform distribution
};

#endif // DICE_HPP


