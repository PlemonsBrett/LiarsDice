//
// Created by Brett on 9/4/2023.
//

#include "Game.hpp"
#include <iostream>
#include <fstream>

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
    auto guess = currentPlayer.MakeGuess();

    if (!ValidateGuess(guess, lastGuess)) {
      std::cout << "Invalid guess. Try again.\n";
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

bool Game::ValidateGuess(const std::pair<int, int>& new_guess, const std::pair<int, int>& last_guess) {
  return (last_guess.first == 0 && last_guess.second == 0) ||  // First turn
      (new_guess.first > last_guess.first) ||  // More dice
      (new_guess.first == last_guess.first && new_guess.second > last_guess.second) ||  // Same number, higher face
      (new_guess.first < last_guess.first && new_guess.second > last_guess.second);  // Fewer dice, but higher face
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