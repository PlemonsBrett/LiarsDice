#!/bin/bash

set -e

# Script to build with Conan 2.0

BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"
CONAN_PROFILE="profiles/default"

echo "Building LiarsDice with Conan ($BUILD_TYPE)..."

# Check if Conan is installed
if ! command -v conan &> /dev/null; then
    echo "Error: Conan is not installed. Please install Conan 2.0:"
    echo "  pip install conan"
    echo "  or"
    echo "  brew install conan"
    exit 1
fi

# Check Conan version
CONAN_VERSION=$(conan --version 2>/dev/null | head -n1 | grep -o '[0-9]\+\.[0-9]\+' | head -n1)
MAJOR_VERSION=$(echo "$CONAN_VERSION" | cut -d. -f1)
if [ -z "$CONAN_VERSION" ] || [ "$MAJOR_VERSION" -lt 2 ]; then
    echo "Error: This project requires Conan 2.0 or later. Found version: ${CONAN_VERSION:-unknown}"
    echo "Please upgrade Conan:"
    echo "  pip install --upgrade conan"
    exit 1
fi

# Create build directory if it doesn't exist
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

# Install dependencies with Conan
echo "Installing dependencies with Conan..."
cd "$BUILD_DIR"

if [ -f "../${CONAN_PROFILE}" ]; then
    echo "Using custom profile: ${CONAN_PROFILE}"
    conan install .. --build=missing -s build_type="$BUILD_TYPE" --profile="../${CONAN_PROFILE}"
else
    echo "Using default profile"
    conan install .. --build=missing -s build_type="$BUILD_TYPE"
fi

# Build with CMake
echo "Configuring CMake..."
cmake .. -DCMAKE_TOOLCHAIN_FILE="$BUILD_TYPE/generators/conan_toolchain.cmake" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo "Building..."
cmake --build . -j

cd ..

echo "Build complete! Executables are in build/bin/"
echo ""
echo "To run the game: ./build/bin/liarsdice-cli"
echo "To run tests: ctest --test-dir build"
echo "To run example: ./build/bin/basic_game"