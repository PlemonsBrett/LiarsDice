#pragma once

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/optional.hpp>
#include <chrono>
#include <functional>

namespace liarsdice::data_structures {

  namespace mi = boost::multi_index;

  /**
   * @brief High-performance LRU cache using boost::multi_index
   *
   * Provides O(1) access and automatic eviction of least recently used items.
   * Uses both sequenced and hashed indices for efficient operations.
   *
   * @tparam Key Key type
   * @tparam Value Value type
   * @tparam Hash Hash function for Key (default: std::hash<Key>)
   * @tparam KeyEqual Equality comparator for Key (default: std::equal_to<Key>)
   */
  template <typename Key, typename Value, typename Hash = std::hash<Key>,
            typename KeyEqual = std::equal_to<Key>>
  class LRUCache {
  private:
    struct CacheEntry {
      Key key;
      Value value;
      std::chrono::steady_clock::time_point timestamp;
      mutable size_t access_count;

      CacheEntry(const Key& k, const Value& v)
          : key(k), value(v), timestamp(std::chrono::steady_clock::now()), access_count(0) {}
    };

    // Tags for multi_index
    struct by_sequence {};
    struct by_key {};

    using cache_container = mi::multi_index_container<
        CacheEntry,
        mi::indexed_by<
            // Sequenced index for LRU ordering
            mi::sequenced<mi::tag<by_sequence>>,
            // Hashed index for O(1) lookup
            mi::hashed_unique<mi::tag<by_key>, mi::member<CacheEntry, Key, &CacheEntry::key>, Hash,
                              KeyEqual>>>;

  public:
    using size_type = typename cache_container::size_type;

    /**
     * @brief Construct LRU cache with maximum capacity
     * @param max_size Maximum number of entries
     */
    explicit LRUCache(size_type max_size)
        : max_size_(max_size), hits_(0), misses_(0), evictions_(0) {
      if (max_size == 0) {
        throw std::invalid_argument("LRUCache: max_size must be > 0");
      }
    }

    /**
     * @brief Insert or update a key-value pair
     * @param key The key
     * @param value The value
     * @return True if a new entry was created, false if updated
     */
    bool put(const Key& key, const Value& value) {
      auto& key_index = cache_.template get<by_key>();
      auto it = key_index.find(key);

      if (it != key_index.end()) {
        // Update existing entry and move to front
        key_index.modify(it, [&value](CacheEntry& entry) {
          entry.value = value;
          entry.timestamp = std::chrono::steady_clock::now();
          entry.access_count++;
        });

        // Move to front (most recently used)
        auto& seq_index = cache_.template get<by_sequence>();
        seq_index.relocate(seq_index.begin(), cache_.template project<by_sequence>(it));

        return false;
      }

      // Insert new entry
      if (cache_.size() >= max_size_) {
        // Evict least recently used (back of sequence)
        auto& seq_index = cache_.template get<by_sequence>();
        seq_index.pop_back();
        evictions_++;
      }

      cache_.push_front(CacheEntry(key, value));
      return true;
    }

    /**
     * @brief Get value by key
     * @param key The key to lookup
     * @return Optional containing the value if found
     */
    [[nodiscard]] boost::optional<Value> get(const Key& key) {
      auto& key_index = cache_.template get<by_key>();
      auto it = key_index.find(key);

      if (it == key_index.end()) {
        misses_++;
        return boost::none;
      }

      hits_++;

      // Update access info and move to front
      key_index.modify(it, [](CacheEntry& entry) {
        entry.timestamp = std::chrono::steady_clock::now();
        entry.access_count++;
      });

      // Move to front (most recently used)
      auto& seq_index = cache_.template get<by_sequence>();
      seq_index.relocate(seq_index.begin(), cache_.template project<by_sequence>(it));

      return it->value;
    }

    /**
     * @brief Check if key exists without updating LRU order
     * @param key The key to check
     * @return True if key exists
     */
    [[nodiscard]] bool contains(const Key& key) const {
      const auto& key_index = cache_.template get<by_key>();
      return key_index.find(key) != key_index.end();
    }

    /**
     * @brief Remove entry by key
     * @param key The key to remove
     * @return True if entry was removed
     */
    bool erase(const Key& key) {
      auto& key_index = cache_.template get<by_key>();
      auto it = key_index.find(key);

      if (it == key_index.end()) {
        return false;
      }

      key_index.erase(it);
      return true;
    }

    /**
     * @brief Clear all entries
     */
    void clear() {
      cache_.clear();
      hits_ = 0;
      misses_ = 0;
      evictions_ = 0;
    }

    /**
     * @brief Get current number of entries
     * @return Number of cached entries
     */
    [[nodiscard]] size_type size() const { return cache_.size(); }

    /**
     * @brief Check if cache is empty
     * @return True if no entries cached
     */
    [[nodiscard]] bool empty() const { return cache_.empty(); }

    /**
     * @brief Check if cache is full
     * @return True if at maximum capacity
     */
    [[nodiscard]] bool full() const { return cache_.size() >= max_size_; }

    /**
     * @brief Get maximum capacity
     * @return Maximum number of entries
     */
    [[nodiscard]] size_type capacity() const { return max_size_; }

    /**
     * @brief Get cache statistics
     * @return Tuple of (hits, misses, evictions, hit_rate)
     */
    [[nodiscard]] std::tuple<size_t, size_t, size_t, double> get_stats() const {
      size_t total = hits_ + misses_;
      double hit_rate = total > 0 ? static_cast<double>(hits_) / total : 0.0;
      return {hits_, misses_, evictions_, hit_rate};
    }

    /**
     * @brief Get all cached keys in LRU order
     * @return Vector of keys (most recently used first)
     */
    [[nodiscard]] std::vector<Key> get_keys() const {
      std::vector<Key> keys;
      keys.reserve(cache_.size());

      const auto& seq_index = cache_.template get<by_sequence>();
      for (const auto& entry : seq_index) {
        keys.push_back(entry.key);
      }

      return keys;
    }

    /**
     * @brief Apply function to all entries in LRU order
     * @tparam Func Function type
     * @param func Function to apply (key, value, access_count)
     */
    template <typename Func> void for_each(Func func) const {
      const auto& seq_index = cache_.template get<by_sequence>();
      for (const auto& entry : seq_index) {
        func(entry.key, entry.value, entry.access_count);
      }
    }

    /**
     * @brief Resize cache capacity
     * @param new_size New maximum capacity
     *
     * If new_size < current size, least recently used entries are evicted
     */
    void resize(size_type new_size) {
      if (new_size == 0) {
        throw std::invalid_argument("LRUCache: new_size must be > 0");
      }

      max_size_ = new_size;

      // Evict entries if necessary
      while (cache_.size() > max_size_) {
        auto& seq_index = cache_.template get<by_sequence>();
        seq_index.pop_back();
        evictions_++;
      }
    }

  private:
    cache_container cache_;
    size_type max_size_;
    mutable size_t hits_;
    mutable size_t misses_;
    mutable size_t evictions_;
  };

  /**
   * @brief Specialized LRU cache for game state lookups
   *
   * Caches frequently accessed game states for AI analysis
   */
  template <typename StateKey, typename GameState> using GameStateCache
      = LRUCache<StateKey, GameState>;

  /**
   * @brief Specialized LRU cache for pattern matching results
   *
   * Caches expensive pattern matching computations
   */
  template <typename PatternKey> using PatternCache = LRUCache<PatternKey, std::vector<double>>;

}  // namespace liarsdice::data_structures