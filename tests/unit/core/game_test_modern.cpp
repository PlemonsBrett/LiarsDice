#include "../test_infrastructure.hpp"
#include "liarsdice/core/dice_impl.hpp"
#include "liarsdice/core/game_impl.hpp"
#include "liarsdice/core/player_impl.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

using namespace liarsdice::testing;
using namespace liarsdice::core;
using namespace liarsdice::interfaces;
using namespace liarsdice::di;

// Game state implementation for testing
class TestGameState : public IGameState {
private:
  std::vector<std::unique_ptr<IPlayer>> players_;
  size_t current_player_index_ = 0;
  bool game_active_ = false;
  int round_number_ = 1;
  std::optional<Guess> last_guess_;

public:
  [[nodiscard]] size_t get_current_player_index() const override { return current_player_index_; }

  void advance_to_next_player() override {
    current_player_index_ = (current_player_index_ + 1) % players_.size();
  }

  [[nodiscard]] size_t get_player_count() const override { return players_.size(); }

  [[nodiscard]] bool is_game_active() const override { return game_active_; }

  void set_game_active(bool active) override { game_active_ = active; }

  [[nodiscard]] int get_round_number() const override { return round_number_; }

  void increment_round() override { ++round_number_; }

  [[nodiscard]] std::optional<Guess> get_last_guess() const override { return last_guess_; }

  void set_last_guess(const Guess &guess) override { last_guess_ = guess; }

  void clear_last_guess() override { last_guess_.reset(); }

  IPlayer &get_player(size_t index) override { return *players_.at(index); }

  [[nodiscard]] const IPlayer &get_player(size_t index) const override {
    return *players_.at(index);
  }

  IPlayer &get_current_player() override { return *players_[current_player_index_]; }

  [[nodiscard]] const IPlayer &get_current_player() const override {
    return *players_[current_player_index_];
  }

  [[nodiscard]] size_t count_total_dice_with_value(unsigned int face_value) const override {
    size_t count = 0;
    for (const auto &player : players_) {
      count += player->count_dice_with_value(face_value);
    }
    return count;
  }

  [[nodiscard]] size_t get_total_dice_count() const override {
    size_t count = 0;
    for (const auto &player : players_) {
      count += player->get_dice_count();
    }
    return count;
  }

  void add_player(std::unique_ptr<IPlayer> player) { players_.push_back(std::move(player)); }
};

// Enhanced game test fixture
class GameIntegrationFixture : public GameTestFixture {
protected:
  TestGameState *game_state_ = nullptr;
  MockRandomGenerator *main_rng_ = nullptr;

  void setup_dependencies() override {
    GameTestFixture::setup_dependencies();

    // Register game state
    container_->register_factory<IGameState>(
        []() -> std::unique_ptr<IGameState> { return std::make_unique<TestGameState>(); },
        ServiceLifetime::kTransient);

    // Register player factory
    container_->register_factory<ServiceFactory<IPlayer>>(
        []() -> std::unique_ptr<ServiceFactory<IPlayer>> {
          return std::make_unique<ServiceFactory<IPlayer>>([]() -> std::unique_ptr<IPlayer> {
            auto rng = std::make_unique<MockRandomGenerator>();
            auto dice_factory =
                std::make_unique<ServiceFactory<IDice>>([]() -> std::unique_ptr<IDice> {
                  return std::make_unique<DiceImpl>(std::make_unique<MockRandomGenerator>());
                });
            static int player_id = 1;
            return std::make_unique<PlayerImpl>(player_id++, std::move(rng),
                                                std::move(dice_factory));
          });
        },
        ServiceLifetime::kSingleton);

    // Register game implementation
    container_->register_factory<IGame>(
        [this]() -> std::unique_ptr<IGame> {
          auto game_state = std::make_unique<TestGameState>();
          game_state_ = game_state.get(); // Keep reference for testing

          auto rng = std::make_unique<MockRandomGenerator>();
          main_rng_ = rng.get(); // Keep reference

          auto player_factory_result = container_->resolve<ServiceFactory<IPlayer>>();
          if (!player_factory_result.has_value()) {
            throw std::runtime_error("Failed to resolve player factory");
          }

          return std::make_unique<GameImpl>(std::move(game_state), std::move(rng),
                                            std::move(player_factory_result.value()));
        },
        ServiceLifetime::kTransient);
  }
};

