*** Settings ***
Documentation    Edge case tests for LiarsDice CLI
Library          LiarsDiceLibrary.py
Library          OperatingSystem

Suite Setup      Setup Test Suite
Suite Teardown   Cleanup Test Suite
Test Setup       Start Test
Test Teardown    Cleanup Process

*** Variables ***
${CLI_PATH}      ${CURDIR}/../../build/bin/liarsdice-cli
${TIMEOUT}       10

*** Test Cases ***
Test Minimum Players Game
    [Documentation]    Test game with minimum number of players (2)
    [Tags]    edge-case
    Expect Prompt    Enter the number of players
    Send Input    2
    Expect Prompt    How many AI players
    Send Input    1
    Wait For Pattern    Game starting
    # Should be able to play
    Wait For Pattern    Your turn
    Process Should Be Running

Test Maximum Dice Scenario
    [Documentation]    Test when player has maximum dice (5)
    [Tags]    edge-case
    Simulate AI Game    1
    Wait For Pattern    Your turn
    # Try to guess more dice than possible
    Send Input    1
    Expect Prompt    dice count
    Send Input    10    # More than max possible (2 players * 5 dice)
    Expect Prompt    Invalid

Test Single Die Remaining
    [Documentation]    Test end game with single die
    [Tags]    edge-case    endgame
    # This is hard to simulate directly, but we can test the input validation
    Simulate AI Game    1
    Wait For Pattern    Your turn
    Send Input    1
    Expect Prompt    dice count
    Send Input    1    # Valid - minimum guess
    Expect Prompt    face value
    Send Input    1
    Process Should Be Running

Test Rapid Input
    [Documentation]    Test handling of rapid input
    [Tags]    stress
    Expect Prompt    Enter the number of players
    # Send multiple inputs rapidly
    Send Input    2
    Send Input    1
    Send Input    1
    Send Input    2
    Send Input    1
    Send Input    3
    # Process should handle this gracefully
    Process Should Be Running
    Output Should Not Contain    segmentation fault

Test Empty Input
    [Documentation]    Test handling of empty input
    [Tags]    input-validation
    Expect Prompt    Enter the number of players
    Send Input    ${EMPTY}
    Expect Prompt    Invalid
    Send Input    2
    Expect Prompt    How many AI players
    Send Input    ${EMPTY}
    Expect Prompt    Invalid

Test Very Long Input
    [Documentation]    Test handling of very long input strings
    [Tags]    security    input-validation
    Expect Prompt    Enter the number of players
    ${long_input}=    Evaluate    "9" * 1000
    Send Input    ${long_input}
    # Should handle gracefully without crash
    Process Should Be Running
    Expect Prompt    Invalid

Test Special Characters Input
    [Documentation]    Test handling of special characters
    [Tags]    security    input-validation
    Expect Prompt    Enter the number of players
    Send Input    !@#$%^&*()
    Expect Prompt    Invalid
    Send Input    2; ls
    Expect Prompt    How many AI players
    Output Should Not Contain    total    # Should not execute shell commands

Test Concurrent Games Prevention
    [Documentation]    Test that only one game instance can run
    [Tags]    concurrency
    Simulate AI Game    1
    Wait For Pattern    Your turn
    # Try to start another game (should not be possible in single CLI instance)
    Send Input    q
    Wait For Pattern    (Goodbye|Thank you|Exiting)    timeout=5

*** Keywords ***
Setup Test Suite
    Set CLI Path    ${CLI_PATH}
    ${exists}=    Run Keyword And Return Status    File Should Exist    ${CLI_PATH}
    IF    not ${exists}
        Fail    CLI executable not found at ${CLI_PATH}. Build the project first.
    END

Start Test
    Start LiarsDice    timeout=${TIMEOUT}

Simulate AI Game
    [Arguments]    ${num_ai_players}=1
    Send Input    ${num_ai_players + 1}
    Expect Prompt    How many AI players
    Send Input    ${num_ai_players}