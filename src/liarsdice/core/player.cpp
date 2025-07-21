//
// Created by Brett on 9/4/2023.
// This file contains the implementation of the Player class, which represents a player in the game
// of Liar's Dice.
//

#include "liarsdice/core/player.hpp"
#include <iostream>
#include <limits>
#include <sstream>
#include <utility>

#ifdef LIARSDICE_ENABLE_LOGGING
#include "liarsdice/logging/logging.hpp"
#endif

// Constructor initializes the player ID and reserves space for 5 dice
Player::Player(int id) : id_(id), dice_(5) {
#ifdef LIARSDICE_ENABLE_LOGGING
  PLAYER_LOG_INFO("Player {} created with {} dice", id, dice_.size());
#endif
  // Roll the dice initially for the player
  roll_dice();
}

// Roll all dice for the player
void Player::roll_dice() {
#ifdef LIARSDICE_ENABLE_LOGGING
  PLAYER_LOG_DEBUG("Player {} rolling all dice", id_);
#endif
  for (auto &die : dice_) {
    die.roll();
  }
#ifdef LIARSDICE_ENABLE_LOGGING
  // Log the dice values after rolling
  std::ostringstream dice_values;
  for (const auto &die : dice_) {
    dice_values << die.get_face_value() << " ";
  }
  PLAYER_LOG_TRACE("Player {} dice after roll: {}", id_, dice_values.str());
#endif
}

// Display the face values of the player's dice
void Player::display_dice() {
  std::cout << "Player " << id_ << ", your dice are: ";
  for (const auto &die : dice_) {
    std::cout << die.get_face_value() << ' ';
  }
  std::cout << '\n';
}

// Allow the player to make a guess
std::pair<int, int> Player::make_guess() {
  std::pair<int, int> guess;
#ifdef LIARSDICE_ENABLE_LOGGING
  PLAYER_LOG_DEBUG("Player making guess");
#endif
  // Loop until a valid guess is made
  while (true) {
    std::cout << "Enter your guess in format (quantity, face_value): ";
    std::string input;
    std::getline(std::cin, input);
    std::istringstream iss(input);

    int quantity = 0;
    int face_value = 0;
    char comma = '\0';

    // Validate the input format
    if (iss >> quantity >> comma >> face_value && comma == ',') {
      guess = {quantity, face_value};
#ifdef LIARSDICE_ENABLE_LOGGING
      PLAYER_LOG_INFO("Player made valid guess: quantity={}, face_value={}", quantity, face_value);
#endif
      break;
    }

#ifdef LIARSDICE_ENABLE_LOGGING
    PLAYER_LOG_DEBUG("Invalid input received: '{}'", input);
#endif
    std::cerr << "Invalid input: " << input << '\n';
    std::cerr << "Please try again. Example: 3,4" << '\n';

    // Clear the input buffer
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return guess;
}

// Allow the player to call "Liar" on another player's guess
bool Player::call_liar() {
  std::cout << "Do you want to call liar? (yes/no) ";
  std::string call_liar;
  std::getline(std::cin, call_liar);
  bool is_calling_liar = (call_liar == "yes");
#ifdef LIARSDICE_ENABLE_LOGGING
  PLAYER_LOG_INFO("Player {} liar call: {}", id_, is_calling_liar ? "yes" : "no");
#endif
  return is_calling_liar;
}
