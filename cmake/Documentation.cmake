# Documentation.cmake
# CMake module for building project documentation

# Find required documentation tools
find_package(Doxygen QUIET)
find_program(SPHINX_BUILD_EXECUTABLE 
    NAMES sphinx-build
    DOC "Sphinx documentation builder"
)

# Set documentation directories
set(DOCS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs")
set(DOCS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs")
set(DOXYGEN_OUTPUT_DIR "${DOCS_OUTPUT_DIR}/api")
set(SPHINX_SOURCE_DIR "${DOCS_SOURCE_DIR}/sphinx")
set(SPHINX_OUTPUT_DIR "${DOCS_OUTPUT_DIR}/html")

# Configure Doxygen
if(DOXYGEN_FOUND)
    message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")
    
    # Set Doxygen configuration variables
    set(DOXYGEN_PROJECT_NAME "LiarsDice Game Engine")
    set(DOXYGEN_PROJECT_NUMBER "${PROJECT_VERSION}")
    set(DOXYGEN_PROJECT_BRIEF "Modern C++23 implementation of Liar's Dice with dependency injection")
    set(DOXYGEN_OUTPUT_DIRECTORY "${DOXYGEN_OUTPUT_DIR}")
    set(DOXYGEN_INPUT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/apps ${CMAKE_CURRENT_SOURCE_DIR}/examples")
    set(DOXYGEN_EXCLUDE_PATTERNS "*/build/* */conan/* */.git/* */tests/* */CMakeFiles/*")
    
    # Create output directory
    file(MAKE_DIRECTORY "${DOXYGEN_OUTPUT_DIR}")
    
    # Configure Doxyfile from template
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile"
        "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        @ONLY
    )
    
    # Add Doxygen target
    add_custom_target(doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    
    # Source files are automatically tracked by Doxygen based on INPUT_DIRS
    
    message(STATUS "Doxygen documentation target added")
else()
    message(STATUS "Doxygen not found - API documentation will not be built")
endif()

# Configure Sphinx
if(SPHINX_BUILD_EXECUTABLE)
    message(STATUS "Sphinx found: ${SPHINX_BUILD_EXECUTABLE}")
    
    # Create output directory
    file(MAKE_DIRECTORY "${SPHINX_OUTPUT_DIR}")
    
    # Find all RST source files
    file(GLOB_RECURSE SPHINX_RST_FILES "${SPHINX_SOURCE_DIR}/**/*.rst")
    file(GLOB_RECURSE SPHINX_CONFIG_FILES "${SPHINX_SOURCE_DIR}/conf.py" "${SPHINX_SOURCE_DIR}/_static/*" "${SPHINX_SOURCE_DIR}/_templates/*")
    
    # Add Sphinx HTML target
    add_custom_target(sphinx-html
        COMMAND ${SPHINX_BUILD_EXECUTABLE} 
                -b html 
                "${SPHINX_SOURCE_DIR}" 
                "${SPHINX_OUTPUT_DIR}"
        WORKING_DIRECTORY "${SPHINX_SOURCE_DIR}"
        DEPENDS ${SPHINX_RST_FILES} ${SPHINX_CONFIG_FILES}
        COMMENT "Building HTML documentation with Sphinx"
        VERBATIM
    )
    
    # Add Sphinx PDF target (if LaTeX is available)
    find_program(LATEX_EXECUTABLE NAMES pdflatex)
    if(LATEX_EXECUTABLE)
        add_custom_target(sphinx-pdf
            COMMAND ${SPHINX_BUILD_EXECUTABLE} 
                    -b latex 
                    "${SPHINX_SOURCE_DIR}" 
                    "${DOCS_OUTPUT_DIR}/latex"
            COMMAND ${CMAKE_COMMAND} -E chdir "${DOCS_OUTPUT_DIR}/latex" 
                    make
            WORKING_DIRECTORY "${SPHINX_SOURCE_DIR}"
            DEPENDS ${SPHINX_RST_FILES} ${SPHINX_CONFIG_FILES}
            COMMENT "Building PDF documentation with Sphinx"
            VERBATIM
        )
        message(STATUS "Sphinx PDF documentation target added")
    endif()
    
    message(STATUS "Sphinx HTML documentation target added")
else()
    message(STATUS "Sphinx not found - user documentation will not be built")
endif()

# Create combined documentation target
if(DOXYGEN_FOUND AND SPHINX_BUILD_EXECUTABLE)
    add_custom_target(docs
        COMMENT "Building all documentation"
    )
    
    # Make docs depend on both Doxygen and Sphinx
    add_dependencies(docs doxygen sphinx-html)
    
    message(STATUS "Combined documentation target 'docs' added")
    
    # Create install target for documentation
    if(LIARSDICE_INSTALL)
        install(
            DIRECTORY "${DOCS_OUTPUT_DIR}/"
            DESTINATION "${CMAKE_INSTALL_DOCDIR}"
            OPTIONAL
        )
        message(STATUS "Documentation install target configured")
    endif()
    
elseif(DOXYGEN_FOUND)
    add_custom_target(docs
        DEPENDS doxygen
        COMMENT "Building API documentation only (Sphinx not available)"
    )
    message(STATUS "API-only documentation target 'docs' added")
    
elseif(SPHINX_BUILD_EXECUTABLE)
    add_custom_target(docs
        DEPENDS sphinx-html
        COMMENT "Building user documentation only (Doxygen not available)"
    )
    message(STATUS "User-only documentation target 'docs' added")
    
else()
    message(STATUS "Neither Doxygen nor Sphinx found - no documentation targets available")
endif()

# Add documentation to clean target
set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    "${DOCS_OUTPUT_DIR}"
)

# Helper function to add documentation dependencies
function(add_documentation_dependency target)
    if(TARGET docs)
        add_dependencies(${target} docs)
    endif()
endfunction()

# Print documentation status
message(STATUS "")
message(STATUS "Documentation Configuration:")
message(STATUS "  Source directory: ${DOCS_SOURCE_DIR}")
message(STATUS "  Output directory: ${DOCS_OUTPUT_DIR}")
if(DOXYGEN_FOUND)
    message(STATUS "  Doxygen API docs: ${DOXYGEN_OUTPUT_DIR}")
endif()
if(SPHINX_BUILD_EXECUTABLE)
    message(STATUS "  Sphinx HTML docs: ${SPHINX_OUTPUT_DIR}")
endif()
message(STATUS "")