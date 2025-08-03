# AI System API

## Overview

The AI system provides computer-controlled players with configurable strategies for the Liar's Dice game. The system
uses inheritance from the base Player class with strategy configuration and Boost libraries for random decision making.

## Key Components

### AI Player Class

**Location**: `include/liarsdice/ai/ai_player.hpp`

The `AIPlayer` class extends the core `Player` class to provide automated decision-making.

{% raw %}
```cpp
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
}
```

{% endraw %}

### Predefined AI Personalities

The system includes predefined AI personalities with different difficulty levels:

{% raw %}
```cpp
namespace liarsdice::ai {
    // Easy AI - Conservative play style
    class EasyAI : public AIPlayer {
    public:
        explicit EasyAI(unsigned int id);
    };
    
    // Medium AI - Balanced play style
    class MediumAI : public AIPlayer {
    public:
        explicit MediumAI(unsigned int id);
    };
    
    // Hard AI - Aggressive and sophisticated play
    class HardAI : public AIPlayer {
    public:
        explicit HardAI(unsigned int id);
    };
}
```

{% endraw %}

## AI Strategy Configuration

### Strategy Parameters

The `Strategy` struct allows fine-tuning of AI behavior:

- **risk_tolerance** (0.0-1.0): Controls how aggressive the AI is
    - 0.0: Very conservative, rarely bluffs
    - 0.5: Balanced approach
    - 1.0: Very aggressive, frequently bluffs

- **bluff_frequency** (0.0–1.0): How often the AI attempts to bluff
    - 0.0: Never bluffs
    - 0.2: Occasional bluffs (default)
    - 0.5: Frequent bluffing

- **call_threshold** (0.0–1.0): Probability threshold for calling liar
    - Lower values: Calls liar more often
    - Higher values: More cautious about calling

- **think_time_ms**: Simulated thinking delay in milliseconds
    - Makes AI feel more human-like
    - Can be set to 0 for instant responses

### Predefined Strategies

```cpp
// Easy AI configuration
EasyAI::EasyAI(unsigned int id) 
    : AIPlayer(id, "Easy AI", {
        .risk_tolerance = 0.2,
        .bluff_frequency = 0.1,
        .call_threshold = 0.8,
        .think_time_ms = 500
    }) {}

// Medium AI configuration
MediumAI::MediumAI(unsigned int id)
    : AIPlayer(id, "Medium AI", {
        .risk_tolerance = 0.5,
        .bluff_frequency = 0.2,
        .call_threshold = 0.7,
        .think_time_ms = 1000
    }) {}

// Hard AI configuration
HardAI::HardAI(unsigned int id)
    : AIPlayer(id, "Hard AI", {
        .risk_tolerance = 0.8,
        .bluff_frequency = 0.3,
        .call_threshold = 0.6,
        .think_time_ms = 1500
    }) {}
```

## Decision Making Algorithm

### Making Guesses

The AI uses a two-phase approach:

1. **Probability Calculation**: Estimates the likelihood of dice combinations
2. **Strategy Application**: Decides between safe play and bluffing based on strategy

```cpp
core::Guess AIPlayer::make_guess(const std::optional<core::Guess>& last_guess) {
    simulate_thinking();
    
    // Decide whether to bluff based on strategy
    double random = probability_dist_(rng_);
    if (random < strategy_.bluff_frequency) {
        return generate_bluff_guess(last_guess);
    } else {
        return generate_safe_guess(last_guess);
    }
}
```

### Calling Liar

The AI calculates the probability of the current guess being true:

```cpp
bool AIPlayer::decide_call_liar(const core::Guess& last_guess) {
    simulate_thinking();
    
    // Calculate probability of the guess being true
    double probability = calculate_probability(last_guess, total_dice_in_game);
    
    // Call liar if probability is below threshold
    return probability < (1.0 - strategy_.call_threshold);
}
```

## Usage Examples

### Creating AI Players

```cpp
#include <liarsdice/ai/ai_player.hpp>

// Create predefined AI players
auto easy_ai = std::make_shared<liarsdice::ai::EasyAI>(2);
auto medium_ai = std::make_shared<liarsdice::ai::MediumAI>(3);
auto hard_ai = std::make_shared<liarsdice::ai::HardAI>(4);

// Create custom AI with specific strategy
liarsdice::ai::AIPlayer::Strategy custom_strategy{
    .risk_tolerance = 0.6,
    .bluff_frequency = 0.25,
    .call_threshold = 0.65,
    .think_time_ms = 800
};
auto custom_ai = std::make_shared<liarsdice::ai::AIPlayer>(5, "Custom AI", custom_strategy);
```

### Integrating with Game

```cpp
#include <liarsdice/core/game.hpp>
#include <liarsdice/ai/ai_player.hpp>

// Create game
liarsdice::core::Game game;

// Add human player
auto human = std::make_shared<liarsdice::core::Player>(1, "Human");
game.add_player(human);

// Add various AI opponents
game.add_player(std::make_shared<liarsdice::ai::EasyAI>(2));
game.add_player(std::make_shared<liarsdice::ai::MediumAI>(3));
game.add_player(std::make_shared<liarsdice::ai::HardAI>(4));

// Start game - AI players will automatically make decisions
game.start_game();
```

### AI Decision Flow

During gameplay, AI players automatically:

1. Make guesses when it's their turn
2. Decide whether to call liar on opponents' guesses
3. Simulate thinking time for realism
4. Adapt strategy based on game state

## Boost Dependencies

The AI system uses:

- **Boost.Random**: For probabilistic decision-making
    - `boost::random::mt19937`: Mersenne Twister RNG
    - `boost::random::uniform_real_distribution`: For probability checks
    - `boost::random::uniform_int_distribution`: For dice face selection

## Performance Considerations

- AI calculations are lightweight and suitable for real-time gameplay
- Thinking time simulation uses `std::this_thread::sleep_for`
- Random number generators are mutable for const-correctness
- No heavy computation or machine learning overhead

## Future Enhancements

Potential improvements could include:

- Learning from game history
- Pattern recognition for opponent behavior
- Tournament-style AI competitions
- Configurable personality traits (cautious, aggressive, unpredictable)

## See Also

- [Core Game API](core.md) — For base Player class and game mechanics
- [Application](../architecture/application.md) — For game loop integration