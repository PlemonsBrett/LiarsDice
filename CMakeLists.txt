cmake_minimum_required(VERSION 3.26)

# Project Name
project(LiarsDice)

# C++ Standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Directories
include_directories(./include/controller)
include_directories(./include/model)

# Source files
set(SOURCES
    ./src/controller/Game.cpp
    ./src/model/Dice.cpp
    ./src/model/Player.cpp
    ./src/main.cpp
)

# Executable
add_executable(LiarsDice ${SOURCES})

# Copy assets to build directory
add_custom_command(TARGET LiarsDice POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:LiarsDice>/assets)
