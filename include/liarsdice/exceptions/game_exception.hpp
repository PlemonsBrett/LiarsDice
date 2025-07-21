#pragma once

#include <stdexcept>
#include <string>

namespace liarsdice::exceptions {

/**
 * @brief Exception thrown for game-related errors
 * 
 * This exception is used for errors that occur during game logic
 * operations, such as invalid moves, rule violations, or state errors.
 */
class GameException : public std::runtime_error {
public:
    explicit GameException(const std::string& message) 
        : std::runtime_error(message) {}
    
    explicit GameException(const char* message) 
        : std::runtime_error(message) {}
};

/**
 * @brief Exception thrown for invalid game state operations
 */
class InvalidGameStateException : public GameException {
public:
    explicit InvalidGameStateException(const std::string& message) 
        : GameException("Invalid game state: " + message) {}
};

/**
 * @brief Exception thrown for invalid player operations
 */
class InvalidPlayerException : public GameException {
public:
    explicit InvalidPlayerException(const std::string& message) 
        : GameException("Invalid player operation: " + message) {}
};

/**
 * @brief Exception thrown for invalid guess operations
 */
class InvalidGuessException : public GameException {
public:
    explicit InvalidGuessException(const std::string& message) 
        : GameException("Invalid guess: " + message) {}
};

} // namespace liarsdice::exceptions