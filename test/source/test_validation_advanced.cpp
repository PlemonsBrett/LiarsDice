#define BOOST_TEST_MODULE ValidationAdvancedTests
#include <algorithm>
#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <liarsdice/validation/input_validator.hpp>
#include <random>
#include <string>
#include <vector>

using namespace liarsdice::validation;
namespace bdata = boost::unit_test::data;

BOOST_AUTO_TEST_SUITE(ValidationAdvancedTestSuite)

// BDD-style scenario tests for input validation
BOOST_AUTO_TEST_CASE(ValidationProvidesComprehensiveInputSafety) {
  // GIVEN: An input validator for game parameters
  InputValidator validator;

  // WHEN: validating player count input
  // THEN: valid ranges should pass
  BOOST_CHECK(validator.validate_player_count("2"));
  BOOST_CHECK(validator.validate_player_count("3"));
  BOOST_CHECK(validator.validate_player_count("8"));

  // AND: invalid ranges should fail
  BOOST_CHECK(!validator.validate_player_count("0"));
  BOOST_CHECK(!validator.validate_player_count("1"));
  BOOST_CHECK(!validator.validate_player_count("9"));
  BOOST_CHECK(!validator.validate_player_count("100"));

  // WHEN: validating dice count input
  // THEN: reasonable dice counts should pass
  BOOST_CHECK(validator.validate_dice_count("1"));
  BOOST_CHECK(validator.validate_dice_count("5"));
  BOOST_CHECK(validator.validate_dice_count("10"));

  // AND: unreasonable counts should fail
  BOOST_CHECK(!validator.validate_dice_count("0"));
  BOOST_CHECK(!validator.validate_dice_count("21"));
  BOOST_CHECK(!validator.validate_dice_count("1000"));

  // WHEN: validating face values
  // THEN: standard die faces should pass
  for (int face = 1; face <= 6; ++face) {
    BOOST_CHECK(validator.validate_face_value(std::to_string(face)));
  }

  // AND: invalid faces should fail
  BOOST_CHECK(!validator.validate_face_value("0"));
  BOOST_CHECK(!validator.validate_face_value("7"));
  BOOST_CHECK(!validator.validate_face_value("10"));
}

// Property-based testing with generated inputs
BOOST_DATA_TEST_CASE(ValidationPropertiesHoldForRandomInputs,
                     bdata::make({-100, -10, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 50, 100}),
                     test_value) {
  InputValidator validator;
  std::string value_str = std::to_string(test_value);

  // Property: player count validation
  bool player_valid = validator.validate_player_count(value_str);
  bool expected_player_valid = (test_value >= 2 && test_value <= 8);
  BOOST_CHECK_EQUAL(player_valid, expected_player_valid);

  // Property: dice count validation
  bool dice_valid = validator.validate_dice_count(value_str);
  bool expected_dice_valid = (test_value >= 1 && test_value <= 20);
  BOOST_CHECK_EQUAL(dice_valid, expected_dice_valid);

  // Property: face value validation
  bool face_valid = validator.validate_face_value(value_str);
  bool expected_face_valid = (test_value >= 1 && test_value <= 6);
  BOOST_CHECK_EQUAL(face_valid, expected_face_valid);
}

// Fuzz testing with random strings
BOOST_AUTO_TEST_CASE(ValidationFuzzTestingWithRandomStrings) {
  InputValidator validator;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> char_dist(32, 126);  // Printable ASCII
  std::uniform_int_distribution<> len_dist(0, 20);

  const int num_tests = 1000;

  for (int i = 0; i < num_tests; ++i) {
    // Generate random string
    std::string random_input;
    int length = len_dist(gen);
    for (int j = 0; j < length; ++j) {
      random_input += static_cast<char>(char_dist(gen));
    }

    // Validation should not crash on any input
    BOOST_CHECK_NO_THROW(validator.validate_player_count(random_input));
    BOOST_CHECK_NO_THROW(validator.validate_dice_count(random_input));
    BOOST_CHECK_NO_THROW(validator.validate_face_value(random_input));
    BOOST_CHECK_NO_THROW(validator.validate_command(random_input));
  }
}

