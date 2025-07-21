======================
Architecture Overview
======================

.. contents:: Table of Contents
   :local:
   :depth: 2

System Architecture
===================

The LiarsDice Game Engine follows a modern, modular architecture designed around **dependency injection**, **interface-based design**, and **separation of concerns**. The system is built with C++23 features and emphasizes testability, maintainability, and extensibility.

High-Level Components
=====================

.. mermaid::

   graph TB
       CLI[CLI Application] --> Game[Game Controller]
       Game --> DI[Dependency Injection Container]
       DI --> Core[Core Components]
       DI --> AI[AI System]
       DI --> Persistence[Data Persistence]
       
       Core --> Player[Player Management]
       Core --> Dice[Dice System]
       Core --> Rules[Game Rules Engine]
       Core --> State[Game State Manager]
       
       AI --> Beginner[Beginner AI]
       AI --> Intermediate[Intermediate AI - Bayesian]
       AI --> Expert[Expert AI - ML/Game Theory]
       
       Persistence --> Database[(Database)]
       Persistence --> Analytics[Analytics Engine]

Architecture Principles
=======================

**1. Inversion of Control (IoC)**

The system uses dependency injection to invert control flow and reduce coupling:

.. code-block:: cpp

   // Bad: Direct dependencies
   class Game {
       Player player_;  // Hard dependency
   };

   // Good: Injected dependencies
   class GameImpl : public IGame {
       std::unique_ptr<IPlayer> player_;  // Injected via interface
   public:
       GameImpl(std::unique_ptr<IPlayer> player) : player_(std::move(player)) {}
   };

**2. Interface Segregation**

Each component exposes only the methods its clients need:

.. code-block:: cpp

   // Specific interfaces for different concerns
   class IDice { /* dice rolling behavior */ };
   class IPlayer { /* player actions */ };
   class IGameState { /* state management */ };
   class IRandomGenerator { /* RNG abstraction */ };

**3. Single Responsibility**

Each class has one reason to change:

- ``DiceImpl``: Manages individual die behavior
- ``PlayerImpl``: Handles player actions and state
- ``GameImpl``: Orchestrates game flow and rules
- ``ServiceContainer``: Manages object lifecycles

**4. Dependency Inversion**

High-level modules don't depend on low-level modules; both depend on abstractions:

.. code-block:: cpp

   // High-level Game depends on abstraction
   class GameImpl {
       std::unique_ptr<IRandomGenerator> rng_;  // Not concrete implementation
   };

   // Low-level implementation
   class StandardRandomGenerator : public IRandomGenerator {
       // Concrete implementation details
   };

Layered Architecture
====================

The system is organized into distinct layers with clear dependencies:

**Layer 1: Core Interfaces**
- Define contracts for all major components
- No implementation details
- Zero dependencies on other layers

**Layer 2: Core Implementations**
- Implement business logic and game rules
- Depend only on Layer 1 interfaces
- Injectable through DI container

**Layer 3: Application Services**
- Coordinate multiple core components
- Handle application-specific workflows
- Orchestrate complex operations

**Layer 4: Presentation Layer**
- CLI interface and user interaction
- Format output and handle input
- Minimal business logic

Component Details
=================

Dependency Injection Container
------------------------------

**ServiceContainer** manages object lifecycles and dependencies:

.. code-block:: cpp

   class ServiceContainer {
   public:
       // Register service implementations
       template<typename TInterface, typename TImpl>
       void register_service(ServiceLifetime lifetime);
       
       // Resolve dependencies
       template<typename T>
       std::expected<std::unique_ptr<T>, DIError> resolve();
   };

**Features:**
- Type-safe service registration
- Multiple lifetime management (Singleton, Transient, Scoped)
- Circular dependency detection
- Thread-safe resolution
- Factory function support

Game Core Components
--------------------

**Game Controller (IGame)**

Orchestrates overall game flow:

.. code-block:: cpp

   class IGame {
   public:
       virtual void initialize() = 0;
       virtual void add_player(int player_id) = 0;
       virtual void start_game() = 0;
       virtual bool process_guess(const Guess& guess) = 0;
       virtual std::string process_liar_call(int calling_player_id) = 0;
       virtual bool is_game_over() const = 0;
   };

**Player Management (IPlayer)**

Handles individual player state and actions:

.. code-block:: cpp

   class IPlayer {
   public:
       virtual void add_die() = 0;
       virtual bool remove_die() = 0;
       virtual void roll_dice() = 0;
       virtual std::vector<unsigned int> get_dice_values() const = 0;
       virtual size_t count_dice_with_value(unsigned int value) const = 0;
   };

**Dice System (IDice)**

Manages individual die behavior:

.. code-block:: cpp

   class IDice {
   public:
       virtual void roll() = 0;
       virtual unsigned int get_face_value() const = 0;
       virtual bool is_valid_face_value(unsigned int value) const = 0;
   };

Data Flow Architecture
======================

The system follows a clear data flow pattern:

