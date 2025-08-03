# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a modern C++23 implementation of the Liar's Dice game following industry best practices. The project is organized as a library (`liarsdice_core`) with a CLI application, AI players, and comprehensive testing infrastructure including automated Robot Framework tests.

## Build System

This project uses **Conan 2.0+** as the primary dependency manager. Conan is required for building the project.

### Prerequisites

```bash
# Install Conan (if not already installed)
pip install conan

# Verify Conan version (must be 2.0+)
conan --version
```

## Essential Commands

### Quick Build and Run

```bash
# Build everything (Release by default)
./scripts/build.sh

# Build Debug version
./scripts/build.sh Debug

# Build with specific Conan profile
./scripts/build.sh Release debug  # Uses profiles/debug

# Run the game
./build/bin/liarsdice-cli

# Run tests
./scripts/test.sh

# Run example
./build/bin/basic_game

# Run Robot Framework tests
./tests/robot/run_tests.sh

# Build documentation (requires Doxygen and Sphinx)
cmake -B build -S . -DLIARSDICE_BUILD_DOCS=ON
cmake --build build --target docs
```

### Manual Build with Conan

```bash
# Install dependencies with Conan
conan install . --build=missing -s build_type=Release

# Configure with CMake using Conan toolchain
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j

# Run tests
ctest --test-dir build --output-on-failure

# Install (optional)
cmake --install build --prefix /usr/local
```

### Development Tools

```bash
# Format code
./scripts/format.sh

# Check formatting (without changes)
./scripts/format.sh --check

# Lint code
./scripts/lint.sh

# Lint with automatic fixes
./scripts/lint.sh --fix

# Lint with verbose output
./scripts/lint.sh --verbose
```

### Documentation

```bash
# Install documentation dependencies
pip install -r docs/requirements.txt

# Build all documentation
cmake --build build --target docs

# Build API docs only (Doxygen)
cmake --build build --target doxygen

# Build user docs only (Sphinx)
cmake --build build --target sphinx-html

# Serve documentation locally
python -m http.server 8000 -d build/docs/html
```

### GitHub Pages Deployment

The project includes automated documentation deployment via GitHub Actions:

- **Workflow**: `.github/workflows/docs.yml` (full CMake build)
- **Simple Workflow**: `.github/workflows/docs-simple.yml` (Sphinx-only)
- **Triggers**: Push to `main` or `Enhancements-SoftwareEngineeringAndDesign` branches
- **Output**: Deployed to `https://[username].github.io/[repository]/`

Manual deployment:

```bash
# Build documentation
cmake -B build -S . -DLIARSDICE_BUILD_DOCS=ON
cmake --build build --target docs

# Deploy to gh-pages branch (requires gh CLI)
gh-pages -d build/docs/html
```

## Project Structure

### Modern C++23 Architecture

```sh
LiarsDice/
├── src/                          # Library implementation
│   └── liarsdice/
│       ├── core/                 # Core game logic
│       │   ├── dice.cpp         # Dice implementation
│       │   ├── player.cpp       # Player management
│       │   └── game.cpp         # Game controller
│       ├── ai/                   # AI strategies
│       │   ├── ai_strategy_factory.cpp  # Strategy factory
│       │   └── easy_ai_strategy.cpp     # Easy AI implementation
│       ├── config/               # Configuration system
│       ├── logging/              # Logging infrastructure
│       └── ...
├── include/                      # Public API headers
│   └── liarsdice/
│       ├── core/                 # Core interfaces
│       ├── ai/                   # AI interfaces
│       │   ├── i_ai_strategy.hpp        # AI strategy interface
│       │   ├── ai_strategy_factory.hpp  # Factory pattern
│       │   └── easy_ai_strategy.hpp     # Easy AI header
│       ├── config/               # Configuration headers
│       ├── exceptions/           # Exception hierarchy
│       └── ...
├── apps/                         # Executable applications
│   └── liarsdice-cli/           # Command-line interface
├── tests/                        # Test suites
│   ├── unit/                    # Unit tests (Catch2)
│   │   ├── core/               # Core component tests
│   │   └── ai/                 # AI tests
│   └── robot/                   # Robot Framework tests
│       ├── liarsdice_tests.robot    # Main test suite
│       ├── edge_cases.robot         # Edge case tests
│       ├── performance.robot        # Performance tests
│       └── LiarsDiceLibrary.py      # Custom library
├── examples/                     # Usage examples
├── cmake/                        # CMake modules
└── scripts/                      # Build scripts
```

### Library Organization

- **Core Library**: `liarsdice::core` - Game logic and data models
- **AI System**: `liarsdice::ai` - AI strategies with factory pattern
- **Configuration**: `liarsdice::config` - Hierarchical configuration system
- **Logging**: `liarsdice::logging` - Structured logging with spdlog
- **Exception Hierarchy**: `liarsdice::exceptions` - Custom exception types
- **CLI Application**: Independent executable using the library

