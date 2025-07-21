#pragma once

/**
 * @file fuzzy_match.hpp
 * @brief Fuzzy matching algorithms for user input
 */

#include <algorithm>
#include <cmath>
#include <liarsdice/utils/format_helper.hpp>
#include <numeric>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace liarsdice::ui {

/**
 * @brief Fuzzy match result
 */
struct FuzzyMatchResult {
  std::string value;
  double score; // 0.0 to 1.0, where 1.0 is perfect match

  auto operator<=>(const FuzzyMatchResult &other) const {
    return score <=> other.score; // Higher scores first
  }
};

/**
 * @brief Fuzzy matching algorithms
 */
class FuzzyMatcher {
public:
  /**
   * @brief Calculate Levenshtein distance
   */
  [[nodiscard]] static std::size_t levenshtein_distance(std::string_view s1, std::string_view s2) {
    if (s1.empty())
      return s2.length();
    if (s2.empty())
      return s1.length();

    std::vector<std::size_t> prev_row(s2.length() + 1);
    std::vector<std::size_t> curr_row(s2.length() + 1);

    // Initialize first row
    std::iota(prev_row.begin(), prev_row.end(), 0);

    for (std::size_t i = 0; i < s1.length(); ++i) {
      curr_row[0] = i + 1;

      for (std::size_t j = 0; j < s2.length(); ++j) {
        std::size_t cost = (s1[i] == s2[j]) ? 0 : 1;

        curr_row[j + 1] = std::min({
            prev_row[j + 1] + 1, // deletion
            curr_row[j] + 1,     // insertion
            prev_row[j] + cost   // substitution
        });
      }

      std::swap(prev_row, curr_row);
    }

    return prev_row.back();
  }

  /**
   * @brief Calculate similarity score based on Levenshtein distance
   */
  [[nodiscard]] static double levenshtein_similarity(std::string_view s1, std::string_view s2) {
    if (s1.empty() && s2.empty())
      return 1.0;

    std::size_t distance = levenshtein_distance(s1, s2);
    std::size_t max_length = std::max(s1.length(), s2.length());

    return 1.0 - (static_cast<double>(distance) / static_cast<double>(max_length));
  }

  /**
   * @brief Calculate Jaro similarity
   */
  [[nodiscard]] static double jaro_similarity(std::string_view s1, std::string_view s2) {
    if (s1.empty() && s2.empty())
      return 1.0;
    if (s1.empty() || s2.empty())
      return 0.0;
    if (s1 == s2)
      return 1.0;

    std::size_t match_window = std::max(s1.length(), s2.length()) / 2 - 1;
    if (match_window < 1)
      match_window = 1;

    std::vector<bool> s1_matches(s1.length(), false);
    std::vector<bool> s2_matches(s2.length(), false);

    std::size_t matches = 0;
    std::size_t transpositions = 0;

    // Find matches
    for (std::size_t i = 0; i < s1.length(); ++i) {
      std::size_t start = (i > match_window) ? i - match_window : 0;
      std::size_t end = std::min(i + match_window + 1, s2.length());

      for (std::size_t j = start; j < end; ++j) {
        if (s2_matches[j] || s1[i] != s2[j])
          continue;

        s1_matches[i] = true;
        s2_matches[j] = true;
        matches++;
        break;
      }
    }

    if (matches == 0)
      return 0.0;

    // Count transpositions
    std::size_t k = 0;
    for (std::size_t i = 0; i < s1.length(); ++i) {
      if (!s1_matches[i])
        continue;
      while (!s2_matches[k])
        k++;
      if (s1[i] != s2[k])
        transpositions++;
      k++;
    }

    double m = static_cast<double>(matches);
    return (m / s1.length() + m / s2.length() + (m - transpositions / 2.0) / m) / 3.0;
  }

  /**
   * @brief Calculate Jaro-Winkler similarity
   */
  [[nodiscard]] static double jaro_winkler_similarity(std::string_view s1, std::string_view s2,
                                                      double scaling_factor = 0.1) {
    double jaro_score = jaro_similarity(s1, s2);

    if (jaro_score < 0.7)
      return jaro_score; // Threshold for applying prefix bonus

    // Calculate common prefix length (up to 4 characters)
    std::size_t prefix_length = 0;
    for (std::size_t i = 0; i < std::min({s1.length(), s2.length(), std::size_t(4)}); ++i) {
      if (s1[i] == s2[i]) {
        prefix_length++;
      } else {
        break;
      }
    }

    return jaro_score + (prefix_length * scaling_factor * (1.0 - jaro_score));
  }

