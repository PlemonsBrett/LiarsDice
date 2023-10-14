//
// Created by Brett on 9/4/2023.
//

#ifndef GAME_HPP
#define GAME_HPP

#include <vector>
#include <string>
#include <utility>
#include "Player.hpp"

class Game {
public:
  Game();
  void Init();

  std::string ReadRulesFromFile(const std::string& filename);
  void SetupPlayers();
  void PlayGame();
  std::string ValidateGuess(const std::pair<int, int>& new_guess, const std::pair<int, int>& last_guess);
  std::string CheckGuessAgainstDice(const std::pair<int, int>& last_guess);

private:
  std::vector<Player> players;
  int currentPlayerIndex;
  std::pair<int, int> lastGuess;
  std::string rulesText;
};

#endif //GAME_HPP

