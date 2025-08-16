cmake_minimum_required(VERSION 3.14...3.28)

project(DatabaseDemo LANGUAGES CXX)

# ---- Create standalone executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Check if target already exists to avoid conflicts
if (NOT TARGET ${PROJECT_NAME})
    add_executable(${PROJECT_NAME} ${sources})
endif ()

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 OUTPUT_NAME "database-demo")

# Link to the main library which should be available from parent
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        LiarsDice::LiarsDice
        ${Boost_LOG_LIBRARY}
        ${Boost_LOG_SETUP_LIBRARY}
)

# Add explicit dependency on sqlite3 if it exists
if(TARGET sqlite3)
    add_dependencies(${PROJECT_NAME} sqlite3)
endif()