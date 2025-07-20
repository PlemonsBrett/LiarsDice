#include <gtest/gtest.h>
#include "liarsdice/core/dice.hpp"
#include <set>

class DiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup code if needed
  }

  void TearDown() override {
    // Cleanup code if needed
  }
};

TEST_F(DiceTest, DefaultConstructor) {
  Dice dice;
  EXPECT_GE(dice.get_face_value(), 1);
  EXPECT_LE(dice.get_face_value(), 6);
}

TEST_F(DiceTest, RollProducesValidValues) {
  Dice dice;
  std::set<unsigned int> possible_values{1, 2, 3, 4, 5, 6};
  
  // Roll the dice many times and ensure all values are valid
  for (int i = 0; i < 100; ++i) {
    dice.roll();
    unsigned int value = dice.get_face_value();
    EXPECT_TRUE(possible_values.contains(value)) 
        << "Invalid dice value: " << value;
  }
}

TEST_F(DiceTest, RollChangesValue) {
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
  EXPECT_TRUE(value_changed) 
      << "Dice value never changed after 50 rolls";
}