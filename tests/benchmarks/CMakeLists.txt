# Benchmark executable
add_executable(validation_benchmarks
    validation_benchmark.cpp
)

# Link libraries
target_link_libraries(validation_benchmarks
    PRIVATE
        liarsdice::core
        liarsdice::warnings
        Catch2::Catch2WithMain
)

target_compile_features(validation_benchmarks PRIVATE cxx_std_23)

# Define benchmark macro for Catch2
target_compile_definitions(validation_benchmarks PRIVATE CATCH_CONFIG_ENABLE_BENCHMARKING)

# Register with CTest
catch_discover_tests(validation_benchmarks)