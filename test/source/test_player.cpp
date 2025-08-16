#define BOOST_TEST_MODULE PlayerTests
#include <boost/test/unit_test.hpp>
#include <liarsdice/core/player.hpp>

using namespace liarsdice::core;

BOOST_AUTO_TEST_SUITE(PlayerTestSuite)

BOOST_AUTO_TEST_CASE(PlayerConstruction) {
  HumanPlayer player(1, "Test Player");
  BOOST_CHECK_EQUAL(player.get_id(), 1);
  BOOST_CHECK_EQUAL(player.get_name(), "Test Player");
  BOOST_CHECK_EQUAL(player.get_dice_count(), 5);
  BOOST_CHECK(player.has_dice());
}

BOOST_AUTO_TEST_CASE(PlayerDiceManagement) {
  HumanPlayer player(1);

  // Initial state
  BOOST_CHECK_EQUAL(player.get_dice_count(), 5);

  // Add a die
  player.add_die();
  BOOST_CHECK_EQUAL(player.get_dice_count(), 6);

  // Remove dice
  BOOST_CHECK(player.remove_die());
  BOOST_CHECK_EQUAL(player.get_dice_count(), 5);

  // Remove all dice
  for (size_t i = 0; i < 5; ++i) {
    BOOST_CHECK(player.remove_die());
  }
  BOOST_CHECK_EQUAL(player.get_dice_count(), 0);
  BOOST_CHECK(!player.has_dice());
  BOOST_CHECK(!player.remove_die());  // Can't remove from empty
}

BOOST_AUTO_TEST_SUITE_END()