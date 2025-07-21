/**
 * @file config_sources_test.cpp
 * @brief Tests for configuration source implementations
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#ifdef LIARSDICE_ENABLE_CONFIG
#include "liarsdice/config/config_sources.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace liarsdice::config;

TEST_CASE("DefaultsSource functionality", "[config][sources][defaults]") {
  DefaultsSource source(10);

  SECTION("Basic operations") {
    REQUIRE(source.get_priority() == 10);
    REQUIRE(source.get_name() == "Defaults");
    REQUIRE_FALSE(source.has_value(ConfigPath{"nonexistent"}));
  }

  SECTION("Adding default values") {
    source.add_default(ConfigPath{"test.key"}, "test_value");
    source.add_default(ConfigPath{"test.number"}, 42);

    REQUIRE(source.has_value(ConfigPath{"test.key"}));
    REQUIRE(source.get_raw_value(ConfigPath{"test.key"}) == "test_value");
    REQUIRE(source.get_raw_value(ConfigPath{"test.number"}) == "42");
  }

  SECTION("Adding multiple defaults") {
    source.add_defaults({
        {ConfigPath{"game.max_players"},    "6"},
        {ConfigPath{"game.min_players"},    "2"},
        {        ConfigPath{"ui.theme"}, "dark"}
    });

    auto paths = source.get_all_paths();
    REQUIRE(paths.size() == 3);

    REQUIRE(source.get_raw_value(ConfigPath{"game.max_players"}) == "6");
    REQUIRE(source.get_raw_value(ConfigPath{"ui.theme"}) == "dark");
  }
}

TEST_CASE("EnvironmentSource functionality", "[config][sources][environment]") {
  EnvironmentSource source("TEST_", 50);

  SECTION("Basic operations") {
    REQUIRE(source.get_priority() == 50);
    REQUIRE(source.get_name() == "Environment(TEST_*)");
  }

  SECTION("Environment variable mapping") {
// Set a test environment variable
#ifdef _WIN32
    _putenv_s("TEST_GAME_MAX_PLAYERS", "8");
#else
    setenv("TEST_GAME_MAX_PLAYERS", "8", 1);
#endif

    REQUIRE(source.has_value(ConfigPath{"game.max.players"}));
    auto value = source.get_raw_value(ConfigPath{"game.max.players"});
    REQUIRE(value == "8");

// Clean up
#ifdef _WIN32
    _putenv_s("TEST_GAME_MAX_PLAYERS", "");
#else
    unsetenv("TEST_GAME_MAX_PLAYERS");
#endif
  }
}

TEST_CASE("CommandLineSource functionality", "[config][sources][cmdline]") {
  SECTION("Parsing command line arguments") {
    std::vector<std::string_view> args = {"--game.max-players=6", "--ui.theme=dark", "--verbose",
                                          "--config=/path/to/config.json"};

    CommandLineSource source(args, 200);

    REQUIRE(source.get_priority() == 200);
    REQUIRE(source.get_name() == "CommandLine");

    REQUIRE(source.has_value(ConfigPath{"game.max.players"}));
    REQUIRE(source.get_raw_value(ConfigPath{"game.max.players"}) == "6");

    REQUIRE(source.has_value(ConfigPath{"ui.theme"}));
    REQUIRE(source.get_raw_value(ConfigPath{"ui.theme"}) == "dark");

    REQUIRE(source.has_value(ConfigPath{"verbose"}));
    REQUIRE(source.get_raw_value(ConfigPath{"verbose"}) == "true");

    REQUIRE(source.has_value(ConfigPath{"config"}));
    REQUIRE(source.get_raw_value(ConfigPath{"config"}) == "/path/to/config.json");
  }
}

TEST_CASE("ArgumentParser functionality", "[config][sources][parser]") {
  SECTION("Parse long options with values") {
    auto parsed = ArgumentParser::parse_single("--key=value");
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->key == "key");
    REQUIRE(parsed->value == "value");
    REQUIRE_FALSE(parsed->is_flag);
  }

  SECTION("Parse long options as flags") {
    auto parsed = ArgumentParser::parse_single("--verbose");
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->key == "verbose");
    REQUIRE_FALSE(parsed->value.has_value());
    REQUIRE(parsed->is_flag);
  }

  SECTION("Parse short options") {
    auto parsed = ArgumentParser::parse_single("-v", "value");
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->key == "v");
    REQUIRE(parsed->value == "value");
    REQUIRE_FALSE(parsed->is_flag);
  }

  SECTION("Key normalization") {
    auto parsed = ArgumentParser::parse_single("--game-max-players=6");
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->key == "game.max.players");
  }

  SECTION("Invalid arguments") {
    auto parsed = ArgumentParser::parse_single("invalid");
    REQUIRE_FALSE(parsed.has_value());
  }
}

TEST_CASE("EnvironmentWrapper functionality", "[config][sources][env_wrapper]") {
  SECTION("Get existing environment variable") {
    // PATH should exist on all systems
    auto path = EnvironmentWrapper::get("PATH");
    REQUIRE(path.has_value());
    REQUIRE_FALSE(path->empty());
  }

  SECTION("Get non-existent environment variable") {
    auto value = EnvironmentWrapper::get("NONEXISTENT_VAR_12345");
    REQUIRE_FALSE(value.has_value());
  }

  SECTION("Get with default value") {
    auto value = EnvironmentWrapper::get_or("NONEXISTENT_VAR_12345", "default");
    REQUIRE(value == "default");

    // Test with existing variable
    auto path = EnvironmentWrapper::get_or("PATH", "default");
    REQUIRE(path != "default");
  }

  SECTION("Check if environment variable exists") {
    REQUIRE(EnvironmentWrapper::exists("PATH"));
    REQUIRE_FALSE(EnvironmentWrapper::exists("NONEXISTENT_VAR_12345"));
  }

  SECTION("Set and get environment variable") {
    REQUIRE(EnvironmentWrapper::set("TEST_VAR_12345", "test_value"));
    REQUIRE(EnvironmentWrapper::exists("TEST_VAR_12345"));

    auto value = EnvironmentWrapper::get("TEST_VAR_12345");
    REQUIRE(value.has_value());
    REQUIRE(*value == "test_value");
  }
}

TEST_CASE("JsonFileSource functionality", "[config][sources][json]") {
  // Create a temporary JSON file for testing
  std::string temp_file = "test_config.json";

  SECTION("Valid JSON file") {
    {
      std::ofstream file(temp_file);
      file << R"({
                "game": {
                    "rules": {
                        "max_players": 6,
                        "dice_per_player": 5
                    }
                },
                "ui": {
                    "theme": "dark",
                    "show_animations": true
                }
            })";
    }

    JsonFileSource source(temp_file, 100);

    REQUIRE(source.get_priority() == 100);
    REQUIRE(source.get_name() == "JsonFile(test_config.json)");
    REQUIRE(source.is_valid());

#ifdef LIARSDICE_HAS_NLOHMANN_JSON
    REQUIRE(source.has_value(ConfigPath{"game.rules.max_players"}));
    REQUIRE(source.get_raw_value(ConfigPath{"game.rules.max_players"}) == "6");

    REQUIRE(source.has_value(ConfigPath{"ui.theme"}));
    REQUIRE(source.get_raw_value(ConfigPath{"ui.theme"}) == "dark");

    auto paths = source.get_all_paths();
    REQUIRE(paths.size() >= 4); // At least the 4 values we defined
#endif

    // Clean up
    std::filesystem::remove(temp_file);
  }

  SECTION("Invalid JSON file") {
    {
      std::ofstream file(temp_file);
      file << "invalid json content {";
    }

    JsonFileSource source(temp_file, 100);
    REQUIRE(source.is_valid()); // File exists but invalid JSON

    // Should handle invalid JSON gracefully
    auto paths = source.get_all_paths();
    // Should be empty or fallback to simple parsing

    // Clean up
    std::filesystem::remove(temp_file);
  }

  SECTION("Non-existent file") {
    JsonFileSource source("nonexistent_file.json", 100);
    REQUIRE_FALSE(source.is_valid());

    auto paths = source.get_all_paths();
    REQUIRE(paths.empty());
  }
}

#endif // LIARSDICE_ENABLE_CONFIG