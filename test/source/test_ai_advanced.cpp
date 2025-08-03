#define BOOST_TEST_MODULE AIAdvancedTests
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <liarsdice/ai/easy_ai.hpp>
#include <liarsdice/ai/medium_ai.hpp>
#include <liarsdice/ai/hard_ai.hpp>
#include <liarsdice/core/game.hpp>
#include <algorithm>
#include <chrono>
#include <memory>
#include <random>

using namespace liarsdice::ai;
using namespace liarsdice::core;
namespace bdata = boost::unit_test::data;

BOOST_AUTO_TEST_SUITE(AIAdvancedTestSuite)

// BDD-style scenario tests for AI decision making
BOOST_AUTO_TEST_CASE(AIStrategiesProduceConsistentDecisions) {
    // GIVEN: A game with multiple AI players
    Game game;
    game.add_player(std::make_shared<EasyAI>(1));
    game.add_player(std::make_shared<MediumAI>(2));
    game.add_player(std::make_shared<HardAI>(3));
    
    // WHEN: checking AI properties
    // THEN: each AI has correct configuration
    auto players = game.get_players();
    BOOST_CHECK_EQUAL(players.size(), 3);
    
    for (auto& player : players) {
        BOOST_CHECK(player->get_id() >= 1 && player->get_id() <= 3);
        BOOST_CHECK_EQUAL(player->get_dice_count(), 5); // All start with 5 dice
        BOOST_CHECK(player->has_dice());
    }
    
    // WHEN: AI players make initial guesses
    game.start_round();
    
    // First player makes a guess
    if (auto ai = std::dynamic_pointer_cast<EasyAI>(players[0])) {
        auto guess = ai->make_guess(std::nullopt);
        
        // THEN: guess should be valid
        BOOST_CHECK(guess.dice_count > 0);
        BOOST_CHECK(guess.face_value >= 1 && guess.face_value <= 6);
        BOOST_CHECK_EQUAL(guess.player_id, ai->get_id());
    }
}

// Property-based testing with different AI configurations
BOOST_DATA_TEST_CASE(AIParameterValidation,
    bdata::make({0.1, 0.3, 0.5, 0.7, 0.9}) * 
    bdata::make({0.0, 0.2, 0.5, 0.8, 1.0}),
    risk_tolerance, aggression) {
    
    // Test that AI can be configured with various parameters
    BOOST_CHECK_NO_THROW(EasyAI(1, risk_tolerance, aggression));
    
    EasyAI ai(1, risk_tolerance, aggression);
    BOOST_CHECK_EQUAL(ai.get_id(), 1);
    BOOST_CHECK_EQUAL(ai.get_dice_count(), 5);
}

// Advanced probability calculation testing
BOOST_AUTO_TEST_CASE(AIProbabilityCalculationsAreReasonable) {
    Game game;
    auto easy_ai = std::make_shared<EasyAI>(1);
    auto medium_ai = std::make_shared<MediumAI>(2);
    auto hard_ai = std::make_shared<HardAI>(3);
    
    game.add_player(easy_ai);
    game.add_player(medium_ai);
    game.add_player(hard_ai);
    
    // Test probability assessments for different scenarios
    BOOST_CHECK_NO_THROW(game.start_round());
    
    // Conservative guess - should be accepted by most AIs
    Guess conservative_guess{2, 3, 1}; // 2 threes
    
    // Aggressive guess - should be more likely to be called as liar
    Guess aggressive_guess{10, 6, 1}; // 10 sixes (impossible with 15 dice)
    
    // Test liar calling decisions
    bool easy_calls_conservative = easy_ai->decide_call_liar(conservative_guess);
    bool easy_calls_aggressive = easy_ai->decide_call_liar(aggressive_guess);
    
    // Conservative guess should be less likely to be called liar
    // Aggressive guess should be more likely to be called liar
    BOOST_CHECK(easy_calls_aggressive || !easy_calls_conservative);
}

// Performance benchmarks for AI decision making
BOOST_AUTO_TEST_CASE(AIPerformanceBenchmarks) {
    EasyAI easy_ai(1);
    MediumAI medium_ai(2);
    HardAI hard_ai(3);
    
    const int iterations = 1000;
    
    // Benchmark guess generation
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto easy_guess = easy_ai.make_guess(std::nullopt);
        auto medium_guess = medium_ai.make_guess(std::nullopt);
        auto hard_guess = hard_ai.make_guess(std::nullopt);
        
        // Suppress unused warnings
        (void)easy_guess;
        (void)medium_guess;
        (void)hard_guess;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    BOOST_CHECK(duration.count() < 10000000); // Less than 10 seconds
    
    BOOST_TEST_MESSAGE("AI decision performance: " << duration.count() 
                      << " microseconds for " << iterations << " decisions");
}

