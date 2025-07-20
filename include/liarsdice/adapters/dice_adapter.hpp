#pragma once

#include "../interfaces/i_dice.hpp"
#include "../interfaces/i_random_generator.hpp"
#include "../core/dice.hpp"
#include <memory>

namespace liarsdice::adapters {

/**
 * @brief Adapter that wraps the existing Dice class to implement IDice interface
 * 
 * This allows the legacy Dice class to work with the new DI system
 * without requiring immediate refactoring of the existing implementation.
 */
class DiceAdapter : public interfaces::IDice {
private:
    Dice dice_;  // Composition instead of inheritance
    std::unique_ptr<interfaces::IRandomGenerator> rng_;
    mutable unsigned int current_face_value_ = 1;  // Custom value storage
    mutable bool has_custom_value_ = false;        // Track if we have custom value

public:
    /**
     * @brief Construct with default random generator (uses existing Dice RNG)
     */
    DiceAdapter() = default;

    /**
     * @brief Construct with injected random generator
     */
    explicit DiceAdapter(std::unique_ptr<interfaces::IRandomGenerator> rng)
        : rng_(std::move(rng)) {}

    // IDice interface implementation
    void roll() override {
        has_custom_value_ = false;  // Clear custom value when rolling
        if (rng_) {
            // Use injected RNG
            unsigned int value = static_cast<unsigned int>(rng_->generate(1, 6));
            current_face_value_ = value;
            has_custom_value_ = true;
        } else {
            // Use existing dice roll method
            dice_.roll();
        }
    }

    unsigned int get_face_value() const override {
        if (has_custom_value_) {
            return current_face_value_;
        }
        return dice_.get_face_value();
    }

    void set_face_value(unsigned int value) override {
        if (is_valid_face_value(value)) {
            // Store the value internally since Dice class doesn't have setValue
            // We'll track the face value ourselves
            current_face_value_ = value;
            has_custom_value_ = true;
        }
    }

    bool is_valid_face_value(unsigned int value) const override {
        return value >= 1 && value <= 6;
    }

    std::unique_ptr<interfaces::IDice> clone() const override {
        auto cloned = std::make_unique<DiceAdapter>();
        if (rng_) {
            // For simplicity, we won't clone the RNG - each clone gets its own default
            // In a production system, you'd want to properly clone or share the RNG
        }
        cloned->set_face_value(get_face_value());
        return std::move(cloned);
    }

    /**
     * @brief Get access to the underlying Dice object (for legacy compatibility)
     */
    const Dice& get_underlying_dice() const { return dice_; }
    Dice& get_underlying_dice() { return dice_; }
};

} // namespace liarsdice::adapters