#pragma once

#include <boost/histogram.hpp>
#include <boost/histogram/axis/regular.hpp>
#include <boost/histogram/axis/integer.hpp>
#include <boost/histogram/axis/category.hpp>
#include <boost/histogram/algorithm/sum.hpp>
#include <boost/histogram/algorithm/reduce.hpp>
#include <boost/histogram/accumulators/mean.hpp>
#include <boost/histogram/make_histogram.hpp>
#include <vector>
#include <string>
#include <algorithm>

namespace liarsdice::statistics {

namespace bh = boost::histogram;

/**
 * @brief Flexible histogram template using boost::histogram
 * 
 * Supports various axis types and provides statistical analysis
 * of binned data. Optimized for game analytics and AI pattern recognition.
 * 
 * @tparam T Value type for the axis
 * @tparam Storage Storage backend (default: std::vector)
 */
template<typename T = double>
class Histogram {
public:
    using value_type = T;
    using axis_type = typename std::conditional_t<
        std::is_integral_v<T>,
        bh::axis::integer<T>,
        bh::axis::regular<T>
    >;
    
    using histogram_type = decltype(bh::make_histogram(std::declval<axis_type>()));
    
    /**
     * @brief Construct histogram with regular bins
     * @param bins Number of bins
     * @param min Minimum value
     * @param max Maximum value
     */
    Histogram(std::size_t bins, T min, T max) {
        if constexpr (std::is_integral_v<T>) {
            hist_ = bh::make_histogram(
                bh::axis::integer<T>(min, max, "value"));
        } else {
            hist_ = bh::make_histogram(
                bh::axis::regular<T>(bins, min, max, "value"));
        }
    }
    
    /**
     * @brief Add a value to the histogram
     * @param value Value to add
     * @param weight Optional weight (default: 1)
     */
    void add(T value, double weight = 1.0) {
        hist_(value, bh::weight(weight));
        total_count_ += weight;
        
        // Track min/max for statistics
        if (first_value_) {
            min_value_ = max_value_ = value;
            first_value_ = false;
        } else {
            min_value_ = std::min(min_value_, value);
            max_value_ = std::max(max_value_, value);
        }
    }
    
    /**
     * @brief Add multiple values
     * @tparam InputIt Input iterator type
     * @param first Beginning of range
     * @param last End of range
     */
    template<typename InputIt>
    void add_range(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            add(*it);
        }
    }
    
    /**
     * @brief Get bin count for a specific value
     * @param value Value to query
     * @return Count in the bin containing value
     */
    [[nodiscard]] double count_at(T value) const {
        auto idx = hist_.axis().index(value);
        if (idx < hist_.size()) {
            return hist_[idx];
        }
        return 0.0;
    }
    
    /**
     * @brief Get total count across all bins
     * @return Sum of all bin counts
     */
    [[nodiscard]] double total_count() const {
        return total_count_;
    }
    
    /**
     * @brief Get number of bins
     * @return Number of histogram bins
     */
    [[nodiscard]] std::size_t bin_count() const {
        return hist_.axis().size();
    }
    
    /**
     * @brief Get bin edges
     * @return Vector of bin edge values
     */
    [[nodiscard]] std::vector<T> bin_edges() const {
        std::vector<T> edges;
        const auto& axis = hist_.axis();
        
        for (auto&& bin : axis) {
            edges.push_back(bin.lower());
        }
        edges.push_back(axis.bin(axis.size() - 1).upper());
        
        return edges;
    }
    
    /**
     * @brief Get bin centers
     * @return Vector of bin center values
     */
    [[nodiscard]] std::vector<T> bin_centers() const {
        std::vector<T> centers;
        const auto& axis = hist_.axis();
        
        for (auto&& bin : axis) {
            if constexpr (std::is_integral_v<T>) {
                centers.push_back((bin.lower() + bin.upper()) / 2);
            } else {
                centers.push_back(bin.center());
            }
        }
        
        return centers;
    }
    
    /**
     * @brief Get bin counts
     * @return Vector of counts for each bin
     */
    [[nodiscard]] std::vector<double> bin_counts() const {
        std::vector<double> counts;
        counts.reserve(hist_.size());
        
        for (auto&& bin : bh::indexed(hist_)) {
            counts.push_back(*bin);
        }
        
        return counts;
    }
    
