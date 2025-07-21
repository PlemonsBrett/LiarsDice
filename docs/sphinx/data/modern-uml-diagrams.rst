===============================
Modern Architecture UML Diagrams
===============================

.. contents:: Table of Contents
   :local:
   :depth: 2

Current System Architecture
===========================

This document presents the UML diagrams for the current modern C++23 implementation of the LiarsDice game engine, featuring dependency injection, interface-based design, and comprehensive testing infrastructure.

Core Interface Hierarchy
=========================

The modern architecture is built around a comprehensive interface system that enables dependency injection and testability.

Interface Structure
-------------------

.. mermaid::

   classDiagram
       %% Core Game Interfaces
       class IGame {
           <<interface>>
           +initialize() void
           +add_player(int player_id) void
           +start_game() void
           +process_guess(const Guess& guess) bool
           +process_liar_call(int calling_player_id) string
           +validate_guess(const Guess& guess) string
           +is_game_over() const bool
           +get_winner_id() const int
           +get_game_state() const IGameState&
           +reset() void
           +get_min_players() const size_t
           +get_max_players() const size_t
       }

       class IPlayer {
           <<interface>>
           +get_id() const int
           +add_die() void
           +remove_die() bool
           +roll_dice() void
           +get_dice_count() const size_t
           +has_dice() const bool
           +is_active() const bool
           +get_dice_values() const vector~unsigned int~
           +get_dice() const vector~reference_wrapper~IDice~~
           +count_dice_with_value(unsigned int value) const size_t
       }

       class IDice {
           <<interface>>
           +roll() void
           +get_face_value() const unsigned int
           +set_face_value(unsigned int value) void
           +is_valid_face_value(unsigned int value) const bool
           +clone() const unique_ptr~IDice~
       }

       class IGameState {
           <<interface>>
           +get_current_player_index() const size_t
           +advance_to_next_player() void
           +get_player_count() const size_t
           +is_game_active() const bool
           +set_game_active(bool active) void
           +get_round_number() const int
           +increment_round() void
           +get_last_guess() const optional~Guess~
           +set_last_guess(const Guess& guess) void
           +clear_last_guess() void
           +get_player(size_t index) IPlayer&
           +get_current_player() IPlayer&
           +count_total_dice_with_value(unsigned int face_value) const size_t
           +get_total_dice_count() const size_t
       }

       class IRandomGenerator {
           <<interface>>
           +generate(int min, int max) int
           +seed(unsigned int seed) void
           +generate_bool() bool
           +generate_normalized() double
       }

       %% Service Factory Interface
       class IServiceFactory~T~ {
           <<interface>>
           +create() unique_ptr~T~
       }

Implementation Classes
======================

The concrete implementations provide the actual game logic while conforming to the interface contracts.

Core Implementations
--------------------

