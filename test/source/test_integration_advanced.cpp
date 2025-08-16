#define BOOST_TEST_MODULE IntegrationAdvancedTests
#include <algorithm>
#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <liarsdice/ai/easy_ai.hpp>
#include <liarsdice/ai/hard_ai.hpp>
#include <liarsdice/ai/medium_ai.hpp>
#include <liarsdice/config/config_manager.hpp>
#include <liarsdice/core/game.hpp>
#include <liarsdice/validation/input_validator.hpp>
#include <memory>
#include <random>

using namespace liarsdice::core;
using namespace liarsdice::ai;
using namespace liarsdice::config;
using namespace liarsdice::validation;
namespace bdata = boost::unit_test::data;

BOOST_AUTO_TEST_SUITE(IntegrationAdvancedTestSuite)

// BDD-style scenario tests for complete game workflows
BOOST_AUTO_TEST_CASE(CompleteGameFlowWithMixedPlayers) {
  // GIVEN: A game with mixed human and AI players
  Game game;

  // Add different types of players
  game.add_player(std::make_shared<EasyAI>(1));
  game.add_player(std::make_shared<MediumAI>(2));
  game.add_player(std::make_shared<HardAI>(3));

  // WHEN: starting the game
  BOOST_CHECK_NO_THROW(game.start());

  // THEN: game should be properly initialized
  auto players = game.get_players();
  BOOST_CHECK_EQUAL(players.size(), 3);

  for (auto& player : players) {
    BOOST_CHECK_EQUAL(player->get_dice_count(), 5);  // All start with 5 dice
    BOOST_CHECK(player->has_dice());
  }

  // WHEN: starting multiple rounds
  for (int round = 1; round <= 3; ++round) {
    BOOST_CHECK_NO_THROW(game.start_round());

    // THEN: each round should have valid state
    auto active_players = game.get_active_players();
    BOOST_CHECK_GE(active_players.size(), 1);  // At least one active player

    // Check that dice values are valid after rolling
    for (auto& player : active_players) {
      auto dice_values = player->get_dice_values();
      for (auto value : dice_values) {
        BOOST_CHECK(value >= 1 && value <= 6);
      }
    }
  }
}

// Property-based testing with different game configurations
BOOST_DATA_TEST_CASE(GameScalabilityWithVariousPlayerCounts, bdata::make({2, 3, 4, 5, 6, 7, 8}),
                     num_players) {
  Game game;

  // Add specified number of AI players
  for (int i = 1; i <= num_players; ++i) {
    auto ai_type = i % 3;
    switch (ai_type) {
      case 0:
        game.add_player(std::make_shared<EasyAI>(i));
        break;
      case 1:
        game.add_player(std::make_shared<MediumAI>(i));
        break;
      case 2:
        game.add_player(std::make_shared<HardAI>(i));
        break;
    }
  }

  // Game should handle any valid number of players
  BOOST_CHECK_NO_THROW(game.start());

  auto players = game.get_players();
  BOOST_CHECK_EQUAL(players.size(), num_players);

  // Test that game can run at least one round
  BOOST_CHECK_NO_THROW(game.start_round());

  // All players should be active initially
  auto active_players = game.get_active_players();
  BOOST_CHECK_EQUAL(active_players.size(), num_players);
}

// Full game simulation with elimination
BOOST_AUTO_TEST_CASE(CompleteGameSimulationWithElimination) {
  Game game;

  // Add 4 AI players for a medium-sized game
  game.add_player(std::make_shared<EasyAI>(1));
  game.add_player(std::make_shared<MediumAI>(2));
  game.add_player(std::make_shared<HardAI>(3));
  game.add_player(std::make_shared<EasyAI>(4));

  game.start();

  // Simulate multiple rounds with dice loss
  int round_count = 0;
  const int max_rounds = 20;  // Prevent infinite loops

  while (!game.is_game_over() && round_count < max_rounds) {
    round_count++;

    BOOST_CHECK_NO_THROW(game.start_round());

    auto active_players = game.get_active_players();
    BOOST_CHECK_GE(active_players.size(), 1);

    // Simulate a player losing a die (simplified game logic)
    if (active_players.size() > 1) {
      auto& player_to_lose = active_players[round_count % active_players.size()];
      if (player_to_lose->has_dice()) {
        player_to_lose->remove_die();
      }
    }

    // Check for eliminated players
    auto all_players = game.get_players();
    size_t eliminated_count = 0;
    for (auto& player : all_players) {
      if (!player->has_dice()) {
        eliminated_count++;
      }
    }

    // Game should end when only one player remains active
    if (eliminated_count >= all_players.size() - 1) {
      break;
    }
  }

  BOOST_CHECK_LE(round_count, max_rounds);
  BOOST_TEST_MESSAGE("Game completed in " << round_count << " rounds");
}

