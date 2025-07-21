//
// Created by Brett on 9/4/2023.
// This file contains the implementation of the Game class, which handles the game logic for Liar's
// Dice.
//

#include "liarsdice/core/game.hpp"
#include "liarsdice/exceptions/file_exception.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifdef LIARSDICE_ENABLE_LOGGING
#include "liarsdice/logging/logging.hpp"
#endif

// Named constants
const std::string kInvalidGuessMsgGeneral =
    "Invalid guess. You must either have more dice or a greater face value"
    ".\n";
const std::string kInvalidGuessMsgFaceValue =
    "Invalid guess. You have the same number of dice but the face value "
    "is not greater.\n";
const std::string kInvalidGuessMsgDiceCount =
    "Invalid guess. You have fewer dice but the face value is not "
    "greater than the last guess.\n";

// Constructor implementation
Game::Game() : players_{}, current_player_index_(0), last_guess_({0, 0}) {
#ifdef LIARSDICE_ENABLE_LOGGING
  GAME_LOG_INFO("Game instance created");
#endif
}

void Game::init() {
#ifdef LIARSDICE_ENABLE_LOGGING
  GAME_LOG_INFO("Initializing game");
  PERF_TIMER("Game initialization");
#endif
  try {
    rules_text_ = read_rules_from_file("./assets/rules.txt");
    std::cout << rules_text_;
#ifdef LIARSDICE_ENABLE_LOGGING
    GAME_LOG_DEBUG("Rules loaded successfully from assets/rules.txt");
#endif
  } catch (const FileException &e) {
#ifdef LIARSDICE_ENABLE_LOGGING
    GAME_LOG_ERROR("Failed to load rules: {}", e.what());
#endif
    std::cerr << e.what() << '\n';
    std::cerr << "Ensure 'assets/rules.txt' exists in the same directory as 'LiarsDice.exe'.";
    exit(EXIT_FAILURE); // Exit the game
  }

  setup_players();
  play_game();
}

std::string Game::read_rules_from_file(const std::string &filename) {
#ifdef LIARSDICE_ENABLE_LOGGING
  GAME_LOG_DEBUG("Attempting to read rules from file: {}", filename);
#endif
  std::string rules_content;
  std::ifstream file_handle(filename);

  // Check if the file could be opened
  if (!file_handle) {
#ifdef LIARSDICE_ENABLE_LOGGING
    GAME_LOG_ERROR("Could not open rules file: {}", filename);
#endif
    throw FileException("Could not open rules.txt");
  }

  [[maybe_unused]] size_t line_count = 0;
  for (std::string line; std::getline(file_handle, line);) {
    rules_content += line + '\n';
    ++line_count;
  }
#ifdef LIARSDICE_ENABLE_LOGGING
  GAME_LOG_DEBUG("Successfully read {} lines from rules file", line_count);
#endif
  return rules_content;
}

void Game::setup_players() {
#ifdef LIARSDICE_ENABLE_LOGGING
  GAME_LOG_INFO("Setting up players");
#endif
  // Validate the number of players
  std::cout << "Enter the number of players: ";
  int num_players = 0;
  get_setup_input(num_players);

  while (num_players < 2) {
#ifdef LIARSDICE_ENABLE_LOGGING
    GAME_LOG_DEBUG("Invalid player count received: {}, requesting again", num_players);
#endif
    std::cout << "Please enter a number greater than 1: ";
    get_setup_input(num_players);
  }

#ifdef LIARSDICE_ENABLE_LOGGING
  GAME_LOG_INFO("Creating {} players", num_players);
#endif
  players_.reserve(static_cast<size_t>(num_players));
  for (int i = 1; i <= num_players; ++i) {
    players_.emplace_back(i);
  }
#ifdef LIARSDICE_ENABLE_LOGGING
  GAME_LOG_DEBUG("All players created successfully");
#endif
}

