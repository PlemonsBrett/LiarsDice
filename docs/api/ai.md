# AI System API

## Overview

The AI system provides a flexible framework for implementing computer-controlled players with varying difficulty levels
and strategies. The system uses the Strategy pattern with a factory for runtime registration and creation of AI players.

## Key Components

### AI Strategy Interface

**Location**: `include/liarsdice/ai/i_ai_strategy.hpp`

The base interface for all AI strategies.

```cpp
namespace liarsdice::ai {
    class IAIStrategy {
    public:
        virtual ~IAIStrategy() = default;
        
        [[nodiscard]] virtual AIDecision make_decision(
            const AIDecisionContext& context) = 0;
        
        [[nodiscard]] virtual std::string_view get_name() const noexcept = 0;
        [[nodiscard]] virtual std::unique_ptr<IAIStrategy> clone() const = 0;
    };
}
```

### AI Decision Types

**Location**: `include/liarsdice/ai/ai_types.hpp`

```cpp
namespace liarsdice::ai {
    // AI actions
    struct AIGuessAction {
        uint32_t quantity;
        uint8_t face_value;
    };
    
    struct AICallLiarAction {};
    
    // Type-safe decision variant
    using AIDecision = std::variant<AIGuessAction, AICallLiarAction>;
    
    // Decision context
    struct AIDecisionContext {
        // Current game state
        std::optional<Bid> current_bid;
        size_t total_dice_count;
        std::vector<Bid> bid_history;
        
        // AI player information
        PlayerID ai_player_id;
        std::vector<Dice> ai_dice;
        size_t ai_dice_count;
        
        // Opponent information
        std::vector<OpponentInfo> opponents;
        
        // Additional context
        size_t round_number;
        std::chrono::milliseconds time_limit;
    };
    
    struct OpponentInfo {
        PlayerID player_id;
        std::string name;
        size_t dice_count;
        std::vector<Bid> previous_bids;
        bool is_active;
    };
}
```

### AI Strategy Factory

**Location**: `include/liarsdice/ai/ai_strategy_factory.hpp`

Singleton factory for registering and creating AI strategies.

```cpp
namespace liarsdice::ai {
    class AIStrategyFactory {
    public:
        static AIStrategyFactory& instance();
        
        // Registration
        template<typename TStrategy>
            requires std::derived_from<TStrategy, IAIStrategy>
        void register_strategy(std::string_view name, 
                             std::string_view description);
        
        // Creation
        [[nodiscard]] std::unique_ptr<IAIStrategy> create(
            std::string_view name) const;
        
        // Query available strategies
        [[nodiscard]] std::vector<std::string> get_strategy_names() const;
        [[nodiscard]] std::optional<std::string> get_description(
            std::string_view name) const;
        
        // Check if strategy exists
        [[nodiscard]] bool has_strategy(std::string_view name) const;
        
    private:
        AIStrategyFactory() = default;
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
}
```

### Easy AI Strategy

**Location**: `include/liarsdice/ai/easy_ai_strategy.hpp`

Simple AI implementation with configurable parameters.

```cpp
namespace liarsdice::ai {
    struct EasyAIConfig {
        double risk_tolerance = 0.3;      // 0.0 (conservative) to 1.0 (aggressive)
        double bluff_frequency = 0.2;     // Probability of bluffing
        double call_threshold = 0.7;      // Probability threshold for calling liar
        bool use_statistical_analysis = true;
        std::chrono::milliseconds decision_delay{500};
    };
    
    class EasyAIStrategy : public IAIStrategy {
    public:
        explicit EasyAIStrategy(EasyAIConfig config = {});
        
        [[nodiscard]] AIDecision make_decision(
            const AIDecisionContext& context) override;
        
        [[nodiscard]] std::string_view get_name() const noexcept override;
        [[nodiscard]] std::unique_ptr<IAIStrategy> clone() const override;
        
        // Configuration
        void set_config(const EasyAIConfig& config);
        [[nodiscard]] const EasyAIConfig& get_config() const noexcept;
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
}
```

