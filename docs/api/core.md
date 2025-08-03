# Core Game API

## Overview

The core game API provides the fundamental classes for implementing the Liar's Dice game logic using Boost libraries.
This includes dice management, player representation, game state control with signal/slot event handling, and turn
management.

## Key Components

### Dice Class

**Location**: `include/liarsdice/core/dice.hpp`

The `Dice` class represents a single die in the game using Boost's random number generation.

```cpp
namespace liarsdice::core {
    class Dice {
    public:
        Dice();
        
        void roll();
        [[nodiscard]] unsigned int get_value() const { return value_; }
        void set_value(unsigned int value) { value_ = value; }
        
    private:
        unsigned int value_ = 0;
        static boost::random::mt19937& get_rng();
    };
}
```

**Key Methods**:

- `roll()`: Generates a random value between 1 and 6 using Boost.Random
- `get_value()`: Returns the current face value
- `set_value()`: Manually sets the face value (for testing)

### Player Class

**Location**: `include/liarsdice/core/player.hpp`

The `Player` class represents a player in the game.

```cpp
namespace liarsdice::core {
    struct Guess {
        unsigned int face = 0;
        unsigned int count = 0;
        
        bool operator<(const Guess& other) const;
        [[nodiscard]] std::string to_string() const;
    };
    
    class Player : public std::enable_shared_from_this<Player> {
    public:
        Player(unsigned int id, const std::string& name);
        virtual ~Player() = default;
        
        // Core functionality
        void roll_dice();
        void lose_die();
        void reset();
        
        // Virtual methods for AI override
        virtual Guess make_guess(const std::optional<Guess>& last_guess);
        virtual bool decide_call_liar(const Guess& last_guess);
        
        // Getters
        [[nodiscard]] unsigned int get_id() const { return id_; }
        [[nodiscard]] const std::string& get_name() const { return name_; }
        [[nodiscard]] bool is_human() const { return is_human_; }
        [[nodiscard]] size_t get_dice_count() const { return dice_.size(); }
        [[nodiscard]] const std::vector<Dice>& get_dice() const { return dice_; }
        [[nodiscard]] bool is_eliminated() const { return dice_.empty(); }
        
    protected:
        unsigned int id_;
        std::string name_;
        std::vector<Dice> dice_;
        bool is_human_ = true;
    };
}
```

### Game Class

**Location**: `include/liarsdice/core/game.hpp`

The `Game` class manages the overall game state and flow using Boost.Signals2 for event handling.

```cpp
namespace liarsdice::core {
    // Game events for signal/slot system
    struct GameEvents {
        boost::signals2::signal<void(unsigned int round)> on_round_start;
        boost::signals2::signal<void(unsigned int round)> on_round_end;
        boost::signals2::signal<void(const Player&)> on_player_turn;
        boost::signals2::signal<void(const Player&, const Guess&)> on_guess_made;
        boost::signals2::signal<void(const Player&)> on_liar_called;
        boost::signals2::signal<void(const Player&, const Player&)> on_round_result;
        boost::signals2::signal<void(const Player&)> on_player_eliminated;
        boost::signals2::signal<void(const Player&)> on_game_winner;
    };
    
    // Game configuration
    struct GameConfig {
        unsigned int min_players = 2;
        unsigned int max_players = 8;
        unsigned int starting_dice = 5;
        bool allow_ones_wild = true;
        unsigned int ai_think_time_ms = 1000;
    };
    
    class Game {
    public:
        enum class State {
            NOT_STARTED,
            WAITING_FOR_PLAYERS,
            IN_PROGRESS,
            ROUND_ENDED,
            GAME_OVER
        };
        
        explicit Game(const GameConfig& config = {});
        
        // Player management
        void add_player(std::shared_ptr<Player> player);
        void remove_player(unsigned int player_id);
        [[nodiscard]] size_t get_player_count() const;
        [[nodiscard]] const std::vector<std::shared_ptr<Player>>& get_players() const;
        [[nodiscard]] std::shared_ptr<Player> get_player(unsigned int id) const;
        
        // Game flow
        void start_game();
        void start_round();
        void make_guess(const Guess& guess);
        void call_liar();
        
        // Game state
        [[nodiscard]] State get_state() const { return state_; }
        [[nodiscard]] const std::optional<Guess>& get_current_guess() const;
        [[nodiscard]] std::shared_ptr<Player> get_current_player() const;
        [[nodiscard]] unsigned int get_total_dice() const;
        [[nodiscard]] unsigned int get_round_number() const { return round_number_; }
        
        // Event access
        GameEvents& events() { return events_; }
        
    private:
        void next_turn();
        void end_round(std::shared_ptr<Player> loser);
        void check_game_over();
        [[nodiscard]] unsigned int count_dice_with_face(unsigned int face) const;
        
    private:
        GameConfig config_;
        State state_ = State::NOT_STARTED;
        std::vector<std::shared_ptr<Player>> players_;
        std::vector<std::shared_ptr<Player>> active_players_;
        size_t current_player_index_ = 0;
        std::optional<Guess> current_guess_;
        unsigned int round_number_ = 0;
        GameEvents events_;
    };
}
```

## Usage Examples

### Creating a Game

```cpp
#include <liarsdice/core/game.hpp>
#include <liarsdice/core/player.hpp>
#include <liarsdice/ai/ai_player.hpp>

// Create a new game with custom config
liarsdice::core::GameConfig config;
config.starting_dice = 5;
config.allow_ones_wild = true;

liarsdice::core::Game game(config);

// Add human and AI players
auto human = std::make_shared<liarsdice::core::Player>(1, "Alice");
auto ai = std::make_shared<liarsdice::ai::EasyAI>(2);

game.add_player(human);
game.add_player(ai);

// Connect to game events
game.events().on_round_start.connect([](unsigned int round) {
    std::cout << "Round " << round << " started!\n";
});

game.events().on_guess_made.connect([](const auto& player, const auto& guess) {
    std::cout << player.get_name() << " guessed " << guess.to_string() << "\n";
});

// Start the game
game.start_game();
```

### Making Guesses

```cpp
// Get current player's guess
auto current_player = game.get_current_player();
auto last_guess = game.get_current_guess();

// Human player makes a guess
if (current_player->is_human()) {
    liarsdice::core::Guess guess;
    guess.count = 3;
    guess.face = 5;
    game.make_guess(guess);
} else {
    // AI player automatically makes decision
    auto ai_guess = current_player->make_guess(last_guess);
    game.make_guess(ai_guess);
}
```

### Calling Liar

```cpp
// Current player can call liar on the previous guess
if (game.get_current_guess().has_value()) {
    game.call_liar();
}

// Handle round result via signals
game.events().on_round_result.connect([](const auto& winner, const auto& loser) {
    std::cout << winner.get_name() << " won the round!\n";
    std::cout << loser.get_name() << " loses a die.\n";
});
```

## Boost Dependencies

This implementation uses several Boost libraries:

- **Boost.Signals2**: Event-driven architecture with signal/slot connections
- **Boost.Random**: Random number generation for dice rolls
- **Boost.Log**: Logging support (via boost::log::trivial)

## Thread Safety

- The `Dice` class uses thread-safe random number generation
- The `Game` class is not thread-safe; external synchronization required for multi-threaded access
- Boost.Signals2 provides thread-safe signal handling when configured appropriately

## See Also

- [AI System API](ai.md) - For AI player implementations
- [Logging System API](logging.md) - For game event logging
- [UI System](../architecture/ui-system.md) - For the menu system and user interface