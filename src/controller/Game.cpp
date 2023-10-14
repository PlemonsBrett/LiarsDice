//
// Created by Brett on 9/4/2023.
// This file contains the implementation of the Game class, which handles the game logic for Liar's Dice.
//

#include "controller/Game.hpp"
#include "exceptions/FileException.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

// Constructor implementation
Game::Game() : currentPlayerIndex(0), lastGuess({0, 0}) {

}

void Game::Init() {
  try {
    rulesText = ReadRulesFromFile("./assets/rules.txt");
    std::cout << rulesText;
  } catch (const FileException& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Ensure 'assets/rules.txt' exists in the same directory as 'LiarsDice.exe'.";
    exit(EXIT_FAILURE); // Exit the game
  }

  SetupPlayers();
  PlayGame();
}

std::string Game::ReadRulesFromFile(const std::string& filename) {
  std::string rulesContent;
  std::ifstream file_handle(filename);

  // Check if the file could be opened
  if (!file_handle) {
    throw FileException("Could not open rules.txt");
  }

  std::string line;
  while (std::getline(file_handle, line)) {
    rulesContent += line + '\n';
  }
  return rulesContent;
}

void Game::SetupPlayers() {
  // Validate the number of players
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
    displayCurrentState(currentPlayer);

    Guess guess = currentPlayer.MakeGuess();
    std::string validationError = ValidateGuess(guess, lastGuess);

    if (!validationError.empty()) {
      std::cout << validationError;
      continue;
    }

    lastGuess = guess;

    if (currentPlayer.CallLiar()) {
      std::string winner = CheckGuessAgainstDice(lastGuess);
      std::cout << "The winner is " << winner << '\n';
      break;
    }

    updateCurrentPlayerIndex();
  }
}

void Game::displayCurrentState(Player& currentPlayer) {
  currentPlayer.DisplayDice();
  if (lastGuess.diceCount != 0 || lastGuess.diceValue != 0) {
    std::cout << "Last guess was (" << lastGuess.diceCount << ", " << lastGuess.diceValue << ")\n";
  }
}

void Game::updateCurrentPlayerIndex() {
  ++currentPlayerIndex;
  if (currentPlayerIndex >= players.size()) {
    currentPlayerIndex = 0;
  }
}

std::string Game::ValidateGuess(const Guess& new_guess, const Guess& last_guess) {
  std::stringstream errorMsg;

  if (last_guess.diceCount != 0 || last_guess.diceValue != 0) {
    errorMsg << "Last guess was (" << last_guess.diceCount << ", " << last_guess.diceValue << ")\n";
  }

  if (new_guess.diceCount < last_guess.diceCount && new_guess.diceValue <= last_guess.diceValue) {
    errorMsg << "Invalid guess. You have fewer dice but the face value is not greater than the last guess.\n";
    return errorMsg.str();
  }

  if (new_guess.diceCount == last_guess.diceCount && new_guess.diceValue <= last_guess.diceValue) {
    errorMsg << "Invalid guess. You have the same number of dice but the face value is not greater.\n";
    return errorMsg.str();
  }

  if (new_guess.diceCount <= last_guess.diceCount && new_guess.diceValue < last_guess.diceValue) {
    errorMsg << "Invalid guess. You must either have more dice or a greater face value.\n";
    return errorMsg.str();
  }

  return ""; // Valid guess
}


std::string Game::CheckGuessAgainstDice(const Guess& last_guess) {
  int counter = 0;
  for (const auto& player : players) {
    for (const auto& die : player.GetDice()) {
      if (die.GetFaceValue() == last_guess.diceValue) {
        ++counter;
      }
    }
  }
  return (counter >= last_guess.diceCount) ? "Guessing Player" : "Calling Player";
}