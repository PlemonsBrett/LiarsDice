//
// Modern C++23 implementation of Player with Dependency Injection
// Created using industry best practices for testability and modularity
//

#include "liarsdice/core/player_impl.hpp"
#include "liarsdice/exceptions/game_exception.hpp"
#include <algorithm>
#include <format>
#include <ranges>

namespace liarsdice::core {

// Type aliases
using DiceFactory = interfaces::ServiceFactory<interfaces::IDice>;

PlayerImpl::PlayerImpl(int player_id,
                       std::unique_ptr<interfaces::IRandomGenerator> random_generator,
                       std::unique_ptr<DiceFactory> dice_factory)
    : player_id_(player_id), dice_{}, random_generator_(std::move(random_generator)),
      dice_factory_(std::move(dice_factory)) {

  if (!random_generator_ || !dice_factory_) {
    throw std::invalid_argument("All dependencies must be non-null");
  }

  if (player_id <= 0) {
    throw std::invalid_argument("Player ID must be positive");
  }
}

int PlayerImpl::get_id() const { return player_id_; }

void PlayerImpl::set_id(int player_id) {
  if (player_id <= 0) {
    throw std::invalid_argument("Player ID must be positive");
  }
  player_id_ = player_id;
}

size_t PlayerImpl::get_dice_count() const { return dice_.size(); }

void PlayerImpl::roll_dice() {
  for (auto &die : dice_) {
    if (die) {
      die->roll();
    }
  }
}

void PlayerImpl::add_die() {
  auto new_die = create_die();
  if (new_die) {
    dice_.push_back(std::move(new_die));
  } else {
    throw std::runtime_error("Failed to create new die");
  }
}

bool PlayerImpl::remove_die() {
  if (dice_.empty()) {
    return false;
  }

  dice_.pop_back();
  return true;
}

bool PlayerImpl::has_dice() const { return !dice_.empty(); }

std::vector<std::reference_wrapper<const interfaces::IDice>> PlayerImpl::get_dice() const {
  std::vector<std::reference_wrapper<const interfaces::IDice>> dice_refs;
  dice_refs.reserve(dice_.size());

  for (const auto &die : dice_) {
    if (die) {
      dice_refs.emplace_back(*die);
    }
  }

  return dice_refs;
}

size_t PlayerImpl::count_dice_with_value(unsigned int face_value) const {
  if (!is_valid_face_value(face_value)) {
    return 0;
  }

  auto count = std::ranges::count_if(
      dice_, [face_value](const auto &die) { return die && die->get_face_value() == face_value; });

  return static_cast<size_t>(count);
}

std::vector<unsigned int> PlayerImpl::get_dice_values() const {
  std::vector<unsigned int> values;
  values.reserve(dice_.size());

  for (const auto &die : dice_) {
    if (die) {
      values.push_back(die->get_face_value());
    }
  }

  return values;
}

bool PlayerImpl::is_active() const { return has_dice(); }

// Private methods
std::unique_ptr<interfaces::IDice> PlayerImpl::create_die() {
  try {
    void *raw_die = nullptr;
    raw_die = dice_factory_->create();
    auto die = std::unique_ptr<interfaces::IDice>(static_cast<interfaces::IDice *>(raw_die));

    // Initialize the die with a random roll
    if (die && random_generator_) {
      unsigned int initial_value = 0;
      initial_value = static_cast<unsigned int>(random_generator_->generate(
          static_cast<int>(kMinDiceValue), static_cast<int>(kMaxDiceValue)));
      die->set_face_value(initial_value);
    }

    return die;
  } catch (const std::exception &e) {
    throw std::runtime_error(
        std::format("Failed to create die for player {}: {}", player_id_, e.what()));
  }
}

constexpr bool PlayerImpl::is_valid_face_value(unsigned int face_value) const {
  return face_value >= kMinDiceValue && face_value <= kMaxDiceValue;
}

} // namespace liarsdice::core