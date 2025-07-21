#!/bin/bash

set -e

# Script to build the LiarsDice project

BUILD_TYPE=${1:-Release}
USE_CONAN="${2:-auto}"  # auto, yes, no
BUILD_DIR="build"

echo "Building LiarsDice (${BUILD_TYPE})..."

# Check if we should use Conan
if [ "$USE_CONAN" = "auto" ]; then
    if command -v conan &> /dev/null && [ -f "conanfile.py" ]; then
        CONAN_VERSION=$(conan --version 2>/dev/null | head -n1 | grep -o '[0-9]\+\.[0-9]\+' | head -n1)
        if [ -n "$CONAN_VERSION" ] && [ "$(echo "$CONAN_VERSION" | cut -d. -f1)" -ge 2 ]; then
            USE_CONAN="yes"
            echo "Conan 2.0+ detected, using Conan for dependencies"
        else
            USE_CONAN="no"
            echo "Conan version < 2.0 detected, using FetchContent for dependencies"
        fi
    else
        USE_CONAN="no"
        echo "Conan not available, using FetchContent for dependencies"
    fi
fi

# Clean build directory if it exists
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

if [ "$USE_CONAN" = "yes" ]; then
    echo "Using Conan build script..."
    ./scripts/build-conan.sh "$BUILD_TYPE"
else
    echo "Using traditional CMake build..."
    
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
fi