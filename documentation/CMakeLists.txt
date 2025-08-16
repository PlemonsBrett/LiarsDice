cmake_minimum_required(VERSION 3.14...3.28)

project(LiarsDiceDocs)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

CPMAddPackage("gh:mosra/m.css#a0d292ec311b97fefd21e93cdefb60f88d19ede6")

# Only add LiarsDice package if not already defined (i.e., when building docs standalone)
if (NOT TARGET LiarsDice)
    CPMAddPackage(NAME LiarsDice SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif ()

# ---- Doxygen variables ----

# set Doxyfile variables
set(DOXYGEN_PROJECT_NAME "Liar's Dice")
set(DOXYGEN_PROJECT_VERSION ${LiarsDice_VERSION})
set(DOXYGEN_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doxygen")

configure_file(${CMAKE_CURRENT_LIST_DIR}/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

configure_file(${CMAKE_CURRENT_LIST_DIR}/conf.py ${CMAKE_CURRENT_BINARY_DIR}/conf.py)

# Check if GenerateDocs target already exists
if (NOT TARGET GenerateDocs)
    add_custom_target(
            GenerateDocs
            ${CMAKE_COMMAND} -E make_directory "${DOXYGEN_OUTPUT_DIRECTORY}"
            COMMAND "${m.css_SOURCE_DIR}/documentation/doxygen.py" "${CMAKE_CURRENT_BINARY_DIR}/conf.py"
            COMMAND echo "Docs written to: ${DOXYGEN_OUTPUT_DIRECTORY}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
endif ()