    /**
     * @brief Get normalized histogram (probability density)
     * @return Vector of normalized bin values
     */
    [[nodiscard]] std::vector<double> normalized() const {
        std::vector<double> norm;
        norm.reserve(hist_.size());
        
        if (total_count_ > 0) {
            const auto& axis = hist_.axis();
            for (std::size_t i = 0; i < hist_.size(); ++i) {
                double bin_width = 1.0;
                if constexpr (!std::is_integral_v<T>) {
                    bin_width = axis.bin(i).width();
                }
                norm.push_back(hist_[i] / (total_count_ * bin_width));
            }
        } else {
            norm.resize(hist_.size(), 0.0);
        }
        
        return norm;
    }
    
    /**
     * @brief Find mode (bin with highest count)
     * @return Pair of (bin_center, count)
     */
    [[nodiscard]] std::pair<T, double> mode() const {
        if (hist_.size() == 0) {
            return {T{}, 0.0};
        }
        
        auto max_it = std::max_element(hist_.begin(), hist_.end());
        if (max_it == hist_.end()) {
            return {T{}, 0.0};
        }
        
        std::size_t max_idx = std::distance(hist_.begin(), max_it);
        const auto& axis = hist_.axis();
        
        // Bounds check
        if (max_idx >= axis.size()) {
            return {T{}, 0.0};
        }
        
        T center;
        try {
            if constexpr (std::is_integral_v<T>) {
                auto bin = axis.bin(max_idx);
                center = static_cast<T>((bin.lower() + bin.upper()) / 2);
            } else {
                center = axis.bin(max_idx).center();
            }
        } catch (...) {
            // Fallback: return middle of histogram range
            center = static_cast<T>((min_value_ + max_value_) / 2);
        }
        
        return {center, *max_it};
    }
    
    /**
     * @brief Calculate mean from histogram
     * @return Weighted mean of bin centers
     */
    [[nodiscard]] double mean() const {
        if (total_count_ == 0) return 0.0;
        
        double sum = 0.0;
        const auto& axis = hist_.axis();
        
        for (std::size_t i = 0; i < hist_.size(); ++i) {
            T center;
            if constexpr (std::is_integral_v<T>) {
                auto bin = axis.bin(i);
                center = (bin.lower() + bin.upper()) / 2;
            } else {
                center = axis.bin(i).center();
            }
            sum += center * hist_[i];
        }
        
        return sum / total_count_;
    }
    
    /**
     * @brief Calculate variance from histogram
     * @return Variance of the distribution
     */
    [[nodiscard]] double variance() const {
        if (total_count_ == 0) return 0.0;
        
        double m = mean();
        double sum_sq = 0.0;
        const auto& axis = hist_.axis();
        
        for (std::size_t i = 0; i < hist_.size(); ++i) {
            T center;
            if constexpr (std::is_integral_v<T>) {
                auto bin = axis.bin(i);
                center = (bin.lower() + bin.upper()) / 2;
            } else {
                center = axis.bin(i).center();
            }
            double diff = center - m;
            sum_sq += diff * diff * hist_[i];
        }
        
        return sum_sq / total_count_;
    }
    
    /**
     * @brief Calculate standard deviation
     * @return Standard deviation
     */
    [[nodiscard]] double standard_deviation() const {
        return std::sqrt(variance());
    }
    
    /**
     * @brief Calculate percentile
     * @param p Percentile (0-100)
     * @return Value at percentile
     */
    [[nodiscard]] T percentile(double p) const {
        if (total_count_ == 0 || p < 0 || p > 100) {
            return T{};
        }
        
        double target = total_count_ * p / 100.0;
        double cumsum = 0.0;
        const auto& axis = hist_.axis();
        
        for (std::size_t i = 0; i < hist_.size(); ++i) {
            cumsum += hist_[i];
            if (cumsum >= target) {
                try {
                    if constexpr (std::is_integral_v<T>) {
                        auto bin = axis.bin(i);
                        return static_cast<T>((bin.lower() + bin.upper()) / 2);
                    } else {
                        return axis.bin(i).center();
                    }
                } catch (...) {
                    return max_value_;
                }
            }
        }
        
        return max_value_;
    }
    
