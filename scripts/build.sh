#!/bin/bash

set -e

# Script to build the LiarsDice project

BUILD_TYPE=${1:-Release}
BUILD_DIR="build"

echo "Building LiarsDice (${BUILD_TYPE})..."

# Clean build directory if it exists
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

# Configure
echo "Configuring CMake..."
cmake -B "$BUILD_DIR" -S . \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo "Building..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Build complete! Executables are in $BUILD_DIR/bin/"
echo ""
echo "To run the game: ./$BUILD_DIR/bin/liarsdice-cli"
echo "To run tests: ctest --test-dir $BUILD_DIR"
echo "To run example: ./$BUILD_DIR/bin/basic_game"