### Medium AI Strategy

**Location**: `include/liarsdice/ai/medium_ai_strategy.hpp`

Advanced AI with statistical analysis and opponent modeling.

```cpp
namespace liarsdice::ai {
    struct MediumAIConfig {
        // Basic parameters
        double risk_tolerance = 0.5;
        double bluff_frequency = 0.3;
        double call_threshold = 0.6;
        
        // Advanced features
        bool use_opponent_modeling = true;
        bool use_pattern_recognition = true;
        bool use_bayesian_inference = true;
        
        // Pattern detection
        size_t pattern_history_size = 10;
        double pattern_weight = 0.4;
        
        // Performance tuning
        std::chrono::milliseconds decision_delay{1000};
        size_t max_simulations = 1000;
    };
    
    class MediumAIStrategy : public IAIStrategy {
    public:
        explicit MediumAIStrategy(MediumAIConfig config = {});
        
        [[nodiscard]] AIDecision make_decision(
            const AIDecisionContext& context) override;
        
        [[nodiscard]] std::string_view get_name() const noexcept override;
        [[nodiscard]] std::unique_ptr<IAIStrategy> clone() const override;
        
        // Configuration
        void set_config(const MediumAIConfig& config);
        [[nodiscard]] const MediumAIConfig& get_config() const noexcept;
        
        // Analysis methods (for testing/debugging)
        [[nodiscard]] double calculate_bid_probability(
            const Bid& bid, const AIDecisionContext& context) const;
        
        [[nodiscard]] std::vector<OpponentModel> get_opponent_models() const;
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
}
```

### AI Player

**Location**: `include/liarsdice/core/ai_player.hpp`

Player implementation that uses AI strategies for decision making.

```cpp
namespace liarsdice::core {
    class AIPlayer : public Player {
    public:
        AIPlayer(PlayerID id, std::unique_ptr<ai::IAIStrategy> strategy,
                std::string name = "");
        
        // AI-specific methods
        [[nodiscard]] ai::AIDecision make_decision(
            const ai::AIDecisionContext& context) const;
        
        [[nodiscard]] const ai::IAIStrategy& get_strategy() const noexcept;
        void set_strategy(std::unique_ptr<ai::IAIStrategy> strategy);
        
        // AI player identification
        [[nodiscard]] bool is_ai() const noexcept override { return true; }
        
    private:
        std::unique_ptr<ai::IAIStrategy> strategy_;
    };
}
```

## Usage Examples

### Registering AI Strategies

```cpp
#include "liarsdice/ai/ai_strategy_factory.hpp"
#include "liarsdice/ai/easy_ai_strategy.hpp"
#include "liarsdice/ai/medium_ai_strategy.hpp"

// Register strategies at startup
auto& factory = liarsdice::ai::AIStrategyFactory::instance();

factory.register_strategy<liarsdice::ai::EasyAIStrategy>(
    "easy", "Easy difficulty AI with simple heuristics");

factory.register_strategy<liarsdice::ai::MediumAIStrategy>(
    "medium", "Medium difficulty AI with statistical analysis");
```

### Creating AI Players

```cpp
#include "liarsdice/core/ai_player.hpp"

// Create AI player with factory
auto ai_strategy = factory.create("easy");
auto ai_player = std::make_unique<liarsdice::core::AIPlayer>(
    1, std::move(ai_strategy), "EasyBot");

// Create AI player with custom configuration
liarsdice::ai::EasyAIConfig config;
config.risk_tolerance = 0.5;
config.bluff_frequency = 0.1;

auto custom_ai = std::make_unique<liarsdice::ai::EasyAIStrategy>(config);
auto custom_player = std::make_unique<liarsdice::core::AIPlayer>(
    2, std::move(custom_ai), "CustomBot");
```

### Using AI in Game

