//
// Created by Brett on 9/4/2023.
// This class handles the game logic for Liar's Dice.
//

#ifndef GAME_HPP
#define GAME_HPP

#include <vector>
#include <string>
#include <utility>
#include "liarsdice/core/player.hpp"

// Struct to represent a guess
struct Guess {
  int dice_count;
  int dice_value;

  // Convert pair<int, int> into Guess
  explicit Guess(std::pair<int, int> guess_pair) 
    : dice_count(guess_pair.first), dice_value(guess_pair.second) {}
};

class Game {
public:
  Game();

  // Initializes the game
  void init();

  // Reads game rules from a file
  static std::string read_rules_from_file(const std::string& filename);

  // Sets up players for the game
  void setup_players();

  // Main game loop
  void play_game();

  // Validates a new guess against the last guess
  static std::string validate_guess(const Guess& new_guess, const Guess& last_guess);

  // Checks the last guess against the actual dice
  std::string check_guess_against_dice(const Guess& last_guess);

private:
  std::vector<Player> players_;
  int current_player_index_;
  Guess last_guess_;
  std::string rules_text_;
  void update_current_player_index();
  void display_current_state(Player &current_player) const;
  static void get_setup_input(int &num_players);
};

#endif //GAME_HPP
