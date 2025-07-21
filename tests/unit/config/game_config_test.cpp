/**
 * @file game_config_test.cpp
 * @brief Tests for game-specific configuration
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#ifdef LIARSDICE_ENABLE_CONFIG
#include "liarsdice/config/game_config.hpp"

using namespace liarsdice::config;

TEST_CASE("Difficulty enum operations", "[config][game][difficulty]") {
  SECTION("String conversion") {
    REQUIRE(to_string(Difficulty::Beginner) == "beginner");
    REQUIRE(to_string(Difficulty::Easy) == "easy");
    REQUIRE(to_string(Difficulty::Normal) == "normal");
    REQUIRE(to_string(Difficulty::Hard) == "hard");
    REQUIRE(to_string(Difficulty::Expert) == "expert");
  }

  SECTION("String parsing") {
    REQUIRE(parse_difficulty("beginner") == Difficulty::Beginner);
    REQUIRE(parse_difficulty("normal") == Difficulty::Normal);
    REQUIRE(parse_difficulty("expert") == Difficulty::Expert);
    REQUIRE_FALSE(parse_difficulty("invalid").has_value());
  }
}

TEST_CASE("DiceFaces enum operations", "[config][game][dice_faces]") {
  SECTION("String conversion") {
    REQUIRE(to_string(DiceFaces::Four) == "4");
    REQUIRE(to_string(DiceFaces::Six) == "6");
    REQUIRE(to_string(DiceFaces::Twenty) == "20");
  }

  SECTION("String parsing") {
    REQUIRE(parse_dice_faces("4") == DiceFaces::Four);
    REQUIRE(parse_dice_faces("6") == DiceFaces::Six);
    REQUIRE(parse_dice_faces("20") == DiceFaces::Twenty);
    REQUIRE_FALSE(parse_dice_faces("3").has_value());
  }
}

TEST_CASE("GameVariant enum operations", "[config][game][variant]") {
  SECTION("String conversion") {
    REQUIRE(to_string(GameVariant::Classic) == "classic");
    REQUIRE(to_string(GameVariant::Perudo) == "perudo");
    REQUIRE(to_string(GameVariant::Dudo) == "dudo");
    REQUIRE(to_string(GameVariant::Challenge) == "challenge");
  }

  SECTION("String parsing") {
    REQUIRE(parse_game_variant("classic") == GameVariant::Classic);
    REQUIRE(parse_game_variant("perudo") == GameVariant::Perudo);
    REQUIRE_FALSE(parse_game_variant("invalid").has_value());
  }
}

TEST_CASE("UITheme enum operations", "[config][game][ui_theme]") {
  SECTION("String conversion") {
    REQUIRE(to_string(UITheme::Auto) == "auto");
    REQUIRE(to_string(UITheme::Light) == "light");
    REQUIRE(to_string(UITheme::Dark) == "dark");
    REQUIRE(to_string(UITheme::HighContrast) == "high_contrast");
  }

  SECTION("String parsing") {
    REQUIRE(parse_ui_theme("auto") == UITheme::Auto);
    REQUIRE(parse_ui_theme("dark") == UITheme::Dark);
    REQUIRE(parse_ui_theme("high_contrast") == UITheme::HighContrast);
    REQUIRE_FALSE(parse_ui_theme("invalid").has_value());
  }
}

TEST_CASE("GameRules validation", "[config][game][rules]") {
  SECTION("Valid rules") {
    GameRules rules;
    rules.min_players = 2;
    rules.max_players = 6;
    rules.dice_per_player = 5;
    rules.turn_timeout = std::chrono::seconds(60);

    auto errors = rules.validate();
    REQUIRE(errors.empty());
  }

  SECTION("Invalid player counts") {
    GameRules rules;
    rules.min_players = 1;  // Too low
    rules.max_players = 10; // Too high

    auto errors = rules.validate();
    REQUIRE_FALSE(errors.empty());
    REQUIRE(errors.size() >= 2);
  }

  SECTION("Invalid dice count") {
    GameRules rules;
    rules.dice_per_player = 0; // Invalid

    auto errors = rules.validate();
    REQUIRE_FALSE(errors.empty());
  }

  SECTION("Invalid timeout") {
    GameRules rules;
    rules.turn_timeout = std::chrono::seconds(5); // Too short

    auto errors = rules.validate();
    REQUIRE_FALSE(errors.empty());
  }

  SECTION("Rules description") {
    GameRules rules;
    auto description = rules.describe();
    REQUIRE_FALSE(description.empty());
    REQUIRE_THAT(description, Catch::Matchers::ContainsSubstring("Game Rules:"));
    REQUIRE_THAT(description, Catch::Matchers::ContainsSubstring("Players:"));
  }
}

TEST_CASE("UIPreferences validation", "[config][game][ui]") {
  SECTION("Valid preferences") {
    UIPreferences ui;
    ui.animation_speed = 100;
    ui.font_scale = 100;
    ui.language = "en";

    auto errors = ui.validate();
    REQUIRE(errors.empty());
  }

  SECTION("Invalid animation speed") {
    UIPreferences ui;
    ui.animation_speed = 30; // Too low

    auto errors = ui.validate();
    REQUIRE_FALSE(errors.empty());
  }

  SECTION("Invalid font scale") {
    UIPreferences ui;
    ui.font_scale = 200; // Too high

    auto errors = ui.validate();
    REQUIRE_FALSE(errors.empty());
  }

  SECTION("Invalid language") {
    UIPreferences ui;
    ui.language = ""; // Empty

    auto errors = ui.validate();
    REQUIRE_FALSE(errors.empty());
  }
}

TEST_CASE("SoundConfig validation", "[config][game][sound]") {
  SECTION("Valid sound config") {
    SoundConfig sound;
    sound.master_volume = 70;
    sound.effects_volume = 80;
    sound.ambient_volume = 50;

    auto errors = sound.validate();
    REQUIRE(errors.empty());
  }

  SECTION("Invalid volume levels") {
    SoundConfig sound;
    sound.master_volume = 150; // Too high

    auto errors = sound.validate();
    REQUIRE_FALSE(errors.empty());
  }
}

TEST_CASE("AIConfig validation", "[config][game][ai]") {
  SECTION("Valid AI config") {
    AIConfig ai;
    ai.ai_delay_min = std::chrono::milliseconds(500);
    ai.ai_delay_max = std::chrono::milliseconds(2000);
    ai.bluff_frequency = 0.15;
    ai.conservative_factor = 0.5;

    auto errors = ai.validate();
    REQUIRE(errors.empty());
  }

  SECTION("Invalid delay range") {
    AIConfig ai;
    ai.ai_delay_min = std::chrono::milliseconds(3000);
    ai.ai_delay_max = std::chrono::milliseconds(1000); // Min > Max

    auto errors = ai.validate();
    REQUIRE_FALSE(errors.empty());
  }

  SECTION("Invalid frequency values") {
    AIConfig ai;
    ai.bluff_frequency = 1.5;      // > 1.0
    ai.conservative_factor = -0.1; // < 0.0

    auto errors = ai.validate();
    REQUIRE(errors.size() >= 2);
  }
}

TEST_CASE("NetworkConfig validation", "[config][game][network]") {
  SECTION("Valid network config") {
    NetworkConfig network;
    network.default_port = 7777;
    network.connection_timeout = std::chrono::seconds(30);
    network.max_reconnect_attempts = 3;

    auto errors = network.validate();
    REQUIRE(errors.empty());
  }

  SECTION("Invalid port") {
    NetworkConfig network;
    network.default_port = 100; // Too low

    auto errors = network.validate();
    REQUIRE_FALSE(errors.empty());
  }

  SECTION("Invalid timeout") {
    NetworkConfig network;
    network.connection_timeout = std::chrono::seconds(200); // Too high

    auto errors = network.validate();
    REQUIRE_FALSE(errors.empty());
  }
}

TEST_CASE("GameConfig operations", "[config][game][integration]") {
  SECTION("Default construction and validation") {
    GameConfig config;

    // Should have reasonable defaults
    REQUIRE(config.rules.min_players == 2);
    REQUIRE(config.rules.max_players == 6);
    REQUIRE(config.ui.theme == UITheme::Auto);
    REQUIRE(config.sound.mode == SoundMode::Full);
    REQUIRE(config.ai.default_difficulty == Difficulty::Normal);

    // Should validate successfully with defaults
    auto errors = config.validate_all();
    REQUIRE(errors.empty());
  }

  SECTION("Reset to defaults") {
    GameConfig config;

    // Modify some values
    config.rules.max_players = 8;
    config.ui.theme = UITheme::Dark;

    // Reset and verify
    config.reset_to_defaults();
    REQUIRE(config.rules.max_players == 6);
    REQUIRE(config.ui.theme == UITheme::Auto);
  }

  SECTION("Version compatibility") {
    GameConfig config;
    REQUIRE(config.get_version() == 1);
  }
}

TEST_CASE("Configuration validators", "[config][game][validators]") {
  using namespace validation;

  SECTION("Percentage validator") {
    auto validator = percentage_validator();

    REQUIRE(validator.validate(0));
    REQUIRE(validator.validate(50));
    REQUIRE(validator.validate(100));
    REQUIRE_FALSE(validator.validate(101));
    REQUIRE_FALSE(validator.validate(static_cast<uint32_t>(-1)));
  }

  SECTION("Port validator") {
    auto validator = port_validator();

    REQUIRE(validator.validate(1024));
    REQUIRE(validator.validate(8080));
    REQUIRE(validator.validate(65535));
    REQUIRE_FALSE(validator.validate(1023));
    REQUIRE_FALSE(validator.validate(65536));
  }

  SECTION("Player count validator") {
    auto validator = player_count_validator(2, 8);

    REQUIRE(validator.validate(2));
    REQUIRE(validator.validate(6));
    REQUIRE(validator.validate(8));
    REQUIRE_FALSE(validator.validate(1));
    REQUIRE_FALSE(validator.validate(9));
  }
}

#endif // LIARSDICE_ENABLE_CONFIG