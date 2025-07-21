===================================
Legacy Architecture UML Diagrams  
===================================

.. contents:: Table of Contents
   :local:
   :depth: 2

.. note::
   **Historical Reference**: This document represents the original, legacy architecture 
   of the LiarsDice game before the modern C++23 enhancements. For the current 
   architecture with dependency injection and interfaces, see :doc:`modern-uml-diagrams`.

Legacy Core Game Classes
========================

The following UML class diagram shows the original structure of the LiarsDice game engine before the dependency injection and interface-based refactoring.

Class Relationships
-------------------

.. mermaid::

   classDiagram
       class Game {
           -vector~Player~ players
           -int currentPlayerIndex
           -Guess lastGuess
           -string rulesText
           +Game()
           +Init() void
           +ReadRulesFromFile(string filename) string
           +SetupPlayers() void
           +PlayGame() void
           +ValidateGuess(Guess newGuess, Guess lastGuess) string
           +CheckGuessAgainstDice(Guess lastGuess) string
           -updateCurrentPlayerIndex() void
           -displayCurrentState(Player& currentPlayer) void
           -GetSetupInput(int& numPlayers) void
       }

       class Player {
           -int id
           -vector~Dice~ dice
           +Player(int id)
           +RollDice() void
           +DisplayDice() void
           +MakeGuess() pair~int,int~
           +CallLiar() bool
           +GetDice() const vector~Dice~&
           +GetPlayerId() int
       }

       class Dice {
           -unsigned int face_value
           -random_device rd
           -mt19937 gen
           -uniform_int_distribution~~ dis
           +Dice()
           +Roll() void
           +GetFaceValue() unsigned int
       }

       class Guess {
           +int diceValue
           +int diceCount
           +Guess(pair~int,int~ guess_pair)
       }

       class CustomException {
           -string message
           +CustomException(string message)
           +what() const char*
       }

       class FileException {
           +FileException(string message)
       }

       class GameLogicException {
           +GameLogicException(string message)
       }

       class InputException {
           +InputException(string message)
       }

       Game --> Player
       Player --> Dice
       Game --> Guess
       Game --> FileException
       Game --> GameLogicException
       Player --> InputException
       CustomException --> FileException
       CustomException --> GameLogicException
       CustomException --> InputException

       note for Game "Main controller class\nHandles game flow and logic"
       note for Player "Represents a game participant\nManages dice and user input"
       note for Dice "Individual die with random generation"

Class Descriptions
==================

Game Class
----------

The central controller class that manages the overall game flow and logic.

**Responsibilities:**
- Player management and turn coordination
- Game rule validation and enforcement
- State management and progression
- File I/O for rules and configuration

**Key Methods:**
- ``ValidateGuess()``: Ensures new guesses follow game rules
- ``CheckGuessAgainstDice()``: Verifies guess accuracy against actual dice
- ``PlayGame()``: Main game loop implementation

Player Class
------------

Represents an individual game participant with their dice collection.

**Responsibilities:**
- Dice management (rolling, displaying)
- Input handling for guesses and liar calls
- Player identification and state tracking

**Key Methods:**
- ``MakeGuess()``: Handles player input for dice guesses
- ``CallLiar()``: Processes liar call decisions
- ``RollDice()``: Initiates dice rolling for the player

Dice Class
----------

Individual die implementation with random number generation.

**Responsibilities:**
- Random value generation (1-6)
- Face value storage and retrieval
- Thread-safe random number generation

**Key Features:**
- Uses ``std::mt19937`` for high-quality randomness
- Proper seeding with ``std::random_device``
- Uniform distribution for fair dice rolls

Guess Class
-----------

Data structure representing a player's guess about dice on the table.

**Properties:**
- ``diceValue``: The face value being guessed (1-6)
- ``diceCount``: The number of dice expected to show that value

Exception Hierarchy
====================

The game implements a comprehensive exception hierarchy for robust error handling:

CustomException (Base)
----------------------

Abstract base class for all game-specific exceptions.

**Design Pattern:** Provides consistent interface for error handling across the application.

FileException
-------------

Thrown when file operations fail (reading rules, saving game state).

**Common Scenarios:**
- Missing rules.txt file
- Insufficient file permissions
- Corrupted game data files

GameLogicException
------------------

Thrown when game rule violations occur.

**Common Scenarios:**
- Invalid guess sequences
- Player actions out of turn
- Malformed game state

InputException
--------------

Thrown when user input validation fails.

**Common Scenarios:**
- Non-numeric input where numbers expected
- Out-of-range values
- Empty or malformed input strings

Design Patterns Employed
=========================

1. **Controller Pattern**
   - Game class acts as central controller
   - Coordinates between Player and Dice components
   - Manages overall application flow

2. **Composition**
   - Game contains multiple Player objects
   - Player contains multiple Dice objects
   - Clear ownership hierarchy

3. **Exception Handling Strategy**
   - Hierarchical exception design
   - Specific exception types for different error categories
   - Consistent error reporting interface

4. **Value Object Pattern**
   - Guess class represents immutable data
   - Encapsulates related data (count + value)
   - Simple data transfer object

Dependencies and Relationships
==============================

Aggregation Relationships
--------------------------

- **Game aggregates Players**: Game manages a collection of Player objects
- **Player aggregates Dice**: Each player owns multiple Dice objects

Association Relationships
-------------------------

- **Game uses Guess**: Game validates and processes Guess objects
- **Game handles Exceptions**: Game catches and processes various exception types

Dependency Relationships
------------------------

- **Player depends on Dice**: Player functionality requires Dice for game actions
- **All classes depend on CustomException**: Exception handling throughout the system

Evolution to Modern Architecture
================================

This legacy architecture was refactored to address several limitations:

**Issues with Legacy Design:**

- **Tight Coupling**: Direct dependencies between classes made testing difficult
- **Hard-coded Dependencies**: No way to substitute implementations for testing
- **Monolithic Structure**: Large classes with multiple responsibilities
- **Limited Extensibility**: Adding new features required modifying existing code

**Modern Improvements:**

The current architecture addresses these issues through:

- **Dependency Injection**: Loose coupling via constructor injection
- **Interface-based Design**: Abstract contracts enable testing and extensibility
- **Single Responsibility**: Each class has one focused purpose
- **Comprehensive Testing**: Mock implementations for all dependencies

.. seealso::
   - :doc:`modern-uml-diagrams` - Current architecture with DI and interfaces
   - :doc:`../architecture/dependency-injection` - Modern DI implementation
   - :doc:`../architecture/overview` - Architectural evolution details
   - :doc:`database-schema` - Data persistence models