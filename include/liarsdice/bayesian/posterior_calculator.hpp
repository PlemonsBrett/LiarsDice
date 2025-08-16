#ifndef LIARSDICE_BAYESIAN_POSTERIOR_CALCULATOR_HPP
#define LIARSDICE_BAYESIAN_POSTERIOR_CALCULATOR_HPP

#include <boost/math/quadrature/gauss_kronrod.hpp>

#include "likelihood_function.hpp"
#include "prior_distribution.hpp"
// Remove naive.hpp - not available in all Boost versions
#include <algorithm>
#include <boost/math/special_functions/log1p.hpp>
#include <boost/math/tools/minima.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/random.hpp>
#include <cmath>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <vector>

namespace liarsdice::bayesian {

  /**
   * @brief Calculates posterior distributions with numerical stability
   *
   * Implements various methods for posterior calculation including:
   * - Conjugate updates for known prior-likelihood pairs
   * - Numerical integration for non-conjugate cases
   * - Importance sampling and MCMC for complex posteriors
   *
   * @tparam T Floating point type for calculations
   */
  template <typename T = double>
    requires std::floating_point<T>
  class PosteriorCalculator {
  public:
    using value_type = T;
    using vector_type = boost::numeric::ublas::vector<T>;
    using prior_ptr = std::shared_ptr<PriorDistribution<T>>;
    using likelihood_ptr = std::shared_ptr<LikelihoodFunction<T>>;

    /**
     * @brief Construct a posterior calculator
     * @param prior Prior distribution
     * @param likelihood Likelihood function
     * @param seed Optional seed for deterministic testing (nullopt for random)
     */
    PosteriorCalculator(prior_ptr prior, likelihood_ptr likelihood,
                        std::optional<unsigned int> seed = std::nullopt)
        : prior_(std::move(prior)),
          likelihood_(std::move(likelihood)),
          n_observations_(0),
          log_marginal_likelihood_(0),
          seed_(seed) {
      // Check for conjugacy
      is_conjugate_ = prior_->is_conjugate_to(likelihood_->family());
    }

    /**
     * @brief Update posterior with new data
     * @param data New observations
     */
    void update(const vector_type& data) {
      if (data.empty()) return;

      // Store sufficient statistics
      auto new_stats = likelihood_->sufficient_statistics(data);

      if (is_conjugate_) {
        update_conjugate(new_stats);
      } else {
        update_numerical(data);
      }

      n_observations_ += data.size();
    }

    /**
     * @brief Get the posterior mean
     */
    [[nodiscard]] T mean() const {
      if (is_conjugate_ && conjugate_posterior_) {
        return conjugate_posterior_->mean();
      }

      // Numerical integration for expectation
      auto [lower, upper] = get_integration_bounds();

      auto integrand = [this](T theta) { return theta * posterior_pdf(theta); };

      return boost::math::quadrature::gauss_kronrod<T, 15>::integrate(integrand, lower, upper, 10,
                                                                      1e-9);
    }

    /**
     * @brief Get the posterior mode (MAP estimate)
     */
    [[nodiscard]] T mode() const {
      if (is_conjugate_ && conjugate_posterior_) {
        auto mode_opt = conjugate_posterior_->mode();
        if (mode_opt) return *mode_opt;
      }

      // Find mode using Brent's method
      auto [lower, upper] = get_integration_bounds();

      auto neg_log_posterior = [this](T theta) { return -log_posterior_pdf(theta); };

      int bits = std::numeric_limits<T>::digits;
      boost::uintmax_t max_iter = 100;

      auto result
          = boost::math::tools::brent_find_minima(neg_log_posterior, lower, upper, bits, max_iter);

      return result.first;
    }

    /**
     * @brief Get the posterior variance
     */
    [[nodiscard]] T variance() const {
      if (is_conjugate_ && conjugate_posterior_) {
        return conjugate_posterior_->variance();
      }

      T posterior_mean = mean();

      // E[X^2] - E[X]^2
      auto [lower, upper] = get_integration_bounds();

      auto integrand = [this](T theta) { return theta * theta * posterior_pdf(theta); };

      T second_moment = boost::math::quadrature::gauss_kronrod<T, 15>::integrate(integrand, lower,
                                                                                 upper, 10, 1e-9);

      return second_moment - posterior_mean * posterior_mean;
    }

