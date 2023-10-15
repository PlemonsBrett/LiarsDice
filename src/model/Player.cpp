//
// Created by Brett on 9/4/2023.
// This file contains the implementation of the Player class, which represents a player in the game of Liar's Dice.
//

#include "Player.hpp"
#include "InputException.hpp"
#include <iostream>
#include <sstream>
#include <utility>
#include <limits>

// Constructor initializes the player ID and reserves space for 5 dice
Player::Player(int id) : id(id), dice(5) {
  // Roll the dice initially for the player
  RollDice();
}

// Roll all dice for the player
void Player::RollDice() {
  for (auto& die : dice) {
    die.Roll();
  }
}

// Display the face values of the player's dice
void Player::DisplayDice() {
  std::cout << "Player " << id << ", your dice are: ";
  for (const auto& die : dice) {
    std::cout << die.GetFaceValue() << ' ';
  }
  std::cout << '\n';
}

// Allow the player to make a guess
std::pair<int, int> Player::MakeGuess() {
  std::pair<int, int> guess;
  // Loop until a valid guess is made
  while (true) {
    std::cout << "Enter your guess in format (quantity, face_value): ";
    std::string input;
    std::getline(std::cin, input);
    std::istringstream iss(input);

    int quantity, face_value;
    char comma;

    // Validate the input format
    if (iss >> quantity >> comma >> face_value && comma == ',') {
      guess = {quantity, face_value};
      break;
    }

    std::cerr << "Invalid input: " << input << std::endl;
    std::cerr << "Please try again. Example: 3,4" << std::endl;

    // Clear the input buffer
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return guess;
}

// Allow the player to call "Liar" on another player's guess
bool Player::CallLiar() {
  std::cout << "Do you want to call liar? (yes/no) ";
  std::string call_liar;
  std::getline(std::cin, call_liar);
  return (call_liar == "yes");
}