.. mermaid::

   classDiagram
       %% Implementations
       class GameImpl {
           -unique_ptr~IGameState~ game_state_
           -unique_ptr~IRandomGenerator~ random_generator_
           -unique_ptr~PlayerFactory~ player_factory_
           -int min_players_
           -int max_players_
           +GameImpl(unique_ptr~IGameState~, unique_ptr~IRandomGenerator~, unique_ptr~PlayerFactory~)
           +initialize() void
           +add_player(int player_id) void
           +start_game() void
           +process_guess(const Guess& guess) bool
           +process_liar_call(int calling_player_id) string
           +validate_guess(const Guess& guess) string
           +is_game_over() const bool
           +get_winner_id() const int
           +get_game_state() const IGameState&
           +reset() void
       }

       class PlayerImpl {
           -int id_
           -vector~unique_ptr~IDice~~ dice_
           -unique_ptr~IRandomGenerator~ random_generator_
           -unique_ptr~ServiceFactory~IDice~~ dice_factory_
           +PlayerImpl(int id, unique_ptr~IRandomGenerator~, unique_ptr~ServiceFactory~IDice~~)
           +get_id() const int
           +add_die() void
           +remove_die() bool
           +roll_dice() void
           +get_dice_count() const size_t
           +has_dice() const bool
           +is_active() const bool
           +get_dice_values() const vector~unsigned int~
           +count_dice_with_value(unsigned int value) const size_t
       }

       class DiceImpl {
           -unsigned int face_value_
           -unique_ptr~IRandomGenerator~ random_generator_
           +DiceImpl(unique_ptr~IRandomGenerator~)
           +DiceImpl(unique_ptr~IRandomGenerator~, unsigned int initial_value)
           +roll() void
           +get_face_value() const unsigned int
           +set_face_value(unsigned int value) void
           +is_valid_face_value(unsigned int value) const bool
           +clone() const unique_ptr~IDice~
       }

       class GameStateImpl {
           -vector~shared_ptr~IPlayer~~ players_
           -size_t current_player_index_
           -bool game_active_
           -int round_number_
           -optional~Guess~ last_guess_
           +get_current_player_index() const size_t
           +advance_to_next_player() void
           +get_player_count() const size_t
           +is_game_active() const bool
           +set_game_active(bool active) void
           +get_round_number() const int
           +increment_round() void
           +get_last_guess() const optional~Guess~
           +set_last_guess(const Guess& guess) void
           +clear_last_guess() void
           +get_player(size_t index) IPlayer&
           +get_current_player() IPlayer&
           +count_total_dice_with_value(unsigned int face_value) const size_t
           +get_total_dice_count() const size_t
       }

       %% Interface Implementation Relationships
       GameImpl ..|> IGame
       PlayerImpl ..|> IPlayer
       DiceImpl ..|> IDice
       GameStateImpl ..|> IGameState

Dependency Injection System
============================

The heart of the modern architecture is the dependency injection container that manages object lifecycles and dependencies.

Service Container Architecture
------------------------------

.. mermaid::

   classDiagram
       class ServiceContainer {
           -unordered_map~type_index, ServiceDescriptor~ services_
           -mutable mutex mutex_
           -thread_local unordered_set~type_index~ resolution_stack_
           +register_service~TInterface, TImpl~(ServiceLifetime lifetime) void
           +register_factory~T~(function~unique_ptr~T~()~ factory) void
           +resolve~T~() expected~unique_ptr~T~, DIError~
           -create_service~T~(const ServiceDescriptor& descriptor) expected~unique_ptr~T~, DIError~
           -validate_no_circular_dependency~T~() expected~void, DIError~
       }

       class ServiceDescriptor {
           +unique_ptr~IServiceFactory~void~~ factory
           +ServiceLifetime lifetime
           +mutable once_flag singleton_flag
           +mutable unique_ptr~void~ singleton_instance
           +ServiceDescriptor(unique_ptr~IServiceFactory~void~~ f, ServiceLifetime lt)
           +ServiceDescriptor(ServiceDescriptor&& other) noexcept
           +operator=(ServiceDescriptor&& other) noexcept
       }

       class ServiceFactory~T~ {
           -function~unique_ptr~T~()~ factory_function_
           +ServiceFactory(function~unique_ptr~T~()~ factory)
           +create() unique_ptr~void~ override
       }

       class IServiceFactory~void~ {
           <<interface>>
           +create() unique_ptr~void~
           +~IServiceFactory() virtual
       }

       enum ServiceLifetime {
           kTransient
           kSingleton
           kScoped
       }

       enum DIError {
           kServiceNotFound
           kCircularDependency
           kFactoryError
           kInvalidLifetime
       }

       ServiceContainer --> ServiceDescriptor : manages
       ServiceDescriptor --> IServiceFactory : contains
       ServiceFactory ..|> IServiceFactory
       ServiceContainer --> ServiceLifetime : uses
       ServiceContainer --> DIError : returns

Data Transfer Objects
=====================

Value objects that carry data between components without behavior.

.. mermaid::

   classDiagram
       class Guess {
           +unsigned int dice_count
           +unsigned int face_value
           +int player_id
           +Guess(unsigned int count, unsigned int value, int id)
           +operator==(const Guess& other) const bool
           +operator!=(const Guess& other) const bool
       }

       class GameConfig {
           +int starting_dice_per_player
           +int minimum_players
           +int maximum_players
           +bool ones_are_wild
           +bool exact_call_rule
           +GameConfig()
           +static GameConfig default_config()
           +validate() bool
       }

       note for Guess "Immutable value object\nRepresents a player's guess"
       note for GameConfig "Configuration data\nfor game rules and setup"

