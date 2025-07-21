/**
 * @file validation_benchmark.cpp
 * @brief Performance benchmarks for input validation and processing
 */

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "liarsdice/ui/fuzzy_match.hpp"
#include "liarsdice/ui/game_input.hpp"
#include "liarsdice/ui/input_history.hpp"
#include "liarsdice/validation/parser_combinators.hpp"
#include "liarsdice/validation/sanitizers.hpp"
#include "liarsdice/validation/validators.hpp"

#include <random>
#include <string>
#include <vector>

using namespace liarsdice::validation;
using namespace liarsdice::ui;

// Helper to generate test data
class BenchmarkData {
public:
  static std::vector<std::string> generate_strings(std::size_t count, std::size_t length) {
    std::vector<std::string> result;
    result.reserve(count);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> char_dist('a', 'z');

    for (std::size_t i = 0; i < count; ++i) {
      std::string str;
      str.reserve(length);
      for (std::size_t j = 0; j < length; ++j) {
        str.push_back(static_cast<char>(char_dist(gen)));
      }
      result.push_back(std::move(str));
    }

    return result;
  }

  static std::vector<int> generate_integers(std::size_t count, int min, int max) {
    std::vector<int> result;
    result.reserve(count);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);

    for (std::size_t i = 0; i < count; ++i) {
      result.push_back(dist(gen));
    }

    return result;
  }
};

TEST_CASE("Validator benchmarks", "[benchmark][validation]") {
  SECTION("Range validator performance") {
    auto validator = validators::range(0, 100);
    auto test_values = BenchmarkData::generate_integers(1000, -50, 150);

    BENCHMARK("Range validation") {
      int valid_count = 0;
      for (int value : test_values) {
        if (!validator(value).has_value()) {
          valid_count++;
        }
      }
      return valid_count;
    };
  }

  SECTION("String length validator performance") {
    auto validator = validators::length(5, 20);
    auto test_strings = BenchmarkData::generate_strings(1000, 15);

    BENCHMARK("String length validation") {
      int valid_count = 0;
      for (const auto &str : test_strings) {
        if (!validator(str).has_value()) {
          valid_count++;
        }
      }
      return valid_count;
    };
  }

  SECTION("Pattern validator performance") {
    auto email_validator = validators::pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");

    std::vector<std::string> test_emails;
    for (int i = 0; i < 100; ++i) {
      test_emails.push_back("user" + std::to_string(i) + "@example.com");
    }

    BENCHMARK("Email pattern validation") {
      int valid_count = 0;
      for (const auto &email : test_emails) {
        if (!email_validator(email).has_value()) {
          valid_count++;
        }
      }
      return valid_count;
    };
  }

  SECTION("Composite validator performance") {
    auto composite =
        validators::non_empty() && validators::length(1, 50) && validators::alphanumeric();

    auto test_strings = BenchmarkData::generate_strings(1000, 25);

    BENCHMARK("Composite validation (3 validators)") {
      int valid_count = 0;
      for (const auto &str : test_strings) {
        if (!composite(str).has_value()) {
          valid_count++;
        }
      }
      return valid_count;
    };
  }
}

TEST_CASE("Sanitizer benchmarks", "[benchmark][sanitizer]") {
  SECTION("Trim sanitizer performance") {
    auto trimmer = sanitizers::trim();
    std::vector<std::string> test_strings;

    for (int i = 0; i < 1000; ++i) {
      test_strings.push_back("   test string " + std::to_string(i) + "   ");
    }

    BENCHMARK("Trim whitespace") {
      std::size_t total_length = 0;
      for (const auto &str : test_strings) {
        auto result = trimmer(str);
        total_length += result.length();
      }
      return total_length;
    };
  }

  SECTION("HTML escape sanitizer performance") {
    auto escaper = sanitizers::escape_html();
    std::vector<std::string> test_strings;

    for (int i = 0; i < 100; ++i) {
      test_strings.push_back("<div>Test & \"quoted\" text</div>");
    }

    BENCHMARK("HTML escape") {
      std::size_t total_length = 0;
      for (const auto &str : test_strings) {
        auto result = escaper(str);
        total_length += result.length();
      }
      return total_length;
    };
  }

  SECTION("Chained sanitizer performance") {
    auto chain =
        sanitizers::trim().then(sanitizers::lowercase()).then(sanitizers::collapse_whitespace());

    std::vector<std::string> test_strings;
    for (int i = 0; i < 1000; ++i) {
      test_strings.push_back("  MIXED   Case   STRING  " + std::to_string(i) + "  ");
    }

    BENCHMARK("Chained sanitizers (3 operations)") {
      std::size_t total_length = 0;
      for (const auto &str : test_strings) {
        auto result = chain(str);
        total_length += result.length();
      }
      return total_length;
    };
  }
}

