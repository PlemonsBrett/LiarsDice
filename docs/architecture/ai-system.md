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

### Strategy Pattern

The AI system's foundation is the Strategy pattern, which encapsulates different AI algorithms behind a common
interface.

```cpp
class IAIStrategy {
public:
    virtual AIDecision make_decision(const AIDecisionContext& context) = 0;
    virtual std::string_view get_name() const noexcept = 0;
    virtual std::unique_ptr<IAIStrategy> clone() const = 0;
};
```

**Benefits**:

- Algorithms can be selected at runtime
- New strategies can be added without modifying existing code
- Each strategy can be tested independently
- Strategies can be swapped during gameplay

### Factory Pattern with Type Erasure

The `AIStrategyFactory` uses type erasure to store heterogeneous strategy types while maintaining type safety.

```cpp
class AIStrategyFactory {
    struct IStrategyHolder {
        virtual ~IStrategyHolder() = default;
        virtual std::unique_ptr<IAIStrategy> create() const = 0;
        virtual std::string_view get_description() const noexcept = 0;
    };
    
    template<typename TStrategy>
    struct StrategyHolder : IStrategyHolder {
        std::string description;
        
        std::unique_ptr<IAIStrategy> create() const override {
            return std::make_unique<TStrategy>();
        }
    };
    
    std::unordered_map<std::string, std::unique_ptr<IStrategyHolder>> strategies_;
};
```

**Benefits**:

- Runtime registration of strategies
- No compile-time dependencies between factory and concrete strategies
- Type-safe creation without switch statements
- Supports plugin-style architecture

### Variant Pattern for Type-Safe Decisions

AI decisions use `std::variant` to ensure type safety and enable pattern matching.

```cpp
using AIDecision = std::variant<AIGuessAction, AICallLiarAction>;

// Usage with std::visit
std::visit(overloaded{
    [&](const AIGuessAction& guess) { /* handle guess */ },
    [&](const AICallLiarAction&) { /* handle call */ }
}, decision);
```

**Benefits**:

- Compile-time exhaustiveness checking
- No runtime type identification needed
- Clear API contract for decision types
- Efficient implementation (no virtual dispatch)

## Component Architecture

### Decision Context

The `AIDecisionContext` provides all information needed for decision-making:

```
AIDecisionContext
├── Game State
│   ├── current_bid
│   ├── total_dice_count
│   └── bid_history
├── AI Player State
│   ├── ai_player_id
│   ├── ai_dice
│   └── ai_dice_count
├── Opponent Information
│   └── opponents[]
│       ├── player_id
│       ├── dice_count
│       └── previous_bids
└── Meta Information
    ├── round_number
    └── time_limit
```

### Strategy Hierarchy

```
IAIStrategy (interface)
├── EasyAIStrategy
│   ├── Simple heuristics
│   ├── Basic probability
│   └── Configurable risk
├── MediumAIStrategy
│   ├── Statistical analysis
│   ├── Opponent modeling
│   ├── Pattern recognition
│   └── Bayesian inference
└── (Future) HardAIStrategy
    ├── Monte Carlo simulation
    ├── Game tree search
    └── Machine learning
```

## Implementation Details

### Easy AI Strategy

The Easy AI uses simple heuristics and basic probability calculations:

1. **Bid Generation**:
    - Count own dice matching potential bid
    - Estimate opponent dice using uniform distribution
    - Apply risk tolerance to adjust estimates
    - Consider bluff frequency for occasional aggressive bids

2. **Call Decision**:
    - Calculate probability of current bid being true
    - Compare against call threshold
    - Factor in game state (remaining dice, players)

### Medium AI Strategy

The Medium AI employs advanced statistical techniques:

1. **Opponent Modeling**:
   ```cpp
   struct OpponentModel {
       PlayerID player_id;
       double aggression_level;      // 0.0 to 1.0
       double bluff_frequency;       // Observed bluff rate
       std::vector<BidPattern> patterns;  // Detected patterns
       BayesianEstimator estimator;  // Probability updates
   };
   ```

2. **Pattern Recognition**:
    - Tracks bid sequences for each opponent
    - Identifies common patterns (e.g., always increasing by 1)
    - Weights patterns by recency and frequency

3. **Bayesian Inference**:
    - Updates beliefs about opponent dice based on bids
    - Incorporates prior knowledge from opponent models
    - Adjusts probabilities based on game events

### Decision Flow

```
make_decision()
├── Analyze Context
│   ├── Parse current game state
│   ├── Update opponent models
│   └── Calculate time constraints
├── Generate Candidates
│   ├── Possible bids
│   ├── Call liar option
│   └── Apply strategy filters
├── Evaluate Options
│   ├── Calculate probabilities
│   ├── Apply risk tolerance
│   └── Consider bluff value
├── Select Best Action
│   ├── Rank by expected value
│   ├── Add randomization
│   └── Apply time delay
└── Return Decision
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

3. **Incremental Updates**:
    - Update opponent models incrementally
    - Reuse calculations from previous turns

## Configuration System

AI strategies support runtime configuration through structured config objects:

```cpp
struct EasyAIConfig {
    double risk_tolerance = 0.3;
    double bluff_frequency = 0.2;
    double call_threshold = 0.7;
    bool use_statistical_analysis = true;
    std::chrono::milliseconds decision_delay{500};
};
```

Configuration can be loaded from:

- JSON files
- Environment variables
- Runtime API calls
- Game difficulty settings

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
    - Rapid decision making
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

### For Strategy Implementers

1. **Stateless Design**: Strategies should not store game state
2. **Deterministic Testing**: Support seed-based randomization
3. **Time Awareness**: Respect time limits in context
4. **Clear Naming**: Use descriptive names for strategies
5. **Configuration**: Expose tunable parameters

### For System Integrators

1. **Register Early**: Register all strategies at startup
2. **Error Handling**: Handle unknown strategy names gracefully
3. **Performance Monitoring**: Track AI decision times
4. **Fair Play**: Ensure AI doesn't have unfair advantages
5. **User Experience**: Add appropriate delays for realism

## Conclusion

The AI system architecture provides a solid foundation for implementing intelligent computer opponents. The combination
of design patterns creates a system that is both powerful and maintainable, supporting everything from simple rule-based
AI to sophisticated statistical analysis. The architecture's extensibility ensures that new AI strategies can be added
as the game evolves, while maintaining backward compatibility and performance standards.