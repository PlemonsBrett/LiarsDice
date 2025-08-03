#ifndef LIARSDICE_BAYESIAN_BAYESIAN_ANALYZER_HPP
#define LIARSDICE_BAYESIAN_BAYESIAN_ANALYZER_HPP

#include <boost/math/distributions.hpp>
#include <boost/math/quadrature/gauss_kronrod.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <concepts>
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <stdexcept>

namespace liarsdice::bayesian {

// Forward declarations
template<typename T>
    requires std::floating_point<T>
class PriorDistribution;

template<typename T>
    requires std::floating_point<T>
class LikelihoodFunction;

template<typename T>
    requires std::floating_point<T>
class PosteriorCalculator;

/**
 * @brief Core Bayesian analysis engine for statistical inference
 * 
 * Provides a comprehensive framework for Bayesian analysis using Boost.Math
 * distributions and numerical methods. Supports conjugate and non-conjugate
 * priors with various likelihood functions.
 * 
 * @tparam T The numeric type for calculations (float, double, long double)
 */
template<typename T = double>
    requires std::floating_point<T>
class BayesianAnalyzer {
public:
    using value_type = T;
    using vector_type = boost::numeric::ublas::vector<T>;
    using matrix_type = boost::numeric::ublas::matrix<T>;
    using prior_ptr = std::shared_ptr<PriorDistribution<T>>;
    using likelihood_ptr = std::shared_ptr<LikelihoodFunction<T>>;
    using posterior_ptr = std::shared_ptr<PosteriorCalculator<T>>;
    
    /**
     * @brief Construct a Bayesian analyzer with optional configuration
     */
    BayesianAnalyzer() = default;
    
    /**
     * @brief Set the prior distribution for analysis
     * @param prior Shared pointer to prior distribution
     */
    void set_prior(prior_ptr prior) {
        prior_ = std::move(prior);
    }
    
    /**
     * @brief Set the likelihood function for analysis
     * @param likelihood Shared pointer to likelihood function
     */
    void set_likelihood(likelihood_ptr likelihood) {
        likelihood_ = std::move(likelihood);
    }
    
    /**
     * @brief Update beliefs with new data observation
     * @param data Vector of observed data points
     * @return Updated posterior distribution
     */
    posterior_ptr update(const vector_type& data) {
        if (!prior_ || !likelihood_) {
            throw std::runtime_error("Prior and likelihood must be set before updating");
        }
        
        // Create posterior calculator if not exists
        if (!posterior_) {
            posterior_ = std::make_shared<PosteriorCalculator<T>>(prior_, likelihood_);
        }
        
        // Update with new data
        posterior_->update(data);
        return posterior_;
    }
    
    /**
     * @brief Perform model comparison using Bayes factors
     * @param model1 First model analyzer
     * @param model2 Second model analyzer
     * @param data Common data for both models
     * @return Bayes factor (evidence ratio) favoring model1
     */
    static T bayes_factor(BayesianAnalyzer& model1, 
                          BayesianAnalyzer& model2,
                          const vector_type& data) {
        auto posterior1 = model1.update(data);
        auto posterior2 = model2.update(data);
        
        T evidence1 = posterior1->marginal_likelihood();
        T evidence2 = posterior2->marginal_likelihood();
        
        return evidence1 / evidence2;
    }
    
    /**
     * @brief Calculate posterior predictive distribution
     * @param n_samples Number of samples to generate
     * @return Vector of predictive samples
     */
    vector_type posterior_predictive(std::size_t n_samples) const {
        if (!posterior_) {
            throw std::runtime_error("No posterior available for prediction");
        }
        
        return posterior_->predictive_sample(n_samples);
    }
    
    /**
     * @brief Get credible interval for parameter
     * @param confidence Confidence level (e.g., 0.95 for 95%)
     * @return Pair of (lower, upper) bounds
     */
    std::pair<T, T> credible_interval(T confidence = 0.95) const {
        if (!posterior_) {
            throw std::runtime_error("No posterior available");
        }
        
        return posterior_->credible_interval(confidence);
    }
    
    /**
     * @brief Calculate highest density interval (HDI)
     * @param confidence Confidence level
     * @return Pair of (lower, upper) bounds for HDI
     */
    std::pair<T, T> highest_density_interval(T confidence = 0.95) const {
        if (!posterior_) {
            throw std::runtime_error("No posterior available");
        }
        
        return posterior_->highest_density_interval(confidence);
    }
    
    /**
     * @brief Get posterior mean estimate
     */
    T posterior_mean() const {
        if (!posterior_) {
            throw std::runtime_error("No posterior available");
        }
        return posterior_->mean();
    }
    
    /**
     * @brief Get posterior mode (MAP estimate)
     */
    T posterior_mode() const {
        if (!posterior_) {
            throw std::runtime_error("No posterior available");
        }
        return posterior_->mode();
    }
    
    /**
     * @brief Get posterior variance
     */
    T posterior_variance() const {
        if (!posterior_) {
            throw std::runtime_error("No posterior available");
        }
        return posterior_->variance();
    }
    
    /**
     * @brief Reset analyzer to initial state
     */
    void reset() {
        posterior_.reset();
    }
    
    /**
     * @brief Get diagnostic information about the analysis
     */
    struct DiagnosticInfo {
        T effective_sample_size;
        T information_gain;
        T convergence_metric;
        std::size_t n_observations;
    };
    
    DiagnosticInfo get_diagnostics() const {
        if (!posterior_) {
            throw std::runtime_error("No posterior available");
        }
        
        return {
            posterior_->effective_sample_size(),
            posterior_->information_gain(),
            posterior_->convergence_metric(),
            posterior_->num_observations()
        };
    }

private:
    prior_ptr prior_;
    likelihood_ptr likelihood_;
    posterior_ptr posterior_;
};

/**
 * @brief Factory function for creating analyzers with conjugate priors
 * 
 * Automatically selects appropriate conjugate prior for common likelihood families
 */
template<typename T = double>
std::shared_ptr<BayesianAnalyzer<T>> create_conjugate_analyzer(
    const std::string& likelihood_family,
    const std::unordered_map<std::string, T>& hyperparameters) {
    
    auto analyzer = std::make_shared<BayesianAnalyzer<T>>();
    
    // Factory logic will be implemented with specific distributions
    // This is a placeholder for the interface
    
    return analyzer;
}

} // namespace liarsdice::bayesian

#endif // LIARSDICE_BAYESIAN_BAYESIAN_ANALYZER_HPP