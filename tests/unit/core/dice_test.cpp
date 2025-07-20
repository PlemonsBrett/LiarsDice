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
  EXPECT_GE(dice.getValue(), 1);
  EXPECT_LE(dice.getValue(), 6);
}

TEST_F(DiceTest, ValueConstructor) {
  Dice dice(3);
  EXPECT_EQ(dice.getValue(), 3);
}

TEST_F(DiceTest, RollProducesValidValues) {
  Dice dice;
  std::set<int> possible_values{1, 2, 3, 4, 5, 6};
  
  // Roll the dice many times and ensure all values are valid
  for (int i = 0; i < 100; ++i) {
    dice.roll();
    int value = dice.getValue();
    EXPECT_TRUE(possible_values.count(value) > 0) 
        << "Invalid dice value: " << value;
  }
}

TEST_F(DiceTest, SetValue) {
  Dice dice;
  dice.setValue(4);
  EXPECT_EQ(dice.getValue(), 4);
}

TEST_F(DiceTest, RollChangesValue) {
  Dice dice(1);
  int original_value = dice.getValue();
  
  // Roll multiple times to increase chance of getting a different value
  bool value_changed = false;
  for (int i = 0; i < 50; ++i) {
    dice.roll();
    if (dice.getValue() != original_value) {
      value_changed = true;
      break;
    }
  }
  
  // With 50 rolls, there's a very high probability the value changed
  // If it didn't, either the RNG is broken or we're extremely unlucky
  EXPECT_TRUE(value_changed) 
      << "Dice value never changed after 50 rolls";
}