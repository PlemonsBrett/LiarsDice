#define BOOST_TEST_MODULE PlayerAdvancedTests
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <liarsdice/core/player.hpp>
#include <liarsdice/ai/ai_player.hpp>
#include <algorithm>
#include <memory>
#include <chrono>

using namespace liarsdice::core;
using namespace liarsdice::ai;
namespace bdata = boost::unit_test::data;

BOOST_AUTO_TEST_SUITE(PlayerAdvancedTestSuite)

// BDD-style scenario tests
BOOST_AUTO_TEST_CASE(PlayerManagesDiceCollectionThroughoutGame) {
    // GIVEN: A new human player with ID 42
    HumanPlayer player(42, "Test Player");
    
    // WHEN: checking initial state
    // THEN: player has correct ID and no dice
    BOOST_CHECK_EQUAL(player.get_id(), 42);
    BOOST_CHECK_EQUAL(player.get_name(), "Test Player");
    BOOST_CHECK_EQUAL(player.get_dice_count(), 5); // Players start with 5 dice
    BOOST_CHECK(player.has_dice());
    
    // WHEN: rolling all dice
    BOOST_CHECK_NO_THROW(player.roll_dice());
    
    // THEN: all dice maintain valid values
    auto dice_values = player.get_dice_values();
    BOOST_CHECK_EQUAL(dice_values.size(), 5);
    
    for (auto value : dice_values) {
        BOOST_CHECK(value >= 1 && value <= 6);
    }
    
    // WHEN: removing dice systematically
    BOOST_CHECK(player.remove_die()); // 5 -> 4
    BOOST_CHECK_EQUAL(player.get_dice_count(), 4);
    
    BOOST_CHECK(player.remove_die()); // 4 -> 3
    BOOST_CHECK_EQUAL(player.get_dice_count(), 3);
    
    BOOST_CHECK(player.remove_die()); // 3 -> 2
    BOOST_CHECK_EQUAL(player.get_dice_count(), 2);
    
    BOOST_CHECK(player.remove_die()); // 2 -> 1
    BOOST_CHECK_EQUAL(player.get_dice_count(), 1);
    
    BOOST_CHECK(player.remove_die()); // 1 -> 0
    
    // THEN: player becomes eliminated
    BOOST_CHECK_EQUAL(player.get_dice_count(), 0);
    BOOST_CHECK(!player.has_dice());
    BOOST_CHECK(!player.remove_die()); // Can't remove from empty
}

// Property-based testing with data-driven tests
BOOST_DATA_TEST_CASE(PlayerIdValidation,
    bdata::make({1u, 2u, 5u, 10u, 100u, 999u, 1000u}),
    player_id) {
    
    // All positive IDs should be valid
    BOOST_CHECK_NO_THROW(HumanPlayer(player_id, "Test"));
    HumanPlayer player(player_id, "Test");
    BOOST_CHECK_EQUAL(player.get_id(), player_id);
}

// Advanced dice counting tests
BOOST_AUTO_TEST_CASE(PlayerDiceValueCountingAndStatistics) {
    EasyAI player(1);
    
    // Roll dice to get random values
    player.roll_dice();
    auto dice_values = player.get_dice_values();
    BOOST_CHECK_EQUAL(dice_values.size(), 5);
    
    // Test counting for each possible value
    for (unsigned int target_value = 1; target_value <= 6; ++target_value) {
        auto count = player.count_dice_with_value(target_value);
        auto expected_count = static_cast<size_t>(
            std::count(dice_values.begin(), dice_values.end(), target_value));
        
        BOOST_CHECK_EQUAL(count, expected_count);
    }
    
    // Sum of all counts should equal total dice
    size_t total_counted = 0;
    for (unsigned int value = 1; value <= 6; ++value) {
        total_counted += player.count_dice_with_value(value);
    }
    BOOST_CHECK_EQUAL(total_counted, dice_values.size());
    
    // Invalid value counting should return 0
    BOOST_CHECK_EQUAL(player.count_dice_with_value(0), 0);
    BOOST_CHECK_EQUAL(player.count_dice_with_value(7), 0);
    BOOST_CHECK_EQUAL(player.count_dice_with_value(999), 0);
}

