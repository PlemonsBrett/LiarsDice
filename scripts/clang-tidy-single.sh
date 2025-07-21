#!/bin/bash

# Simple wrapper to run clang-tidy on a single file
# Usage: ./scripts/clang-tidy-single.sh <file>

set -e

if [ $# -eq 0 ]; then
    echo "Usage: $0 <file>"
    exit 1
fi

FILE="$1"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
CLANG_TIDY="/opt/homebrew/opt/llvm/bin/clang-tidy"

# Ensure build directory exists
if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "Error: compile_commands.json not found. Run ./scripts/build.sh first."
    exit 1
fi

# Run clang-tidy with filtered output
echo "Running clang-tidy on $FILE..."
"$CLANG_TIDY" \
    -p "$BUILD_DIR" \
    "$FILE" \
    2>&1 | grep -E "^${PROJECT_ROOT}" || echo "No warnings found for project files."