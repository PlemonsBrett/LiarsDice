//
// Created by Brett on 9/4/2023.
// This file contains the implementation of the Dice class, which represents a dice in the game of Liar's Dice.
//

#include "Dice.hpp"

// Constructor initializes the random number generator and rolls the dice
Dice::Dice() : rd(), gen(rd()), dis(1, 6) {
  Roll();
}

// Rolls the dice using std::mt19937 and std::uniform_int_distribution
void Dice::Roll() {
  face_value = dis(gen);
}

// Returns the current face value of the dice
unsigned int Dice::GetFaceValue() const {
  return face_value;
}
