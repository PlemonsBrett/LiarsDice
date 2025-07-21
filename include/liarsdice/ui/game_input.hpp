#pragma once

/**
 * @file game_input.hpp
 * @brief Game-specific input processing with robust validation
 */

#include "../core/dice.hpp"
#include "../core/game.hpp"
#include "../validation/error_aggregator.hpp"
#include "../validation/parser_combinators.hpp"
#include "../validation/sanitizers.hpp"
#include "../validation/validation_concepts.hpp"
#include "../validation/validators.hpp"
#include <expected>
#include <format>
#include <optional>
#include <string>

namespace liarsdice::ui {

/**
 * @brief Bid structure for game input
 */
struct Bid {
  uint32_t quantity{0};
  uint32_t face_value{0};

  [[nodiscard]] std::string to_string() const {
    return std::format("{} dice showing {}", quantity, face_value);
  }

  [[nodiscard]] bool is_valid(uint32_t total_dice, uint32_t max_face_value) const {
    return quantity > 0 && quantity <= total_dice && face_value >= 1 &&
           face_value <= max_face_value;
  }

  auto operator<=>(const Bid &other) const = default;
};

/**
 * @brief Game action types
 */
enum class GameAction { MakeBid, CallLiar, ShowHelp, ShowHistory, Quit };

/**
 * @brief Action result
 */
struct ActionResult {
  GameAction action;
  std::optional<Bid> bid;
};

/**
 * @brief Game input validator
 */
class GameInputValidator {
public:
  explicit GameInputValidator(uint32_t total_dice = 30, uint32_t max_face_value = 6)
      : total_dice_(total_dice), max_face_value_(max_face_value) {}

  /**
   * @brief Create bid parser
   */
  [[nodiscard]] auto bid_parser() const {
    using namespace validation::parsers;

    // Parse format: "3 5" or "3d5" or "3 dice showing 5"
    auto simple_bid =
        lexeme(unsigned_integer())
            .then(lexeme(unsigned_integer()))
            .map([](const auto &pair) -> Bid { return Bid{pair.first, pair.second}; });

    auto dice_notation = lexeme(unsigned_integer())
                             .then(char_parser('d') | char_parser('D'))
                             .then(unsigned_integer())
                             .map([](const auto &result) -> Bid {
                               const auto &[qty_d, face] = result;
                               const auto &[qty, _] = qty_d;
                               return Bid{qty, face};
                             });

    auto verbose_bid = lexeme(unsigned_integer())
                           .then(lexeme(string_parser("dice")) | lexeme(string_parser("die")))
                           .then(lexeme(string_parser("showing")) | lexeme(string_parser("of")))
                           .then(lexeme(unsigned_integer()))
                           .map([](const auto &result) -> Bid {
                             const auto &[qty_dice_showing, face] = result;
                             const auto &[qty_dice, _] = qty_dice_showing;
                             const auto &[qty, __] = qty_dice;
                             return Bid{qty, face};
                           });

    return dice_notation | simple_bid | verbose_bid;
  }

  /**
   * @brief Validate bid
   */
  [[nodiscard]] validation::ValidationResult<Bid> validate_bid(const Bid &bid) const {
    using namespace validation::validators;

    validation::ValidationErrorAggregator aggregator;

    // Validate quantity
    auto qty_validator = range(1u, total_dice_, "quantity");
    if (auto error = qty_validator(bid.quantity)) {
      error->error_message =
          std::format("Quantity must be between 1 and {} (total dice in game)", total_dice_);
      aggregator.add(*error);
    }

    // Validate face value
    auto face_validator = range(1u, max_face_value_, "face_value");
    if (auto error = face_validator(bid.face_value)) {
      error->error_message = std::format("Face value must be between 1 and {}", max_face_value_);
      aggregator.add(*error);
    }

    if (aggregator.has_errors()) {
      return std::unexpected(aggregator.get_errors());
    }

    return bid;
  }

