#pragma once

#include "../interfaces/i_random_generator.hpp"
#include <algorithm>
#include <random>

namespace liarsdice::adapters {

/**
 * @brief Standard random number generator implementation
 *
 * Uses std::mt19937 for high-quality random number generation.
 * Thread-safe and suitable for game use.
 */
class StandardRandomGenerator : public interfaces::IRandomGenerator {
private:
  std::random_device rd_;
  mutable std::mt19937 generator_;

public:
  /**
   * @brief Construct with random seed
   */
  StandardRandomGenerator() : generator_(rd_()) {}

  /**
   * @brief Construct with specific seed
   */
  explicit StandardRandomGenerator(unsigned int seed) : generator_(seed) {}

  int generate(int min, int max) override {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(generator_);
  }

  void seed(unsigned int seed) override { generator_.seed(seed); }

  bool generate_bool() override {
    std::uniform_int_distribution<int> dist(0, 1);
    return dist(generator_) == 1;
  }

  double generate_normalized() override {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(generator_);
  }
};

/**
 * @brief Deterministic random generator for testing
 *
 * Returns predictable sequences for reproducible tests.
 */
class MockRandomGenerator : public interfaces::IRandomGenerator {
private:
  mutable size_t current_index_ = 0;
  std::vector<int> predetermined_values_;
  int default_value_;

public:
  /**
   * @brief Construct with predetermined sequence
   */
  explicit MockRandomGenerator(std::vector<int> values, int default_val = 1)
      : predetermined_values_(std::move(values)), default_value_(default_val) {}

  /**
   * @brief Construct with single repeating value
   */
  explicit MockRandomGenerator(int value = 1) : default_value_(value) {}

  int generate(int min, int max) override {
    if (!predetermined_values_.empty()) {
      int value = predetermined_values_[current_index_ % predetermined_values_.size()];
      ++current_index_;
      return std::clamp(value, min, max);
    }
    return std::clamp(default_value_, min, max);
  }

  void seed(unsigned int /* seed */) override {
    // No-op for deterministic generator
    current_index_ = 0;
  }

  bool generate_bool() override { return generate(0, 1) == 1; }

  double generate_normalized() override { return static_cast<double>(generate(0, 1000)) / 1000.0; }

  /**
   * @brief Set new predetermined values
   */
  void set_values(std::vector<int> values) {
    predetermined_values_ = std::move(values);
    current_index_ = 0;
  }

  /**
   * @brief Reset to beginning of sequence
   */
  void reset() { current_index_ = 0; }
};

} // namespace liarsdice::adapters
