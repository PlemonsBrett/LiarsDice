#define BOOST_TEST_MODULE BayesianTests
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/dataset.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/random.hpp>
#include <liarsdice/bayesian/bayesian_analyzer.hpp>
#include <liarsdice/bayesian/prior_distribution.hpp>
#include <liarsdice/bayesian/likelihood_function.hpp>
#include <liarsdice/bayesian/posterior_calculator.hpp>
#include <cmath>
#include <vector>

namespace bdata = boost::unit_test::data;
using namespace liarsdice::bayesian;

BOOST_AUTO_TEST_SUITE(BayesianInferenceTests)

BOOST_AUTO_TEST_SUITE(PriorDistributionTests)

BOOST_AUTO_TEST_CASE(BetaPriorBasicTests) {
    // Test Beta(2, 5) prior
    BetaPrior<double> beta_prior(2.0, 5.0);
    
    // Test mean
    double expected_mean = 2.0 / (2.0 + 5.0);
    BOOST_CHECK_CLOSE(beta_prior.mean(), expected_mean, 0.001);
    
    // Test variance
    double alpha = 2.0, beta = 5.0;
    double expected_var = (alpha * beta) / ((alpha + beta) * (alpha + beta) * (alpha + beta + 1));
    BOOST_CHECK_CLOSE(beta_prior.variance(), expected_var, 0.001);
    
    // Test mode
    auto mode = beta_prior.mode();
    BOOST_REQUIRE(mode.has_value());
    double expected_mode = (alpha - 1) / (alpha + beta - 2);
    BOOST_CHECK_CLOSE(*mode, expected_mode, 0.001);
    
    // Test PDF
    BOOST_CHECK_CLOSE(beta_prior.pdf(0.3), 1.6537, 0.001);
    BOOST_CHECK_EQUAL(beta_prior.pdf(-0.1), 0.0);
    BOOST_CHECK_EQUAL(beta_prior.pdf(1.1), 0.0);
    
    // Test CDF
    BOOST_CHECK_EQUAL(beta_prior.cdf(0.0), 0.0);
    BOOST_CHECK_EQUAL(beta_prior.cdf(1.0), 1.0);
    BOOST_CHECK_GT(beta_prior.cdf(0.5), beta_prior.cdf(0.3));
    
    // Test support
    auto [lower, upper] = beta_prior.support();
    BOOST_CHECK_EQUAL(lower, 0.0);
    BOOST_CHECK_EQUAL(upper, 1.0);
    
    // Test conjugacy
    BOOST_CHECK(beta_prior.is_conjugate_to("bernoulli"));
    BOOST_CHECK(beta_prior.is_conjugate_to("binomial"));
    BOOST_CHECK(!beta_prior.is_conjugate_to("normal"));
}

BOOST_AUTO_TEST_CASE(NormalPriorTests) {
    NormalPrior<double> normal_prior(10.0, 2.0);
    
    BOOST_CHECK_EQUAL(normal_prior.mean(), 10.0);
    BOOST_CHECK_EQUAL(normal_prior.variance(), 4.0);
    
    auto mode = normal_prior.mode();
    BOOST_REQUIRE(mode.has_value());
    BOOST_CHECK_EQUAL(*mode, 10.0);
    
    // Test PDF is maximum at mean
    BOOST_CHECK_GT(normal_prior.pdf(10.0), normal_prior.pdf(8.0));
    BOOST_CHECK_GT(normal_prior.pdf(10.0), normal_prior.pdf(12.0));
    
    // Test conjugacy
    BOOST_CHECK(normal_prior.is_conjugate_to("normal_known_variance"));
    BOOST_CHECK(!normal_prior.is_conjugate_to("bernoulli"));
}

BOOST_AUTO_TEST_CASE(GammaPriorTests) {
    GammaPrior<double> gamma_prior(3.0, 2.0); // shape=3, rate=2
    
    BOOST_CHECK_EQUAL(gamma_prior.mean(), 1.5); // shape/rate
    BOOST_CHECK_EQUAL(gamma_prior.variance(), 0.75); // shape/(rate^2)
    
    auto mode = gamma_prior.mode();
    BOOST_REQUIRE(mode.has_value());
    BOOST_CHECK_EQUAL(*mode, 1.0); // (shape-1)/rate
    
    // Test support
    auto [lower, upper] = gamma_prior.support();
    BOOST_CHECK_EQUAL(lower, 0.0);
    BOOST_CHECK(std::isinf(upper));
    
    // Test conjugacy
    BOOST_CHECK(gamma_prior.is_conjugate_to("poisson"));
    BOOST_CHECK(gamma_prior.is_conjugate_to("exponential"));
}

