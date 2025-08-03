*** Settings ***
Documentation     Test suite for single player mode with AI opponents
Library           ./LiarsDiceLibrary.py
Library           OperatingSystem
Suite Setup       Setup Test Suite
Suite Teardown    Cleanup Test Suite
Test Setup        Start Test
Test Teardown     Cleanup Process
Test Tags         ai    single-player

*** Variables ***
${TIMEOUT}        30
${CLI_PATH}       ${EMPTY}

*** Test Cases ***
Test Single Player Mode Selection
    [Documentation]    Test entering single player mode and difficulty selection
    [Tags]    smoke    ai-menu
    Expect Prompt    Welcome to Liar's Dice
    Expect Prompt    Enter the number of players
    Send Input    1
    Expect Prompt    Single Player Mode!
    Expect Prompt    Select difficulty level
    Expect Prompt    1. Easy
    Expect Prompt    2. Medium
    Expect Prompt    3. Hard
    Expect Prompt    4. Expert
    Expect Prompt    Enter your choice (1-4)
    Send Input    1
    Wait For Pattern    Starting Turn

Test Easy AI Makes Valid Guess
    [Documentation]    Test that Easy AI makes a valid guess after human player
    [Tags]    easy-ai    gameplay
    Expect Prompt    Welcome to Liar's Dice
    Expect Prompt    Enter the number of players
    Send Input    1
    Expect Prompt    Enter your choice (1-4)
    Send Input    1
    # Wait for game to start and show player's turn
    Expect Prompt    PLAYER 1's Turn
    Expect Prompt    Your Dice
    Expect Prompt    Enter your guess in format (quantity, face_value)
    Send Input    1,2
    Expect Prompt    Do you want to call liar? (yes/no)
    Send Input    no
    # Now AI should make its turn
    Expect Prompt    PLAYER 2's Turn
    Expect Prompt    Easy AI guesses
    # AI should make a valid guess that's higher than 1,2
    Output Should Not Contain    Invalid guess

Test Medium Difficulty Creates Multiple AIs
    [Documentation]    Test that medium difficulty creates 2 Easy AI opponents
    [Tags]    medium-difficulty    ai-setup
    Expect Prompt    Welcome to Liar's Dice
    Expect Prompt    Enter the number of players
    Send Input    1
    Expect Prompt    Enter your choice (1-4)
    Send Input    2
    Expect Prompt    Game starting with 3 players
    Expect Prompt    You are Player 1
    Expect Prompt    Player 2: Easy AI 1
    Expect Prompt    Player 3: Easy AI 2

Test Hard Difficulty Mixed AI Types
    [Documentation]    Test that hard difficulty creates 1 Easy + 1 Medium AI
    [Tags]    hard-difficulty    ai-setup
    Expect Prompt    Welcome to Liar's Dice
    Expect Prompt    Enter the number of players
    Send Input    1
    Expect Prompt    Enter your choice (1-4)
    Send Input    3
    Expect Prompt    Game starting with 3 players
    Expect Prompt    Player 3: Medium AI

Test Expert Difficulty Multiple Medium AIs
    [Documentation]    Test that expert difficulty creates 2 Medium AI opponents
    [Tags]    expert-difficulty    ai-setup
    Expect Prompt    Welcome to Liar's Dice
    Expect Prompt    Enter the number of players
    Send Input    1
    Expect Prompt    Enter your choice (1-4)
    Send Input    4
    Expect Prompt    Game starting with 3 players
    Expect Prompt    You are Player 1
    Expect Prompt    Player 2: Medium AI 1
    Expect Prompt    Player 3: Medium AI 2

Test AI Calls Liar
    [Documentation]    Test that AI can call liar on an obviously bad guess
    [Tags]    ai-behavior    liar-call
    Expect Prompt    Welcome to Liar's Dice
    Expect Prompt    Enter the number of players
    Send Input    1
    Expect Prompt    Enter your choice (1-4)
    Send Input    1
    # Play several rounds to get to a high guess
    Expect Prompt    PLAYER 1's Turn
    Send Input    3,6
    Send Input    no
    # AI makes a guess
    Expect Prompt    PLAYER 2's Turn
    Expect Prompt    Easy AI guesses
    # Human makes an unrealistic guess
    Expect Prompt    PLAYER 1's Turn
    Send Input    10,6
    Send Input    no
    # AI should call liar on such a high guess
    Expect Prompt    Easy AI calls liar
    Expect Prompt    The winner is

*** Keywords ***
Setup Test Suite
    [Documentation]    One-time setup for the test suite
    Set Cli Path    ${CLI_PATH}
    ${exists}=    Run Keyword And Return Status    File Should Exist    ${CLI_PATH}
    IF    not ${exists}
        Fail    CLI executable not found at ${CLI_PATH}. Build the project first.
    END

Cleanup Test Suite
    [Documentation]    One-time cleanup for the test suite
    Cleanup Process

Start Test
    [Documentation]    Setup for each test
    Start Liarsdice    timeout=${TIMEOUT}