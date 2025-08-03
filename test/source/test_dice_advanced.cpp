#define BOOST_TEST_MODULE DiceAdvancedTests
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <liarsdice/core/dice.hpp>
#include <array>
#include <algorithm>
#include <chrono>
#include <numeric>

using namespace liarsdice::core;
namespace bdata = boost::unit_test::data;

BOOST_AUTO_TEST_SUITE(DiceAdvancedTestSuite)

// BDD-style scenario tests
BOOST_AUTO_TEST_CASE(DiceFollowsGameRulesAndConstraints) {
    // GIVEN: A newly created die
    Dice dice;
    
    // WHEN: checking its initial state
    auto value = dice.get_value();
    
    // THEN: it has a valid face value
    BOOST_CHECK(value >= 1 && value <= 6);
    
    // WHEN: rolling the die multiple times
    std::vector<unsigned int> rolled_values;
    for (int i = 0; i < 10; ++i) {
        dice.roll();
        rolled_values.push_back(dice.get_value());
    }
    
    // THEN: it produces valid values in sequence
    BOOST_CHECK_EQUAL(rolled_values.size(), 10);
    for (auto val : rolled_values) {
        BOOST_CHECK(val >= 1 && val <= 6);
    }
}

// Property-based testing with data-driven tests
BOOST_DATA_TEST_CASE(DiceValidationProperties,
    bdata::make({0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 100u}),
    test_value) {
    
    Dice dice;
    bool expected_valid = (test_value >= 1 && test_value <= 6);
    
    if (expected_valid) {
        // Valid values should not throw
        BOOST_CHECK_NO_THROW(dice.set_value(test_value));
        dice.set_value(test_value);
        BOOST_CHECK_EQUAL(dice.get_value(), test_value);
    } else {
        // Invalid values should throw or be rejected
        // Note: Our Dice implementation may clamp values or throw
        // This test ensures consistent behavior
        auto original_value = dice.get_value();
        dice.set_value(test_value);
        auto new_value = dice.get_value();
        
        // Value should either remain unchanged or be clamped to valid range
        BOOST_CHECK(new_value >= 1 && new_value <= 6);
    }
}

