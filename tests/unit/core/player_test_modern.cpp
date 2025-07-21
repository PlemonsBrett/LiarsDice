#include "../test_infrastructure.hpp"
#include "liarsdice/core/dice_impl.hpp"
#include "liarsdice/core/player_impl.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

using namespace liarsdice::testing;
using namespace liarsdice::core;
using namespace liarsdice::interfaces;

// Test fixture for Player tests
class PlayerTestFixture {
protected:
  std::unique_ptr<MockRandomGenerator> mock_rng;
  std::unique_ptr<PlayerImpl> player;

  void create_player_with_id(int id) {
    mock_rng = std::make_unique<MockRandomGenerator>();
    auto dice_factory = std::make_unique<ServiceFactory<IDice>>([this]() -> std::unique_ptr<IDice> {
      return std::make_unique<DiceImpl>(std::make_unique<MockRandomGenerator>());
    });

    player = std::make_unique<PlayerImpl>(id, std::unique_ptr<IRandomGenerator>(mock_rng.release()),
                                          std::move(dice_factory));
  }

  void add_dice_to_player(size_t count) {
    for (size_t i = 0; i < count; ++i) {
      player->add_die();
    }
  }
};

// BDD-style tests
SCENARIO("Player manages dice collection", "[player][bdd]") {
  GIVEN("A new player with ID 1") {
    auto rng = std::make_unique<MockRandomGenerator>();
    auto dice_factory = std::make_unique<ServiceFactory<IDice>>([]() -> std::unique_ptr<IDice> {
      return std::make_unique<DiceImpl>(std::make_unique<MockRandomGenerator>());
    });

    PlayerImpl player(1, std::move(rng), std::move(dice_factory));

    WHEN("checking initial state") {
      THEN("player has correct ID and no dice") {
        REQUIRE(player.get_id() == 1);
        REQUIRE(player.get_dice_count() == 0);
        REQUIRE_FALSE(player.has_dice());
        REQUIRE_FALSE(player.is_active());
      }
    }

    WHEN("adding dice") {
      player.add_die();
      player.add_die();
      player.add_die();

      THEN("player has correct dice count") {
        REQUIRE(player.get_dice_count() == 3);
        REQUIRE(player.has_dice());
        REQUIRE(player.is_active());
      }

      AND_WHEN("rolling all dice") {
        REQUIRE_NOTHROW(player.roll_dice());

        THEN("all dice have valid values") {
          auto dice = player.get_dice();
          REQUIRE(dice.size() == 3);

          for (const auto &die : dice) {
            auto value = die.get().get_face_value();
            REQUIRE_THAT(value, IsInRange(std::array{1u, 2u, 3u, 4u, 5u, 6u}));
          }
        }
      }

      AND_WHEN("removing dice") {
        REQUIRE(player.remove_die());

        THEN("dice count decreases") {
          REQUIRE(player.get_dice_count() == 2);
          REQUIRE(player.has_dice());
        }

        AND_WHEN("removing all remaining dice") {
          REQUIRE(player.remove_die());
          REQUIRE(player.remove_die());
          REQUIRE_FALSE(player.remove_die()); // No more dice

          THEN("player becomes inactive") {
            REQUIRE(player.get_dice_count() == 0);
            REQUIRE_FALSE(player.has_dice());
            REQUIRE_FALSE(player.is_active());
          }
        }
      }
    }
  }
}

// TEST_CASE_METHOD tests
TEST_CASE_METHOD(PlayerTestFixture, "Player dice counting", "[player][fixture]") {
  create_player_with_id(42);
  add_dice_to_player(5);

  SECTION("Count dice with specific value") {
    // Set up dice with known values using mock
    auto dice_values = player->get_dice_values();

    // Count occurrences
    for (unsigned int value = 1; value <= 6; ++value) {
      auto count = player->count_dice_with_value(value);
      auto expected = std::count(dice_values.begin(), dice_values.end(), value);
      REQUIRE(count == static_cast<size_t>(expected));
    }
  }

  SECTION("Get dice values returns correct array") {
    auto values = player->get_dice_values();
    REQUIRE(values.size() == 5);

    for (auto value : values) {
      REQUIRE_THAT(value, IsInRange(std::array{1u, 2u, 3u, 4u, 5u, 6u}));
    }
  }
}

