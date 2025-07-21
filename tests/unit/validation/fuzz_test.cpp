/**
 * @file fuzz_test.cpp
 * @brief Fuzz testing for input validation using random generators
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "liarsdice/ui/fuzzy_match.hpp"
#include "liarsdice/ui/game_input.hpp"
#include "liarsdice/validation/parser_combinators.hpp"
#include "liarsdice/validation/sanitizers.hpp"

#include <algorithm>
#include <random>
#include <string>

using namespace liarsdice::validation;
using namespace liarsdice::validation::sanitizers;
using namespace liarsdice::validation::parsers;
using namespace liarsdice::ui;

class FuzzGenerator {
public:
  explicit FuzzGenerator(unsigned seed = std::random_device{}()) : gen_(seed) {}

  // Generate random string with various characters
  std::string random_string(std::size_t min_len = 0, std::size_t max_len = 100) {
    std::uniform_int_distribution<std::size_t> len_dist(min_len, max_len);
    std::size_t length = len_dist(gen_);

    std::uniform_int_distribution<int> char_dist(0, 255);
    std::string result;
    result.reserve(length);

    for (std::size_t i = 0; i < length; ++i) {
      result.push_back(static_cast<char>(char_dist(gen_)));
    }

    return result;
  }

  // Generate string with specific character set
  std::string random_string_from_set(const std::string &charset, std::size_t min_len = 0,
                                     std::size_t max_len = 100) {
    if (charset.empty())
      return "";

    std::uniform_int_distribution<std::size_t> len_dist(min_len, max_len);
    std::size_t length = len_dist(gen_);

    std::uniform_int_distribution<std::size_t> char_dist(0, charset.size() - 1);
    std::string result;
    result.reserve(length);

    for (std::size_t i = 0; i < length; ++i) {
      result.push_back(charset[char_dist(gen_)]);
    }

    return result;
  }

  // Generate malicious input patterns
  std::string malicious_input() {
    std::vector<std::string> patterns = {
        std::string(10000, 'A'),         // Very long string
        "<script>alert('xss')</script>", // XSS attempt
        "'; DROP TABLE players; --",     // SQL injection
        "../../../etc/passwd",           // Path traversal
        std::string(1, '\0'),            // Null byte
        "\x1B[2J\x1B[1;1H",              // ANSI escape sequences
        std::string(100, '\n'),          // Many newlines
        "\\u0000\\u0001\\u0002",         // Unicode escapes
        std::string(50, '\t'),           // Many tabs
    };

    std::uniform_int_distribution<std::size_t> dist(0, patterns.size() - 1);
    return patterns[dist(gen_)];
  }

private:
  std::mt19937 gen_;
};

TEST_CASE("Sanitizer fuzz testing", "[validation][fuzz][sanitizer]") {
  FuzzGenerator fuzzer;

  SECTION("Trim sanitizer should not crash") {
    auto trimmer = trim();

    for (int i = 0; i < 1000; ++i) {
      std::string input = fuzzer.random_string();

      REQUIRE_NOTHROW([&]() {
        std::string result = trimmer(input);
        // Result should not have leading/trailing whitespace
        if (!result.empty()) {
          REQUIRE(!std::isspace(static_cast<unsigned char>(result.front())));
          REQUIRE(!std::isspace(static_cast<unsigned char>(result.back())));
        }
      }());
    }
  }

  SECTION("HTML escape sanitizer") {
    auto escaper = escape_html();

    for (int i = 0; i < 1000; ++i) {
      std::string input = fuzzer.random_string(0, 200);

      REQUIRE_NOTHROW([&]() {
        std::string result = escaper(input);
        // Result should not contain unescaped HTML characters
        REQUIRE(result.find('<') == std::string::npos || result.find("&lt;") != std::string::npos);
        REQUIRE(result.find('>') == std::string::npos || result.find("&gt;") != std::string::npos);
      }());
    }
  }

  SECTION("Filename sanitizer with malicious inputs") {
    auto sanitizer = filename_safe();

    for (int i = 0; i < 100; ++i) {
      std::string input = fuzzer.malicious_input();

      REQUIRE_NOTHROW([&]() {
        std::string result = sanitizer(input);
        // Result should be a safe filename
        REQUIRE(!result.empty());
        REQUIRE(result.find('/') == std::string::npos);
        REQUIRE(result.find('\\') == std::string::npos);
        REQUIRE(result.find('\0') == std::string::npos);
      }());
    }
  }

  SECTION("Chained sanitizers") {
    auto chain = trim().then(lowercase()).then(alphanumeric_only());

    for (int i = 0; i < 1000; ++i) {
      std::string input = fuzzer.random_string();

      REQUIRE_NOTHROW([&]() {
        std::string result = chain(input);
        // Result should only contain lowercase alphanumeric
        for (char c : result) {
          REQUIRE((std::isalnum(static_cast<unsigned char>(c)) &&
                   std::islower(static_cast<unsigned char>(c))) ||
                  std::isdigit(static_cast<unsigned char>(c)));
        }
      }());
    }
  }
}

TEST_CASE("Parser combinator fuzz testing", "[validation][fuzz][parser]") {
  FuzzGenerator fuzzer;

  SECTION("Integer parser with random input") {
    auto parser = integer();

    for (int i = 0; i < 1000; ++i) {
      std::string input = fuzzer.random_string(0, 50);

      REQUIRE_NOTHROW([&]() {
        auto result = parser.parse(input);
        // Parser should either succeed or fail gracefully
        if (result) {
          auto [value, rest] = *result;
          // If successful, the parsed part should be valid
          std::string parsed_part = input.substr(0, input.length() - rest.length());
          REQUIRE(!parsed_part.empty());
        }
      }());
    }
  }

  SECTION("Complex parser combination") {
    auto complex_parser = lexeme(string_parser("bid:"))
                              .then(lexeme(unsigned_integer()))
                              .then(lexeme(char_parser('d')))
                              .then(lexeme(unsigned_integer()));

    // Test with malformed inputs
    std::vector<std::string> fuzz_inputs;
    for (int i = 0; i < 100; ++i) {
      fuzz_inputs.push_back(fuzzer.random_string(0, 30));
    }

    // Add some semi-valid inputs
    fuzz_inputs.push_back("bid: 3 d 5");
    fuzz_inputs.push_back("bid:3d5");
    fuzz_inputs.push_back("bid: 999999999999d999999999999");

    for (const auto &input : fuzz_inputs) {
      REQUIRE_NOTHROW([&]() {
        auto result = complex_parser.parse(input);
        // Should not crash regardless of input
      }());
    }
  }
}

TEST_CASE("Game input validator fuzz testing", "[validation][fuzz][game]") {
  FuzzGenerator fuzzer;
  GameInputValidator validator(30, 6);

  SECTION("Bid parsing with random input") {
    for (int i = 0; i < 1000; ++i) {
      std::string input = fuzzer.random_string(0, 50);

      REQUIRE_NOTHROW([&]() {
        auto result = validator.parse_action(input, true);
        // Should handle any input without crashing
      }());
    }
  }

  SECTION("Bid parsing with malicious input") {
    for (int i = 0; i < 100; ++i) {
      std::string input = fuzzer.malicious_input();

      REQUIRE_NOTHROW([&]() {
        auto result = validator.parse_action(input, true);
        // Malicious input should be rejected safely
        if (!result) {
          REQUIRE(!result.error().empty());
        }
      }());
    }
  }

  SECTION("Edge case bid values") {
    std::vector<std::pair<uint32_t, uint32_t>> edge_cases = {
        {         0,          0},
        {         0,          1},
        {         1,          0},
        {UINT32_MAX, UINT32_MAX},
        {UINT32_MAX,          1},
        {         1, UINT32_MAX},
        {   1000000,    1000000}
    };

    for (const auto &[qty, face] : edge_cases) {
      Bid bid{qty, face};
      REQUIRE_NOTHROW([&]() {
        auto result = validator.validate_bid(bid);
        // Should handle edge cases gracefully
      }());
    }
  }
}

TEST_CASE("Fuzzy matching fuzz testing", "[validation][fuzz][fuzzy]") {
  FuzzGenerator fuzzer;

  SECTION("Levenshtein distance with random strings") {
    for (int i = 0; i < 1000; ++i) {
      std::string s1 = fuzzer.random_string(0, 50);
      std::string s2 = fuzzer.random_string(0, 50);

      REQUIRE_NOTHROW([&]() {
        auto distance = FuzzyMatcher::levenshtein_distance(s1, s2);
        // Distance should be valid
        REQUIRE(distance <= std::max(s1.length(), s2.length()));

        // Distance to self should be 0
        REQUIRE(FuzzyMatcher::levenshtein_distance(s1, s1) == 0);
      }());
    }
  }

  SECTION("Fuzzy score with extreme inputs") {
    std::vector<std::string> candidates = {"help", "quit", "liar", "history", "yes", "no"};

    for (int i = 0; i < 100; ++i) {
      std::string input = fuzzer.random_string(0, 100);

      REQUIRE_NOTHROW([&]() {
        auto matches = FuzzyMatcher::find_best_matches(input, candidates);
        // Should always return valid results
        for (const auto &match : matches) {
          REQUIRE(match.score >= 0.0);
          REQUIRE(match.score <= 1.0);
        }
      }());
    }
  }

  SECTION("Unicode and special characters") {
    std::vector<std::string> unicode_inputs = {
        u8"Hello 世界",         u8"🎲🎲🎲", u8"Тест", u8"مرحبا", std::string(100, '\xFF'),
        std::string(50, '\x00')};

    CommandSuggester suggester({"test", "help", "quit"});

    for (const auto &input : unicode_inputs) {
      REQUIRE_NOTHROW([&]() {
        auto suggestions = suggester.get_suggestions(input);
        // Should handle unicode gracefully
      }());
    }
  }
}

TEST_CASE("Input history fuzz testing", "[validation][fuzz][history]") {
  FuzzGenerator fuzzer;
  InputHistory history(100);

  SECTION("Adding random entries") {
    for (int i = 0; i < 1000; ++i) {
      std::string input = fuzzer.random_string(0, 200);
      std::string context = fuzzer.random_string_from_set("abcdefghijklmnopqrstuvwxyz", 0, 20);

      REQUIRE_NOTHROW([&]() {
        history.add(input, context);
        // History size should not exceed max
        REQUIRE(history.size() <= 100);
      }());
    }
  }

  SECTION("Searching with random patterns") {
    // Add some entries
    for (int i = 0; i < 50; ++i) {
      history.add("command " + std::to_string(i));
    }

    // Search with random patterns
    for (int i = 0; i < 100; ++i) {
      std::string pattern = fuzzer.random_string(0, 30);

      REQUIRE_NOTHROW([&]() {
        auto results = history.search(pattern);
        // Results should be valid
        for (const auto &entry : results) {
          REQUIRE(entry.input.find(pattern) != std::string::npos);
        }
      }());
    }
  }

  SECTION("File operations with malicious paths") {
    std::vector<std::string> malicious_paths = {"../../../etc/passwd",
                                                "C:\\Windows\\System32\\config\\SAM",
                                                "/dev/null",
                                                "PRN",
                                                "CON",
                                                std::string(1000, '/'),
                                                "\0file.txt"};

    for (const auto &path : malicious_paths) {
      REQUIRE_NOTHROW([&]() {
        // Should handle malicious paths safely
        history.save_to_file(path);
        history.load_from_file(path);
      }());
    }
  }
}