BOOST_AUTO_TEST_CASE(UniformPriorTests) {
    UniformPrior<double> uniform_prior(0.0, 10.0);
    
    BOOST_CHECK_EQUAL(uniform_prior.mean(), 5.0);
    BOOST_CHECK_CLOSE(uniform_prior.variance(), 100.0/12.0, 0.001);
    
    // Test PDF is constant
    BOOST_CHECK_EQUAL(uniform_prior.pdf(3.0), 0.1);
    BOOST_CHECK_EQUAL(uniform_prior.pdf(7.0), 0.1);
    BOOST_CHECK_EQUAL(uniform_prior.pdf(-1.0), 0.0);
    BOOST_CHECK_EQUAL(uniform_prior.pdf(11.0), 0.0);
    
    // Uniform has no unique mode
    BOOST_CHECK(!uniform_prior.mode().has_value());
}

BOOST_AUTO_TEST_CASE(PriorSamplingTests) {
    boost::random::mt19937 gen(42);
    
    // Test Beta sampling
    BetaPrior<double> beta_prior(2.0, 5.0);
    auto beta_samples = beta_prior.sample(1000, gen);
    
    // All samples should be in [0, 1]
    for (auto sample : beta_samples) {
        BOOST_CHECK(sample >= 0.0 && sample <= 1.0);
    }
    
    // Sample mean should be close to theoretical mean
    double sample_mean = std::accumulate(beta_samples.begin(), beta_samples.end(), 0.0) / beta_samples.size();
    BOOST_CHECK_CLOSE(sample_mean, beta_prior.mean(), 10.0); // 10% tolerance
}

BOOST_AUTO_TEST_SUITE_END() // PriorDistributionTests

BOOST_AUTO_TEST_SUITE(LikelihoodFunctionTests)

BOOST_AUTO_TEST_CASE(BernoulliLikelihoodTests) {
    BernoulliLikelihood<double> likelihood;
    
    // Test single observation
    BOOST_CHECK_EQUAL(likelihood.evaluate(0.7, 1.0), 0.7);
    BOOST_CHECK_EQUAL(likelihood.evaluate(0.7, 0.0), 0.3);
    
    // Test multiple observations
    boost::numeric::ublas::vector<double> observations(4);
    observations[0] = 1; observations[1] = 0; observations[2] = 1; observations[3] = 1;
    
    double expected = 0.7 * 0.3 * 0.7 * 0.7;
    BOOST_CHECK_CLOSE(likelihood.evaluate(0.7, observations), expected, 0.001);
    
    // Test sufficient statistics
    auto stats = likelihood.sufficient_statistics(observations);
    BOOST_CHECK_EQUAL(stats[0], 3.0); // sum of successes
    BOOST_CHECK_EQUAL(stats[1], 4.0); // number of trials
    
    // Test conjugacy
    BOOST_CHECK(likelihood.has_conjugate_prior());
    BOOST_CHECK_EQUAL(*likelihood.conjugate_prior_family(), "beta");
}

BOOST_AUTO_TEST_CASE(BinomialLikelihoodTests) {
    BinomialLikelihood<double> likelihood(10); // 10 trials
    
    // Test evaluation
    boost::math::binomial_distribution<double> binom(10, 0.6);
    BOOST_CHECK_CLOSE(likelihood.evaluate(0.6, 7), boost::math::pdf(binom, 7), 0.001);
    
    // Test conjugacy
    BOOST_CHECK(likelihood.has_conjugate_prior());
    BOOST_CHECK_EQUAL(*likelihood.conjugate_prior_family(), "beta");
}

BOOST_AUTO_TEST_CASE(NormalKnownVarianceLikelihoodTests) {
    NormalKnownVarianceLikelihood<double> likelihood(4.0); // variance = 4
    
    // Test evaluation
    boost::math::normal_distribution<double> normal(5.0, 2.0); // mean=5, sd=2
    BOOST_CHECK_CLOSE(likelihood.evaluate(5.0, 7.0), boost::math::pdf(normal, 7.0), 0.001);
    
    // Test log evaluation
    double log_like = likelihood.log_evaluate(5.0, 7.0);
    BOOST_CHECK_CLOSE(log_like, std::log(likelihood.evaluate(5.0, 7.0)), 0.001);
    
    // Test conjugacy
    BOOST_CHECK(likelihood.has_conjugate_prior());
    BOOST_CHECK_EQUAL(*likelihood.conjugate_prior_family(), "normal");
}

