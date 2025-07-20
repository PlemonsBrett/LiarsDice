# 🎲 Liar's Dice

A modern C++23 implementation of the classic Liar's Dice game with a professional library structure, comprehensive testing, and development tools.

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.21+-green.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## 📖 About the Game

Liar's Dice is a multiplayer dice game with a minimum of two players and no upper limit on participants. The goal is to make correct guesses about the total dice values across all players or successfully call another player a liar.

### Game Rules

- Each player starts with 5 dice
- Players take turns guessing how many dice of a specific value exist across all players' dice
- Each guess must be higher than the previous (more dice or higher value)
- Players can call "liar" instead of making a guess
- When "liar" is called, all dice are revealed to determine the winner

### Cultural Background

Throughout China, Liar's Dice (說謊者的骰子, shuōhuǎng zhě de shǎizi) is played during holidays, especially Chinese New Year. It's traditionally a social drinking game played at celebrations, bars, and restaurants.

## 🚀 Quick Start

### Prerequisites

- **C++23 compatible compiler** (GCC 12+, Clang 15+, or MSVC 2022+)
- **CMake 3.21+**
- **Git** (for cloning)

### Installation & Build

```bash
# Clone the repository
git clone <repository-url>
cd LiarsDice

# Build the project (Release mode)
./scripts/build.sh

# Or build manually
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Running the Game

```bash
# Run the interactive CLI game
./build/bin/liarsdice-cli

# Run the library example
./build/bin/basic_game

# Run tests
./scripts/test.sh
```

## 🏗️ Project Architecture

This project follows modern C++ best practices with a clean separation between library and application code:

```
LiarsDice/
├── src/                          # Library implementation
│   └── liarsdice/
│       ├── core/                 # Core game logic
│       │   ├── dice.cpp         # Dice implementation
│       │   ├── player.cpp       # Player management  
│       │   └── game.cpp         # Game controller
│       ├── exceptions/           # Custom exceptions
│       └── utils/                # Utility functions
├── include/                      # Public API headers
│   └── liarsdice/
│       ├── core/                 # Core interfaces
│       ├── exceptions/           # Exception hierarchy
│       └── liarsdice.hpp        # Main convenience header
├── apps/                         # Executable applications
│   └── liarsdice-cli/           # Command-line interface
├── tests/                        # Test suite (Catch2)
│   └── unit/core/               # Unit tests
├── examples/                     # Usage examples
├── cmake/                        # CMake modules
├── scripts/                      # Build automation
└── assets/                       # Game resources
```

### Library Design

- **`liarsdice::core`** - Reusable game logic library
- **Modern C++23** - Uses latest language features
- **Exception Safety** - Custom exception hierarchy
- **RAII** - Proper resource management
- **Testing** - Comprehensive unit test coverage

## 🛠️ Development

### Build Options

```bash
# Development build with debug info
./scripts/build.sh Debug

# Manual configuration with options
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLIARSDICE_BUILD_TESTS=ON \
  -DLIARSDICE_BUILD_EXAMPLES=ON \
  -DLIARSDICE_BUILD_BENCHMARKS=OFF
```

### Available CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `LIARSDICE_BUILD_TESTS` | `ON` | Build the test suite |
| `LIARSDICE_BUILD_EXAMPLES` | `ON` | Build example programs |
| `LIARSDICE_BUILD_BENCHMARKS` | `OFF` | Build performance benchmarks |
| `LIARSDICE_BUILD_DOCS` | `OFF` | Build documentation |
| `LIARSDICE_INSTALL` | `ON` | Generate install targets |
| `LIARSDICE_USE_SANITIZERS` | `ON` | Enable sanitizers in debug builds |

### Testing

```bash
# Run all tests
./scripts/test.sh

# Run specific test categories
./scripts/test.sh --filter "[dice]"

# Run tests matching a pattern  
./scripts/test.sh --filter "*constructor*"

# List all available tests
./scripts/test.sh --list

# Run tests with verbose output
./scripts/test.sh --verbose

# Get help with test script options
./scripts/test.sh --help

# Run tests manually with ctest
ctest --test-dir build --output-on-failure

# Run tests directly with Catch2
./build/bin/unit_tests "[dice]"
```

### Code Quality

```bash
# Format code (requires clang-format)
find . -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Lint code (requires clang-tidy)  
clang-tidy src/**/*.cpp -p build

# Generate compile commands for IDE support
cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

## 🔧 IDE Setup

### Visual Studio Code

The project includes VSCode configuration for:
- **IntelliSense** - C++23 support with proper include paths
- **Debugging** - Ready-to-use launch configurations
- **Building** - Integrated build tasks
- **Testing** - Catch2 integration

Open the project in VSCode and use:
- `Ctrl+Shift+P` → "CMake: Configure" to set up the build
- `F5` to debug the application
- `Ctrl+Shift+P` → "Test: Run All Tests" to run tests

### Other IDEs

- **CLion** - Import as CMake project
- **Visual Studio** - Open folder or import CMake
- **Qt Creator** - Open CMakeLists.txt

## 📚 Using as a Library

```cpp
#include "liarsdice/liarsdice.hpp"

int main() {
    // Create dice
    liarsdice::Dice die;
    die.Roll();
    std::cout << "Rolled: " << die.GetFaceValue() << std::endl;
    
    // Create player
    liarsdice::Player player(1);
    player.RollDice();
    
    // Access player's dice
    const auto& dice = player.GetDice();
    for (const auto& d : dice) {
        std::cout << d.GetFaceValue() << " ";
    }
    
    return 0;
}
```

### Integration

#### CMake

```cmake
find_package(LiarsDice REQUIRED)
target_link_libraries(your_target PRIVATE liarsdice::core)
```

#### Manual Linking

```bash
g++ -std=c++23 your_app.cpp -I/path/to/include -L/path/to/lib -lliarsdice_core
```

## 🐛 Troubleshooting

### Common Issues

**Build fails with C++23 errors:**
- Ensure you have a modern compiler (GCC 12+, Clang 15+)
- Check that CMake is version 3.21 or higher

**Tests fail to build:**
- Catch2 is automatically fetched, ensure internet connectivity
- For offline builds, install Catch2 system-wide

**Missing compile_commands.json:**
```bash
cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

**Asset loading errors:**
- Assets are automatically copied to build directory
- Ensure `assets/rules.txt` exists in the source directory

### Performance

- **Release builds** use IPO/LTO optimization automatically
- **Debug builds** include sanitizers for memory safety
- **Profiling** builds can be created with `-DCMAKE_BUILD_TYPE=RelWithDebInfo`

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Follow the existing code style (`.clang-format` provided)
4. Add tests for new functionality
5. Ensure all tests pass (`./scripts/test.sh`)
6. Commit changes (`git commit -m 'Add amazing feature'`)
7. Push to branch (`git push origin feature/amazing-feature`)
8. Open a Pull Request

### Code Standards

- **C++23** standard compliance
- **Google style** formatting (automated with `.clang-format`)
- **Comprehensive testing** for new features
- **Exception safety** and RAII principles
- **Clear documentation** and comments

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Game rules and cultural background from [ThoughtCo](https://www.thoughtco.com/how-to-play-liars-dice-687532)
- Modern C++ project structure inspired by industry best practices
- CMake configuration follows conventions from major C++ libraries

## 📞 Support

- **Issues** - Report bugs and request features via GitHub Issues
- **Documentation** - See `CLAUDE.md` for detailed development guidance
- **Examples** - Check the `examples/` directory for usage patterns

---

**Happy gaming! 🎲**