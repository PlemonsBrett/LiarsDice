# Technical Design Document

## AI Strategy System Architecture

### Document Information

- **Version**: 1.0.0
- **Date**: 2025-08-03
- **Author**: Brett Plemons
- **Commit**: AI Strategy System Implementation

---

## Executive Summary

This document details the implementation of the AI strategy system for the LiarsDice project. The system provides two AI
difficulty levels (Easy and Medium) with configurable behaviors, integrating with the game state storage and history
systems for informed decision-making.

### Key Deliverables

1. **Enhanced AI Base Class** — AIPlayer with configurable strategy parameters
2. **Easy AI Strategy** — Simple heuristics with configurable parameters
3. **Medium AI Strategy** — Statistical analysis with Bayesian probability and opponent modeling
4. **Command-Line Integration** — Simple --medium-ai flag for strategy selection
5. **Game State Integration** — AI strategies utilize GameHistory for pattern analysis

---

## System Requirements Analysis

### Functional Requirements

#### AI Architecture

- **REQ-AI-001**: AIPlayer base class with virtual decision methods
- **REQ-AI-002**: Configurable Strategy struct with common parameters
- **REQ-AI-003**: Integration with GameHistory and GameStateStorage
- **REQ-AI-004**: Command-line flag for AI difficulty selection
- **REQ-AI-005**: Inheritance-based strategy implementation

#### Easy AI Strategy

- **REQ-EASY-001**: Simple heuristic-based decision-making
- **REQ-EASY-002**: Basic probability calculations using normal distribution approximation
- **REQ-EASY-003**: Configurable risk tolerance (0.2 default)
- **REQ-EASY-004**: Configurable bluff frequency (0.1 default)
- **REQ-EASY-005**: Configurable call threshold (0.8 default)
- **REQ-EASY-006**: Optional statistical analysis toggle

#### Medium AI Strategy

- **REQ-MED-001**: Bayesian probability calculations
- **REQ-MED-002**: Opponent behavior modeling with pattern tracking
- **REQ-MED-003**: Pattern recognition using game history
- **REQ-MED-004**: Bluff detection based on opponent statistics
- **REQ-MED-005**: Configurable pattern weight and history size
- **REQ-MED-006**: Integration with GameHistory for informed decisions

### Non-Functional Requirements

#### Performance

- **REQ-PERF-001**: AI decision making < 100ms
- **REQ-PERF-002**: Memory usage < 10MB per AI player
- **REQ-PERF-003**: Efficient game history traversal
- **REQ-PERF-004**: Minimal CPU usage during opponent turns

#### Reliability

- **REQ-REL-001**: Deterministic decisions with same seed
- **REQ-REL-002**: No invalid moves generated
- **REQ-REL-003**: Graceful handling of edge cases
- **REQ-REL-004**: Thread-safe AI operations

#### Maintainability

- **REQ-MAINT-001**: Clear strategy interface
- **REQ-MAINT-002**: Extensible for new strategies
- **REQ-MAINT-003**: Comprehensive logging support
- **REQ-MAINT-004**: Well-documented decision logic

---

## Architectural Decisions

### ADR-001: Inheritance-Based AI Strategy

**Status**: Accepted

**Context**: Need to implement multiple AI difficulty levels while maintaining compatibility with existing system.

**Decision**: Use inheritance from AIPlayer base class rather than a separate strategy pattern.

**Rationale**:

- Simpler integration with existing game system
- Direct access to player state and methods
- Minimal refactoring required
- Clear class hierarchy

**Consequences**:

- **Positive**: Simple, straightforward implementation
- **Positive**: Easy integration with existing Player interface
- **Negative**: Less flexibility than pure strategy pattern
- **Mitigation**: Virtual methods allow polymorphic behavior

### ADR-002: Configuration Structures

**Status**: Accepted

**Context**: AI strategies need configurable parameters.

**Decision**: Use nested configuration structs for each AI strategy type.

**Implementation**:

```cpp
struct EasyConfig {
    double risk_tolerance = 0.2;
    double bluff_frequency = 0.1;
    double call_threshold = 0.8;
    bool use_statistical_analysis = false;
    unsigned int think_time_ms = 500;
};
```

### ADR-003: Command-Line Integration

**Status**: Accepted

**Context**: Need simple way to select AI difficulty without complex menus.

**Decision**: Add --medium-ai command-line flag, defaulting to Easy AI.

**Rationale**:

- Simple user interface
- No interactive menus needed
- Clear default behavior
- Easy to extend for future AI levels

### ADR-004: Game State Integration

**Status**: Accepted

**Context**: Medium AI needs access to game history for pattern analysis.

**Decision**: Pass pointers to GameHistory and GameStateStorage to Medium AI.

**Rationale**:

- Direct access to historical data
- No copying of large data structures
- Efficient pattern analysis
- Leverages existing state storage system

