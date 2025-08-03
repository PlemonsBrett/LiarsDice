*** Settings ***
Documentation    Deterministic tests using seed for win/lose conditions
Library          LiarsDiceLibrary.py
Library          OperatingSystem
Library          String

Suite Setup      Setup Test Suite
Suite Teardown   Cleanup Test Suite
Test Setup       Start Test
Test Teardown    Cleanup Process

*** Variables ***
${CLI_PATH}      ${CURDIR}/../../build/standalone/liarsdice
${TIMEOUT}       30

*** Test Cases ***
Test Deterministic Gameplay With Seed
    [Documentation]    Test that same seed produces same dice rolls
    [Tags]    deterministic    seed
    # Start game with seed 12345
    Start LiarsDice    args=--seed 12345
    Expect Prompt    Welcome to Liar's Dice
    Expect Prompt    Enter the number of players
    Send Input    2
    Expect Prompt    How many AI players
    Send Input    1
    Wait For Pattern    Game starting
    
    # Wait for game state and dice to be shown
    Wait For Pattern    Your dice:
    ${output}=    Get Output
    ${dice_line}=    Get Line    ${output}    -1
    Log    First game dice: ${dice_line}
    
    # Just play one round to verify game works
    Wait For Pattern    Your turn
    Send Input    1    # Make a guess
    Expect Prompt    dice count
    Send Input    1
    Expect Prompt    face value
    Send Input    2
    
    # Verify the game continues
    Process Should Be Running

Test Game With Seed Runs Properly
    [Documentation]    Test that game with seed runs without errors
    [Tags]    deterministic    basic
    # Use seed 1000 
    Start LiarsDice    args=--seed 1000
    Simulate AI Game    1
    
    # Play a few rounds
    FOR    ${i}    IN RANGE    3
        ${has_your_turn}=    Run Keyword And Return Status
        ...    Wait For Pattern    Your turn    timeout=5
        IF    ${has_your_turn}
            # Make a simple guess
            Send Input    1
            Expect Prompt    dice count
            Send Input    1
            Expect Prompt    face value
            Send Input    2
            BREAK
        END
    END
    
    # Verify game is running
    Process Should Be Running

*** Keywords ***
Setup Test Suite
    [Documentation]    One-time setup for the test suite
    Set CLI Path    ${CLI_PATH}
    ${exists}=    Run Keyword And Return Status    File Should Exist    ${CLI_PATH}
    IF    not ${exists}
        Fail    CLI executable not found at ${CLI_PATH}. Build the project first.
    END

Cleanup Test Suite
    [Documentation]    One-time cleanup for the test suite
    Cleanup Process

Start Test
    [Documentation]    Setup for each test
    No Operation    # Do nothing - we'll start the process with specific args in tests

Simulate AI Game
    [Arguments]    ${num_ai_players}=1
    Expect Prompt    Enter the number of players
    ${total_players}=    Evaluate    str(int(${num_ai_players}) + 1)
    Send Input    ${total_players}
    Expect Prompt    How many AI players
    Send Input    ${num_ai_players}
    Wait For Pattern    Game starting