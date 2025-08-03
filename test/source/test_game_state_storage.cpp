#define BOOST_TEST_MODULE GameStateStorageTests
#include <boost/test/unit_test.hpp>
#include <liarsdice/core/game_state_storage.hpp>
#include <algorithm>
#include <random>

using namespace liarsdice::core;

BOOST_AUTO_TEST_SUITE(CompactGameStateTestSuite)

BOOST_AUTO_TEST_CASE(CompactGameStateConstruction) {
    CompactGameState state;
    
    // Check default values
    BOOST_CHECK_EQUAL(state.dice_bits, 0);
    BOOST_CHECK_EQUAL(state.player_state.points, 0);
    BOOST_CHECK_EQUAL(state.player_state.dice_count, 0);
    BOOST_CHECK_EQUAL(state.player_state.is_active, 0);
    BOOST_CHECK_EQUAL(state.last_action.action_type, 0);
    
    // Check size is compact
    BOOST_CHECK_LE(sizeof(CompactGameState), 8); // Should be very small
}

BOOST_AUTO_TEST_CASE(DiceValueStorage) {
    CompactGameState state;
    
    // Test setting individual dice values
    state.set_dice_value(0, 1);
    state.set_dice_value(1, 2);
    state.set_dice_value(2, 3);
    state.set_dice_value(3, 4);
    state.set_dice_value(4, 5);
    
    // Verify values
    BOOST_CHECK_EQUAL(state.get_dice_value(0), 1);
    BOOST_CHECK_EQUAL(state.get_dice_value(1), 2);
    BOOST_CHECK_EQUAL(state.get_dice_value(2), 3);
    BOOST_CHECK_EQUAL(state.get_dice_value(3), 4);
    BOOST_CHECK_EQUAL(state.get_dice_value(4), 5);
    
    // Test edge cases
    state.set_dice_value(0, 6);
    BOOST_CHECK_EQUAL(state.get_dice_value(0), 6);
    
    // Test boundary values
    state.set_dice_value(10, 3); // Invalid index
    BOOST_CHECK_EQUAL(state.get_dice_value(10), 0); // Should return 0
}

BOOST_AUTO_TEST_CASE(AllDiceOperations) {
    CompactGameState state;
    
    // Test set_all_dice
    std::vector<std::uint8_t> dice = {6, 5, 4, 3, 2};
    state.set_all_dice(dice);
    
    auto retrieved = state.get_all_dice();
    for (std::size_t i = 0; i < dice.size(); ++i) {
        BOOST_CHECK_EQUAL(retrieved[i], dice[i]);
    }
    
    // Check dice count was set
    BOOST_CHECK_EQUAL(state.player_state.dice_count, 5);
}

BOOST_AUTO_TEST_CASE(PlayerStateManipulation) {
    CompactGameState state;
    
    // Test player state fields
    state.player_state.points = 5;
    state.player_state.dice_count = 4;
    state.player_state.is_active = 1;
    
    BOOST_CHECK_EQUAL(state.player_state.points, 5);
    BOOST_CHECK_EQUAL(state.player_state.dice_count, 4);
    BOOST_CHECK_EQUAL(state.player_state.is_active, 1);
    BOOST_CHECK(!state.is_eliminated());
    
    // Test elimination
    state.player_state.points = 0;
    BOOST_CHECK(state.is_eliminated());
}

