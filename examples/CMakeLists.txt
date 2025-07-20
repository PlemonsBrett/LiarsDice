add_executable(basic_game basic_game.cpp)

target_link_libraries(basic_game
    PRIVATE
        liarsdice::core
        liarsdice::warnings
)

target_compile_features(basic_game PRIVATE cxx_std_23)

# DI Example
add_executable(di_example di_example.cpp)

target_link_libraries(di_example
    PRIVATE
        liarsdice::core
        liarsdice::warnings
)

target_compile_features(di_example PRIVATE cxx_std_23)