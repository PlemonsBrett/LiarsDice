# Find or fetch Catch2
if(USING_CONAN)
    # Conan already found Catch2 in main CMakeLists.txt
    message(STATUS "Using Catch2 from Conan")
else()
    find_package(Catch2 3 QUIET)

    if(NOT Catch2_FOUND)
        message(STATUS "Catch2 not found. Fetching from GitHub...")
        include(FetchContent)
        FetchContent_Declare(
            catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG        v3.4.0
        )
        FetchContent_MakeAvailable(catch2)
    endif()
endif()

# Enable testing
enable_testing()
include(Catch)

# Add subdirectories
add_subdirectory(unit)

# Integration tests (if we have any)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/CMakeLists.txt)
    add_subdirectory(integration)
endif()