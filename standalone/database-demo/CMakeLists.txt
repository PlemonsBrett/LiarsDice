cmake_minimum_required(VERSION 3.14...3.28)

project(DatabaseDemo LANGUAGES CXX)

# ---- Create standalone executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${sources})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 OUTPUT_NAME "database-demo")

# Link to the main library which should be available from parent
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        LiarsDice::LiarsDice
        Boost::log
        Boost::filesystem
        Boost::thread
)