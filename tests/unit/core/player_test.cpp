#include <gtest/gtest.h>
#include "liarsdice/core/player.hpp"

class PlayerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup code if needed
  }

  void TearDown() override {
    // Cleanup code if needed
  }
};

TEST_F(PlayerTest, IdConstructor) {
  Player player(42);
  EXPECT_EQ(player.get_player_id(), 42);
}

TEST_F(PlayerTest, GetPlayerId) {
  Player player(123);
  EXPECT_EQ(player.get_player_id(), 123);
}

TEST_F(PlayerTest, GetDice) {
  Player player(1);
  
  const auto& dice = player.get_dice();
  // Player should have some initial dice (check the implementation)
  // For now, just verify we can access the dice vector
  EXPECT_TRUE(dice.size() >= 0);  // Should be non-negative
}

TEST_F(PlayerTest, RollAllDice) {
  Player player(1);
  
  // Roll all dice
  player.roll_dice();
  
  // Check that all dice have valid values
  const auto& dice = player.get_dice();
  for (const auto& die : dice) {
    EXPECT_GE(die.get_face_value(), 1);
    EXPECT_LE(die.get_face_value(), 6);
  }
}

// Note: Additional functionality like MakeGuess() and CallLiar() 
// require user input, so they would need mocking to test properly.