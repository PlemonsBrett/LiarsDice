cmake_minimum_required(VERSION 3.14...3.28)

project(LiarsDiceStandalone LANGUAGES CXX)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

# When building standalone, check if parent project is available
if(NOT TARGET LiarsDice::LiarsDice)
  # Add the main library as a dependency
  CPMAddPackage(NAME LiarsDice SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

# ---- Create standalone executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(${PROJECT_NAME} ${sources})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 OUTPUT_NAME "liarsdice")

target_link_libraries(${PROJECT_NAME} 
  PRIVATE 
    LiarsDice::LiarsDice
)