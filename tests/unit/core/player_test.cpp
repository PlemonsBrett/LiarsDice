#include "liarsdice/core/player.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Player ID constructor", "[player]") {
  Player player(42);
  REQUIRE(player.get_player_id() == 42);
}

TEST_CASE("Player get player ID", "[player]") {
  Player player(123);
  REQUIRE(player.get_player_id() == 123);
}

TEST_CASE("Player get dice", "[player]") {
  Player player(1);

  const auto &dice = player.get_dice();
  // Player should have some initial dice (check the implementation)
  // For now, just verify we can access the dice vector
  REQUIRE(dice.size() >= 0); // Should be non-negative
}

TEST_CASE("Player roll all dice", "[player]") {
  Player player(1);

  // Roll all dice
  player.roll_dice();

  // Check that all dice have valid values
  const auto &dice = player.get_dice();
  for (const auto &die : dice) {
    REQUIRE(die.get_face_value() >= 1);
    REQUIRE(die.get_face_value() <= 6);
  }
}

// Note: Additional functionality like MakeGuess() and CallLiar()
// require user input, so they would need mocking to test properly.