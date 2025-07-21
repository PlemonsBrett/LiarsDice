cmake_minimum_required(VERSION 3.28)

# Force use of Homebrew LLVM toolchain
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=c++23")

# Project definition
project(LiarsDice 
    VERSION 1.0.0
    DESCRIPTION "A modern C++23 implementation of Liar's Dice"
    LANGUAGES CXX)

# Guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Options
option(LIARSDICE_BUILD_TESTS "Build the test suite" ON)
option(LIARSDICE_BUILD_EXAMPLES "Build example programs" ON)
option(LIARSDICE_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(LIARSDICE_BUILD_DOCS "Build documentation" OFF)
option(LIARSDICE_INSTALL "Generate installation target" ON)
option(LIARSDICE_USE_SANITIZERS "Enable sanitizers in debug builds" ON)
option(LIARSDICE_ENABLE_LOGGING "Enable comprehensive logging with spdlog" ON)
option(LIARSDICE_ENABLE_CONFIG "Enable modern C++23 configuration system" ON)

# Logging configuration
set(LIARSDICE_LOG_LEVEL "INFO" CACHE STRING "Set compile-time log level")
set_property(CACHE LIARSDICE_LOG_LEVEL PROPERTY STRINGS 
    "TRACE" "DEBUG" "INFO" "WARN" "ERROR" "CRITICAL" "OFF")

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cmake/StandardProjectSettings.cmake)
include(cmake/CompilerWarnings.cmake)
include(cmake/Logging.cmake)

# Documentation support
if(LIARSDICE_BUILD_DOCS)
    include(cmake/Documentation.cmake)
endif()

# Conan integration (if available)
if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conan_toolchain.cmake" OR 
   EXISTS "${CMAKE_CURRENT_BINARY_DIR}/Release/generators/conan_toolchain.cmake" OR
   EXISTS "${CMAKE_CURRENT_BINARY_DIR}/Debug/generators/conan_toolchain.cmake")
    message(STATUS "Using Conan for dependency management")
    set(USING_CONAN TRUE)
else()
    message(STATUS "Conan not detected, using FetchContent for dependencies")
    set(USING_CONAN FALSE)
endif()

# Find packages if needed
if(LIARSDICE_ENABLE_LOGGING OR LIARSDICE_ENABLE_CONFIG)
    if(USING_CONAN)
        if(LIARSDICE_ENABLE_LOGGING)
            find_package(spdlog REQUIRED)
        endif()
        find_package(nlohmann_json REQUIRED)
        message(STATUS "Using dependencies from Conan")
    else()
        if(LIARSDICE_ENABLE_LOGGING)
            find_package(spdlog QUIET)
        endif()
        find_package(nlohmann_json QUIET)
        
        set(NEED_FETCH_CONTENT FALSE)
        if(LIARSDICE_ENABLE_LOGGING AND NOT spdlog_FOUND)
            set(NEED_FETCH_CONTENT TRUE)
        endif()
        if(NOT nlohmann_json_FOUND)
            set(NEED_FETCH_CONTENT TRUE)
        endif()
        
        if(NEED_FETCH_CONTENT)
            message(STATUS "Some dependencies not found. Using FetchContent.")
            include(FetchContent)
            
            if(LIARSDICE_ENABLE_LOGGING AND NOT spdlog_FOUND)
                FetchContent_Declare(
                    spdlog
                    GIT_REPOSITORY https://github.com/gabime/spdlog.git
                    GIT_TAG v1.12.0
                )
                FetchContent_MakeAvailable(spdlog)
            endif()
            
            if(NOT nlohmann_json_FOUND)
                FetchContent_Declare(
                    nlohmann_json
                    GIT_REPOSITORY https://github.com/nlohmann/json.git
                    GIT_TAG v3.11.2
                )
                FetchContent_MakeAvailable(nlohmann_json)
            endif()
        endif()
    endif()
endif()

if(LIARSDICE_BUILD_TESTS)
    if(USING_CONAN)
        find_package(Catch2 REQUIRED)
        message(STATUS "Using Catch2 from Conan")
    else()
        find_package(Catch2 3 QUIET)
        if(NOT Catch2_FOUND)
            message(STATUS "Catch2 not found. Tests will use FetchContent.")
        endif()
    endif()
endif()

# Interface library for common settings
add_library(liarsdice_warnings INTERFACE)
add_library(liarsdice::warnings ALIAS liarsdice_warnings)
set_project_warnings(liarsdice_warnings)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(apps)

if(LIARSDICE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(LIARSDICE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(LIARSDICE_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Documentation is handled by cmake/Documentation.cmake
# No need for docs subdirectory

# Installation
if(LIARSDICE_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # Install the library
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    # Install interface library for warnings
    if(TARGET liarsdice_warnings)
        install(TARGETS liarsdice_warnings
            EXPORT LiarsDiceTargets
        )
    endif()

    # Generate and install CMake config files
    install(EXPORT LiarsDiceTargets
        FILE LiarsDiceTargets.cmake
        NAMESPACE liarsdice::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LiarsDice
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LiarsDiceConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/LiarsDiceConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LiarsDice
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/LiarsDiceConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/LiarsDiceConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/LiarsDiceConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LiarsDice
    )
endif()

# Package configuration
include(CPack)
set(CPACK_PACKAGE_VENDOR "LiarsDice")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Status messages
print_logging_status()

message(STATUS "")
message(STATUS "LiarsDice configuration summary:")
message(STATUS "  Version:          ${PROJECT_VERSION}")
message(STATUS "  Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ compiler:     ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ standard:     ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "  Build tests:      ${LIARSDICE_BUILD_TESTS}")
message(STATUS "  Build examples:   ${LIARSDICE_BUILD_EXAMPLES}")
message(STATUS "  Build benchmarks: ${LIARSDICE_BUILD_BENCHMARKS}")
message(STATUS "  Build docs:       ${LIARSDICE_BUILD_DOCS}")
message(STATUS "  Install:          ${LIARSDICE_INSTALL}")
message(STATUS "")