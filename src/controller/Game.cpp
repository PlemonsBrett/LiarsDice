//
// Created by Brett on 9/4/2023.
//

#include "Game.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

// Constructor implementation
Game::Game() : currentPlayerIndex(0), lastGuess({0, 0}) {

}

void Game::Init() {
  rulesText = ReadRulesFromFile("./assets/rules.txt");
  std::cout << rulesText;
  SetupPlayers();
  PlayGame();
}

std::string Game::ReadRulesFromFile(const std::string& filename) {
  std::string rulesContent;
  std::ifstream file_handle(filename);
  if (!file_handle) {
    std::cout << "Error: Could not open rules file.\n";
    return "";
  }
  std::string line;
  while (std::getline(file_handle, line)) {
    rulesContent += line + '\n';
  }
  return rulesContent;
}

void Game::SetupPlayers() {
  std::cout << "Enter the number of players: ";
  int num_players;
  std::cin >> num_players;
  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  while (num_players < 2) {
    std::cout << "Please enter a number greater than 1: ";
    std::cin >> num_players;
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  players.reserve(num_players);
  for (int i = 1; i <= num_players; ++i) {
    players.emplace_back(i);
  }
}

void Game::PlayGame() {
  while (true) {
    Player& currentPlayer = players[currentPlayerIndex];
    currentPlayer.DisplayDice();

    if (lastGuess.first != 0 || lastGuess.second != 0) {
      std::cout << "Last guess was (" << lastGuess.first << ", " << lastGuess.second << ")\n";
    }

    auto guess = currentPlayer.MakeGuess();
    std::string validationError = ValidateGuess(guess, lastGuess);

    if (!validationError.empty()) {
      std::cout << validationError;
      currentPlayer.DisplayDice();  // Show the current player's dice again after the error message
      continue;
    }

    lastGuess = guess;

    if (currentPlayer.CallLiar()) {
      std::string winner = CheckGuessAgainstDice(lastGuess);
      std::cout << "The winner is " << winner << '\n';
      break;
    }

    ++currentPlayerIndex;
    if (currentPlayerIndex >= players.size()) {
      currentPlayerIndex = 0;
    }
  }
}

std::string Game::ValidateGuess(const std::pair<int, int>& new_guess, const std::pair<int, int>& last_guess) {
  std::stringstream errorMsg;

  if (last_guess.first != 0 || last_guess.second != 0) {
    errorMsg << "Last guess was (" << last_guess.first << ", " << last_guess.second << ")\n";
  }

  if (new_guess.first < last_guess.first && new_guess.second <= last_guess.second) {
    errorMsg << "Invalid guess. You have fewer dice but the face value is not greater than the last guess.\n";
    return errorMsg.str();
  }

  if (new_guess.first == last_guess.first && new_guess.second <= last_guess.second) {
    errorMsg << "Invalid guess. You have the same number of dice but the face value is not greater.\n";
    return errorMsg.str();
  }

  if (new_guess.first <= last_guess.first && new_guess.second < last_guess.second) {
    errorMsg << "Invalid guess. You must either have more dice or a greater face value.\n";
    return errorMsg.str();
  }

  return ""; // Valid guess
}


std::string Game::CheckGuessAgainstDice(const std::pair<int, int>& last_guess) {
  int counter = 0;
  for (const auto& player : players) {
    for (const auto& die : player.GetDice()) {
      if (die.GetFaceValue() == last_guess.second) {
        ++counter;
      }
    }
  }
  return (counter >= last_guess.first) ? "Guessing Player" : "Calling Player";
}