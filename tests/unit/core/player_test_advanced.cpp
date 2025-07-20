#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "liarsdice/core/player_impl.hpp"
#include "liarsdice/core/dice_impl.hpp"
#include "liarsdice/interfaces/i_service_factory.hpp"
#include <memory>
#include <algorithm>
#include <random>

using namespace liarsdice::core;
using namespace liarsdice::interfaces;

// Test helper classes
class TestRandomGenerator : public IRandomGenerator {
private:
    mutable std::vector<int> sequence_;
    mutable size_t index_ = 0;
    mutable std::mt19937 fallback_gen_{42};
    
public:
    explicit TestRandomGenerator(std::vector<int> sequence = {})
        : sequence_(std::move(sequence)) {}
    
    int generate(int min, int max) override {
        if (index_ < sequence_.size()) {
            return sequence_[index_++];
        }
        std::uniform_int_distribution<int> dist(min, max);
        return dist(fallback_gen_);
    }
    
    void seed(unsigned int seed) override { fallback_gen_.seed(seed); }
    bool generate_bool() override { return generate(0, 1) == 1; }
    double generate_normalized() override { return static_cast<double>(generate(0, 1000)) / 1000.0; }
};

class TestDiceFactory : public ServiceFactory<IDice> {
public:
    TestDiceFactory() : ServiceFactory<IDice>(
        []() -> std::unique_ptr<IDice> {
            auto rng = std::make_unique<TestRandomGenerator>();
            return std::make_unique<DiceImpl>(
                std::unique_ptr<IRandomGenerator>(rng.release())
            );
        }
    ) {}
};

// Test fixture
class PlayerTestFixture {
public:
    std::unique_ptr<PlayerImpl> create_player(int id, std::vector<int> rng_sequence = {}) {
        auto rng = std::make_unique<TestRandomGenerator>(std::move(rng_sequence));
        auto factory = std::make_unique<TestDiceFactory>();
        
        return std::make_unique<PlayerImpl>(id, std::move(rng), std::move(factory));
    }
};

// BDD-style scenario tests
SCENARIO("Player manages dice collection throughout game", "[player][bdd]") {
    PlayerTestFixture fixture;
    
    GIVEN("A new player with ID 42") {
        auto player = fixture.create_player(42);
        
        WHEN("checking initial state") {
            THEN("player has correct ID and no dice") {
                REQUIRE(player->get_id() == 42);
                REQUIRE(player->get_dice_count() == 0);
                REQUIRE_FALSE(player->has_dice());
                REQUIRE_FALSE(player->is_active());
            }
        }
        
        WHEN("adding dice to the player") {
            player->add_die();
            player->add_die();
            player->add_die();
            
            THEN("player has correct dice count and becomes active") {
                REQUIRE(player->get_dice_count() == 3);
                REQUIRE(player->has_dice());
                REQUIRE(player->is_active());
                
                auto dice_refs = player->get_dice();
                REQUIRE(dice_refs.size() == 3);
                
                for (const auto& die_ref : dice_refs) {
                    auto value = die_ref.get().get_face_value();
                    REQUIRE(value >= 1);
                    REQUIRE(value <= 6);
                }
            }
            
            AND_WHEN("rolling all dice") {
                REQUIRE_NOTHROW(player->roll_dice());
                
                THEN("all dice maintain valid values") {
                    auto dice_values = player->get_dice_values();
                    REQUIRE(dice_values.size() == 3);
                    
                    for (auto value : dice_values) {
                        REQUIRE(value >= 1);
                        REQUIRE(value <= 6);
                    }
                }
            }
            
            AND_WHEN("removing dice systematically") {
                REQUIRE(player->remove_die()); // 3 -> 2
                REQUIRE(player->get_dice_count() == 2);
                REQUIRE(player->is_active());
                
                REQUIRE(player->remove_die()); // 2 -> 1
                REQUIRE(player->get_dice_count() == 1);
                REQUIRE(player->is_active());
                
                REQUIRE(player->remove_die()); // 1 -> 0
                
                THEN("player becomes eliminated") {
                    REQUIRE(player->get_dice_count() == 0);
                    REQUIRE_FALSE(player->has_dice());
                    REQUIRE_FALSE(player->is_active());
                    REQUIRE_FALSE(player->remove_die()); // Can't remove from empty
                }
            }
        }
    }
}

