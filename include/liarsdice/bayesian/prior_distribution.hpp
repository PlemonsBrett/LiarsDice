#ifndef LIARSDICE_BAYESIAN_PRIOR_DISTRIBUTION_HPP
#define LIARSDICE_BAYESIAN_PRIOR_DISTRIBUTION_HPP

#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/gamma.hpp>
#include <boost/math/distributions/uniform.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/random.hpp>
#include <concepts>
#include <memory>
#include <string>
#include <vector>

namespace liarsdice::bayesian {

/**
 * @brief Abstract base class for prior distributions in Bayesian analysis
 * 
 * Provides a common interface for all prior distributions with support
 * for evaluation, sampling, and moment calculations.
 * 
 * @tparam T Floating point type for calculations
 */
template<typename T = double>
    requires std::floating_point<T>
class PriorDistribution {
public:
    using value_type = T;
    
    virtual ~PriorDistribution() = default;
    
    /**
     * @brief Evaluate the probability density at a given point
     * @param x Point at which to evaluate the PDF
     * @return Probability density value
     */
    [[nodiscard]] virtual T pdf(T x) const = 0;
    
    /**
     * @brief Evaluate the log probability density
     * @param x Point at which to evaluate the log PDF
     * @return Log probability density value
     */
    [[nodiscard]] virtual T log_pdf(T x) const = 0;
    
    /**
     * @brief Evaluate the cumulative distribution function
     * @param x Point at which to evaluate the CDF
     * @return Cumulative probability
     */
    [[nodiscard]] virtual T cdf(T x) const = 0;
    
    /**
     * @brief Generate a random sample from the distribution
     * @param gen Random number generator
     * @return Random sample
     */
    [[nodiscard]] virtual T sample(boost::random::mt19937& gen) const = 0;
    
    /**
     * @brief Generate multiple samples from the distribution
     * @param n Number of samples
     * @param gen Random number generator
     * @return Vector of samples
     */
    [[nodiscard]] virtual std::vector<T> sample(std::size_t n, 
                                                boost::random::mt19937& gen) const {
        std::vector<T> samples;
        samples.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            samples.push_back(sample(gen));
        }
        return samples;
    }
    
    /**
     * @brief Get the mean of the distribution
     */
    [[nodiscard]] virtual T mean() const = 0;
    
    /**
     * @brief Get the variance of the distribution
     */
    [[nodiscard]] virtual T variance() const = 0;
    
    /**
     * @brief Get the mode of the distribution (if exists)
     */
    [[nodiscard]] virtual std::optional<T> mode() const = 0;
    
    /**
     * @brief Get the support of the distribution
     * @return Pair of (lower_bound, upper_bound)
     */
    [[nodiscard]] virtual std::pair<T, T> support() const = 0;
    
    /**
     * @brief Get the name of the distribution
     */
    [[nodiscard]] virtual std::string name() const = 0;
    
    /**
     * @brief Clone the distribution
     */
    [[nodiscard]] virtual std::unique_ptr<PriorDistribution<T>> clone() const = 0;
    
    /**
     * @brief Check if this is a conjugate prior for a given likelihood
     */
    [[nodiscard]] virtual bool is_conjugate_to(const std::string& likelihood_family) const {
        return false;
    }
};

/**
 * @brief Beta distribution prior - conjugate for Bernoulli/Binomial likelihoods
 * 
 * Particularly useful for modeling probabilities and proportions in [0,1]
 */
template<typename T = double>
class BetaPrior : public PriorDistribution<T> {
public:
    using distribution_type = boost::math::beta_distribution<T>;
    
    /**
     * @brief Construct a Beta prior
     * @param alpha Shape parameter α > 0
     * @param beta Shape parameter β > 0
     */
    BetaPrior(T alpha, T beta) 
        : dist_(alpha, beta), alpha_(alpha), beta_(beta) {
        if (alpha <= 0 || beta <= 0) {
            throw std::invalid_argument("Beta parameters must be positive");
        }
    }
    
    T pdf(T x) const override {
        if (x < 0 || x > 1) return 0;
        return boost::math::pdf(dist_, x);
    }
    
    T log_pdf(T x) const override {
        if (x < 0 || x > 1) return -std::numeric_limits<T>::infinity();
        return std::log(pdf(x));
    }
    
    T cdf(T x) const override {
        if (x <= 0) return 0;
        if (x >= 1) return 1;
        return boost::math::cdf(dist_, x);
    }
    