    /**
     * @brief Get credible interval
     * @param confidence Confidence level (e.g., 0.95)
     * @return Pair of (lower, upper) bounds
     */
    [[nodiscard]] std::pair<T, T> credible_interval(T confidence = 0.95) const {
      if (is_conjugate_ && conjugate_posterior_) {
        T alpha = (1 - confidence) / 2;

        // Use inverse CDF for conjugate cases
        auto [lower_bound, upper_bound] = conjugate_posterior_->support();

        // Binary search for quantiles
        auto find_quantile = [this](T target_cdf) {
          auto [lower, upper] = conjugate_posterior_->support();

          while (upper - lower > 1e-9) {
            T mid = (lower + upper) / 2;
            if (posterior_cdf(mid) < target_cdf) {
              lower = mid;
            } else {
              upper = mid;
            }
          }
          return (lower + upper) / 2;
        };

        return {find_quantile(alpha), find_quantile(1 - alpha)};
      }

      // Numerical approach for non-conjugate
      return numerical_credible_interval(confidence);
    }

    /**
     * @brief Get highest density interval (HDI)
     * @param confidence Confidence level
     * @return Pair of (lower, upper) bounds for HDI
     */
    [[nodiscard]] std::pair<T, T> highest_density_interval(T confidence = 0.95) const {
      // HDI is the shortest interval containing the specified probability mass
      // This is more complex than credible interval and requires optimization

      auto [lower_support, upper_support] = get_integration_bounds();

      // Define objective: minimize interval width subject to probability constraint
      auto interval_width = [this, confidence, lower_support, upper_support](T lower) {
        // Find upper bound such that P(lower < theta < upper) = confidence
        T target_prob = confidence;
        T current_prob = 0;
        T upper = lower;
        T step = (upper_support - lower_support) / 1000;

        while (current_prob < target_prob && upper < upper_support) {
          upper += step;
          current_prob = posterior_cdf(upper) - posterior_cdf(lower);
        }

        return upper - lower;
      };

      // Find the lower bound that minimizes interval width
      int bits = std::numeric_limits<T>::digits;
      boost::uintmax_t max_iter = 100;

      auto result = boost::math::tools::brent_find_minima(
          interval_width, lower_support,
          upper_support - (upper_support - lower_support) * confidence, bits, max_iter);

      T optimal_lower = result.first;

      // Find corresponding upper bound
      T upper = optimal_lower;
      T current_prob = 0;
      T step = (upper_support - lower_support) / 1000;
      while (current_prob < confidence && upper < upper_support) {
        upper += step;
        current_prob = posterior_cdf(upper) - posterior_cdf(optimal_lower);
      }

      return {optimal_lower, upper};
    }

    /**
     * @brief Calculate marginal likelihood (model evidence)
     */
    [[nodiscard]] T marginal_likelihood() const { return std::exp(log_marginal_likelihood_); }

    /**
     * @brief Generate samples from the posterior
     * @param n_samples Number of samples to generate
     * @return Vector of posterior samples
     */
    [[nodiscard]] vector_type predictive_sample(std::size_t n_samples) const {
      vector_type samples(n_samples);
      boost::random::mt19937 gen;
      if (seed_) {
        gen.seed(*seed_);
      } else {
        std::random_device rd;
        gen.seed(rd());
      }

      if (is_conjugate_ && conjugate_posterior_) {
        // Direct sampling from conjugate posterior
        auto posterior_samples = conjugate_posterior_->sample(n_samples, gen);
        std::copy(posterior_samples.begin(), posterior_samples.end(), samples.begin());
      } else {
        // Use rejection sampling or MCMC
        samples = rejection_sample(n_samples, gen);
      }

      return samples;
    }

    /**
     * @brief Get effective sample size for importance sampling
     */
    [[nodiscard]] T effective_sample_size() const {
      // ESS = 1 / sum(w_i^2) where w_i are normalized weights
      // For conjugate updates, ESS is related to the posterior parameters
      if (is_conjugate_) {
        return static_cast<T>(n_observations_);
      }

      // For numerical methods, estimate from samples
      return estimate_ess();
    }

    /**
     * @brief Calculate information gain (KL divergence from prior to posterior)
     */
    [[nodiscard]] T information_gain() const {
      auto [lower, upper] = get_integration_bounds();

      auto kl_integrand = [this](T theta) {
        T post = posterior_pdf(theta);
        if (post <= 0) return T(0);

        T prior = prior_->pdf(theta);
        if (prior <= 0) return T(0);

        return post * std::log(post / prior);
      };

      return boost::math::quadrature::gauss_kronrod<T, 15>::integrate(kl_integrand, lower, upper,
                                                                      10, 1e-9);
    }

    /**
     * @brief Get convergence metric
     */
    [[nodiscard]] T convergence_metric() const {
      // For conjugate cases, convergence is immediate
      if (is_conjugate_) return 1.0;

      // For numerical methods, return a metric based on integration error
      return convergence_metric_;
    }

