#
# Format.cmake
# Provides code formatting capabilities using clang-format and cmake-format
#

# Adds a target that runs clang-format on all project headers and sources
function(add_clang_format_target)
  # Find all source and header files
  file(GLOB_RECURSE ALL_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/include/*.hpp
    ${PROJECT_SOURCE_DIR}/source/*.cpp
    ${PROJECT_SOURCE_DIR}/standalone/*.cpp
    ${PROJECT_SOURCE_DIR}/test/*.cpp
  )

  # Filter out build directories and external dependencies
  list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*build.*")
  list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*cpm_modules.*")
  list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*_deps.*")
  list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*venv.*")

  # Create format target
  add_custom_target(format
    COMMAND clang-format -i ${ALL_SOURCE_FILES}
    COMMENT "Running clang-format on all source files"
    VERBATIM
  )

  # Create check-format target
  add_custom_target(check-format
    COMMAND clang-format --dry-run --Werror ${ALL_SOURCE_FILES}
    COMMENT "Checking code formatting with clang-format"
    VERBATIM
  )
endfunction()

# Adds a target that runs cmake-format on all CMake files
function(add_cmake_format_target)
  # Find all CMake files
  file(GLOB_RECURSE CMAKE_FILES
    ${PROJECT_SOURCE_DIR}/CMakeLists.txt
    ${PROJECT_SOURCE_DIR}/*.cmake
    ${PROJECT_SOURCE_DIR}/cmake/*.cmake
    ${PROJECT_SOURCE_DIR}/test/CMakeLists.txt
    ${PROJECT_SOURCE_DIR}/standalone/CMakeLists.txt
    ${PROJECT_SOURCE_DIR}/standalone/*/CMakeLists.txt
    ${PROJECT_SOURCE_DIR}/documentation/CMakeLists.txt
  )

  # Filter out build directories
  list(FILTER CMAKE_FILES EXCLUDE REGEX ".*build.*")
  list(FILTER CMAKE_FILES EXCLUDE REGEX ".*cpm_modules.*")
  list(FILTER CMAKE_FILES EXCLUDE REGEX ".*_deps.*")

  # Create cmake-format target
  add_custom_target(cmake-format
    COMMAND cmake-format -i ${CMAKE_FILES}
    COMMENT "Running cmake-format on all CMake files"
    VERBATIM
  )

  # Create check-cmake-format target
  add_custom_target(check-cmake-format
    COMMAND cmake-format --check ${CMAKE_FILES}
    COMMENT "Checking CMake formatting"
    VERBATIM
  )
endfunction()

# Main formatting setup
if(NOT DEFINED FORMAT_SKIP_CMAKE)
  set(FORMAT_SKIP_CMAKE OFF)
endif()

if(NOT DEFINED FORMAT_SKIP_CLANG)
  set(FORMAT_SKIP_CLANG OFF)
endif()

# Add formatting targets
if(NOT FORMAT_SKIP_CLANG)
  add_clang_format_target()
endif()

if(NOT FORMAT_SKIP_CMAKE)
  add_cmake_format_target()
endif()

# Add combined check target for CI
if(TARGET check-format AND TARGET check-cmake-format)
  add_custom_target(check-all-format
    DEPENDS check-format check-cmake-format
    COMMENT "Checking all formatting"
  )
elseif(TARGET check-format)
  add_custom_target(check-all-format
    DEPENDS check-format
    COMMENT "Checking code formatting"
  )
elseif(TARGET check-cmake-format)
  add_custom_target(check-all-format
    DEPENDS check-cmake-format
    COMMENT "Checking CMake formatting"
  )
endif()