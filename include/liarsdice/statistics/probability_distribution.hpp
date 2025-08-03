#pragma once

#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/math/distributions/poisson.hpp>
#include <boost/math/distributions/uniform.hpp>
#include <boost/math/distributions/exponential.hpp>
#include <boost/math/distributions/chi_squared.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/gamma.hpp>
#include <boost/random.hpp>
#include <memory>
#include <variant>

namespace liarsdice::statistics {

namespace bm = boost::math;
namespace br = boost::random;

/**
 * @brief Base interface for probability distributions
 * 
 * Provides unified interface for various boost::math distributions
 * with support for PDF, CDF, quantiles, and random sampling.
 */
class IProbabilityDistribution {
public:
    virtual ~IProbabilityDistribution() = default;
    
    /**
     * @brief Probability density function
     * @param x Point to evaluate
     * @return PDF value at x
     */
    [[nodiscard]] virtual double pdf(double x) const = 0;
    
    /**
     * @brief Cumulative distribution function
     * @param x Point to evaluate
     * @return CDF value at x
     */
    [[nodiscard]] virtual double cdf(double x) const = 0;
    
    /**
     * @brief Quantile function (inverse CDF)
     * @param p Probability (0 < p < 1)
     * @return Quantile value
     */
    [[nodiscard]] virtual double quantile(double p) const = 0;
    
    /**
     * @brief Mean of the distribution
     * @return Mean value
     */
    [[nodiscard]] virtual double mean() const = 0;
    
    /**
     * @brief Variance of the distribution
     * @return Variance
     */
    [[nodiscard]] virtual double variance() const = 0;
    
    /**
     * @brief Standard deviation
     * @return Standard deviation
     */
    [[nodiscard]] virtual double standard_deviation() const {
        return std::sqrt(variance());
    }
    
    /**
     * @brief Generate random sample
     * @param gen Random number generator
     * @return Random value from distribution
     */
    virtual double sample(br::mt19937& gen) const = 0;
    
    /**
     * @brief Generate multiple random samples
     * @param gen Random number generator
     * @param n Number of samples
     * @return Vector of samples
     */
    virtual std::vector<double> sample(br::mt19937& gen, std::size_t n) const {
        std::vector<double> samples;
        samples.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            samples.push_back(sample(gen));
        }
        return samples;
    }
    
    /**
     * @brief Get distribution name
     * @return String name of distribution
     */
    [[nodiscard]] virtual std::string name() const = 0;
};

/**
 * @brief Template wrapper for boost::math distributions
 */
template<typename Distribution>
class DistributionWrapper : public IProbabilityDistribution {
public:
    using distribution_type = Distribution;
    
    template<typename... Args>
    explicit DistributionWrapper(Args&&... args) 
        : dist_(std::forward<Args>(args)...) {}
    
    [[nodiscard]] double pdf(double x) const override {
        return bm::pdf(dist_, x);
    }
    
    [[nodiscard]] double cdf(double x) const override {
        return bm::cdf(dist_, x);
    }
    
    [[nodiscard]] double quantile(double p) const override {
        return bm::quantile(dist_, p);
    }
    
    [[nodiscard]] double mean() const override {
        return bm::mean(dist_);
    }
    
    [[nodiscard]] double variance() const override {
        return bm::variance(dist_);
    }
    
protected:
    Distribution dist_;
};

/**
 * @brief Normal (Gaussian) distribution
 */
class NormalDistribution : public DistributionWrapper<bm::normal_distribution<>> {
public:
    NormalDistribution(double mean = 0.0, double std_dev = 1.0)
        : DistributionWrapper(mean, std_dev), 
          sampler_(mean, std_dev) {}
    
    double sample(br::mt19937& gen) const override {
        return sampler_(gen);
    }
    
    [[nodiscard]] std::string name() const override {
        return "Normal";
    }
    
private:
    mutable br::normal_distribution<> sampler_;
};

/**
 * @brief Binomial distribution
 */
class BinomialDistribution : public DistributionWrapper<bm::binomial_distribution<>> {
public:
    BinomialDistribution(unsigned int n, double p)
        : DistributionWrapper(static_cast<double>(n), p),
          n_(n), p_(p), sampler_(n, p) {}
    