BOOST_AUTO_TEST_CASE(Serialization) {
    CompactGameState state;
    
    // Set up test state
    state.set_dice_value(0, 6);
    state.set_dice_value(1, 5);
    state.set_dice_value(2, 4);
    state.player_state.points = 3;
    state.player_state.dice_count = 3;
    state.player_state.is_active = 1;
    state.last_action.action_type = 1;
    state.last_action.dice_count = 2;
    state.last_action.face_value = 4;
    
    // Serialize and deserialize
    std::uint32_t serialized = state.serialize();
    CompactGameState deserialized = CompactGameState::deserialize(serialized);
    
    // Verify all fields match
    BOOST_CHECK_EQUAL(deserialized.get_dice_value(0), 6);
    BOOST_CHECK_EQUAL(deserialized.get_dice_value(1), 5);
    BOOST_CHECK_EQUAL(deserialized.get_dice_value(2), 4);
    BOOST_CHECK_EQUAL(deserialized.player_state.points, 3);
    BOOST_CHECK_EQUAL(deserialized.player_state.dice_count, 3);
    BOOST_CHECK_EQUAL(deserialized.player_state.is_active, 1);
    BOOST_CHECK_EQUAL(deserialized.last_action.action_type, 1);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(GameStateStorageTestSuite)

BOOST_AUTO_TEST_CASE(StorageOperations) {
    GameStateStorage storage;
    
    // Test initial state
    BOOST_CHECK_EQUAL(storage.size(), 0);
    
    // Add player states
    CompactGameState state1;
    state1.player_state.points = 5;
    storage.store_player_state(1, state1);
    
    CompactGameState state2;
    state2.player_state.points = 4;
    storage.store_player_state(2, state2);
    
    BOOST_CHECK_EQUAL(storage.size(), 2);
    
    // Retrieve states
    auto* retrieved1 = storage.get_player_state(1);
    BOOST_REQUIRE(retrieved1 != nullptr);
    BOOST_CHECK_EQUAL(retrieved1->player_state.points, 5);
    
    auto* retrieved2 = storage.get_player_state(2);
    BOOST_REQUIRE(retrieved2 != nullptr);
    BOOST_CHECK_EQUAL(retrieved2->player_state.points, 4);
    
    // Non-existent player
    BOOST_CHECK(storage.get_player_state(99) == nullptr);
}

BOOST_AUTO_TEST_CASE(ActivePlayerManagement) {
    GameStateStorage storage;
    
    // Add active players
    storage.add_active_player(1);
    storage.add_active_player(2);
    storage.add_active_player(3);
    
    BOOST_CHECK(storage.is_player_active(1));
    BOOST_CHECK(storage.is_player_active(2));
    BOOST_CHECK(storage.is_player_active(3));
    BOOST_CHECK(!storage.is_player_active(4));
    
    // Remove player
    storage.remove_active_player(2);
    BOOST_CHECK(!storage.is_player_active(2));
    
    // Check active set
    const auto& active = storage.get_active_players();
    BOOST_CHECK_EQUAL(active.size(), 2);
    BOOST_CHECK(active.find(1) != active.end());
    BOOST_CHECK(active.find(3) != active.end());
}

BOOST_AUTO_TEST_CASE(ClearOperation) {
    GameStateStorage storage;
    
    // Add data
    CompactGameState state;
    storage.store_player_state(1, state);
    storage.add_active_player(1);
    
    BOOST_CHECK_EQUAL(storage.size(), 1);
    BOOST_CHECK(storage.is_player_active(1));
    
    // Clear
    storage.clear();
    
    BOOST_CHECK_EQUAL(storage.size(), 0);
    BOOST_CHECK(!storage.is_player_active(1));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(GameHistoryTestSuite)

BOOST_AUTO_TEST_CASE(HistoryRecording) {
    GameHistory history(5);
    
    // Test empty history
    BOOST_CHECK(history.empty());
    BOOST_CHECK_EQUAL(history.size(), 0);
    
    // Record states
    for (int i = 0; i < 3; ++i) {
        CompactGameState state;
        state.player_state.points = static_cast<std::uint8_t>(5 - i);
        history.record_state(state);
    }
    
    BOOST_CHECK(!history.empty());
    BOOST_CHECK_EQUAL(history.size(), 3);
}

BOOST_AUTO_TEST_CASE(HistoryRetrieval) {
    GameHistory history(10);
    
    // Record states with different points
    for (int i = 0; i < 5; ++i) {
        CompactGameState state;
        state.player_state.points = static_cast<std::uint8_t>(i);
        history.record_state(state);
    }
    
    // Test retrieval
    auto* recent = history.get_state(0); // Most recent
    BOOST_REQUIRE(recent != nullptr);
    BOOST_CHECK_EQUAL(recent->player_state.points, 4);
    
    auto* older = history.get_state(2); // 2 steps back
    BOOST_REQUIRE(older != nullptr);
    BOOST_CHECK_EQUAL(older->player_state.points, 2);
    
    // Out of bounds
    BOOST_CHECK(history.get_state(10) == nullptr);
}

BOOST_AUTO_TEST_CASE(CircularBufferBehavior) {
    GameHistory history(3); // Small buffer
    
    // Fill beyond capacity
    for (int i = 0; i < 5; ++i) {
        CompactGameState state;
        state.player_state.points = static_cast<std::uint8_t>(i);
        history.record_state(state);
    }
    
    // Should only have last 3
    BOOST_CHECK_EQUAL(history.size(), 3);
    
    // Verify oldest entries were removed
    auto* recent = history.get_state(0);
    BOOST_REQUIRE(recent != nullptr);
    BOOST_CHECK_EQUAL(recent->player_state.points, 4); // Latest
    
    auto* oldest = history.get_state(2);
    BOOST_REQUIRE(oldest != nullptr);
    BOOST_CHECK_EQUAL(oldest->player_state.points, 2); // 3 back from 4
}

BOOST_AUTO_TEST_CASE(DiceFrequencyAnalysis) {
    GameHistory history(10);
    
    // Create states with known dice
    for (int i = 0; i < 3; ++i) {
        CompactGameState state;
        state.set_dice_value(0, 1);
        state.set_dice_value(1, 1);
        state.set_dice_value(2, 6);
        state.player_state.dice_count = 3;
        history.record_state(state);
    }
    
    auto freq = history.get_dice_frequency(3);
    BOOST_CHECK_EQUAL(freq[1], 6); // 2 dice showing 1, 3 times
    BOOST_CHECK_EQUAL(freq[6], 3); // 1 die showing 6, 3 times
    BOOST_CHECK_EQUAL(freq[2], 0); // No 2s
}

BOOST_AUTO_TEST_CASE(AverageDiceCount) {
    GameHistory history(10);
    
    // States with different dice counts
    CompactGameState state1;
    state1.player_state.dice_count = 5;
    history.record_state(state1);
    
    CompactGameState state2;
    state2.player_state.dice_count = 3;
    history.record_state(state2);
    
    CompactGameState state3;
    state3.player_state.dice_count = 4;
    history.record_state(state3);
    
    double avg = history.get_average_dice_count(3);
    BOOST_CHECK_CLOSE(avg, 4.0, 0.01); // (5+3+4)/3 = 4
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(GameStateAllocatorTestSuite)

BOOST_AUTO_TEST_CASE(AllocatorBasicOperations) {
    GameStateAllocator<CompactGameState> allocator;
    
    // Allocate single object
    auto* state = allocator.allocate(1);
    BOOST_REQUIRE(state != nullptr);
    
    // Construct in-place
    new (state) CompactGameState();
    state->player_state.points = 5;
    
    // Verify
    BOOST_CHECK_EQUAL(state->player_state.points, 5);
    
    // Destroy and deallocate
    state->~CompactGameState();
    allocator.deallocate(state, 1);
}

BOOST_AUTO_TEST_CASE(AllocatorStressTest) {
    GameStateAllocator<CompactGameState> allocator;
    
    // Allocate and deallocate many objects
    std::vector<CompactGameState*> states;
    states.reserve(100);
    
    // Allocate
    for (int i = 0; i < 100; ++i) {
        auto* state = allocator.allocate(1);
        new (state) CompactGameState();
        state->player_state.points = static_cast<std::uint8_t>(i % 6);
        states.push_back(state);
    }
    
    // Verify some
    BOOST_CHECK_EQUAL(states[0]->player_state.points, 0);
    BOOST_CHECK_EQUAL(states[5]->player_state.points, 5);
    
    // Deallocate
    for (auto* state : states) {
        state->~CompactGameState();
        allocator.deallocate(state, 1);
    }
}

BOOST_AUTO_TEST_SUITE_END()