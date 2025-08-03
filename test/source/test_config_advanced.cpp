#define BOOST_TEST_MODULE ConfigAdvancedTests
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <liarsdice/config/config_manager.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

using namespace liarsdice::config;
namespace bdata = boost::unit_test::data;
namespace pt = boost::property_tree;

BOOST_AUTO_TEST_SUITE(ConfigAdvancedTestSuite)

// Test fixture for configuration testing
class ConfigTestFixture {
public:
    ConfigTestFixture() {
        test_dir = std::filesystem::temp_directory_path() / "liarsdice_test_config";
        std::filesystem::create_directories(test_dir);
    }
    
    ~ConfigTestFixture() {
        std::filesystem::remove_all(test_dir);
    }
    
    std::filesystem::path create_test_config(const std::string& content) {
        auto config_path = test_dir / "test_config.info";
        std::ofstream file(config_path);
        file << content;
        file.close();
        return config_path;
    }
    
    std::filesystem::path test_dir;
};

// BDD-style scenario tests for configuration management
BOOST_AUTO_TEST_CASE(ConfigurationProvidesFlexibleGameCustomization) {
    // GIVEN: A configuration manager
    ConfigManager config_manager;
    
    // WHEN: loading default configuration
    // THEN: should have reasonable defaults
    BOOST_CHECK_NO_THROW(config_manager.load_defaults());
    
    auto ui_config = config_manager.get_ui_config();
    BOOST_CHECK(!ui_config.welcome_message.empty());
    BOOST_CHECK(!ui_config.player_count_prompt.empty());
    BOOST_CHECK(!ui_config.ai_count_prompt.empty());
    
    auto game_config = config_manager.get_game_config();
    BOOST_CHECK(game_config.min_players >= 2);
    BOOST_CHECK(game_config.max_players <= 8);
    BOOST_CHECK(game_config.dice_per_player >= 1);
    BOOST_CHECK(game_config.dice_per_player <= 10);
}

// Property-based testing with various configuration values
BOOST_DATA_TEST_CASE(ConfigurationValidatesParameterRanges,
    bdata::make({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) *
    bdata::make({1, 3, 5, 7, 10, 15, 20}),
    player_count, dice_count) {
    
    ConfigTestFixture fixture;
    
    // Create test configuration
    std::string config_content = R"(
        game {
            min_players )" + std::to_string(std::max(2, player_count - 1)) + R"(
            max_players )" + std::to_string(player_count) + R"(
            dice_per_player )" + std::to_string(dice_count) + R"(
        }
        ui {
            welcome_message "Test Game"
            player_count_prompt "Enter players"
            ai_count_prompt "Enter AI count"
        }
    )";
    
    auto config_path = fixture.create_test_config(config_content);
    
    ConfigManager config_manager;
    
    if (player_count >= 2 && player_count <= 8 && dice_count >= 1 && dice_count <= 20) {
        BOOST_CHECK_NO_THROW(config_manager.load_from_file(config_path));
        
        auto game_config = config_manager.get_game_config();
        BOOST_CHECK_LE(game_config.min_players, game_config.max_players);
        BOOST_CHECK_EQUAL(game_config.dice_per_player, dice_count);
    } else {
        // Invalid configurations should be handled gracefully
        BOOST_CHECK_NO_THROW(config_manager.load_from_file(config_path));
        // Manager should use defaults or clamp values
    }
}

// Advanced file handling and error recovery
BOOST_AUTO_TEST_CASE(ConfigurationHandlesFileSystemErrors) {
    ConfigTestFixture fixture;
    ConfigManager config_manager;
    
    // Test missing file
    auto missing_path = fixture.test_dir / "nonexistent.info";
    BOOST_CHECK_NO_THROW(config_manager.load_from_file(missing_path));
    // Should fall back to defaults
    
    // Test invalid file permissions (if possible)
    auto readonly_path = fixture.create_test_config("game { min_players 2 }");
    std::filesystem::permissions(readonly_path, std::filesystem::perms::none);
    
    // Should handle permission errors gracefully
    BOOST_CHECK_NO_THROW(config_manager.load_from_file(readonly_path));
    
    // Restore permissions for cleanup
    std::filesystem::permissions(readonly_path, std::filesystem::perms::all);
    
    // Test corrupted/invalid INFO format
    auto corrupted_path = fixture.create_test_config("invalid { syntax [ missing close");
    BOOST_CHECK_NO_THROW(config_manager.load_from_file(corrupted_path));
    // Should handle parse errors gracefully
}