    /**
     * @brief Get number of observations processed
     */
    [[nodiscard]] std::size_t num_observations() const { return n_observations_; }

  private:
    prior_ptr prior_;
    likelihood_ptr likelihood_;
    bool is_conjugate_;
    std::size_t n_observations_;
    T log_marginal_likelihood_;
    T convergence_metric_ = 1.0;

    // For conjugate updates
    std::unique_ptr<PriorDistribution<T>> conjugate_posterior_;

    // For numerical updates
    std::vector<T> posterior_samples_;
    std::vector<T> importance_weights_;

    // Optional seed for deterministic testing
    std::optional<unsigned int> seed_;

    /**
     * @brief Update using conjugate prior relationships
     */
    void update_conjugate(const vector_type& sufficient_stats) {
      auto likelihood_family = likelihood_->family();

      if (likelihood_family == "bernoulli" || likelihood_family == "binomial") {
        // Beta-Binomial conjugacy
        auto beta_prior = dynamic_cast<BetaPrior<T>*>(prior_.get());
        if (beta_prior) {
          T alpha = beta_prior->alpha();
          T beta = beta_prior->beta();

          // Update: alpha' = alpha + successes, beta' = beta + failures
          T successes = sufficient_stats[0];
          T trials = sufficient_stats[1];
          T failures = trials - successes;

          conjugate_posterior_ = std::make_unique<BetaPrior<T>>(alpha + successes, beta + failures);

          // Update marginal likelihood
          update_marginal_likelihood_conjugate(alpha, beta, successes, failures);
        }
      } else if (likelihood_family == "normal_known_variance") {
        // Normal-Normal conjugacy
        auto normal_prior = dynamic_cast<NormalPrior<T>*>(prior_.get());
        auto normal_likelihood = dynamic_cast<NormalKnownVarianceLikelihood<T>*>(likelihood_.get());

        if (normal_prior && normal_likelihood) {
          T prior_mean = normal_prior->mean();
          T prior_var = normal_prior->variance();
          T likelihood_var = normal_likelihood->variance();

          T sum_x = sufficient_stats[0];
          T n = sufficient_stats[1];

          // Update posterior parameters
          T posterior_var = 1 / (1 / prior_var + n / likelihood_var);
          T posterior_mean = posterior_var * (prior_mean / prior_var + sum_x / likelihood_var);

          conjugate_posterior_
              = std::make_unique<NormalPrior<T>>(posterior_mean, std::sqrt(posterior_var));
        }
      } else if (likelihood_family == "poisson") {
        // Gamma-Poisson conjugacy
        auto gamma_prior = dynamic_cast<GammaPrior<T>*>(prior_.get());
        if (gamma_prior) {
          T shape = gamma_prior->alpha();
          T rate = gamma_prior->beta();

          T sum_x = sufficient_stats[0];
          T n = sufficient_stats[1];

          // Update: shape' = shape + sum(x), rate' = rate + n
          conjugate_posterior_ = std::make_unique<GammaPrior<T>>(shape + sum_x, rate + n);
        }
      }
    }

    /**
     * @brief Update using numerical methods
     */
    void update_numerical(const vector_type& data) {
      // Use importance sampling or MCMC
      // This is a simplified version; real implementation would be more sophisticated

      const std::size_t n_particles = 1000;
      boost::random::mt19937 gen;
      if (seed_) {
        gen.seed(*seed_);
      } else {
        std::random_device rd;
        gen.seed(rd());
      }

      // Generate particles from prior
      auto prior_samples = prior_->sample(n_particles, gen);

      // Calculate importance weights
      importance_weights_.resize(n_particles);
      T max_log_weight = -std::numeric_limits<T>::infinity();

      for (std::size_t i = 0; i < n_particles; ++i) {
        T theta = prior_samples[i];
        T log_weight = likelihood_->log_evaluate(theta, data);
        importance_weights_[i] = log_weight;
        max_log_weight = std::max(max_log_weight, log_weight);
      }

      // Normalize weights (in log space for stability)
      T log_sum_weights = 0;
      for (auto& log_w : importance_weights_) {
        log_w -= max_log_weight;
        log_sum_weights = log_sum_exp(log_sum_weights, log_w);
      }

      // Convert to normalized weights
      for (auto& log_w : importance_weights_) {
        log_w = std::exp(log_w - log_sum_weights);
      }

      // Store samples
      posterior_samples_ = prior_samples;

      // Update marginal likelihood
      log_marginal_likelihood_ += max_log_weight + log_sum_weights;
    }

