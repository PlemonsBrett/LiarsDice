#pragma once

#include <optional>
#include "i_player.hpp"

namespace liarsdice::interfaces {

/**
 * @brief Represents a player's guess in the game
 */
struct Guess {
    unsigned int dice_count;    ///< Number of dice claimed
    unsigned int face_value;    ///< Face value claimed (1-6)
    int player_id;              ///< ID of the player who made the guess

    Guess(unsigned int count, unsigned int value, int id)
        : dice_count(count), face_value(value), player_id(id) {}
};

/**
 * @brief Interface for game state management
 * 
 * Manages the current state of the game including players,
 * current turn, and game progression.
 */
class IGameState {
public:
    virtual ~IGameState() = default;

    /**
     * @brief Get the index of the current player
     * @return Current player index
     */
    [[nodiscard]] virtual size_t get_current_player_index() const = 0;

    /**
     * @brief Move to the next player's turn
     */
    virtual void advance_to_next_player() = 0;

    /**
     * @brief Get the total number of players
     * @return Player count
     */
    [[nodiscard]] virtual size_t get_player_count() const = 0;

    /**
     * @brief Check if the game is currently active
     * @return true if game is in progress, false otherwise
     */
    [[nodiscard]] virtual bool is_game_active() const = 0;

    /**
     * @brief Set the game active state
     * @param active The new active state
     */
    virtual void set_game_active(bool active) = 0;

    /**
     * @brief Get the current round number
     * @return The round number (starts at 1)
     */
    [[nodiscard]] virtual int get_round_number() const = 0;

    /**
     * @brief Increment the round number
     */
    virtual void increment_round() = 0;

    /**
     * @brief Get the last guess made in the game
     * @return Optional guess (empty if no guess has been made)
     */
    [[nodiscard]] virtual std::optional<Guess> get_last_guess() const = 0;

    /**
     * @brief Set the last guess made
     * @param guess The guess to record
     */
    virtual void set_last_guess(const Guess& guess) = 0;

    /**
     * @brief Clear the last guess (start of new round)
     */
    virtual void clear_last_guess() = 0;

    /**
     * @brief Get a player by index
     * @param index The player index
     * @return Reference to the player interface
     */
    virtual IPlayer& get_player(size_t index) = 0;

    /**
     * @brief Get a player by index (const version)
     * @param index The player index
     * @return Const reference to the player interface
     */
    [[nodiscard]] virtual const IPlayer& get_player(size_t index) const = 0;

    /**
     * @brief Get the current player
     * @return Reference to the current player
     */
    virtual IPlayer& get_current_player() = 0;

    /**
     * @brief Get the current player (const version)
     * @return Const reference to the current player
     */
    [[nodiscard]] virtual const IPlayer& get_current_player() const = 0;

    /**
     * @brief Count total dice across all players with a specific face value
     * @param face_value The face value to count
     * @return Total count of dice with that value
     */
    [[nodiscard]] virtual size_t count_total_dice_with_value(unsigned int face_value) const = 0;

    /**
     * @brief Get the total number of dice remaining in the game
     * @return Total dice count
     */
    [[nodiscard]] virtual size_t get_total_dice_count() const = 0;
};

} // namespace liarsdice::interfaces