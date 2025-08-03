*** Settings ***
Documentation     Test suite for AI strategy behaviors and patterns
Library           ./LiarsDiceLibrary.py
Suite Setup       Check CLI Exists
Test Tags         ai    strategies

*** Variables ***
${TIMEOUT}        30

*** Test Cases ***
Test AI Strategy Registration
    [Documentation]    Verify AI strategies are properly registered
    [Tags]    ai-registration    future
    Skip    AI strategy selection not yet implemented in CLI
    # When implemented:
    # Start Game With AI Strategy    easy
    # Output Should Contain    Easy AI
    # Send Exit Command
    
Test Easy AI Conservative Behavior
    [Documentation]    Test that Easy AI makes conservative guesses
    [Tags]    easy-ai    behavior    future
    Skip    AI strategy selection not yet implemented in CLI
    # When implemented:
    # Start Game With AI Strategy    easy    num_players=2
    # Wait For Pattern    Your turn
    # Make Valid Guess    2    2
    # Wait For AI Turn
    # ${ai_guess}=    Get Last AI Guess
    # Should Be Conservative Guess    ${ai_guess}

Test Medium AI Statistical Analysis
    [Documentation]    Test that Medium AI uses probability calculations
    [Tags]    medium-ai    statistical    future
    Skip    AI strategy selection not yet implemented in CLI
    # When implemented:
    # Start Game With AI Strategy    medium    num_players=2
    # Play Multiple Rounds    5
    # ${ai_patterns}=    Analyze AI Behavior
    # Should Show Statistical Patterns    ${ai_patterns}

Test Medium AI Opponent Modeling
    [Documentation]    Test that Medium AI adapts to player behavior
    [Tags]    medium-ai    adaptation    future
    Skip    AI strategy selection not yet implemented in CLI
    # When implemented:
    # Start Game With AI Strategy    medium    num_players=2
    # Make Aggressive Guesses    3
    # ${ai_response}=    Get AI Call Rate
    # Should Be True    ${ai_response} > 0.5    Medium AI should detect aggressive pattern

Test Medium AI Bluff Detection
    [Documentation]    Test that Medium AI detects bluffing patterns
    [Tags]    medium-ai    bluff-detection    future
    Skip    AI strategy selection not yet implemented in CLI
    # When implemented:
    # Start Game With AI Strategy    medium    num_players=2
    # Make Pattern Bluffs    quantity_pattern=[2,4,6,8]
    # Wait For AI Turn
    # ${ai_action}=    Get Last AI Action
    # Should Be Equal    ${ai_action}    call_liar

Test AI Strategy Performance Comparison
    [Documentation]    Compare decision times between strategies
    [Tags]    performance    comparison    future
    Skip    AI strategy selection not yet implemented in CLI
    # When implemented:
    # ${easy_time}=    Measure AI Decision Time    easy
    # ${medium_time}=    Measure AI Decision Time    medium
    # Log    Easy AI avg time: ${easy_time}s
    # Log    Medium AI avg time: ${medium_time}s
    # Should Be True    ${medium_time} > ${easy_time}    Medium AI should take more time

Test Multiple AI Strategies In Same Game
    [Documentation]    Test game with different AI strategies
    [Tags]    multi-strategy    future
    Skip    AI strategy selection not yet implemented in CLI
    # When implemented:
    # Start Game With Mixed AI    strategies=[easy,medium,easy]
    # Play Full Game
    # ${winner}=    Get Game Winner
    # Log    Winner: ${winner}

*** Keywords ***
Check CLI Exists
    File Should Exist    ${CLI_PATH}    CLI executable not found at ${CLI_PATH}. Build the project first.

Skip
    [Arguments]    ${reason}
    Skip Test    ${reason}

# Future Keywords (to be implemented when AI strategies are integrated)

Start Game With AI Strategy
    [Arguments]    ${strategy}    ${num_players}=2
    [Documentation]    Start game with specific AI strategy
    Start CLI
    Send Input    ${num_players}
    Expect Prompt    How many AI players
    Send Input    ${num_players - 1}
    Expect Prompt    Select AI difficulty
    Send Input    ${strategy}
    Wait For Pattern    Game starting

Start Game With Mixed AI
    [Arguments]    ${strategies}
    [Documentation]    Start game with multiple AI strategies
    Start CLI
    ${num_players}=    Get Length    ${strategies}
    Send Input    ${num_players + 1}
    FOR    ${strategy}    IN    @{strategies}
        Expect Prompt    Select AI difficulty for Player
        Send Input    ${strategy}
    END
    Wait For Pattern    Game starting