    T sample(boost::random::mt19937& gen) const override {
        boost::random::beta_distribution<T> sampler(alpha_, beta_);
        return sampler(gen);
    }
    
    T mean() const override {
        return boost::math::mean(dist_);
    }
    
    T variance() const override {
        return boost::math::variance(dist_);
    }
    
    std::optional<T> mode() const override {
        if (alpha_ > 1 && beta_ > 1) {
            return (alpha_ - 1) / (alpha_ + beta_ - 2);
        } else if (alpha_ < 1 && beta_ < 1) {
            return std::nullopt; // Bimodal
        } else if (alpha_ == 1 && beta_ == 1) {
            return 0.5; // Uniform, any value in [0,1]
        }
        return std::nullopt;
    }
    
    std::pair<T, T> support() const override {
        return {0, 1};
    }
    
    std::string name() const override {
        return "Beta(" + std::to_string(alpha_) + ", " + std::to_string(beta_) + ")";
    }
    
    std::unique_ptr<PriorDistribution<T>> clone() const override {
        return std::make_unique<BetaPrior<T>>(alpha_, beta_);
    }
    
    bool is_conjugate_to(const std::string& likelihood_family) const override {
        return likelihood_family == "bernoulli" || 
               likelihood_family == "binomial";
    }
    
    /**
     * @brief Get the alpha parameter
     */
    T alpha() const { return alpha_; }
    
    /**
     * @brief Get the beta parameter
     */
    T beta() const { return beta_; }

private:
    distribution_type dist_;
    T alpha_;
    T beta_;
};

/**
 * @brief Normal distribution prior - conjugate for Normal likelihood with known variance
 */
template<typename T = double>
class NormalPrior : public PriorDistribution<T> {
public:
    using distribution_type = boost::math::normal_distribution<T>;
    
    NormalPrior(T mean, T std_dev) 
        : dist_(mean, std_dev), mean_(mean), std_dev_(std_dev) {
        if (std_dev <= 0) {
            throw std::invalid_argument("Standard deviation must be positive");
        }
    }
    
    T pdf(T x) const override {
        return boost::math::pdf(dist_, x);
    }
    
    T log_pdf(T x) const override {
        return -0.5 * std::log(2 * M_PI) - std::log(std_dev_) 
               - 0.5 * std::pow((x - mean_) / std_dev_, 2);
    }
    
    T cdf(T x) const override {
        return boost::math::cdf(dist_, x);
    }
    
    T sample(boost::random::mt19937& gen) const override {
        boost::random::normal_distribution<T> sampler(mean_, std_dev_);
        return sampler(gen);
    }
    
    T mean() const override { return mean_; }
    T variance() const override { return std_dev_ * std_dev_; }
    
    std::optional<T> mode() const override { return mean_; }
    
    std::pair<T, T> support() const override {
        return {-std::numeric_limits<T>::infinity(), 
                std::numeric_limits<T>::infinity()};
    }
    
    std::string name() const override {
        return "Normal(" + std::to_string(mean_) + ", " + std::to_string(std_dev_) + ")";
    }
    
    std::unique_ptr<PriorDistribution<T>> clone() const override {
        return std::make_unique<NormalPrior<T>>(mean_, std_dev_);
    }
    
    bool is_conjugate_to(const std::string& likelihood_family) const override {
        return likelihood_family == "normal_known_variance";
    }

private:
    distribution_type dist_;
    T mean_;
    T std_dev_;
};

/**
 * @brief Gamma distribution prior - conjugate for Poisson and Exponential likelihoods
 */
template<typename T = double>
class GammaPrior : public PriorDistribution<T> {
public:
    using distribution_type = boost::math::gamma_distribution<T>;
    
    GammaPrior(T shape, T rate) 
        : dist_(shape, 1/rate), shape_(shape), rate_(rate) {
        if (shape <= 0 || rate <= 0) {
            throw std::invalid_argument("Gamma parameters must be positive");
        }
    }
    
    T pdf(T x) const override {
        if (x <= 0) return 0;
        return boost::math::pdf(dist_, x);
    }
    
    T log_pdf(T x) const override {
        if (x <= 0) return -std::numeric_limits<T>::infinity();
        return shape_ * std::log(rate_) - std::lgamma(shape_) 
               + (shape_ - 1) * std::log(x) - rate_ * x;
    }
    
