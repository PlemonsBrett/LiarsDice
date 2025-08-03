#!/bin/bash
# Build script for LiarsDice

set -e

# Default build type
BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"

echo "Building LiarsDice (${BUILD_TYPE})..."

# Create build directory if it doesn't exist
mkdir -p ${BUILD_DIR}

# Configure
echo "Configuring CMake..."
cmake -B ${BUILD_DIR} -S . -DCMAKE_BUILD_TYPE=${BUILD_TYPE}

# Build
echo "Building..."
cmake --build ${BUILD_DIR} -j

echo "Build complete! Executable is at: ${BUILD_DIR}/standalone/liarsdice"