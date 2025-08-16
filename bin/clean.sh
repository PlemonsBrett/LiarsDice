#!/bin/bash
# Clean script for LiarsDice

echo "Cleaning build artifacts..."

# Remove build directory
rm -rf build/

# Remove Python cache
find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
find . -type f -name "*.pyc" -delete 2>/dev/null || true

# Remove Robot Framework results
rm -rf test/robot/results/

echo "Clean complete!"