void Game::get_setup_input(int &num_players) {
  if (!(std::cin >> num_players)) {
    // Input failed - clear error state and discard bad input
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    num_players = 0; // Ensure invalid value
  } else {
    // Successfully read a number - clear remaining input
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
}

void Game::play_game() {
#ifdef LIARSDICE_ENABLE_LOGGING
  GAME_LOG_INFO("Starting game loop with {} players", players_.size());
#endif
  while (true) {
    // Clear the screen
    system("cls");

    // Display the rules
    std::cout << rules_text_;

    Player &current_player = players_[static_cast<size_t>(current_player_index_)];
#ifdef LIARSDICE_ENABLE_LOGGING
    GAME_LOG_DEBUG("Current player turn: Player {}", current_player.get_player_id());
#endif
    display_current_state(current_player);

    auto guess = Guess(current_player.make_guess());
    if (std::string validation_error = validate_guess(guess, last_guess_);
        !validation_error.empty()) {
#ifdef LIARSDICE_ENABLE_LOGGING
      GAME_LOG_DEBUG("Invalid guess by Player {}: {}", current_player.get_player_id(),
                     validation_error);
#endif
      std::cout << validation_error;
      continue;
    }

    last_guess_ = guess;
#ifdef LIARSDICE_ENABLE_LOGGING
    GAME_LOG_INFO("Valid guess accepted: quantity={}, face_value={}", guess.dice_count,
                  guess.dice_value);
#endif

    if (current_player.call_liar()) {
      std::cout << "The winner is " << check_guess_against_dice(last_guess_) << '\n';
      break;
    }

    update_current_player_index();
  }
}

void Game::display_current_state(Player &current_player) const {
  std::cout << "PLAYER " << current_player.get_player_id() << "'s Turn:\n";
  if (last_guess_.dice_count != 0 || last_guess_.dice_value != 0) {
    std::cout << "Last Guess: " << last_guess_.dice_count << ", " << last_guess_.dice_value << '\n';
  }
  std::cout << "Your Dice: ";
  current_player.display_dice();
  std::cout << '\n';
}

void Game::update_current_player_index() {
  ++current_player_index_;
  if (static_cast<size_t>(current_player_index_) >= players_.size()) {
    current_player_index_ = 0;
  }
}

std::string Game::validate_guess(const Guess &new_guess, const Guess &last_guess) {
  std::stringstream error_msg;

  if (last_guess.dice_count != 0 || last_guess.dice_value != 0) {
    error_msg << "Last guess was (" << last_guess.dice_count << ", " << last_guess.dice_value
              << ")\n";
  }

  if (new_guess.dice_count < last_guess.dice_count &&
      new_guess.dice_value <= last_guess.dice_value) {
    error_msg << kInvalidGuessMsgDiceCount;
    return error_msg.str();
  }

  if (new_guess.dice_count == last_guess.dice_count &&
      new_guess.dice_value <= last_guess.dice_value) {
    error_msg << kInvalidGuessMsgFaceValue;
    return error_msg.str();
  }

  if (new_guess.dice_count <= last_guess.dice_count &&
      new_guess.dice_value < last_guess.dice_value) {
    error_msg << kInvalidGuessMsgGeneral;
    return error_msg.str();
  }

  return ""; // Valid guess
}

std::string Game::check_guess_against_dice(const Guess &last_guess) {
#ifdef LIARSDICE_ENABLE_LOGGING
  PERF_TIMER("Dice verification");
  GAME_LOG_DEBUG("Checking guess: {} dice with face value {}", last_guess.dice_count,
                 last_guess.dice_value);
#endif
  int counter = 0;
  for (const auto &player : players_) {
    for (const auto &die : player.get_dice()) {
      if (die.get_face_value() == static_cast<unsigned int>(last_guess.dice_value)) {
        ++counter;
      }
    }
  }
#ifdef LIARSDICE_ENABLE_LOGGING
  GAME_LOG_INFO("Dice verification: found {} dice with face value {} (needed {})", counter,
                last_guess.dice_value, last_guess.dice_count);
#endif
  return (counter >= last_guess.dice_count) ? "Guessing Player" : "Calling Player";
}