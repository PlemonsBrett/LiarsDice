#ifndef LIARSDICE_BAYESIAN_BAYESIAN_HPP
#define LIARSDICE_BAYESIAN_BAYESIAN_HPP

/**
 * @file bayesian.hpp
 * @brief Main header file for Bayesian inference components
 * 
 * This header includes all the necessary components for Bayesian analysis:
 * - Prior distributions (Beta, Normal, Gamma, Uniform)
 * - Likelihood functions (Bernoulli, Binomial, Normal, Poisson)
 * - Posterior calculator with conjugate and numerical methods
 * - Bayesian analyzer for complete inference workflows
 */

#include "bayesian_analyzer.hpp"
#include "prior_distribution.hpp"
#include "likelihood_function.hpp"
#include "posterior_calculator.hpp"

namespace liarsdice::bayesian {

/**
 * @brief Example: Simple coin flip analysis
 * 
 * @code
 * // Create analyzer
 * auto analyzer = create_conjugate_analyzer<double>("bernoulli", {{"alpha", 1}, {"beta", 1}});
 * 
 * // Observe data (1 = heads, 0 = tails)
 * boost::numeric::ublas::vector<double> flips(10);
 * // ... fill with observations ...
 * 
 * // Update beliefs
 * auto posterior = analyzer->update(flips);
 * 
 * // Get estimates
 * double prob_heads = analyzer->posterior_mean();
 * auto [lower, upper] = analyzer->credible_interval(0.95);
 * @endcode
 */

/**
 * @brief Example: A/B testing with Bayesian inference
 * 
 * @code
 * // Create analyzers for each variant
 * BayesianAnalyzer<double> variant_a, variant_b;
 * 
 * // Set uninformative priors
 * auto prior = std::make_shared<BetaPrior<double>>(1, 1);
 * auto likelihood = std::make_shared<BernoulliLikelihood<double>>();
 * 
 * variant_a.set_prior(prior);
 * variant_a.set_likelihood(likelihood);
 * variant_b.set_prior(prior->clone());
 * variant_b.set_likelihood(likelihood->clone());
 * 
 * // Update with conversion data
 * variant_a.update(conversions_a);
 * variant_b.update(conversions_b);
 * 
 * // Compare variants
 * double bayes_factor = BayesianAnalyzer<double>::bayes_factor(variant_a, variant_b, data);
 * @endcode
 */

/**
 * @brief Example: Parameter estimation with uncertainty
 * 
 * @code
 * // Estimate Poisson rate parameter
 * auto analyzer = create_conjugate_analyzer<double>("poisson", {{"shape", 2}, {"rate", 1}});
 * 
 * // Observe count data
 * boost::numeric::ublas::vector<double> counts = {3, 5, 2, 4, 6, 3, 4};
 * analyzer->update(counts);
 * 
 * // Get posterior estimates
 * double rate_estimate = analyzer->posterior_mean();
 * double uncertainty = std::sqrt(analyzer->posterior_variance());
 * auto hdi = analyzer->highest_density_interval(0.95);
 * @endcode
 */

} // namespace liarsdice::bayesian

#endif // LIARSDICE_BAYESIAN_BAYESIAN_HPP