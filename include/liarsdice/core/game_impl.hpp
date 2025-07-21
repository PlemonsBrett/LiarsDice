#pragma once

#include "liarsdice/interfaces/i_game.hpp"
#include "liarsdice/interfaces/i_game_state.hpp"
#include "liarsdice/interfaces/i_player.hpp"
#include "liarsdice/interfaces/i_random_generator.hpp"
#include "liarsdice/interfaces/i_service_factory.hpp"
#include <memory>
#include <string>
#include <vector>

// LLDB debugging support
#ifdef __clang__
#define LLDB_VISUALIZER(name) __attribute__((annotate("lldb.visualizer:" name)))
#define LLDB_DEBUG_INFO __attribute__((used, retain))
#else
#define LLDB_VISUALIZER(name)
#define LLDB_DEBUG_INFO
#endif

namespace liarsdice::core {

// Forward declarations and type aliases
using PlayerFactory = interfaces::ServiceFactory<interfaces::IPlayer>;

}

namespace liarsdice::core {

/**
 * @brief Modern C++23 implementation of IGame with dependency injection
 * 
 * This class implements the IGame interface using dependency injection
 * to manage players, game state, and random number generation.
 * Designed for testability and modularity.
 */
class LLDB_VISUALIZER("GameImpl") GameImpl final : public interfaces::IGame {
private:
    std::unique_ptr<interfaces::IGameState> game_state_;
    std::unique_ptr<interfaces::IRandomGenerator> random_generator_;
    std::unique_ptr<PlayerFactory> player_factory_;
    
    // Configuration constants
    static constexpr size_t kMinPlayers = 2;
    static constexpr size_t kMaxPlayers = 8;
    static constexpr size_t kInitialDicePerPlayer = 5;
    
    std::vector<std::unique_ptr<interfaces::IPlayer>> players_;
    bool initialized_ = false;

public:
    /**
     * @brief Constructor with dependency injection
     * 
     * @param game_state Game state manager
     * @param random_generator Random number generator
     * @param player_factory Factory for creating player instances
     */
    explicit GameImpl(
        std::unique_ptr<interfaces::IGameState> game_state,
        std::unique_ptr<interfaces::IRandomGenerator> random_generator,
        std::unique_ptr<PlayerFactory> player_factory
    );

    ~GameImpl() override = default;

    // Non-copyable, movable
    GameImpl(const GameImpl&) = delete;
    GameImpl& operator=(const GameImpl&) = delete;
    GameImpl(GameImpl&&) = default;
    GameImpl& operator=(GameImpl&&) = default;

    // IGame interface implementation
    void initialize() override;
    void add_player(int player_id) override;
    void start_game() override;
    [[nodiscard]] bool is_game_over() const override;
    [[nodiscard]] int get_winner_id() const override;
    [[nodiscard]] std::string validate_guess(const interfaces::Guess& guess) const override;
    bool process_guess(const interfaces::Guess& guess) override;
    std::string process_liar_call(int calling_player_id) override;
    interfaces::IGameState& get_game_state() override;
    [[nodiscard]] const interfaces::IGameState& get_game_state() const override;
    void reset() override;
    [[nodiscard]] size_t get_min_players() const override;
    [[nodiscard]] size_t get_max_players() const override;
    [[nodiscard]] size_t get_initial_dice_per_player() const override;

private:
    /**
     * @brief Initialize all players with starting dice
     */
    void initialize_players();

    /**
     * @brief Remove a player from the game when they lose all dice
     * @param player_index Index of the player to remove
     */
    void eliminate_player(size_t player_index);

    /**
     * @brief Check if the game has a winner
     * @return Player ID of winner, or -1 if no winner yet
     */
    [[nodiscard]] int determine_winner() const;

    /**
     * @brief Validate that a guess follows game rules
     * @param new_guess The guess to validate
     * @param last_guess The previous guess (if any)
     * @return Error message if invalid, empty string if valid
     */
    [[nodiscard]] std::string validate_guess_rules(
        const interfaces::Guess& new_guess,
        const std::optional<interfaces::Guess>& last_guess
    ) const;

    /**
     * @brief Count total dice with specific face value across all players
     * @param face_value The face value to count
     * @return Total count
     */
    [[nodiscard]] size_t count_dice_with_value(unsigned int face_value) const;

    /**
     * @brief Find player by ID
     * @param player_id The player ID to find
     * @return Iterator to player, or end() if not found
     */
    auto find_player_by_id(int player_id) -> decltype(players_.begin());
    
    /**
     * @brief Find player by ID (const version)
     * @param player_id The player ID to find
     * @return Const iterator to player, or end() if not found
     */
    [[nodiscard]] auto find_player_by_id(int player_id) const -> decltype(players_.cbegin());
};

} // namespace liarsdice::core