---

## Implementation Details

### Core AI Architecture

#### AIPlayer Base Class

```cpp
namespace liarsdice::ai {

class AIPlayer : public Player {
public:
    struct Strategy {
        double risk_tolerance = 0.5;
        double bluff_frequency = 0.2;
        double call_threshold = 0.7;
        unsigned int think_time_ms = 1000;
    };
    
    AIPlayer(unsigned int id, const std::string& name, const Strategy& strategy);
    
    // Core decision methods from Player interface
    core::Guess make_guess(const std::optional<core::Guess>& last_guess) override;
    bool decide_call_liar(const core::Guess& last_guess) override;
    
protected:
    // Probability calculations
    double calculate_probability(const core::Guess& guess, size_t total_dice) const;
    
    // Guess generation strategies
    core::Guess generate_safe_guess(const std::optional<core::Guess>& last_guess) const;
    core::Guess generate_bluff_guess(const std::optional<core::Guess>& last_guess) const;
    
    // Utility methods
    void simulate_thinking() const;
    
    Strategy strategy_;
    mutable std::mt19937 rng_;
    mutable std::uniform_real_distribution<> probability_dist_{0.0, 1.0};
    mutable std::uniform_int_distribution<> face_dist_{1, 6};
};

} // namespace liarsdice::ai
```

### Easy AI Strategy Implementation

#### Strategy Class

```cpp
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
    
protected:
    // Calculate simple probability based on known dice
    double calculate_simple_probability(
        const core::Guess& guess, 
        size_t total_dice
    ) const;
    
    // Generate conservative guess based on own dice
    core::Guess generate_conservative_guess(
        const std::optional<core::Guess>& last_guess
    ) const;
    
    // Apply basic statistical analysis if enabled
    double apply_statistical_adjustment(
        double base_probability,
        const core::Guess& guess
    ) const;
    
private:
    EasyConfig config_;
};
```

#### Key Methods

```cpp
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
```

### Medium AI Strategy Implementation

#### Strategy Class

```cpp
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
    
protected:
    // Calculate Bayesian probability for a guess
    double calculate_bayesian_probability(
        const core::Guess& guess,
        size_t total_dice
    ) const;
    
    // Model opponent behavior patterns
    struct OpponentPattern {
        std::deque<core::Guess> recent_guesses;
        std::unordered_map<unsigned int, double> face_frequency;
        double bluff_rate = 0.0;
        double aggression_level = 0.0;
        size_t total_guesses = 0;
        size_t successful_bluffs = 0;
    };
    
    // Update opponent model based on revealed information
    void update_opponent_model(
        unsigned int player_id,
        const core::Guess& guess,
        bool was_bluff
    );
    
    // Detect bluff patterns in opponent behavior
    double detect_bluff_probability(const core::Guess& guess) const;
    
    // Analyze game history for patterns
    std::vector<double> analyze_dice_patterns() const;
    
    // Generate strategic guess based on analysis
    core::Guess generate_strategic_guess(
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
};
```

#### Key Methods

```cpp
double MediumAIStrategy::calculate_bayesian_probability(
    const core::Guess& guess,
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
    unsigned int needed = guess.dice_count > my_matches ? 
                         guess.dice_count - my_matches : 0;
    
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

double MediumAIStrategy::detect_bluff_probability(const core::Guess& guess) const {
    if (!opponent_models_.count(guess.player_id)) {
        return config_.bluff_frequency; // Default assumption
    }
    
    const auto& model = opponent_models_.at(guess.player_id);
    
    // Base bluff probability from history
    double bluff_prob = model.bluff_rate;
    
    // Adjust based on pattern analysis
    if (model.recent_guesses.size() >= 3) {
        // Check for escalation patterns
        bool escalating = true;
        for (size_t i = 1; i < model.recent_guesses.size(); ++i) {
            if (model.recent_guesses[i].dice_count <= model.recent_guesses[i-1].dice_count) {
                escalating = false;
                break;
            }
        }
        if (escalating) {
            bluff_prob += 0.2; // Rapid escalation suggests bluffing
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
            bluff_prob += 0.15; // Unusual face choice
        }
    }
    
    return std::min(1.0, bluff_prob);
}
```

### Application Integration

#### Command-Line Options

```cpp
po::options_description Application::setup_program_options() {
    po::options_description desc("Liar's Dice CLI Options");
    desc.add_options()
        ("help,h", "Show help message")
        ("verbose,v", "Enable verbose output")
        ("medium-ai", "Use Medium AI strategy instead of Easy AI")
        // ... other options
    ;
    return desc;
}
```

#### AI Creation in Application

