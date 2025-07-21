#pragma once

#include "liarsdice/interfaces/i_dice.hpp"
#include "liarsdice/interfaces/i_random_generator.hpp"
#include <memory>

// LLDB debugging support
#ifdef __clang__
#define LLDB_VISUALIZER(name) __attribute__((annotate("lldb.visualizer:" name)))
#else
#define LLDB_VISUALIZER(name)
#endif

namespace liarsdice::core {

/**
 * @brief Modern C++23 implementation of IDice with dependency injection
 * 
 * This class implements the IDice interface using dependency injection
 * for random number generation. Designed for testability and modularity.
 */
class LLDB_VISUALIZER("DiceImpl") DiceImpl final : public interfaces::IDice {
private:
    unsigned int face_value_;
    std::unique_ptr<interfaces::IRandomGenerator> random_generator_;
    
    // Configuration constants
    static constexpr unsigned int kMinFaceValue = 1;
    static constexpr unsigned int kMaxFaceValue = 6;
    static constexpr unsigned int kDefaultFaceValue = 1;

public:
    /**
     * @brief Constructor with dependency injection
     * 
     * @param random_generator Random number generator for dice rolls
     */
    explicit DiceImpl(std::unique_ptr<interfaces::IRandomGenerator> random_generator);

    /**
     * @brief Constructor with dependency injection and initial value
     * 
     * @param random_generator Random number generator for dice rolls
     * @param initial_value Initial face value (must be 1-6)
     */
    DiceImpl(std::unique_ptr<interfaces::IRandomGenerator> random_generator, 
             unsigned int initial_value);

    ~DiceImpl() override = default;

    // Non-copyable but movable
    DiceImpl(const DiceImpl&) = delete;
    DiceImpl& operator=(const DiceImpl&) = delete;
    DiceImpl(DiceImpl&&) = default;
    DiceImpl& operator=(DiceImpl&&) = default;

    // IDice interface implementation
    void roll() override;
    [[nodiscard]] unsigned int get_face_value() const override;
    void set_face_value(unsigned int value) override;
    [[nodiscard]] bool is_valid_face_value(unsigned int value) const override;
    [[nodiscard]] std::unique_ptr<interfaces::IDice> clone() const override;

private:
    /**
     * @brief Validate and set face value with bounds checking
     * @param value The face value to set
     * @throws std::invalid_argument if value is out of range
     */
    void validate_and_set_face_value(unsigned int value);

    /**
     * @brief Get minimum valid face value
     * @return Minimum face value (always 1)
     */
    [[nodiscard]] static constexpr unsigned int get_min_face_value() noexcept {
        return kMinFaceValue;
    }

    /**
     * @brief Get maximum valid face value
     * @return Maximum face value (always 6)
     */
    [[nodiscard]] static constexpr unsigned int get_max_face_value() noexcept {
        return kMaxFaceValue;
    }
};

} // namespace liarsdice::core