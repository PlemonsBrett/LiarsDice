//
// Created by Brett on 9/4/2023.
// This file contains the implementation of the Dice class, which represents a dice in the game of
// Liar's Dice.
//

#include "liarsdice/core/dice.hpp"

#ifdef LIARSDICE_ENABLE_LOGGING
#include "liarsdice/logging/logging.hpp"
#endif

// Constructor initializes the random number generator and rolls the dice
Dice::Dice() : generator_(random_device_()), distribution_(1, 6) {
#ifdef LIARSDICE_ENABLE_LOGGING
  DICE_LOG_DEBUG("Dice constructor: Initializing random number generator");
#endif
  roll();
}

// Rolls the dice using std::mt19937 and std::uniform_int_distribution
void Dice::roll() {
  face_value_ = static_cast<unsigned int>(distribution_(generator_));
#ifdef LIARSDICE_ENABLE_LOGGING
  DICE_LOG_TRACE("Dice rolled: face_value={}", face_value_);
#endif
}

// Returns the current face value of the dice
unsigned int Dice::get_face_value() const { return face_value_; }