.. mermaid::

   sequenceDiagram
       participant CLI
       participant Game
       participant Player
       participant Dice
       participant State
       
       CLI->>Game: start_game()
       Game->>State: initialize_game()
       Game->>Player: roll_dice()
       Player->>Dice: roll()
       Dice-->>Player: face_value
       Player-->>Game: dice_values
       Game->>State: update_state()
       State-->>Game: game_state
       Game-->>CLI: game_status

**Request Flow:**
1. CLI receives user input
2. Game controller processes commands
3. Core components execute business logic
4. State manager updates game state
5. Response flows back through layers

**Event Handling:**
- Events are processed synchronously
- State changes are atomic
- Error handling at each layer
- Rollback capability for failed operations

Error Handling Strategy
=======================

The system implements comprehensive error handling:

**Expected<T, Error> Pattern**

Uses ``std::expected`` for explicit error handling:

.. code-block:: cpp

   auto result = container.resolve<IGame>();
   if (!result) {
       switch (result.error()) {
           case DIError::kServiceNotFound:
               // Handle missing service
               break;
           case DIError::kCircularDependency:
               // Handle configuration error
               break;
       }
   }

**Exception Hierarchy**

Custom exceptions for different error categories:

.. code-block:: cpp

   class GameException : public std::exception {};
   class InvalidGuessException : public GameException {};
   class PlayerNotFoundException : public GameException {};
   class GameStateException : public GameException {};

**Error Propagation**

Errors are handled at appropriate levels:
- **Input validation**: At presentation layer
- **Business rule violations**: At core layer
- **System errors**: At infrastructure layer

Performance Considerations
==========================

Memory Management
-----------------

**RAII Principles:**
- All resources managed through smart pointers
- Automatic cleanup on scope exit
- No manual memory management

**Object Pooling:**
- Dice objects reused across games
- Player instances pooled for AI opponents
- State objects recycled when possible

**Move Semantics:**
- Extensive use of move constructors
- Avoid unnecessary copying of large objects
- Efficient container operations

Threading Model
---------------

**Single-Threaded Game Logic:**
- Game state modifications are single-threaded
- Eliminates need for complex synchronization
- Predictable behavior and easier debugging

**Thread-Safe Components:**
- ServiceContainer supports concurrent resolution
- AI analysis can run in background threads
- Database operations are naturally concurrent

Extensibility Points
====================

The architecture provides several extension mechanisms:

**Custom AI Implementations**

.. code-block:: cpp

   class CustomAI : public IAIPlayer {
   public:
       Decision makeDecision(const GameState& state) override {
           // Custom decision logic
       }
   };

   // Register with DI container
   container.register_service<IAIPlayer, CustomAI>("custom");

**Game Rule Variants**

.. code-block:: cpp

   class CustomGameRules : public IGameRules {
   public:
       bool is_valid_guess(const Guess& guess, const GameState& state) override {
           // Custom rule implementation
       }
   };

**Persistence Backends**

.. code-block:: cpp

   class CustomPersistence : public IGamePersistence {
   public:
       void save_game(const GameState& state) override {
           // Custom storage implementation
       }
   };

Testing Architecture
====================

The architecture enables comprehensive testing:

**Unit Testing**

Each component can be tested in isolation:

.. code-block:: cpp

   TEST_CASE("Dice rolling behavior") {
       auto mock_rng = std::make_unique<MockRandomGenerator>(sequence);
       auto dice = DiceImpl{std::move(mock_rng)};
       
       dice.roll();
       REQUIRE(dice.get_face_value() == expected_value);
   }

**Integration Testing**

Components tested together through DI:

.. code-block:: cpp

   TEST_CASE("Game integration") {
       auto container = ServiceContainer{};
       container.register_test_services();
       
       auto game = container.resolve<IGame>().value();
       // Test complete game scenarios
   }

**Property-Based Testing**

Verify invariants across many inputs:

.. code-block:: cpp

   TEST_CASE("Game state invariants") {
       auto game_state = GENERATE(valid_game_states());
       
       REQUIRE(game_state.total_dice() >= 0);
       REQUIRE(game_state.active_players() <= game_state.total_players());
   }

Future Architecture Evolution
=============================

Planned Enhancements
--------------------

**1. Event-Driven Architecture**
- Publish-subscribe pattern for game events
- Loose coupling between components
- Real-time analytics and monitoring

**2. Microservices Decomposition**
- AI service as separate process
- Database service with REST API
- Distributed game state management

**3. Plugin Architecture**
- Dynamic loading of game variants
- Custom AI modules as plugins
- Third-party rule implementations

**4. Reactive Streams**
- Asynchronous game state updates
- Real-time multiplayer support
- Event sourcing for game replay

Migration Strategy
------------------

The current architecture supports gradual evolution:

1. **Interface Stability**: Core interfaces remain stable
2. **Implementation Swapping**: New implementations via DI
3. **Layered Rollout**: Component-by-component updates
4. **Backward Compatibility**: Legacy adapter support

.. seealso::
   - :doc:`dependency-injection` - Detailed DI implementation
   - :doc:`interfaces` - Interface design patterns
   - :doc:`../development/testing` - Testing strategies
   - :doc:`../data/uml-diagrams` - Visual architecture diagrams