Exception Hierarchy
====================

Modern exception handling with specific error types for different failure scenarios.

.. mermaid::

   classDiagram
       class std_exception {
           <<std>>
           +what() const char*
       }

       class LiarsDiceException {
           -string message_
           +LiarsDiceException(const string& message)
           +what() const char* override
       }

       class InvalidGuessException {
           +InvalidGuessException(const string& message)
       }

       class PlayerNotFoundException {
           +PlayerNotFoundException(int player_id)
       }

       class GameStateException {
           +GameStateException(const string& message)
       }

       class DependencyInjectionException {
           +DependencyInjectionException(const string& message)
       }

       class InvalidDiceValueException {
           +InvalidDiceValueException(unsigned int value)
       }

       std_exception <|-- LiarsDiceException
       LiarsDiceException <|-- InvalidGuessException
       LiarsDiceException <|-- PlayerNotFoundException
       LiarsDiceException <|-- GameStateException
       LiarsDiceException <|-- DependencyInjectionException
       LiarsDiceException <|-- InvalidDiceValueException

Testing Infrastructure
=======================

Comprehensive testing support with mocks, fixtures, and property-based testing.

Test Support Classes
--------------------

.. mermaid::

   classDiagram
       %% Mock Implementations
       class MockRandomGenerator {
           -vector~int~ sequence_
           -mutable size_t index_
           -mutable mt19937 fallback_gen_
           +MockRandomGenerator(vector~int~ sequence)
           +generate(int min, int max) int override
           +seed(unsigned int seed) void override
           +generate_bool() bool override
           +generate_normalized() double override
           +set_sequence(vector~int~ sequence) void
           +reset_index() void
       }

       class TestGameState {
           -vector~shared_ptr~IPlayer~~ players_
           -size_t current_player_index_
           -bool game_active_
           -int round_number_
           -optional~Guess~ last_guess_
           +add_test_player(shared_ptr~IPlayer~ player) void
           +get_current_player_index() const size_t override
           +advance_to_next_player() void override
           +get_player_count() const size_t override
           +is_game_active() const bool override
           +set_game_active(bool active) void override
       }

       %% Test Fixtures
       class GameTestFixture {
           -ServiceContainer container_
           -shared_ptr~MockRandomGenerator~ mock_rng_
           +GameTestFixture()
           +create_game() unique_ptr~IGame~
           +create_player(int id) unique_ptr~IPlayer~
           +setup_predictable_dice(vector~int~ sequence) void
           +verify_game_state() bool
       }

       class PlayerTestFixture {
           +create_player(int id, vector~int~ rng_sequence) unique_ptr~PlayerImpl~
           +create_test_dice_factory() unique_ptr~ServiceFactory~IDice~~
       }

       %% Custom Matchers
       class ValidDiceValueMatcher {
           +match(unsigned int value) const bool
           +describe() const string override
       }

       MockRandomGenerator ..|> IRandomGenerator
       TestGameState ..|> IGameState
       ValidDiceValueMatcher ..|> Catch::Matchers::MatcherGenericBase

Component Interaction Patterns
===============================

Sequence Diagrams
-----------------

**Game Initialization Sequence:**

.. mermaid::

   sequenceDiagram
       participant CLI
       participant Container as ServiceContainer
       participant Game as GameImpl
       participant State as GameStateImpl
       participant Factory as PlayerFactory
       participant Player as PlayerImpl
       
       CLI->>Container: resolve<IGame>()
       Container->>Container: create GameImpl dependencies
       Container->>Game: new GameImpl(state, rng, factory)
       Container-->>CLI: unique_ptr<IGame>
       
       CLI->>Game: initialize()
       Game->>State: set_game_active(false)
       Game->>State: set_round_number(1)
       
       CLI->>Game: add_player(1)
       Game->>Factory: create()
       Factory->>Player: new PlayerImpl(1, rng, dice_factory)
       Factory-->>Game: unique_ptr<IPlayer>
       Game->>State: add_player(player)
       
       CLI->>Game: start_game()
       Game->>State: set_game_active(true)
       Game->>Player: roll_dice()
       Player->>Player: for each die: roll()

