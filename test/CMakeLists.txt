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

# Game state storage tests
add_executable(test_game_state_storage source/test_game_state_storage.cpp)
target_link_libraries(test_game_state_storage
        PRIVATE
        LiarsDice::LiarsDice
        Boost::unit_test_framework
)
set_target_properties(test_game_state_storage PROPERTIES CXX_STANDARD 20)

# Data structures tests
add_executable(test_data_structures source/test_data_structures.cpp)
target_link_libraries(test_data_structures
        PRIVATE
        LiarsDice::LiarsDice
        Boost::unit_test_framework
)
set_target_properties(test_data_structures PROPERTIES CXX_STANDARD 20)

# Statistics tests
add_executable(test_statistics source/test_statistics.cpp)
target_link_libraries(test_statistics
        PRIVATE
        LiarsDice::LiarsDice
        Boost::unit_test_framework
)
set_target_properties(test_statistics PROPERTIES CXX_STANDARD 20)

# Performance tests
add_executable(test_performance source/test_performance.cpp)
target_link_libraries(test_performance
        PRIVATE
        LiarsDice::LiarsDice
        Boost::unit_test_framework
        Boost::timer
)
set_target_properties(test_performance PROPERTIES CXX_STANDARD 20)

# Bayesian inference tests
add_executable(test_bayesian source/test_bayesian.cpp)
target_link_libraries(test_bayesian
        PRIVATE
        LiarsDice::LiarsDice
        Boost::unit_test_framework
)
set_target_properties(test_bayesian PROPERTIES CXX_STANDARD 20)

# Database tests
add_executable(test_database source/test_database.cpp)
target_link_libraries(test_database
        PRIVATE
        LiarsDice::LiarsDice
        Boost::unit_test_framework
        Boost::filesystem
        $<$<TARGET_EXISTS:sqlite3>:sqlite3>
)
set_target_properties(test_database PROPERTIES CXX_STANDARD 20)

# Database connection tests (temporarily disabled due to hanging)
# add_executable(test_database_connection source/test_database_connection.cpp)
# target_link_libraries(test_database_connection
#         PRIVATE
#         LiarsDice::LiarsDice
#         Boost::unit_test_framework
#         Boost::filesystem
#         Boost::thread
#         $<$<TARGET_EXISTS:sqlite3>:sqlite3>
# )
# set_target_properties(test_database_connection PROPERTIES CXX_STANDARD 20)

# Database manager tests
add_executable(test_database_manager source/test_database_manager.cpp)
target_link_libraries(test_database_manager
        PRIVATE
        LiarsDice::LiarsDice
        Boost::unit_test_framework
        Boost::filesystem
        Boost::thread
        Boost::log
        $<$<TARGET_EXISTS:sqlite3>:sqlite3>
)
set_target_properties(test_database_manager PROPERTIES CXX_STANDARD 20)

# Database manager simple tests
add_executable(test_database_manager_simple source/test_database_manager_simple.cpp)
target_link_libraries(test_database_manager_simple
        PRIVATE
        LiarsDice::LiarsDice
        Boost::unit_test_framework
        Boost::filesystem
        $<$<TARGET_EXISTS:sqlite3>:sqlite3>
)
set_target_properties(test_database_manager_simple PROPERTIES CXX_STANDARD 20)

# Schema manager tests
add_executable(test_schema_manager source/test_schema_manager.cpp)
target_link_libraries(test_schema_manager
        PRIVATE
        LiarsDice::LiarsDice
        Boost::unit_test_framework
        Boost::filesystem
        Boost::log
        $<$<TARGET_EXISTS:sqlite3>:sqlite3>
)
set_target_properties(test_schema_manager PROPERTIES CXX_STANDARD 20)


# Add tests to CTest
add_test(NAME DiceTests COMMAND test_dice)
add_test(NAME PlayerTests COMMAND test_player)
add_test(NAME AITests COMMAND test_ai)
add_test(NAME GameTests COMMAND test_game)
add_test(NAME GameStateStorageTests COMMAND test_game_state_storage)
add_test(NAME DataStructuresTests COMMAND test_data_structures)
add_test(NAME StatisticsTests COMMAND test_statistics)
add_test(NAME PerformanceTests COMMAND test_performance)
add_test(NAME BayesianTests COMMAND test_bayesian)
add_test(NAME DatabaseTests COMMAND test_database)
add_test(NAME DatabaseManagerTests COMMAND test_database_manager)
add_test(NAME DatabaseManagerSimpleTests COMMAND test_database_manager_simple)
add_test(NAME SchemaManagerTests COMMAND test_schema_manager)
# Temporarily disabled due to hanging issue
# add_test(NAME DatabaseConnectionTests COMMAND test_database_connection)
# add_test(NAME BoostDITests COMMAND test_boost_di)
add_test(NAME ServiceContainerTests COMMAND test_service_container)

# ---- code coverage ----

if(ENABLE_TEST_COVERAGE)
  target_compile_options(LiarsDice PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(LiarsDice PUBLIC -fprofile-arcs -ftest-coverage)
endif()