// Configuration integration testing
BOOST_AUTO_TEST_CASE(GameIntegrationWithConfiguration) {
  // Create temporary configuration
  ConfigManager config_manager;
  config_manager.load_defaults();

  auto game_config = config_manager.get_game_config();
  auto ui_config = config_manager.get_ui_config();

  // Verify configuration is loaded properly
  BOOST_CHECK_GE(game_config.min_players, 2);
  BOOST_CHECK_LE(game_config.max_players, 8);
  BOOST_CHECK(!ui_config.welcome_message.empty());

  // Create game using configuration
  Game game;

  // Add players within configured limits
  int player_count = std::min(4, game_config.max_players);
  for (int i = 1; i <= player_count; ++i) {
    game.add_player(std::make_shared<EasyAI>(i));
  }

  // Game should work with configured parameters
  BOOST_CHECK_NO_THROW(game.start());

  auto players = game.get_players();
  BOOST_CHECK_EQUAL(players.size(), player_count);
  BOOST_CHECK_GE(player_count, game_config.min_players);
  BOOST_CHECK_LE(player_count, game_config.max_players);
}

// Validation integration with game flow
BOOST_AUTO_TEST_CASE(ValidationIntegrationWithGameInput) {
  InputValidator validator;
  Game game;

  // Test validation of game setup inputs
  std::vector<std::string> player_inputs = {"2", "3", "4", "5", "6", "7", "8"};
  std::vector<std::string> invalid_inputs = {"0", "1", "9", "10", "abc", ""};

  for (const auto& input : player_inputs) {
    BOOST_CHECK(validator.validate_player_count(input));

    // Each valid player count should work with the game
    int count = std::stoi(input);
    Game test_game;

    for (int i = 1; i <= count; ++i) {
      test_game.add_player(std::make_shared<EasyAI>(i));
    }

    BOOST_CHECK_NO_THROW(test_game.start());
  }

  for (const auto& input : invalid_inputs) {
    BOOST_CHECK(!validator.validate_player_count(input));
  }

  // Test dice and face value validation
  std::vector<std::string> dice_inputs = {"1", "2", "5", "10", "15", "20"};
  std::vector<std::string> face_inputs = {"1", "2", "3", "4", "5", "6"};

  for (const auto& dice_input : dice_inputs) {
    BOOST_CHECK(validator.validate_dice_count(dice_input));
  }

  for (const auto& face_input : face_inputs) {
    BOOST_CHECK(validator.validate_face_value(face_input));
  }
}