    double sample(br::mt19937& gen) const override {
        return static_cast<double>(sampler_(gen));
    }
    
    [[nodiscard]] std::string name() const override {
        return "Binomial";
    }
    
    [[nodiscard]] unsigned int trials() const { return n_; }
    [[nodiscard]] double probability() const { return p_; }
    
private:
    unsigned int n_;
    double p_;
    mutable br::binomial_distribution<> sampler_;
};

/**
 * @brief Poisson distribution
 */
class PoissonDistribution : public DistributionWrapper<bm::poisson_distribution<>> {
public:
    explicit PoissonDistribution(double lambda)
        : DistributionWrapper(lambda),
          lambda_(lambda), sampler_(lambda) {}
    
    double sample(br::mt19937& gen) const override {
        return static_cast<double>(sampler_(gen));
    }
    
    [[nodiscard]] std::string name() const override {
        return "Poisson";
    }
    
    [[nodiscard]] double rate() const { return lambda_; }
    
private:
    double lambda_;
    mutable br::poisson_distribution<> sampler_;
};

/**
 * @brief Uniform distribution
 */
class UniformDistribution : public DistributionWrapper<bm::uniform_distribution<>> {
public:
    UniformDistribution(double a = 0.0, double b = 1.0)
        : DistributionWrapper(a, b),
          a_(a), b_(b), sampler_(a, b) {}
    
    double sample(br::mt19937& gen) const override {
        return sampler_(gen);
    }
    
    [[nodiscard]] std::string name() const override {
        return "Uniform";
    }
    
    [[nodiscard]] double lower() const { return a_; }
    [[nodiscard]] double upper() const { return b_; }
    
private:
    double a_, b_;
    mutable br::uniform_real_distribution<> sampler_;
};

/**
 * @brief Exponential distribution
 */
class ExponentialDistribution : public DistributionWrapper<bm::exponential_distribution<>> {
public:
    explicit ExponentialDistribution(double lambda = 1.0)
        : DistributionWrapper(lambda),
          lambda_(lambda), sampler_(lambda) {}
    
    double sample(br::mt19937& gen) const override {
        return sampler_(gen);
    }
    
    [[nodiscard]] std::string name() const override {
        return "Exponential";
    }
    
    [[nodiscard]] double rate() const { return lambda_; }
    
private:
    double lambda_;
    mutable br::exponential_distribution<> sampler_;
};

/**
 * @brief Beta distribution
 */
class BetaDistribution : public DistributionWrapper<bm::beta_distribution<>> {
public:
    BetaDistribution(double alpha, double beta)
        : DistributionWrapper(alpha, beta),
          alpha_(alpha), beta_(beta) {
        // Beta sampling using gamma distributions
        gamma_alpha_ = std::make_unique<br::gamma_distribution<>>(alpha, 1.0);
        gamma_beta_ = std::make_unique<br::gamma_distribution<>>(beta, 1.0);
    }
    
    double sample(br::mt19937& gen) const override {
        // Beta = Gamma(α,1) / (Gamma(α,1) + Gamma(β,1))
        double x = (*gamma_alpha_)(gen);
        double y = (*gamma_beta_)(gen);
        return x / (x + y);
    }
    
    [[nodiscard]] std::string name() const override {
        return "Beta";
    }
    
    [[nodiscard]] double alpha() const { return alpha_; }
    [[nodiscard]] double beta() const { return beta_; }
    
private:
    double alpha_, beta_;
    mutable std::unique_ptr<br::gamma_distribution<>> gamma_alpha_;
    mutable std::unique_ptr<br::gamma_distribution<>> gamma_beta_;
};

/**
 * @brief Factory for creating distributions
 */
class DistributionFactory {
public:
    /**
     * @brief Create normal distribution
     */
    static std::unique_ptr<IProbabilityDistribution> create_normal(
        double mean = 0.0, double std_dev = 1.0) {
        return std::make_unique<NormalDistribution>(mean, std_dev);
    }
    
    /**
     * @brief Create binomial distribution
     */
    static std::unique_ptr<IProbabilityDistribution> create_binomial(
        unsigned int n, double p) {
        return std::make_unique<BinomialDistribution>(n, p);
    }
    
