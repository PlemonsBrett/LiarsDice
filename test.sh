#!/bin/bash
# Test script for LiarsDice

set -e

# Build first if needed
if [ ! -d "build" ]; then
    echo "Build directory not found. Building first..."
    ./build.sh
fi

echo "Running unit tests..."

# Run all unit tests
cd build/test
for test in test_*; do
    if [ -f "$test" ] && [ -x "$test" ]; then
        echo "Running $test..."
        ./$test
    fi
done

cd ../..

echo ""
echo "Running Robot Framework tests..."
./test/robot/run_tests.sh

echo ""
echo "All tests complete!"