    /**
     * @brief Get entropy of the distribution
     * @return Shannon entropy
     */
    [[nodiscard]] double entropy() const {
        if (total_count_ == 0) return 0.0;
        
        double ent = 0.0;
        for (auto&& count : hist_) {
            if (count > 0) {
                double p = count / total_count_;
                ent -= p * std::log2(p);
            }
        }
        
        return ent;
    }
    
    /**
     * @brief Merge another histogram
     * @param other Histogram to merge
     */
    void merge(const Histogram& other) {
        hist_ += other.hist_;
        total_count_ += other.total_count_;
        min_value_ = std::min(min_value_, other.min_value_);
        max_value_ = std::max(max_value_, other.max_value_);
    }
    
    /**
     * @brief Reset histogram to empty state
     */
    void reset() {
        hist_.reset();
        total_count_ = 0;
        first_value_ = true;
    }
    
    /**
     * @brief Access underlying boost histogram
     * @return Reference to boost histogram
     */
    [[nodiscard]] const histogram_type& get_histogram() const {
        return hist_;
    }
    
private:
    histogram_type hist_;
    double total_count_ = 0;
    T min_value_{};
    T max_value_{};
    bool first_value_ = true;
};

/**
 * @brief Specialized histogram for dice values
 */
class DiceHistogram : public Histogram<int> {
public:
    DiceHistogram() : Histogram<int>(6, 1, 7) {} // 6 bins for faces 1-6
    
    /**
     * @brief Check if dice distribution is fair
     * @param alpha Significance level
     * @return True if distribution appears uniform
     */
    [[nodiscard]] bool is_fair(double alpha = 0.05) const {
        // Chi-square test for uniformity
        double expected = total_count() / 6.0;
        double chi_square = 0.0;
        
        for (int face = 1; face <= 6; ++face) {
            double observed = count_at(face);
            double diff = observed - expected;
            chi_square += (diff * diff) / expected;
        }
        
        // Critical value for 5 degrees of freedom
        double critical_value = 11.070; // alpha = 0.05
        return chi_square < critical_value;
    }
};

/**
 * @brief 2D histogram for correlation analysis
 */
template<typename T1 = double, typename T2 = double>
class Histogram2D {
public:
    using histogram_type = decltype(
        bh::make_histogram(
            std::declval<typename Histogram<T1>::axis_type>(),
            std::declval<typename Histogram<T2>::axis_type>()
        )
    );
    
    Histogram2D(std::size_t bins_x, T1 min_x, T1 max_x,
                std::size_t bins_y, T2 min_y, T2 max_y) {
        if constexpr (std::is_integral_v<T1> && std::is_integral_v<T2>) {
            hist_ = bh::make_histogram(
                bh::axis::integer<T1>(min_x, max_x, "x"),
                bh::axis::integer<T2>(min_y, max_y, "y")
            );
        } else if constexpr (std::is_integral_v<T1>) {
            hist_ = bh::make_histogram(
                bh::axis::integer<T1>(min_x, max_x, "x"),
                bh::axis::regular<T2>(bins_y, min_y, max_y, "y")
            );
        } else if constexpr (std::is_integral_v<T2>) {
            hist_ = bh::make_histogram(
                bh::axis::regular<T1>(bins_x, min_x, max_x, "x"),
                bh::axis::integer<T2>(min_y, max_y, "y")
            );
        } else {
            hist_ = bh::make_histogram(
                bh::axis::regular<T1>(bins_x, min_x, max_x, "x"),
                bh::axis::regular<T2>(bins_y, min_y, max_y, "y")
            );
        }
    }
    
    void add(T1 x, T2 y, double weight = 1.0) {
        hist_(x, y, bh::weight(weight));
    }
    
    [[nodiscard]] double correlation() const {
        // Calculate Pearson correlation coefficient
        // This is a simplified version - full implementation would be more complex
        return 0.0; // Placeholder
    }
    
private:
    histogram_type hist_;
};

} // namespace liarsdice::statistics