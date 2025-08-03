# AI Strategy Testing Guide

This guide explains how to test the AI strategies once they are integrated into the game's CLI.

## Current Status

The LiarsDice game has implemented multiple AI strategies:

- **EasyAIStrategy**: Simple rule-based AI with basic probability calculations
- **MediumAIStrategy**: Statistical AI with opponent modeling and pattern recognition

However, these strategies are not yet integrated into the game's CLI. The current game creates all players as human players.

## AI Strategy Test Suite

The `ai_strategies.robot` file contains comprehensive tests for AI behaviors, including:

### 1. Strategy Registration Tests

- Verify AI strategies are properly registered with the factory
- Test strategy selection during game setup

### 2. Easy AI Tests

- Conservative behavior verification
- Basic probability calculations
- Predictable patterns

### 3. Medium AI Tests

- Statistical analysis verification
- Opponent modeling adaptation
- Bluff detection capabilities
- Pattern recognition testing

### 4. Performance Tests

- Decision time comparison between strategies
- Memory usage monitoring
- Scalability with multiple AI players

### 5. Integration Tests

- Multiple AI strategies in the same game
- AI vs AI gameplay
- Human vs different AI difficulties

## Running AI Tests (Future)

Once AI strategies are integrated into the CLI, run the tests with:

```bash
# Run all AI strategy tests
./tests/robot/run_tests.sh --suite ai_strategies

# Run specific AI tests
./tests/robot/run_tests.sh --tag medium-ai
./tests/robot/run_tests.sh --tag easy-ai
./tests/robot/run_tests.sh --tag ai-performance
```

## Integration Requirements

To enable these tests, the game needs to:

1. **Add AI player type selection** during game setup:

   ```
   How many players? 3
   How many AI players? 2
   Select AI difficulty for Player 2 (easy/medium/hard): medium
   Select AI difficulty for Player 3 (easy/medium/hard): easy
   ```

2. **Create AI players** using the strategy factory:

   ```cpp
   auto& factory = AIStrategyFactory::instance();
   auto strategy = factory.create(difficulty_name);
   players_.emplace_back(std::make_unique<AIPlayer>(i, std::move(strategy)));
   ```

3. **Implement AIPlayer class** that uses strategies for decisions:

   ```cpp
   class AIPlayer : public Player {
     std::unique_ptr<IAIStrategy> strategy_;
     // Use strategy_->make_decision() for game actions
   };
   ```

## Test Implementation Details

### Behavior Verification

The tests verify AI behaviors by:

1. Analyzing output patterns
2. Tracking decision frequencies
3. Measuring response times
4. Detecting adaptation patterns

### Pattern Detection

Medium AI pattern tests check for:

- Bluff detection after repeated patterns
- Increased call rates against aggressive players
- Statistical decision-making based on probability

### Performance Metrics

Tests measure:

- Average decision time per strategy
- Memory usage during games
- Scalability with multiple AI players

## Custom Keywords

The test suite includes keywords for:

- `Start Game With AI Strategy`: Initialize game with specific AI
- `Wait For AI Turn`: Synchronize with AI decisions
- `Get Last AI Guess`: Extract AI actions from output
- `Analyze AI Behavior`: Pattern analysis utilities
- `Measure AI Decision Time`: Performance measurements

## Future Enhancements

1. **Hard AI Strategy Tests**: Once implemented
2. **Machine Learning AI Tests**: For adaptive strategies
3. **Tournament Mode Tests**: AI vs AI competitions
4. **Strategy Configuration Tests**: Custom AI parameters

## Debugging AI Tests

When tests fail:

1. Check CLI output for AI decision logs
2. Verify strategy registration in factory
3. Ensure proper AI player instantiation
4. Review decision timing constraints

## Contributing

When adding new AI strategies:

1. Add corresponding test cases in `ai_strategies.robot`
2. Update this guide with new test descriptions
3. Ensure performance benchmarks are included
4. Add integration tests with existing strategies
