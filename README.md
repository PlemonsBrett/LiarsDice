# 🎲 Liar's Dice (Boost Edition)

A modern C++20 implementation of the classic Liar's Dice game using Boost libraries, featuring AI players, comprehensive testing with Robot Framework, and clean architecture.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.14+-green.svg)](https://cmake.org/)
[![Boost](https://img.shields.io/badge/Boost-1.70+-orange.svg)](https://www.boost.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## 📖 About the Game

Liar's Dice is a multiplayer dice game where players make increasingly bold claims about dice rolls and challenge each other's honesty. It's a game of probability, psychology, and deception.

### Game Rules

- Each player starts with five dice
- Players take turns guessing how many dice of a specific value exist across all players' dice
- Each guess must be higher than the previous (more dice or same dice with higher face value)
- Players can call "liar" instead of making a guess
- When "liar" is called, all dice are revealed to determine the winner
- The loser of a challenge loses points
- Last player with dice wins!

## 🚀 Quick Start

### Prerequisites

- **C++20 compatible compiler** (GCC 11+, Clang 13+, or MSVC 2019+)
- **CMake 3.14+**
- **Boost 1.70+** (will be found automatically on most systems)
- **Python 3.8+** (for Robot Framework tests)

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd LiarsDice

# Quick build
./build.sh

# Or manual build
mkdir build && cd build
cmake ..
make -j

# Build in Debug mode
./build.sh Debug
```

### Running the Game

```bash
# Run the game
./build/standalone/liarsdice

# Run with verbose logging
./build/standalone/liarsdice --verbose
```

### Running Tests

```bash
# Run all tests (unit + Robot Framework)
./test.sh

# Run only unit tests
cd build/test
./test_dice
./test_player
./test_game
./test_ai
./test_service_container

# Run only Robot Framework tests
./test/robot/run_tests.sh
```

## 🏗️ Architecture

This implementation uses modern C++20 with Boost libraries for a clean, maintainable architecture:

### Core Components

- **Dependency Injection**: Custom lightweight DI container for loose coupling
- **Event System**: Boost.Signals2 for game events and notifications
- **Logging**: Boost.Log with configurable severity levels
- **Testing**: Boost.Test for unit tests, Robot Framework for E2E tests
- **Random Numbers**: Boost.Random for cryptographically secure dice rolls

### Project Structure

```
LiarsDice/
├── include/          # Public headers
│   └── liarsdice/
│       ├── ai/       # AI player strategies
│       ├── core/     # Core game logic
│       ├── di/       # Dependency injection
│       └── ui/       # User interface components
├── source/           # Implementation files
├── test/             # All tests
│   ├── robot/        # Robot Framework E2E tests
│   └── source/       # Unit tests
├── standalone/       # CLI application
└── docs/            # Documentation
```

## 🤖 AI Players

The game features AI opponents with different difficulty levels:

- **Easy AI**: Makes conservative, probability-based decisions
- **Medium AI**: Uses statistical analysis and pattern recognition
- **Hard AI**: Advanced strategies with opponent modeling

## 🧪 Testing

### Unit Tests (Boost.Test)
- Comprehensive coverage of all components
- Property-based testing for edge cases
- Performance benchmarks

### E2E Tests (Robot Framework)
- Automated CLI interaction testing
- Input validation testing
- Game flow scenarios
- Signal handling tests

## 📝 Development

### Adding Build Scripts

The project includes convenient build scripts:

- `build.sh` - Build the project
- `test.sh` - Run all tests
- `clean.sh` - Clean build artifacts

### Code Style

- Modern C++20 features
- RAII for resource management
- Strong type safety with concepts
- Minimal use of raw pointers

## 🤝 Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct and the process for submitting pull requests.

## 📄 License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Boost C++ Libraries for excellent infrastructure
- Robot Framework for powerful E2E testing capabilities
- The C++ community for continuous inspiration