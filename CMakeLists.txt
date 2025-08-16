cmake_minimum_required(VERSION 3.14...3.28)

# ---- Project ----

project(
  LiarsDice
  VERSION 1.0
  LANGUAGES CXX C
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
  timer
  iostreams
  regex
)

# Add Boost.DI (header-only library)
CPMAddPackage(
  NAME boost-di
  GITHUB_REPOSITORY boost-ext/di
  GIT_TAG v1.3.0
  DOWNLOAD_ONLY YES
)

# Add xsimd (modern SIMD library)
CPMAddPackage(
        NAME xsimd
        GITHUB_REPOSITORY xtensor-stack/xsimd
        GIT_TAG 13.0.0
        OPTIONS "BUILD_TESTS OFF"
)

CPMAddPackage(
  NAME fmt
  GIT_TAG 10.2.1
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES" # create an installable target
)

# Add SQLite3 for database support (version 3.47.2 - latest as of Dec 2024)
CPMAddPackage(
  NAME sqlite3
  URL https://www.sqlite.org/2024/sqlite-amalgamation-3470200.zip
  URL_HASH SHA256=aa73d8748095808471deaa8e6f34aa700e37f2f787f4425744f53fdd15a89c40
  DOWNLOAD_ONLY YES
)

if(sqlite3_ADDED)
  # Create SQLite3 library target
  add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
  target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
  
  # Enable SQLite3 features
  target_compile_definitions(sqlite3 
    PUBLIC 
      SQLITE_ENABLE_FTS5          # Full-text search
      SQLITE_ENABLE_JSON1         # JSON support
      SQLITE_ENABLE_RTREE         # R-tree support
      SQLITE_THREADSAFE=2         # Multi-threaded mode
      SQLITE_ENABLE_LOAD_EXTENSION # Dynamic extension loading
  )
  
  # Link with Boost.Thread for thread safety
  target_link_libraries(sqlite3 PRIVATE Boost::thread)
  
  # Platform-specific flags
  if(WIN32)
    target_link_libraries(sqlite3 PRIVATE ws2_32)
  else()
    target_link_libraries(sqlite3 PRIVATE pthread dl)
  endif()
endif()

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
    Boost::iostreams
    Boost::regex
    $<$<TARGET_EXISTS:sqlite3>:sqlite3>
  PUBLIC
    Boost::boost
)

# Add Boost.DI include directory if available
if(boost-di_ADDED)
  target_include_directories(${PROJECT_NAME} PRIVATE ${boost-di_SOURCE_DIR}/include)
endif()

# Add xsimd if available
if (xsimd_ADDED)
  target_link_libraries(${PROJECT_NAME} PUBLIC xsimd)
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
  DEPENDENCIES "fmt 10.2.1" "xsimd 13.0.0"
)

# ---- Add subdirectories ----

# ---- Options ----
option(LIARSDICE_BUILD_TESTS "Build tests" ON)
option(LIARSDICE_BUILD_DOCS "Build documentation" OFF)
option(LIARSDICE_BUILD_EXAMPLES "Build examples" ON)

if (LIARSDICE_BUILD_TESTS)
  add_subdirectory(test)
endif ()

# Only add standalone when building the main project
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_subdirectory(standalone)
endif ()

# ---- Documentation ----
if (LIARSDICE_BUILD_DOCS)
  add_subdirectory(documentation)
endif ()
