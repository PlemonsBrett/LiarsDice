#pragma once

/**
 * @file parser_combinators.hpp
 * @brief Parser combinators for robust input parsing with std::expected
 */

#include "validation_concepts.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <expected>
#include <liarsdice/utils/format_helper.hpp>

namespace liarsdice::validation {

/**
 * @brief Parser result type
 */
template <typename T>
using ParseResult = std::expected<std::pair<T, std::string_view>, std::string>;

/**
 * @brief Base parser combinator class
 */
template <typename T> class ParserCombinator {
public:
  using value_type = T;
  using parser_func = std::function<ParseResult<T>(std::string_view)>;

  explicit ParserCombinator(parser_func func, std::string name = "parser")
      : parser_(std::move(func)), name_(std::move(name)) {}

  /**
   * @brief Parse input
   */
  [[nodiscard]] ParseResult<T> parse(std::string_view input) const { return parser_(input); }

  /**
   * @brief Map parser result to a different type
   */
  template <typename U> [[nodiscard]] auto map(std::function<U(const T &)> f) const {
    return ParserCombinator<U>(
        [*this, f](std::string_view input) -> ParseResult<U> {
          auto result = parse(input);
          if (!result)
            return std::unexpected(result.error());

          auto [value, rest] = *result;
          return std::make_pair(f(value), rest);
        },
        liarsdice::utils::format_string("map({})", name_));
  }

  /**
   * @brief Sequence parser (then)
   */
  template <typename U> [[nodiscard]] auto then(const ParserCombinator<U> &next) const {
    using ResultType = std::pair<T, U>;
    return ParserCombinator<ResultType>(
        [*this, next](std::string_view input) -> ParseResult<ResultType> {
          auto first_result = parse(input);
          if (!first_result)
            return std::unexpected(first_result.error());

          auto [first_value, rest1] = *first_result;
          auto second_result = next.parse(rest1);
          if (!second_result)
            return std::unexpected(second_result.error());

          auto [second_value, rest2] = *second_result;
          return std::make_pair(std::make_pair(first_value, second_value), rest2);
        },
        liarsdice::utils::format_string("{} >> {}", name_, next.name_));
  }

  /**
   * @brief Alternative parser (or)
   */
  [[nodiscard]] auto operator|(const ParserCombinator<T> &other) const {
    return ParserCombinator<T>(
        [*this, other](std::string_view input) -> ParseResult<T> {
          auto result1 = parse(input);
          if (result1)
            return result1;

          auto result2 = other.parse(input);
          if (result2)
            return result2;

          return std::unexpected(
              liarsdice::utils::format_string("{} failed, {} failed", name_, other.name_));
        },
        liarsdice::utils::format_string("{} | {}", name_, other.name_));
  }

  /**
   * @brief Optional parser
   */
  [[nodiscard]] auto optional() const {
    using OptionalType = std::optional<T>;
    return ParserCombinator<OptionalType>(
        [*this](std::string_view input) -> ParseResult<OptionalType> {
          auto result = parse(input);
          if (!result) {
            return std::make_pair(std::nullopt, input);
          }

          auto [value, rest] = *result;
          return std::make_pair(std::make_optional(value), rest);
        },
        liarsdice::utils::format_string("optional({})", name_));
  }

  /**
   * @brief Many parser (zero or more)
   */
  [[nodiscard]] auto many() const {
    using VectorType = std::vector<T>;
    return ParserCombinator<VectorType>(
        [*this](std::string_view input) -> ParseResult<VectorType> {
          VectorType results;
          std::string_view remaining = input;

          while (true) {
            auto result = parse(remaining);
            if (!result)
              break;

            auto [value, rest] = *result;
            results.push_back(std::move(value));
            remaining = rest;
          }

          return std::make_pair(std::move(results), remaining);
        },
        liarsdice::utils::format_string("many({})", name_));
  }

  /**
   * @brief Many1 parser (one or more)
   */
  [[nodiscard]] auto many1() const {
    using VectorType = std::vector<T>;
    return ParserCombinator<VectorType>(
        [*this](std::string_view input) -> ParseResult<VectorType> {
          auto result = many().parse(input);
          if (!result)
            return std::unexpected(result.error());

          auto [values, rest] = *result;
          if (values.empty()) {
            return std::unexpected(name_ + ": Expected at least one match");
          }

          return std::make_pair(std::move(values), rest);
        },
        liarsdice::utils::format_string("many1({})", name_));
  }

  [[nodiscard]] const std::string &name() const { return name_; }

private:
  parser_func parser_;
  std::string name_;
};

/**
 * @brief Parser combinator factory functions
 */
namespace parsers {

/**
 * @brief Parse a specific character
 */
[[nodiscard]] inline auto char_parser(char c) {
  return ParserCombinator<char>(
      [c](std::string_view input) -> ParseResult<char> {
        if (input.empty() || input[0] != c) {
          return std::unexpected(std::string("Expected '") + c + "', got '" +
                                 (input.empty() ? "EOF" : std::string(1, input[0])) + "'");
        }
        return std::make_pair(c, input.substr(1));
      },
      std::string("char('") + c + "')");
}

/**
 * @brief Parse any character matching a predicate
 */
[[nodiscard]] inline auto satisfy(std::function<bool(char)> pred, std::string name = "satisfy") {
  return ParserCombinator<char>(
      [pred, name](std::string_view input) -> ParseResult<char> {
        if (input.empty() || !pred(input[0])) {
          return std::unexpected(name + ": No match");
        }
        return std::make_pair(input[0], input.substr(1));
      },
      name);
}

/**
 * @brief Parse a digit
 */
[[nodiscard]] inline auto digit() {
  return satisfy([](char c) { return std::isdigit(c); }, "digit");
}

/**
 * @brief Parse a letter
 */
[[nodiscard]] inline auto letter() {
  return satisfy([](char c) { return std::isalpha(c); }, "letter");
}

/**
 * @brief Parse whitespace
 */
[[nodiscard]] inline auto whitespace() {
  return satisfy([](char c) { return std::isspace(c); }, "whitespace").many();
}

/**
 * @brief Parse a specific string
 */
[[nodiscard]] inline auto string_parser(std::string_view str) {
  return ParserCombinator<std::string>(
      [str](std::string_view input) -> ParseResult<std::string> {
        if (!input.starts_with(str)) {
          return std::unexpected(std::string("Expected '") + std::string(str) + "', got '" +
                                 std::string(input.substr(0, std::min(str.size(), input.size()))) +
                                 "'");
        }
        return std::make_pair(std::string(str), input.substr(str.size()));
      },
      std::string("string(\"") + std::string(str) + "\")");
}

/**
 * @brief Parse an integer
 */
[[nodiscard]] inline auto integer() {
  return ParserCombinator<int>(
      [](std::string_view input) -> ParseResult<int> {
        int value;
        auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), value);

        if (ec != std::errc{}) {
          return std::unexpected("Failed to parse integer");
        }

        std::size_t consumed = ptr - input.data();
        return std::make_pair(value, input.substr(consumed));
      },
      "integer");
}

