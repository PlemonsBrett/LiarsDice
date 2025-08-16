#include <algorithm>
#include <boost/log/trivial.hpp>
#include <cmath>
#include <liarsdice/ai/medium_ai_strategy.hpp>
#include <numeric>

namespace liarsdice::ai {

  MediumAIStrategy::MediumAIStrategy(unsigned int id, const MediumConfig& config)
      : AIPlayer(id, "Medium AI " + std::to_string(id),
                 Strategy{config.risk_tolerance, config.bluff_frequency, config.call_threshold,
                          config.think_time_ms}),
        config_(config) {
    BOOST_LOG_TRIVIAL(info) << "Created MediumAIStrategy " << id
                            << " (risk=" << config.risk_tolerance
                            << ", pattern_weight=" << config.pattern_weight << ")";
  }

  MediumAIStrategy::MediumAIStrategy(unsigned int id) : MediumAIStrategy(id, MediumConfig{}) {}

  core::Guess MediumAIStrategy::make_guess(const std::optional<core::Guess>& last_guess) {
    simulate_thinking();
    invalidate_cache();

    // Analyze patterns if we have game history
    if (config_.track_opponents && game_history_ && !game_history_->empty()) {
      cached_face_probabilities_ = analyze_dice_patterns();
      cache_valid_ = true;
    }

    // Decide strategy based on multiple factors
    double bluff_roll = probability_dist_(rng_);
    double situation_modifier = 0.0;

    // Adjust bluff frequency based on opponent patterns
    if (last_guess.has_value() && opponent_models_.count(last_guess->player_id)) {
      const auto& opponent = opponent_models_.at(last_guess->player_id);
      situation_modifier = opponent.aggression_level * 0.2;
    }

    if (bluff_roll < (config_.bluff_frequency + situation_modifier)) {
      BOOST_LOG_TRIVIAL(debug) << get_name() << " makes strategic bluff";
      return generate_bluff_guess(last_guess);
    }

    return generate_strategic_guess(last_guess);
  }

  bool MediumAIStrategy::decide_call_liar(const core::Guess& last_guess) {
    simulate_thinking();

    // Calculate Bayesian probability
    size_t total_dice = get_dice_count() * 2;  // Better estimate based on game state
    double probability = calculate_bayesian_probability(last_guess, total_dice);

    // Detect bluff patterns
    double bluff_probability = detect_bluff_probability(last_guess);

    // Combine probabilities
    double combined_probability = probability * (1.0 - bluff_probability);

    // Adjust threshold based on opponent model
    double adjusted_threshold = config_.call_threshold;
    if (opponent_models_.count(last_guess.player_id)) {
      const auto& opponent = opponent_models_.at(last_guess.player_id);
      adjusted_threshold -= opponent.bluff_rate * 0.15;
    }

    BOOST_LOG_TRIVIAL(debug) << get_name() << " calculates combined probability "
                             << combined_probability << " vs threshold " << adjusted_threshold
                             << " (bluff prob: " << bluff_probability << ")";

    bool should_call = combined_probability < adjusted_threshold;

    // Update opponent model after decision
    if (should_call) {
      // We'll update the model when we see the result
      // For now, mark this as a potential bluff
      if (opponent_models_.count(last_guess.player_id)) {
        opponent_models_[last_guess.player_id].recent_guesses.push_back(last_guess);
        if (opponent_models_[last_guess.player_id].recent_guesses.size() > config_.history_size) {
          opponent_models_[last_guess.player_id].recent_guesses.pop_front();
        }
      }
    }

    return should_call;
  }

  double MediumAIStrategy::calculate_bayesian_probability(const core::Guess& guess,
                                                          size_t total_dice) const {
    // Prior probability based on uniform distribution
    double prior = 1.0 / 6.0;

    // Adjust prior based on game history patterns
    if (cache_valid_ && guess.face_value <= cached_face_probabilities_.size()) {
      prior = cached_face_probabilities_[guess.face_value - 1];
    }

    // Count our own dice
    auto my_dice = get_dice_values();
    unsigned int my_matches = std::count(my_dice.begin(), my_dice.end(), guess.face_value);

    // Calculate likelihood using binomial distribution
    size_t other_dice = total_dice - my_dice.size();
    unsigned int needed = guess.dice_count > my_matches ? guess.dice_count - my_matches : 0;

    if (needed > other_dice) return 0.0;
    if (needed == 0) return 1.0;

    // Bayesian update
    double likelihood = 0.0;
    for (unsigned int k = needed; k <= other_dice; ++k) {
      // Binomial probability
      double binom_coeff = 1.0;
      for (unsigned int i = 0; i < k; ++i) {
        binom_coeff *= (other_dice - i) / (i + 1.0);
      }
      likelihood += binom_coeff * std::pow(prior, k) * std::pow(1 - prior, other_dice - k);
    }

    return likelihood;
  }

