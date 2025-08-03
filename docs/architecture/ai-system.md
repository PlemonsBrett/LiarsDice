# AI System Architecture

## Overview

The Liar's Dice AI system is designed with flexibility, extensibility, and performance in mind. It employs several
design patterns to create a robust framework for implementing computer-controlled players with varying difficulty levels
and play styles.

## Architecture Goals

1. **Extensibility**: Easy addition of new AI strategies without modifying existing code
2. **Testability**: AI decisions can be tested in isolation from game logic
3. **Performance**: Efficient decision-making within reasonable time constraints
4. **Configurability**: Runtime adjustment of AI behavior parameters
5. **Maintainability**: Clear separation of concerns and well-defined interfaces

## Core Design Patterns

### Inheritance-Based AI

The AI system uses inheritance from the base `Player` class to implement computer-controlled players.

{% raw %}
```cpp
class AIPlayer : public core::Player {
public:
    struct Strategy {
        double risk_tolerance = 0.5;
        double bluff_frequency = 0.2;
        double call_threshold = 0.7;
        unsigned int think_time_ms = 1000;
    };
    
    core::Guess make_guess(const std::optional<core::Guess>& last_guess) override;
    bool decide_call_liar(const core::Guess& last_guess) override;
};
```

{% endraw %}

**Benefits**:

- Algorithms can be selected at runtime
- New strategies can be added without modifying existing code
- Each strategy can be tested independently
- Strategies can be swapped during gameplay

### Predefined AI Classes

The system provides predefined AI difficulty levels through specialized classes:

{% raw %}
```cpp
class EasyAI : public AIPlayer {
public:
    explicit EasyAI(unsigned int id)
        : AIPlayer(id, "Easy AI", {
            .risk_tolerance = 0.2,
            .bluff_frequency = 0.1,
            .call_threshold = 0.8,
            .think_time_ms = 500
        }) {}
};

class MediumAI : public AIPlayer {
public:
    explicit MediumAI(unsigned int id)
        : AIPlayer(id, "Medium AI", {
            .risk_tolerance = 0.5,
            .bluff_frequency = 0.2,
            .call_threshold = 0.7,
            .think_time_ms = 1000
        }) {}
};
```

{% endraw %}

**Benefits**:

- Simple inheritance model
- Clear difficulty progression
- Easy to instantiate and use
- Integrates seamlessly with game loop

### Virtual Method Override Pattern

AI behavior is implemented by overriding virtual methods from the base Player class:

{% raw %}
```cpp
// In AIPlayer class
core::Guess make_guess(const std::optional<core::Guess>& last_guess) override {
    simulate_thinking();
    
    double random = probability_dist_(rng_);
    if (random < strategy_.bluff_frequency) {
        return generate_bluff_guess(last_guess);
    } else {
        return generate_safe_guess(last_guess);
    }
}

bool decide_call_liar(const core::Guess& last_guess) override {
    simulate_thinking();
    double probability = calculate_probability(last_guess, total_dice_in_game);
    return probability < (1.0 - strategy_.call_threshold);
}
```

{% endraw %}

**Benefits**:

- Natural integration with game flow
- AI players work exactly like human players
- No special handling required in game logic
- Simple and straightforward implementation

## Component Architecture

### AI Decision Information

The AI has access to game state through the Player class interface:

```
AI Information Access
├── Own State
│   ├── dice_ (vector of own dice)
│   ├── get_dice_count()
│   └── is_eliminated()
├── Game Information
│   ├── last_guess (previous player's guess)
│   ├── total_dice_in_game (calculated)
│   └── number_of_players (from game)
└── Random Number Generation
    ├── boost::random::mt19937 (RNG)
    ├── uniform_real_distribution (0.0-1.0)
    └── uniform_int_distribution (1-6)
```

### AI Class Hierarchy

```
core::Player (base class)
└── ai::AIPlayer
    ├── ai::EasyAI
    │   ├── Low risk tolerance (0.2)
    │   ├── Minimal bluffing (0.1)
    │   └── Conservative play
    ├── ai::MediumAI
    │   ├── Balanced risk (0.5)
    │   ├── Moderate bluffing (0.2)
    │   └── Strategic play
    └── ai::HardAI
        ├── High risk tolerance (0.8)
        ├── Frequent bluffing (0.3)
        └── Aggressive play
```

## Implementation Details

### Easy AI Implementation

The Easy AI uses simple heuristics and basic probability calculations:

1. **Guess Generation**:
   - Count own dice matching potential guess
    - Estimate opponent dice using uniform distribution
    - Apply risk tolerance to adjust estimates
   - Consider bluff frequency for occasional aggressive guesses

2. **Call Decision**:
   - Calculate probability of current guess being true
    - Compare against call threshold
    - Factor in game state (remaining dice, players)

### Medium AI Implementation

The Medium AI uses balanced statistical techniques:

1. **Enhanced Probability Calculation**:
   {% raw %}
   ```cpp
   double calculate_probability(const core::Guess& guess, size_t total_dice) const {
       // Count our matching dice
       auto my_matches = count_matching_dice(guess.face);
       
       // Calculate expected matches in unknown dice
       size_t unknown_dice = total_dice - get_dice_count();
       double expected_matches = unknown_dice / 6.0;
       
       // Apply strategy adjustments
       return (my_matches + expected_matches) / guess.count;
   }
   ```
   {% endraw %}