// Property-based tests
TEST_CASE("Player properties", "[player][property]") {
  SECTION("Player ID validation") {
    auto player_id = GENERATE(range(-100, 101));

    auto rng = std::make_unique<MockRandomGenerator>();
    auto dice_factory = std::make_unique<ServiceFactory<IDice>>([]() -> std::unique_ptr<IDice> {
      return std::make_unique<DiceImpl>(std::make_unique<MockRandomGenerator>());
    });

    if (player_id > 0) {
      REQUIRE_NOTHROW(PlayerImpl(player_id, std::move(rng), std::move(dice_factory)));
    } else {
      REQUIRE_THROWS_AS(PlayerImpl(player_id, std::move(rng), std::move(dice_factory)),
                        std::invalid_argument);
    }
  }

  SECTION("Dice count consistency") {
    // Generate a sequence of operations
    std::vector<int> operations;
    for (int i = 0; i < 20; ++i) {
      operations.push_back(GENERATE(0, 1, 2));
    }

    PlayerTestFixture fixture;
    fixture.create_player_with_id(1);

    size_t expected_count = 0;

    for (int op : operations) {
      if (op == 0 && expected_count > 0) {
        // Remove die
        fixture.player->remove_die();
        expected_count--;
      } else if (op >= 1) {
        // Add die
        fixture.player->add_die();
        expected_count++;
      }

      REQUIRE(fixture.player->get_dice_count() == expected_count);
      REQUIRE(fixture.player->has_dice() == (expected_count > 0));
    }
  }
}

// Performance benchmarks
TEST_CASE("Player performance benchmarks", "[player][benchmark]") {
  PlayerTestFixture fixture;
  fixture.create_player_with_id(1);
  fixture.add_dice_to_player(5);

  BENCHMARK("Add die performance") {
    fixture.player->add_die();
    fixture.player->remove_die(); // Keep count stable
  };

  BENCHMARK("Roll all dice performance") { return fixture.player->roll_dice(); };

  BENCHMARK("Count dice with value performance") {
    return fixture.player->count_dice_with_value(3);
  };

  BENCHMARK("Get dice values performance") { return fixture.player->get_dice_values(); };
}

// Integration test with multiple players
TEST_CASE("Multiple players interaction", "[player][integration]") {
  std::vector<std::unique_ptr<PlayerImpl>> players;

  // Create 4 players
  for (int id = 1; id <= 4; ++id) {
    auto rng = std::make_unique<MockRandomGenerator>();
    auto dice_factory = std::make_unique<ServiceFactory<IDice>>([]() -> std::unique_ptr<IDice> {
      return std::make_unique<DiceImpl>(std::make_unique<MockRandomGenerator>());
    });

    auto player = std::make_unique<PlayerImpl>(id, std::move(rng), std::move(dice_factory));

    // Each player starts with 5 dice
    for (int i = 0; i < 5; ++i) {
      player->add_die();
    }

    players.push_back(std::move(player));
  }

  SECTION("All players start active") {
    for (const auto &player : players) {
      REQUIRE(player->is_active());
      REQUIRE(player->get_dice_count() == 5);
    }
  }

  SECTION("Simulate elimination") {
    // Remove all dice from player 2
    for (int i = 0; i < 5; ++i) {
      players[1]->remove_die();
    }

    REQUIRE_FALSE(players[1]->is_active());

    // Count active players
    auto active_count =
        std::count_if(players.begin(), players.end(), [](const auto &p) { return p->is_active(); });

    REQUIRE(active_count == 3);
  }
}

// Custom matcher test
TEST_CASE("Player with custom matchers", "[player][matchers]") {
  PlayerTestFixture fixture;
  fixture.create_player_with_id(7);

  SECTION("Player ID in expected range") {
    REQUIRE_THAT(fixture.player->get_id(),
                 Catch::Matchers::Predicate<int>([](int id) { return id > 0 && id < 100; },
                                                 "is valid player ID"));
  }

  SECTION("Dice collection behavior") {
    fixture.add_dice_to_player(3);

    auto dice_refs = fixture.player->get_dice();
    REQUIRE_THAT(dice_refs, Catch::Matchers::SizeIs(3));

    // All dice should be valid
    for (const auto &die_ref : dice_refs) {
      REQUIRE_THAT(die_ref.get().get_face_value(), IsInRange(std::array{1u, 2u, 3u, 4u, 5u, 6u}));
    }
  }
}