  /**
   * @brief Substring matching score
   */
  [[nodiscard]] static double substring_score(std::string_view needle, std::string_view haystack) {
    if (needle.empty())
      return 1.0;
    if (haystack.empty())
      return 0.0;

    // Convert to lowercase for case-insensitive matching
    std::string needle_lower(needle);
    std::string haystack_lower(haystack);

    std::ranges::transform(needle_lower, needle_lower.begin(),
                           [](char c) { return std::tolower(static_cast<unsigned char>(c)); });
    std::ranges::transform(haystack_lower, haystack_lower.begin(),
                           [](char c) { return std::tolower(static_cast<unsigned char>(c)); });

    // Check if needle is a substring
    if (haystack_lower.find(needle_lower) != std::string::npos) {
      // Score based on position and length ratio
      auto pos = haystack_lower.find(needle_lower);
      double position_score = 1.0 - (static_cast<double>(pos) / haystack_lower.length());
      double length_ratio = static_cast<double>(needle_lower.length()) / haystack_lower.length();

      return (position_score + length_ratio) / 2.0;
    }

    // Check for partial matches
    std::size_t matched_chars = 0;
    std::size_t j = 0;

    for (char c : needle_lower) {
      bool found = false;
      for (; j < haystack_lower.length(); ++j) {
        if (haystack_lower[j] == c) {
          matched_chars++;
          j++;
          found = true;
          break;
        }
      }
      if (!found)
        break;
    }

    return static_cast<double>(matched_chars) / needle_lower.length() *
           0.5; // Partial match penalty
  }

  /**
   * @brief Combined fuzzy matching score
   */
  [[nodiscard]] static double fuzzy_score(std::string_view input, std::string_view candidate,
                                          double levenshtein_weight = 0.3,
                                          double jaro_winkler_weight = 0.4,
                                          double substring_weight = 0.3) {
    double lev_score = levenshtein_similarity(input, candidate);
    double jw_score = jaro_winkler_similarity(input, candidate);
    double sub_score = substring_score(input, candidate);

    return lev_score * levenshtein_weight + jw_score * jaro_winkler_weight +
           sub_score * substring_weight;
  }

  /**
   * @brief Find best matches from candidates
   */
  [[nodiscard]] static std::vector<FuzzyMatchResult>
  find_best_matches(std::string_view input, std::span<const std::string> candidates,
                    double min_score = 0.6, std::size_t max_results = 5) {

    std::vector<FuzzyMatchResult> results;
    results.reserve(candidates.size());

    for (const auto &candidate : candidates) {
      double score = fuzzy_score(input, candidate);
      if (score >= min_score) {
        results.emplace_back(FuzzyMatchResult{candidate, score});
      }
    }

    // Sort by score (descending)
    std::ranges::sort(results, std::greater{});

    // Limit results
    if (results.size() > max_results) {
      results.resize(max_results);
    }

    return results;
  }

  /**
   * @brief Check if input is close to any candidate
   */
  [[nodiscard]] static std::optional<std::string>
  find_closest_match(std::string_view input, std::span<const std::string> candidates,
                     double min_score = 0.7) {

    auto matches = find_best_matches(input, candidates, min_score, 1);

    if (!matches.empty()) {
      return matches.front().value;
    }

    return std::nullopt;
  }
};

/**
 * @brief Command suggestion system
 */
class CommandSuggester {
public:
  explicit CommandSuggester(std::vector<std::string> commands) : commands_(std::move(commands)) {}

  /**
   * @brief Add a command
   */
  void add_command(std::string command) { commands_.push_back(std::move(command)); }

  /**
   * @brief Get suggestions for input
   */
  [[nodiscard]] std::vector<FuzzyMatchResult>
  get_suggestions(std::string_view input, std::size_t max_suggestions = 3) const {

    return FuzzyMatcher::find_best_matches(input, commands_,
                                           0.5, // Lower threshold for suggestions
                                           max_suggestions);
  }

  /**
   * @brief Format suggestions as string
   */
  [[nodiscard]] std::string format_suggestions(std::string_view input,
                                               std::size_t max_suggestions = 3) const {

    auto suggestions = get_suggestions(input, max_suggestions);

    if (suggestions.empty()) {
      return "";
    }

    if (suggestions.size() == 1) {
      return "Did you mean: " + suggestions[0].value + "?";
    }

    std::string result = "Did you mean one of: ";
    for (std::size_t i = 0; i < suggestions.size(); ++i) {
      if (i > 0)
        result += ", ";
      result += suggestions[i].value;
    }
    result += "?";

    return result;
  }

private:
  std::vector<std::string> commands_;
};

/**
 * @brief Predefined game command suggester
 */
[[nodiscard]] inline CommandSuggester create_game_command_suggester() {
  return CommandSuggester(
      {"liar", "call", "challenge", "help", "history", "quit", "exit", "yes", "no", "y", "n"});
}

} // namespace liarsdice::ui