#pragma once

#include <liarsdice/core/player.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <chrono>
#include <thread>

namespace liarsdice::ai {

// Base AI player with configurable strategy
class AIPlayer : public core::Player {
public:
    struct Strategy {
        double risk_tolerance = 0.5;      // 0.0 = conservative, 1.0 = aggressive
        double bluff_frequency = 0.2;     // How often to bluff
        double call_threshold = 0.7;      // Probability threshold to call liar
        unsigned int think_time_ms = 1000; // Simulated thinking time
    };
    
    AIPlayer(unsigned int id, const std::string& name, const Strategy& strategy);
    
    core::Guess make_guess(const std::optional<core::Guess>& last_guess) override;
    bool decide_call_liar(const core::Guess& last_guess) override;
    
protected:
    [[nodiscard]] double calculate_probability(const core::Guess& guess, size_t total_dice) const;
    [[nodiscard]] core::Guess generate_safe_guess(const std::optional<core::Guess>& last_guess) const;
    [[nodiscard]] core::Guess generate_bluff_guess(const std::optional<core::Guess>& last_guess) const;
    
    void simulate_thinking() const;
    
protected:
    Strategy strategy_;
    mutable boost::random::mt19937 rng_;
    mutable boost::random::uniform_real_distribution<> probability_dist_{0.0, 1.0};
    mutable boost::random::uniform_int_distribution<> face_dist_{1, 6};
};

// Predefined AI personalities
class EasyAI : public AIPlayer {
public:
    explicit EasyAI(unsigned int id);
};

class MediumAI : public AIPlayer {
public:
    explicit MediumAI(unsigned int id);
};

class HardAI : public AIPlayer {
public:
    explicit HardAI(unsigned int id);
    
    // Hard AI tracks opponent patterns
    core::Guess make_guess(const std::optional<core::Guess>& last_guess) override;
    bool decide_call_liar(const core::Guess& last_guess) override;
    
private:
    struct OpponentModel {
        std::vector<core::Guess> guess_history;
        double average_bluff_rate = 0.0;
        std::map<unsigned int, double> face_preferences;
    };
    
    std::map<unsigned int, OpponentModel> opponent_models_;
    
    void update_opponent_model(const core::Guess& guess);
    [[nodiscard]] double calculate_adjusted_probability(const core::Guess& guess, size_t total_dice) const;
};

} // namespace liarsdice::ai