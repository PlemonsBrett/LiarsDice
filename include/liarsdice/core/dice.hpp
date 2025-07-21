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
  void roll();

  // Returns the current face value of the dice
  [[nodiscard]] unsigned int get_face_value() const;

private:
  unsigned int face_value_{};                    // Holds the face value of the dice
  std::random_device random_device_;             // Random device engine
  std::mt19937 generator_;                       // Mersenne Twister random number generator
  std::uniform_int_distribution<> distribution_; // Uniform distribution
};

#endif // DICE_HPP
