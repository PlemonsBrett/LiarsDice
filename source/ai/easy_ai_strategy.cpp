#include <liarsdice/ai/easy_ai_strategy.hpp>
#include <boost/log/trivial.hpp>
#include <algorithm>
#include <cmath>

namespace liarsdice::ai {

EasyAIStrategy::EasyAIStrategy(unsigned int id, const EasyConfig& config)
    : AIPlayer(id, "Easy AI " + std::to_string(id), 
               Strategy{config.risk_tolerance, config.bluff_frequency, 
                       config.call_threshold, config.think_time_ms}),
      config_(config) {
    BOOST_LOG_TRIVIAL(info) << "Created EasyAIStrategy " << id 
                           << " (risk=" << config.risk_tolerance 
                           << ", bluff=" << config.bluff_frequency << ")";
}

EasyAIStrategy::EasyAIStrategy(unsigned int id)
    : EasyAIStrategy(id, EasyConfig{}) {}

core::Guess EasyAIStrategy::make_guess(const std::optional<core::Guess>& last_guess) {
    simulate_thinking();
    
    // Decide whether to bluff based on configuration
    double bluff_roll = probability_dist_(rng_);
    
    if (bluff_roll < config_.bluff_frequency && last_guess.has_value()) {
        BOOST_LOG_TRIVIAL(debug) << get_name() << " decides to bluff";
        return generate_bluff_guess(last_guess);
    }
    
    // Make conservative guess based on own dice
    return generate_conservative_guess(last_guess);
}

bool EasyAIStrategy::decide_call_liar(const core::Guess& last_guess) {
    simulate_thinking();
    
    // Calculate probability of the guess being true
    size_t total_dice = get_dice_count() * 2; // Rough estimate
    double probability = calculate_simple_probability(last_guess, total_dice);
    
    // Apply statistical adjustment if enabled
    if (config_.use_statistical_analysis) {
        probability = apply_statistical_adjustment(probability, last_guess);
    }
    
    // Adjust threshold based on risk tolerance
    double adjusted_threshold = config_.call_threshold - (config_.risk_tolerance * 0.2);
    
    BOOST_LOG_TRIVIAL(debug) << get_name() << " calculates probability " 
                            << probability << " vs threshold " << adjusted_threshold;
    
    return probability < adjusted_threshold;
}

double EasyAIStrategy::calculate_simple_probability(
    const core::Guess& guess, 
    size_t total_dice) const {
    
    // Count our own dice matching the guess
    auto my_dice = get_dice_values();
    unsigned int my_matches = std::count(my_dice.begin(), my_dice.end(), guess.face_value);
    
    // Calculate probability for remaining dice
    size_t other_dice = total_dice - my_dice.size();
    if (other_dice == 0) return my_matches >= guess.dice_count ? 1.0 : 0.0;
    
    // Simple binomial probability
    double p = 1.0 / 6.0; // Probability of any specific face
    unsigned int needed = guess.dice_count > my_matches ? 
                         guess.dice_count - my_matches : 0;
    
    if (needed > other_dice) return 0.0;
    if (needed == 0) return 1.0;
    
    // Approximate using normal distribution for larger numbers
    double mean = other_dice * p;
    double variance = other_dice * p * (1 - p);
    double std_dev = std::sqrt(variance);
    
    // Z-score
    double z = (needed - mean) / std_dev;
    
    // Approximate cumulative probability
    return 1.0 / (1.0 + std::exp(1.5976 * z + 0.070566 * z * z * z));
}

core::Guess EasyAIStrategy::generate_conservative_guess(
    const std::optional<core::Guess>& last_guess) const {
    
    auto my_dice = get_dice_values();
    
    // Count frequencies of our dice
    std::array<unsigned int, 7> frequencies = {0}; // Index 0 unused
    for (auto die : my_dice) {
        if (die >= 1 && die <= 6) {
            frequencies[die]++;
        }
    }
    
    // Find our most common die
    auto max_it = std::max_element(frequencies.begin() + 1, frequencies.end());
    unsigned int best_face = std::distance(frequencies.begin(), max_it);
    unsigned int our_count = *max_it;
    
    if (!last_guess.has_value()) {
        // First guess - be conservative
        unsigned int guess_count = std::max(1u, our_count);
        return core::Guess{guess_count, best_face, get_id()};
    }
    
    // Need to beat last guess
    core::Guess new_guess = *last_guess;
    new_guess.player_id = get_id();
    
    // Try to increase face value first if we have it
    if (last_guess->face_value < 6) {
        for (unsigned int face = last_guess->face_value + 1; face <= 6; ++face) {
            if (frequencies[face] > 0) {
                new_guess.face_value = face;
                return new_guess;
            }
        }
    }
    
    // Otherwise increase dice count conservatively
    new_guess.dice_count = last_guess->dice_count + 1;
    
    // Pick face we actually have
    for (unsigned int face = 1; face <= 6; ++face) {
        if (frequencies[face] > 0) {
            new_guess.face_value = face;
            break;
        }
    }
    
    return new_guess;
}

double EasyAIStrategy::apply_statistical_adjustment(
    double base_probability,
    const core::Guess& guess) const {
    
    // Simple adjustment based on game stage
    // Early game: more conservative
    // Late game: more aggressive
    
    // Estimate game progress (simplified)
    size_t my_dice = get_dice_count();
    double game_progress = 1.0 - (my_dice / 5.0); // 0 = early, 1 = late
    
    // Adjust probability based on game stage
    double adjustment = game_progress * 0.1; // Up to 10% adjustment
    
    // High face values are less likely
    if (guess.face_value >= 5) {
        adjustment -= 0.05;
    }
    
    return std::max(0.0, std::min(1.0, base_probability + adjustment));
}

} // namespace liarsdice::ai