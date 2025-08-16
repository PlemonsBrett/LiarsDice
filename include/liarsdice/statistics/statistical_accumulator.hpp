#pragma once

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/kurtosis.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/moment.hpp>
#include <boost/accumulators/statistics/rolling_mean.hpp>
#include <boost/accumulators/statistics/rolling_variance.hpp>
#include <boost/accumulators/statistics/skewness.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <cmath>

namespace liarsdice::statistics {

  namespace acc = boost::accumulators;

  /**
   * @brief Comprehensive statistical accumulator for running statistics
   *
   * Uses boost::accumulators to efficiently calculate various statistics
   * in a single pass. Supports both complete and rolling window statistics.
   *
   * @tparam T Value type (must be numeric)
   * @tparam WindowSize Size of rolling window for windowed statistics
   */
  template <typename T = double, std::size_t WindowSize = 100> class StatisticalAccumulator {
  public:
    using value_type = T;

    // Complete statistics accumulator
    using accumulator_set = acc::accumulator_set<
        T, acc::stats<acc::tag::count, acc::tag::mean, acc::tag::variance, acc::tag::min,
                      acc::tag::max, acc::tag::median, acc::tag::skewness, acc::tag::kurtosis,
                      acc::tag::moment<2>, acc::tag::moment<3>, acc::tag::moment<4> > >;

    // Rolling window accumulator
    using rolling_accumulator_set
        = acc::accumulator_set<T, acc::stats<acc::tag::rolling_mean, acc::tag::rolling_variance> >;

    /**
     * @brief Default constructor
     */
    StatisticalAccumulator() : rolling_acc_(acc::tag::rolling_window::window_size = WindowSize) {}

    /**
     * @brief Add a value to the accumulator
     * @param value Value to accumulate
     */
    void add(T value) {
      acc_(value);
      rolling_acc_(value);

      // Track sum of squares for RMS calculation
      sum_of_squares_ += value * value;
    }

    /**
     * @brief Add multiple values
     * @tparam InputIt Input iterator type
     * @param first Beginning of range
     * @param last End of range
     */
    template <typename InputIt> void add_range(InputIt first, InputIt last) {
      for (auto it = first; it != last; ++it) {
        add(*it);
      }
    }

    /**
     * @brief Get number of accumulated values
     * @return Count of values
     */
    [[nodiscard]] std::size_t count() const { return acc::count(acc_); }

    /**
     * @brief Get mean of all values
     * @return Mean value
     */
    [[nodiscard]] T mean() const { return acc::mean(acc_); }

    /**
     * @brief Get variance of all values
     * @return Variance
     */
    [[nodiscard]] T variance() const { return acc::variance(acc_); }

    /**
     * @brief Get standard deviation
     * @return Standard deviation
     */
    [[nodiscard]] T standard_deviation() const { return std::sqrt(variance()); }

    /**
     * @brief Get minimum value
     * @return Minimum value seen
     */
    [[nodiscard]] T min() const { return acc::min(acc_); }

    /**
     * @brief Get maximum value
     * @return Maximum value seen
     */
    [[nodiscard]] T max() const { return acc::max(acc_); }

    /**
     * @brief Get range (max - min)
     * @return Range of values
     */
    [[nodiscard]] T range() const { return max() - min(); }

    /**
     * @brief Get median value
     * @return Median
     */
    [[nodiscard]] T median() const { return acc::median(acc_); }

    /**
     * @brief Get skewness
     * @return Skewness measure
     */
    [[nodiscard]] T skewness() const { return acc::skewness(acc_); }

    /**
     * @brief Get kurtosis
     * @return Kurtosis measure
     */
    [[nodiscard]] T kurtosis() const { return acc::kurtosis(acc_); }

    /**
     * @brief Get coefficient of variation (CV)
     * @return CV = std_dev / mean
     */
    [[nodiscard]] T coefficient_of_variation() const {
      T m = mean();
      return m != 0 ? standard_deviation() / std::abs(m) : 0;
    }

    /**
     * @brief Get root mean square (RMS)
     * @return RMS value
     */
    [[nodiscard]] T rms() const {
      std::size_t n = count();
      return n > 0 ? std::sqrt(sum_of_squares_ / n) : 0;
    }

    /**
     * @brief Get nth central moment
     * @tparam N Moment order
     * @return Nth central moment
     */
    template <int N> [[nodiscard]] T moment() const { return acc::moment<N>(acc_); }

    /**
     * @brief Get rolling mean over window
     * @return Mean of last WindowSize values
     */
    [[nodiscard]] T rolling_mean() const { return acc::rolling_mean(rolling_acc_); }

    /**
     * @brief Get rolling variance over window
     * @return Variance of last WindowSize values
     */
    [[nodiscard]] T rolling_variance() const { return acc::rolling_variance(rolling_acc_); }

    /**
     * @brief Get rolling standard deviation
     * @return Standard deviation of last WindowSize values
     */
    [[nodiscard]] T rolling_std_dev() const { return std::sqrt(rolling_variance()); }

    /**
     * @brief Get comprehensive statistics summary
     * @return Struct containing all statistics
     */
    struct Statistics {
      std::size_t count;
      T mean;
      T std_dev;
      T min;
      T max;
      T range;
      T median;
      T skewness;
      T kurtosis;
      T cv;  // coefficient of variation
      T rms;
      T rolling_mean;
      T rolling_std_dev;
    };

    [[nodiscard]] Statistics get_statistics() const {
      return Statistics{
          count(), mean(),         standard_deviation(), min(),      max(),
          range(), median(),       skewness(),           kurtosis(), coefficient_of_variation(),
          rms(),   rolling_mean(), rolling_std_dev()};
    }

    /**
     * @brief Check if distribution is approximately normal
     * @param alpha Significance level (default: 0.05)
     * @return True if skewness and kurtosis suggest normality
     */
    [[nodiscard]] bool is_normal_distributed(double alpha = 0.05) const {
      // Simple normality check based on skewness and excess kurtosis
      // For normal distribution: skewness ≈ 0, excess kurtosis ≈ 0
      T skew = std::abs(skewness());
      T excess_kurt = std::abs(kurtosis() - 3.0);  // kurtosis - 3 for excess

      // Rule of thumb thresholds
      double skew_threshold = 2.0;
      double kurt_threshold = 7.0;

      return skew < skew_threshold && excess_kurt < kurt_threshold;
    }

    /**
     * @brief Reset accumulator to initial state
     */
    void reset() {
      acc_ = accumulator_set();
      rolling_acc_ = rolling_accumulator_set(acc::tag::rolling_window::window_size = WindowSize);
      sum_of_squares_ = 0;
    }

  private:
    accumulator_set acc_;
    rolling_accumulator_set rolling_acc_;
    T sum_of_squares_ = 0;
  };

