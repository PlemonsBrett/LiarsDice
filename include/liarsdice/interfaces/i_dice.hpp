#pragma once

namespace liarsdice::interfaces {

/**
 * @brief Interface for dice objects
 * 
 * Defines the contract for dice behavior in the game.
 * Implementations must provide rolling functionality and face value management.
 */
class IDice {
public:
    virtual ~IDice() = default;

    /**
     * @brief Roll the die to generate a new face value
     */
    virtual void roll() = 0;

    /**
     * @brief Get the current face value of the die
     * @return The current face value (1-6)
     */
    virtual unsigned int get_face_value() const = 0;

    /**
     * @brief Set the face value of the die (for testing purposes)
     * @param value The face value to set (1-6)
     */
    virtual void set_face_value(unsigned int value) = 0;

    /**
     * @brief Check if a face value is valid
     * @param value The value to check
     * @return true if the value is valid (1-6), false otherwise
     */
    virtual bool is_valid_face_value(unsigned int value) const = 0;

    /**
     * @brief Clone this die (for copying purposes)
     * @return A unique pointer to a cloned die
     */
    virtual std::unique_ptr<IDice> clone() const = 0;
};

} // namespace liarsdice::interfaces