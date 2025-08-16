#include <algorithm>
#include <boost/log/trivial.hpp>
#include <cstring>
#include <liarsdice/core/game_state_storage.hpp>
#include <numeric>

namespace liarsdice::core {

  // GameStateStorage implementation
  GameStateStorage::GameStateStorage() {
    // Reserve space for typical game size
    player_states_.reserve(8);
    active_players_.reserve(8);
    BOOST_LOG_TRIVIAL(debug) << "GameStateStorage initialized with reserved capacity for 8 players";
  }

  void GameStateStorage::store_player_state(PlayerId id, const CompactGameState& state) {
    player_states_[id] = state;
    BOOST_LOG_TRIVIAL(trace) << "Stored state for player " << static_cast<int>(id);
  }

  const CompactGameState* GameStateStorage::get_player_state(PlayerId id) const {
    auto it = player_states_.find(id);
    return (it != player_states_.end()) ? &it->second : nullptr;
  }

  CompactGameState* GameStateStorage::get_player_state(PlayerId id) {
    auto it = player_states_.find(id);
    return (it != player_states_.end()) ? &it->second : nullptr;
  }

  void GameStateStorage::add_active_player(PlayerId id) {
    active_players_.insert(id);
    BOOST_LOG_TRIVIAL(trace) << "Player " << static_cast<int>(id) << " marked as active";
  }

  void GameStateStorage::remove_active_player(PlayerId id) {
    active_players_.erase(id);
    BOOST_LOG_TRIVIAL(trace) << "Player " << static_cast<int>(id) << " marked as inactive";
  }

  bool GameStateStorage::is_player_active(PlayerId id) const {
    return active_players_.find(id) != active_players_.end();
  }

  void GameStateStorage::clear() {
    player_states_.clear();
    active_players_.clear();
    BOOST_LOG_TRIVIAL(debug) << "GameStateStorage cleared";
  }

  std::size_t GameStateStorage::memory_usage() const {
    // Calculate approximate memory usage
    std::size_t total = sizeof(*this);
    total += player_states_.capacity() * sizeof(PlayerDataMap::value_type);
    total += active_players_.capacity() * sizeof(PlayerId);
    return total;
  }

  // CompactGameState implementation
  void CompactGameState::set_dice_value(std::size_t index, std::uint8_t value) {
    if (index >= MAX_DICE || value < 1 || value > 6) {
      BOOST_LOG_TRIVIAL(warning) << "Invalid dice index or value: " << index << ", "
                                 << static_cast<int>(value);
      return;
    }

    // Clear the bits for this die
    std::uint16_t mask = ~(0x7 << (index * DICE_BITS));
    dice_bits &= mask;

    // Set the new value (subtract 1 to store 0-5 instead of 1-6)
    dice_bits |= ((value - 1) & 0x7) << (index * DICE_BITS);
  }

  std::uint8_t CompactGameState::get_dice_value(std::size_t index) const {
    if (index >= MAX_DICE) {
      BOOST_LOG_TRIVIAL(warning) << "Invalid dice index: " << index;
      return 0;
    }

    // Extract the 3 bits for this die and add 1 (to convert 0-5 to 1-6)
    std::uint8_t value = (dice_bits >> (index * DICE_BITS)) & 0x7;
    return value + 1;
  }

  std::array<std::uint8_t, CompactGameState::MAX_DICE> CompactGameState::get_all_dice() const {
    std::array<std::uint8_t, MAX_DICE> dice{};
    for (std::size_t i = 0; i < MAX_DICE; ++i) {
      dice[i] = get_dice_value(i);
    }
    return dice;
  }

  void CompactGameState::set_all_dice(std::span<const std::uint8_t> dice) {
    dice_bits = 0;
    std::size_t count = std::min(dice.size(), MAX_DICE);
    for (std::size_t i = 0; i < count; ++i) {
      set_dice_value(i, dice[i]);
    }
    player_state.dice_count = static_cast<std::uint8_t>(count);
  }