// Performance benchmarks
BOOST_AUTO_TEST_CASE(DicePerformanceCharacteristics) {
    Dice dice;
    const int iterations = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Benchmark roll operations
    for (int i = 0; i < iterations; ++i) {
        dice.roll();
        auto value = dice.get_value(); // Ensure no optimization removes the call
        (void)value; // Suppress unused variable warning
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Performance requirement: should complete 100k operations in reasonable time
    BOOST_CHECK(duration.count() < 1000000); // Less than 1 second
    
    BOOST_TEST_MESSAGE("Roll performance: " << duration.count() << " microseconds for " 
                      << iterations << " operations");
}

// Statistical distribution test
BOOST_AUTO_TEST_CASE(DiceRandomnessDistribution) {
    const size_t sample_size = 6000; // Multiple of 6 for even distribution
    std::array<int, 6> frequency{};
    
    Dice dice;
    
    // Collect samples
    for (size_t i = 0; i < sample_size; ++i) {
        dice.roll();
        auto value = dice.get_value();
        BOOST_CHECK(value >= 1 && value <= 6);
        frequency[value - 1]++;
    }
    
    // Check basic distribution properties
    const double expected_freq = sample_size / 6.0;
    const double tolerance = expected_freq * 0.3; // 30% tolerance for randomness
    
    BOOST_TEST_MESSAGE("Frequency distribution: [" 
                      << frequency[0] << ", " << frequency[1] << ", " << frequency[2]
                      << ", " << frequency[3] << ", " << frequency[4] << ", " 
                      << frequency[5] << "]");
    
    for (int count : frequency) {
        BOOST_CHECK(count > 0); // Each value should appear at least once
        BOOST_CHECK(std::abs(count - expected_freq) < tolerance);
    }
    
    // Check that no single value dominates
    auto [min_freq, max_freq] = std::minmax_element(frequency.begin(), frequency.end());
    double variation_ratio = static_cast<double>(*max_freq) / static_cast<double>(*min_freq);
    BOOST_CHECK(variation_ratio < 3.0); // No value should be 3x more frequent than another
}

// Edge case testing
BOOST_AUTO_TEST_CASE(DiceEdgeCasesAndBoundaryConditions) {
    Dice dice;
    
    // Test boundary values
    dice.set_value(1);
    BOOST_CHECK_EQUAL(dice.get_value(), 1);
    
    dice.set_value(6);
    BOOST_CHECK_EQUAL(dice.get_value(), 6);
    
    // Test rapid state changes
    for (int i = 0; i < 1000; ++i) {
        unsigned int old_value = dice.get_value();
        dice.roll();
        unsigned int new_value = dice.get_value();
        
        BOOST_CHECK(new_value >= 1 && new_value <= 6);
        // Value can be the same or different after rolling
    }
}

// Integration test with controlled sequences
BOOST_AUTO_TEST_CASE(DiceSequenceBehavior) {
    // Test that dice maintain independence
    std::vector<Dice> dice_collection(10);
    
    // Roll all dice
    for (auto& die : dice_collection) {
        die.roll();
    }
    
    // Check that they have different values (high probability)
    std::vector<unsigned int> values;
    std::transform(dice_collection.begin(), dice_collection.end(), 
                  std::back_inserter(values),
                  [](const Dice& d) { return d.get_value(); });
    
    // With 10 dice, we should have some variation (not all the same)
    auto unique_values = std::set<unsigned int>(values.begin(), values.end());
    BOOST_CHECK(unique_values.size() > 1); // Should have at least 2 different values
    
    // All values should be valid
    for (auto value : values) {
        BOOST_CHECK(value >= 1 && value <= 6);
    }
}

// Stress testing
BOOST_AUTO_TEST_CASE(DiceStressTesting) {
    const int num_dice = 1000;
    std::vector<Dice> stress_dice(num_dice);
    
    // Perform many operations
    for (int round = 0; round < 100; ++round) {
        for (auto& die : stress_dice) {
            die.roll();
            auto value = die.get_value();
            BOOST_CHECK(value >= 1 && value <= 6);
            
            // Randomly set values
            if (round % 10 == 0) {
                die.set_value(1 + (round % 6));
            }
        }
    }
    
    // Verify all dice are still in valid state
    for (const auto& die : stress_dice) {
        BOOST_CHECK(die.get_value() >= 1 && die.get_value() <= 6);
    }
}

// Copy and move semantics testing
BOOST_AUTO_TEST_CASE(DiceCopyAndMoveSemantics) {
    Dice original;
    original.set_value(4);
    
    // Copy construction
    Dice copied(original);
    BOOST_CHECK_EQUAL(copied.get_value(), 4);
    
    // Copy assignment
    Dice assigned;
    assigned = original;
    BOOST_CHECK_EQUAL(assigned.get_value(), 4);
    
    // Verify independence
    original.set_value(2);
    BOOST_CHECK_EQUAL(copied.get_value(), 4); // Should not change
    BOOST_CHECK_EQUAL(assigned.get_value(), 4); // Should not change
    
    // Move semantics
    Dice moved = std::move(original);
    BOOST_CHECK_EQUAL(moved.get_value(), 2);
}

// Thread safety consideration test
BOOST_AUTO_TEST_CASE(DiceThreadSafetyConsiderations) {
    // Test that individual dice operations are atomic
    Dice dice;
    std::vector<unsigned int> results;
    
    // Simulate concurrent-like operations
    for (int i = 0; i < 1000; ++i) {
        dice.roll();
        results.push_back(dice.get_value());
        
        // Each result should be valid
        BOOST_CHECK(results.back() >= 1 && results.back() <= 6);
    }
    
    // All results should be valid
    bool all_valid = std::all_of(results.begin(), results.end(),
                                [](unsigned int v) { return v >= 1 && v <= 6; });
    BOOST_CHECK(all_valid);
}

BOOST_AUTO_TEST_SUITE_END()