  /**
   * @brief Specialized accumulator for dice roll statistics
   */
  class DiceRollAccumulator : public StatisticalAccumulator<double, 20> {
  public:
    /**
     * @brief Add dice roll outcome
     * @param face_value Face value (1-6)
     * @param count Number of dice showing this face
     */
    void add_roll(unsigned int face_value, unsigned int count) {
      if (face_value < 1 || face_value > 6) {
        throw std::invalid_argument("Invalid dice face value");
      }

      // Add to general statistics
      add(static_cast<double>(count));

      // Track face frequencies
      face_counts_[face_value - 1] += count;
      total_rolls_++;
    }

    /**
     * @brief Get probability of specific face value
     * @param face Face value (1-6)
     * @return Empirical probability
     */
    [[nodiscard]] double face_probability(unsigned int face) const {
      if (face < 1 || face > 6) return 0.0;

      // Calculate total dice count
      unsigned int total_dice = 0;
      for (auto count : face_counts_) {
        total_dice += count;
      }

      if (total_dice == 0) return 0.0;

      return static_cast<double>(face_counts_[face - 1]) / total_dice;
    }

    /**
     * @brief Get chi-square test statistic for uniformity
     * @return Chi-square value
     */
    [[nodiscard]] double chi_square_uniformity() const {
      // Calculate total dice count
      unsigned int total_dice = 0;
      for (auto count : face_counts_) {
        total_dice += count;
      }

      if (total_dice == 0) return 0.0;

      double expected = total_dice / 6.0;  // Expected count per face for uniform distribution
      double chi_square = 0.0;

      for (auto count : face_counts_) {
        double diff = count - expected;
        chi_square += (diff * diff) / expected;
      }

      return chi_square;
    }

  private:
    std::array<unsigned int, 6> face_counts_ = {0, 0, 0, 0, 0, 0};
    unsigned int total_rolls_ = 0;
  };

}  // namespace liarsdice::statistics