// BDD-style game tests
SCENARIO("Liar's Dice game flow", "[game][bdd][integration]") {
  GIVEN("A new game with 3 players") {
    GameIntegrationFixture fixture;
    fixture.create_game_with_players(3);

    WHEN("the game starts") {
      THEN("game is active with correct initial state") {
        REQUIRE(fixture.game_->is_game_over() == false);
        REQUIRE(fixture.game_->get_winner_id() == -1);

        auto &state = fixture.game_->get_game_state();
        REQUIRE(state.is_game_active());
        REQUIRE(state.get_player_count() == 3);
        REQUIRE(state.get_round_number() == 1);
      }

      AND_WHEN("a player makes a valid guess") {
        Guess first_guess{2, 3, 1}; // 2 dice showing 3

        THEN("the guess is accepted") {
          REQUIRE(fixture.game_->validate_guess(first_guess).empty());
          REQUIRE(fixture.game_->process_guess(first_guess));

          auto last_guess = fixture.game_->get_game_state().get_last_guess();
          REQUIRE(last_guess.has_value());
          REQUIRE(last_guess->dice_count == 2);
          REQUIRE(last_guess->face_value == 3);
        }

        AND_WHEN("the next player makes an invalid guess") {
          Guess invalid_guess{1, 2, 2}; // Lower than previous

          THEN("the guess is rejected") {
            auto error = fixture.game_->validate_guess(invalid_guess);
            REQUIRE_FALSE(error.empty());
            REQUIRE_FALSE(fixture.game_->process_guess(invalid_guess));
          }
        }

        AND_WHEN("the next player makes a valid higher guess") {
          Guess valid_guess{3, 3, 2}; // 3 dice showing 3

          THEN("the guess is accepted") {
            REQUIRE(fixture.game_->validate_guess(valid_guess).empty());
            REQUIRE(fixture.game_->process_guess(valid_guess));
          }
        }
      }
    }

    WHEN("a player calls liar") {
      // Set up a scenario with a specific guess
      Guess current_guess{4, 5, 1}; // 4 dice showing 5
      fixture.game_->process_guess(current_guess);

      auto result = fixture.game_->process_liar_call(2);

      THEN("the call is processed and someone loses a die") {
        REQUIRE_FALSE(result.empty());
        REQUIRE((result.find("loses a die") != std::string::npos));
      }
    }
  }
}

// Property-based game tests
TEST_CASE("Game rule validation properties", "[game][property]") {
  GameIntegrationFixture fixture;
  fixture.create_game_with_players(2);

  SECTION("Guess validation follows rules") {
    auto dice_count = GENERATE(range(1U, 11U));
    auto face_value = GENERATE(range(1U, 7U));

    Guess guess{dice_count, face_value, 1};
    auto error = fixture.game_->validate_guess(guess);

    // First guess should always be valid if within bounds
    bool should_be_valid = (dice_count <= 10 && face_value >= 1 && face_value <= 6);
    REQUIRE(error.empty() == should_be_valid);
  }

  SECTION("Sequential guess validation") {
    // First guess
    Guess first{3, 4, 1};
    fixture.game_->process_guess(first);

    // Generate second guess
    auto dice_count = GENERATE(range(1U, 7U));
    auto face_value = GENERATE(range(1U, 7U));

    Guess second{dice_count, face_value, 2};
    auto error = fixture.game_->validate_guess(second);

    // Check if it should be valid according to rules
    bool should_be_valid = (dice_count > 3) || (dice_count == 3 && face_value > 4);

    REQUIRE(error.empty() == should_be_valid);
  }
}

