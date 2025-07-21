/**
 * @file validator_test.cpp
 * @brief Property-based tests for input validation
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "liarsdice/validation/validators.hpp"
#include <random>
#include <set>
#include <string>

using namespace liarsdice::validation;
using namespace liarsdice::validation::validators;

TEST_CASE("Range validator properties", "[validation][property]") {
  SECTION("Range validator accepts values within bounds") {
    auto validator = range(10, 20, "test");

    // Property: all values in range should pass
    for (int i = 10; i <= 20; ++i) {
      REQUIRE(!validator(i).has_value());
    }

    // Property: all values outside range should fail
    for (int i : {-100, -1, 0, 9, 21, 100, 1000}) {
      REQUIRE(validator(i).has_value());
    }
  }

  SECTION("Range validator with generated values") {
    std::random_device rd;
    std::mt19937 gen(rd());

    int min_val = GENERATE(0, 1, 10, -10, -100);
    int max_val = min_val + GENERATE(1, 10, 100, 1000);

    auto validator = range(min_val, max_val);

    // Generate values within range
    std::uniform_int_distribution<> in_range(min_val, max_val);
    for (int i = 0; i < 100; ++i) {
      int value = in_range(gen);
      REQUIRE(!validator(value).has_value());
    }

    // Generate values outside range
    if (min_val > std::numeric_limits<int>::min() + 1000) {
      std::uniform_int_distribution<> below_range(std::numeric_limits<int>::min(), min_val - 1);
      for (int i = 0; i < 10; ++i) {
        int value = below_range(gen);
        REQUIRE(validator(value).has_value());
      }
    }

    if (max_val < std::numeric_limits<int>::max() - 1000) {
      std::uniform_int_distribution<> above_range(max_val + 1, std::numeric_limits<int>::max());
      for (int i = 0; i < 10; ++i) {
        int value = above_range(gen);
        REQUIRE(validator(value).has_value());
      }
    }
  }
}

TEST_CASE("String validators properties", "[validation][property][string]") {
  SECTION("Length validator") {
    auto len_validator = length(3, 10);

    // Property: strings within length bounds pass
    std::vector<std::string> valid_strings = {"abc", "test", "hello", "world123", "0123456789"};

    for (const auto &s : valid_strings) {
      REQUIRE(!len_validator(s).has_value());
    }

    // Property: strings outside bounds fail
    std::vector<std::string> invalid_strings = {"", "a", "ab", "this is too long for validator"};

    for (const auto &s : invalid_strings) {
      REQUIRE(len_validator(s).has_value());
    }
  }

  SECTION("Pattern validator with generated strings") {
    auto email_validator = pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})", "email");

    // Valid emails should pass
    std::vector<std::string> valid_emails = {"test@example.com", "user.name@domain.co.uk",
                                             "123@test.org"};

    for (const auto &email : valid_emails) {
      REQUIRE(!email_validator(email).has_value());
    }

    // Invalid emails should fail
    std::vector<std::string> invalid_emails = {"notanemail", "@example.com", "test@", "test@.com",
                                               "test..@example.com"};

    for (const auto &email : invalid_emails) {
      REQUIRE(email_validator(email).has_value());
    }
  }
}

TEST_CASE("Validator composition properties", "[validation][property][composition]") {
  SECTION("AND composition") {
    auto positive = min(0, "value");
    auto less_than_100 = max(100, "value");
    auto combined = positive && less_than_100;

    // Property: value must satisfy both conditions
    for (int i = 0; i <= 100; ++i) {
      REQUIRE(!combined(i).has_value());
    }

    for (int i : {-10, -1, 101, 1000}) {
      REQUIRE(combined(i).has_value());
    }
  }

  SECTION("OR composition") {
    auto very_small = max(10, "value");
    auto very_large = min(1000, "value");
    auto combined = very_small || very_large;

    // Property: value must satisfy at least one condition
    for (int i : {-100, 0, 5, 10, 1000, 2000}) {
      REQUIRE(!combined(i).has_value());
    }

    for (int i : {50, 100, 500, 999}) {
      REQUIRE(combined(i).has_value());
    }
  }

  SECTION("NOT composition") {
    auto positive = min(0, "value");
    auto negative = !positive;

    // Property: NOT inverts the validation
    for (int i = -100; i < 0; ++i) {
      REQUIRE(!negative(i).has_value());
    }

    for (int i = 0; i <= 100; ++i) {
      REQUIRE(negative(i).has_value());
    }
  }
}

TEST_CASE("Custom predicate validators", "[validation][property][predicate]") {
  SECTION("Even number validator") {
    auto even_validator =
        predicate<int>([](int n) { return n % 2 == 0; }, "Must be an even number", "number");

    // Generate random even and odd numbers
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(-1000, 1000);

    for (int i = 0; i < 100; ++i) {
      int value = dist(gen);
      bool is_even = (value % 2 == 0);
      bool validation_passed = !even_validator(value).has_value();
      REQUIRE(is_even == validation_passed);
    }
  }

  SECTION("Prime number validator") {
    auto is_prime = [](int n) {
      if (n < 2)
        return false;
      if (n == 2)
        return true;
      if (n % 2 == 0)
        return false;
      for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0)
          return false;
      }
      return true;
    };

    auto prime_validator = predicate<int>(is_prime, "Must be a prime number");

    // Known primes
    std::vector<int> primes = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31};
    for (int p : primes) {
      REQUIRE(!prime_validator(p).has_value());
    }

    // Known non-primes
    std::vector<int> non_primes = {0, 1, 4, 6, 8, 9, 10, 12, 14, 15};
    for (int n : non_primes) {
      REQUIRE(prime_validator(n).has_value());
    }
  }
}

TEST_CASE("One-of validator properties", "[validation][property][one_of]") {
  SECTION("String one-of") {
    auto color_validator = one_of<std::string>({"red", "green", "blue"}, "color");

    // Valid values
    for (const auto &color : {"red", "green", "blue"}) {
      REQUIRE(!color_validator(std::string(color)).has_value());
    }

    // Invalid values
    for (const auto &color : {"yellow", "purple", "orange", "", "RED", "Blue"}) {
      REQUIRE(color_validator(std::string(color)).has_value());
    }
  }

  SECTION("Integer one-of with generated values") {
    std::set<int> valid_values = {1, 5, 10, 20, 50, 100};
    auto validator = one_of<int>({1, 5, 10, 20, 50, 100}, "denomination");

    // Test with random values
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 200);

    for (int i = 0; i < 1000; ++i) {
      int value = dist(gen);
      bool should_pass = valid_values.count(value) > 0;
      bool did_pass = !validator(value).has_value();
      REQUIRE(should_pass == did_pass);
    }
  }
}

TEST_CASE("Numeric string validators", "[validation][property][numeric]") {
  SECTION("Numeric validator with generated strings") {
    auto num_validator = numeric();

    // Generate numeric strings
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> len_dist(1, 10);
    std::uniform_int_distribution<> digit_dist(0, 9);

    for (int i = 0; i < 100; ++i) {
      std::string num_str;
      int len = len_dist(gen);
      for (int j = 0; j < len; ++j) {
        num_str += std::to_string(digit_dist(gen));
      }
      REQUIRE(!num_validator(num_str).has_value());
    }

    // Non-numeric strings
    std::vector<std::string> non_numeric = {"abc", "12a34", "12.34", "-123", "+123", "1 2 3", ""};

    for (const auto &s : non_numeric) {
      REQUIRE(num_validator(s).has_value());
    }
  }

  SECTION("Alphanumeric validator") {
    auto alnum_validator = alphanumeric();

    // Valid alphanumeric strings
    std::vector<std::string> valid = {"abc123", "TEST", "123", "NoSpaces2023"};

    for (const auto &s : valid) {
      REQUIRE(!alnum_validator(s).has_value());
    }

    // Invalid strings
    std::vector<std::string> invalid = {"has spaces", "special@char", "dash-test", "",
                                        "under_score"};

    for (const auto &s : invalid) {
      REQUIRE(alnum_validator(s).has_value());
    }
  }
}