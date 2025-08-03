#include <liarsdice/ai/ai_player.hpp>
#include <boost/log/trivial.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <algorithm>
#include <numeric>

namespace liarsdice::ai {

AIPlayer::AIPlayer(unsigned int id, const std::string& name, const Strategy& strategy)
    : Player(id, name), strategy_(strategy),
      rng_(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count())) {
    BOOST_LOG_TRIVIAL(info) << "Created AI player: " << get_name() 
                            << " (risk=" << strategy.risk_tolerance 
                            << ", bluff=" << strategy.bluff_frequency << ")";
}

core::Guess AIPlayer::make_guess(const std::optional<core::Guess>& last_guess) {
    simulate_thinking();
    
    // Decide whether to bluff
    bool should_bluff = probability_dist_(rng_) < strategy_.bluff_frequency;
    
    core::Guess guess = should_bluff ? 
        generate_bluff_guess(last_guess) : 
        generate_safe_guess(last_guess);
    
    BOOST_LOG_TRIVIAL(debug) << get_name() << " guesses: " 
                             << guess.dice_count << " x " << guess.face_value;
    
    on_guess(guess);
    return guess;
}

bool AIPlayer::decide_call_liar(const core::Guess& last_guess) {
    simulate_thinking();
    
    // Calculate probability of the guess being true
    // Assume ~30 total dice in play (rough estimate)
    size_t estimated_total_dice = 30;
    double probability = calculate_probability(last_guess, estimated_total_dice);
    
    // Adjust threshold based on risk tolerance
    double adjusted_threshold = strategy_.call_threshold * (1.0 - strategy_.risk_tolerance * 0.3);
    
    bool call_liar = probability < adjusted_threshold;
    
    BOOST_LOG_TRIVIAL(debug) << get_name() << " probability assessment: " << probability 
                             << " (threshold: " << adjusted_threshold << ")";
    
    if (call_liar) {
        BOOST_LOG_TRIVIAL(info) << get_name() << " calls LIAR!";
        on_call_liar();
    }
    
    return call_liar;
}

double AIPlayer::calculate_probability(const core::Guess& guess, size_t total_dice) const {
    // Use binomial distribution to calculate probability
    // Each die has 1/6 chance of showing the guessed face value
    boost::math::binomial_distribution<> dist(total_dice, 1.0/6.0);
    
    // Calculate cumulative probability of having at least 'guess.dice_count' dice
    double probability = 1.0 - boost::math::cdf(dist, guess.dice_count - 1);
    
    // Adjust based on our own dice
    size_t our_matching_dice = count_dice_with_value(guess.face_value);
    if (our_matching_dice > 0) {
        // We know about our dice, so adjust the probability
        size_t other_dice = total_dice - get_dice_count();
        size_t needed = guess.dice_count > our_matching_dice ? 
                       guess.dice_count - our_matching_dice : 0;
        
        if (needed == 0) {
            probability = 1.0; // We already have enough
        } else {
            boost::math::binomial_distribution<> other_dist(other_dice, 1.0/6.0);
            probability = 1.0 - boost::math::cdf(other_dist, needed - 1);
        }
    }
    
    return probability;
}

core::Guess AIPlayer::generate_safe_guess(const std::optional<core::Guess>& last_guess) const {
    auto my_dice = get_dice_values();
    std::map<unsigned int, size_t> face_counts;
    
    // Count our dice
    for (auto value : my_dice) {
        face_counts[value]++;
    }
    
    // Find our most common face value
    unsigned int best_face = 1;
    size_t best_count = 0;
    for (const auto& [face, count] : face_counts) {
        if (count > best_count) {
            best_count = count;
            best_face = face;
        }
    }
    
    // Start with a conservative guess based on our dice
    unsigned int quantity = best_count + 1; // Assume others have some too
    unsigned int face_value = best_face;
    
    // Adjust to beat last guess if needed
    if (last_guess) {
        if (face_value < last_guess->face_value || 
            (face_value == last_guess->face_value && quantity <= last_guess->dice_count)) {
            // Need to increase our guess
            if (last_guess->face_value < 6) {
                face_value = last_guess->face_value + 1;
                quantity = last_guess->dice_count;
            } else {
                face_value = last_guess->face_value;
                quantity = last_guess->dice_count + 1;
            }
        }
    }
    
    return core::Guess{quantity, face_value, get_id()};
}

