#define BOOST_TEST_MODULE GameTests
#include <boost/test/unit_test.hpp>
#include <liarsdice/ai/ai_player.hpp>
#include <liarsdice/core/game.hpp>
#include <liarsdice/core/player.hpp>

using namespace liarsdice::core;
using namespace liarsdice::ai;

BOOST_AUTO_TEST_SUITE(GameTestSuite)

BOOST_AUTO_TEST_CASE(GameConstruction) {
  GameConfig config;
  config.min_players = 2;
  config.max_players = 4;
  config.starting_dice = 5;

  Game game(config);

  BOOST_CHECK_EQUAL(game.get_state(), Game::State::NOT_STARTED);
  BOOST_CHECK_EQUAL(game.get_player_count(), 0);
  BOOST_CHECK_EQUAL(game.get_round_number(), 0);
  BOOST_CHECK(!game.is_game_active());
}

BOOST_AUTO_TEST_CASE(PlayerManagement) {
  Game game;

  auto player1 = std::make_shared<EasyAI>(1);
  auto player2 = std::make_shared<EasyAI>(2);

  // Add players
  game.add_player(player1);
  BOOST_CHECK_EQUAL(game.get_player_count(), 1);
  BOOST_CHECK_EQUAL(game.get_state(), Game::State::WAITING_FOR_PLAYERS);

  game.add_player(player2);
  BOOST_CHECK_EQUAL(game.get_player_count(), 2);

  // Verify players have starting dice
  BOOST_CHECK_EQUAL(player1->get_dice_count(), 5);
  BOOST_CHECK_EQUAL(player2->get_dice_count(), 5);

  // Test player retrieval
  auto found_player = game.get_player(1);
  BOOST_CHECK(found_player != nullptr);
  BOOST_CHECK_EQUAL(found_player->get_id(), 1);

  auto not_found = game.get_player(999);
  BOOST_CHECK(not_found == nullptr);
}

BOOST_AUTO_TEST_CASE(GameFlow) {
  Game game;
  bool round_started = false;
  bool player_turn_called = false;

  // Connect to events
  game.events().on_round_start.connect([&](unsigned int round) {
    round_started = true;
    BOOST_CHECK_EQUAL(round, 1);
  });

  game.events().on_player_turn.connect([&](const Player& player) { player_turn_called = true; });

  // Add players
  auto player1 = std::make_shared<EasyAI>(1);
  auto player2 = std::make_shared<EasyAI>(2);

  game.add_player(player1);
  game.add_player(player2);

  BOOST_CHECK_EQUAL(game.get_state(), Game::State::WAITING_FOR_PLAYERS);

  // Start game
  game.start_game();

  BOOST_CHECK_EQUAL(game.get_state(), Game::State::IN_PROGRESS);
  BOOST_CHECK_EQUAL(game.get_round_number(), 1);
  BOOST_CHECK(game.is_game_active());
  BOOST_CHECK(round_started);
  BOOST_CHECK(player_turn_called);

  // Verify current player
  auto current_player = game.get_current_player();
  BOOST_CHECK(current_player != nullptr);
  BOOST_CHECK(current_player->get_id() == 1 || current_player->get_id() == 2);
}

BOOST_AUTO_TEST_CASE(GuessValidation) {
  Game game;
  auto player1 = std::make_shared<EasyAI>(1);
  auto player2 = std::make_shared<EasyAI>(2);

  game.add_player(player1);
  game.add_player(player2);
  game.start_game();

  // First guess should be valid
  Guess first_guess{3, 4, 1};  // 3 fours from player 1
  auto current_player = game.get_current_player();
  first_guess.player_id = current_player->get_id();

  game.process_guess(first_guess);

  auto last_guess = game.get_last_guess();
  BOOST_CHECK(last_guess.has_value());
  BOOST_CHECK_EQUAL(last_guess->dice_count, 3);
  BOOST_CHECK_EQUAL(last_guess->face_value, 4);
}

BOOST_AUTO_TEST_CASE(CallLiarScenario) {
  Game game;
  bool liar_called = false;
  bool round_ended = false;

  // Connect to events
  game.events().on_liar_called.connect([&](const Player&) { liar_called = true; });

  game.events().on_round_end.connect([&](unsigned int) { round_ended = true; });

  auto player1 = std::make_shared<EasyAI>(1);
  auto player2 = std::make_shared<EasyAI>(2);

  game.add_player(player1);
  game.add_player(player2);
  game.start_game();

  // Make a guess
  auto current_player = game.get_current_player();
  Guess guess{10, 6, current_player->get_id()};  // Obviously false guess
  game.process_guess(guess);

  // Call liar
  game.process_call_liar();

  BOOST_CHECK(liar_called);
  BOOST_CHECK(round_ended);
}

BOOST_AUTO_TEST_CASE(PlayerElimination) {
  GameConfig config;

  Game game(config);
  bool player_eliminated = false;
  bool game_ended = false;

  game.events().on_player_eliminated.connect([&](const Player&) { player_eliminated = true; });

  game.events().on_game_winner.connect([&](const Player&) { game_ended = true; });

  auto player1 = std::make_shared<EasyAI>(1);
  auto player2 = std::make_shared<EasyAI>(2);

  game.add_player(player1);
  game.add_player(player2);
  game.start_game();

  // With point system, players start with 5 points
  // Make a very unlikely guess to guarantee liar will be called correctly
  // This ensures someone loses points and gets eliminated
  for (int round = 0; round < 6 && !player_eliminated && !game_ended; ++round) {
    auto current_player = game.get_current_player();

    if (round % 2 == 0) {
      // Make an obviously false guess (all dice showing 6s)
      Guess bad_guess{10, 6, current_player->get_id()};
      game.process_guess(bad_guess);
      // Next player will correctly call liar (guesser loses 1 point)
      game.process_call_liar();
    } else {
      // Make a reasonable guess
      Guess ok_guess{3, 4, current_player->get_id()};
      game.process_guess(ok_guess);
      // Force a false liar call (caller loses 2 points)
      game.process_call_liar();
    }
  }

  // Someone should be eliminated by now
  BOOST_CHECK(player_eliminated || game_ended);

  if (game_ended) {
    BOOST_CHECK_EQUAL(game.get_state(), Game::State::GAME_OVER);
  }
}

BOOST_AUTO_TEST_CASE(DiceCountTracking) {
  Game game;
  auto player1 = std::make_shared<EasyAI>(1);
  auto player2 = std::make_shared<EasyAI>(2);

  game.add_player(player1);
  game.add_player(player2);
  game.start_game();

  // Check total dice count
  auto total_dice = game.get_total_dice_count();
  BOOST_CHECK_EQUAL(total_dice, 10);  // 2 players * 5 dice each

  // Count specific face values (depends on random rolls)
  for (unsigned int face = 1; face <= 6; ++face) {
    auto count = game.count_total_dice_with_value(face);
    BOOST_CHECK(count <= total_dice);
  }
}

BOOST_AUTO_TEST_SUITE_END()