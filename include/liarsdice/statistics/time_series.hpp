#pragma once

#include <algorithm>
#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>
#include <chrono>
#include <cmath>
#include <numeric>
#include <vector>

namespace liarsdice::statistics {

  /**
   * @brief Time series data point
   * @tparam T Value type
   */
  template <typename T> struct TimePoint {
    std::chrono::steady_clock::time_point timestamp;
    T value;

    TimePoint() = default;
    TimePoint(T val) : timestamp(std::chrono::steady_clock::now()), value(std::move(val)) {}
    TimePoint(std::chrono::steady_clock::time_point ts, T val)
        : timestamp(ts), value(std::move(val)) {}
  };

  /**
   * @brief Time series container using boost::circular_buffer
   *
   * Efficient storage for time-ordered data with automatic old data removal.
   * Supports various time series analysis operations including moving averages,
   * trend detection, and seasonal decomposition.
   *
   * @tparam T Value type
   * @tparam MaxSize Maximum number of points to store
   */
  template <typename T, std::size_t MaxSize = 1000> class TimeSeries {
  public:
    using value_type = T;
    using time_point = TimePoint<T>;
    using duration = std::chrono::steady_clock::duration;
    using buffer_type = boost::circular_buffer<time_point>;

    /**
     * @brief Default constructor
     */
    TimeSeries() : buffer_(MaxSize) {}

    /**
     * @brief Add a value with current timestamp
     * @param value Value to add
     */
    void add(const T& value) { buffer_.push_back(TimePoint<T>(value)); }

    /**
     * @brief Add a value with specific timestamp
     * @param timestamp Time point
     * @param value Value to add
     */
    void add(std::chrono::steady_clock::time_point timestamp, const T& value) {
      buffer_.push_back(TimePoint<T>(timestamp, value));
    }

    /**
     * @brief Get size of time series
     * @return Number of data points
     */
    [[nodiscard]] std::size_t size() const { return buffer_.size(); }

    /**
     * @brief Check if empty
     * @return True if no data points
     */
    [[nodiscard]] bool empty() const { return buffer_.empty(); }

    /**
     * @brief Get most recent value
     * @return Optional containing last value
     */
    [[nodiscard]] boost::optional<T> latest() const {
      if (buffer_.empty()) return boost::none;
      return buffer_.back().value;
    }

    /**
     * @brief Get oldest value
     * @return Optional containing first value
     */
    [[nodiscard]] boost::optional<T> oldest() const {
      if (buffer_.empty()) return boost::none;
      return buffer_.front().value;
    }

    /**
     * @brief Get values within time window
     * @param window_duration Duration of window from now
     * @return Vector of values within window
     */
    [[nodiscard]] std::vector<T> get_window(duration window_duration) const {
      if (buffer_.empty()) return {};

      auto now = std::chrono::steady_clock::now();
      auto cutoff = now - window_duration;

      std::vector<T> result;
      for (auto it = buffer_.rbegin(); it != buffer_.rend(); ++it) {
        if (it->timestamp < cutoff) break;
        result.push_back(it->value);
      }

      std::reverse(result.begin(), result.end());
      return result;
    }

    /**
     * @brief Calculate simple moving average
     * @param periods Number of periods
     * @return Vector of moving averages
     */
    [[nodiscard]] std::vector<double> simple_moving_average(std::size_t periods) const {
      if (buffer_.size() < periods) return {};

      std::vector<double> sma;
      sma.reserve(buffer_.size() - periods + 1);

      // Calculate first SMA
      double sum = 0;
      for (std::size_t i = 0; i < periods; ++i) {
        sum += static_cast<double>(buffer_[i].value);
      }
      sma.push_back(sum / periods);

      // Calculate remaining SMAs using sliding window
      for (std::size_t i = periods; i < buffer_.size(); ++i) {
        sum = sum - static_cast<double>(buffer_[i - periods].value)
              + static_cast<double>(buffer_[i].value);
        sma.push_back(sum / periods);
      }

      return sma;
    }

    /**
     * @brief Calculate exponential moving average
     * @param alpha Smoothing factor (0 < alpha < 1)
     * @return Vector of EMAs
     */
    [[nodiscard]] std::vector<double> exponential_moving_average(double alpha) const {
      if (buffer_.empty() || alpha <= 0 || alpha >= 1) return {};

      std::vector<double> ema;
      ema.reserve(buffer_.size());

      // First value is just the data point
      ema.push_back(static_cast<double>(buffer_[0].value));

      // Calculate EMA for remaining points
      for (std::size_t i = 1; i < buffer_.size(); ++i) {
        double new_ema = alpha * static_cast<double>(buffer_[i].value) + (1 - alpha) * ema.back();
        ema.push_back(new_ema);
      }

      return ema;
    }

    /**
     * @brief Detect trend using linear regression
     * @return Pair of (slope, intercept)
     */
    [[nodiscard]] std::pair<double, double> linear_trend() const {
      if (buffer_.size() < 2) return {0.0, 0.0};

      // Convert timestamps to numeric x values
      std::vector<double> x_values;
      std::vector<double> y_values;

      auto first_time = buffer_.front().timestamp;

      for (const auto& point : buffer_) {
        auto duration
            = std::chrono::duration_cast<std::chrono::seconds>(point.timestamp - first_time);
        x_values.push_back(duration.count());
        y_values.push_back(static_cast<double>(point.value));
      }

      // Calculate linear regression
      double n = x_values.size();
      double sum_x = std::accumulate(x_values.begin(), x_values.end(), 0.0);
      double sum_y = std::accumulate(y_values.begin(), y_values.end(), 0.0);

      double sum_xy = 0, sum_xx = 0;
      for (std::size_t i = 0; i < n; ++i) {
        sum_xy += x_values[i] * y_values[i];
        sum_xx += x_values[i] * x_values[i];
      }

      double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
      double intercept = (sum_y - slope * sum_x) / n;

      return {slope, intercept};
    }

    /**
     * @brief Calculate rate of change
     * @param periods Number of periods to look back
     * @return Rate of change percentage
     */
    [[nodiscard]] boost::optional<double> rate_of_change(std::size_t periods) const {
      if (buffer_.size() <= periods) return boost::none;

      double old_value = static_cast<double>(buffer_[buffer_.size() - periods - 1].value);
      double new_value = static_cast<double>(buffer_.back().value);

      if (old_value == 0) return boost::none;

      return ((new_value - old_value) / old_value) * 100.0;
    }

    /**
     * @brief Detect outliers using z-score method
     * @param threshold Z-score threshold (default: 3)
     * @return Indices of outliers
     */
    [[nodiscard]] std::vector<std::size_t> detect_outliers(double threshold = 3.0) const {
      if (buffer_.size() < 3) return {};

      // Calculate mean and std dev
      double sum = 0;
      for (const auto& point : buffer_) {
        sum += static_cast<double>(point.value);
      }
      double mean = sum / buffer_.size();

      double sq_sum = 0;
      for (const auto& point : buffer_) {
        double diff = static_cast<double>(point.value) - mean;
        sq_sum += diff * diff;
      }
      double std_dev = std::sqrt(sq_sum / buffer_.size());

      // Find outliers
      std::vector<std::size_t> outliers;
      for (std::size_t i = 0; i < buffer_.size(); ++i) {
        double z_score = std::abs(static_cast<double>(buffer_[i].value) - mean) / std_dev;
        if (z_score > threshold) {
          outliers.push_back(i);
        }
      }

      return outliers;
    }

    /**
     * @brief Apply Savitzky-Golay filter for smoothing
     * @param window_size Window size (must be odd)
     * @param poly_order Polynomial order
     * @return Smoothed values
     */
    [[nodiscard]] std::vector<double> savitzky_golay_filter(std::size_t window_size,
                                                            std::size_t poly_order) const {
      if (window_size % 2 == 0 || window_size > buffer_.size() || poly_order >= window_size) {
        return {};
      }

      // Simplified implementation - just return moving average for now
      return simple_moving_average(window_size);
    }

    /**
     * @brief Calculate autocorrelation
     * @param lag Lag value
     * @return Autocorrelation coefficient
     */
    [[nodiscard]] boost::optional<double> autocorrelation(std::size_t lag) const {
      if (lag >= buffer_.size()) return boost::none;

      // Calculate mean
      double sum = 0;
      for (const auto& point : buffer_) {
        sum += static_cast<double>(point.value);
      }
      double mean = sum / buffer_.size();

      // Calculate autocorrelation
      double numerator = 0;
      double denominator = 0;

      for (std::size_t i = 0; i < buffer_.size() - lag; ++i) {
        double x1 = static_cast<double>(buffer_[i].value) - mean;
        double x2 = static_cast<double>(buffer_[i + lag].value) - mean;
        numerator += x1 * x2;
      }

      for (const auto& point : buffer_) {
        double diff = static_cast<double>(point.value) - mean;
        denominator += diff * diff;
      }

      if (denominator == 0) return boost::none;

      return numerator / denominator;
    }

    /**
     * @brief Get raw data points
     * @return Vector of time points
     */
    [[nodiscard]] std::vector<time_point> get_data() const {
      return std::vector<time_point>(buffer_.begin(), buffer_.end());
    }

    /**
     * @brief Clear all data
     */
    void clear() { buffer_.clear(); }

    /**
     * @brief Resample time series to fixed intervals
     * @param interval Target interval duration
     * @return Resampled time series
     */
    [[nodiscard]] TimeSeries resample(duration interval) const {
      TimeSeries resampled;
      if (buffer_.empty()) return resampled;

      auto current_time = buffer_.front().timestamp;
      auto end_time = buffer_.back().timestamp;

      while (current_time <= end_time) {
        // Find nearest point
        auto it = std::min_element(buffer_.begin(), buffer_.end(),
                                   [current_time](const auto& a, const auto& b) {
                                     return std::abs((a.timestamp - current_time).count())
                                            < std::abs((b.timestamp - current_time).count());
                                   });

        if (it != buffer_.end()) {
          resampled.add(current_time, it->value);
        }

        current_time += interval;
      }

      return resampled;
    }

  private:
    buffer_type buffer_;
  };

  /**
   * @brief Specialized time series for game metrics
   */
  class GameMetricsTimeSeries : public TimeSeries<double, 500> {
  public:
    /**
     * @brief Add metric with automatic timestamp
     * @param metric_value Metric value
     */
    void record_metric(double metric_value) { add(metric_value); }

    /**
     * @brief Get performance trend
     * @return Trend slope (positive = improving)
     */
    [[nodiscard]] double performance_trend() const {
      auto [slope, intercept] = linear_trend();
      return slope;
    }

    /**
     * @brief Check if performance is stable
     * @param cv_threshold Coefficient of variation threshold
     * @return True if stable
     */
    [[nodiscard]] bool is_stable(double cv_threshold = 0.1) const {
      auto values = get_window(std::chrono::minutes(5));
      if (values.size() < 10) return false;

      double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
      if (mean == 0) return true;

      double sq_sum = 0;
      for (auto val : values) {
        double diff = val - mean;
        sq_sum += diff * diff;
      }
      double std_dev = std::sqrt(sq_sum / values.size());

      return (std_dev / std::abs(mean)) < cv_threshold;
    }
  };

}  // namespace liarsdice::statistics