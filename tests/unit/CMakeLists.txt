# Create test executable
add_executable(unit_tests
    test_main.cpp
    core/dice_test.cpp
    core/player_test.cpp
    core/game_test.cpp
    di/service_container_test.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        liarsdice::core
        liarsdice::warnings
        gtest
        gtest_main
)

target_compile_features(unit_tests PRIVATE cxx_std_23)

# Discover tests
gtest_discover_tests(unit_tests)