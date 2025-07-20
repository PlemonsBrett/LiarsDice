#pragma once

#include <vector>
#include <memory>
#include "i_dice.hpp"

namespace liarsdice::interfaces {

/**
 * @brief Interface for player objects
 * 
 * Defines the contract for player behavior in the game.
 * Players manage their dice collection and participate in game rounds.
 */
class IPlayer {
public:
    virtual ~IPlayer() = default;

    /**
     * @brief Get the player's unique identifier
     * @return The player ID
     */
    virtual int get_id() const = 0;

    /**
     * @brief Get the number of dice the player currently has
     * @return The count of dice
     */
    virtual size_t get_dice_count() const = 0;

    /**
     * @brief Roll all of the player's dice
     */
    virtual void roll_dice() = 0;

    /**
     * @brief Add a new die to the player's collection
     */
    virtual void add_die() = 0;

    /**
     * @brief Remove a die from the player's collection
     * @return true if a die was removed, false if no dice to remove
     */
    virtual bool remove_die() = 0;

    /**
     * @brief Check if the player has any dice remaining
     * @return true if the player has dice, false otherwise
     */
    virtual bool has_dice() const = 0;

    /**
     * @brief Get a read-only view of the player's dice
     * @return Vector of dice interfaces
     */
    virtual std::vector<std::reference_wrapper<const IDice>> get_dice() const = 0;

    /**
     * @brief Count how many dice show a specific face value
     * @param face_value The face value to count (1-6)
     * @return The number of dice showing that value
     */
    virtual size_t count_dice_with_value(unsigned int face_value) const = 0;

    /**
     * @brief Get all face values of the player's dice
     * @return Vector of face values
     */
    virtual std::vector<unsigned int> get_dice_values() const = 0;

    /**
     * @brief Check if the player is still active in the game
     * @return true if active (has dice), false if eliminated
     */
    virtual bool is_active() const = 0;
};

} // namespace liarsdice::interfaces