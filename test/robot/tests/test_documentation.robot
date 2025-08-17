*** Settings ***
Documentation     Test suite for validating Doxygen documentation
Library           OperatingSystem
Library           Collections
Library           String
Library           Process
Library           ../lib/DocumentationLibrary.py

Suite Setup       Build Documentation
Suite Teardown    Cleanup Documentation

*** Variables ***
${PROJECT_ROOT}        ${CURDIR}/../../..
${BUILD_DIR}           ${PROJECT_ROOT}/build_docs
${DOCS_HTML_DIR}       ${BUILD_DIR}/documentation/doxygen/html

*** Test Cases ***
Documentation Builds Successfully
    [Documentation]    Verify that Doxygen documentation builds without errors
    Directory Should Exist    ${DOCS_HTML_DIR}
    File Should Exist         ${DOCS_HTML_DIR}/index.html
    
Main Documentation Pages Exist
    [Documentation]    Verify that main documentation pages are generated
    File Should Exist    ${DOCS_HTML_DIR}/index.html
    File Should Exist    ${DOCS_HTML_DIR}/annotated.html
    File Should Exist    ${DOCS_HTML_DIR}/classes.html
    File Should Exist    ${DOCS_HTML_DIR}/namespaces.html
    File Should Exist    ${DOCS_HTML_DIR}/files.html

Core Module Documentation Exists
    [Documentation]    Verify core module documentation is generated
    File Should Exist    ${DOCS_HTML_DIR}/group__core.html
    ${content}=    Get File    ${DOCS_HTML_DIR}/group__core.html
    Should Contain    ${content}    Game
    Should Contain    ${content}    Player
    Should Contain    ${content}    Dice

AI Module Documentation Exists
    [Documentation]    Verify AI module documentation is generated
    ${files}=    List Files In Directory    ${DOCS_HTML_DIR}    pattern=*ai*.html
    Should Not Be Empty    ${files}    AI documentation files not found

Database Documentation Page Exists
    [Documentation]    Verify database documentation page is generated
    ${files}=    List Files In Directory    ${DOCS_HTML_DIR}    pattern=*database*.html
    Should Not Be Empty    ${files}    Database documentation files not found

Validate Internal Links
    [Documentation]    Verify that internal documentation links are valid
    [Tags]    links
    ${result}=    Validate Documentation Links    ${DOCS_HTML_DIR}
    Should Be Equal    ${result}[status]    PASS    ${result}[message]

Check For Broken Cross References
    [Documentation]    Check for broken @ref and @see references
    [Tags]    links
    ${result}=    Check Cross References    ${DOCS_HTML_DIR}
    Should Be Equal    ${result}[status]    PASS    ${result}[message]

Validate Class Hierarchy
    [Documentation]    Verify class inheritance documentation is correct
    File Should Exist    ${DOCS_HTML_DIR}/inherits.html
    ${content}=    Get File    ${DOCS_HTML_DIR}/inherits.html
    Should Contain    ${content}    Player
    Should Contain    ${content}    AIPlayer
    Should Contain    ${content}    HumanPlayer

Check For Missing Documentation
    [Documentation]    Check for undocumented public APIs
    [Tags]    coverage
    ${result}=    Check Documentation Coverage    ${PROJECT_ROOT}/include
    Log    Documentation coverage: ${result}[coverage]%
    Should Be True    ${result}[coverage] >= 80    Documentation coverage ${result}[coverage]% is below threshold

*** Keywords ***
Build Documentation
    [Documentation]    Build Doxygen documentation if not already built
    ${exists}=    Run Keyword And Return Status    Directory Should Exist    ${DOCS_HTML_DIR}
    Run Keyword If    not ${exists}    Build Docs
    
Build Docs
    [Documentation]    Configure and build documentation
    ${result}=    Run Process    cmake    -S.    -B${BUILD_DIR}    
    ...    -DLIARSDICE_BUILD_DOCS=ON    -DLIARSDICE_BUILD_TESTS=OFF    
    ...    cwd=${PROJECT_ROOT}    shell=False
    Should Be Equal As Integers    ${result.rc}    0    CMake configuration failed: ${result.stderr}
    
    ${result}=    Run Process    cmake    --build    ${BUILD_DIR}    --target    GenerateDocs
    ...    cwd=${PROJECT_ROOT}    shell=False
    Should Be Equal As Integers    ${result.rc}    0    Documentation build failed: ${result.stderr}

Cleanup Documentation
    [Documentation]    Cleanup after tests (optional)
    No Operation    # Keep built docs for inspection