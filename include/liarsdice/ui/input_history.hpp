#pragma once

/**
 * @file input_history.hpp
 * @brief Input history management with std::deque and iterator concepts
 */

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <liarsdice/utils/format_helper.hpp>
#include <optional>
#include <ranges>
#include <string>

namespace liarsdice::ui {

/**
 * @brief Input history entry
 */
struct HistoryEntry {
  std::string input;
  std::chrono::system_clock::time_point timestamp;
  std::optional<std::string> context; // e.g., "player1_turn", "bid_phase"

  [[nodiscard]] std::string to_string() const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::string time_str(30, '\0');
    std::strftime(time_str.data(), time_str.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
    time_str.resize(std::strlen(time_str.c_str()));

    if (context) {
      return "[" + time_str + "] [" + *context + "] " + input;
    }
    return "[" + time_str + "] " + input;
  }
};

/**
 * @brief Input history manager
 */
class InputHistory {
public:
  using container_type = std::deque<HistoryEntry>;
  using iterator = container_type::iterator;
  using const_iterator = container_type::const_iterator;
  using reverse_iterator = container_type::reverse_iterator;
  using const_reverse_iterator = container_type::const_reverse_iterator;

  /**
   * @brief Constructor with optional max size
   */
  explicit InputHistory(std::size_t max_size = 1000) : max_size_(max_size), current_position_(0) {}

  /**
   * @brief Add entry to history
   */
  void add(std::string input, std::optional<std::string> context = std::nullopt) {
    if (input.empty())
      return;

    // Don't add duplicates of the last entry
    if (!history_.empty() && history_.back().input == input) {
      return;
    }

    history_.emplace_back(HistoryEntry{.input = std::move(input),
                                       .timestamp = std::chrono::system_clock::now(),
                                       .context = std::move(context)});

    // Maintain max size
    if (history_.size() > max_size_) {
      history_.pop_front();
    }

    // Reset position to end
    current_position_ = history_.size();
  }

  /**
   * @brief Navigate to previous entry
   */
  [[nodiscard]] std::optional<std::string> previous() {
    if (history_.empty() || current_position_ == 0) {
      return std::nullopt;
    }

    --current_position_;
    return history_[current_position_].input;
  }

  /**
   * @brief Navigate to next entry
   */
  [[nodiscard]] std::optional<std::string> next() {
    if (history_.empty() || current_position_ >= history_.size() - 1) {
      current_position_ = history_.size();
      return std::nullopt; // Return to current input
    }

    ++current_position_;
    return history_[current_position_].input;
  }

  /**
   * @brief Search history
   */
  [[nodiscard]] std::vector<HistoryEntry> search(std::string_view pattern) const {
    std::vector<HistoryEntry> results;

    std::ranges::copy_if(history_, std::back_inserter(results),
                         [pattern](const HistoryEntry &entry) {
                           return entry.input.find(pattern) != std::string::npos;
                         });

    return results;
  }

  /**
   * @brief Search with context filter
   */
  [[nodiscard]] std::vector<HistoryEntry> search_by_context(std::string_view context) const {
    std::vector<HistoryEntry> results;

    std::ranges::copy_if(history_, std::back_inserter(results),
                         [context](const HistoryEntry &entry) {
                           return entry.context && *entry.context == context;
                         });

    return results;
  }

  /**
   * @brief Get entries within time range
   */
  [[nodiscard]] std::vector<HistoryEntry>
  get_range(std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end) const {

    std::vector<HistoryEntry> results;

    std::ranges::copy_if(history_, std::back_inserter(results),
                         [start, end](const HistoryEntry &entry) {
                           return entry.timestamp >= start && entry.timestamp <= end;
                         });

    return results;
  }

  /**
   * @brief Get recent entries
   */
  [[nodiscard]] std::vector<HistoryEntry> get_recent(std::size_t count) const {
    std::vector<HistoryEntry> results;

    auto start = history_.size() > count ? history_.end() - count : history_.begin();
    std::copy(start, history_.end(), std::back_inserter(results));

    return results;
  }

  /**
   * @brief Clear history
   */
  void clear() {
    history_.clear();
    current_position_ = 0;
  }

  /**
   * @brief Save history to file
   */
  bool save_to_file(const std::filesystem::path &path) const {
    try {
      std::ofstream file(path);
      if (!file)
        return false;

      for (const auto &entry : history_) {
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        file << time_t << '\t';

        if (entry.context) {
          file << *entry.context;
        }
        file << '\t' << entry.input << '\n';
      }

      return true;
    } catch (...) {
      return false;
    }
  }

