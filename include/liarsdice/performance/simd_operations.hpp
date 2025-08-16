#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>
#include <xsimd/xsimd.hpp>

namespace liarsdice::performance {

  namespace xs = xsimd;

  /**
   * @brief SIMD-accelerated operations using xsimd
   *
   * Provides vectorized implementations of common operations
   * for maximum performance on modern CPUs with SIMD support.
   * Uses xsimd for vectorized operations with automatic dispatch
   * to the best available instruction set (SSE, AVX, AVX2, etc.)
   */
  template <typename T = float> class SimdOperations {
  public:
    using value_type = T;
    using batch_type = xsimd::batch<T>;
    static constexpr std::size_t simd_size = batch_type::size;

    /**
     * @brief SIMD-accelerated dot product
     * @param a First vector
     * @param b Second vector
     * @return Dot product result
     */
    static T dot_product(std::span<const T> a, std::span<const T> b) {
      if (a.size() != b.size()) {
        throw std::invalid_argument("Vectors must have same size");
      }

      const std::size_t size = a.size();
      const std::size_t simd_end = size - (size % simd_size);
      T result = 0;

      // Process aligned data in SIMD batches
      for (std::size_t i = 0; i < simd_end; i += simd_size) {
        auto va = batch_type::load_unaligned(&a[i]);
        auto vb = batch_type::load_unaligned(&b[i]);
        result += xsimd::reduce_add(va * vb);
      }

      // Process remaining elements
      for (std::size_t i = simd_end; i < size; ++i) {
        result += a[i] * b[i];
      }

      return result;
    }

    /**
     * @brief SIMD vector addition
     * @param a First vector
     * @param b Second vector
     * @param result Output vector
     */
    static void vector_add(std::span<const T> a, std::span<const T> b, std::span<T> result) {
      if (a.size() != b.size() || a.size() != result.size()) {
        throw std::invalid_argument("All vectors must have same size");
      }

      const std::size_t size = a.size();
      const std::size_t simd_end = size - (size % simd_size);

      // Process aligned data
      for (std::size_t i = 0; i < simd_end; i += simd_size) {
        auto va = batch_type::load_unaligned(&a[i]);
        auto vb = batch_type::load_unaligned(&b[i]);
        (va + vb).store_unaligned(&result[i]);
      }

      // Process remaining elements
      for (std::size_t i = simd_end; i < size; ++i) {
        result[i] = a[i] + b[i];
      }
    }

    /**
     * @brief SIMD scalar multiplication
     * @param scalar Scalar value
     * @param vec Input vector
     * @param result Output vector
     */
    static void scalar_multiply(T scalar, std::span<const T> vec, std::span<T> result) {
      if (vec.size() != result.size()) {
        throw std::invalid_argument("Vectors must have same size");
      }

      const std::size_t size = vec.size();
      const std::size_t simd_end = size - (size % simd_size);
      auto scalar_batch = batch_type(scalar);

      // SIMD portion
      for (std::size_t i = 0; i < simd_end; i += simd_size) {
        auto v = batch_type::load_unaligned(&vec[i]);
        (scalar_batch * v).store_unaligned(&result[i]);
      }

      // Scalar remainder
      for (std::size_t i = simd_end; i < size; ++i) {
        result[i] = scalar * vec[i];
      }
    }

    /**
     * @brief SIMD mean and variance calculation
     * @param data Input data
     * @return Pair of (mean, variance)
     */
    static std::pair<double, double> mean_variance(std::span<const T> data) {
      if (data.empty()) return {0.0, 0.0};

      const std::size_t n = data.size();
      const std::size_t simd_end = n - (n % simd_size);

      batch_type sum_vec = batch_type(T(0));
      batch_type sum_sq_vec = batch_type(T(0));

      for (std::size_t i = 0; i < simd_end; i += simd_size) {
        auto v = batch_type::load_unaligned(&data[i]);
        sum_vec += v;
        sum_sq_vec = xsimd::fma(v, v, sum_sq_vec);
      }

      T sum = xsimd::reduce_add(sum_vec);
      T sum_sq = xsimd::reduce_add(sum_sq_vec);

      // Process remaining elements
      for (std::size_t i = simd_end; i < n; ++i) {
        sum += data[i];
        sum_sq += data[i] * data[i];
      }

      double mean = static_cast<double>(sum) / n;
      double variance = (static_cast<double>(sum_sq) / n) - (mean * mean);

      // Use Bessel's correction for sample variance
      if (n > 1) {
        variance = variance * n / (n - 1);
      }

      return {mean, variance};
    }

    /**
     * @brief SIMD min/max finding
     * @param data Input data
     * @return Pair of (min, max)
     */
    static std::pair<T, T> minmax(std::span<const T> data) {
      if (data.empty()) {
        return {std::numeric_limits<T>::max(), std::numeric_limits<T>::lowest()};
      }

      const std::size_t n = data.size();
      const std::size_t simd_end = n - (n % simd_size);

      batch_type min_vec = batch_type(std::numeric_limits<T>::max());
      batch_type max_vec = batch_type(std::numeric_limits<T>::lowest());

      for (std::size_t i = 0; i < simd_end; i += simd_size) {
        auto v = batch_type::load_unaligned(&data[i]);
        min_vec = xsimd::min(min_vec, v);
        max_vec = xsimd::max(max_vec, v);
      }

      T min_val = xsimd::reduce_min(min_vec);
      T max_val = xsimd::reduce_max(max_vec);

      // Process remaining elements
      for (std::size_t i = simd_end; i < n; ++i) {
        min_val = std::min(min_val, data[i]);
        max_val = std::max(max_val, data[i]);
      }

      return {min_val, max_val};
    }

    /**
     * @brief SIMD element-wise absolute value
     * @param data Input data
     * @param result Output vector
     */
    static void abs(std::span<const T> data, std::span<T> result) {
      if (data.size() != result.size()) {
        throw std::invalid_argument("Vectors must have same size");
      }

      const std::size_t size = data.size();
      const std::size_t simd_end = size - (size % simd_size);

      for (std::size_t i = 0; i < simd_end; i += simd_size) {
        auto v = batch_type::load_unaligned(&data[i]);
        xsimd::abs(v).store_unaligned(&result[i]);
      }

      for (std::size_t i = simd_end; i < size; ++i) {
        result[i] = std::abs(data[i]);
      }
    }

    /**
     * @brief SIMD Manhattan distance
     * @param a First vector
     * @param b Second vector
     * @return L1 distance
     */
    static T manhattan_distance(std::span<const T> a, std::span<const T> b) {
      if (a.size() != b.size()) {
        throw std::invalid_argument("Vectors must have same size");
      }

      const std::size_t size = a.size();
      const std::size_t simd_end = size - (size % simd_size);

      batch_type sum_vec = batch_type(T(0));

      for (std::size_t i = 0; i < simd_end; i += simd_size) {
        auto va = batch_type::load_unaligned(&a[i]);
        auto vb = batch_type::load_unaligned(&b[i]);
        sum_vec += xsimd::abs(va - vb);
      }

      T result = xsimd::reduce_add(sum_vec);

      for (std::size_t i = simd_end; i < size; ++i) {
        result += std::abs(a[i] - b[i]);
      }

      return result;
    }

    /**
     * @brief SIMD-accelerated histogram computation
     * @param data Input data
     * @param bins Number of bins
     * @param min Minimum value
     * @param max Maximum value
     * @return Histogram counts
     */
    static std::vector<std::size_t> fast_histogram(std::span<const T> data, std::size_t bins, T min,
                                                   T max) {
      std::vector<std::size_t> hist(bins, 0);
      if (data.empty() || bins == 0 || min >= max) return hist;

      T range = max - min;
      T bin_width = range / bins;
      T inv_bin_width = T(1) / bin_width;

      // Process data
      for (const auto& value : data) {
        if (value >= min && value <= max) {
          std::size_t bin = static_cast<std::size_t>((value - min) * inv_bin_width);
          if (bin >= bins) bin = bins - 1;
          hist[bin]++;
        }
      }

      return hist;
    }

    /**
     * @brief SIMD matrix-vector multiplication
     * @param matrix Row-major matrix
     * @param vec Input vector
     * @param result Output vector
     * @param rows Number of rows
     * @param cols Number of columns
     */
    static void matrix_vector_multiply(const T* matrix, const T* vec, T* result, std::size_t rows,
                                       std::size_t cols) {
      const std::size_t simd_cols = cols - (cols % simd_size);

      for (std::size_t i = 0; i < rows; ++i) {
        batch_type sum_batch(T(0));
        const T* row = matrix + i * cols;

        // SIMD portion
        for (std::size_t j = 0; j < simd_cols; j += simd_size) {
          auto m = batch_type::load_unaligned(&row[j]);
          auto v = batch_type::load_unaligned(&vec[j]);
          sum_batch = xsimd::fma(m, v, sum_batch);
        }

        T sum = xsimd::reduce_add(sum_batch);

        // Scalar remainder
        for (std::size_t j = simd_cols; j < cols; ++j) {
          sum += row[j] * vec[j];
        }

        result[i] = sum;
      }
    }

    /**
     * @brief SIMD exponential moving average
     * @param data Input/output data
     * @param alpha Smoothing factor (0 < alpha < 1)
     */
    static void exponential_moving_average(std::span<T> data, T alpha) {
      if (data.empty() || alpha <= 0 || alpha >= 1) return;

      T beta = T(1) - alpha;
      T ema = data[0];

      for (std::size_t i = 1; i < data.size(); ++i) {
        ema = alpha * data[i] + beta * ema;
        data[i] = ema;
      }
    }
  };