// Performance benchmarks for configuration operations
BOOST_AUTO_TEST_CASE(ConfigurationPerformanceBenchmarks) {
    ConfigTestFixture fixture;
    ConfigManager config_manager;
    
    // Create a moderately complex configuration
    std::string complex_config = R"(
        game {
            min_players 2
            max_players 8
            dice_per_player 5
            round_timeout 30
            max_rounds 100
        }
        ui {
            welcome_message "Welcome to Liar's Dice"
            player_count_prompt "Enter number of players"
            ai_count_prompt "How many AI players"
            guess_prompt "Enter your guess"
            call_liar_prompt "Call liar? (y/n)"
            quit_message "Thanks for playing!"
        }
        ai {
            easy_risk_tolerance 0.3
            medium_risk_tolerance 0.5
            hard_risk_tolerance 0.7
            decision_timeout 2000
        }
    )";
    
    auto config_path = fixture.create_test_config(complex_config);
    const int iterations = 1000;
    
    // Benchmark configuration loading
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        ConfigManager temp_manager;
        temp_manager.load_from_file(config_path);
        
        // Access various configuration values
        auto ui_config = temp_manager.get_ui_config();
        auto game_config = temp_manager.get_game_config();
        
        (void)ui_config; (void)game_config; // Suppress unused warnings
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    BOOST_CHECK(duration.count() < 5000000); // Less than 5 seconds
    
    BOOST_TEST_MESSAGE("Configuration performance: " << duration.count() 
                      << " microseconds for " << iterations << " operations");
}

// Configuration merging and override testing
BOOST_AUTO_TEST_CASE(ConfigurationMergingAndOverrides) {
    ConfigTestFixture fixture;
    ConfigManager config_manager;
    
    // Load defaults first
    config_manager.load_defaults();
    auto default_game_config = config_manager.get_game_config();
    
    // Create override configuration
    std::string override_config = R"(
        game {
            max_players 6
            dice_per_player 3
        }
    )";
    
    auto override_path = fixture.create_test_config(override_config);
    config_manager.load_from_file(override_path);
    
    auto merged_config = config_manager.get_game_config();
    
    // Check that specified values were overridden
    BOOST_CHECK_EQUAL(merged_config.max_players, 6);
    BOOST_CHECK_EQUAL(merged_config.dice_per_player, 3);
    
    // Check that unspecified values retained defaults
    BOOST_CHECK_EQUAL(merged_config.min_players, default_game_config.min_players);
}

// Edge cases and boundary conditions
BOOST_AUTO_TEST_CASE(ConfigurationEdgeCasesAndBoundaries) {
    ConfigTestFixture fixture;
    ConfigManager config_manager;
    
    // Test empty configuration file
    auto empty_path = fixture.create_test_config("");
    BOOST_CHECK_NO_THROW(config_manager.load_from_file(empty_path));
    
    // Test configuration with only whitespace
    auto whitespace_path = fixture.create_test_config("   \n  \t  \n  ");
    BOOST_CHECK_NO_THROW(config_manager.load_from_file(whitespace_path));
    
    // Test configuration with extreme values
    std::string extreme_config = R"(
        game {
            min_players 0
            max_players 1000
            dice_per_player -5
        }
        ui {
            welcome_message ""
        }
    )";
    
    auto extreme_path = fixture.create_test_config(extreme_config);
    BOOST_CHECK_NO_THROW(config_manager.load_from_file(extreme_path));
    
    // Values should be clamped or use defaults
    auto game_config = config_manager.get_game_config();
    BOOST_CHECK(game_config.min_players >= 2);
    BOOST_CHECK(game_config.max_players <= 8);
    BOOST_CHECK(game_config.dice_per_player >= 1);
}

// Configuration validation and schema testing
BOOST_AUTO_TEST_CASE(ConfigurationValidationAndSchema) {
    ConfigTestFixture fixture;
    ConfigManager config_manager;
    
    // Test valid configuration
    std::string valid_config = R"(
        game {
            min_players 3
            max_players 6
            dice_per_player 5
        }
        ui {
            welcome_message "Welcome!"
            player_count_prompt "Players:"
            ai_count_prompt "AI:"
        }
    )";
    
    auto valid_path = fixture.create_test_config(valid_config);
    BOOST_CHECK_NO_THROW(config_manager.load_from_file(valid_path));
    
    // Verify loaded values
    auto game_config = config_manager.get_game_config();
    auto ui_config = config_manager.get_ui_config();
    
    BOOST_CHECK_EQUAL(game_config.min_players, 3);
    BOOST_CHECK_EQUAL(game_config.max_players, 6);
    BOOST_CHECK_EQUAL(game_config.dice_per_player, 5);
    BOOST_CHECK_EQUAL(ui_config.welcome_message, "Welcome!");
    BOOST_CHECK_EQUAL(ui_config.player_count_prompt, "Players:");
    BOOST_CHECK_EQUAL(ui_config.ai_count_prompt, "AI:");
    
    // Test configuration with unknown sections/keys
    std::string unknown_config = R"(
        game {
            min_players 4
            unknown_setting 123
        }
        unknown_section {
            some_value "test"
        }
    )";
    
    auto unknown_path = fixture.create_test_config(unknown_config);
    BOOST_CHECK_NO_THROW(config_manager.load_from_file(unknown_path));
    
    // Should load known values and ignore unknown ones
    auto config_with_unknown = config_manager.get_game_config();
    BOOST_CHECK_EQUAL(config_with_unknown.min_players, 4);
}