  void MediumAIStrategy::update_opponent_model(unsigned int player_id, const core::Guess& guess,
                                               bool was_bluff) {
    auto& model = opponent_models_[player_id];

    // Update guess history
    model.recent_guesses.push_back(guess);
    if (model.recent_guesses.size() > config_.history_size) {
      model.recent_guesses.pop_front();
    }

    // Update face frequency
    model.face_frequency[guess.face_value] += 1.0;
    model.total_guesses++;

    // Update bluff statistics
    if (was_bluff) {
      model.successful_bluffs++;
    }
    model.bluff_rate = static_cast<double>(model.successful_bluffs) / model.total_guesses;

    // Calculate aggression level (high dice counts relative to expected)
    double avg_dice_count = 0.0;
    for (const auto& g : model.recent_guesses) {
      avg_dice_count += g.dice_count;
    }
    avg_dice_count /= model.recent_guesses.size();

    // Expected average around 1/6 of total dice
    size_t total_dice = 10;  // Estimate based on typical game
    double expected_avg = total_dice / 6.0;
    model.aggression_level = std::min(1.0, avg_dice_count / expected_avg);
  }

  double MediumAIStrategy::detect_bluff_probability(const core::Guess& guess) const {
    if (!opponent_models_.count(guess.player_id)) {
      return config_.bluff_frequency;  // Default assumption
    }

    const auto& model = opponent_models_.at(guess.player_id);

    // Base bluff probability from history
    double bluff_prob = model.bluff_rate;

    // Adjust based on pattern analysis
    if (model.recent_guesses.size() >= 3) {
      // Check for escalation patterns
      bool escalating = true;
      for (size_t i = 1; i < model.recent_guesses.size(); ++i) {
        if (model.recent_guesses[i].dice_count <= model.recent_guesses[i - 1].dice_count) {
          escalating = false;
          break;
        }
      }
      if (escalating) {
        bluff_prob += 0.2;  // Rapid escalation suggests bluffing
      }
    }

    // Check if guess deviates from player's face preferences
    double total_face_guesses = 0.0;
    for (const auto& [face, count] : model.face_frequency) {
      total_face_guesses += count;
    }

    if (total_face_guesses > 0 && model.face_frequency.count(guess.face_value)) {
      double face_preference = model.face_frequency.at(guess.face_value) / total_face_guesses;
      if (face_preference < 0.1) {
        bluff_prob += 0.15;  // Unusual face choice
      }
    }

    return std::min(1.0, bluff_prob);
  }

  std::vector<double> MediumAIStrategy::analyze_dice_patterns() const {
    std::vector<double> face_probabilities(6, 1.0 / 6.0);

    if (!game_history_ || game_history_->empty()) {
      return face_probabilities;
    }

    // Analyze recent game states
    auto frequencies = game_history_->get_dice_frequency(config_.history_size);

    // Calculate total dice observed
    double total_observed = std::accumulate(frequencies.begin() + 1, frequencies.end(), 0.0);

    if (total_observed > 0) {
      // Update probabilities based on observed frequencies
      for (size_t i = 1; i <= 6; ++i) {
        double observed_freq = frequencies[i] / total_observed;
        // Weighted average with prior
        face_probabilities[i - 1] = (observed_freq * config_.pattern_weight)
                                    + (face_probabilities[i - 1] * (1.0 - config_.pattern_weight));
      }
    }

    return face_probabilities;
  }

  core::Guess MediumAIStrategy::generate_strategic_guess(
      const std::optional<core::Guess>& last_guess) const {
    auto my_dice = get_dice_values();
    std::array<unsigned int, 7> frequencies = {0};

    for (auto die : my_dice) {
      if (die >= 1 && die <= 6) {
        frequencies[die]++;
      }
    }

    // If we have pattern data, use it to make informed guesses
    if (cache_valid_) {
      // Find face with highest expected value
      double best_score = -1.0;
      unsigned int best_face = 1;

      for (unsigned int face = 1; face <= 6; ++face) {
        double my_contribution = frequencies[face];
        double expected_others
            = cached_face_probabilities_[face - 1] * (10 - my_dice.size());  // Estimate other dice
        double score = my_contribution + expected_others;

        if (score > best_score) {
          best_score = score;
          best_face = face;
        }
      }

      if (!last_guess.has_value()) {
        unsigned int guess_count = std::max(1u, static_cast<unsigned int>(best_score * 0.8));
        return core::Guess{guess_count, best_face, get_id()};
      }

      // Beat last guess strategically
      core::Guess new_guess = *last_guess;
      new_guess.player_id = get_id();

      // Try to use our analysis to make a good guess
      if (last_guess->dice_count < static_cast<unsigned int>(best_score)) {
        new_guess.dice_count = last_guess->dice_count + 1;
        new_guess.face_value = best_face;
      } else {
        new_guess.dice_count = last_guess->dice_count + 1;
        new_guess.face_value = (last_guess->face_value % 6) + 1;
      }

      return new_guess;
    }

    // Fall back to safe guess
    return generate_safe_guess(last_guess);
  }

}  // namespace liarsdice::ai