  std::uint32_t CompactGameState::serialize() const {
    // Pack all state into 32 bits
    // Bits 0-15: dice_bits
    // Bits 16-23: player_state (as byte)
    // Bits 24-31: last_action (lower byte)
    std::uint32_t result = dice_bits;
    result |= static_cast<std::uint32_t>(*reinterpret_cast<const std::uint8_t*>(&player_state))
              << 16;
    result |= static_cast<std::uint32_t>(*reinterpret_cast<const std::uint8_t*>(&last_action))
              << 24;
    return result;
  }

  CompactGameState CompactGameState::deserialize(std::uint32_t data) {
    CompactGameState state;
    state.dice_bits = data & 0xFFFF;

    std::uint8_t player_byte = (data >> 16) & 0xFF;
    std::memcpy(&state.player_state, &player_byte, sizeof(player_byte));

    std::uint8_t action_byte = (data >> 24) & 0xFF;
    std::memcpy(&state.last_action, &action_byte, sizeof(action_byte));

    return state;
  }

  // GameHistory implementation
  GameHistory::GameHistory(std::size_t max_entries) : history_(max_entries) {
    BOOST_LOG_TRIVIAL(debug) << "GameHistory initialized with capacity: " << max_entries;
  }

  void GameHistory::record_state(const CompactGameState& state) {
    history_.push_back(state);
    BOOST_LOG_TRIVIAL(trace) << "Recorded game state, history size: " << history_.size();
  }

  void GameHistory::record_state(CompactGameState&& state) {
    history_.push_back(std::move(state));
    BOOST_LOG_TRIVIAL(trace) << "Recorded game state (move), history size: " << history_.size();
  }

  const CompactGameState* GameHistory::get_state(std::size_t steps_back) const {
    if (steps_back >= history_.size()) {
      return nullptr;
    }

    auto it = history_.end();
    std::advance(it, -static_cast<std::ptrdiff_t>(steps_back + 1));
    return &(*it);
  }

  std::span<const CompactGameState> GameHistory::get_recent_states(std::size_t count) const {
    if (history_.empty()) {
      return {};
    }

    count = std::min(count, history_.size());
    auto start = history_.end() - count;

    // Note: In C++20, we need to create a contiguous view
    // boost::circular_buffer may not guarantee contiguous storage
    static thread_local std::vector<CompactGameState> temp_buffer;
    temp_buffer.clear();
    temp_buffer.reserve(count);

    std::copy(start, history_.end(), std::back_inserter(temp_buffer));
    return std::span<const CompactGameState>(temp_buffer.data(), temp_buffer.size());
  }

  std::vector<std::uint8_t> GameHistory::get_dice_frequency(std::size_t last_n_states) const {
    std::vector<std::uint8_t> frequency(7, 0);  // Index 0 unused, 1-6 for dice values

    if (history_.empty()) {
      return frequency;
    }

    last_n_states = std::min(last_n_states, history_.size());
    auto start = history_.end() - last_n_states;

    for (auto it = start; it != history_.end(); ++it) {
      auto dice = it->get_all_dice();
      for (std::size_t i = 0; i < it->player_state.dice_count; ++i) {
        if (dice[i] >= 1 && dice[i] <= 6) {
          frequency[dice[i]]++;
        }
      }
    }

    return frequency;
  }

  double GameHistory::get_average_dice_count(std::size_t last_n_states) const {
    if (history_.empty()) {
      return 0.0;
    }

    last_n_states = std::min(last_n_states, history_.size());
    auto start = history_.end() - last_n_states;

    std::size_t total_dice = 0;
    std::size_t state_count = 0;

    for (auto it = start; it != history_.end(); ++it) {
      total_dice += it->player_state.dice_count;
      state_count++;
    }

    return state_count > 0 ? static_cast<double>(total_dice) / state_count : 0.0;
  }

}  // namespace liarsdice::core