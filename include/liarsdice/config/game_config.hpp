#pragma once

/**
 * @file game_config.hpp
 * @brief Game-specific configuration with type-safe enums and validation
 */

#include "config_manager.hpp"
#include <chrono>

namespace liarsdice::config {

/**
 * @brief Game difficulty levels
 */
enum class Difficulty : uint8_t { Beginner = 1, Easy = 2, Normal = 3, Hard = 4, Expert = 5 };

/**
 * @brief Dice face count options
 */
enum class DiceFaces : uint8_t { Four = 4, Six = 6, Eight = 8, Ten = 10, Twelve = 12, Twenty = 20 };

/**
 * @brief Game variant types
 */
enum class GameVariant {
  Classic,  // Standard Liar's Dice
  Perudo,   // Perudo variant with wild aces
  Dudo,     // South American variant
  Challenge // Challenge-based variant
};

/**
 * @brief Player turn timeout handling
 */
enum class TimeoutAction {
  Forfeit,   // Player forfeits the round
  AutoCall,  // Automatically call "liar"
  RandomBid, // Make a random valid bid
  Extend     // Extend timeout once
};

/**
 * @brief UI theme options
 */
enum class UITheme {
  Auto,        // Follow system theme
  Light,       // Light theme
  Dark,        // Dark theme
  HighContrast // High contrast for accessibility
};

/**
 * @brief Sound configuration
 */
enum class SoundMode {
  Off,       // No sounds
  Essential, // Only important sounds
  Full       // All sound effects
};

/**
 * @brief Game rules configuration
 */
struct GameRules {
  uint32_t min_players{2};
  uint32_t max_players{6};
  uint32_t dice_per_player{5};
  DiceFaces dice_faces{DiceFaces::Six};
  GameVariant variant{GameVariant::Classic};
  bool allow_spectators{true};
  std::chrono::seconds turn_timeout{60};
  TimeoutAction timeout_action{TimeoutAction::Extend};
  bool show_dice_count{false}; // Show total dice count to players
  bool enable_undo{false};     // Allow players to undo their last action

  /**
   * @brief Validate game rules
   */
  std::vector<std::string> validate() const;

  /**
   * @brief Get rules description
   */
  std::string describe() const;
};

/**
 * @brief UI preferences configuration
 */
struct UIPreferences {
  UITheme theme{UITheme::Auto};
  bool show_animations{true};
  bool show_tooltips{true};
  bool confirm_actions{true};
  uint32_t animation_speed{100}; // Percentage (50-200)
  std::string language{"en"};
  bool accessibility_mode{false};
  uint32_t font_scale{100}; // Percentage (75-150)

  /**
   * @brief Validate UI preferences
   */
  std::vector<std::string> validate() const;
};

/**
 * @brief Sound configuration
 */
struct SoundConfig {
  SoundMode mode{SoundMode::Full};
  uint32_t master_volume{70};  // 0-100
  uint32_t effects_volume{80}; // 0-100
  uint32_t ambient_volume{50}; // 0-100
  bool mute_when_unfocused{true};

  /**
   * @brief Validate sound configuration
   */
  std::vector<std::string> validate() const;
};

/**
 * @brief AI difficulty and behavior configuration
 */
struct AIConfig {
  Difficulty default_difficulty{Difficulty::Normal};
  bool enable_ai_learning{false};
  bool show_ai_thinking{true};
  std::chrono::milliseconds ai_delay_min{500};
  std::chrono::milliseconds ai_delay_max{2000};
  double bluff_frequency{0.15};    // 0.0-1.0
  double conservative_factor{0.5}; // 0.0-1.0 (higher = more conservative)

  /**
   * @brief Validate AI configuration
   */
  std::vector<std::string> validate() const;
};

/**
 * @brief Network/multiplayer configuration
 */
struct NetworkConfig {
  uint16_t default_port{7777};
  std::chrono::seconds connection_timeout{30};
  uint32_t max_reconnect_attempts{3};
  bool enable_lan_discovery{true};
  std::string server_region{"auto"};

  /**
   * @brief Validate network configuration
   */
  std::vector<std::string> validate() const;
};

/**
 * @brief Complete game configuration
 */
class GameConfig {
public:
  /**
   * @brief Constructor
   */
  explicit GameConfig(ConfigManager &config_manager = global_config());

  /**
   * @brief Load configuration from all sources
   */
  void load();

  /**
   * @brief Save configuration (to runtime overrides)
   */
  void save();

  /**
   * @brief Validate all configuration sections
   */
  std::vector<std::string> validate_all() const;

  /**
   * @brief Reset to defaults
   */
  void reset_to_defaults();

  /**
   * @brief Get configuration version for compatibility
   */
  uint32_t get_version() const { return version_; }

  // Configuration sections
  GameRules rules;
  UIPreferences ui;
  SoundConfig sound;
  AIConfig ai;
  NetworkConfig network;

private:
  void load_game_rules();
  void load_ui_preferences();
  void load_sound_config();
  void load_ai_config();
  void load_network_config();

  void save_game_rules();
  void save_ui_preferences();
  void save_sound_config();
  void save_ai_config();
  void save_network_config();

  ConfigManager &config_manager_;
  static constexpr uint32_t version_ = 1;
};

/**
 * @brief Configuration validation helpers
 */
namespace validation {

/**
 * @brief Validate percentage value (0-100)
 */
constexpr auto percentage_validator() { return make_range_validator<uint32_t>(0, 100); }

/**
 * @brief Validate extended percentage value (50-200)
 */
constexpr auto extended_percentage_validator() { return make_range_validator<uint32_t>(50, 200); }

/**
 * @brief Validate port number
 */
constexpr auto port_validator() { return make_range_validator<uint16_t>(1024, 65535); }

/**
 * @brief Validate player count
 */
constexpr auto player_count_validator(uint32_t min = 2, uint32_t max = 8) {
  return make_range_validator<uint32_t>(min, max);
}

/**
 * @brief Validate timeout duration
 */
constexpr auto timeout_validator() {
  return ConfigValidator<std::chrono::seconds>(
      [](const std::chrono::seconds &duration) {
        return duration.count() >= 10 && duration.count() <= 300;
      },
      []() { return "Timeout must be between 10 and 300 seconds"; });
}

} // namespace validation

/**
 * @brief String conversion functions for enums
 */
std::string to_string(Difficulty difficulty);
std::string to_string(DiceFaces faces);
std::string to_string(GameVariant variant);
std::string to_string(TimeoutAction action);
std::string to_string(UITheme theme);
std::string to_string(SoundMode mode);

/**
 * @brief Parsing functions for enums
 */
std::optional<Difficulty> parse_difficulty(const std::string &str);
std::optional<DiceFaces> parse_dice_faces(const std::string &str);
std::optional<GameVariant> parse_game_variant(const std::string &str);
std::optional<TimeoutAction> parse_timeout_action(const std::string &str);
std::optional<UITheme> parse_ui_theme(const std::string &str);
std::optional<SoundMode> parse_sound_mode(const std::string &str);

} // namespace liarsdice::config