// Property-based testing
TEST_CASE("Player properties hold under various conditions", "[player][property]") {
    PlayerTestFixture fixture;
    
    SECTION("Player ID validation") {
        auto player_id = GENERATE(-10, -5, -1, 0, 1, 2, 5, 10);
        
        if (player_id > 0) {
            REQUIRE_NOTHROW(fixture.create_player(player_id));
            auto player = fixture.create_player(player_id);
            REQUIRE(player->get_id() == player_id);
        } else {
            REQUIRE_THROWS_AS(
                fixture.create_player(player_id),
                std::invalid_argument
            );
        }
    }
    
    SECTION("Dice operations consistency") {
        auto player = fixture.create_player(1);
        auto num_operations = GENERATE(1, 3, 5, 10, 15, 20);
        
        size_t expected_count = 0;
        
        for (int i = 0; i < num_operations; ++i) {
            auto operation = GENERATE(0, 1, 1, 1); // Bias toward adding dice
            
            if (operation == 0 && expected_count > 0) {
                // Remove die
                REQUIRE(player->remove_die());
                expected_count--;
            } else {
                // Add die
                player->add_die();
                expected_count++;
            }
            
            REQUIRE(player->get_dice_count() == expected_count);
            REQUIRE(player->has_dice() == (expected_count > 0));
            REQUIRE(player->is_active() == (expected_count > 0));
        }
    }
}

// Advanced dice counting tests
TEST_CASE("Player dice value counting and statistics", "[player][dice-counting]") {
    PlayerTestFixture fixture;
    auto player = fixture.create_player(1);
    
    // Add dice
    const size_t num_dice = 5;
    for (size_t i = 0; i < num_dice; ++i) {
        player->add_die();
    }
    
    SECTION("Count dice with specific values") {
        auto dice_values = player->get_dice_values();
        REQUIRE(dice_values.size() == num_dice);
        
        // Test counting for each possible value
        for (unsigned int target_value = 1; target_value <= 6; ++target_value) {
            auto count = player->count_dice_with_value(target_value);
            auto expected_count = static_cast<size_t>(std::count(
                dice_values.begin(), dice_values.end(), target_value));
            
            REQUIRE(count == expected_count);
        }
        
        // Sum of all counts should equal total dice
        size_t total_counted = 0;
        for (unsigned int value = 1; value <= 6; ++value) {
            total_counted += player->count_dice_with_value(value);
        }
        REQUIRE(total_counted == num_dice);
    }
    
    SECTION("Invalid value counting") {
        REQUIRE(player->count_dice_with_value(0) == 0);
        REQUIRE(player->count_dice_with_value(7) == 0);
        REQUIRE(player->count_dice_with_value(999) == 0);
    }
}

// Performance benchmarks
TEST_CASE("Player performance benchmarks", "[player][benchmark]") {
    PlayerTestFixture fixture;
    auto player = fixture.create_player(1);
    
    // Setup player with dice
    for (int i = 0; i < 5; ++i) {
        player->add_die();
    }
    
    BENCHMARK("Add and remove die cycle") {
        player->add_die();
        player->remove_die();
        return player->get_dice_count();
    };
    
    BENCHMARK("Roll all dice") {
        player->roll_dice();
        return player->get_dice_values();
    };
    
    BENCHMARK("Count dice with value") {
        return player->count_dice_with_value(3);
    };
    
    BENCHMARK("Get all dice values") {
        return player->get_dice_values();
    };
    
    BENCHMARK("Get dice references") {
        return player->get_dice();
    };
}

// Exception safety and error handling
TEST_CASE("Player exception safety", "[player][exceptions]") {
    PlayerTestFixture fixture;
    
    SECTION("Constructor with null dependencies") {
        REQUIRE_THROWS_AS(
            PlayerImpl(1, nullptr, std::make_unique<TestDiceFactory>()),
            std::invalid_argument
        );
        
        REQUIRE_THROWS_AS(
            PlayerImpl(1, std::make_unique<TestRandomGenerator>(), nullptr),
            std::invalid_argument
        );
    }
    
    SECTION("Invalid player ID") {
        REQUIRE_THROWS_WITH(
            fixture.create_player(0),
            Catch::Matchers::ContainsSubstring("Player ID must be positive")
        );
        
        REQUIRE_THROWS_WITH(
            fixture.create_player(-5),
            Catch::Matchers::ContainsSubstring("Player ID must be positive")
        );
    }
}