// Performance benchmarks
BOOST_AUTO_TEST_CASE(PlayerPerformanceBenchmarks) {
    EasyAI player(1);
    const int iterations = 10000;
    
    // Benchmark dice operations
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        player.roll_dice();
        auto values = player.get_dice_values();
        auto count = player.count_dice_with_value(3);
        (void)values; (void)count; // Suppress unused warnings
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    BOOST_CHECK(duration.count() < 5000000); // Less than 5 seconds
    
    BOOST_TEST_MESSAGE("Player operations performance: " << duration.count() 
                      << " microseconds for " << iterations << " operations");
}

// Integration test scenarios
BOOST_AUTO_TEST_CASE(PlayerIntegrationScenarios) {
    // Multiple players with different dice counts
    std::vector<std::shared_ptr<Player>> players;
    
    // Create different types of players
    players.push_back(std::make_shared<EasyAI>(1));
    players.push_back(std::make_shared<MediumAI>(2));
    players.push_back(std::make_shared<HardAI>(3));
    
    // Verify each player has correct setup
    for (size_t i = 0; i < players.size(); ++i) {
        auto& player = players[i];
        BOOST_CHECK_EQUAL(player->get_id(), i + 1);
        BOOST_CHECK_EQUAL(player->get_dice_count(), 5); // All start with 5
        BOOST_CHECK(player->has_dice());
    }
    
    // Test AI decision making
    for (auto& player : players) {
        if (auto ai = std::dynamic_pointer_cast<EasyAI>(player)) {
            // Test first guess (no previous guess)
            auto guess = ai->make_guess(std::nullopt);
            BOOST_CHECK(guess.dice_count > 0);
            BOOST_CHECK(guess.face_value >= 1 && guess.face_value <= 6);
            BOOST_CHECK_EQUAL(guess.player_id, ai->get_id());
        }
    }
}

// Player elimination simulation
BOOST_AUTO_TEST_CASE(PlayerEliminationSimulation) {
    EasyAI player(1);
    
    BOOST_CHECK(player.has_dice());
    BOOST_CHECK_EQUAL(player.get_dice_count(), 5);
    
    // Lose dice one by one
    std::vector<size_t> dice_counts;
    while (player.has_dice()) {
        dice_counts.push_back(player.get_dice_count());
        BOOST_CHECK(player.remove_die());
    }
    
    // Should have tracked elimination: 5 -> 4 -> 3 -> 2 -> 1 -> 0
    std::vector<size_t> expected = {5, 4, 3, 2, 1};
    BOOST_CHECK_EQUAL_COLLECTIONS(dice_counts.begin(), dice_counts.end(),
                                 expected.begin(), expected.end());
    
    BOOST_CHECK(!player.has_dice());
    BOOST_CHECK_EQUAL(player.get_dice_count(), 0);
}

// Edge cases and boundary conditions
BOOST_AUTO_TEST_CASE(PlayerEdgeCases) {
    EasyAI player(1);
    
    // Test maximum practical dice count manipulation
    // Start with 5, add more
    for (int i = 0; i < 10; ++i) {
        player.add_die();
    }
    
    BOOST_CHECK_EQUAL(player.get_dice_count(), 15); // 5 + 10
    BOOST_CHECK(player.has_dice());
    
    // Verify all dice have valid values
    auto values = player.get_dice_values();
    BOOST_CHECK_EQUAL(values.size(), 15);
    
    for (auto value : values) {
        BOOST_CHECK(value >= 1 && value <= 6);
    }
    
    // Rapid add/remove cycles
    for (int cycle = 0; cycle < 50; ++cycle) {
        size_t initial_count = player.get_dice_count();
        
        player.add_die();
        BOOST_CHECK_EQUAL(player.get_dice_count(), initial_count + 1);
        BOOST_CHECK(player.has_dice());
        
        BOOST_CHECK(player.remove_die());
        BOOST_CHECK_EQUAL(player.get_dice_count(), initial_count);
    }
}