### Key Design Patterns

- **Separation of Concerns**: Library vs application code
- **Modern CMake**: Target-based configuration with proper visibility
- **RAII**: Resource management with smart pointers
- **Exception Safety**: Custom exception hierarchy for error handling
- **Factory Pattern**: AI strategy creation with type erasure
- **Variant Pattern**: Type-safe AI decisions using std::variant

## CMake Build Options

```bash
# Available options (default values shown)
-DLIARSDICE_BUILD_TESTS=ON         # Build test suite
-DLIARSDICE_BUILD_EXAMPLES=ON      # Build example programs  
-DLIARSDICE_BUILD_BENCHMARKS=OFF   # Build performance benchmarks
-DLIARSDICE_BUILD_DOCS=OFF         # Build documentation
-DLIARSDICE_INSTALL=ON             # Generate install targets
-DLIARSDICE_USE_SANITIZERS=ON      # Enable sanitizers in debug builds
-DLIARSDICE_ENABLE_LOGGING=ON      # Enable spdlog logging
-DLIARSDICE_ENABLE_CONFIG=ON       # Enable configuration system
```

## Testing

### Unit Tests (Catch2)

```bash
# All tests (recommended)
./scripts/test.sh

# Specific test by tag
./scripts/test.sh --filter "[dice]"

# Specific test by pattern
./scripts/test.sh --filter "*constructor*"

# List all available tests
./scripts/test.sh --list

# Verbose output
./scripts/test.sh --verbose

# Get help with test options
./scripts/test.sh --help

# Manual test execution
ctest --test-dir build --output-on-failure
./build/bin/unit_tests "[dice]"
```

### Robot Framework Tests (Automated CLI Testing)

```bash
# Run all automated tests
./tests/robot/run_tests.sh

# Run specific test suite
./tests/robot/run_tests.sh --suite main
./tests/robot/run_tests.sh --suite edge
./tests/robot/run_tests.sh --suite performance

# Run tests with specific tags
./tests/robot/run_tests.sh --tag smoke
./tests/robot/run_tests.sh --tag input-validation
./tests/robot/run_tests.sh --exclude long-running

# Enable verbose/debug output
./tests/robot/run_tests.sh --verbose --debug
```

### Test Structure

- **Unit Tests**: `tests/unit/` - Component-level testing with Catch2
- **Robot Framework Tests**: `tests/robot/` - End-to-end CLI testing with Pexpect
- **Test Coverage**: Core components, AI strategies, input validation, performance, signal handling

## AI System

### AI Strategy Architecture

The AI system uses a flexible strategy pattern with:

- **IAIStrategy Interface**: Pure virtual base for all AI strategies
- **AIStrategyFactory**: Singleton factory with type erasure for strategy creation
- **AIDecision Variant**: Type-safe decisions (AIGuessAction, AICallLiarAction)
- **AIDecisionContext**: Structured game state and history for decision making

### Implemented AI Strategies

- **EasyAIStrategy**: Simple heuristics with configurable parameters
  - Risk tolerance (0.0-1.0)
  - Bluff frequency
  - Call threshold
  - Statistical analysis toggle
  
- **MediumAIStrategy**: Statistical AI with advanced probability calculations
  - Bayesian probability calculations
  - Opponent behavior modeling with pattern tracking
  - Bluff detection using statistical analysis
  - Pattern recognition algorithms
  - Uses C++23 ranges for efficient game history analysis
  - Configurable pattern weight and history size

### Adding New AI Strategies

1. Create new strategy class inheriting from `IAIStrategy`
2. Implement `make_decision()`, `get_name()`, and `clone()` methods
3. Register with factory in main or CLI initialization
4. Add configuration in AI strategy config

## Dependencies

### Required

- **CMake 3.28+**: Build system
- **C++23 Compiler**: GCC 12+, Clang 15+, or MSVC 2022+

### Optional

- **Python 3.8+**: For Robot Framework tests
- **Doxygen**: For API documentation
- **Sphinx**: For user documentation

### Logging System

- **Enabled by Default**: All profiles now have logging enabled
- **Log Levels**: 
  - Debug builds use `DEBUG` level for detailed diagnostics
  - Release builds use `INFO` level for important events only
- **Log Format**: Structured logging with spdlog for efficient filtering
- **Performance**: Logging macros are compiled out when disabled

### Dependencies

- **Catch2**: Testing framework
  - Via Conan: `catch2/3.7.0` (preferred)
  - Via FetchContent: Automatically fetched if Conan not available
- **spdlog**: Logging library (optional)
  - Via Conan: `spdlog/1.14.1`
- **fmt**: Formatting library (required by spdlog)
  - Via Conan: `fmt/10.2.1`
