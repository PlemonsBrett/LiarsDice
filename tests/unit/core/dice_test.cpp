#include "liarsdice/core/dice.hpp"
#include <catch2/catch_test_macros.hpp>
#include <set>

TEST_CASE("Dice default constructor", "[dice]") {
  Dice dice;
  REQUIRE(dice.get_face_value() >= 1);
  REQUIRE(dice.get_face_value() <= 6);
}

TEST_CASE("Dice roll produces valid values", "[dice]") {
  Dice dice;
  std::set<unsigned int> possible_values{1, 2, 3, 4, 5, 6};

  // Roll the dice many times and ensure all values are valid
  for (int i = 0; i < 100; ++i) {
    dice.roll();
    unsigned int value = dice.get_face_value();
    REQUIRE(possible_values.contains(value));
  }
}

TEST_CASE("Dice roll changes value", "[dice]") {
  Dice dice;
  unsigned int original_value = dice.get_face_value();

  // Roll multiple times to increase chance of getting a different value
  bool value_changed = false;
  for (int i = 0; i < 50; ++i) {
    dice.roll();
    if (dice.get_face_value() != original_value) {
      value_changed = true;
      break;
    }
  }

  // With 50 rolls, there's a very high probability the value changed
  // If it didn't, either the RNG is broken or we're extremely unlucky
  REQUIRE(value_changed);
}