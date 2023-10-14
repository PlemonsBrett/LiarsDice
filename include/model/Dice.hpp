//
// Created by Brett on 9/4/2023.
//

#ifndef DICE_HPP
#define DICE_HPP

#include <random>

// Dice class definition
class Dice {
public:
  // Constructor
  Dice();

  // Methods
  void Roll();
  [[nodiscard]] unsigned int GetFaceValue() const;

private:
  unsigned int face_value{};
  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<> dis;
};

#endif // DICE_HPP