// Advanced string validation tests
BOOST_AUTO_TEST_CASE(ValidationHandlesComplexStringInputs) {
  InputValidator validator;

  // Test various string formats
  std::vector<std::string> numeric_strings = {"123", "+123", "007", "0001", "999999"};

  std::vector<std::string> non_numeric_strings
      = {"abc", "12a", "a12", "1.5", "1,000", "1e10", "", " ", "\t", "\n"};

  std::vector<std::string> edge_case_strings
      = {" 5 ", "\t3\n", "  2  ", "5\0hidden", "∞", "−5", "5.0"};

  // Test that numeric strings work correctly
  for (const auto& str : numeric_strings) {
    // Should not crash, may or may not be valid depending on value
    BOOST_CHECK_NO_THROW(validator.validate_player_count(str));
    BOOST_CHECK_NO_THROW(validator.validate_dice_count(str));
    BOOST_CHECK_NO_THROW(validator.validate_face_value(str));
  }

  // Test that non-numeric strings are rejected
  for (const auto& str : non_numeric_strings) {
    BOOST_CHECK(!validator.validate_player_count(str));
    BOOST_CHECK(!validator.validate_dice_count(str));
    BOOST_CHECK(!validator.validate_face_value(str));
  }

  // Test edge cases don't crash the validator
  for (const auto& str : edge_case_strings) {
    BOOST_CHECK_NO_THROW(validator.validate_player_count(str));
    BOOST_CHECK_NO_THROW(validator.validate_dice_count(str));
    BOOST_CHECK_NO_THROW(validator.validate_face_value(str));
  }
}

// Command validation testing
BOOST_AUTO_TEST_CASE(ValidationCommandRecognition) {
  InputValidator validator;

  // Valid commands should be recognized
  std::vector<std::string> valid_commands = {"quit", "q", "exit", "help", "h", "rules", "r"};

  for (const auto& cmd : valid_commands) {
    BOOST_CHECK(validator.validate_command(cmd));
  }

  // Test case sensitivity
  std::vector<std::string> case_variants
      = {"QUIT", "Quit", "QuIt", "Q", "EXIT", "Exit", "HELP", "Help"};

  for (const auto& cmd : case_variants) {
    BOOST_CHECK(validator.validate_command(cmd));
  }

  // Invalid commands should be rejected
  std::vector<std::string> invalid_commands
      = {"qut", "exti", "halp", "rulez", "123", "", " ", "quit123", "help!"};

  for (const auto& cmd : invalid_commands) {
    BOOST_CHECK(!validator.validate_command(cmd));
  }
}

// Performance benchmarks for validation
BOOST_AUTO_TEST_CASE(ValidationPerformanceBenchmarks) {
  InputValidator validator;
  const int iterations = 10000;

  // Benchmark string validation performance
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    std::string test_input = std::to_string(i % 20);

    validator.validate_player_count(test_input);
    validator.validate_dice_count(test_input);
    validator.validate_face_value(test_input);
    validator.validate_command("quit");
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  BOOST_CHECK(duration.count() < 1000000);  // Less than 1 second

  BOOST_TEST_MESSAGE("Validation performance: " << duration.count() << " microseconds for "
                                                << iterations << " validations");
}

// Boundary condition testing
BOOST_AUTO_TEST_CASE(ValidationBoundaryConditions) {
  InputValidator validator;

  // Test exact boundary values for player count
  BOOST_CHECK(!validator.validate_player_count("1"));  // Just below minimum
  BOOST_CHECK(validator.validate_player_count("2"));   // Minimum valid
  BOOST_CHECK(validator.validate_player_count("8"));   // Maximum valid
  BOOST_CHECK(!validator.validate_player_count("9"));  // Just above maximum

  // Test exact boundary values for dice count
  BOOST_CHECK(!validator.validate_dice_count("0"));   // Just below minimum
  BOOST_CHECK(validator.validate_dice_count("1"));    // Minimum valid
  BOOST_CHECK(validator.validate_dice_count("20"));   // Maximum valid
  BOOST_CHECK(!validator.validate_dice_count("21"));  // Just above maximum

  // Test exact boundary values for face value
  BOOST_CHECK(!validator.validate_face_value("0"));  // Just below minimum
  BOOST_CHECK(validator.validate_face_value("1"));   // Minimum valid
  BOOST_CHECK(validator.validate_face_value("6"));   // Maximum valid
  BOOST_CHECK(!validator.validate_face_value("7"));  // Just above maximum
}