// Performance benchmarks
TEST_CASE("Game performance benchmarks", "[game][benchmark]") {
  GameIntegrationFixture fixture;

  BENCHMARK("Game initialization with 4 players") {
    fixture.container_ = std::make_unique<ServiceContainer>();
    fixture.setup_dependencies();
    fixture.create_game_with_players(4);
  };

  fixture.create_game_with_players(4);

  BENCHMARK("Guess validation") {
    Guess guess{3, 4, 1};
    return fixture.game_->validate_guess(guess);
  };

  BENCHMARK("Process guess") {
    Guess guess{3, 4, 1};
    fixture.game_->process_guess(guess);
    fixture.game_->get_game_state().clear_last_guess(); // Reset
  };

  BENCHMARK("Process liar call") {
    Guess guess{3, 4, 1};
    fixture.game_->process_guess(guess);
    auto result = fixture.game_->process_liar_call(2);
    fixture.game_->get_game_state().clear_last_guess(); // Reset
    return result;
  };
}

// Edge case testing
TEST_CASE("Game edge cases", "[game][edge-cases]") {
  GameIntegrationFixture fixture;

  SECTION("Minimum players") {
    fixture.create_game_with_players(fixture.game_->get_min_players());
    REQUIRE_FALSE(fixture.game_->is_game_over());
  }

  SECTION("Maximum players") {
    fixture.create_game_with_players(fixture.game_->get_max_players());
    REQUIRE_FALSE(fixture.game_->is_game_over());
  }

  SECTION("Too few players") {
    fixture.game_ = fixture.container_->resolve<IGame>().value();
    fixture.game_->initialize();
    fixture.game_->add_player(1);

    REQUIRE_THROWS(fixture.game_->start_game());
  }

  SECTION("Too many players") {
    fixture.create_game_with_players(fixture.game_->get_max_players());
    REQUIRE_THROWS(fixture.game_->add_player(99));
  }
}

// Death tests for exception safety
TEST_CASE("Game exception safety", "[game][death-test]") {
  GameIntegrationFixture fixture;

  SECTION("Null dependency handling") {
    REQUIRE_THROWS_AS(GameImpl(nullptr, std::make_unique<MockRandomGenerator>(), nullptr),
                      std::invalid_argument);
  }

  SECTION("Invalid player operations") {
    fixture.create_game_with_players(2);

    // Try to process liar call with invalid player ID
    REQUIRE_NOTHROW(fixture.game_->process_liar_call(999));
  }
}

// Statistical game simulation
TEST_CASE("Game simulation statistics", "[game][statistics][!mayfail]") {
  const size_t num_games = 100;
  std::vector<int> winner_distribution;
  std::vector<int> round_counts;

  for (size_t i = 0; i < num_games; ++i) {
    GameIntegrationFixture fixture;
    fixture.create_game_with_players(4);

    int rounds = 0;
    while (!fixture.game_->is_game_over() && rounds < 100) {
      // Simulate random play
      auto current_state = fixture.game_->get_game_state();
      auto last_guess = current_state.get_last_guess();

      if (last_guess && fixture.main_rng_->generate_bool()) {
        // Call liar sometimes
        fixture.game_->process_liar_call(current_state.get_current_player_index());
      } else {
        // Make a guess
        unsigned int dice_count =
            last_guess ? last_guess->dice_count + 1 : fixture.main_rng_->generate(1, 3);
        unsigned int face_value = fixture.main_rng_->generate(1, 6);

        Guess guess{dice_count, face_value,
                    static_cast<int>(current_state.get_current_player_index())};

        if (fixture.game_->validate_guess(guess).empty()) {
          fixture.game_->process_guess(guess);
        }
      }

      rounds++;
    }

    if (fixture.game_->is_game_over()) {
      winner_distribution.push_back(fixture.game_->get_winner_id());
      round_counts.push_back(rounds);
    }
  }

  // Analyze results
  INFO("Games completed: " << winner_distribution.size());
  INFO("Average rounds per game: "
       << std::accumulate(round_counts.begin(), round_counts.end(), 0.0) / round_counts.size());

  // We expect some reasonable distribution
  REQUIRE(winner_distribution.size() > num_games / 2);
}