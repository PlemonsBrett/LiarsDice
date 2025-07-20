#!/bin/bash

set -e

# Script to run tests with Catch2

BUILD_DIR="build"
TEST_EXECUTABLE="$BUILD_DIR/bin/unit_tests"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Please run ./scripts/build.sh first."
    exit 1
fi

if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Test executable not found. Please run ./scripts/build.sh first."
    exit 1
fi

# Parse command line arguments
FILTER=""
LIST_TESTS=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --filter)
            FILTER="$2"
            shift 2
            ;;
        --list)
            LIST_TESTS=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --filter <pattern>  Run tests matching pattern (e.g., '[dice]' or '*constructor*')"
            echo "  --list             List all available tests"
            echo "  --verbose          Run with verbose output"
            echo "  --help             Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                    # Run all tests"
            echo "  $0 --filter '[dice]' # Run dice tests only" 
            echo "  $0 --list            # List all test names"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

if [ "$LIST_TESTS" = true ]; then
    echo "Available tests:"
    "$TEST_EXECUTABLE" --list-tests
    exit 0
fi

echo "Running tests..."

if [ -n "$FILTER" ]; then
    echo "Filtering tests with: $FILTER"
    if [ "$VERBOSE" = true ]; then
        "$TEST_EXECUTABLE" "$FILTER" -v
    else
        "$TEST_EXECUTABLE" "$FILTER"
    fi
else
    # Run via ctest for full test discovery and reporting
    if [ "$VERBOSE" = true ]; then
        ctest --test-dir "$BUILD_DIR" --output-on-failure --verbose
    else
        ctest --test-dir "$BUILD_DIR" --output-on-failure
    fi
fi

echo "All tests completed!"