#pragma once

#include <liarsdice/ai/ai_player.hpp>
#include <liarsdice/core/game_state_storage.hpp>
#include <deque>
#include <unordered_map>

namespace liarsdice::ai {

/**
 * @brief Medium AI Strategy with statistical analysis
 * 
 * Advanced AI that uses Bayesian probability, opponent modeling,
 * and pattern recognition to make informed decisions.
 */
class MediumAIStrategy : public AIPlayer {
public:
    struct MediumConfig {
        double risk_tolerance = 0.5;       // 0.0-1.0 (conservative to aggressive)
        double bluff_frequency = 0.25;     // How often to bluff (0.0-1.0)
        double call_threshold = 0.65;      // Probability threshold to call liar
        double pattern_weight = 0.3;       // Weight for pattern-based adjustments
        size_t history_size = 20;          // Number of states to analyze
        bool use_bayesian = true;          // Enable Bayesian probability
        bool track_opponents = true;       // Enable opponent modeling
        unsigned int think_time_ms = 1000; // Simulated thinking time
    };
    
    explicit MediumAIStrategy(unsigned int id, const MediumConfig& config);
    explicit MediumAIStrategy(unsigned int id);
    
    core::Guess make_guess(const std::optional<core::Guess>& last_guess) override;
    bool decide_call_liar(const core::Guess& last_guess) override;
    
    // Game context integration
    void set_game_history(const core::GameHistory* history) { game_history_ = history; }
    void set_game_state(const core::GameStateStorage* state) { game_state_ = state; }
    
    // Configuration access
    void set_config(const MediumConfig& config) { config_ = config; }
    const MediumConfig& get_config() const { return config_; }
    
protected:
    /**
     * @brief Calculate Bayesian probability for a guess
     */
    [[nodiscard]] double calculate_bayesian_probability(
        const core::Guess& guess,
        size_t total_dice
    ) const;
    
    /**
     * @brief Model opponent behavior patterns
     */
    struct OpponentPattern {
        std::deque<core::Guess> recent_guesses;
        std::unordered_map<unsigned int, double> face_frequency;
        double bluff_rate = 0.0;
        double aggression_level = 0.0;
        size_t total_guesses = 0;
        size_t successful_bluffs = 0;
    };
    
    /**
     * @brief Update opponent model based on revealed information
     */
    void update_opponent_model(
        unsigned int player_id,
        const core::Guess& guess,
        bool was_bluff
    );
    
    /**
     * @brief Detect bluff patterns in opponent behavior
     */
    [[nodiscard]] double detect_bluff_probability(
        const core::Guess& guess
    ) const;
    
    /**
     * @brief Analyze game history for patterns
     */
    [[nodiscard]] std::vector<double> analyze_dice_patterns() const;
    
    /**
     * @brief Generate strategic guess based on analysis
     */
    [[nodiscard]] core::Guess generate_strategic_guess(
        const std::optional<core::Guess>& last_guess
    ) const;
    
private:
    MediumConfig config_;
    const core::GameHistory* game_history_ = nullptr;
    const core::GameStateStorage* game_state_ = nullptr;
    
    // Opponent modeling data
    std::unordered_map<unsigned int, OpponentPattern> opponent_models_;
    
    // Pattern recognition cache
    mutable std::vector<double> cached_face_probabilities_;
    mutable bool cache_valid_ = false;
    
    void invalidate_cache() { cache_valid_ = false; }
};

} // namespace liarsdice::ai