**Guess Processing Sequence:**

.. mermaid::

   sequenceDiagram
       participant CLI
       participant Game as GameImpl
       participant State as GameStateImpl
       participant Player as PlayerImpl
       
       CLI->>Game: process_guess(guess)
       Game->>Game: validate_guess(guess)
       Game->>State: get_last_guess()
       State-->>Game: optional<Guess>
       
       alt Valid Guess
           Game->>State: set_last_guess(guess)
           Game->>State: advance_to_next_player()
           Game-->>CLI: true
       else Invalid Guess
           Game-->>CLI: false
       end

**Liar Call Sequence:**

.. mermaid::

   sequenceDiagram
       participant CLI
       participant Game as GameImpl
       participant State as GameStateImpl
       participant Player as PlayerImpl
       
       CLI->>Game: process_liar_call(player_id)
       Game->>State: get_last_guess()
       State-->>Game: Guess
       
       Game->>State: count_total_dice_with_value(face_value)
       State->>Player: count_dice_with_value(face_value)
       Player-->>State: count
       State-->>Game: total_count
       
       alt Liar Call Correct
           Game->>State: get_player(previous_player)
           Game->>Player: remove_die()
           Game-->>CLI: "Previous player loses a die"
       else Liar Call Incorrect
           Game->>State: get_player(calling_player)
           Game->>Player: remove_die()
           Game-->>CLI: "Calling player loses a die"
       end
       
       Game->>State: clear_last_guess()
       Game->>State: increment_round()

Architecture Benefits
=====================

The modern architecture provides significant advantages over the legacy design:

**Testability**
- Interface-based design enables comprehensive mocking
- Dependency injection allows isolated unit testing
- Property-based testing verifies system invariants

**Maintainability**
- Clear separation of concerns through interfaces
- Single responsibility principle for each component
- Explicit dependency management

**Extensibility**
- New implementations easily plugged in via DI
- AI players as interface implementations
- Custom game rules through strategy pattern

**Performance**
- Move semantics throughout for efficient object handling
- RAII resource management prevents leaks
- Optional optimizations like object pooling

**Reliability**
- Comprehensive exception handling
- Type safety with C++23 features
- Circular dependency detection in DI container

Key Design Patterns
===================

**Dependency Injection**
- Constructor injection for required dependencies
- Factory pattern for complex object creation
- Service locator pattern for optional dependencies

**Strategy Pattern**
- Different AI implementations via common interface
- Pluggable random number generators
- Configurable game rules

**Observer Pattern**
- Game state change notifications
- Event-driven architecture for analytics
- Real-time UI updates

**Factory Pattern**
- Service factories for dependency creation
- Abstract factory for game component families
- Factory method for specialized object creation

**Template Method Pattern**
- Base game flow with customizable steps
- AI decision-making framework
- Test fixture base classes

Comparison with Legacy Architecture
===================================

.. list-table:: Architecture Comparison
   :header-rows: 1
   :widths: 30 35 35

   * - Aspect
     - Legacy Design
     - Modern Design
   * - Coupling
     - Tight coupling between classes
     - Loose coupling via interfaces
   * - Testing
     - Difficult to unit test
     - Comprehensive test coverage
   * - Dependencies
     - Hard-coded dependencies
     - Injected dependencies
   * - Extensibility
     - Modification required
     - Plugin-based extension
   * - Error Handling
     - Basic exception hierarchy
     - Comprehensive error types
   * - Memory Management
     - Manual resource management
     - RAII with smart pointers
   * - Performance
     - Basic optimization
     - Move semantics, LTO
   * - Standards
     - C++17 features
     - Modern C++23 features

The modern architecture represents a complete transformation from a monolithic, tightly-coupled design to a modular, testable, and maintainable system that follows industry best practices for modern C++ development.

.. seealso::
   - :doc:`uml-diagrams` - Legacy architecture for historical reference
   - :doc:`../architecture/dependency-injection` - Detailed DI implementation
   - :doc:`../architecture/interfaces` - Interface design patterns
   - :doc:`../development/testing` - Testing strategies and infrastructure