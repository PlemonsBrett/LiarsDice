#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "liarsdice/core/dice_impl.hpp"
#include "liarsdice/interfaces/i_random_generator.hpp"
#include <array>
#include <memory>
#include <random>
#include <vector>

using namespace liarsdice::core;
using namespace liarsdice::interfaces;

// Simple mock for testing
class TestRandomGenerator : public IRandomGenerator {
private:
  mutable std::vector<int> sequence_;
  mutable size_t index_ = 0;
  mutable std::mt19937 fallback_gen_{42}; // Fixed seed for reproducible tests

public:
  explicit TestRandomGenerator(std::vector<int> sequence = {}) : sequence_(std::move(sequence)) {}

  int generate(int min, int max) override {
    if (index_ < sequence_.size()) {
      return sequence_[index_++];
    }
    std::uniform_int_distribution<int> dist(min, max);
    return dist(fallback_gen_);
  }

  void seed(unsigned int seed) override {
    fallback_gen_.seed(seed);
    index_ = 0;
  }

  bool generate_bool() override { return generate(0, 1) == 1; }

  double generate_normalized() override { return static_cast<double>(generate(0, 1000)) / 1000.0; }

  void set_sequence(std::vector<int> sequence) {
    sequence_ = std::move(sequence);
    index_ = 0;
  }
};

// Custom matcher for valid dice values
class ValidDiceValueMatcher : public Catch::Matchers::MatcherGenericBase {
public:
  bool match(unsigned int value) const { return value >= 1 && value <= 6; }

  std::string describe() const override { return "is a valid dice value (1-6)"; }
};

auto IsValidDiceValue() { return ValidDiceValueMatcher{}; }

// BDD-style scenario tests
SCENARIO("Dice follows game rules and constraints", "[dice][bdd]") {
  GIVEN("A newly created die with predictable randomness") {
    auto rng = std::make_unique<TestRandomGenerator>(std::vector<int>{3, 6, 1, 4, 2, 5});
    DiceImpl dice(std::unique_ptr<IRandomGenerator>(rng.release()));

    WHEN("checking its initial state") {
      auto value = dice.get_face_value();

      THEN("it has a valid face value") {
        REQUIRE_THAT(value, IsValidDiceValue());
        REQUIRE(dice.is_valid_face_value(value));
      }
    }

    WHEN("rolling the die multiple times") {
      std::vector<unsigned int> rolled_values;
      for (int i = 0; i < 5; ++i) {
        dice.roll();
        rolled_values.push_back(dice.get_face_value());
      }

      THEN("it produces valid values in sequence") {
        REQUIRE(rolled_values.size() == 5);
        for (auto val : rolled_values) {
          REQUIRE_THAT(val, IsValidDiceValue());
        }
      }

      AND_THEN("the sequence matches expectations") {
        REQUIRE(rolled_values == std::vector<unsigned int>{6, 1, 4, 2, 5});
      }
    }
  }
}

// Property-based testing
TEST_CASE("Dice validation properties", "[dice][property]") {
  auto test_value = GENERATE(0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U);

  auto rng = std::make_unique<TestRandomGenerator>();
  DiceImpl dice(std::unique_ptr<IRandomGenerator>(rng.release()));

  bool is_valid = dice.is_valid_face_value(test_value);
  bool expected_valid = (test_value >= 1 && test_value <= 6);

  REQUIRE(is_valid == expected_valid);

  if (is_valid) {
    REQUIRE_NOTHROW(dice.set_face_value(test_value));
    REQUIRE(dice.get_face_value() == test_value);
  } else {
    REQUIRE_THROWS_AS(dice.set_face_value(test_value), std::invalid_argument);
  }
}

// Performance benchmarks
TEST_CASE("Dice performance characteristics", "[dice][benchmark]") {
  auto rng = std::make_unique<TestRandomGenerator>();
  DiceImpl dice(std::unique_ptr<IRandomGenerator>(rng.release()));

  BENCHMARK("Roll operation") {
    dice.roll();
    return dice.get_face_value();
  };

  BENCHMARK("Face value getter") { return dice.get_face_value(); };

  BENCHMARK("Validation check") { return dice.is_valid_face_value(3); };

  BENCHMARK("Set face value") {
    dice.set_face_value(4);
    return dice.get_face_value();
  };
}