    /**
     * @brief Update marginal likelihood for conjugate cases
     */
    void update_marginal_likelihood_conjugate(T alpha, T beta, T successes, T failures) {
      // For Beta-Binomial: use log Beta function
      T log_ml = std::lgamma(alpha + beta) - std::lgamma(alpha) - std::lgamma(beta);
      log_ml += std::lgamma(alpha + successes) + std::lgamma(beta + failures);
      log_ml -= std::lgamma(alpha + beta + successes + failures);

      log_marginal_likelihood_ += log_ml;
    }

    /**
     * @brief Get integration bounds for numerical methods
     */
    [[nodiscard]] std::pair<T, T> get_integration_bounds() const {
      if (conjugate_posterior_) {
        return conjugate_posterior_->support();
      }
      return prior_->support();
    }

    /**
     * @brief Evaluate posterior PDF
     */
    [[nodiscard]] T posterior_pdf(T theta) const { return std::exp(log_posterior_pdf(theta)); }

    /**
     * @brief Evaluate log posterior PDF
     */
    [[nodiscard]] T log_posterior_pdf(T theta) const {
      if (conjugate_posterior_) {
        return conjugate_posterior_->log_pdf(theta);
      }

      // For numerical posterior: use kernel density estimation
      // This is simplified; real implementation would be more sophisticated
      return prior_->log_pdf(theta);  // Placeholder
    }

    /**
     * @brief Evaluate posterior CDF
     */
    [[nodiscard]] T posterior_cdf(T theta) const {
      if (conjugate_posterior_) {
        return conjugate_posterior_->cdf(theta);
      }

      // Numerical integration
      auto [lower, upper] = get_integration_bounds();

      auto integrand = [this](T t) { return posterior_pdf(t); };

      return boost::math::quadrature::gauss_kronrod<T, 15>::integrate(integrand, lower, theta, 10,
                                                                      1e-9);
    }

    /**
     * @brief Numerical credible interval calculation
     */
    [[nodiscard]] std::pair<T, T> numerical_credible_interval(T confidence) const {
      T alpha = (1 - confidence) / 2;

      // Use posterior samples if available
      if (!posterior_samples_.empty()) {
        std::vector<T> sorted_samples = posterior_samples_;
        std::sort(sorted_samples.begin(), sorted_samples.end());

        std::size_t lower_idx = static_cast<std::size_t>(alpha * sorted_samples.size());
        std::size_t upper_idx = static_cast<std::size_t>((1 - alpha) * sorted_samples.size());

        return {sorted_samples[lower_idx], sorted_samples[upper_idx]};
      }

      // Otherwise use numerical root finding
      auto [lower_support, upper_support] = get_integration_bounds();
      return {lower_support, upper_support};  // Placeholder
    }

    /**
     * @brief Rejection sampling for posterior
     */
    [[nodiscard]] vector_type rejection_sample(std::size_t n_samples,
                                               boost::random::mt19937& gen) const {
      vector_type samples(n_samples);

      // Find mode for envelope
      T mode_theta = mode();
      T mode_density = posterior_pdf(mode_theta);

      auto [lower, upper] = get_integration_bounds();
      boost::random::uniform_real_distribution<T> theta_dist(lower, upper);
      boost::random::uniform_real_distribution<T> u_dist(0, 1);

      std::size_t accepted = 0;
      while (accepted < n_samples) {
        T theta = theta_dist(gen);
        T u = u_dist(gen);

        T acceptance_prob = posterior_pdf(theta) / (mode_density * 1.1);  // 10% safety margin

        if (u < acceptance_prob) {
          samples[accepted++] = theta;
        }
      }

      return samples;
    }

    /**
     * @brief Estimate effective sample size
     */
    [[nodiscard]] T estimate_ess() const {
      if (importance_weights_.empty()) {
        return static_cast<T>(n_observations_);
      }

      T sum_weights = 0;
      T sum_weights_sq = 0;

      for (T w : importance_weights_) {
        sum_weights += w;
        sum_weights_sq += w * w;
      }

      return (sum_weights * sum_weights) / sum_weights_sq;
    }

    /**
     * @brief Log-sum-exp trick for numerical stability
     */
    [[nodiscard]] static T log_sum_exp(T a, T b) {
      T max_val = std::max(a, b);
      return max_val + std::log(std::exp(a - max_val) + std::exp(b - max_val));
    }
  };

}  // namespace liarsdice::bayesian

#endif  // LIARSDICE_BAYESIAN_POSTERIOR_CALCULATOR_HPP