- **nlohmann_json**: JSON parsing (optional)
  - Via Conan: `nlohmann_json/3.11.3`

## Development Workflow

### Adding New Features

1. Implement in `src/liarsdice/` (library code)
2. Add public interface to `include/liarsdice/`
3. Write unit tests in `tests/unit/`
4. Add Robot Framework tests in `tests/robot/`
5. Update CLI app in `apps/liarsdice-cli/` if needed
6. Update CLAUDE.md and README.md with changes

### Code Style

- **Formatting**: `.clang-format` (LLVM style with C++23 support)
- **Linting**: `.clang-tidy` (comprehensive rule set for C++23)
- **Naming**: CamelCase for classes, snake_case for functions/variables
- **Standards**: Full C++23 compatibility with clang-tidy integration
- **Comments**: Minimal inline comments, prefer self-documenting code

## Build System Details

### Target-Based CMake

- `liarsdice::core` - Main library target
- `liarsdice::warnings` - Compiler warnings interface
- `liarsdice-cli` - CLI executable
- `unit_tests` - Unit test executable
- `basic_game` - Example executable

### Compiler Features

- **C++23 Standard**: Required for all targets
- **IPO/LTO**: Enabled for Release builds
- **Warnings**: Comprehensive warning set as errors
- **Sanitizers**: Address/UB sanitizers in Debug builds

## Git Workflow

- **Feature Branch**: `Enhancements-SoftwareEngineeringAndDesign` (current)
- **Main Branch**: `main` (target for PRs)
- **Build Artifacts**: All in `build/` directory (gitignored)
- **Documentation**: Auto-deployed via GitHub Actions on push to main/feature branches

## Assets

- **Game Rules**: `assets/rules.txt` automatically copied to build directory
- **Asset Path**: CLI expects assets in `./assets/` relative to executable

## Known Considerations

- Platform-specific code exists (console clearing) - may need abstraction
- Game uses interactive I/O - consider dependency injection for better testability
- Mersenne Twister RNG properly seeded for realistic dice behavior
- Logging can be disabled at compile time to reduce dependencies

## CI/CD Configuration

### Conan Profiles

The project includes Conan profiles for different environments:

- **profiles/default**: macOS/Linux development profile with C++23
  - Logging disabled by default to avoid dependency issues
  - Can be enabled by setting `&:enable_logging=True`
- **profiles/ci**: CI environment profile for GitHub Actions (Linux, Clang 15, C++23)

When running in CI, ensure the profile includes:

```ini
[settings]
compiler.cppstd=23  # Required for dependencies like catch2 and spdlog

[options]
&:enable_logging=False  # Disable if spdlog causes issues
```

### Common CI Issues

1. **"The compiler.cppstd is not defined" error**: The CI profile must include `compiler.cppstd=23`
2. **Build tools profile**: Conan uses separate profiles for host and build - ensure both have required settings
3. **spdlog/fmt compatibility**: If encountering issues, disable logging in Conan profile

## Robot Framework Test Details

### Test Categories

- **Input Validation**: Invalid inputs, edge cases, security tests
- **Game Logic**: Win/lose conditions, game flow, AI behavior
- **Performance**: Response times, memory usage, stress testing
- **Signal Handling**: Ctrl+C, SIGTERM, graceful shutdown
- **Timeouts**: User inactivity handling

### Key Test Library Features

- **Pexpect Integration**: Simulates real CLI interaction
- **Performance Monitoring**: Memory and response time tracking
- **Signal Testing**: Tests interrupt handling
- **Pattern Matching**: Flexible output validation

## Type Safety

- **Unsigned Integers**: All dice counts, face values, and player IDs use unsigned integers
- **Size Types**: Use `size_t` for container indices and sizes  
- **No Negative Values**: Game logic never requires negative numbers for dice or players
- **Explicit Conversions**: Always use explicit casts when converting between signed/unsigned

## Developer Guidance

- **General Practice**:
  - Always create scripts to make running common tasks easier
  - Ensure to always use industry best practices for C++ Standard 23 with Clang, Clang-Tidy, and Clang-Format
  - When adding a new script, or making changes to build or project configurations, make sure that we update CLAUDE.md and README.md
  - Prefer LLVM code style for consistency
  - Write Robot Framework tests for new CLI features
  - Keep AI strategies modular and configurable
  - Use the configuration system for runtime parameters
  - Add appropriate logging for debugging (when enabled)

- **Documentation**:
  - Update Sphinx documentation when making architectural changes
  - Technical design documents go in `docs/technical/`
  - RST versions for Sphinx go in `docs/sphinx/technical/`
  - Update the Sphinx index.rst when adding new documentation sections
  - Keep both markdown and RST versions in sync

## Development Memory

- When adding new features, integrate into existing game, and write end-to-end tests with robot framework and pexpect.
- Ensure that we are regularly updating the sphinx documentation when changes are made.