# Core Game API

## Overview

The core game API provides the fundamental classes and interfaces for implementing the Liar's Dice game logic. This includes dice management, player representation, game state control, and turn management.

## Key Components

### Dice Class

**Location**: `include/liarsdice/core/dice.hpp`

The `Dice` class represents a single die in the game.

```cpp
namespace liarsdice::core {
    class Dice {
    public:
        using value_type = uint8_t;
        
        Dice();
        void Roll();
        [[nodiscard]] value_type GetFaceValue() const noexcept;
        void SetFaceValue(value_type value);
        [[nodiscard]] bool IsRolled() const noexcept;
        
    private:
        value_type face_value_{0};
        bool is_rolled_{false};
        static std::mt19937& GetRNG();
    };
}
```

**Key Methods**:
- `Roll()`: Generates a random value between 1 and 6
- `GetFaceValue()`: Returns the current face value (0 if not rolled)
- `SetFaceValue()`: Manually sets the face value (for testing)
- `IsRolled()`: Checks if the die has been rolled

### Player Interface

**Location**: `include/liarsdice/interfaces/i_player.hpp`

The `IPlayer` interface defines the contract for all player types.

```cpp
namespace liarsdice::interfaces {
    class IPlayer {
    public:
        virtual ~IPlayer() = default;
        
        [[nodiscard]] virtual PlayerID GetId() const noexcept = 0;
        [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
        [[nodiscard]] virtual bool IsActive() const noexcept = 0;
        [[nodiscard]] virtual size_t GetDiceCount() const noexcept = 0;
        [[nodiscard]] virtual std::span<const core::Dice> GetDice() const = 0;
        
        virtual void SetName(std::string name) = 0;
        virtual void RollDice() = 0;
        virtual void RemoveDie() = 0;
        virtual void Reset(size_t dice_count = 5) = 0;
    };
}
```

### Player Implementation

**Location**: `include/liarsdice/core/player.hpp`

The concrete `Player` class implements the `IPlayer` interface.

```cpp
namespace liarsdice::core {
    class Player : public interfaces::IPlayer {
    public:
        explicit Player(PlayerID id, std::string name = "");
        
        // IPlayer interface implementation
        [[nodiscard]] PlayerID GetId() const noexcept override;
        [[nodiscard]] std::string_view GetName() const noexcept override;
        [[nodiscard]] bool IsActive() const noexcept override;
        [[nodiscard]] size_t GetDiceCount() const noexcept override;
        [[nodiscard]] std::span<const Dice> GetDice() const override;
        
        void SetName(std::string name) override;
        void RollDice() override;
        void RemoveDie() override;
        void Reset(size_t dice_count = 5) override;
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
}
```

### Game Interface

**Location**: `include/liarsdice/interfaces/i_game.hpp`

The `IGame` interface defines the contract for game management.

```cpp
namespace liarsdice::interfaces {
    class IGame {
    public:
        virtual ~IGame() = default;
        
        // Game state
        [[nodiscard]] virtual GameState GetState() const noexcept = 0;
        [[nodiscard]] virtual size_t GetPlayerCount() const noexcept = 0;
        [[nodiscard]] virtual PlayerID GetCurrentPlayerId() const noexcept = 0;
        [[nodiscard]] virtual std::optional<PlayerID> GetWinnerId() const noexcept = 0;
        
        // Player management
        virtual void AddPlayer(std::unique_ptr<IPlayer> player) = 0;
        [[nodiscard]] virtual const IPlayer* GetPlayer(PlayerID id) const = 0;
        [[nodiscard]] virtual std::vector<const IPlayer*> GetPlayers() const = 0;
        
        // Game flow
        virtual void StartNewGame() = 0;
        virtual void StartRound() = 0;
        virtual BidResult MakeBid(const Bid& bid) = 0;
        virtual CallLiarResult CallLiar() = 0;
        
        // Game information
        [[nodiscard]] virtual std::optional<Bid> GetCurrentBid() const noexcept = 0;
        [[nodiscard]] virtual std::vector<Bid> GetBidHistory() const = 0;
        [[nodiscard]] virtual size_t GetTotalDiceCount() const noexcept = 0;
    };
}
```

### Game Implementation

**Location**: `include/liarsdice/core/game.hpp`

The concrete `Game` class implements the `IGame` interface.

```cpp
namespace liarsdice::core {
    class Game : public interfaces::IGame {
    public:
        Game();
        ~Game();
        
        // Move semantics
        Game(Game&&) noexcept;
        Game& operator=(Game&&) noexcept;
        
        // Deleted copy semantics
        Game(const Game&) = delete;
        Game& operator=(const Game&) = delete;
        
        // IGame interface implementation
        // ... (all virtual methods from IGame)
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
}
```

## Common Types

**Location**: `include/liarsdice/common/types.hpp`

```cpp
namespace liarsdice {
    // Player identification
    using PlayerID = uint32_t;
    
    // Game state enumeration
    enum class GameState : uint8_t {
        NOT_STARTED,
        IN_PROGRESS,
        ROUND_OVER,
        GAME_OVER
    };
    
    // Bid structure
    struct Bid {
        uint32_t quantity;
        uint8_t face_value;
        PlayerID player_id;
        
        auto operator<=>(const Bid&) const = default;
    };
    
    // Result types
    enum class BidResult : uint8_t {
        SUCCESS,
        INVALID_TURN,
        INVALID_BID_TOO_LOW,
        INVALID_QUANTITY,
        INVALID_FACE_VALUE,
        GAME_NOT_IN_PROGRESS
    };
    
    enum class CallLiarResult : uint8_t {
        CALLER_WINS,
        BIDDER_WINS,
        INVALID_TURN,
        NO_CURRENT_BID,
        GAME_NOT_IN_PROGRESS
    };
}
```

## Usage Examples

### Creating a Game

```cpp
#include "liarsdice/core/game.hpp"
#include "liarsdice/core/player.hpp"

// Create a new game
liarsdice::core::Game game;

// Add players
auto player1 = std::make_unique<liarsdice::core::Player>(1, "Alice");
auto player2 = std::make_unique<liarsdice::core::Player>(2, "Bob");

game.AddPlayer(std::move(player1));
game.AddPlayer(std::move(player2));

// Start the game
game.StartNewGame();
game.StartRound();
```

### Making Bids

```cpp
// Make a bid
liarsdice::Bid bid{
    .quantity = 3,
    .face_value = 5,
    .player_id = 1
};

auto result = game.MakeBid(bid);
if (result == liarsdice::BidResult::SUCCESS) {
    // Bid was successful
}
```

### Calling Liar

```cpp
auto result = game.CallLiar();
switch (result) {
    case liarsdice::CallLiarResult::CALLER_WINS:
        // The caller was correct
        break;
    case liarsdice::CallLiarResult::BIDDER_WINS:
        // The bidder was truthful
        break;
    default:
        // Handle error
        break;
}
```

## Thread Safety

- The `Dice` class uses thread-local RNG for thread safety
- `Player` and `Game` classes are not thread-safe by default
- External synchronization is required for multi-threaded access

## Performance Considerations

- Uses PIMPL idiom for ABI stability and compilation speed
- `std::span` for efficient dice access without copying
- Move semantics supported for `Game` objects
- `noexcept` specifications for performance-critical methods

## See Also

- [AI System API](ai.md) - For AI player implementations
- [Logging System API](logging.md) - For game event logging
- [Configuration System API](configuration.md) - For game configuration options