  /**
   * @brief Load history from file
   */
  bool load_from_file(const std::filesystem::path &path) {
    try {
      std::ifstream file(path);
      if (!file)
        return false;

      clear();

      std::string line;
      while (std::getline(file, line)) {
        if (line.empty())
          continue;

        // Parse tab-separated values
        std::size_t pos1 = line.find('\t');
        if (pos1 == std::string::npos)
          continue;

        std::size_t pos2 = line.find('\t', pos1 + 1);
        if (pos2 == std::string::npos)
          continue;

        // Parse timestamp
        std::time_t time_t = std::stoll(line.substr(0, pos1));
        auto timestamp = std::chrono::system_clock::from_time_t(time_t);

        // Parse context (may be empty)
        std::string context_str = line.substr(pos1 + 1, pos2 - pos1 - 1);
        std::optional<std::string> context;
        if (!context_str.empty()) {
          context = context_str;
        }

        // Parse input
        std::string input = line.substr(pos2 + 1);

        history_.emplace_back(HistoryEntry{
            .input = std::move(input), .timestamp = timestamp, .context = std::move(context)});
      }

      current_position_ = history_.size();
      return true;
    } catch (...) {
      return false;
    }
  }

  // Iterator support
  [[nodiscard]] iterator begin() { return history_.begin(); }
  [[nodiscard]] iterator end() { return history_.end(); }
  [[nodiscard]] const_iterator begin() const { return history_.begin(); }
  [[nodiscard]] const_iterator end() const { return history_.end(); }
  [[nodiscard]] const_iterator cbegin() const { return history_.cbegin(); }
  [[nodiscard]] const_iterator cend() const { return history_.cend(); }

  [[nodiscard]] reverse_iterator rbegin() { return history_.rbegin(); }
  [[nodiscard]] reverse_iterator rend() { return history_.rend(); }
  [[nodiscard]] const_reverse_iterator rbegin() const { return history_.rbegin(); }
  [[nodiscard]] const_reverse_iterator rend() const { return history_.rend(); }
  [[nodiscard]] const_reverse_iterator crbegin() const { return history_.crbegin(); }
  [[nodiscard]] const_reverse_iterator crend() const { return history_.crend(); }

  // Size operations
  [[nodiscard]] std::size_t size() const { return history_.size(); }
  [[nodiscard]] bool empty() const { return history_.empty(); }
  [[nodiscard]] std::size_t max_size() const { return max_size_; }

private:
  container_type history_;
  std::size_t max_size_;
  std::size_t current_position_;
};

/**
 * @brief Command frequency analyzer
 */
class CommandFrequencyAnalyzer {
public:
  /**
   * @brief Analyze command frequency from history
   */
  [[nodiscard]] std::vector<std::pair<std::string, std::size_t>>
  analyze_frequency(const InputHistory &history) const {
    std::unordered_map<std::string, std::size_t> frequency;

    for (const auto &entry : history) {
      frequency[entry.input]++;
    }

    std::vector<std::pair<std::string, std::size_t>> results(frequency.begin(), frequency.end());

    // Sort by frequency (descending)
    std::ranges::sort(results, [](const auto &a, const auto &b) { return a.second > b.second; });

    return results;
  }

  /**
   * @brief Get most common commands
   */
  [[nodiscard]] std::vector<std::string> get_most_common(const InputHistory &history,
                                                         std::size_t count = 10) const {
    auto frequency = analyze_frequency(history);

    std::vector<std::string> results;
    results.reserve(std::min(count, frequency.size()));

    for (std::size_t i = 0; i < std::min(count, frequency.size()); ++i) {
      results.push_back(frequency[i].first);
    }

    return results;
  }

  /**
   * @brief Get command suggestions based on prefix
   */
  [[nodiscard]] std::vector<std::string>
  get_prefix_suggestions(const InputHistory &history, std::string_view prefix,
                         std::size_t max_suggestions = 5) const {
    std::unordered_map<std::string, std::size_t> frequency;

    // Count frequency of commands with prefix
    for (const auto &entry : history) {
      if (entry.input.starts_with(prefix)) {
        frequency[entry.input]++;
      }
    }

    // Convert to vector and sort by frequency
    std::vector<std::pair<std::string, std::size_t>> sorted(frequency.begin(), frequency.end());

    std::ranges::sort(sorted, [](const auto &a, const auto &b) { return a.second > b.second; });

    // Extract command strings
    std::vector<std::string> results;
    results.reserve(std::min(max_suggestions, sorted.size()));

    for (std::size_t i = 0; i < std::min(max_suggestions, sorted.size()); ++i) {
      results.push_back(sorted[i].first);
    }

    return results;
  }
};

} // namespace liarsdice::ui