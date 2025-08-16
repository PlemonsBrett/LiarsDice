# Contributing to Liar's Dice

Thank you for your interest in contributing to this modern C++20 implementation of Liar's Dice! This document provides
guidelines and information for contributors.

## Table of Contents

- [Project Overview](#project-overview)
- [Development Setup](#development-setup)
- [Development Workflow](#development-workflow)
- [Code Style Guidelines](#code-style-guidelines)
- [Testing Guidelines](#testing-guidelines)
- [Project Structure](#project-structure)
- [Build System](#build-system)
- [Submitting Changes](#submitting-changes)

## Project Overview

This is a modern C++20 implementation of the Liar's Dice game following industry best practices. The project features
advanced AI strategies with Bayesian inference, custom data structures optimized for gaming, and comprehensive testing
infrastructure using Boost.Test and Robot Framework.

## Development Setup

### Prerequisites

#### Required

- **CMake 3.15+**: Build system with CPM (CMake Package Manager)
- **C++20 Compiler**: GCC 12+, Clang 15+, or MSVC 2022+
- **Git**: For CPM dependency management

#### Optional
- **Python 3.8+**: For Robot Framework tests
- **Doxygen**: For API documentation

### Quick Start

```bash
# Clone the repository
git clone <repository-url>
cd LiarsDice

# Build everything (Release by default)
./build.sh

# Run tests
./scripts/test.sh

# Run the game
./build/standalone/liarsdice
```

### Build Commands

#### Using Build Scripts (Recommended)

```bash
# Build with specific type
./build.sh Debug

# Run all tests
./test.sh

# Run Robot Framework tests
./test/robot/run_tests.sh

# Clean build
./clean.sh
```

#### Manual CMake Commands

```bash
# Configure (creates build directory)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j

# Run tests
ctest --test-dir build --output-on-failure

# Install (optional)
cmake --install build --prefix /usr/local
```

### CMake Build Options

```bash
# Available options (default values shown)
-DLiarsDice_BUILD_TESTS=ON         # Build test suite
-DLiarsDice_BUILD_EXAMPLES=ON      # Build example programs  
-DLiarsDice_BUILD_DOCS=OFF         # Build documentation
-DLiarsDice_USE_SANITIZERS=ON      # Enable sanitizers in debug builds
-DLiarsDice_WARNINGS_AS_ERRORS=ON  # Treat warnings as errors
```

## Development Workflow

### Adding New Features

1. Create a feature branch from `main`
2. Implement in `source/` (source code)
3. Add public interface to `include/liarsdice/`
4. Write unit tests in `test/source/`
5. Add Robot Framework tests in `test/robot/`
6. Update CLI app in `standalone/` if needed
7. Update documentation (README.md, Journal.MD, etc.)
8. Submit a pull request

### Development Tools

```bash
# Build the project
./build.sh

# Run all tests
./test.sh

# Clean build directory
./clean.sh

# Format code (if available)
clang-format -i source/**/*.cpp include/**/*.hpp

# Lint code (if available)
clang-tidy source/**/*.cpp -- -Iinclude
```

## Code Style Guidelines

- **Language Standard**: C++20
- **Formatting**: LLVM style (enforced by `.clang-format`)
- **Linting**: Comprehensive rule set (enforced by `.clang-tidy`)
- **Naming Conventions**:
  - Classes: `CamelCase`
  - Functions/Variables: `snake_case`
  - Constants: `UPPER_SNAKE_CASE`
  - Namespaces: `lowercase`
- **Comments**: Minimal inline comments, prefer self-documenting code
- **Documentation**: Doxygen comments for public APIs

### Best Practices

- Follow RAII principles for resource management
- Use smart pointers instead of raw pointers
- Prefer standard library algorithms over manual loops
- Write exception-safe code
- Keep functions small and focused
- Use const-correctness
- Avoid magic numbers—use named constants

## Testing Guidelines

### Unit Tests (Boost.Test)

All new code should include comprehensive unit tests using Boost.Test framework:

```bash
# Run all tests
./test.sh

# Run specific test executable
./build/test/test_dice
./build/test/test_bayesian
./build/test/test_ai

# Run with verbose output
ctest --test-dir build --output-on-failure
```

### Robot Framework Tests

For CLI features, add end-to-end tests:

```bash
# Run all automated tests
./test/robot/run_tests.sh

# Run specific test suite
./test/robot/run_tests.sh --suite main

# Run with specific tags
./test/robot/run_tests.sh --tag smoke
```

### Test Coverage Areas

- **Unit Tests**: Component-level testing with Boost.Test
- **Integration Tests**: Multi-component interaction testing
- **Robot Framework Tests**: End-to-end CLI testing with Pexpect
- **Performance Tests**: Response time and memory usage validation
- **Bayesian Tests**: Statistical algorithm validation with deterministic seeding

## Project Structure

```
LiarsDice/
├── source/                # Library implementation
│   ├── ai/               # AI strategies with statistical analysis
│   ├── app/              # Application framework
│   ├── core/             # Core game logic
│   ├── di/               # Dependency injection
│   ├── liarsdice/        # Bayesian inference engine
│   │   └── bayesian/     # Bayesian analysis components
│   ├── network/          # Network components
│   └── ui/               # User interface
├── include/              # Public API headers
│   └── liarsdice/
│       ├── ai/           # AI interfaces
│       ├── bayesian/     # Bayesian analysis
│       ├── core/         # Core interfaces
│       ├── data_structures/ # Custom data structures
│       ├── performance/  # Performance optimizations
│       └── statistics/   # Statistical algorithms
├── standalone/           # Standalone executable
├── test/                 # Test suites
│   ├── source/          # Unit tests (Boost.Test)
│   └── robot/           # Robot Framework tests
├── cmake/               # CMake modules
├── build.sh             # Main build script
├── test.sh              # Test runner
├── clean.sh             # Clean script
├── Journal.MD           # Development journal
├── CONTRIBUTING.md      # This file
└── README.md            # Project overview
```

### Key Components

- **Core Library**: `liarsdice::core` - Game logic and data models
- **AI System**: `liarsdice::ai` - Statistical AI strategies with Bayesian inference
- **Bayesian Engine**: `liarsdice::bayesian` - Advanced statistical analysis and inference
- **Data Structures**: `liarsdice::data_structures` - Custom optimized containers (LRU cache, circular buffer, etc.)
- **Performance**: `liarsdice::performance` - SIMD operations and custom allocators
- **Statistics**: `liarsdice::statistics` - Statistical algorithms and accumulators
- **Standalone Application**: Command-line game using the library

## Build System

The project uses modern CMake with CPM (CMake Package Manager) for dependencies:

- **Main Targets**:
    - `LiarsDice` - Main library
    - `LiarsDiceStandalone` - Standalone executable
    - `test_*` - Individual test executables (test_dice, test_bayesian, etc.)

- **Dependencies**:
    - **Boost**: Math, test framework, and utility libraries
    - **xsimd**: SIMD vectorization library
    - **fmt**: String formatting library

- **Compiler Features**:
    - C++20 standard required
  - IPO/LTO enabled for Release builds
  - Comprehensive warnings treated as errors
  - Address/UB sanitizers in Debug builds

## Submitting Changes

### Pull Request Process

1. **Before Submitting**:
    - Ensure all tests pass (`./test.sh`)
    - Verify build completes successfully (`./build.sh`)
    - Update documentation if needed (Journal.MD, README.md)
   - Add tests for new functionality

2. **PR Guidelines**:
   - Create a descriptive title
   - Reference any related issues
   - Provide a clear description of changes
   - Include test results
   - Ensure CI checks pass

3. **Code Review**:
   - Address reviewer feedback promptly
   - Keep commits clean and logical
   - Squash commits if requested

### Commit Message Format

Use clear, descriptive commit messages:

```
<type>: <subject>

<body>

<footer>
```

Types: feat, fix, docs, style, refactor, test, chore

Example:
```
feat: Implement Bayesian inference engine

Add comprehensive Bayesian analysis framework with
PosteriorCalculator, PriorDistribution hierarchy, and
LikelihoodFunction interface for advanced AI decision-making.

Closes #123
```

## Getting Help

- Check existing issues and documentation
- Ask questions in discussions or issues
- Review the code style guide and examples
- Refer to the comprehensive test suite for usage patterns

## Additional Resources

- [README.md](README.md) - Project overview and usage
- [Journal.MD](DataStructuresAndAlgorithmsJournal.MD) - Development journal and technical documentation
- [CMake Documentation](https://cmake.org/documentation/)
- [C++20 Reference](https://en.cppreference.com/)
- [Boost Libraries](https://www.boost.org/) - Core mathematical and utility libraries
- [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) - CMake Package Manager
- [Robot Framework User Guide](https://robotframework.org/robotframework/latest/RobotFrameworkUserGuide.html)

Thank you for contributing to Liar's Dice!