#ifndef LIARSDICE_CORE_GAME_STATE_STORAGE_HPP
#define LIARSDICE_CORE_GAME_STATE_STORAGE_HPP

#include <array>
#include <boost/circular_buffer.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <cstdint>
#include <memory>
#include <span>

namespace liarsdice::core {

  // Forward declarations
  struct CompactGameState;
  class GameHistory;

  /**
   * @brief Cache-efficient game state storage using Boost flat containers
   *
   * Uses boost::container::flat_map and flat_set for better cache locality
   * compared to standard node-based containers.
   */
  class GameStateStorage {
  public:
    using PlayerId = std::uint8_t;
    using DiceCount = std::uint8_t;
    using FaceValue = std::uint8_t;
    using Points = std::uint8_t;

    // Cache-efficient containers
    using PlayerDataMap = boost::container::flat_map<PlayerId, CompactGameState>;
    using ActivePlayerSet = boost::container::flat_set<PlayerId>;

    GameStateStorage();
    ~GameStateStorage() = default;

    // Disable copy, enable move
    GameStateStorage(const GameStateStorage&) = delete;
    GameStateStorage& operator=(const GameStateStorage&) = delete;
    GameStateStorage(GameStateStorage&&) = default;
    GameStateStorage& operator=(GameStateStorage&&) = default;

    // State management
    void store_player_state(PlayerId id, const CompactGameState& state);
    [[nodiscard]] const CompactGameState* get_player_state(PlayerId id) const;
    [[nodiscard]] CompactGameState* get_player_state(PlayerId id);

    // Active player management
    void add_active_player(PlayerId id);
    void remove_active_player(PlayerId id);
    [[nodiscard]] bool is_player_active(PlayerId id) const;
    [[nodiscard]] const ActivePlayerSet& get_active_players() const { return active_players_; }

    // Bulk operations
    void clear();
    [[nodiscard]] std::size_t size() const { return player_states_.size(); }

    // Memory statistics
    [[nodiscard]] std::size_t memory_usage() const;

  private:
    PlayerDataMap player_states_;
    ActivePlayerSet active_players_;
  };

  /**
   * @brief Compact game state representation using bit manipulation
   *
   * Uses bitset and bit packing to minimize memory footprint while
   * maintaining fast access to game state data.
   */
  struct CompactGameState {
    static constexpr std::size_t MAX_DICE = 5;
    static constexpr std::size_t DICE_BITS = 3;  // 3 bits per die (values 1-6)
    static constexpr std::size_t TOTAL_DICE_BITS = MAX_DICE * DICE_BITS;

    // Bit-packed representation of dice values (15 bits total)
    std::uint16_t dice_bits{0};

    // Player state packed into single byte
    struct PlayerState {
      std::uint8_t points : 4;      // 0-15 points (we use 0-5)
      std::uint8_t dice_count : 3;  // 0-7 dice (we use 0-5)
      std::uint8_t is_active : 1;   // Active/eliminated flag
    } player_state{};

    // Last action information (packed)
    struct LastAction {
      std::uint8_t action_type : 2;  // 0=none, 1=guess, 2=call_liar
      std::uint8_t dice_count : 4;   // 0-15 (for guess)
      std::uint8_t face_value : 3;   // 0-7 (1-6 for guess)
      std::uint8_t reserved : 7;     // Future use
    } last_action{};

    // Helper methods
    void set_dice_value(std::size_t index, std::uint8_t value);
    [[nodiscard]] std::uint8_t get_dice_value(std::size_t index) const;
    [[nodiscard]] std::array<std::uint8_t, MAX_DICE> get_all_dice() const;
    void set_all_dice(std::span<const std::uint8_t> dice);

    // State helpers
    [[nodiscard]] bool is_eliminated() const { return player_state.points == 0; }
    [[nodiscard]] std::size_t total_size() const { return sizeof(*this); }

    // Serialization for network/storage
    [[nodiscard]] std::uint32_t serialize() const;
    static CompactGameState deserialize(std::uint32_t data);
  };

  /**
   * @brief Ring buffer-based game history with custom memory management
   *
   * Uses boost::circular_buffer for efficient history storage with
   * automatic old entry removal.
   */
  class GameHistory {
  public:
    static constexpr std::size_t DEFAULT_HISTORY_SIZE = 100;

    // Use pool allocator for efficient memory management
    using HistoryAllocator = boost::pool_allocator<CompactGameState>;
    using HistoryBuffer = boost::circular_buffer<CompactGameState, HistoryAllocator>;

    explicit GameHistory(std::size_t max_entries = DEFAULT_HISTORY_SIZE);
    ~GameHistory() = default;

    // History management
    void record_state(const CompactGameState& state);
    void record_state(CompactGameState&& state);

    // Access methods
    [[nodiscard]] const CompactGameState* get_state(std::size_t steps_back) const;
    [[nodiscard]] std::span<const CompactGameState> get_recent_states(std::size_t count) const;
    [[nodiscard]] std::size_t size() const { return history_.size(); }
    [[nodiscard]] bool empty() const { return history_.empty(); }

    // Analysis methods
    [[nodiscard]] std::vector<std::uint8_t> get_dice_frequency(std::size_t last_n_states) const;
    [[nodiscard]] double get_average_dice_count(std::size_t last_n_states) const;

    // Memory management
    void clear() { history_.clear(); }
    void resize(std::size_t new_size) { history_.resize(new_size); }
    [[nodiscard]] std::size_t capacity() const { return history_.capacity(); }

  private:
    HistoryBuffer history_;
  };

  /**
   * @brief Memory pool allocator for game states
   *
   * Provides efficient allocation/deallocation for frequently created
   * game state objects.
   */
  template <typename T> class GameStateAllocator {
  public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    // Pool configuration
    static constexpr size_type POOL_BLOCK_SIZE = 256;

    GameStateAllocator() = default;

    template <typename U> GameStateAllocator(const GameStateAllocator<U>&) noexcept {}

    [[nodiscard]] pointer allocate(size_type n);
    void deallocate(pointer p, size_type n) noexcept;

    template <typename U> struct rebind {
      using other = GameStateAllocator<U>;
    };

    bool operator==(const GameStateAllocator&) const noexcept { return true; }
    bool operator!=(const GameStateAllocator&) const noexcept { return false; }

  private:
    static boost::pool<> pool_;
  };

  // Static pool definition
  template <typename T> boost::pool<> GameStateAllocator<T>::pool_(sizeof(T));

  template <typename T>
  typename GameStateAllocator<T>::pointer GameStateAllocator<T>::allocate(size_type n) {
    if (n == 1) {
      return static_cast<pointer>(pool_.malloc());
    }
    return static_cast<pointer>(::operator new(n * sizeof(T)));
  }

  template <typename T> void GameStateAllocator<T>::deallocate(pointer p, size_type n) noexcept {
    if (n == 1) {
      pool_.free(p);
    } else {
      ::operator delete(p);
    }
  }

}  // namespace liarsdice::core

#endif  // LIARSDICE_CORE_GAME_STATE_STORAGE_HPP