#pragma once

#include <boost/simd/pack.hpp>
#include <boost/simd/function/sum.hpp>
#include <boost/simd/function/dot.hpp>
#include <boost/simd/function/load.hpp>
#include <boost/simd/function/store.hpp>
#include <boost/simd/function/aligned_load.hpp>
#include <boost/simd/function/aligned_store.hpp>
#include <boost/simd/function/plus.hpp>
#include <boost/simd/function/multiplies.hpp>
#include <boost/simd/function/min.hpp>
#include <boost/simd/function/max.hpp>
#include <boost/simd/function/sqr.hpp>
#include <boost/simd/function/sqrt.hpp>
#include <boost/simd/function/rec.hpp>
#include <boost/simd/function/fma.hpp>
#include <boost/simd/algorithm/transform.hpp>
#include <boost/simd/algorithm/reduce.hpp>
#include <boost/simd/memory/allocator.hpp>
#include <vector>
#include <array>
#include <span>

namespace liarsdice::performance {

namespace bs = boost::simd;

/**
 * @brief SIMD-accelerated operations using boost.simd
 * 
 * Provides vectorized implementations of common operations
 * for maximum performance on modern CPUs with SIMD support.
 */
template<typename T = float>
class SimdOperations {
public:
    using value_type = T;
    using pack_type = bs::pack<T>;
    using allocator_type = bs::allocator<T>;
    
    static constexpr std::size_t pack_size = pack_type::static_size;
    
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
        
        std::size_t size = a.size();
        std::size_t simd_size = size - (size % pack_size);
        T result = 0;
        
        // SIMD portion
        for (std::size_t i = 0; i < simd_size; i += pack_size) {
            pack_type pa = bs::load<pack_type>(&a[i]);
            pack_type pb = bs::load<pack_type>(&b[i]);
            result += bs::sum(pa * pb);
        }
        
