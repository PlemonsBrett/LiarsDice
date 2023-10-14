//
// Created by Brett on 9/4/2023.
// This class handles the game logic for Liar's Dice.
//

#ifndef GAME_HPP
#define GAME_HPP

#include <vector>
#include <string>
#include <utility>
#include "model/Player.hpp"

// Struct to represent a guess
struct Guess {
  int diceValue;
  int diceCount;
};

class Game {
public:
  Game();

  // Initializes the game
  void Init();

  // Reads game rules from a file
  std::string ReadRulesFromFile(const std::string& filename);

  // Sets up players for the game
  void SetupPlayers();

  // Main game loop
  void PlayGame();

  // Validates a new guess against the last guess
  std::string ValidateGuess(const Guess& new_guess, const Guess& last_guess);

  // Checks the last guess against the actual dice
  std::string CheckGuessAgainstDice(const Guess& last_guess);

private:
  std::vector<Player> players;
  int currentPlayerIndex;
  Guess lastGuess;
  std::string rulesText;
  void updateCurrentPlayerIndex();
  void displayCurrentState(Player &currentPlayer);
};

#endif //GAME_HPP