// Integration test scenarios
TEST_CASE("Player integration scenarios", "[player][integration]") {
    PlayerTestFixture fixture;
    
    SECTION("Multiple players with different dice counts") {
        std::vector<std::unique_ptr<PlayerImpl>> players;
        
        // Create players with varying dice counts
        for (int id = 1; id <= 4; ++id) {
            auto player = fixture.create_player(id);
            
            // Give each player different number of dice
            for (int dice = 0; dice < id; ++dice) {
                player->add_die();
            }
            
            players.push_back(std::move(player));
        }
        
        // Verify each player has correct setup
        for (size_t i = 0; i < players.size(); ++i) {
            auto& player = players[i];
            REQUIRE(player->get_id() == static_cast<int>(i + 1));
            REQUIRE(player->get_dice_count() == i + 1);
            REQUIRE(player->is_active() == (i > 0)); // Player 1 has 1 die, others have more
        }
        
        // Count active players
        auto active_count = std::count_if(players.begin(), players.end(),
            [](const auto& p) { return p->is_active(); });
        
        REQUIRE(active_count == 4); // All players should be active
    }
    
    SECTION("Player elimination simulation") {
        auto player = fixture.create_player(1);
        
        // Start with 3 dice
        for (int i = 0; i < 3; ++i) {
            player->add_die();
        }
        
        REQUIRE(player->is_active());
        
        // Lose dice one by one
        std::vector<size_t> dice_counts;
        while (player->has_dice()) {
            dice_counts.push_back(player->get_dice_count());
            player->remove_die();
        }
        
        REQUIRE(dice_counts == std::vector<size_t>{3, 2, 1});
        REQUIRE_FALSE(player->is_active());
    }
}

// Edge cases and boundary conditions
TEST_CASE("Player edge cases", "[player][edge-cases]") {
    PlayerTestFixture fixture;
    
    SECTION("Maximum practical dice count") {
        auto player = fixture.create_player(1);
        
        // Add a large number of dice
        const size_t max_dice = 100;
        for (size_t i = 0; i < max_dice; ++i) {
            REQUIRE_NOTHROW(player->add_die());
        }
        
        REQUIRE(player->get_dice_count() == max_dice);
        REQUIRE(player->is_active());
        
        // Verify all dice have valid values
        auto values = player->get_dice_values();
        REQUIRE(values.size() == max_dice);
        
        for (auto value : values) {
            REQUIRE(value >= 1);
            REQUIRE(value <= 6);
        }
    }
    
    SECTION("Rapid add/remove cycles") {
        auto player = fixture.create_player(1);
        
        // Perform many add/remove cycles
        for (int cycle = 0; cycle < 50; ++cycle) {
            player->add_die();
            REQUIRE(player->get_dice_count() == 1);
            REQUIRE(player->is_active());
            
            REQUIRE(player->remove_die());
            REQUIRE(player->get_dice_count() == 0);
            REQUIRE_FALSE(player->is_active());
        }
    }
}

// Custom matchers test
TEST_CASE("Player with custom validation", "[player][matchers]") {
    PlayerTestFixture fixture;
    auto player = fixture.create_player(7);
    
    SECTION("Player ID validation") {
        REQUIRE_THAT(player->get_id(), 
                     Catch::Matchers::Predicate<int>(
                         [](int id) { return id > 0 && id < 1000; },
                         "is valid player ID range"
                     ));
    }
    
    SECTION("Dice collection properties") {
        player->add_die();
        player->add_die();
        player->add_die();
        
        auto dice_refs = player->get_dice();
        REQUIRE_THAT(dice_refs, Catch::Matchers::SizeIs(3));
        
        auto dice_values = player->get_dice_values();
        REQUIRE_THAT(dice_values, Catch::Matchers::SizeIs(3));
        
        // All values should be in valid range
        for (auto value : dice_values) {
            REQUIRE_THAT(value, 
                        Catch::Matchers::Predicate<unsigned int>(
                            [](unsigned int v) { return v >= 1 && v <= 6; },
                            "is valid dice value"
                        ));
        }
    }
}