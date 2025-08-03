cmake_minimum_required(VERSION 3.14...3.28)

# ---- Project ----

project(
  LiarsDice
  VERSION 1.0
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Add dependencies via CPM ----
# see https://github.com/TheLartians/CPM.cmake for more info

include(cmake/CPM.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.8.0")

# Find system Boost or install via package manager
find_package(Boost 1.70.0 REQUIRED COMPONENTS
  log
  log_setup
  unit_test_framework
  program_options
  system
  filesystem
  serialization
  thread
        container
)

# Add Boost.DI (header-only library)
CPMAddPackage(
  NAME boost-di
  GITHUB_REPOSITORY boost-ext/di
  GIT_TAG v1.3.0
  DOWNLOAD_ONLY YES
)

# Add Boost.SIMD (header-only SIMD library)
CPMAddPackage(
        NAME boost-simd
        GITHUB_REPOSITORY agenium-scale/boost.simd
        GIT_TAG develop
        DOWNLOAD_ONLY YES
)

CPMAddPackage(
  NAME fmt
  GIT_TAG 10.2.1
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES" # create an installable target
)

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface
# target: add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

# Link dependencies
target_link_libraries(${PROJECT_NAME} 
  PRIVATE 
    fmt::fmt
    Boost::log
    Boost::log_setup
    Boost::program_options
    Boost::system
    Boost::filesystem
    Boost::serialization
    Boost::thread
        Boost::container
  PUBLIC
    Boost::boost
)

# Add Boost.DI include directory if available
if(boost-di_ADDED)
  target_include_directories(${PROJECT_NAME} PRIVATE ${boost-di_SOURCE_DIR}/include)
endif()

# Add Boost.SIMD include directory if available
if (boost-simd_ADDED)
  target_include_directories(${PROJECT_NAME} PUBLIC ${boost-simd_SOURCE_DIR}/include)
endif ()

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Create an installable target ----
# this allows users to install and find the library via `find_package()`.

# the location where the project's version header will be placed should match the project's regular
# header paths
string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  VERSION_HEADER "${VERSION_HEADER_LOCATION}"
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES "fmt 10.2.1"
)

# ---- Add subdirectories ----

add_subdirectory(standalone)
add_subdirectory(test)
