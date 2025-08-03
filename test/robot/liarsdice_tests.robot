*** Settings ***
Documentation    Automated tests for LiarsDice CLI using Robot Framework and Pexpect
Library          LiarsDiceLibrary.py
Library          OperatingSystem
Library          DateTime

Suite Setup      Setup Test Suite
Suite Teardown   Cleanup Test Suite
Test Setup       Start Test
Test Teardown    Cleanup Process

*** Variables ***
${CLI_PATH}      ${CURDIR}/../../build/bin/liarsdice-cli
${TIMEOUT}       10
${MAX_MEMORY}    100    # MB
${MAX_RESPONSE}  2      # seconds

*** Test Cases ***
Test CLI Startup
    [Documentation]    Test that CLI starts properly and shows main menu
    [Tags]    smoke
    Expect Prompt    Welcome to Liar's Dice
    Expect Prompt    Enter the number of players
    Output Should Not Contain    error
    Output Should Not Contain    Error

Test Input Validation - Invalid Player Count
    [Documentation]    Test input validation for player count
    [Tags]    input-validation
    Expect Prompt    Enter the number of players
    Send Input    0
    Expect Prompt    Invalid
    Send Input    10
    Expect Prompt    Invalid
    Send Input    abc
    Expect Prompt    Invalid
    Send Input    2
    Expect Prompt    How many AI players

Test Input Validation - Invalid AI Count
    [Documentation]    Test input validation for AI player count
    [Tags]    input-validation
    Expect Prompt    Enter the number of players
    Send Input    3
    Expect Prompt    How many AI players
    Send Input    5    # More than total players
    Expect Prompt    Invalid
    Send Input    -1
    Expect Prompt    Invalid
    Send Input    xyz
    Expect Prompt    Invalid

Test Input Validation - Invalid Guess
    [Documentation]    Test input validation during gameplay
    [Tags]    input-validation
    Simulate AI Game    1
    Wait For Pattern    Your turn
    Send Input    1    # Make a guess
    Expect Prompt    dice count
    Send Input    0    # Invalid dice count
    Expect Prompt    Invalid
    Send Input    100    # Too many dice
    Expect Prompt    Invalid
    Send Input    2
    Expect Prompt    face value
    Send Input    0    # Invalid face value
    Expect Prompt    Invalid
    Send Input    7    # Invalid face value
    Expect Prompt    Invalid

Test Game With Single AI - Win Condition
    [Documentation]    Test a complete game where human wins
    [Tags]    win-condition    gameplay
    Simulate AI Game    1
    ${round}=    Set Variable    0
    FOR    ${i}    IN RANGE    20    # Maximum rounds to prevent infinite loop
        ${output}=    Get Output
        ${has_your_turn}=    Run Keyword And Return Status    
        ...    Output Should Contain    Your turn
        IF    ${has_your_turn}
            Make Conservative Guess
        END
        ${has_winner}=    Run Keyword And Return Status
        ...    Wait For Pattern    (wins the game|You win|Game Over)    timeout=5
        IF    ${has_winner}
            BREAK
        END
    END
    Output Should Contain    wins the game

Test Game With Multiple AIs
    [Documentation]    Test game with multiple AI players
    [Tags]    gameplay    ai
    Expect Prompt    Enter the number of players
    Send Input    4
    Expect Prompt    How many AI players
    Send Input    3
    Wait For Pattern    Game starting
    ${game_ended}=    Set Variable    ${FALSE}
    FOR    ${i}    IN RANGE    30    # Maximum rounds
        ${output}=    Get Output
        ${has_your_turn}=    Run Keyword And Return Status
        ...    Output Should Contain    Your turn
        IF    ${has_your_turn}
            Send Input    2    # Call liar
        END
        ${has_winner}=    Run Keyword And Return Status
        ...    Wait For Pattern    (wins the game|Game Over)    timeout=3
        IF    ${has_winner}
            ${game_ended}=    Set Variable    ${TRUE}
            BREAK
        END
    END
    Should Be True    ${game_ended}    Game should have ended