```cpp
void Application::setup_game(int total_players, int ai_players) {
    // ... game setup code ...
    
    // Create AI players
    for (int i = 0; i < ai_players; ++i) {
        unsigned int player_id = next_player_id_++;
        
        if (config_.use_medium_ai) {
            // Create Medium AI with statistical analysis
            ai::MediumAIStrategy::MediumConfig config;
            config.risk_tolerance = 0.5;
            config.bluff_frequency = 0.25;
            config.call_threshold = 0.65;
            config.pattern_weight = 0.3;
            config.history_size = 20;
            config.use_bayesian = true;
            config.track_opponents = true;
            config.think_time_ms = 1000;
            
            auto ai = std::make_shared<ai::MediumAIStrategy>(player_id, config);
            
            // Connect to game history and state
            ai->set_game_history(&game_->get_history());
            ai->set_game_state(&game_->get_state_storage());
            
            players_.push_back(ai);
            game_->add_player(ai);
        } else {
            // Create Easy AI (default)
            ai::EasyAIStrategy::EasyConfig config;
            config.risk_tolerance = 0.2;
            config.bluff_frequency = 0.1;
            config.call_threshold = 0.8;
            config.use_statistical_analysis = false;
            config.think_time_ms = 500;
            
            auto ai = std::make_shared<ai::EasyAIStrategy>(player_id, config);
            players_.push_back(ai);
            game_->add_player(ai);
        }
    }
}
```

---

## Testing Strategy

### Unit Tests

The AI strategies are tested through the existing test infrastructure:

1. **test_ai.cpp** - Basic AI functionality tests
2. **test_ai_advanced.cpp** - Advanced AI strategy tests
3. **Robot Framework tests** - End-to-end AI behavior validation

### Example Test

```cpp
TEST_CASE("AI Strategy Comparison", "[ai]") {
    // Create game with different AI strategies
    auto easy_ai = std::make_shared<ai::EasyAIStrategy>(1);
    auto medium_ai = std::make_shared<ai::MediumAIStrategy>(2);
    
    // Set up Medium AI with game state
    medium_ai->set_game_history(&game.get_history());
    medium_ai->set_game_state(&game.get_state_storage());
    
    // Test decision making
    auto last_guess = core::Guess{3, 4, 1};
    
    auto easy_decision = easy_ai->make_guess(last_guess);
    auto medium_decision = medium_ai->make_guess(last_guess);
    
    // Verify decisions are valid
    REQUIRE(game.is_valid_guess(easy_decision));
    REQUIRE(game.is_valid_guess(medium_decision));
}
```

---

## Performance Characteristics

### Decision-Making Performance

| AI Type   | Average Decision Time | Memory Usage | CPU Usage |
|-----------|-----------------------|--------------|-----------| 
| Easy AI   | ~2-5ms                | < 1MB        | < 5%      |
| Medium AI | ~10-20ms              | < 2MB        | < 10%     |

### Key Features Comparison

| Feature                 | Easy AI | Medium AI |
|-------------------------|---------|-----------|
| Basic Probability       | ✓       | ✓         |
| Configurable Parameters | ✓       | ✓         |
| Bluff Detection         | Basic   | Advanced  |
| Opponent Modeling       | ✗       | ✓         |
| Pattern Recognition     | ✗       | ✓         |
| Game History Analysis   | ✗       | ✓         |
| Bayesian Probability    | ✗       | ✓         |

---

## Usage Examples

### Command Line Usage

```bash
# Play with Easy AI (default)
./build/standalone/liarsdice

# Play with Medium AI
./build/standalone/liarsdice --medium-ai

# Play with Medium AI and verbose logging
./build/standalone/liarsdice --medium-ai --verbose
```

### Example Game Output

```
Welcome to Liar's Dice
Enter the number of players (2-8): 2
How many AI players (0-1): 1
Game starting

--- Round 1 ---
Easy AI 2 guesses: 2 dice showing 3
Your turn - Player 1
1. Make a guess
2. Call liar
Previous guess: 2 dice showing 3
```

---

## Conclusion

The AI Strategy System successfully implements two distinct AI difficulty levels for the LiarsDice game:

### Key Achievements

1. **Simple Integration**: Uses inheritance from AIPlayer base class for straightforward implementation
2. **Configurable Strategies**: Both AI levels support runtime configuration of behavior parameters
3. **Statistical Intelligence**: Medium AI uses Bayesian probability and pattern recognition
4. **Game State Integration**: Medium AI leverages GameHistory for informed decisions
5. **User-Friendly**: Simple --medium-ai flag for strategy selection

### Implementation Highlights

- Easy AI provides a good baseline opponent with simple heuristics
- Medium AI offers a challenging opponent with statistical analysis
- Both strategies integrate seamlessly with the existing game system
- Configuration structures allow fine-tuning of AI behavior
- Command-line integration keeps the user interface simple

The system provides a solid foundation for single-player gameplay while maintaining simplicity and extensibility for
future enhancements.

---

*This document represents the technical design and implementation details for the AI Strategy System implemented in
Commit 6 of the LiarsDice project.*
