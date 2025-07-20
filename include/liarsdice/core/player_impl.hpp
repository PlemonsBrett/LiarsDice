#pragma once

#include "liarsdice/interfaces/i_player.hpp"
#include "liarsdice/interfaces/i_dice.hpp"
#include "liarsdice/interfaces/i_random_generator.hpp"
#include "liarsdice/interfaces/i_service_factory.hpp"
#include <memory>
#include <vector>
#include <functional>

// LLDB debugging support
#ifdef __clang__
#define LLDB_VISUALIZER(name) __attribute__((annotate("lldb.visualizer:" name)))
#else
#define LLDB_VISUALIZER(name)
#endif

namespace liarsdice::core {

// Type aliases for cleaner code
using DiceFactory = interfaces::ServiceFactory<interfaces::IDice>;

/**
 * @brief Modern C++23 implementation of IPlayer with dependency injection
 * 
 * This class implements the IPlayer interface using dependency injection
 * to manage dice creation and random number generation.
 * Designed for testability and modularity.
 */
class LLDB_VISUALIZER("PlayerImpl") PlayerImpl final : public interfaces::IPlayer {
private:
    int player_id_;
    std::vector<std::unique_ptr<interfaces::IDice>> dice_;
    std::unique_ptr<interfaces::IRandomGenerator> random_generator_;
    std::unique_ptr<DiceFactory> dice_factory_;
    
    // Configuration constants
    static constexpr unsigned int kMinDiceValue = 1;
    static constexpr unsigned int kMaxDiceValue = 6;

public:
    /**
     * @brief Constructor with dependency injection
     * 
     * @param player_id Unique identifier for this player
     * @param random_generator Random number generator for dice rolls
     * @param dice_factory Factory for creating dice instances
     */
    explicit PlayerImpl(
        int player_id,
        std::unique_ptr<interfaces::IRandomGenerator> random_generator,
        std::unique_ptr<DiceFactory> dice_factory
    );

    ~PlayerImpl() override = default;

    // Non-copyable, movable
    PlayerImpl(const PlayerImpl&) = delete;
    PlayerImpl& operator=(const PlayerImpl&) = delete;
    PlayerImpl(PlayerImpl&&) = default;
    PlayerImpl& operator=(PlayerImpl&&) = default;

    // IPlayer interface implementation
    [[nodiscard]] int get_id() const override;
    [[nodiscard]] size_t get_dice_count() const override;
    void roll_dice() override;
    void add_die() override;
    bool remove_die() override;
    [[nodiscard]] bool has_dice() const override;
    [[nodiscard]] std::vector<std::reference_wrapper<const interfaces::IDice>> get_dice() const override;
    [[nodiscard]] size_t count_dice_with_value(unsigned int face_value) const override;
    [[nodiscard]] std::vector<unsigned int> get_dice_values() const override;
    [[nodiscard]] bool is_active() const override;

private:
    /**
     * @brief Create a new die using the factory
     * @return Unique pointer to a new die instance
     */
    std::unique_ptr<interfaces::IDice> create_die();

    /**
     * @brief Validate that a face value is within valid range
     * @param face_value The value to validate
     * @return true if valid, false otherwise
     */
    [[nodiscard]] constexpr bool is_valid_face_value(unsigned int face_value) const;
};

} // namespace liarsdice::core