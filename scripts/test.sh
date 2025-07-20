#!/bin/bash

set -e

# Script to run tests

BUILD_DIR="build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Please run ./scripts/build.sh first."
    exit 1
fi

echo "Running tests..."
ctest --test-dir "$BUILD_DIR" --output-on-failure

echo "All tests completed!"