#include <gtest/gtest.h>
#include "liarsdice/core/game.hpp"

class GameTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup code if needed
  }

  void TearDown() override {
    // Cleanup code if needed
  }
};

TEST_F(GameTest, DefaultConstructor) {
  Game game;
  EXPECT_EQ(game.getPlayerCount(), 0);
}

TEST_F(GameTest, AddPlayer) {
  Game game;
  
  game.addPlayer("Alice");
  EXPECT_EQ(game.getPlayerCount(), 1);
  
  game.addPlayer("Bob");
  EXPECT_EQ(game.getPlayerCount(), 2);
}

TEST_F(GameTest, GetPlayers) {
  Game game;
  
  game.addPlayer("Alice");
  game.addPlayer("Bob");
  
  auto players = game.getPlayers();
  EXPECT_EQ(players.size(), 2);
  EXPECT_EQ(players[0].getName(), "Alice");
  EXPECT_EQ(players[1].getName(), "Bob");
}

// Note: More comprehensive game logic tests would require mocking
// user input or restructuring the Game class to be more testable.
// For now, we test the basic functionality that doesn't require I/O.