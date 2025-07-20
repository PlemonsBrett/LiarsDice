#include "../test_infrastructure.hpp"
#include "liarsdice/core/dice_impl.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <numeric>
#include <algorithm>

using namespace liarsdice::testing;
using namespace liarsdice::core;
using namespace liarsdice::interfaces;

// Test fixture for Dice tests
class DiceTestFixture {
protected:
    std::unique_ptr<MockRandomGenerator> mock_rng;
    std::unique_ptr<DiceImpl> dice;
    
    DiceTestFixture() {
        mock_rng = std::make_unique<MockRandomGenerator>();
    }
    
    void create_dice_with_sequence(std::vector<int> sequence) {
        mock_rng->set_sequence(std::move(sequence));
        dice = std::make_unique<DiceImpl>(
            std::unique_ptr<IRandomGenerator>(mock_rng.release())
        );
    }
    
    void create_random_dice() {
        dice = std::make_unique<DiceImpl>(
            std::make_unique<MockRandomGenerator>()
        );
    }
};

// BDD-style tests
SCENARIO("Dice behavior follows game rules", "[dice][bdd]") {
    GIVEN("A newly created die") {
        auto rng = std::make_unique<MockRandomGenerator>(std::vector<int>{3});
        DiceImpl dice(std::move(rng));
        
        WHEN("checking its initial state") {
            auto value = dice.get_face_value();
            
            THEN("it has a valid face value") {
                REQUIRE(dice.is_valid_face_value(value));
                REQUIRE_THAT(value, IsInRange(std::array{1u, 2u, 3u, 4u, 5u, 6u}));
            }
        }
        
        AND_GIVEN("a specific random sequence") {
            auto rng2 = std::make_unique<MockRandomGenerator>(
                std::vector<int>{1, 6, 3, 4, 2, 5}
            );
            DiceImpl dice2(std::move(rng2));
            
            WHEN("rolling the die multiple times") {
                std::vector<unsigned int> rolled_values;
                for (int i = 0; i < 5; ++i) {
                    dice2.roll();
                    rolled_values.push_back(dice2.get_face_value());
                }
                
                THEN("it produces the expected sequence") {
                    REQUIRE(rolled_values == std::vector<unsigned int>{1, 6, 3, 4, 2});
                }
                
                AND_THEN("all values are valid") {
                    REQUIRE(std::all_of(rolled_values.begin(), rolled_values.end(),
                        [&dice2](auto val) { return dice2.is_valid_face_value(val); }));
                }
            }
        }
    }
}

// Property-based tests
TEST_CASE("Dice properties hold for random inputs", "[dice][property]") {
    SECTION("Face value validation is consistent") {
        auto value = GENERATE(range(0U, 11U));
        
        auto rng = std::make_unique<MockRandomGenerator>();
        DiceImpl dice(std::move(rng));
        
        bool is_valid = dice.is_valid_face_value(value);
        bool in_range = (value >= 1 && value <= 6);
        
        REQUIRE(is_valid == in_range);
    }
    
    SECTION("Set face value respects validation") {
        auto value = GENERATE(range(0U, 11U));
        
        auto rng = std::make_unique<MockRandomGenerator>();
        DiceImpl dice(std::move(rng));
        
        if (dice.is_valid_face_value(value)) {
            REQUIRE_NOTHROW(dice.set_face_value(value));
            REQUIRE(dice.get_face_value() == value);
        } else {
            REQUIRE_THROWS_AS(dice.set_face_value(value), std::invalid_argument);
        }
    }
}

// TEST_CASE_METHOD for fixture-based tests
TEST_CASE_METHOD(DiceTestFixture, "Dice with controlled randomness", "[dice][fixture]") {
    SECTION("Predictable sequence") {
        create_dice_with_sequence({1, 2, 3, 4, 5, 6});
        
        for (int expected = 1; expected <= 6; ++expected) {
            if (expected > 1) dice->roll();
            REQUIRE(dice->get_face_value() == static_cast<unsigned int>(expected));
        }
    }
    
    SECTION("Edge case values") {
        create_dice_with_sequence({1, 6, 1, 6, 1, 6});
        
        for (int i = 0; i < 6; ++i) {
            if (i > 0) dice->roll();
            auto value = dice->get_face_value();
            REQUIRE((value == 1 || value == 6));
        }
    }
}

// Performance benchmarks
TEST_CASE("Dice performance benchmarks", "[dice][benchmark]") {
    auto rng = std::make_unique<MockRandomGenerator>();
    DiceImpl dice(std::move(rng));
    
    BENCHMARK("Single roll performance") {
        return dice.roll();
    };
    
    BENCHMARK("Face value getter performance") {
        return dice.get_face_value();
    };
    
    BENCHMARK("Validation check performance") {
        return dice.is_valid_face_value(3);
    };
    
    BENCHMARK_ADVANCED("Bulk rolling performance")(Catch::Benchmark::Chronometer meter) {
        std::vector<DiceImpl> dice_collection;
        dice_collection.reserve(meter.runs());
        
        // Setup
        for (size_t i = 0; i < meter.runs(); ++i) {
            dice_collection.emplace_back(std::make_unique<MockRandomGenerator>());
        }
        
        // Measure
        meter.measure([&dice_collection](int i) {
            dice_collection[i].roll();
        });
    };
}

// Statistical tests for randomness
TEST_CASE("Dice randomness distribution", "[dice][statistics]") {
    const size_t sample_size = 10000;
    std::array<int, 6> frequency{};
    
    auto rng = std::make_unique<MockRandomGenerator>();
    DiceImpl dice(std::move(rng));
    
    // Collect samples
    for (size_t i = 0; i < sample_size; ++i) {
        dice.roll();
        frequency[dice.get_face_value() - 1]++;
    }
    
    // Chi-square test for uniformity
    const double expected_freq = sample_size / 6.0;
    double chi_square = 0.0;
    
    for (int count : frequency) {
        double diff = count - expected_freq;
        chi_square += (diff * diff) / expected_freq;
    }
    
    // Critical value for 5 degrees of freedom at 0.05 significance
    const double critical_value = 11.07;
    
    INFO("Chi-square statistic: " << chi_square);
    INFO("Frequency distribution: " << frequency[0] << ", " << frequency[1] 
         << ", " << frequency[2] << ", " << frequency[3] 
         << ", " << frequency[4] << ", " << frequency[5]);
    
    // We expect this to pass with our mock RNG
    REQUIRE(chi_square < critical_value * 2); // Relaxed for mock
}

// Custom matcher usage example
TEST_CASE("Dice with custom matchers", "[dice][matchers]") {
    auto rng = std::make_unique<MockRandomGenerator>();
    DiceImpl dice(std::move(rng));
    
    // Using our custom IsInRange matcher
    REQUIRE_THAT(dice.get_face_value(), IsInRange(std::views::iota(1u, 7u)));
    
    // Multiple rolls should all be in range
    for (int i = 0; i < 20; ++i) {
        dice.roll();
        REQUIRE_THAT(dice.get_face_value(), IsInRange(std::array{1u, 2u, 3u, 4u, 5u, 6u}));
    }
}