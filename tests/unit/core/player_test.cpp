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

TEST_F(PlayerTest, DefaultConstructor) {
  Player player;
  EXPECT_EQ(player.getName(), "");
  EXPECT_EQ(player.getDiceCount(), 0);
}

TEST_F(PlayerTest, NameConstructor) {
  Player player("Alice");
  EXPECT_EQ(player.getName(), "Alice");
  EXPECT_EQ(player.getDiceCount(), 0);
}

TEST_F(PlayerTest, SetAndGetName) {
  Player player;
  player.setName("Bob");
  EXPECT_EQ(player.getName(), "Bob");
}

TEST_F(PlayerTest, AddDice) {
  Player player("Alice");
  
  player.addDie(3);
  EXPECT_EQ(player.getDiceCount(), 1);
  
  player.addDie(5);
  EXPECT_EQ(player.getDiceCount(), 2);
}

TEST_F(PlayerTest, RemoveDie) {
  Player player("Alice");
  
  // Add some dice first
  player.addDie(1);
  player.addDie(2);
  player.addDie(3);
  EXPECT_EQ(player.getDiceCount(), 3);
  
  // Remove a die
  player.removeDie();
  EXPECT_EQ(player.getDiceCount(), 2);
  
  // Remove another
  player.removeDie();
  EXPECT_EQ(player.getDiceCount(), 1);
}

TEST_F(PlayerTest, GetDice) {
  Player player("Alice");
  
  player.addDie(2);
  player.addDie(4);
  
  auto dice = player.getDice();
  EXPECT_EQ(dice.size(), 2);
  EXPECT_EQ(dice[0].getValue(), 2);
  EXPECT_EQ(dice[1].getValue(), 4);
}

TEST_F(PlayerTest, RollAllDice) {
  Player player("Alice");
  
  // Add some dice
  player.addDie(1);
  player.addDie(1);
  player.addDie(1);
  
  // Roll all dice
  player.rollDice();
  
  // Check that all dice have valid values
  auto dice = player.getDice();
  for (const auto& die : dice) {
    EXPECT_GE(die.getValue(), 1);
    EXPECT_LE(die.getValue(), 6);
  }
}