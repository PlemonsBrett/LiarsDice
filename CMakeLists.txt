cmake_minimum_required(VERSION 3.21)

# Force use of Homebrew LLVM toolchain
set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++")

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

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cmake/StandardProjectSettings.cmake)
include(cmake/CompilerWarnings.cmake)

# Find packages if needed
if(LIARSDICE_BUILD_TESTS)
    find_package(Catch2 3 QUIET)
    if(NOT Catch2_FOUND)
        message(STATUS "Catch2 not found. Tests will use FetchContent.")
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

if(LIARSDICE_BUILD_DOCS)
    add_subdirectory(docs)
endif()

# Installation
if(LIARSDICE_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # Install the library
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    # Install interface library for warnings
    install(TARGETS liarsdice_warnings
        EXPORT LiarsDiceTargets
    )

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