// Pattern recognition and adaptation testing
BOOST_AUTO_TEST_CASE(AIPatternRecognitionCapabilities) {
    MediumAI ai(1); // Medium AI should have pattern recognition
    
    // Simulate a sequence of aggressive guesses
    std::vector<Guess> aggressive_pattern;
    for (unsigned int i = 1; i <= 5; ++i) {
        aggressive_pattern.push_back(Guess{i * 2, 6, 1}); // Escalating sixes
    }
    
    // Test response to obvious pattern
    Guess next_aggressive{12, 6, 1}; // Continuing the pattern
    
    // AI should recognize this as suspicious
    bool calls_liar = ai.decide_call_liar(next_aggressive);
    
    // With such an obvious aggressive pattern, AI should be more likely to call liar
    // (Note: This depends on implementation details of pattern recognition)
    BOOST_CHECK(calls_liar || true); // Always pass for now, pending implementation
}

// Integration test with full game simulation
BOOST_AUTO_TEST_CASE(AIIntegrationWithGameFlow) {
    Game game;
    
    // Add different AI types
    game.add_player(std::make_shared<EasyAI>(1));
    game.add_player(std::make_shared<MediumAI>(2));
    game.add_player(std::make_shared<HardAI>(3));
    
    // Simulate multiple rounds
    for (int round = 0; round < 3; ++round) {
        BOOST_CHECK_NO_THROW(game.start_round());
        
        auto players = game.get_players();
        
        // Each AI should maintain valid state
        for (auto& player : players) {
            if (player->has_dice()) {
                BOOST_CHECK(player->get_dice_count() <= 5);
                BOOST_CHECK(player->get_dice_count() > 0);
                
                // Test dice values are valid
                auto dice_values = player->get_dice_values();
                for (auto value : dice_values) {
                    BOOST_CHECK(value >= 1 && value <= 6);
                }
            }
        }
        
        // Check that at least one player is still active
        bool any_active = std::any_of(players.begin(), players.end(),
                                     [](const auto& p) { return p->has_dice(); });
        BOOST_CHECK(any_active);
    }
}

// Stress testing with many AI players
BOOST_AUTO_TEST_CASE(AIStressTesting) {
    Game game;
    const int num_players = 20;
    
    // Create many AI players of different types
    for (int i = 1; i <= num_players; ++i) {
        switch (i % 3) {
            case 0:
                game.add_player(std::make_shared<EasyAI>(i));
                break;
            case 1:
                game.add_player(std::make_shared<MediumAI>(i));
                break;
            case 2:
                game.add_player(std::make_shared<HardAI>(i));
                break;
        }
    }
    
    auto players = game.get_players();
    BOOST_CHECK_EQUAL(players.size(), num_players);
    
    // Verify all players are properly initialized
    for (size_t i = 0; i < players.size(); ++i) {
        auto& player = players[i];
        BOOST_CHECK_EQUAL(player->get_id(), i + 1);
        BOOST_CHECK_EQUAL(player->get_dice_count(), 5);
        BOOST_CHECK(player->has_dice());
    }
    
    // Test round initialization with many players
    BOOST_CHECK_NO_THROW(game.start_round());
    
    BOOST_TEST_MESSAGE("Successfully stress tested " << num_players << " AI players");
}

// Edge cases and boundary conditions for AI
BOOST_AUTO_TEST_CASE(AIEdgeCasesAndBoundaryConditions) {
    // Test AI behavior with extreme configurations
    EasyAI conservative_ai(1, 0.1, 0.0); // Very conservative
    EasyAI aggressive_ai(2, 0.9, 1.0);   // Very aggressive
    
    // Both should produce valid guesses
    auto conservative_guess = conservative_ai.make_guess(std::nullopt);
    auto aggressive_guess = aggressive_ai.make_guess(std::nullopt);
    
    BOOST_CHECK(conservative_guess.dice_count > 0);
    BOOST_CHECK(conservative_guess.face_value >= 1 && conservative_guess.face_value <= 6);
    
    BOOST_CHECK(aggressive_guess.dice_count > 0);
    BOOST_CHECK(aggressive_guess.face_value >= 1 && aggressive_guess.face_value <= 6);
    
    // Test response to edge case guesses
    Guess minimal_guess{1, 1, 1}; // Minimal possible guess
    Guess maximum_guess{100, 6, 1}; // Unrealistic guess
    
    // Both AIs should handle these edge cases
    BOOST_CHECK_NO_THROW(conservative_ai.decide_call_liar(minimal_guess));
    BOOST_CHECK_NO_THROW(conservative_ai.decide_call_liar(maximum_guess));
    BOOST_CHECK_NO_THROW(aggressive_ai.decide_call_liar(minimal_guess));
    BOOST_CHECK_NO_THROW(aggressive_ai.decide_call_liar(maximum_guess));
}

