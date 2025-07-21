# Create test executable with basic tests
add_executable(unit_tests
    core/dice_test.cpp
    core/player_test.cpp
    core/game_test.cpp
    di/service_container_test.cpp
)

# Advanced test suite with modern C++23 features
add_executable(advanced_tests
    core/dice_test_advanced.cpp
    core/player_test_advanced.cpp
    integration/game_integration_test.cpp
    test_infrastructure.hpp
)

# Link libraries for basic tests
target_link_libraries(unit_tests
    PRIVATE
        liarsdice::core
        liarsdice::warnings
        Catch2::Catch2WithMain
)

# Link libraries for advanced tests
target_link_libraries(advanced_tests
    PRIVATE
        liarsdice::core
        liarsdice::warnings
        Catch2::Catch2WithMain
)

target_compile_features(unit_tests PRIVATE cxx_std_23)
target_compile_features(advanced_tests PRIVATE cxx_std_23)

# Discover tests for both executables
catch_discover_tests(unit_tests)
catch_discover_tests(advanced_tests)