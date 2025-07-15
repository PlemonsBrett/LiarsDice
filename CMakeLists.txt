# Minimum required CMake version
cmake_minimum_required(VERSION 3.26)

# Project name
project(LiarsDice)

# Set C++ standard and make it required
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories - use the main include directory
include_directories(./include)

# List of source files
set(SOURCES
        ./src/controller/Game.cpp
        ./src/model/Dice.cpp
        ./src/model/Player.cpp
        ./src/main.cpp
)

# Define the executable and link it with the source files
add_executable(LiarsDice ${SOURCES})

# Custom command to copy assets to the build directory after building the project
add_custom_command(TARGET LiarsDice POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:LiarsDice>/assets)`