// Performance testing with realistic game scenarios
BOOST_AUTO_TEST_CASE(GamePerformanceUnderLoad) {
  const int num_games = 100;
  const int players_per_game = 4;

  auto start = std::chrono::high_resolution_clock::now();

  for (int game_num = 0; game_num < num_games; ++game_num) {
    Game game;

    // Add players
    for (int i = 1; i <= players_per_game; ++i) {
      auto ai_type = i % 3;
      switch (ai_type) {
        case 0:
          game.add_player(std::make_shared<EasyAI>(i));
          break;
        case 1:
          game.add_player(std::make_shared<MediumAI>(i));
          break;
        case 2:
          game.add_player(std::make_shared<HardAI>(i));
          break;
      }
    }

    // Start game and run a few rounds
    game.start();

    for (int round = 0; round < 3; ++round) {
      game.start_round();

      // Simulate some game activity
      auto players = game.get_players();
      for (auto& player : players) {
        if (player->has_dice()) {
          auto dice_values = player->get_dice_values();
          (void)dice_values;  // Suppress unused warning
        }
      }
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  BOOST_CHECK(duration.count() < 30000);  // Less than 30 seconds

  BOOST_TEST_MESSAGE("Performance test: " << duration.count() << " ms for " << num_games
                                          << " games");
}

// Error handling and recovery testing
BOOST_AUTO_TEST_CASE(GameErrorHandlingAndRecovery) {
  Game game;

  // Test error handling with no players
  BOOST_CHECK_THROW(game.start(), std::runtime_error);

  // Test with insufficient players
  game.add_player(std::make_shared<EasyAI>(1));
  BOOST_CHECK_THROW(game.start(), std::runtime_error);

  // Add sufficient players
  game.add_player(std::make_shared<MediumAI>(2));
  BOOST_CHECK_NO_THROW(game.start());

  // Test round operations
  BOOST_CHECK_NO_THROW(game.start_round());

  // Test that game handles player elimination gracefully
  auto players = game.get_players();
  for (auto& player : players) {
    while (player->has_dice()) {
      player->remove_die();
    }
  }

  // Game should detect that all players are eliminated
  BOOST_CHECK(game.is_game_over());
}

// Multi-component integration stress test
BOOST_AUTO_TEST_CASE(MultiComponentIntegrationStressTest) {
  // Test integration of all major components under stress
  ConfigManager config_manager;
  InputValidator validator;

  config_manager.load_defaults();

  const int stress_iterations = 50;

  for (int i = 0; i < stress_iterations; ++i) {
    // Create new game
    Game game;

    // Add random number of players
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> player_dist(2, 6);
    int num_players = player_dist(gen);

    for (int j = 1; j <= num_players; ++j) {
      auto ai_type = j % 3;
      switch (ai_type) {
        case 0:
          game.add_player(std::make_shared<EasyAI>(j));
          break;
        case 1:
          game.add_player(std::make_shared<MediumAI>(j));
          break;
        case 2:
          game.add_player(std::make_shared<HardAI>(j));
          break;
      }
    }

    // Start game
    BOOST_CHECK_NO_THROW(game.start());

    // Run multiple rounds
    std::uniform_int_distribution<> round_dist(1, 5);
    int num_rounds = round_dist(gen);

    for (int round = 0; round < num_rounds; ++round) {
      BOOST_CHECK_NO_THROW(game.start_round());

      // Test validation during game
      BOOST_CHECK(validator.validate_player_count(std::to_string(num_players)));
      BOOST_CHECK(validator.validate_dice_count("5"));
      BOOST_CHECK(validator.validate_face_value("3"));

      // Test configuration access
      auto game_config = config_manager.get_game_config();
      BOOST_CHECK_GE(game_config.min_players, 2);
    }
  }

  BOOST_TEST_MESSAGE("Completed " << stress_iterations << " stress test iterations");
}

// Real-world scenario simulation
BOOST_AUTO_TEST_CASE(RealWorldGameplayScenario) {
  // Simulate a realistic game session
  Game game;

  // Add 4 players (common game size)
  game.add_player(std::make_shared<EasyAI>(1));
  game.add_player(std::make_shared<MediumAI>(2));
  game.add_player(std::make_shared<HardAI>(3));
  game.add_player(std::make_shared<EasyAI>(4));

  game.start();

  // Track game statistics
  int total_rounds = 0;
  int dice_lost = 0;

  while (!game.is_game_over() && total_rounds < 50) {
    total_rounds++;

    game.start_round();

    auto active_players = game.get_active_players();
    BOOST_CHECK_GE(active_players.size(), 1);

    // Count total dice in play
    size_t total_dice = 0;
    for (auto& player : active_players) {
      total_dice += player->get_dice_count();
    }

    // Simulate typical game progression (dice loss)
    if (total_rounds % 3 == 0 && active_players.size() > 1) {
      // Someone loses a die
      auto& unlucky_player = active_players[total_rounds % active_players.size()];
      if (unlucky_player->has_dice()) {
        unlucky_player->remove_die();
        dice_lost++;
      }
    }

    // Check for game end condition
    size_t active_count = 0;
    for (auto& player : game.get_players()) {
      if (player->has_dice()) {
        active_count++;
      }
    }

    if (active_count <= 1) {
      break;
    }
  }

  BOOST_CHECK_LE(total_rounds, 50);
  BOOST_CHECK_GT(dice_lost, 0);

  BOOST_TEST_MESSAGE("Realistic game: " << total_rounds << " rounds, " << dice_lost
                                        << " dice lost");
}

// Integration testing with different AI combinations
BOOST_AUTO_TEST_CASE(AICombinationGameplayTesting) {
  // Test various AI combinations
  std::vector<std::vector<std::function<std::shared_ptr<Player>(int)>>> ai_combinations
      = {// All easy
         {[](int id) { return std::make_shared<EasyAI>(id); },
          [](int id) { return std::make_shared<EasyAI>(id); },
          [](int id) { return std::make_shared<EasyAI>(id); }},

         // All medium
         {[](int id) { return std::make_shared<MediumAI>(id); },
          [](int id) { return std::make_shared<MediumAI>(id); },
          [](int id) { return std::make_shared<MediumAI>(id); }},

         // All hard
         {[](int id) { return std::make_shared<HardAI>(id); },
          [](int id) { return std::make_shared<HardAI>(id); },
          [](int id) { return std::make_shared<HardAI>(id); }},

         // Mixed
         {[](int id) { return std::make_shared<EasyAI>(id); },
          [](int id) { return std::make_shared<MediumAI>(id); },
          [](int id) { return std::make_shared<HardAI>(id); }}};

  for (size_t combo = 0; combo < ai_combinations.size(); ++combo) {
    Game game;

    // Add players according to combination
    for (size_t i = 0; i < ai_combinations[combo].size(); ++i) {
      game.add_player(ai_combinations[combo][i](i + 1));
    }

    BOOST_CHECK_NO_THROW(game.start());

    // Run a few rounds to test AI interactions
    for (int round = 0; round < 3; ++round) {
      BOOST_CHECK_NO_THROW(game.start_round());

      auto active_players = game.get_active_players();
      BOOST_CHECK_EQUAL(active_players.size(), ai_combinations[combo].size());

      // Test that AIs can make decisions
      for (auto& player : active_players) {
        if (auto ai = std::dynamic_pointer_cast<EasyAI>(player)) {
          auto guess = ai->make_guess(std::nullopt);
          BOOST_CHECK(guess.dice_count > 0);
          BOOST_CHECK(guess.face_value >= 1 && guess.face_value <= 6);
        }
      }
    }

    BOOST_TEST_MESSAGE("AI combination " << combo << " tested successfully");
  }
}

BOOST_AUTO_TEST_SUITE_END()