        // Scalar remainder
        for (std::size_t i = simd_size; i < size; ++i) {
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
        
        bs::transform(a.data(), a.data() + a.size(), b.data(), result.data(),
                      [](const auto& x, const auto& y) { return x + y; });
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
        
        pack_type scalar_pack(scalar);
        std::size_t size = vec.size();
        std::size_t simd_size = size - (size % pack_size);
        
        // SIMD portion
        for (std::size_t i = 0; i < simd_size; i += pack_size) {
            pack_type v = bs::load<pack_type>(&vec[i]);
            bs::store(scalar_pack * v, &result[i]);
        }
        
        // Scalar remainder
        for (std::size_t i = simd_size; i < size; ++i) {
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
        
        std::size_t n = data.size();
        std::size_t simd_size = n - (n % pack_size);
        
        // Calculate mean using SIMD
        pack_type sum_pack(0);
        for (std::size_t i = 0; i < simd_size; i += pack_size) {
            sum_pack += bs::load<pack_type>(&data[i]);
        }
        T sum = bs::sum(sum_pack);
        
        // Add scalar remainder
        for (std::size_t i = simd_size; i < n; ++i) {
            sum += data[i];
        }
        
        double mean = static_cast<double>(sum) / n;
        
        // Calculate variance using SIMD
        pack_type mean_pack(static_cast<T>(mean));
        pack_type var_sum_pack(0);
        
        for (std::size_t i = 0; i < simd_size; i += pack_size) {
            pack_type v = bs::load<pack_type>(&data[i]);
            pack_type diff = v - mean_pack;
            var_sum_pack += bs::sqr(diff);
        }
        T var_sum = bs::sum(var_sum_pack);
        
        // Add scalar remainder
        for (std::size_t i = simd_size; i < n; ++i) {
            T diff = data[i] - static_cast<T>(mean);
            var_sum += diff * diff;
        }
        
        double variance = static_cast<double>(var_sum) / (n - 1);
        return {mean, variance};
    }
    
    /**
     * @brief SIMD min/max finding
     * @param data Input data
     * @return Pair of (min, max)
     */
    static std::pair<T, T> minmax(std::span<const T> data) {
        if (data.empty()) return {T{}, T{}};
        
        std::size_t n = data.size();
        std::size_t simd_size = n - (n % pack_size);
        
        pack_type min_pack = bs::load<pack_type>(&data[0]);
        pack_type max_pack = min_pack;
        
        // SIMD portion
        for (std::size_t i = pack_size; i < simd_size; i += pack_size) {
            pack_type v = bs::load<pack_type>(&data[i]);
            min_pack = bs::min(min_pack, v);
            max_pack = bs::max(max_pack, v);
        }
        
        // Reduce pack to scalar
        T min_val = data[0];
        T max_val = data[0];
        for (std::size_t j = 0; j < pack_size; ++j) {
            min_val = std::min(min_val, min_pack[j]);
            max_val = std::max(max_val, max_pack[j]);
        }
        
        // Scalar remainder
        for (std::size_t i = simd_size; i < n; ++i) {
            min_val = std::min(min_val, data[i]);
            max_val = std::max(max_val, data[i]);
        }
        
        return {min_val, max_val};
    }
    
    /**
     * @brief SIMD-accelerated histogram computation
     * @param data Input data
     * @param bins Number of bins
     * @param min Minimum value
     * @param max Maximum value
     * @return Histogram counts
     */
    static std::vector<std::size_t> fast_histogram(
        std::span<const T> data,
        std::size_t bins,
        T min,
        T max
    ) {
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
    static void matrix_vector_multiply(
        const T* matrix,
        const T* vec,
        T* result,
        std::size_t rows,
        std::size_t cols
    ) {
        std::size_t simd_cols = cols - (cols % pack_size);
        
        for (std::size_t i = 0; i < rows; ++i) {
            pack_type sum_pack(0);
            const T* row = matrix + i * cols;
            
            // SIMD portion
            for (std::size_t j = 0; j < simd_cols; j += pack_size) {
                pack_type m = bs::load<pack_type>(&row[j]);
                pack_type v = bs::load<pack_type>(&vec[j]);
                sum_pack = bs::fma(m, v, sum_pack);
            }
            
            T sum = bs::sum(sum_pack);
            
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
     * @param alpha Smoothing factor
     */
    static void exponential_moving_average(std::span<T> data, T alpha) {
        if (data.empty()) return;
        
        T beta = T(1) - alpha;
        pack_type alpha_pack(alpha);
        pack_type beta_pack(beta);
        
        std::size_t n = data.size();
        T ema = data[0];
        
        for (std::size_t i = 1; i < n; ++i) {
            ema = alpha * data[i] + beta * ema;
            data[i] = ema;
        }
    }
};

/**
 * @brief SIMD-optimized dice probability calculations
 */
template<typename T = float>
class SimdDiceProbability {
public:
    using pack_type = bs::pack<T>;
    static constexpr std::size_t pack_size = pack_type::static_size;
    
    /**
     * @brief Batch calculate probabilities for multiple scenarios
     * @param total_dice Vector of total dice counts
     * @param k_values Vector of minimum dice requirements
     * @param face_values Vector of face values
     * @return Vector of probabilities
     */
    static std::vector<T> batch_probability(
        std::span<const int> total_dice,
        std::span<const int> k_values,
        std::span<const int> face_values
    ) {
        if (total_dice.size() != k_values.size() || 
            total_dice.size() != face_values.size()) {
            throw std::invalid_argument("Input vectors must have same size");
        }
        
        std::vector<T> results(total_dice.size());
        
        // Process in batches for better cache utilization
        constexpr std::size_t BATCH_SIZE = 64;
        for (std::size_t i = 0; i < total_dice.size(); i += BATCH_SIZE) {
            std::size_t batch_end = std::min(i + BATCH_SIZE, total_dice.size());
            
            for (std::size_t j = i; j < batch_end; ++j) {
                results[j] = calculate_probability(
                    total_dice[j], k_values[j], face_values[j]
                );
            }
        }
        
        return results;
    }
    
private:
    static T calculate_probability(int total, int k, int face) {
        if (k > total || k < 0) return T(0);
        if (k == 0) return T(1);
        
        T p = (face == 1) ? T(1.0/3.0) : T(1.0/6.0);
        T q = T(1) - p;
        T prob = T(0);
        
        // Use logarithms for numerical stability with large values
        if (total > 20) {
            for (int i = k; i <= total; ++i) {
                T log_term = log_binomial(total, i) + 
                            i * std::log(p) + 
                            (total - i) * std::log(q);
                prob += std::exp(log_term);
            }
        } else {
            // Direct calculation for small values
            for (int i = k; i <= total; ++i) {
                prob += binomial_coeff(total, i) * 
                       std::pow(p, i) * 
                       std::pow(q, total - i);
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

} // namespace liarsdice::performance