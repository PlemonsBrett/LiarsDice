cmake_minimum_required(VERSION 3.14...3.28)

project(LiarsDiceTests LANGUAGES CXX)

# ---- Options ----

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

# Parent project already provides LiarsDice target
if(TEST_INSTALLED_VERSION)
  find_package(LiarsDice REQUIRED)
  find_package(Boost 1.70.0 REQUIRED COMPONENTS unit_test_framework)
endif()

# ---- Create test executables ----

enable_testing()

# Core tests
add_executable(test_dice source/test_dice.cpp)
target_link_libraries(test_dice 
  PRIVATE 
    LiarsDice::LiarsDice
    Boost::unit_test_framework
)
set_target_properties(test_dice PROPERTIES CXX_STANDARD 20)

# Player tests
add_executable(test_player source/test_player.cpp)
target_link_libraries(test_player 
  PRIVATE 
    LiarsDice::LiarsDice
    Boost::unit_test_framework
)
set_target_properties(test_player PROPERTIES CXX_STANDARD 20)

# AI tests
add_executable(test_ai source/test_ai.cpp)
target_link_libraries(test_ai 
  PRIVATE 
    LiarsDice::LiarsDice
    Boost::unit_test_framework
)
set_target_properties(test_ai PROPERTIES CXX_STANDARD 20)

# Boost.DI tests (commented out for now due to include issues)
# add_executable(test_boost_di source/test_boost_di.cpp)
# target_link_libraries(test_boost_di 
#   PRIVATE 
#     LiarsDice::LiarsDice
#     Boost::unit_test_framework
# )
# set_target_properties(test_boost_di PROPERTIES CXX_STANDARD 20)

# Service container tests
add_executable(test_service_container source/test_service_container.cpp)
target_link_libraries(test_service_container 
  PRIVATE 
    LiarsDice::LiarsDice
    Boost::unit_test_framework
)
set_target_properties(test_service_container PROPERTIES CXX_STANDARD 20)

# Game tests
add_executable(test_game source/test_game.cpp)
target_link_libraries(test_game 
  PRIVATE 
    LiarsDice::LiarsDice
    Boost::unit_test_framework
)
set_target_properties(test_game PROPERTIES CXX_STANDARD 20)

# Add tests to CTest
add_test(NAME DiceTests COMMAND test_dice)
add_test(NAME PlayerTests COMMAND test_player)
add_test(NAME AITests COMMAND test_ai)
add_test(NAME GameTests COMMAND test_game)
# add_test(NAME BoostDITests COMMAND test_boost_di)
add_test(NAME ServiceContainerTests COMMAND test_service_container)

# ---- code coverage ----

if(ENABLE_TEST_COVERAGE)
  target_compile_options(LiarsDice PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(LiarsDice PUBLIC -fprofile-arcs -ftest-coverage)
endif()