# Contributing to Liar's Dice

Thank you for your interest in contributing to this modern C++23 implementation of Liar's Dice! This document provides guidelines and information for contributors.

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

This is a modern C++23 implementation of the Liar's Dice game following industry best practices. The project is organized as a library (`liarsdice_core`) with a CLI application, AI players, and comprehensive testing infrastructure including automated Robot Framework tests.

## Development Setup

### Prerequisites

#### Required
- **CMake 3.28+**: Build system
- **C++23 Compiler**: GCC 12+, Clang 15+, or MSVC 2022+

#### Optional
- **Conan 2.0+**: Modern package manager (recommended for easier dependency management)
- **Python 3.8+**: For Robot Framework tests
- **Doxygen**: For API documentation
- **Sphinx**: For user documentation

### Quick Start

```bash
# Clone the repository
git clone <repository-url>
cd LiarsDice

# Build everything (Release by default, auto-detects Conan)
./scripts/build.sh

# Run tests
./scripts/test.sh

# Run the game
./build/bin/liarsdice-cli
```

### Build Commands

#### Using Build Scripts (Recommended)

```bash
# Build with specific type
./scripts/build.sh Debug

# Build with Conan explicitly
./scripts/build-conan.sh

# Build without Conan (FetchContent)
./scripts/build.sh Release no

# Run Robot Framework tests
./tests/robot/run_tests.sh
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
-DLIARSDICE_BUILD_TESTS=ON         # Build test suite
-DLIARSDICE_BUILD_EXAMPLES=ON      # Build example programs  
-DLIARSDICE_BUILD_BENCHMARKS=OFF   # Build performance benchmarks
-DLIARSDICE_BUILD_DOCS=OFF         # Build documentation
-DLIARSDICE_INSTALL=ON             # Generate install targets
-DLIARSDICE_USE_SANITIZERS=ON      # Enable sanitizers in debug builds
-DLIARSDICE_ENABLE_LOGGING=ON      # Enable spdlog logging
-DLIARSDICE_ENABLE_CONFIG=ON       # Enable configuration system
```

## Development Workflow

### Adding New Features

1. Create a feature branch from `main`
2. Implement in `src/liarsdice/` (library code)
3. Add public interface to `include/liarsdice/`
4. Write unit tests in `tests/unit/`
5. Add Robot Framework tests in `tests/robot/`
6. Update CLI app in `apps/liarsdice-cli/` if needed
7. Update documentation (README.md, etc.)
8. Submit a pull request

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

## Code Style Guidelines

- **Language Standard**: C++23
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

### Unit Tests (Catch2)

All new code should include comprehensive unit tests:

```bash
# Run all tests
./scripts/test.sh

# Run specific test by tag
./scripts/test.sh --filter "[dice]"

# Run with verbose output
./scripts/test.sh --verbose
```

### Robot Framework Tests

For CLI features, add end-to-end tests:

```bash
# Run all automated tests
./tests/robot/run_tests.sh

# Run specific test suite
./tests/robot/run_tests.sh --suite main

# Run with specific tags
./tests/robot/run_tests.sh --tag smoke
```

### Test Coverage Areas

- **Unit Tests**: Component-level testing with Catch2
- **Integration Tests**: Multi-component interaction testing
- **Robot Framework Tests**: End-to-end CLI testing with Pexpect
- **Performance Tests**: Response time and memory usage validation

## Project Structure

```
LiarsDice/
├── src/                    # Library implementation
│   └── liarsdice/
│       ├── core/          # Core game logic
│       ├── ai/            # AI strategies
│       ├── config/        # Configuration system
│       └── logging/       # Logging infrastructure
├── include/               # Public API headers
│   └── liarsdice/
├── apps/                  # Executable applications
│   └── liarsdice-cli/
├── tests/                 # Test suites
│   ├── unit/             # Unit tests (Catch2)
│   └── robot/            # Robot Framework tests
├── examples/             # Usage examples
├── cmake/                # CMake modules
├── scripts/              # Build/development scripts
└── docs/                 # Documentation
```

### Key Components

- **Core Library**: `liarsdice::core` - Game logic and data models
- **AI System**: `liarsdice::ai` - AI strategies with a factory pattern
- **Configuration**: `liarsdice::config` - Hierarchical configuration system
- **Logging**: `liarsdice::logging` - Structured logging with spdlog
- **CLI Application**: Command-line interface using the library

## Build System

The project uses modern CMake with target-based configuration:

- **Main Targets**:
  - `liarsdice::core` - Main library
  - `liarsdice::warnings` - Compiler warnings interface
  - `liarsdice-cli` - CLI executable
  - `unit_tests` - Unit test executable
  - `basic_game` - Example executable

- **Compiler Features**:
  - C++23 standard required
  - IPO/LTO enabled for Release builds
  - Comprehensive warnings treated as errors
  - Address/UB sanitizers in Debug builds

## Submitting Changes

### Pull Request Process

1. **Before Submitting**:
   - Ensure all tests pass (`./scripts/test.sh`)
   - Run code formatting (`./scripts/format.sh`)
   - Run linting and fix any issues (`./scripts/lint.sh`)
   - Update documentation if needed
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
feat: Add medium difficulty AI strategy

Implement statistical analysis and pattern recognition
for more challenging gameplay. Includes configurable
parameters for risk tolerance and bluff detection.

Closes #123
```

## Getting Help

- Check existing issues and documentation
- Ask questions in discussions or issues
- Review the code style guide and examples
- Refer to the comprehensive test suite for usage patterns

## Additional Resources

- [README.md—](README.md)Project overview and usage
- [CMake Documentation](https://cmake.org/documentation/)
- [C++23 Reference](https://en.cppreference.com/)
- [Catch2 Documentation](https://github.com/catchorg/Catch2)
- [Robot Framework User Guide](https://robotframework.org/robotframework/latest/RobotFrameworkUserGuide.html)

Thank you for contributing to Liar's Dice!