Test Response Time
    [Documentation]    Test that CLI responds within reasonable time
    [Tags]    performance
    ${start}=    Get Current Date    result_format=epoch
    Expect Prompt    Enter the number of players
    ${end}=    Get Current Date    result_format=epoch
    ${response_time}=    Evaluate    ${end} - ${start}
    Should Be True    ${response_time} < ${MAX_RESPONSE}
    
    Send Input    2
    ${start}=    Get Current Date    result_format=epoch
    Expect Prompt    How many AI players
    ${end}=    Get Current Date    result_format=epoch
    ${response_time}=    Evaluate    ${end} - ${start}
    Should Be True    ${response_time} < ${MAX_RESPONSE}

Test Memory Usage During Game
    [Documentation]    Test memory usage stays within limits
    [Tags]    performance
    Simulate AI Game    1
    ${initial_memory}=    Get Process Memory Usage
    
    # Play several rounds
    FOR    ${i}    IN RANGE    5
        ${has_your_turn}=    Run Keyword And Return Status
        ...    Wait For Pattern    Your turn    timeout=5
        IF    ${has_your_turn}
            Make Conservative Guess
        END
    END
    
    ${final_memory}=    Get Process Memory Usage
    Memory Usage Should Be Less Than    ${MAX_MEMORY}
    ${memory_increase}=    Evaluate    ${final_memory} - ${initial_memory}
    Should Be True    ${memory_increase} < 20    Memory leak detected

Test Ctrl+C Signal Handling
    [Documentation]    Test graceful shutdown with Ctrl+C
    [Tags]    signal-handling
    Expect Prompt    Enter the number of players
    Send Control C
    Wait For Pattern    (Goodbye|Exiting|Interrupted)    timeout=2
    Process Should Be Terminated

Test SIGTERM Signal Handling
    [Documentation]    Test graceful shutdown with SIGTERM
    [Tags]    signal-handling
    Expect Prompt    Enter the number of players
    Send Signal    SIGTERM
    Process Should Be Terminated    timeout=3

Test Quit Command
    [Documentation]    Test quit command functionality
    [Tags]    quit
    Expect Prompt    Enter the number of players
    Send Input    q
    Wait For Pattern    (Goodbye|Thank you|Exiting)    timeout=2
    Process Should Be Terminated

Test Timeout Behavior
    [Documentation]    Test that game handles inactivity appropriately
    [Tags]    timeout
    Simulate AI Game    1
    Wait For Pattern    Your turn
    # Don't send any input and wait
    Sleep    15s
    # Game should either timeout or AI should continue
    ${still_running}=    Run Keyword And Return Status    Process Should Be Running
    IF    ${still_running}
        # If still running, it should have handled the timeout gracefully
        ${output}=    Get Output
        Should Not Contain    ${output}    segmentation fault
        Should Not Contain    ${output}    core dumped
    END

Test Invalid Menu Choice
    [Documentation]    Test handling of invalid menu choices
    [Tags]    input-validation
    Simulate AI Game    1
    Wait For Pattern    Your turn
    Send Input    5    # Invalid choice
    Expect Prompt    Invalid
    Send Input    abc
    Expect Prompt    Invalid

Test Large Number of Rounds
    [Documentation]    Test stability over many rounds
    [Tags]    stress    long-running
    Simulate AI Game    1
    ${rounds_played}=    Set Variable    0
    FOR    ${i}    IN RANGE    50
        ${has_your_turn}=    Run Keyword And Return Status
        ...    Wait For Pattern    Your turn    timeout=3
        IF    ${has_your_turn}
            ${rounds_played}=    Evaluate    ${rounds_played} + 1
            # Alternate between guess and call
            ${choice}=    Evaluate    (${rounds_played} % 3) + 1
            IF    ${choice} <= 2
                Make Conservative Guess
            ELSE
                Send Input    2    # Call liar
            END
        END
        ${game_ended}=    Run Keyword And Return Status
        ...    Output Should Contain    wins the game
        IF    ${game_ended}
            BREAK
        END
    END
    Should Be True    ${rounds_played} > 5    Should have played several rounds

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
    Start LiarsDice    timeout=${TIMEOUT}

Make Conservative Guess
    [Documentation]    Make a safe guess based on probability
    Send Input    1    # Make a guess
    Expect Prompt    dice count
    Send Input    2    # Conservative dice count
    Expect Prompt    face value
    Send Input    3    # Middle face value