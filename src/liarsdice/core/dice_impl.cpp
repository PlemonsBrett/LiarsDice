//
// Modern C++23 implementation of Dice with Dependency Injection
// Created using industry best practices for testability and modularity
//

#include "liarsdice/core/dice_impl.hpp"
#include "liarsdice/exceptions/game_exception.hpp"
#include <format>
#include <stdexcept>

namespace liarsdice::core {

DiceImpl::DiceImpl(std::unique_ptr<interfaces::IRandomGenerator> random_generator)
    : face_value_(kDefaultFaceValue), random_generator_(std::move(random_generator)) {

  if (!random_generator_) {
    throw std::invalid_argument("Random generator must be non-null");
  }

  // Perform initial roll
  roll();
}

DiceImpl::DiceImpl(std::unique_ptr<interfaces::IRandomGenerator> random_generator,
                   unsigned int initial_value)
    : face_value_(kDefaultFaceValue), random_generator_(std::move(random_generator)) {

  if (!random_generator_) {
    throw std::invalid_argument("Random generator must be non-null");
  }

  validate_and_set_face_value(initial_value);
}

void DiceImpl::roll() {
  if (!random_generator_) {
    throw std::runtime_error("Cannot roll dice: random generator is null");
  }

  try {
    int rolled_value = random_generator_->generate(static_cast<int>(kMinFaceValue),
                                                   static_cast<int>(kMaxFaceValue));

    face_value_ = static_cast<unsigned int>(rolled_value);

    // Additional validation to ensure generator behaves correctly
    if (!is_valid_face_value(face_value_)) {
      throw std::runtime_error(
          std::format("Random generator produced invalid value: {}", face_value_));
    }
  } catch (const std::exception &e) {
    throw std::runtime_error(std::format("Failed to roll dice: {}", e.what()));
  }
}

unsigned int DiceImpl::get_face_value() const { return face_value_; }

void DiceImpl::set_face_value(unsigned int value) { validate_and_set_face_value(value); }

bool DiceImpl::is_valid_face_value(unsigned int value) const {
  return value >= kMinFaceValue && value <= kMaxFaceValue;
}

std::unique_ptr<interfaces::IDice> DiceImpl::clone() const {
  // Note: Clone creates a dice with the same face value but no random generator
  // This is because random generators are typically not cloneable and should be
  // injected fresh through the DI container when needed
  throw std::runtime_error("Clone operation not supported for DiceImpl. "
                           "Use ServiceContainer to create new dice instances instead.");
}

// Private methods
void DiceImpl::validate_and_set_face_value(unsigned int value) {
  if (!is_valid_face_value(value)) {
    throw std::invalid_argument(std::format("Invalid face value: {}. Must be between {} and {}",
                                            value, kMinFaceValue, kMaxFaceValue));
  }

  face_value_ = value;
}

} // namespace liarsdice::core