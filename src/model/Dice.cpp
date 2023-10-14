//
// Created by Brett on 9/4/2023.
//

#include "Dice.hpp"

// Constructor
Dice::Dice() : rd(), gen(rd()), dis(1, 6) {
  Roll();
}

// Roll method implementation using std::mt19937 and std::uniform_int_distribution
void Dice::Roll() {
  face_value = dis(gen);
}

// GetFaceValue method implementation
unsigned int Dice::GetFaceValue() const {
  return face_value;
}