BOOST_AUTO_TEST_CASE(PoissonLikelihoodTests) {
    PoissonLikelihood<double> likelihood;
    
    // Test evaluation
    boost::math::poisson_distribution<double> poisson(3.5);
    BOOST_CHECK_CLOSE(likelihood.evaluate(3.5, 4), boost::math::pdf(poisson, 4), 0.001);
    
    // Test sufficient statistics
    boost::numeric::ublas::vector<double> observations(3);
    observations[0] = 2; observations[1] = 4; observations[2] = 3;
    
    auto stats = likelihood.sufficient_statistics(observations);
    BOOST_CHECK_EQUAL(stats[0], 9.0); // sum
    BOOST_CHECK_EQUAL(stats[1], 3.0); // count
    
    // Test conjugacy
    BOOST_CHECK(likelihood.has_conjugate_prior());
    BOOST_CHECK_EQUAL(*likelihood.conjugate_prior_family(), "gamma");
}

BOOST_AUTO_TEST_SUITE_END() // LikelihoodFunctionTests

BOOST_AUTO_TEST_SUITE(BayesianAnalyzerTests)

BOOST_AUTO_TEST_CASE(BetaBernoulliConjugateUpdateTest) {
    // Classic Beta-Bernoulli conjugate example
    BayesianAnalyzer<double> analyzer;
    
    // Set Beta(1, 1) uniform prior
    auto prior = std::make_shared<BetaPrior<double>>(1.0, 1.0);
    analyzer.set_prior(prior);
    
    // Set Bernoulli likelihood
    auto likelihood = std::make_shared<BernoulliLikelihood<double>>();
    analyzer.set_likelihood(likelihood);
    
    // Observe data: 7 successes, 3 failures
    boost::numeric::ublas::vector<double> data(10);
    for (int i = 0; i < 7; ++i) data[i] = 1.0;
    for (int i = 7; i < 10; ++i) data[i] = 0.0;
    
    // Update
    auto posterior = analyzer.update(data);
    
    // Posterior should be Beta(8, 4)
    double expected_mean = 8.0 / 12.0;
    BOOST_CHECK_CLOSE(analyzer.posterior_mean(), expected_mean, 0.001);
    
    // Test credible interval
    auto [lower, upper] = analyzer.credible_interval(0.95);
    BOOST_CHECK(lower > 0.3 && lower < 0.5);
    BOOST_CHECK(upper > 0.8 && upper < 0.95);
}

BOOST_AUTO_TEST_CASE(GammaPoissonConjugateUpdateTest) {
    BayesianAnalyzer<double> analyzer;
    
    // Gamma(2, 1) prior
    auto prior = std::make_shared<GammaPrior<double>>(2.0, 1.0);
    analyzer.set_prior(prior);
    
    // Poisson likelihood
    auto likelihood = std::make_shared<PoissonLikelihood<double>>();
    analyzer.set_likelihood(likelihood);
    
    // Observe data
    boost::numeric::ublas::vector<double> data(5);
    data[0] = 3; data[1] = 5; data[2] = 2; data[3] = 4; data[4] = 3;
    double data_sum = 17.0;
    
    analyzer.update(data);
    
    // Posterior should be Gamma(2 + 17, 1 + 5) = Gamma(19, 6)
    double expected_mean = 19.0 / 6.0;
    BOOST_CHECK_CLOSE(analyzer.posterior_mean(), expected_mean, 0.001);
}

BOOST_AUTO_TEST_CASE(ModelComparisonTest) {
    // Compare two models with different priors
    BayesianAnalyzer<double> model1, model2;
    
    // Model 1: Optimistic prior Beta(8, 2)
    model1.set_prior(std::make_shared<BetaPrior<double>>(8.0, 2.0));
    model1.set_likelihood(std::make_shared<BernoulliLikelihood<double>>());
    
    // Model 2: Pessimistic prior Beta(2, 8)
    model2.set_prior(std::make_shared<BetaPrior<double>>(2.0, 8.0));
    model2.set_likelihood(std::make_shared<BernoulliLikelihood<double>>());
    
    // Observe mostly successes
    boost::numeric::ublas::vector<double> data(10);
    for (int i = 0; i < 8; ++i) data[i] = 1.0;
    for (int i = 8; i < 10; ++i) data[i] = 0.0;
    
    double bayes_factor = BayesianAnalyzer<double>::bayes_factor(model1, model2, data);
    
    // Model 1 should be favored
    BOOST_CHECK_GT(bayes_factor, 1.0);
}

