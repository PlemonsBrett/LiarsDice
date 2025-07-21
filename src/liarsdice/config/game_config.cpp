/**
 * @file game_config.cpp
 * @brief Implementation of game-specific configuration
 */

#include "liarsdice/config/game_config.hpp"
#include <algorithm>
#include <sstream>

namespace liarsdice::config {

// GameRules implementation
std::vector<std::string> GameRules::validate() const {
  std::vector<std::string> errors;

  if (min_players < 2) {
    errors.push_back("Minimum players must be at least 2");
  }

  if (max_players > 8) {
    errors.push_back("Maximum players cannot exceed 8");
  }

  if (min_players > max_players) {
    errors.push_back("Minimum players cannot be greater than maximum players");
  }

  if (dice_per_player < 1 || dice_per_player > 10) {
    errors.push_back("Dice per player must be between 1 and 10");
  }

  if (turn_timeout.count() < 10 || turn_timeout.count() > 300) {
    errors.push_back("Turn timeout must be between 10 and 300 seconds");
  }

  return errors;
}

std::string GameRules::describe() const {
  std::ostringstream oss;
  oss << "Game Rules:\n";
  oss << "  Players: " << min_players << "-" << max_players << "\n";
  oss << "  Dice per player: " << dice_per_player << "\n";
  oss << "  Dice faces: " << static_cast<int>(dice_faces) << "\n";
  oss << "  Variant: " << to_string(variant) << "\n";
  oss << "  Turn timeout: " << turn_timeout.count() << " seconds\n";
  oss << "  Timeout action: " << to_string(timeout_action) << "\n";
  oss << "  Allow spectators: " << (allow_spectators ? "Yes" : "No") << "\n";
  oss << "  Show dice count: " << (show_dice_count ? "Yes" : "No") << "\n";
  oss << "  Enable undo: " << (enable_undo ? "Yes" : "No");
  return oss.str();
}

// UIPreferences implementation
std::vector<std::string> UIPreferences::validate() const {
  std::vector<std::string> errors;

  if (animation_speed < 50 || animation_speed > 200) {
    errors.push_back("Animation speed must be between 50% and 200%");
  }

  if (font_scale < 75 || font_scale > 150) {
    errors.push_back("Font scale must be between 75% and 150%");
  }

  if (language.empty() || language.length() > 5) {
    errors.push_back("Language code must be 2-5 characters");
  }

  return errors;
}

// SoundConfig implementation
std::vector<std::string> SoundConfig::validate() const {
  std::vector<std::string> errors;

  if (master_volume > 100) {
    errors.push_back("Master volume must be between 0 and 100");
  }

  if (effects_volume > 100) {
    errors.push_back("Effects volume must be between 0 and 100");
  }

  if (ambient_volume > 100) {
    errors.push_back("Ambient volume must be between 0 and 100");
  }

  return errors;
}

// AIConfig implementation
std::vector<std::string> AIConfig::validate() const {
  std::vector<std::string> errors;

  if (ai_delay_min > ai_delay_max) {
    errors.push_back("AI minimum delay cannot be greater than maximum delay");
  }

  if (ai_delay_min.count() < 100 || ai_delay_max.count() > 10000) {
    errors.push_back("AI delays must be between 100ms and 10000ms");
  }

  if (bluff_frequency < 0.0 || bluff_frequency > 1.0) {
    errors.push_back("Bluff frequency must be between 0.0 and 1.0");
  }

  if (conservative_factor < 0.0 || conservative_factor > 1.0) {
    errors.push_back("Conservative factor must be between 0.0 and 1.0");
  }

  return errors;
}

// NetworkConfig implementation
std::vector<std::string> NetworkConfig::validate() const {
  std::vector<std::string> errors;

  if (default_port < 1024 || default_port > 65535) {
    errors.push_back("Default port must be between 1024 and 65535");
  }

  if (connection_timeout.count() < 5 || connection_timeout.count() > 120) {
    errors.push_back("Connection timeout must be between 5 and 120 seconds");
  }

  if (max_reconnect_attempts > 10) {
    errors.push_back("Maximum reconnect attempts cannot exceed 10");
  }

  return errors;
}

// GameConfig implementation
GameConfig::GameConfig(ConfigManager &config_manager) : config_manager_(config_manager) {}

void GameConfig::load() {
  load_game_rules();
  load_ui_preferences();
  load_sound_config();
  load_ai_config();
  load_network_config();
}

void GameConfig::save() {
  save_game_rules();
  save_ui_preferences();
  save_sound_config();
  save_ai_config();
  save_network_config();
}

std::vector<std::string> GameConfig::validate_all() const {
  std::vector<std::string> all_errors;

  auto rules_errors = rules.validate();
  auto ui_errors = ui.validate();
  auto sound_errors = sound.validate();
  auto ai_errors = ai.validate();
  auto network_errors = network.validate();

  all_errors.insert(all_errors.end(), rules_errors.begin(), rules_errors.end());
  all_errors.insert(all_errors.end(), ui_errors.begin(), ui_errors.end());
  all_errors.insert(all_errors.end(), sound_errors.begin(), sound_errors.end());
  all_errors.insert(all_errors.end(), ai_errors.begin(), ai_errors.end());
  all_errors.insert(all_errors.end(), network_errors.begin(), network_errors.end());

  return all_errors;
}

void GameConfig::reset_to_defaults() {
  rules = GameRules{};
  ui = UIPreferences{};
  sound = SoundConfig{};
  ai = AIConfig{};
  network = NetworkConfig{};
}

void GameConfig::load_game_rules() {
  rules.min_players =
      config_manager_.get_or<uint32_t>(ConfigPath{"game.rules.min_players"}, rules.min_players);
  rules.max_players =
      config_manager_.get_or<uint32_t>(ConfigPath{"game.rules.max_players"}, rules.max_players);
  rules.dice_per_player = config_manager_.get_or<uint32_t>(ConfigPath{"game.rules.dice_per_player"},
                                                           rules.dice_per_player);

  if (auto faces_str = config_manager_.get<std::string>(ConfigPath{"game.rules.dice_faces"})) {
    if (auto faces = parse_dice_faces(*faces_str)) {
      rules.dice_faces = *faces;
    }
  }

  if (auto variant_str = config_manager_.get<std::string>(ConfigPath{"game.rules.variant"})) {
    if (auto variant = parse_game_variant(*variant_str)) {
      rules.variant = *variant;
    }
  }

  rules.allow_spectators = config_manager_.get_or<bool>(ConfigPath{"game.rules.allow_spectators"},
                                                        rules.allow_spectators);

  if (auto timeout = config_manager_.get<int64_t>(ConfigPath{"game.rules.turn_timeout"})) {
    rules.turn_timeout = std::chrono::seconds(*timeout);
  }

  if (auto action_str = config_manager_.get<std::string>(ConfigPath{"game.rules.timeout_action"})) {
    if (auto action = parse_timeout_action(*action_str)) {
      rules.timeout_action = *action;
    }
  }

  rules.show_dice_count =
      config_manager_.get_or<bool>(ConfigPath{"game.rules.show_dice_count"}, rules.show_dice_count);
  rules.enable_undo =
      config_manager_.get_or<bool>(ConfigPath{"game.rules.enable_undo"}, rules.enable_undo);
}

void GameConfig::load_ui_preferences() {
  if (auto theme_str = config_manager_.get<std::string>(ConfigPath{"ui.theme"})) {
    if (auto theme = parse_ui_theme(*theme_str)) {
      ui.theme = *theme;
    }
  }

  ui.show_animations =
      config_manager_.get_or<bool>(ConfigPath{"ui.show_animations"}, ui.show_animations);
  ui.show_tooltips = config_manager_.get_or<bool>(ConfigPath{"ui.show_tooltips"}, ui.show_tooltips);
  ui.confirm_actions =
      config_manager_.get_or<bool>(ConfigPath{"ui.confirm_actions"}, ui.confirm_actions);
  ui.animation_speed =
      config_manager_.get_or<uint32_t>(ConfigPath{"ui.animation_speed"}, ui.animation_speed);
  ui.language = config_manager_.get_or<std::string>(ConfigPath{"ui.language"}, ui.language);
  ui.accessibility_mode =
      config_manager_.get_or<bool>(ConfigPath{"ui.accessibility_mode"}, ui.accessibility_mode);
  ui.font_scale = config_manager_.get_or<uint32_t>(ConfigPath{"ui.font_scale"}, ui.font_scale);
}

void GameConfig::load_sound_config() {
  if (auto mode_str = config_manager_.get<std::string>(ConfigPath{"sound.mode"})) {
    if (auto mode = parse_sound_mode(*mode_str)) {
      sound.mode = *mode;
    }
  }

  sound.master_volume =
      config_manager_.get_or<uint32_t>(ConfigPath{"sound.master_volume"}, sound.master_volume);
  sound.effects_volume =
      config_manager_.get_or<uint32_t>(ConfigPath{"sound.effects_volume"}, sound.effects_volume);
  sound.ambient_volume =
      config_manager_.get_or<uint32_t>(ConfigPath{"sound.ambient_volume"}, sound.ambient_volume);
  sound.mute_when_unfocused = config_manager_.get_or<bool>(ConfigPath{"sound.mute_when_unfocused"},
                                                           sound.mute_when_unfocused);
}

void GameConfig::load_ai_config() {
  if (auto difficulty_str = config_manager_.get<std::string>(ConfigPath{"ai.default_difficulty"})) {
    if (auto difficulty = parse_difficulty(*difficulty_str)) {
      ai.default_difficulty = *difficulty;
    }
  }

  ai.enable_ai_learning =
      config_manager_.get_or<bool>(ConfigPath{"ai.enable_learning"}, ai.enable_ai_learning);
  ai.show_ai_thinking =
      config_manager_.get_or<bool>(ConfigPath{"ai.show_thinking"}, ai.show_ai_thinking);

  if (auto delay_min = config_manager_.get<int64_t>(ConfigPath{"ai.delay_min_ms"})) {
    ai.ai_delay_min = std::chrono::milliseconds(*delay_min);
  }

  if (auto delay_max = config_manager_.get<int64_t>(ConfigPath{"ai.delay_max_ms"})) {
    ai.ai_delay_max = std::chrono::milliseconds(*delay_max);
  }

  ai.bluff_frequency =
      config_manager_.get_or<double>(ConfigPath{"ai.bluff_frequency"}, ai.bluff_frequency);
  ai.conservative_factor =
      config_manager_.get_or<double>(ConfigPath{"ai.conservative_factor"}, ai.conservative_factor);
}

void GameConfig::load_network_config() {
  network.default_port =
      config_manager_.get_or<uint32_t>(ConfigPath{"network.default_port"}, network.default_port);

  if (auto timeout = config_manager_.get<int64_t>(ConfigPath{"network.connection_timeout"})) {
    network.connection_timeout = std::chrono::seconds(*timeout);
  }

  network.max_reconnect_attempts = config_manager_.get_or<uint32_t>(
      ConfigPath{"network.max_reconnect_attempts"}, network.max_reconnect_attempts);
  network.enable_lan_discovery = config_manager_.get_or<bool>(
      ConfigPath{"network.enable_lan_discovery"}, network.enable_lan_discovery);
  network.server_region = config_manager_.get_or<std::string>(ConfigPath{"network.server_region"},
                                                              network.server_region);
}

void GameConfig::save_game_rules() {
  config_manager_.set<uint32_t>(ConfigPath{"game.rules.min_players"}, rules.min_players);
  config_manager_.set<uint32_t>(ConfigPath{"game.rules.max_players"}, rules.max_players);
  config_manager_.set<uint32_t>(ConfigPath{"game.rules.dice_per_player"}, rules.dice_per_player);
  config_manager_.set<std::string>(ConfigPath{"game.rules.dice_faces"},
                                   to_string(rules.dice_faces));
  config_manager_.set<std::string>(ConfigPath{"game.rules.variant"}, to_string(rules.variant));
  config_manager_.set<bool>(ConfigPath{"game.rules.allow_spectators"}, rules.allow_spectators);
  config_manager_.set<int64_t>(ConfigPath{"game.rules.turn_timeout"}, rules.turn_timeout.count());
  config_manager_.set<std::string>(ConfigPath{"game.rules.timeout_action"},
                                   to_string(rules.timeout_action));
  config_manager_.set<bool>(ConfigPath{"game.rules.show_dice_count"}, rules.show_dice_count);
  config_manager_.set<bool>(ConfigPath{"game.rules.enable_undo"}, rules.enable_undo);
}

void GameConfig::save_ui_preferences() {
  config_manager_.set<std::string>(ConfigPath{"ui.theme"}, to_string(ui.theme));
  config_manager_.set<bool>(ConfigPath{"ui.show_animations"}, ui.show_animations);
  config_manager_.set<bool>(ConfigPath{"ui.show_tooltips"}, ui.show_tooltips);
  config_manager_.set<bool>(ConfigPath{"ui.confirm_actions"}, ui.confirm_actions);
  config_manager_.set<uint32_t>(ConfigPath{"ui.animation_speed"}, ui.animation_speed);
  config_manager_.set<std::string>(ConfigPath{"ui.language"}, ui.language);
  config_manager_.set<bool>(ConfigPath{"ui.accessibility_mode"}, ui.accessibility_mode);
  config_manager_.set<uint32_t>(ConfigPath{"ui.font_scale"}, ui.font_scale);
}

void GameConfig::save_sound_config() {
  config_manager_.set<std::string>(ConfigPath{"sound.mode"}, to_string(sound.mode));
  config_manager_.set<uint32_t>(ConfigPath{"sound.master_volume"}, sound.master_volume);
  config_manager_.set<uint32_t>(ConfigPath{"sound.effects_volume"}, sound.effects_volume);
  config_manager_.set<uint32_t>(ConfigPath{"sound.ambient_volume"}, sound.ambient_volume);
  config_manager_.set<bool>(ConfigPath{"sound.mute_when_unfocused"}, sound.mute_when_unfocused);
}

void GameConfig::save_ai_config() {
  config_manager_.set<std::string>(ConfigPath{"ai.default_difficulty"},
                                   to_string(ai.default_difficulty));
  config_manager_.set<bool>(ConfigPath{"ai.enable_learning"}, ai.enable_ai_learning);
  config_manager_.set<bool>(ConfigPath{"ai.show_thinking"}, ai.show_ai_thinking);
  config_manager_.set<int64_t>(ConfigPath{"ai.delay_min_ms"}, ai.ai_delay_min.count());
  config_manager_.set<int64_t>(ConfigPath{"ai.delay_max_ms"}, ai.ai_delay_max.count());
  config_manager_.set<double>(ConfigPath{"ai.bluff_frequency"}, ai.bluff_frequency);
  config_manager_.set<double>(ConfigPath{"ai.conservative_factor"}, ai.conservative_factor);
}

void GameConfig::save_network_config() {
  config_manager_.set<uint32_t>(ConfigPath{"network.default_port"}, network.default_port);
  config_manager_.set<int64_t>(ConfigPath{"network.connection_timeout"},
                               network.connection_timeout.count());
  config_manager_.set<uint32_t>(ConfigPath{"network.max_reconnect_attempts"},
                                network.max_reconnect_attempts);
  config_manager_.set<bool>(ConfigPath{"network.enable_lan_discovery"},
                            network.enable_lan_discovery);
  config_manager_.set<std::string>(ConfigPath{"network.server_region"}, network.server_region);
}

// Enum to string conversions
std::string to_string(Difficulty difficulty) {
  switch (difficulty) {
  case Difficulty::Beginner:
    return "beginner";
  case Difficulty::Easy:
    return "easy";
  case Difficulty::Normal:
    return "normal";
  case Difficulty::Hard:
    return "hard";
  case Difficulty::Expert:
    return "expert";
  default:
    return "normal";
  }
}

std::string to_string(DiceFaces faces) { return std::to_string(static_cast<uint8_t>(faces)); }

std::string to_string(GameVariant variant) {
  switch (variant) {
  case GameVariant::Classic:
    return "classic";
  case GameVariant::Perudo:
    return "perudo";
  case GameVariant::Dudo:
    return "dudo";
  case GameVariant::Challenge:
    return "challenge";
  default:
    return "classic";
  }
}

std::string to_string(TimeoutAction action) {
  switch (action) {
  case TimeoutAction::Forfeit:
    return "forfeit";
  case TimeoutAction::AutoCall:
    return "auto_call";
  case TimeoutAction::RandomBid:
    return "random_bid";
  case TimeoutAction::Extend:
    return "extend";
  default:
    return "extend";
  }
}

std::string to_string(UITheme theme) {
  switch (theme) {
  case UITheme::Auto:
    return "auto";
  case UITheme::Light:
    return "light";
  case UITheme::Dark:
    return "dark";
  case UITheme::HighContrast:
    return "high_contrast";
  default:
    return "auto";
  }
}

std::string to_string(SoundMode mode) {
  switch (mode) {
  case SoundMode::Off:
    return "off";
  case SoundMode::Essential:
    return "essential";
  case SoundMode::Full:
    return "full";
  default:
    return "full";
  }
}

// Enum parsing functions
std::optional<Difficulty> parse_difficulty(const std::string &str) {
  if (str == "beginner")
    return Difficulty::Beginner;
  if (str == "easy")
    return Difficulty::Easy;
  if (str == "normal")
    return Difficulty::Normal;
  if (str == "hard")
    return Difficulty::Hard;
  if (str == "expert")
    return Difficulty::Expert;
  return std::nullopt;
}

std::optional<DiceFaces> parse_dice_faces(const std::string &str) {
  if (str == "4")
    return DiceFaces::Four;
  if (str == "6")
    return DiceFaces::Six;
  if (str == "8")
    return DiceFaces::Eight;
  if (str == "10")
    return DiceFaces::Ten;
  if (str == "12")
    return DiceFaces::Twelve;
  if (str == "20")
    return DiceFaces::Twenty;
  return std::nullopt;
}

std::optional<GameVariant> parse_game_variant(const std::string &str) {
  if (str == "classic")
    return GameVariant::Classic;
  if (str == "perudo")
    return GameVariant::Perudo;
  if (str == "dudo")
    return GameVariant::Dudo;
  if (str == "challenge")
    return GameVariant::Challenge;
  return std::nullopt;
}

std::optional<TimeoutAction> parse_timeout_action(const std::string &str) {
  if (str == "forfeit")
    return TimeoutAction::Forfeit;
  if (str == "auto_call")
    return TimeoutAction::AutoCall;
  if (str == "random_bid")
    return TimeoutAction::RandomBid;
  if (str == "extend")
    return TimeoutAction::Extend;
  return std::nullopt;
}

std::optional<UITheme> parse_ui_theme(const std::string &str) {
  if (str == "auto")
    return UITheme::Auto;
  if (str == "light")
    return UITheme::Light;
  if (str == "dark")
    return UITheme::Dark;
  if (str == "high_contrast")
    return UITheme::HighContrast;
  return std::nullopt;
}

std::optional<SoundMode> parse_sound_mode(const std::string &str) {
  if (str == "off")
    return SoundMode::Off;
  if (str == "essential")
    return SoundMode::Essential;
  if (str == "full")
    return SoundMode::Full;
  return std::nullopt;
}

} // namespace liarsdice::config