    /**
     * @brief Create Poisson distribution
     */
    static std::unique_ptr<IProbabilityDistribution> create_poisson(double lambda) {
        return std::make_unique<PoissonDistribution>(lambda);
    }
    
    /**
     * @brief Create uniform distribution
     */
    static std::unique_ptr<IProbabilityDistribution> create_uniform(
        double a = 0.0, double b = 1.0) {
        return std::make_unique<UniformDistribution>(a, b);
    }
    
    /**
     * @brief Create exponential distribution
     */
    static std::unique_ptr<IProbabilityDistribution> create_exponential(
        double lambda = 1.0) {
        return std::make_unique<ExponentialDistribution>(lambda);
    }
    
    /**
     * @brief Create beta distribution
     */
    static std::unique_ptr<IProbabilityDistribution> create_beta(
        double alpha, double beta) {
        return std::make_unique<BetaDistribution>(alpha, beta);
    }
};

/**
 * @brief Statistical hypothesis testing utilities
 */
class HypothesisTest {
public:
    /**
     * @brief Perform Kolmogorov-Smirnov test
     * @param data Sample data
     * @param dist Distribution to test against
     * @return Pair of (test_statistic, p_value)
     */
    static std::pair<double, double> kolmogorov_smirnov_test(
        const std::vector<double>& data,
        const IProbabilityDistribution& dist) {
        
        if (data.empty()) return {0.0, 1.0};
        
        // Sort data for empirical CDF
        std::vector<double> sorted_data = data;
        std::sort(sorted_data.begin(), sorted_data.end());
        
        // Calculate K-S statistic
        double max_diff = 0.0;
        std::size_t n = sorted_data.size();
        
        for (std::size_t i = 0; i < n; ++i) {
            double empirical_cdf = (i + 1.0) / n;
            double theoretical_cdf = dist.cdf(sorted_data[i]);
            double diff1 = std::abs(empirical_cdf - theoretical_cdf);
            double diff2 = std::abs(i / static_cast<double>(n) - theoretical_cdf);
            max_diff = std::max({max_diff, diff1, diff2});
        }
        
        // Approximate p-value (simplified)
        double sqrt_n = std::sqrt(static_cast<double>(n));
        double p_value = 2.0 * std::exp(-2.0 * n * max_diff * max_diff);
        
        return {max_diff, p_value};
    }
    
    /**
     * @brief Chi-square goodness of fit test
     * @param observed Observed frequencies
     * @param expected Expected frequencies
     * @return Pair of (chi_square_statistic, degrees_of_freedom)
     */
    static std::pair<double, int> chi_square_test(
        const std::vector<double>& observed,
        const std::vector<double>& expected) {
        
        if (observed.size() != expected.size() || observed.empty()) {
            return {0.0, 0};
        }
        
        double chi_square = 0.0;
        for (std::size_t i = 0; i < observed.size(); ++i) {
            if (expected[i] > 0) {
                double diff = observed[i] - expected[i];
                chi_square += (diff * diff) / expected[i];
            }
        }
        
        int df = static_cast<int>(observed.size()) - 1;
        return {chi_square, df};
    }
};

/**
 * @brief Bayesian inference utilities for game analysis
 */
class BayesianInference {
public:
    /**
     * @brief Update beta distribution with new observation
     * @param prior Prior beta distribution
     * @param successes Number of successes
     * @param failures Number of failures
     * @return Updated beta distribution
     */
    static std::unique_ptr<BetaDistribution> update_beta(
        const BetaDistribution& prior,
        unsigned int successes,
        unsigned int failures) {
        
        double new_alpha = prior.alpha() + successes;
        double new_beta = prior.beta() + failures;
        return std::make_unique<BetaDistribution>(new_alpha, new_beta);
    }
    
    /**
     * @brief Calculate credible interval for beta distribution
     * @param dist Beta distribution
     * @param credibility Credibility level (e.g., 0.95)
     * @return Pair of (lower_bound, upper_bound)
     */
    static std::pair<double, double> credible_interval(
        const BetaDistribution& dist,
        double credibility = 0.95) {
        
        double alpha = (1.0 - credibility) / 2.0;
        return {dist.quantile(alpha), dist.quantile(1.0 - alpha)};
    }
};

} // namespace liarsdice::statistics