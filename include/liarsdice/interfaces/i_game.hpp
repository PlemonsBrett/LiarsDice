#pragma once

#include <memory>
#include <string>
#include "i_game_state.hpp"
#include "i_player.hpp"

namespace liarsdice::interfaces {

/**
 * @brief Interface for the main game controller
 * 
 * Manages game flow, rule validation, and player interactions.
 * This is the main orchestrator of the Liar's Dice game.
 */
class IGame {
public:
    virtual ~IGame() = default;

    /**
     * @brief Initialize the game with default settings
     */
    virtual void initialize() = 0;

    /**
     * @brief Add a player to the game
     * @param player_id Unique identifier for the player
     */
    virtual void add_player(int player_id) = 0;

    /**
     * @brief Start the game (all players must be added first)
     */
    virtual void start_game() = 0;

    /**
     * @brief Check if the game has ended
     * @return true if game is over, false otherwise
     */
    virtual bool is_game_over() const = 0;

    /**
     * @brief Get the ID of the winning player
     * @return Winner's player ID, or -1 if no winner yet
     */
    virtual int get_winner_id() const = 0;

    /**
     * @brief Validate a player's guess against game rules
     * @param guess The guess to validate
     * @return Error message if invalid, empty string if valid
     */
    virtual std::string validate_guess(const Guess& guess) const = 0;

    /**
     * @brief Process a player's guess
     * @param guess The guess to process
     * @return true if guess was accepted, false if invalid
     */
    virtual bool process_guess(const Guess& guess) = 0;

    /**
     * @brief Process a "liar" call and determine the outcome
     * @param calling_player_id ID of the player calling "liar"
     * @return Result message describing the outcome
     */
    virtual std::string process_liar_call(int calling_player_id) = 0;

    /**
     * @brief Get the current game state
     * @return Reference to the game state interface
     */
    virtual IGameState& get_game_state() = 0;

    /**
     * @brief Get the current game state (const version)
     * @return Const reference to the game state interface
     */
    virtual const IGameState& get_game_state() const = 0;

    /**
     * @brief Reset the game to initial state
     */
    virtual void reset() = 0;

    /**
     * @brief Get the minimum number of players required
     * @return Minimum player count
     */
    virtual size_t get_min_players() const = 0;

    /**
     * @brief Get the maximum number of players allowed
     * @return Maximum player count
     */
    virtual size_t get_max_players() const = 0;

    /**
     * @brief Get the number of dice each player starts with
     * @return Initial dice count per player
     */
    virtual size_t get_initial_dice_per_player() const = 0;
};

} // namespace liarsdice::interfaces