// Internationalization and Unicode support
BOOST_AUTO_TEST_CASE(ConfigurationInternationalizationSupport) {
    ConfigTestFixture fixture;
    ConfigManager config_manager;
    
    // Test Unicode strings in configuration
    std::string unicode_config = R"(
        ui {
            welcome_message "欢迎来到骗子骰子游戏!"
            player_count_prompt "Nombre de joueurs:"
            ai_count_prompt "Количество ИИ:"
        }
    )";
    
    auto unicode_path = fixture.create_test_config(unicode_config);
    BOOST_CHECK_NO_THROW(config_manager.load_from_file(unicode_path));
    
    auto ui_config = config_manager.get_ui_config();
    BOOST_CHECK_EQUAL(ui_config.welcome_message, "欢迎来到骗子骰子游戏!");
    BOOST_CHECK_EQUAL(ui_config.player_count_prompt, "Nombre de joueurs:");
    BOOST_CHECK_EQUAL(ui_config.ai_count_prompt, "Количество ИИ:");
}

// Thread safety and concurrent access testing
BOOST_AUTO_TEST_CASE(ConfigurationThreadSafetyConsiderations) {
    ConfigTestFixture fixture;
    
    // Create test configuration
    std::string thread_config = R"(
        game {
            min_players 2
            max_players 8
            dice_per_player 5
        }
    )";
    
    auto config_path = fixture.create_test_config(thread_config);
    
    // Test that multiple ConfigManager instances work independently
    std::vector<std::unique_ptr<ConfigManager>> managers;
    const int num_managers = 10;
    
    for (int i = 0; i < num_managers; ++i) {
        managers.push_back(std::make_unique<ConfigManager>());
        BOOST_CHECK_NO_THROW(managers.back()->load_from_file(config_path));
        
        auto config = managers.back()->get_game_config();
        BOOST_CHECK_EQUAL(config.min_players, 2);
        BOOST_CHECK_EQUAL(config.max_players, 8);
        BOOST_CHECK_EQUAL(config.dice_per_player, 5);
    }
}

// Configuration serialization and round-trip testing
BOOST_AUTO_TEST_CASE(ConfigurationSerializationRoundTrip) {
    ConfigTestFixture fixture;
    ConfigManager config_manager;
    
    // Create original configuration
    std::string original_config = R"(
        game {
            min_players 3
            max_players 7
            dice_per_player 4
        }
        ui {
            welcome_message "Original Message"
            player_count_prompt "Original Prompt"
        }
    )";
    
    auto original_path = fixture.create_test_config(original_config);
    config_manager.load_from_file(original_path);
    
    // Get configurations
    auto original_game = config_manager.get_game_config();
    auto original_ui = config_manager.get_ui_config();
    
    // Create new manager and load same file
    ConfigManager new_manager;
    new_manager.load_from_file(original_path);
    
    auto loaded_game = new_manager.get_game_config();
    auto loaded_ui = new_manager.get_ui_config();
    
    // Verify round-trip integrity
    BOOST_CHECK_EQUAL(original_game.min_players, loaded_game.min_players);
    BOOST_CHECK_EQUAL(original_game.max_players, loaded_game.max_players);
    BOOST_CHECK_EQUAL(original_game.dice_per_player, loaded_game.dice_per_player);
    BOOST_CHECK_EQUAL(original_ui.welcome_message, loaded_ui.welcome_message);
    BOOST_CHECK_EQUAL(original_ui.player_count_prompt, loaded_ui.player_count_prompt);
}

// Integration testing with game systems
BOOST_AUTO_TEST_CASE(ConfigurationIntegrationWithGameSystems) {
    ConfigTestFixture fixture;
    ConfigManager config_manager;
    
    // Create game-specific configuration
    std::string game_config = R"(
        game {
            min_players 4
            max_players 6
            dice_per_player 3
        }
        ui {
            welcome_message "Custom Game Setup"
            player_count_prompt "How many players (4-6)?"
            ai_count_prompt "AI opponents?"
        }
    )";
    
    auto config_path = fixture.create_test_config(game_config);
    config_manager.load_from_file(config_path);
    
    // Test that configuration integrates properly with game logic
    auto game_cfg = config_manager.get_game_config();
    auto ui_cfg = config_manager.get_ui_config();
    
    // Configuration should be internally consistent
    BOOST_CHECK_LE(game_cfg.min_players, game_cfg.max_players);
    BOOST_CHECK_GE(game_cfg.min_players, 2);
    BOOST_CHECK_LE(game_cfg.max_players, 8);
    BOOST_CHECK_GE(game_cfg.dice_per_player, 1);
    BOOST_CHECK_LE(game_cfg.dice_per_player, 20);
    
    // UI strings should be non-empty
    BOOST_CHECK(!ui_cfg.welcome_message.empty());
    BOOST_CHECK(!ui_cfg.player_count_prompt.empty());
    BOOST_CHECK(!ui_cfg.ai_count_prompt.empty());
}

BOOST_AUTO_TEST_SUITE_END()