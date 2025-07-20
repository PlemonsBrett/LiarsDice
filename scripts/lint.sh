#!/bin/bash

# Script to run clang-tidy linting on the codebase
# Usage: ./scripts/lint.sh [--fix]

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Running build first..."
    "$SCRIPT_DIR/build.sh"
fi

# Check if compile_commands.json exists
if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "compile_commands.json not found. Regenerating..."
    cd "$PROJECT_ROOT"
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
fi

# Determine if we should apply fixes
FIX_FLAG=""
if [ "$1" == "--fix" ]; then
    FIX_FLAG="--fix"
    echo "Running clang-tidy with automatic fixes..."
else
    echo "Running clang-tidy (dry run)..."
fi

# Change to project root
cd "$PROJECT_ROOT"

# Run clang-tidy on all source files
echo "Analyzing source files..."
find src -name "*.cpp" -o -name "*.hpp" | xargs /opt/homebrew/opt/llvm/bin/clang-tidy $FIX_FLAG -p build

echo "Analyzing app files..."
find apps -name "*.cpp" -o -name "*.hpp" | xargs /opt/homebrew/opt/llvm/bin/clang-tidy $FIX_FLAG -p build

echo "Analyzing example files..."
find examples -name "*.cpp" -o -name "*.hpp" | xargs /opt/homebrew/opt/llvm/bin/clang-tidy $FIX_FLAG -p build

if [ "$1" == "--fix" ]; then
    echo "Linting complete with automatic fixes applied!"
else
    echo "Linting complete! Use './scripts/lint.sh --fix' to apply automatic fixes."
fi