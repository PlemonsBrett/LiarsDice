//
// Created by Brett on 9/4/2023.
// This file contains the implementation of the Dice class, which represents a dice in the game of Liar's Dice.
//

#include "liarsdice/core/dice.hpp"

// Constructor initializes the random number generator and rolls the dice
Dice::Dice() : generator_(random_device_()), distribution_(1, 6) {
  roll();
}

// Rolls the dice using std::mt19937 and std::uniform_int_distribution
void Dice::roll() {
  face_value_ = static_cast<unsigned int>(distribution_(generator_));
}

// Returns the current face value of the dice
unsigned int Dice::get_face_value() const {
  return face_value_;
}