// AI Strategy Testing
BOOST_AUTO_TEST_CASE(AIStrategyBehaviorTesting) {
    EasyAI easy(1);
    MediumAI medium(2); 
    HardAI hard(3);
    
    // Test AI names and configuration
    BOOST_CHECK_EQUAL(easy.get_name(), "Easy AI 1");
    BOOST_CHECK_EQUAL(medium.get_name(), "Medium AI 2");
    BOOST_CHECK_EQUAL(hard.get_name(), "Hard AI 3");
    
    // Test decision making with previous guess
    Guess prev_guess{3, 4, 1}; // 3 fours from player 1
    
    auto easy_response = easy.make_guess(prev_guess);
    auto medium_response = medium.make_guess(prev_guess);
    auto hard_response = hard.make_guess(prev_guess);
    
    // All responses should be valid (higher than previous)
    auto validate_guess = [&prev_guess](const Guess& response) {
        return (response.face_value > prev_guess.face_value) ||
               (response.face_value == prev_guess.face_value && 
                response.dice_count > prev_guess.dice_count) ||
               (response.face_value < prev_guess.face_value && 
                response.dice_count > prev_guess.dice_count);
    };
    
    BOOST_CHECK(validate_guess(easy_response));
    BOOST_CHECK(validate_guess(medium_response));
    BOOST_CHECK(validate_guess(hard_response));
    
    // Test liar calling decision
    Guess high_guess{20, 6, 1}; // Obviously false guess
    
    bool easy_calls = easy.decide_call_liar(high_guess);
    bool medium_calls = medium.decide_call_liar(high_guess);
    bool hard_calls = hard.decide_call_liar(high_guess);
    
    // At least one AI should call liar on an obviously false guess
    BOOST_CHECK(easy_calls || medium_calls || hard_calls);
}

// Stress testing with many players
BOOST_AUTO_TEST_CASE(PlayerStressTesting) {
    const int num_players = 100;
    std::vector<std::shared_ptr<Player>> players;
    
    // Create many players
    for (int i = 1; i <= num_players; ++i) {
        auto ai_type = i % 3;
        switch (ai_type) {
            case 0:
                players.push_back(std::make_shared<EasyAI>(i));
                break;
            case 1:
                players.push_back(std::make_shared<MediumAI>(i));
                break;
            case 2:
                players.push_back(std::make_shared<HardAI>(i));
                break;
        }
    }
    
    // Perform operations on all players
    for (auto& player : players) {
        // Roll dice
        BOOST_CHECK_NO_THROW(player->roll_dice());
        
        // Check dice count
        BOOST_CHECK_EQUAL(player->get_dice_count(), 5);
        
        // Get dice values
        auto values = player->get_dice_values();
        BOOST_CHECK_EQUAL(values.size(), 5);
        
        // Count dice
        for (unsigned int face = 1; face <= 6; ++face) {
            auto count = player->count_dice_with_value(face);
            BOOST_CHECK(count <= 5);
        }
        
        // Verify all values are valid
        for (auto value : values) {
            BOOST_CHECK(value >= 1 && value <= 6);
        }
    }
    
    BOOST_TEST_MESSAGE("Successfully stress tested " << num_players << " players");
}

// Copy and move semantics for players
BOOST_AUTO_TEST_CASE(PlayerCopyAndMoveSemantics) {
    // Players should be movable but not copyable (game entities)
    EasyAI original(42);
    
    // Move construction
    EasyAI moved = std::move(original);
    BOOST_CHECK_EQUAL(moved.get_id(), 42);
    
    // Test that moved player is functional
    BOOST_CHECK_EQUAL(moved.get_dice_count(), 5);
    BOOST_CHECK_NO_THROW(moved.roll_dice());
}

BOOST_AUTO_TEST_SUITE_END()