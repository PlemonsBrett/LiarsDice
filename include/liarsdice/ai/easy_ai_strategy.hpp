#pragma once

#include <liarsdice/ai/ai_player.hpp>
#include <liarsdice/core/game_state_storage.hpp>

namespace liarsdice::ai {

/**
 * @brief Easy AI Strategy with configurable parameters
 * 
 * Simple heuristics-based AI that makes conservative decisions
 * with configurable risk tolerance and behavior parameters.
 */
class EasyAIStrategy : public AIPlayer {
public:
    struct EasyConfig {
        double risk_tolerance = 0.2;       // 0.0-1.0 (conservative to aggressive)
        double bluff_frequency = 0.1;      // How often to bluff (0.0-1.0)
        double call_threshold = 0.8;       // Probability threshold to call liar
        bool use_statistical_analysis = false; // Toggle for basic statistics
        unsigned int think_time_ms = 500;  // Simulated thinking time
    };
    
    explicit EasyAIStrategy(unsigned int id, const EasyConfig& config);
    explicit EasyAIStrategy(unsigned int id);
    
    core::Guess make_guess(const std::optional<core::Guess>& last_guess) override;
    bool decide_call_liar(const core::Guess& last_guess) override;
    
    // Configuration access
    void set_config(const EasyConfig& config) { config_ = config; }
    const EasyConfig& get_config() const { return config_; }
    
protected:
    /**
     * @brief Calculate simple probability based on known dice
     */
    [[nodiscard]] double calculate_simple_probability(
        const core::Guess& guess, 
        size_t total_dice
    ) const;
    
    /**
     * @brief Generate conservative guess based on own dice
     */
    [[nodiscard]] core::Guess generate_conservative_guess(
        const std::optional<core::Guess>& last_guess
    ) const;
    
    /**
     * @brief Apply basic statistical analysis if enabled
     */
    [[nodiscard]] double apply_statistical_adjustment(
        double base_probability,
        const core::Guess& guess
    ) const;
    
private:
    EasyConfig config_;
};

} // namespace liarsdice::ai