```cpp
// In game loop
liarsdice::ai::AIDecisionContext context{
    .current_bid = game.GetCurrentBid(),
    .total_dice_count = game.GetTotalDiceCount(),
    .bid_history = game.GetBidHistory(),
    .ai_player_id = ai_player->GetId(),
    .ai_dice = std::vector<liarsdice::core::Dice>(
        ai_player->GetDice().begin(), 
        ai_player->GetDice().end()),
    .ai_dice_count = ai_player->GetDiceCount(),
    .opponents = gather_opponent_info(game),
    .round_number = current_round,
    .time_limit = std::chrono::seconds(30)
};

auto decision = ai_player->make_decision(context);

// Process decision
std::visit(overloaded{
    [&](const liarsdice::ai::AIGuessAction& guess) {
        liarsdice::Bid bid{
            .quantity = guess.quantity,
            .face_value = guess.face_value,
            .player_id = ai_player->GetId()
        };
        game.MakeBid(bid);
    },
    [&](const liarsdice::ai::AICallLiarAction&) {
        game.CallLiar();
    }
}, decision);
```

### Implementing Custom AI Strategy

```cpp
class HardAIStrategy : public liarsdice::ai::IAIStrategy {
public:
    [[nodiscard]] liarsdice::ai::AIDecision make_decision(
        const liarsdice::ai::AIDecisionContext& context) override {
        
        // Implement sophisticated decision logic
        if (should_call_liar(context)) {
            return liarsdice::ai::AICallLiarAction{};
        }
        
        auto [quantity, face_value] = calculate_optimal_bid(context);
        return liarsdice::ai::AIGuessAction{quantity, face_value};
    }
    
    [[nodiscard]] std::string_view get_name() const noexcept override {
        return "hard";
    }
    
    [[nodiscard]] std::unique_ptr<IAIStrategy> clone() const override {
        return std::make_unique<HardAIStrategy>(*this);
    }
    
private:
    bool should_call_liar(const liarsdice::ai::AIDecisionContext& context);
    std::pair<uint32_t, uint8_t> calculate_optimal_bid(
        const liarsdice::ai::AIDecisionContext& context);
};
```

## AI Strategy Guidelines

### Decision Making

1. **Risk Assessment**: Evaluate the probability of success for each action
2. **Opponent Modeling**: Track opponent behavior patterns
3. **Bluff Detection**: Analyze bid patterns for inconsistencies
4. **Strategic Timing**: Consider when to call liar vs continue bidding

### Performance Considerations

- AI decisions should complete within the time limit
- Use caching for expensive calculations
- Consider using async computation for complex strategies
- Profile AI performance in release builds

### Testing AI Strategies

```cpp
// Unit test example
TEST_CASE("AI makes valid decisions") {
    liarsdice::ai::EasyAIStrategy ai;
    liarsdice::ai::AIDecisionContext context{
        .current_bid = liarsdice::Bid{2, 4, 1},
        .total_dice_count = 10,
        .ai_dice = {/* dice values */},
        // ... other context
    };
    
    auto decision = ai.make_decision(context);
    
    std::visit(overloaded{
        [&](const liarsdice::ai::AIGuessAction& guess) {
            // Verify bid is valid and higher than current
            REQUIRE(guess.quantity > context.current_bid->quantity || 
                   (guess.quantity == context.current_bid->quantity && 
                    guess.face_value > context.current_bid->face_value));
        },
        [&](const liarsdice::ai::AICallLiarAction&) {
            // Verify call liar is reasonable
            REQUIRE(context.current_bid.has_value());
        }
    }, decision);
}
```

## Thread Safety

- AI strategies should be stateless or use thread-safe internal state
- The factory is thread-safe for strategy creation
- AI players are not thread-safe by default

## See Also

- [Core Game API](core.md) - For game mechanics and player interfaces
- [Technical Design: AI Strategy System](../technical/commit-6-ai-strategy.md) - Architecture details
- [Configuration System API](configuration.md) - For AI configuration management