/**
 * @brief Parse an unsigned integer
 */
[[nodiscard]] inline auto unsigned_integer() {
  return ParserCombinator<unsigned int>(
      [](std::string_view input) -> ParseResult<unsigned int> {
        unsigned int value;
        auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), value);

        if (ec != std::errc{}) {
          return std::unexpected("Failed to parse unsigned integer");
        }

        std::size_t consumed = ptr - input.data();
        return std::make_pair(value, input.substr(consumed));
      },
      "unsigned_integer");
}

/**
 * @brief Parse a floating point number
 */
[[nodiscard]] inline auto floating() {
  return ParserCombinator<double>(
      [](std::string_view input) -> ParseResult<double> {
        // Note: std::from_chars for floating point requires C++17 with proper library support
        // For compatibility, we might need to use a different approach
        double value;
        char *end;
        std::string temp(input);
        value = std::strtod(temp.c_str(), &end);

        if (end == temp.c_str()) {
          return std::unexpected("Failed to parse floating point number");
        }

        std::size_t consumed = end - temp.c_str();
        return std::make_pair(value, input.substr(consumed));
      },
      "floating");
}

/**
 * @brief Parse between delimiters
 */
template <typename T>
[[nodiscard]] auto between(const ParserCombinator<char> &left, const ParserCombinator<char> &right,
                           const ParserCombinator<T> &parser) {
  return left.then(parser)
      .map([](const auto &pair) { return pair.second; })
      .then(right)
      .map([](const auto &pair) { return pair.first; });
}

/**
 * @brief Parse separated list
 */
template <typename T>
[[nodiscard]] auto sep_by(const ParserCombinator<T> &parser,
                          const ParserCombinator<char> &separator) {
  using VectorType = std::vector<T>;
  return ParserCombinator<VectorType>(
      [parser, separator](std::string_view input) -> ParseResult<VectorType> {
        VectorType results;

        // Try to parse first element
        auto first = parser.parse(input);
        if (!first) {
          return std::make_pair(results, input); // Empty list is valid
        }

        auto [value, rest] = *first;
        results.push_back(std::move(value));

        // Parse remaining elements
        while (true) {
          auto sep_result = separator.parse(rest);
          if (!sep_result)
            break;

          auto [_, rest2] = *sep_result;
          auto elem_result = parser.parse(rest2);
          if (!elem_result)
            break;

          auto [elem, rest3] = *elem_result;
          results.push_back(std::move(elem));
          rest = rest3;
        }

        return std::make_pair(std::move(results), rest);
      },
      liarsdice::utils::format_string("sep_by({}, {})", parser.name(), separator.name()));
}

/**
 * @brief Skip whitespace before parsing
 */
template <typename T> [[nodiscard]] auto lexeme(const ParserCombinator<T> &parser) {
  using PairType = std::pair<std::vector<char>, T>;
  return whitespace().then(parser).map([](const PairType &pair) { return pair.second; });
}

} // namespace parsers

} // namespace liarsdice::validation