// Exception safety tests
TEST_CASE("Dice exception safety", "[dice][exceptions]") {
  SECTION("Constructor with null RNG") {
    REQUIRE_THROWS_AS(DiceImpl(nullptr), std::invalid_argument);
  }

  SECTION("Invalid face value boundaries") {
    auto rng = std::make_unique<TestRandomGenerator>();
    DiceImpl dice(std::move(rng));

    REQUIRE_THROWS_WITH(dice.set_face_value(0),
                        Catch::Matchers::ContainsSubstring("Invalid face value"));

    REQUIRE_THROWS_WITH(dice.set_face_value(7),
                        Catch::Matchers::ContainsSubstring("Invalid face value"));
  }
}

// Statistical distribution test
TEST_CASE("Dice randomness distribution", "[dice][statistics]") {
  const size_t sample_size = 6000; // Multiple of 6 for even distribution
  std::array<int, 6> frequency{};

  auto rng = std::make_unique<TestRandomGenerator>(); // Uses fallback with fixed seed
  DiceImpl dice(std::move(rng));

  // Collect samples
  for (size_t i = 0; i < sample_size; ++i) {
    dice.roll();
    auto value = dice.get_face_value();
    REQUIRE_THAT(value, IsValidDiceValue());
    frequency[value - 1]++;
  }

  // Check basic distribution properties
  const double expected_freq = sample_size / 6.0;
  const double tolerance = expected_freq * 0.2; // 20% tolerance

  INFO("Frequency distribution: " << frequency[0] << ", " << frequency[1] << ", " << frequency[2]
                                  << ", " << frequency[3] << ", " << frequency[4] << ", "
                                  << frequency[5]);

  for (int count : frequency) {
    REQUIRE(count > 0); // Each value should appear at least once
    REQUIRE(std::abs(count - expected_freq) < tolerance);
  }
}

// Edge case testing
TEST_CASE("Dice edge cases and boundary conditions", "[dice][edge-cases]") {
  SECTION("Boundary values") {
    auto rng = std::make_unique<TestRandomGenerator>();
    DiceImpl dice(std::move(rng));

    // Test minimum valid value
    REQUIRE_NOTHROW(dice.set_face_value(1));
    REQUIRE(dice.get_face_value() == 1);

    // Test maximum valid value
    REQUIRE_NOTHROW(dice.set_face_value(6));
    REQUIRE(dice.get_face_value() == 6);
  }

  SECTION("Clone operation") {
    auto rng = std::make_unique<TestRandomGenerator>();
    DiceImpl dice(std::move(rng));

    // Our implementation doesn't support cloning (by design)
    REQUIRE_THROWS_WITH(dice.clone(),
                        Catch::Matchers::ContainsSubstring("Clone operation not supported"));
  }
}

// Integration test with sequences
TEST_CASE("Dice integration with controlled sequences", "[dice][integration]") {
  SECTION("Predictable sequence rollout") {
    std::vector<int> sequence{1, 2, 3, 4, 5, 6, 1, 2, 3};
    auto rng = std::make_unique<TestRandomGenerator>(sequence);
    DiceImpl dice(std::unique_ptr<IRandomGenerator>(rng.release()));

    for (size_t i = 0; i < sequence.size(); ++i) {
      if (i > 0)
        dice.roll(); // First value is set in constructor
      REQUIRE(dice.get_face_value() == static_cast<unsigned int>(sequence[i]));
    }
  }

  SECTION("Empty sequence fallback") {
    auto rng = std::make_unique<TestRandomGenerator>(); // No sequence, uses fallback
    DiceImpl dice(std::move(rng));

    // Should still produce valid values
    for (int i = 0; i < 10; ++i) {
      dice.roll();
      REQUIRE_THAT(dice.get_face_value(), IsValidDiceValue());
    }
  }
}