#include <liarsdice/bayesian/bayesian_analyzer.hpp>
#include <liarsdice/bayesian/likelihood_function.hpp>
#include <liarsdice/bayesian/posterior_calculator.hpp>
#include <liarsdice/bayesian/prior_distribution.hpp>

namespace liarsdice::bayesian {

  // Explicit template instantiations for common types
  template class BayesianAnalyzer<float>;
  template class BayesianAnalyzer<double>;
  template class BayesianAnalyzer<long double>;

  // Factory function implementation
  template <> std::shared_ptr<BayesianAnalyzer<double>> create_conjugate_analyzer<double>(
      const std::string& likelihood_family,
      const std::unordered_map<std::string, double>& hyperparameters) {
    auto analyzer = std::make_shared<BayesianAnalyzer<double>>();

    if (likelihood_family == "bernoulli" || likelihood_family == "binomial") {
      // Use Beta prior for Bernoulli/Binomial likelihood
      double alpha = hyperparameters.count("alpha") ? hyperparameters.at("alpha") : 1.0;
      double beta = hyperparameters.count("beta") ? hyperparameters.at("beta") : 1.0;

      analyzer->set_prior(std::make_shared<BetaPrior<double>>(alpha, beta));

      if (likelihood_family == "bernoulli") {
        analyzer->set_likelihood(std::make_shared<BernoulliLikelihood<double>>());
      } else {
        double n_trials = hyperparameters.at("n_trials");
        analyzer->set_likelihood(
            std::make_shared<BinomialLikelihood<double>>(static_cast<std::size_t>(n_trials)));
      }
    } else if (likelihood_family == "normal_known_variance") {
      // Use Normal prior for Normal likelihood with known variance
      double prior_mean
          = hyperparameters.count("prior_mean") ? hyperparameters.at("prior_mean") : 0.0;
      double prior_std = hyperparameters.count("prior_std") ? hyperparameters.at("prior_std") : 1.0;
      double likelihood_var = hyperparameters.at("likelihood_variance");

      analyzer->set_prior(std::make_shared<NormalPrior<double>>(prior_mean, prior_std));
      analyzer->set_likelihood(
          std::make_shared<NormalKnownVarianceLikelihood<double>>(likelihood_var));
    } else if (likelihood_family == "poisson") {
      // Use Gamma prior for Poisson likelihood
      double shape = hyperparameters.count("shape") ? hyperparameters.at("shape") : 1.0;
      double rate = hyperparameters.count("rate") ? hyperparameters.at("rate") : 1.0;

      analyzer->set_prior(std::make_shared<GammaPrior<double>>(shape, rate));
      analyzer->set_likelihood(std::make_shared<PoissonLikelihood<double>>());
    } else {
      throw std::invalid_argument("Unknown likelihood family: " + likelihood_family);
    }

    return analyzer;
  }

  // Explicit instantiation of factory function
  template std::shared_ptr<BayesianAnalyzer<float>> create_conjugate_analyzer<float>(
      const std::string&, const std::unordered_map<std::string, float>&);
  template std::shared_ptr<BayesianAnalyzer<long double>> create_conjugate_analyzer<long double>(
      const std::string&, const std::unordered_map<std::string, long double>&);

}  // namespace liarsdice::bayesian