TEST_CASE("Parser combinator benchmarks", "[benchmark][parser]") {
  SECTION("Integer parser performance") {
    auto parser = parsers::integer();
    std::vector<std::string> test_inputs;

    for (int i = 0; i < 1000; ++i) {
      test_inputs.push_back(std::to_string(i * 1000 + i));
    }

    BENCHMARK("Integer parsing") {
      int sum = 0;
      for (const auto &input : test_inputs) {
        auto result = parser.parse(input);
        if (result) {
          sum += result->first;
        }
      }
      return sum;
    };
  }

  SECTION("Complex parser performance") {
    auto bid_parser = parsers::lexeme(parsers::unsigned_integer())
                          .then(parsers::lexeme(parsers::char_parser('d')))
                          .then(parsers::lexeme(parsers::unsigned_integer()));

    std::vector<std::string> test_inputs;
    for (int i = 1; i <= 100; ++i) {
      test_inputs.push_back(std::to_string(i) + "d6");
    }

    BENCHMARK("Bid notation parsing") {
      int count = 0;
      for (const auto &input : test_inputs) {
        auto result = bid_parser.parse(input);
        if (result) {
          count++;
        }
      }
      return count;
    };
  }
}

TEST_CASE("Fuzzy matching benchmarks", "[benchmark][fuzzy]") {
  SECTION("Levenshtein distance performance") {
    auto test_pairs = std::vector<std::pair<std::string, std::string>>();

    for (int i = 0; i < 100; ++i) {
      test_pairs.emplace_back("command" + std::to_string(i), "comand" + std::to_string(i));
    }

    BENCHMARK("Levenshtein distance calculation") {
      std::size_t total_distance = 0;
      for (const auto &[s1, s2] : test_pairs) {
        total_distance += FuzzyMatcher::levenshtein_distance(s1, s2);
      }
      return total_distance;
    };
  }

  SECTION("Fuzzy search performance") {
    std::vector<std::string> candidates = {"help",     "quit",      "exit",    "liar",
                                           "call",     "challenge", "history", "stats",
                                           "settings", "about",     "rules",   "tutorial"};

    std::vector<std::string> queries = {"hlp", "qit", "lair", "histroy", "seting"};

    BENCHMARK("Fuzzy matching against candidates") {
      int match_count = 0;
      for (const auto &query : queries) {
        auto matches = FuzzyMatcher::find_best_matches(query, candidates, 0.5, 3);
        match_count += matches.size();
      }
      return match_count;
    };
  }
}

TEST_CASE("Game input validation benchmarks", "[benchmark][game]") {
  SECTION("Bid validation performance") {
    GameInputValidator validator(30, 6);
    std::vector<Bid> test_bids;

    for (uint32_t i = 1; i <= 30; ++i) {
      for (uint32_t j = 1; j <= 6; ++j) {
        test_bids.push_back(Bid{i, j});
      }
    }

    BENCHMARK("Bid validation") {
      int valid_count = 0;
      for (const auto &bid : test_bids) {
        auto result = validator.validate_bid(bid);
        if (result) {
          valid_count++;
        }
      }
      return valid_count;
    };
  }

  SECTION("Action parsing performance") {
    GameInputValidator validator(30, 6);
    std::vector<std::string> test_inputs = {
        "3 5", "3d5", "liar", "help", "quit", "10 dice showing 6", "challenge", "history"};

    BENCHMARK("Action parsing") {
      int action_count = 0;
      for (const auto &input : test_inputs) {
        auto result = validator.parse_action(input, true);
        if (result) {
          action_count++;
        }
      }
      return action_count;
    };
  }
}

TEST_CASE("Input history benchmarks", "[benchmark][history]") {
  SECTION("History insertion performance") {
    InputHistory history(10000);
    auto test_inputs = BenchmarkData::generate_strings(1000, 20);

    BENCHMARK("History insertion") {
      for (const auto &input : test_inputs) {
        history.add(input);
      }
      return history.size();
    };
  }

  SECTION("History search performance") {
    InputHistory history(10000);

    // Pre-populate history
    for (int i = 0; i < 1000; ++i) {
      history.add("command " + std::to_string(i));
    }

    BENCHMARK("History search") {
      int total_matches = 0;
      for (int i = 0; i < 100; i += 10) {
        auto results = history.search(std::to_string(i));
        total_matches += results.size();
      }
      return total_matches;
    };
  }

  SECTION("History navigation performance") {
    InputHistory history(1000);

    // Pre-populate history
    for (int i = 0; i < 1000; ++i) {
      history.add("command " + std::to_string(i));
    }

    BENCHMARK("History navigation (prev/next)") {
      int nav_count = 0;

      // Navigate backward
      while (history.previous()) {
        nav_count++;
        if (nav_count > 100)
          break; // Limit iterations
      }

      // Navigate forward
      while (history.next()) {
        nav_count++;
        if (nav_count > 200)
          break; // Limit iterations
      }

      return nav_count;
    };
  }
}