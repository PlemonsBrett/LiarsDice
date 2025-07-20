# Create test executable
add_executable(unit_tests
    core/dice_test.cpp
    core/player_test.cpp
    core/game_test.cpp
    di/service_container_test.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        liarsdice::core
        liarsdice::warnings
        Catch2::Catch2WithMain
)

target_compile_features(unit_tests PRIVATE cxx_std_23)

# Discover tests
catch_discover_tests(unit_tests)