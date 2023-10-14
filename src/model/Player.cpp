//
// Created by Brett on 9/4/2023.
//

#include "Player.hpp"
#include <iostream>
#include <sstream>
#include <utility>
#include <limits>

Player::Player(int id) : id(id), dice(5) {  // Initialize id and reserve space for 5 Dice objects
  RollDice();
}

void Player::RollDice() {
  for (auto& die : dice) {
    die.Roll();
  }
}

void Player::DisplayDice() {
  std::cout << "Player " << id << ", your dice are: ";
  for (const auto& die : dice) {
    std::cout << die.GetFaceValue() << ' ';
  }
  std::cout << '\n';
}

std::pair<int, int> Player::MakeGuess() {
  std::cout << "Enter your guess in format (quantity, face_value): ";
  std::string input;
  std::getline(std::cin, input);
  std::istringstream iss(input);

  int quantity, face_value;
  char comma;

  if (!(iss >> quantity >> comma >> face_value) || comma != ',') {
    std::cout << "Invalid input. Please try again.\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return MakeGuess();
  }

  return {quantity, face_value};
}

bool Player::CallLiar() {
  std::cout << "Do you want to call liar? (yes/no) ";
  std::string call_liar;
  std::getline(std::cin, call_liar);
  return (call_liar == "yes");
}