core::Guess AIPlayer::generate_bluff_guess(const std::optional<core::Guess>& last_guess) const {
    unsigned int face_value = face_dist_(rng_);
    unsigned int quantity = 3 + (get_dice_count() / 2); // Moderate bluff
    
    // Adjust to beat last guess if needed
    if (last_guess) {
        if (face_value < last_guess->face_value || 
            (face_value == last_guess->face_value && quantity <= last_guess->dice_count)) {
            quantity = last_guess->dice_count + 1 + (probability_dist_(rng_) < 0.3 ? 1 : 0);
            if (quantity > 10) { // Don't go too crazy
                face_value = std::min(6u, last_guess->face_value + 1);
                quantity = last_guess->dice_count;
            }
        }
    }
    
    return core::Guess{quantity, face_value, get_id()};
}

void AIPlayer::simulate_thinking() const {
    if (strategy_.think_time_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(strategy_.think_time_ms));
    }
}

// Predefined AI personalities
EasyAI::EasyAI(unsigned int id) 
    : AIPlayer(id, "Easy AI " + std::to_string(id), 
               Strategy{0.2, 0.1, 0.8, 500}) {}

MediumAI::MediumAI(unsigned int id) 
    : AIPlayer(id, "Medium AI " + std::to_string(id), 
               Strategy{0.5, 0.25, 0.7, 1000}) {}

HardAI::HardAI(unsigned int id) 
    : AIPlayer(id, "Hard AI " + std::to_string(id), 
               Strategy{0.8, 0.4, 0.6, 1500}) {}

core::Guess HardAI::make_guess(const std::optional<core::Guess>& last_guess) {
    // Update opponent model if there was a last guess
    if (last_guess && last_guess->player_id != get_id()) {
        update_opponent_model(*last_guess);
    }
    
    // Use parent class logic but with pattern recognition
    return AIPlayer::make_guess(last_guess);
}

bool HardAI::decide_call_liar(const core::Guess& last_guess) {
    simulate_thinking();
    
    // Use adjusted probability based on opponent patterns
    size_t estimated_total_dice = 30;
    double probability = calculate_adjusted_probability(last_guess, estimated_total_dice);
    
    double adjusted_threshold = strategy_.call_threshold * (1.0 - strategy_.risk_tolerance * 0.3);
    
    // Also consider opponent's bluff rate
    auto it = opponent_models_.find(last_guess.player_id);
    if (it != opponent_models_.end() && it->second.guess_history.size() >= 5) {
        adjusted_threshold -= it->second.average_bluff_rate * 0.2;
    }
    
    bool call_liar = probability < adjusted_threshold;
    
    if (call_liar) {
        BOOST_LOG_TRIVIAL(info) << get_name() << " calls LIAR! (pattern-adjusted)";
        on_call_liar();
    }
    
    return call_liar;
}

void HardAI::update_opponent_model(const core::Guess& guess) {
    auto& model = opponent_models_[guess.player_id];
    model.guess_history.push_back(guess);
    model.face_preferences[guess.face_value]++;
    
    // Simple bluff detection: high guesses relative to dice count
    if (model.guess_history.size() >= 3) {
        double avg_guess_ratio = 0.0;
        for (const auto& g : model.guess_history) {
            avg_guess_ratio += static_cast<double>(g.dice_count) / 30.0;
        }
        avg_guess_ratio /= model.guess_history.size();
        model.average_bluff_rate = std::min(1.0, avg_guess_ratio * 1.5);
    }
}

double HardAI::calculate_adjusted_probability(const core::Guess& guess, size_t total_dice) const {
    double base_probability = calculate_probability(guess, total_dice);
    
    // Adjust based on opponent patterns
    auto it = opponent_models_.find(guess.player_id);
    if (it != opponent_models_.end() && it->second.guess_history.size() >= 3) {
        // If opponent favors this face value, slightly increase probability
        auto face_it = it->second.face_preferences.find(guess.face_value);
        if (face_it != it->second.face_preferences.end()) {
            double preference_rate = static_cast<double>(face_it->second) / 
                                   it->second.guess_history.size();
            base_probability *= (1.0 + preference_rate * 0.1);
        }
        
        // Adjust for bluff tendency
        base_probability *= (1.0 - it->second.average_bluff_rate * 0.3);
    }
    
    return std::min(1.0, base_probability);
}

} // namespace liarsdice::ai