    T cdf(T x) const override {
        if (x <= 0) return 0;
        return boost::math::cdf(dist_, x);
    }
    
    T sample(boost::random::mt19937& gen) const override {
        boost::random::gamma_distribution<T> sampler(shape_, 1/rate_);
        return sampler(gen);
    }
    
    T mean() const override { return shape_ / rate_; }
    T variance() const override { return shape_ / (rate_ * rate_); }
    
    std::optional<T> mode() const override {
        if (shape_ >= 1) {
            return (shape_ - 1) / rate_;
        }
        return std::nullopt;
    }
    
    std::pair<T, T> support() const override {
        return {0, std::numeric_limits<T>::infinity()};
    }
    
    std::string name() const override {
        return "Gamma(" + std::to_string(shape_) + ", " + std::to_string(rate_) + ")";
    }
    
    std::unique_ptr<PriorDistribution<T>> clone() const override {
        return std::make_unique<GammaPrior<T>>(shape_, rate_);
    }
    
    bool is_conjugate_to(const std::string& likelihood_family) const override {
        return likelihood_family == "poisson" || 
               likelihood_family == "exponential";
    }
    
    /**
     * @brief Get the shape parameter (alpha)
     */
    T alpha() const { return shape_; }
    
    /**
     * @brief Get the rate parameter (beta)
     */
    T beta() const { return rate_; }

private:
    distribution_type dist_;
    T shape_;
    T rate_;
};

/**
 * @brief Uniform distribution prior - minimally informative prior
 */
template<typename T = double>
class UniformPrior : public PriorDistribution<T> {
public:
    using distribution_type = boost::math::uniform_distribution<T>;
    
    UniformPrior(T lower, T upper) 
        : dist_(lower, upper), lower_(lower), upper_(upper) {
        if (lower >= upper) {
            throw std::invalid_argument("Lower bound must be less than upper bound");
        }
    }
    
    T pdf(T x) const override {
        if (x < lower_ || x > upper_) return 0;
        return 1 / (upper_ - lower_);
    }
    
    T log_pdf(T x) const override {
        if (x < lower_ || x > upper_) return -std::numeric_limits<T>::infinity();
        return -std::log(upper_ - lower_);
    }
    
    T cdf(T x) const override {
        if (x <= lower_) return 0;
        if (x >= upper_) return 1;
        return (x - lower_) / (upper_ - lower_);
    }
    
    T sample(boost::random::mt19937& gen) const override {
        boost::random::uniform_real_distribution<T> sampler(lower_, upper_);
        return sampler(gen);
    }
    
    T mean() const override { return (lower_ + upper_) / 2; }
    T variance() const override { 
        T range = upper_ - lower_;
        return (range * range) / 12; 
    }
    
    std::optional<T> mode() const override { 
        return std::nullopt; // Any value in [lower, upper]
    }
    
    std::pair<T, T> support() const override { return {lower_, upper_}; }
    
    std::string name() const override {
        return "Uniform(" + std::to_string(lower_) + ", " + std::to_string(upper_) + ")";
    }
    
    std::unique_ptr<PriorDistribution<T>> clone() const override {
        return std::make_unique<UniformPrior<T>>(lower_, upper_);
    }

private:
    distribution_type dist_;
    T lower_;
    T upper_;
};

/**
 * @brief Factory function for creating common prior distributions
 */
template<typename T = double>
std::unique_ptr<PriorDistribution<T>> create_prior(
    const std::string& distribution_name,
    const std::vector<T>& parameters) {
    
    if (distribution_name == "beta" && parameters.size() == 2) {
        return std::make_unique<BetaPrior<T>>(parameters[0], parameters[1]);
    } else if (distribution_name == "normal" && parameters.size() == 2) {
        return std::make_unique<NormalPrior<T>>(parameters[0], parameters[1]);
    } else if (distribution_name == "gamma" && parameters.size() == 2) {
        return std::make_unique<GammaPrior<T>>(parameters[0], parameters[1]);
    } else if (distribution_name == "uniform" && parameters.size() == 2) {
        return std::make_unique<UniformPrior<T>>(parameters[0], parameters[1]);
    }
    
    throw std::invalid_argument("Unknown distribution or incorrect parameters");
}

} // namespace liarsdice::bayesian

#endif // LIARSDICE_BAYESIAN_PRIOR_DISTRIBUTION_HPP