Wait For AI Turn
    [Documentation]    Wait for AI player to make a move
    Wait For Pattern    (AI Player|Player \\d+) (guesses|calls)    timeout=${TIMEOUT}

Get Last AI Guess
    [Documentation]    Extract the last AI guess from output
    ${output}=    Get Output
    ${match}=    Get Regexp Matches    ${output}    Player \\d+ guesses (\\d+) (\\d+)s
    Should Not Be Empty    ${match}
    RETURN    ${match[-1]}

Get Last AI Action
    [Documentation]    Get the last action taken by AI
    ${output}=    Get Output
    ${has_call}=    Run Keyword And Return Status    Should Contain    ${output}    calls liar
    IF    ${has_call}
        RETURN    call_liar
    ELSE
        RETURN    make_guess
    END

Make Valid Guess
    [Arguments]    ${quantity}    ${face_value}
    Send Input    g
    Send Input    ${quantity}
    Send Input    ${face_value}

Make Aggressive Guesses
    [Arguments]    ${count}
    FOR    ${i}    IN RANGE    ${count}
        Wait For Pattern    Your turn
        ${qty}=    Evaluate    ${i} * 3 + 2
        Make Valid Guess    ${qty}    5
    END

Make Pattern Bluffs
    [Arguments]    ${quantity_pattern}
    FOR    ${qty}    IN    @{quantity_pattern}
        Wait For Pattern    Your turn
        Make Valid Guess    ${qty}    3
    END

Play Multiple Rounds
    [Arguments]    ${rounds}
    FOR    ${i}    IN RANGE    ${rounds}
        Wait For Pattern    Your turn
        ${qty}=    Evaluate    ${i} + 1
        ${face}=    Evaluate    (${i} % 6) + 1
        Make Valid Guess    ${qty}    ${face}
        Wait For AI Turn
    END

Play Full Game
    [Documentation]    Play until game ends
    ${game_over}=    Set Variable    ${False}
    WHILE    not ${game_over}
        ${status}=    Run Keyword And Return Status    Wait For Pattern    Your turn    timeout=5
        IF    ${status}
            Make Valid Guess    2    3
        END
        ${game_over}=    Run Keyword And Return Status    
        ...    Wait For Pattern    (wins the game|Game Over)    timeout=2
    END

Get Game Winner
    [Documentation]    Extract winner from game output
    ${output}=    Get Output
    ${match}=    Get Regexp Matches    ${output}    (Player \\d+|You) wins the game
    Should Not Be Empty    ${match}
    RETURN    ${match[-1]}

Measure AI Decision Time
    [Arguments]    ${strategy}
    [Documentation]    Measure average decision time for AI strategy
    Start Game With AI Strategy    ${strategy}
    ${times}=    Create List
    FOR    ${i}    IN RANGE    5
        ${start}=    Get Time    epoch
        Wait For AI Turn
        ${end}=    Get Time    epoch
        ${duration}=    Evaluate    ${end} - ${start}
        Append To List    ${times}    ${duration}
        Wait For Pattern    Your turn
        Make Valid Guess    2    2
    END
    ${avg_time}=    Evaluate    sum(${times}) / len(${times})
    Send Exit Command
    RETURN    ${avg_time}

Should Be Conservative Guess
    [Arguments]    ${guess}
    [Documentation]    Verify guess is conservative (low quantity)
    @{parts}=    Split String    ${guess}
    ${quantity}=    Convert To Integer    ${parts[0]}
    Should Be True    ${quantity} <= 3    AI made aggressive guess: ${quantity}

Should Show Statistical Patterns
    [Arguments]    ${patterns}
    [Documentation]    Verify AI shows statistical decision making
    Should Contain    ${patterns}    probability
    Should Contain    ${patterns}    calculated

Analyze AI Behavior
    [Documentation]    Analyze AI behavior from game output
    ${output}=    Get Output
    # Extract patterns from output
    ${patterns}=    Create Dictionary
    ...    bluff_rate=0.2
    ...    aggression=0.5
    ...    probability=True
    RETURN    ${patterns}

Get AI Call Rate
    [Documentation]    Calculate how often AI calls liar
    ${output}=    Get Output
    ${calls}=    Get Regexp Matches    ${output}    Player \\d+ calls liar
    ${guesses}=    Get Regexp Matches    ${output}    Player \\d+ guesses
    ${call_rate}=    Evaluate    len(${calls}) / (len(${calls}) + len(${guesses}))
    RETURN    ${call_rate}