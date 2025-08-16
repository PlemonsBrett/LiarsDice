#ifndef LIARSDICE_BAYESIAN_LIKELIHOOD_FUNCTION_HPP
#define LIARSDICE_BAYESIAN_LIKELIHOOD_FUNCTION_HPP

#include <boost/function.hpp>
#include <boost/math/distributions.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <concepts>
#include <functional>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace liarsdice::bayesian {

  /**
   * @brief Concept for likelihood function parameters
   *
   * Ensures that parameter types support basic arithmetic operations
   * and can be used in likelihood calculations
   */
  template <typename T>
  concept LikelihoodParameter
      = std::floating_point<T> || (std::is_arithmetic_v<T> && requires(T a, T b) {
          { a + b } -> std::convertible_to<T>;
          { a * b } -> std::convertible_to<T>;
          { a / b } -> std::convertible_to<T>;
        });

  /**
   * @brief Abstract base class for likelihood functions in Bayesian analysis
   *
   * Represents the probability of observing data given parameters.
   * Supports both single observations and vectorized calculations.
   *
   * @tparam T Floating point type for calculations
   */
  template <typename T = double>
    requires std::floating_point<T>
  class LikelihoodFunction {
  public:
    using value_type = T;
    using vector_type = boost::numeric::ublas::vector<T>;
    using function_type = boost::function<T(T, const vector_type&)>;

    virtual ~LikelihoodFunction() = default;

    /**
     * @brief Evaluate likelihood for a single observation
     * @param theta Parameter value
     * @param observation Single data point
     * @return Likelihood value
     */
    [[nodiscard]] virtual T evaluate(T theta, T observation) const = 0;

    /**
     * @brief Evaluate likelihood for multiple observations
     * @param theta Parameter value
     * @param observations Vector of data points
     * @return Combined likelihood value
     */
    [[nodiscard]] virtual T evaluate(T theta, const vector_type& observations) const = 0;

    /**
     * @brief Evaluate log-likelihood for numerical stability
     * @param theta Parameter value
     * @param observation Single data point
     * @return Log-likelihood value
     */
    [[nodiscard]] virtual T log_evaluate(T theta, T observation) const {
      return std::log(evaluate(theta, observation));
    }

    /**
     * @brief Evaluate log-likelihood for multiple observations
     * @param theta Parameter value
     * @param observations Vector of data points
     * @return Combined log-likelihood value
     */
    [[nodiscard]] virtual T log_evaluate(T theta, const vector_type& observations) const {
      return std::log(evaluate(theta, observations));
    }

    /**
     * @brief Get the sufficient statistics for the data
     * @param observations Data points
     * @return Vector of sufficient statistics
     */
    [[nodiscard]] virtual vector_type sufficient_statistics(const vector_type& observations) const
        = 0;

    /**
     * @brief Get the family name of the likelihood
     */
    [[nodiscard]] virtual std::string family() const = 0;

    /**
     * @brief Clone the likelihood function
     */
    [[nodiscard]] virtual std::unique_ptr<LikelihoodFunction<T>> clone() const = 0;

    /**
     * @brief Check if the likelihood has a conjugate prior
     */
    [[nodiscard]] virtual bool has_conjugate_prior() const { return false; }

    /**
     * @brief Get the name of the conjugate prior family (if exists)
     */
    [[nodiscard]] virtual std::optional<std::string> conjugate_prior_family() const {
      return std::nullopt;
    }
  };

  /**
   * @brief Bernoulli likelihood for binary outcomes
   *
   * Models the probability of success in a single trial
   */
  template <typename T = double> class BernoulliLikelihood : public LikelihoodFunction<T> {
  public:
    using typename LikelihoodFunction<T>::vector_type;

    T evaluate(T theta, T observation) const override {
      if (theta < 0 || theta > 1) {
        throw std::domain_error("Bernoulli parameter must be in [0,1]");
      }

      if (observation == 1) {
        return theta;
      } else if (observation == 0) {
        return 1 - theta;
      } else {
        throw std::invalid_argument("Bernoulli observation must be 0 or 1");
      }
    }

    T evaluate(T theta, const vector_type& observations) const override {
      T likelihood = 1.0;
      for (std::size_t i = 0; i < observations.size(); ++i) {
        likelihood *= evaluate(theta, observations[i]);
      }
      return likelihood;
    }

    T log_evaluate(T theta, T observation) const override {
      if (theta < 0 || theta > 1) {
        throw std::domain_error("Bernoulli parameter must be in [0,1]");
      }

      if (observation == 1) {
        return std::log(theta);
      } else if (observation == 0) {
        return std::log(1 - theta);
      } else {
        throw std::invalid_argument("Bernoulli observation must be 0 or 1");
      }
    }

    T log_evaluate(T theta, const vector_type& observations) const override {
      T log_likelihood = 0.0;
      for (std::size_t i = 0; i < observations.size(); ++i) {
        log_likelihood += log_evaluate(theta, observations[i]);
      }
      return log_likelihood;
    }

    vector_type sufficient_statistics(const vector_type& observations) const override {
      vector_type stats(2);
      stats[0] = std::accumulate(observations.begin(), observations.end(), T(0));
      stats[1] = observations.size();
      return stats;
    }

    std::string family() const override { return "bernoulli"; }

    std::unique_ptr<LikelihoodFunction<T>> clone() const override {
      return std::make_unique<BernoulliLikelihood<T>>();
    }

    bool has_conjugate_prior() const override { return true; }

    std::optional<std::string> conjugate_prior_family() const override { return "beta"; }
  };

  /**
   * @brief Binomial likelihood for count data
   *
   * Models the number of successes in a fixed number of trials
   */
  template <typename T = double> class BinomialLikelihood : public LikelihoodFunction<T> {
  public:
    using typename LikelihoodFunction<T>::vector_type;

    explicit BinomialLikelihood(std::size_t n_trials) : n_trials_(n_trials) {}

    T evaluate(T theta, T observation) const override {
      if (theta < 0 || theta > 1) {
        throw std::domain_error("Binomial parameter must be in [0,1]");
      }

      std::size_t k = static_cast<std::size_t>(observation);
      if (k > n_trials_) {
        throw std::invalid_argument("Observation exceeds number of trials");
      }

      boost::math::binomial_distribution<T> dist(n_trials_, theta);
      return boost::math::pdf(dist, k);
    }

    T evaluate(T theta, const vector_type& observations) const override {
      T likelihood = 1.0;
      for (std::size_t i = 0; i < observations.size(); ++i) {
        likelihood *= evaluate(theta, observations[i]);
      }
      return likelihood;
    }

    vector_type sufficient_statistics(const vector_type& observations) const override {
      vector_type stats(2);
      stats[0] = std::accumulate(observations.begin(), observations.end(), T(0));
      stats[1] = observations.size() * n_trials_;
      return stats;
    }

    std::string family() const override { return "binomial"; }

    std::unique_ptr<LikelihoodFunction<T>> clone() const override {
      return std::make_unique<BinomialLikelihood<T>>(n_trials_);
    }

    bool has_conjugate_prior() const override { return true; }

    std::optional<std::string> conjugate_prior_family() const override { return "beta"; }

  private:
    std::size_t n_trials_;
  };

  /**
   * @brief Normal likelihood with known variance
   *
   * Models continuous data with Gaussian noise of known variance
   */
  template <typename T = double> class NormalKnownVarianceLikelihood
      : public LikelihoodFunction<T> {
  public:
    using typename LikelihoodFunction<T>::vector_type;

    explicit NormalKnownVarianceLikelihood(T variance)
        : variance_(variance), std_dev_(std::sqrt(variance)) {
      if (variance <= 0) {
        throw std::invalid_argument("Variance must be positive");
      }
    }

    T evaluate(T theta, T observation) const override {
      boost::math::normal_distribution<T> dist(theta, std_dev_);
      return boost::math::pdf(dist, observation);
    }

    T evaluate(T theta, const vector_type& observations) const override {
      T likelihood = 1.0;
      for (std::size_t i = 0; i < observations.size(); ++i) {
        likelihood *= evaluate(theta, observations[i]);
      }
      return likelihood;
    }

    T log_evaluate(T theta, T observation) const override {
      T z = (observation - theta) / std_dev_;
      return -0.5 * std::log(2 * M_PI) - std::log(std_dev_) - 0.5 * z * z;
    }

    T log_evaluate(T theta, const vector_type& observations) const override {
      T log_likelihood = 0.0;
      for (std::size_t i = 0; i < observations.size(); ++i) {
        log_likelihood += log_evaluate(theta, observations[i]);
      }
      return log_likelihood;
    }

    vector_type sufficient_statistics(const vector_type& observations) const override {
      vector_type stats(2);
      stats[0] = std::accumulate(observations.begin(), observations.end(), T(0));
      stats[1] = observations.size();
      return stats;
    }

    std::string family() const override { return "normal_known_variance"; }

    std::unique_ptr<LikelihoodFunction<T>> clone() const override {
      return std::make_unique<NormalKnownVarianceLikelihood<T>>(variance_);
    }

    bool has_conjugate_prior() const override { return true; }

    std::optional<std::string> conjugate_prior_family() const override { return "normal"; }

    /**
     * @brief Get the known variance
     */
    T variance() const { return variance_; }

  private:
    T variance_;
    T std_dev_;
  };

  /**
   * @brief Poisson likelihood for count data
   *
   * Models the number of events in a fixed interval
   */
  template <typename T = double> class PoissonLikelihood : public LikelihoodFunction<T> {
  public:
    using typename LikelihoodFunction<T>::vector_type;

    T evaluate(T theta, T observation) const override {
      if (theta <= 0) {
        throw std::domain_error("Poisson parameter must be positive");
      }

      std::size_t k = static_cast<std::size_t>(observation);
      boost::math::poisson_distribution<T> dist(theta);
      return boost::math::pdf(dist, k);
    }

    T evaluate(T theta, const vector_type& observations) const override {
      T likelihood = 1.0;
      for (std::size_t i = 0; i < observations.size(); ++i) {
        likelihood *= evaluate(theta, observations[i]);
      }
      return likelihood;
    }

    vector_type sufficient_statistics(const vector_type& observations) const override {
      vector_type stats(2);
      stats[0] = std::accumulate(observations.begin(), observations.end(), T(0));
      stats[1] = observations.size();
      return stats;
    }

    std::string family() const override { return "poisson"; }

    std::unique_ptr<LikelihoodFunction<T>> clone() const override {
      return std::make_unique<PoissonLikelihood<T>>();
    }

    bool has_conjugate_prior() const override { return true; }

    std::optional<std::string> conjugate_prior_family() const override { return "gamma"; }
  };

  /**
   * @brief Custom likelihood function wrapper
   *
   * Allows users to define custom likelihood functions using boost::function
   */
  template <typename T = double> class CustomLikelihood : public LikelihoodFunction<T> {
  public:
    using typename LikelihoodFunction<T>::vector_type;
    using typename LikelihoodFunction<T>::function_type;

    CustomLikelihood(function_type likelihood_fn, const std::string& name = "custom")
        : likelihood_fn_(likelihood_fn), name_(name) {}

    T evaluate(T theta, T observation) const override {
      vector_type obs(1);
      obs[0] = observation;
      return likelihood_fn_(theta, obs);
    }

    T evaluate(T theta, const vector_type& observations) const override {
      return likelihood_fn_(theta, observations);
    }

    vector_type sufficient_statistics(const vector_type& observations) const override {
      // For custom likelihood, return raw data as sufficient statistics
      return observations;
    }

    std::string family() const override { return name_; }

    std::unique_ptr<LikelihoodFunction<T>> clone() const override {
      return std::make_unique<CustomLikelihood<T>>(likelihood_fn_, name_);
    }

  private:
    function_type likelihood_fn_;
    std::string name_;
  };

  /**
   * @brief Factory function for creating common likelihood functions
   */
  template <typename T = double>
  std::unique_ptr<LikelihoodFunction<T>> create_likelihood(const std::string& family_name,
                                                           const std::vector<T>& parameters = {}) {
    if (family_name == "bernoulli") {
      return std::make_unique<BernoulliLikelihood<T>>();
    } else if (family_name == "binomial" && !parameters.empty()) {
      return std::make_unique<BinomialLikelihood<T>>(static_cast<std::size_t>(parameters[0]));
    } else if (family_name == "normal_known_variance" && !parameters.empty()) {
      return std::make_unique<NormalKnownVarianceLikelihood<T>>(parameters[0]);
    } else if (family_name == "poisson") {
      return std::make_unique<PoissonLikelihood<T>>();
    }

    throw std::invalid_argument("Unknown likelihood family or missing parameters");
  }

}  // namespace liarsdice::bayesian

#endif  // LIARSDICE_BAYESIAN_LIKELIHOOD_FUNCTION_HPP