  // Aligned allocator for SIMD operations
  template <typename T, std::size_t Align = xsimd::default_arch::alignment()>
  using aligned_allocator = xsimd::aligned_allocator<T, Align>;

  /**
   * @brief SIMD-optimized dice probability calculations
   */
  template <typename T = float> class SimdDiceProbability {
  public:
    using batch_type = xsimd::batch<T>;
    static constexpr std::size_t simd_size = batch_type::size;

    /**
     * @brief Batch calculate probabilities for multiple scenarios
     * @param total_dice Vector of total dice counts
     * @param k_values Vector of minimum dice requirements
     * @param face_values Vector of face values
     * @return Vector of probabilities
     */
    static std::vector<T> batch_probability(std::span<const int> total_dice,
                                            std::span<const int> k_values,
                                            std::span<const int> face_values) {
      if (total_dice.size() != k_values.size() || total_dice.size() != face_values.size()) {
        throw std::invalid_argument("Input vectors must have same size");
      }

      std::vector<T> results(total_dice.size());

      // Process in batches for better cache utilization
      constexpr std::size_t BATCH_SIZE = 64;
      for (std::size_t i = 0; i < total_dice.size(); i += BATCH_SIZE) {
        std::size_t batch_end = std::min(i + BATCH_SIZE, total_dice.size());

        for (std::size_t j = i; j < batch_end; ++j) {
          results[j] = calculate_probability(total_dice[j], k_values[j], face_values[j]);
        }
      }

      return results;
    }

  private:
    static T calculate_probability(int total, int k, int face) {
      if (k > total || k < 0) return T(0);
      if (k == 0) return T(1);

      T p = (face == 1) ? T(1.0 / 3.0) : T(1.0 / 6.0);
      T q = T(1) - p;
      T prob = T(0);

      // Use logarithms for numerical stability with large values
      if (total > 20) {
        for (int i = k; i <= total; ++i) {
          T log_term = log_binomial(total, i) + i * std::log(p) + (total - i) * std::log(q);
          prob += std::exp(log_term);
        }
      } else {
        // Direct calculation for small values
        for (int i = k; i <= total; ++i) {
          prob += binomial_coeff(total, i) * std::pow(p, i) * std::pow(q, total - i);
        }
      }

      return prob;
    }

    static T log_binomial(int n, int k) {
      if (k > n - k) k = n - k;
      if (k == 0) return T(0);

      T result = T(0);
      for (int i = 0; i < k; ++i) {
        result += std::log(T(n - i)) - std::log(T(i + 1));
      }
      return result;
    }

    static T binomial_coeff(int n, int k) {
      if (k > n - k) k = n - k;
      if (k == 0) return T(1);

      T result = T(1);
      for (int i = 0; i < k; ++i) {
        result *= T(n - i) / T(i + 1);
      }
      return result;
    }
  };

}  // namespace liarsdice::performance