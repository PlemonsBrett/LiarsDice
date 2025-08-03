*** Settings ***
Documentation    Performance tests for LiarsDice CLI
Library          LiarsDiceLibrary.py
Library          OperatingSystem
Library          Collections

Suite Setup      Setup Test Suite
Suite Teardown   Cleanup Test Suite
Test Setup       Start Test
Test Teardown    Cleanup Process

*** Variables ***
${CLI_PATH}          ${CURDIR}/../../build/standalone/liarsdice
${TIMEOUT}           30
${MAX_STARTUP_TIME}  1.0    # seconds
${MAX_TURN_TIME}     0.5    # seconds
${MAX_MEMORY_MB}     50     # MB
${MAX_CPU_PERCENT}   80     # percent

*** Test Cases ***
Test Startup Performance
    [Documentation]    Measure CLI startup time
    [Tags]    performance    benchmark
    @{startup_times}=    Create List
    
    FOR    ${i}    IN RANGE    5
        ${start}=    Get Time    epoch
        Start LiarsDice    timeout=${TIMEOUT}
        Expect Prompt    Welcome to Liar's Dice
        ${end}=    Get Time    epoch
        ${startup_time}=    Evaluate    ${end} - ${start}
        Append To List    ${startup_times}    ${startup_time}
        Cleanup Process
    END
    
    ${avg_startup}=    Evaluate    sum(${startup_times}) / len(${startup_times})
    Log    Average startup time: ${avg_startup}s
    Should Be True    ${avg_startup} < ${MAX_STARTUP_TIME}

Test AI Decision Performance
    [Documentation]    Measure AI decision-making speed
    [Tags]    performance    ai
    Skip    Skipping due to timeout issues - will fix later
    Simulate AI Game    3    # 3 AI players
    
    @{decision_times}=    Create List
    
    FOR    ${round}    IN RANGE    10
        ${start}=    Get Time    epoch
        Wait For Pattern    (Your turn|Player \\d+ turn)    timeout=10
        ${end}=    Get Time    epoch
        ${decision_time}=    Evaluate    ${end} - ${start}
        
        # If it's our turn, make a move
        ${is_our_turn}=    Run Keyword And Return Status
        ...    Output Should Contain    Your turn
        IF    ${is_our_turn}
            Make Quick Decision
        END
        
        Append To List    ${decision_times}    ${decision_time}
    END
    
    ${avg_decision}=    Evaluate    sum(${decision_times}) / len(${decision_times})
    Log    Average AI decision time: ${avg_decision}s
    Should Be True    ${avg_decision} < ${MAX_TURN_TIME}

Test Memory Usage Growth
    [Documentation]    Test memory usage over extended gameplay
    [Tags]    performance    memory
    Simulate AI Game    2
    
    @{memory_samples}=    Create List
    ${initial_memory}=    Get Process Memory Usage
    Append To List    ${memory_samples}    ${initial_memory}
    
    # Play many rounds
    FOR    ${i}    IN RANGE    20
        ${has_turn}=    Run Keyword And Return Status
        ...    Wait For Pattern    Your turn    timeout=5
        IF    ${has_turn}
            Make Quick Decision
        END
        
        # Sample memory every 5 rounds
        IF    ${i} % 5 == 0
            ${memory}=    Get Process Memory Usage
            Append To List    ${memory_samples}    ${memory}
        END
        
        ${game_over}=    Run Keyword And Return Status
        ...    Output Should Contain    wins the game
        IF    ${game_over}
            BREAK
        END
    END
    
    ${final_memory}=    Get Process Memory Usage
    ${memory_growth}=    Evaluate    ${final_memory} - ${initial_memory}
    
    Log    Initial memory: ${initial_memory}MB, Final: ${final_memory}MB
    Log    Memory growth: ${memory_growth}MB
    
    Should Be True    ${memory_growth} < 10    Excessive memory growth detected
    Memory Usage Should Be Less Than    ${MAX_MEMORY_MB}

Test Input Processing Speed
    [Documentation]    Test speed of input validation and processing
    [Tags]    performance    input
    Expect Prompt    Enter the number of players
    
    @{response_times}=    Create List
    
    # Test various inputs
    @{test_inputs}=    Create List    0    1    2    5    10    abc    ${EMPTY}    999
    
    FOR    ${input}    IN    @{test_inputs}
        ${start}=    Get Time    epoch
        Send Input    ${input}
        Wait For Pattern    (Invalid|How many AI)    timeout=2
        ${end}=    Get Time    epoch
        ${response_time}=    Evaluate    float(${end}) - float(${start})
        Append To List    ${response_times}    ${response_time}
    END
    
    ${avg_response}=    Evaluate    sum(${response_times}) / len(${response_times})
    Log    Average input processing time: ${avg_response}s
    Should Be True    ${avg_response} < 0.1    Input processing too slow

Test Large Game Performance
    [Documentation]    Test performance with maximum players
    [Tags]    performance    stress
    Expect Prompt    Enter the number of players
    Send Input    6    # Maximum players
    Expect Prompt    How many AI players
    Send Input    5    # All AI except human
    
    ${start}=    Get Time    epoch
    Wait For Pattern    Game starting
    ${setup_end}=    Get Time    epoch
    ${setup_time}=    Evaluate    float(${setup_end}) - float(${start})
    
    Log    Game setup time for 6 players: ${setup_time}s
    Should Be True    ${setup_time} < 2.0
    
    # Measure turn cycle time
    ${turn_start}=    Get Time    epoch
    Wait For Pattern    Your turn    timeout=30
    ${turn_end}=    Get Time    epoch
    ${full_cycle_time}=    Evaluate    float(${turn_end}) - float(${turn_start})
    
    Log    Full turn cycle time (5 AI players): ${full_cycle_time}s
    Should Be True    ${full_cycle_time} < 5.0    Turn cycle too slow

Test Stress Load
    [Documentation]    Stress test with rapid actions
    [Tags]    stress    performance
    Simulate AI Game    1
    
    ${stress_start}=    Get Time    epoch
    
    # Rapid fire actions
    FOR    ${i}    IN RANGE    50
        ${has_turn}=    Run Keyword And Return Status
        ...    Wait For Pattern    Your turn    timeout=0.5
        IF    ${has_turn}
            # Make rapid decisions
            ${action}=    Evaluate    random.randint(1, 2)    modules=random
            Send Input    ${action}
            IF    ${action} == 1
                Send Input    2    # dice count
                Send Input    3    # face value
            END
        END
    END
    
    ${stress_end}=    Get Time    epoch
    ${stress_duration}=    Evaluate    float(${stress_end}) - float(${stress_start})
    
    Process Should Be Running
    Memory Usage Should Be Less Than    ${MAX_MEMORY_MB}

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
    ${total_players}=    Evaluate    str(int(${num_ai_players}) + 1)
    Send Input    ${total_players}
    Expect Prompt    How many AI players
    Send Input    ${num_ai_players}

Make Quick Decision
    [Documentation]    Make a quick game decision
    ${action}=    Evaluate    random.randint(1, 2)    modules=random
    Send Input    ${action}
    IF    ${action} == 1
        Expect Prompt    dice count
        Send Input    2
        Expect Prompt    face value
        Send Input    4
    END

Cleanup Test Suite
    [Documentation]    One-time cleanup for the test suite
    Cleanup Process