  /**
   * @brief Validate bid is higher than previous
   */
  [[nodiscard]] validation::ValidationResult<Bid>
  validate_bid_progression(const Bid &new_bid, const Bid &previous_bid) const {

    validation::ValidationErrorAggregator aggregator;

    // First validate the bid itself
    auto basic_validation = validate_bid(new_bid);
    if (!basic_validation) {
      return basic_validation;
    }

    // Check progression rules
    bool is_valid = false;

    if (new_bid.face_value > previous_bid.face_value) {
      // Higher face value with same or more dice
      is_valid = new_bid.quantity >= previous_bid.quantity;
    } else if (new_bid.face_value == previous_bid.face_value) {
      // Same face value requires more dice
      is_valid = new_bid.quantity > previous_bid.quantity;
    } else {
      // Lower face value requires significantly more dice
      is_valid = new_bid.quantity > previous_bid.quantity;
    }

    if (!is_valid) {
      aggregator.add(validation::ValidationError{
          .field_name = "bid",
          .error_message = "Bid must be higher than previous bid (" + previous_bid.to_string() +
                           "). " + "Either increase the quantity or the face value."});

      // Add suggestion
      aggregator.add_warning(
          "bid", "Suggestion",
          "Try " + Bid{previous_bid.quantity + 1, previous_bid.face_value}.to_string() + " or " +
              Bid{previous_bid.quantity, previous_bid.face_value + 1}.to_string());

      return std::unexpected(aggregator.get_errors());
    }

    return new_bid;
  }

  /**
   * @brief Parse game action from input
   */
  [[nodiscard]] validation::ValidationResult<ActionResult>
  parse_action(std::string_view input, bool has_previous_bid = false) const {

    using namespace validation::sanitizers;

    // Sanitize input
    std::string clean_input = chain(trim(), lowercase(), collapse_whitespace())(input);

    validation::ValidationErrorAggregator aggregator;

    // Check for special commands first
    if (clean_input == "liar" || clean_input == "call" || clean_input == "challenge") {
      if (!has_previous_bid) {
        aggregator.add(validation::ValidationError{
            .field_name = "action", .error_message = "Cannot call liar on the first turn"});
        return std::unexpected(aggregator.get_errors());
      }
      return ActionResult{GameAction::CallLiar, std::nullopt};
    }

    if (clean_input == "help" || clean_input == "?") {
      return ActionResult{GameAction::ShowHelp, std::nullopt};
    }

    if (clean_input == "history" || clean_input == "h") {
      return ActionResult{GameAction::ShowHistory, std::nullopt};
    }

    if (clean_input == "quit" || clean_input == "exit" || clean_input == "q") {
      return ActionResult{GameAction::Quit, std::nullopt};
    }

    // Try to parse as bid
    auto bid_result = bid_parser().parse(clean_input);
    if (bid_result) {
      auto [bid, _] = *bid_result;
      auto validation_result = validate_bid(bid);
      if (!validation_result) {
        return std::unexpected(validation_result.error());
      }
      return ActionResult{GameAction::MakeBid, bid};
    }

    // Invalid input
    aggregator.add(validation::ValidationError{
        .field_name = "input",
        .error_message =
            "Invalid input. Expected bid (e.g., '3 5' or '3d5'), 'liar', 'help', or 'quit'"});

    return std::unexpected(aggregator.get_errors());
  }

  /**
   * @brief Update game parameters
   */
  void update_parameters(uint32_t total_dice, uint32_t max_face_value) {
    total_dice_ = total_dice;
    max_face_value_ = max_face_value;
  }

private:
  uint32_t total_dice_;
  uint32_t max_face_value_;
};

/**
 * @brief Player name validator
 */
[[nodiscard]] inline auto create_player_name_validator() {
  using namespace validation::validators;

  return non_empty("name") && length(1, 20, "name") && pattern(R"(^[a-zA-Z0-9_\-\s]+$)", "name") &&
         predicate<std::string>(
             [](const std::string &name) {
               // At least one non-space character
               return std::ranges::any_of(
                   name, [](char c) { return !std::isspace(static_cast<unsigned char>(c)); });
             },
             "Name must contain at least one non-space character", "name");
}

/**
 * @brief Yes/No validator
 */
[[nodiscard]] inline validation::ValidationResult<bool> parse_yes_no(std::string_view input) {
  using namespace validation::sanitizers;

  std::string clean = chain(trim(), lowercase())(input);

  if (clean == "y" || clean == "yes" || clean == "1" || clean == "true") {
    return true;
  }

  if (clean == "n" || clean == "no" || clean == "0" || clean == "false") {
    return false;
  }

  return std::unexpected(validation::ValidationErrors{{validation::ValidationError{
      .field_name = "response", .error_message = "Please enter 'yes' or 'no' (or y/n)"}}});
}

/**
 * @brief Number of players validator
 */
[[nodiscard]] inline auto create_player_count_validator(uint32_t min_players = 2,
                                                        uint32_t max_players = 6) {
  using namespace validation::validators;

  return range(min_players, max_players, "player_count") &&
         predicate<uint32_t>([](uint32_t count) { return count >= 2; },
                             "At least 2 players are required", "player_count");
}

} // namespace liarsdice::ui