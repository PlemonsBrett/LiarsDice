#!/bin/bash
# Run the LiarsDice game

# Build if needed
if [ ! -f "build/standalone/liarsdice" ]; then
    echo "Executable not found. Building first..."
    ./build.sh
fi

# Run the game
./build/standalone/liarsdice "$@"