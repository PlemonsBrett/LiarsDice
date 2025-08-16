#pragma once

#include <algorithm>
#include <boost/circular_buffer.hpp>
#include <cmath>
#include <numeric>
#include <utility>

namespace liarsdice::data_structures {

  /**
   * @brief High-performance circular buffer with perfect forwarding
   *
   * Wraps boost::circular_buffer with additional functionality for
   * game state analysis and pattern recognition.
   *
   * @tparam T Element type
   * @tparam Allocator Allocator type (default: std::allocator<T>)
   */
  template <typename T, typename Allocator = std::allocator<T>> class CircularBuffer {
  public:
    using value_type = T;
    using allocator_type = Allocator;
    using buffer_type = boost::circular_buffer<T, Allocator>;
    using size_type = typename buffer_type::size_type;
    using iterator = typename buffer_type::iterator;
    using const_iterator = typename buffer_type::const_iterator;

    /**
     * @brief Construct circular buffer with specified capacity
     * @param capacity Maximum number of elements
     * @param alloc Allocator instance
     */
    explicit CircularBuffer(size_type capacity, const Allocator& alloc = Allocator())
        : buffer_(capacity, alloc) {}

    /**
     * @brief Push element with perfect forwarding
     * @tparam Args Argument types for T's constructor
     * @param args Arguments to forward to T's constructor
     */
    template <typename... Args> void emplace_back(Args&&... args) {
      buffer_.push_back(T(std::forward<Args>(args)...));
    }

    /**
     * @brief Push element with perfect forwarding at front
     * @tparam Args Argument types for T's constructor
     * @param args Arguments to forward to T's constructor
     */
    template <typename... Args> void emplace_front(Args&&... args) {
      buffer_.push_front(T(std::forward<Args>(args)...));
    }

    /**
     * @brief Access element at index with bounds checking
     * @param index Element index
     * @return Reference to element
     * @throw std::out_of_range if index >= size()
     */
    T& at(size_type index) {
      if (index >= buffer_.size()) {
        throw std::out_of_range("CircularBuffer: index out of range");
      }
      return buffer_[index];
    }

    [[nodiscard]] const T& at(size_type index) const {
      if (index >= buffer_.size()) {
        throw std::out_of_range("CircularBuffer: index out of range");
      }
      return buffer_[index];
    }

    /**
     * @brief Get sliding window of elements
     * @param window_size Size of the window
     * @return Vector containing the last window_size elements
     */
    [[nodiscard]] std::vector<T> get_window(size_type window_size) const {
      window_size = std::min(window_size, buffer_.size());
      return std::vector<T>(buffer_.end() - window_size, buffer_.end());
    }

    /**
     * @brief Apply function to sliding windows
     * @tparam Func Function type
     * @param window_size Size of each window
     * @param func Function to apply to each window
     */
    template <typename Func> void for_each_window(size_type window_size, Func func) const {
      if (buffer_.size() < window_size) return;

      for (size_type i = 0; i <= buffer_.size() - window_size; ++i) {
        func(buffer_.begin() + i, buffer_.begin() + i + window_size);
      }
    }

    /**
     * @brief Calculate statistics over the buffer
     * @tparam Extractor Function to extract numeric value from T
     * @param extractor Function to extract value for statistics
     * @return Tuple of (mean, std_dev, min, max)
     */
    template <typename Extractor> [[nodiscard]] auto calculate_statistics(Extractor extractor) const
        -> std::tuple<double, double, double, double> {
      if (buffer_.empty()) {
        return {0.0, 0.0, 0.0, 0.0};
      }

      std::vector<double> values;
      values.reserve(buffer_.size());

      for (const auto& elem : buffer_) {
        values.push_back(static_cast<double>(extractor(elem)));
      }

      double sum = std::accumulate(values.begin(), values.end(), 0.0);
      double mean = sum / values.size();

      double sq_sum = std::accumulate(
          values.begin(), values.end(), 0.0,
          [mean](double acc, double val) { return acc + (val - mean) * (val - mean); });
      double std_dev = std::sqrt(sq_sum / values.size());

      auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());

      return {mean, std_dev, *min_it, *max_it};
    }

    /**
     * @brief Find patterns in the buffer
     * @tparam Predicate Predicate type
     * @param pattern_size Size of pattern to search for
     * @param predicate Function to check if elements form a pattern
     * @return Vector of starting indices where patterns are found
     */
    template <typename Predicate>
    [[nodiscard]] std::vector<size_type> find_patterns(size_type pattern_size,
                                                       Predicate predicate) const {
      std::vector<size_type> indices;

      if (buffer_.size() < pattern_size) return indices;

      for (size_type i = 0; i <= buffer_.size() - pattern_size; ++i) {
        if (predicate(buffer_.begin() + i, buffer_.begin() + i + pattern_size)) {
          indices.push_back(i);
        }
      }

      return indices;
    }

    // Standard container interface
    void push_back(const T& value) { buffer_.push_back(value); }
    void push_back(T&& value) { buffer_.push_back(std::move(value)); }
    void push_front(const T& value) { buffer_.push_front(value); }
    void push_front(T&& value) { buffer_.push_front(std::move(value)); }

    void pop_back() { buffer_.pop_back(); }
    void pop_front() { buffer_.pop_front(); }
    void clear() { buffer_.clear(); }

    [[nodiscard]] bool empty() const { return buffer_.empty(); }
    [[nodiscard]] size_type size() const { return buffer_.size(); }
    [[nodiscard]] size_type capacity() const { return buffer_.capacity(); }
    [[nodiscard]] bool full() const { return buffer_.full(); }

    [[nodiscard]] T& front() { return buffer_.front(); }
    [[nodiscard]] const T& front() const { return buffer_.front(); }
    [[nodiscard]] T& back() { return buffer_.back(); }
    [[nodiscard]] const T& back() const { return buffer_.back(); }

    [[nodiscard]] iterator begin() { return buffer_.begin(); }
    [[nodiscard]] const_iterator begin() const { return buffer_.begin(); }
    [[nodiscard]] iterator end() { return buffer_.end(); }
    [[nodiscard]] const_iterator end() const { return buffer_.end(); }

    // Serialization support (requires custom implementation)
    // template<class Archive>
    // void serialize(Archive& ar, const unsigned int version) {
    //     ar & buffer_;
    // }

  private:
    buffer_type buffer_;
  };

}  // namespace liarsdice::data_structures