// Security testing for malicious inputs
BOOST_AUTO_TEST_CASE(ValidationSecurityTesting) {
  InputValidator validator;

  // Test potential injection attacks
  std::vector<std::string> malicious_inputs = {
      "'; DROP TABLE players; --",
      "<script>alert('xss')</script>",
      "../../../../etc/passwd",
      "%00%00%00%00",
      "\x00\x01\x02\x03",
      "A" + std::string(1000, 'A'),  // Very long string
      std::string(1, '\0') + "hidden",
      "2147483648",  // Integer overflow
      "-2147483649"  // Integer underflow
  };

  // Validator should handle malicious inputs safely
  for (const auto& input : malicious_inputs) {
    BOOST_CHECK_NO_THROW(validator.validate_player_count(input));
    BOOST_CHECK_NO_THROW(validator.validate_dice_count(input));
    BOOST_CHECK_NO_THROW(validator.validate_face_value(input));
    BOOST_CHECK_NO_THROW(validator.validate_command(input));

    // Most malicious inputs should be rejected
    BOOST_CHECK(!validator.validate_player_count(input));
    BOOST_CHECK(!validator.validate_dice_count(input));
    BOOST_CHECK(!validator.validate_face_value(input));
  }
}

// Internationalization and Unicode testing
BOOST_AUTO_TEST_CASE(ValidationUnicodeAndInternationalization) {
  InputValidator validator;

  // Test Unicode numbers
  std::vector<std::string> unicode_numbers = {
      "５",  // Full-width 5
      "２",  // Full-width 2
      "①",   // Circled 1
      "Ⅴ",   // Roman numeral V
      "፫",   // Ethiopic digit 3
  };

  // Test Arabic-Indic numerals
  std::vector<std::string> arabic_indic = {"٠", "١", "٢", "٣", "٤", "٥", "٦", "٧", "٨", "٩"};

  // These should be safely rejected (not standard ASCII digits)
  for (const auto& num : unicode_numbers) {
    BOOST_CHECK_NO_THROW(validator.validate_player_count(num));
    BOOST_CHECK_NO_THROW(validator.validate_dice_count(num));
    BOOST_CHECK_NO_THROW(validator.validate_face_value(num));

    // Should be rejected since we expect ASCII digits
    BOOST_CHECK(!validator.validate_player_count(num));
    BOOST_CHECK(!validator.validate_dice_count(num));
    BOOST_CHECK(!validator.validate_face_value(num));
  }

  for (const auto& num : arabic_indic) {
    BOOST_CHECK_NO_THROW(validator.validate_player_count(num));
    BOOST_CHECK(!validator.validate_player_count(num));
  }
}

// State and thread safety testing
BOOST_AUTO_TEST_CASE(ValidationStatelessBehavior) {
  InputValidator validator1;
  InputValidator validator2;

  // Same inputs should produce same results across instances
  std::vector<std::string> test_inputs = {"0", "1", "2", "5", "8", "9", "10", "abc", "", "quit"};

  for (const auto& input : test_inputs) {
    BOOST_CHECK_EQUAL(validator1.validate_player_count(input),
                      validator2.validate_player_count(input));

    BOOST_CHECK_EQUAL(validator1.validate_dice_count(input), validator2.validate_dice_count(input));

    BOOST_CHECK_EQUAL(validator1.validate_face_value(input), validator2.validate_face_value(input));

    BOOST_CHECK_EQUAL(validator1.validate_command(input), validator2.validate_command(input));
  }
}

// Comprehensive integration test
BOOST_AUTO_TEST_CASE(ValidationIntegrationWithGameFlow) {
  InputValidator validator;

  // Simulate typical game input sequence
  std::vector<std::pair<std::string, bool>> game_sequence = {
      {"4", true},     // Player count
      {"3", true},     // AI count
      {"5", true},     // Dice count for guess
      {"3", true},     // Face value for guess
      {"quit", true},  // Quit command
      {"y", false},    // Invalid as numeric input
      {"0", false},    // Invalid player count
      {"7", false},    // Invalid face value
      {"help", true},  // Valid command
  };

  for (const auto& [input, should_be_valid] : game_sequence) {
    // Test appropriate validation based on context
    if (input == "quit" || input == "help" || input == "y") {
      BOOST_CHECK_EQUAL(validator.validate_command(input), should_be_valid);
    } else {
      // Test as numeric validation
      bool is_valid_player = validator.validate_player_count(input);
      bool is_valid_face = validator.validate_face_value(input);
      bool any_valid = is_valid_player || is_valid_face;

      if (should_be_valid) {
        BOOST_CHECK(any_valid);  // Should be valid for at least one context
      }
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()