2. **Strategic Guessing**:
   - Balances between safe and aggressive plays
   - Considers game state when making decisions
   - Moderate bluffing based on configuration

3. **Adaptive Calling**:
   - Adjusts the call threshold based on game progress
   - More likely to call when fewer dice remain
   - Considers risk vs. reward

### Decision Flow

```
make_guess() / decide_call_liar()
├── Simulate Thinking
│   ├── Apply configured delay
│   └── Make AI feel human-like
├── Analyze Situation
│   ├── Count own dice
│   ├── Calculate total dice
│   └── Check last guess
├── Apply Strategy
│   ├── Use risk tolerance
│   ├── Consider bluff frequency
│   └── Apply call threshold
├── Generate Decision
│   ├── For guesses: safe vs bluff
│   ├── For calls: probability check
│   └── Apply randomization
└── Return Result
```

## Performance Considerations

### Time Complexity

- **Easy AI**: O(n) where n is the number of opponents
- **Medium AI**: O(n * m) where m is history length
- **Decision Time**: Target < 2 seconds for user experience

### Memory Usage

- **Per Strategy**: ~1KB base + opponent models
- **Opponent Model**: ~100 bytes per opponent
- **History Storage**: Configurable, typically last 20 bids

### Optimization Techniques

1. **Caching**:
   ```cpp
   class MediumAIStrategy {
       mutable std::unordered_map<BidHash, double> probability_cache_;
       mutable std::optional<OpponentModels> cached_models_;
   };
   ```

2. **Early Exit**:
    - Skip complex calculations for obvious decisions
    - Use heuristics before full analysis

3. **Boost.Random Efficiency**:
   - Thread-safe random number generation
   - Reuse RNG instance across calls

## Configuration System

AI behavior is configured through the Strategy struct:

{% raw %}
```cpp
struct Strategy {
    double risk_tolerance = 0.5;      // 0.0 = conservative, 1.0 = aggressive
    double bluff_frequency = 0.2;     // How often to bluff
    double call_threshold = 0.7;      // Probability threshold to call liar
    unsigned int think_time_ms = 1000; // Simulated thinking time
};
```

{% endraw %}

Configuration is set through:

- Constructor parameters for predefined AI
- Custom Strategy struct for custom AI
- Direct instantiation with specific values
- UI configuration (via UIConfig class)

## Testing Strategy

### Unit Tests

1. **Decision Validity**:
    - All decisions follow game rules
    - Bids are always higher than current
    - No impossible dice combinations

2. **Behavior Tests**:
    - Risk tolerance affects bid aggression
    - Bluff frequency matches configuration
    - Call threshold triggers appropriately

3. **Performance Tests**:
    - Decision time within limits
    - Memory usage stable over many games
    - No performance degradation over time

### Integration Tests

1. **Game Simulation**:
    - AI vs AI games complete successfully
    - Reasonable win rates between difficulties
    - No deadlocks or infinite loops

2. **Stress Tests**:
    - Multiple AI players (6+)
   - Rapid decision-making
    - Long game sessions

## Future Enhancements

### Planned Features

1. **Hard AI Strategy**:
    - Monte Carlo Tree Search (MCTS)
    - Neural network integration
    - Perfect information analysis

2. **Learning System**:
    - Persistent opponent models
    - Strategy adaptation
    - Play style clustering

3. **Advanced Features**:
    - Multi-level reasoning ("I think that you think...")
    - Psychological modeling
    - Tournament-level play

### Extension Points

1. **Custom Strategies**:
   ```cpp
   class CustomAI : public IAIStrategy {
       // Implement domain-specific logic
   };
   ```

2. **Strategy Decorators**:
   ```cpp
   class LoggingStrategy : public IAIStrategy {
       std::unique_ptr<IAIStrategy> wrapped_;
       // Log all decisions
   };
   ```

3. **Hybrid Strategies**:
   ```cpp
   class AdaptiveAI : public IAIStrategy {
       std::vector<std::unique_ptr<IAIStrategy>> strategies_;
       // Switch strategies based on game state
   };
   ```

## Best Practices

### For AI Implementers

1. **Inherit from AIPlayer**: Extend the base AI class
2. **Override Virtual Methods**: Implement make_guess and decide_call_liar
3. **Use Boost.Random**: For all randomization needs
4. **Respect Think Time**: Add appropriate delays
5. **Configuration**: Use the Strategy struct for parameters

### For System Integrators

1. **Register Early**: Register all strategies at startup
2. **Error Handling**: Handle unknown strategy names gracefully
3. **Performance Monitoring**: Track AI decision times
4. **Fair Play**: Ensure AI doesn't have unfair advantages
5. **User Experience**: Add appropriate delays for realism

## Conclusion

The AI system architecture provides a straightforward and effective implementation of computer opponents using
inheritance
and virtual methods. The use of Boost libraries ensures reliable random number generation and consistent behavior. The
simple Strategy configuration struct makes it easy to tune AI behavior, while the predefined difficulty classes provide
ready-to-use opponents for players of all skill levels.