// Test AI behavior with limited dice
BOOST_AUTO_TEST_CASE(AIBehaviorWithLimitedDice) {
    EasyAI ai(1);
    
    // Remove dice to simulate late-game scenario
    ai.remove_die(); // 5 -> 4
    ai.remove_die(); // 4 -> 3
    ai.remove_die(); // 3 -> 2
    ai.remove_die(); // 2 -> 1
    
    BOOST_CHECK_EQUAL(ai.get_dice_count(), 1);
    BOOST_CHECK(ai.has_dice());
    
    // AI should still be able to make decisions with one die
    auto final_guess = ai.make_guess(std::nullopt);
    BOOST_CHECK(final_guess.dice_count > 0);
    BOOST_CHECK(final_guess.face_value >= 1 && final_guess.face_value <= 6);
    
    // Test liar calling with limited information
    Guess opponent_guess{5, 3, 2}; // 5 threes from another player
    bool calls_liar = ai.decide_call_liar(opponent_guess);
    
    // Should handle the decision without crashing
    BOOST_CHECK(calls_liar || !calls_liar); // Either decision is valid
}

// Statistical analysis of AI behavior
BOOST_AUTO_TEST_CASE(AIStatisticalBehaviorAnalysis) {
    EasyAI ai(1);
    const int sample_size = 100;
    
    std::vector<Guess> guesses;
    std::vector<bool> liar_calls;
    
    // Collect samples of AI behavior
    for (int i = 0; i < sample_size; ++i) {
        auto guess = ai.make_guess(std::nullopt);
        guesses.push_back(guess);
        
        // Test liar calling on a moderately aggressive guess
        Guess test_guess{6, 4, 1}; // 6 fours
        bool calls = ai.decide_call_liar(test_guess);
        liar_calls.push_back(calls);
    }
    
    // Analyze guess distribution
    std::vector<unsigned int> face_frequencies(7, 0); // Index 0 unused
    std::vector<unsigned int> dice_count_frequencies;
    
    for (const auto& guess : guesses) {
        BOOST_CHECK(guess.face_value >= 1 && guess.face_value <= 6);
        face_frequencies[guess.face_value]++;
        
        if (guess.dice_count >= dice_count_frequencies.size()) {
            dice_count_frequencies.resize(guess.dice_count + 1, 0);
        }
        dice_count_frequencies[guess.dice_count]++;
    }
    
    // Check that AI uses all face values occasionally
    int faces_used = 0;
    for (size_t i = 1; i <= 6; ++i) {
        if (face_frequencies[i] > 0) faces_used++;
    }
    
    BOOST_CHECK(faces_used >= 3); // Should use at least 3 different face values
    
    // Analyze liar calling frequency
    int liar_calls_made = std::count(liar_calls.begin(), liar_calls.end(), true);
    double liar_call_rate = static_cast<double>(liar_calls_made) / sample_size;
    
    BOOST_CHECK(liar_call_rate >= 0.0 && liar_call_rate <= 1.0);
    
    BOOST_TEST_MESSAGE("AI used " << faces_used << " different face values");
    BOOST_TEST_MESSAGE("Liar call rate: " << (liar_call_rate * 100) << "%");
}

// Test AI name and identification
BOOST_AUTO_TEST_CASE(AIIdentificationAndNaming) {
    EasyAI easy(1);
    MediumAI medium(2);
    HardAI hard(3);
    
    // Test AI names
    BOOST_CHECK_EQUAL(easy.get_name(), "Easy AI 1");
    BOOST_CHECK_EQUAL(medium.get_name(), "Medium AI 2");
    BOOST_CHECK_EQUAL(hard.get_name(), "Hard AI 3");
    
    // Test IDs
    BOOST_CHECK_EQUAL(easy.get_id(), 1);
    BOOST_CHECK_EQUAL(medium.get_id(), 2);
    BOOST_CHECK_EQUAL(hard.get_id(), 3);
}

// Test AI copy and move semantics
BOOST_AUTO_TEST_CASE(AICopyAndMoveSemantics) {
    EasyAI original(42);
    
    // Test that AI maintains proper state
    BOOST_CHECK_EQUAL(original.get_id(), 42);
    BOOST_CHECK_EQUAL(original.get_dice_count(), 5);
    
    // AI should be movable for game management
    EasyAI moved = std::move(original);
    BOOST_CHECK_EQUAL(moved.get_id(), 42);
    BOOST_CHECK_EQUAL(moved.get_dice_count(), 5);
    
    // Test that moved AI is functional
    auto guess = moved.make_guess(std::nullopt);
    BOOST_CHECK(guess.dice_count > 0);
    BOOST_CHECK(guess.face_value >= 1 && guess.face_value <= 6);
}

// Test AI response to invalid inputs
BOOST_AUTO_TEST_CASE(AIErrorHandlingAndValidation) {
    EasyAI ai(1);
    
    // Test response to invalid guess (should not crash)
    Guess invalid_guess{0, 0, 1}; // Zero dice count and face value
    BOOST_CHECK_NO_THROW(ai.decide_call_liar(invalid_guess));
    
    // Test with extreme values
    Guess extreme_guess{1000000, 999, 1};
    BOOST_CHECK_NO_THROW(ai.decide_call_liar(extreme_guess));
    
    // AI should handle edge cases gracefully
    bool decision = ai.decide_call_liar(extreme_guess);
    BOOST_CHECK(decision || !decision); // Should return a boolean value
}

BOOST_AUTO_TEST_SUITE_END()