BOOST_AUTO_TEST_CASE(DiagnosticsTest) {
    BayesianAnalyzer<double> analyzer;
    
    analyzer.set_prior(std::make_shared<BetaPrior<double>>(1.0, 1.0));
    analyzer.set_likelihood(std::make_shared<BernoulliLikelihood<double>>());
    
    boost::numeric::ublas::vector<double> data(5);
    for (int i = 0; i < 3; ++i) data[i] = 1.0;
    for (int i = 3; i < 5; ++i) data[i] = 0.0;
    
    analyzer.update(data);
    
    auto diagnostics = analyzer.get_diagnostics();
    BOOST_CHECK_EQUAL(diagnostics.n_observations, 5);
    BOOST_CHECK_GT(diagnostics.information_gain, 0.0);
    BOOST_CHECK_GT(diagnostics.effective_sample_size, 0.0);
}

BOOST_AUTO_TEST_SUITE_END() // BayesianAnalyzerTests

BOOST_AUTO_TEST_SUITE(PosteriorCalculatorTests)

BOOST_AUTO_TEST_CASE(ConjugateUpdateTest) {
    // Test Beta-Bernoulli conjugate update
    auto prior = std::make_shared<BetaPrior<double>>(2.0, 3.0);
    auto likelihood = std::make_shared<BernoulliLikelihood<double>>();
    
    PosteriorCalculator<double> calculator(prior, likelihood);
    
    // Data: 4 successes, 1 failure
    boost::numeric::ublas::vector<double> data(5);
    data[0] = 1; data[1] = 1; data[2] = 0; data[3] = 1; data[4] = 1;
    
    calculator.update(data);
    
    // Posterior should be Beta(2+4, 3+1) = Beta(6, 4)
    double expected_mean = 6.0 / 10.0;
    BOOST_CHECK_CLOSE(calculator.mean(), expected_mean, 0.001);
    
    // Check variance
    double alpha = 6.0, beta = 4.0;
    double expected_var = (alpha * beta) / ((alpha + beta) * (alpha + beta) * (alpha + beta + 1));
    BOOST_CHECK_CLOSE(calculator.variance(), expected_var, 0.001);
}

BOOST_AUTO_TEST_CASE(InformationGainTest) {
    auto prior = std::make_shared<BetaPrior<double>>(1.0, 1.0); // Uniform prior
    auto likelihood = std::make_shared<BernoulliLikelihood<double>>();
    
    PosteriorCalculator<double> calculator(prior, likelihood);
    
    // Strong evidence
    boost::numeric::ublas::vector<double> data(20);
    for (int i = 0; i < 18; ++i) data[i] = 1.0;
    for (int i = 18; i < 20; ++i) data[i] = 0.0;
    
    calculator.update(data);
    
    // Information gain should be positive
    double info_gain = calculator.information_gain();
    BOOST_CHECK_GT(info_gain, 0.0);
    
    // More data should give more information
    boost::numeric::ublas::vector<double> more_data(10);
    for (int i = 0; i < 10; ++i) more_data[i] = 1.0;
    
    calculator.update(more_data);
    double new_info_gain = calculator.information_gain();
    BOOST_CHECK_GT(new_info_gain, info_gain);
}

BOOST_AUTO_TEST_CASE(PredictiveSamplingTest) {
    auto prior = std::make_shared<BetaPrior<double>>(5.0, 5.0);
    auto likelihood = std::make_shared<BernoulliLikelihood<double>>();
    
    PosteriorCalculator<double> calculator(prior, likelihood);
    
    // Get predictive samples
    auto samples = calculator.predictive_sample(1000);
    
    // All samples should be in [0, 1]
    for (std::size_t i = 0; i < samples.size(); ++i) {
        BOOST_CHECK(samples[i] >= 0.0 && samples[i] <= 1.0);
    }
    
    // Sample mean should be close to prior mean
    double sample_mean = std::accumulate(&samples[0], &samples[0] + samples.size(), 0.0) / samples.size();
    BOOST_CHECK_CLOSE(sample_mean, 0.5, 10.0); // 10% tolerance
}

BOOST_AUTO_TEST_SUITE_END() // PosteriorCalculatorTests

BOOST_AUTO_TEST_SUITE_END() // BayesianInferenceTests