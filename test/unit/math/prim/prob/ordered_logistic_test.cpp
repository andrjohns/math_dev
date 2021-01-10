#include <stan/math/prim.hpp>
#include <gtest/gtest.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/math/distributions.hpp>
#include <limits>
#include <vector>

stan::math::vector_d get_simplex_inv_logit(double lambda,
                                           const stan::math::vector_d& c) {
  using stan::math::inv_logit;
  int K = c.size() + 1;
  stan::math::vector_d theta(K);
  theta(0) = 1.0 - inv_logit(lambda - c(0));
  for (int k = 1; k < (K - 1); ++k)
    theta(k) = inv_logit(lambda - c(k - 1)) - inv_logit(lambda - c(k));
  // - 0.0
  theta(K - 1) = inv_logit(lambda - c(K - 2));
  return theta;
}

TEST(ProbDistributions, ordered_logistic_vals) {
  using Eigen::Dynamic;
  using Eigen::Matrix;

  using stan::math::inv_logit;
  using stan::math::ordered_logistic_log;

  std::vector<int> y{1, 2, 3, 4, 5};
  std::vector<int> zero{1, 2, 0, 4, 5};
  std::vector<int> six{1, 2, 6, 4, 5};
  int K = 5;
  Matrix<double, Dynamic, 1> c(K - 1);
  c << -1.7, -0.3, 1.2, 2.6;

  Matrix<double, Dynamic, 1> lambda(K);
  lambda << 1.1, 1.1, 1.1, 1.1, 1.1;

  stan::math::vector_d theta = get_simplex_inv_logit(lambda[0], c);

  double sum = 0.0;
  double log_sum = 0.0;
  for (int k = 0; k < theta.size(); ++k) {
    sum += theta(k);
    log_sum += log(theta(k));
  }
  EXPECT_FLOAT_EQ(1.0, sum);

  for (int k = 0; k < K; ++k)
    EXPECT_FLOAT_EQ(log(theta(k)), ordered_logistic_log(k + 1, lambda[k], c));

  EXPECT_FLOAT_EQ(log_sum, ordered_logistic_log(y, lambda, c));

  EXPECT_THROW(ordered_logistic_log(0, lambda[0], c), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(6, lambda[0], c), std::domain_error);

  EXPECT_THROW(ordered_logistic_log(zero, lambda, c), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(six, lambda, c), std::domain_error);
}

TEST(ProbDistributions, ordered_logistic_vals_2) {
  using Eigen::Dynamic;
  using Eigen::Matrix;

  using stan::math::inv_logit;
  using stan::math::ordered_logistic_log;

  std::vector<int> y{1, 2, 3};
  std::vector<int> zero{1, 0, 3};
  std::vector<int> six{1, 6, 3};

  int K = 3;
  Matrix<double, Dynamic, 1> c(K - 1);
  c << -0.2, 4;
  Matrix<double, Dynamic, 1> lambda(K);
  lambda << -0.9, -0.9, -0.9;

  stan::math::vector_d theta = get_simplex_inv_logit(lambda[0], c);

  double sum = 0.0;
  double log_sum = 0.0;
  for (int k = 0; k < theta.size(); ++k) {
    sum += theta(k);
    log_sum += log(theta(k));
  }
  EXPECT_FLOAT_EQ(1.0, sum);

  for (int k = 0; k < K; ++k)
    EXPECT_FLOAT_EQ(log(theta(k)), ordered_logistic_log(k + 1, lambda[0], c));

  EXPECT_FLOAT_EQ(log_sum, ordered_logistic_log(y, lambda, c));

  EXPECT_THROW(ordered_logistic_log(0, lambda[0], c), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(4, lambda[0], c), std::domain_error);

  EXPECT_THROW(ordered_logistic_log(zero, lambda, c), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(six, lambda, c), std::domain_error);
}

TEST(ProbDistributions, ordered_logistic) {
  using stan::math::ordered_logistic_log;
  std::vector<int> y{1, 1, 1, 1};
  int K = 4;
  Eigen::Matrix<double, Eigen::Dynamic, 1> c(K - 1);
  c << -0.3, 0.1, 1.2;
  Eigen::Matrix<double, Eigen::Dynamic, 1> lambda(K);
  lambda << 0.5, 0.5, 0.5, 0.5;

  // init size zero
  Eigen::Matrix<double, Eigen::Dynamic, 1> c_zero;
  EXPECT_EQ(0, c_zero.size());
  EXPECT_THROW(ordered_logistic_log(1, lambda[0], c_zero),
               std::invalid_argument);
  EXPECT_THROW(ordered_logistic_log(y, lambda, c_zero), std::invalid_argument);

  Eigen::Matrix<double, Eigen::Dynamic, 1> c_neg(1);
  c_neg << -13.7;
  EXPECT_NO_THROW(ordered_logistic_log(1, lambda[0], c_neg));

  Eigen::Matrix<double, Eigen::Dynamic, 1> c_unord(3);
  c_unord << 1.0, 0.4, 2.0;
  EXPECT_THROW(ordered_logistic_log(1, lambda[0], c_unord), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(y, lambda, c_unord), std::domain_error);

  double nan = std::numeric_limits<double>::quiet_NaN();
  double inf = std::numeric_limits<double>::infinity();

  Eigen::Matrix<double, Eigen::Dynamic, 1> nan_vec(4);
  nan_vec << nan, nan, nan, nan;

  Eigen::Matrix<double, Eigen::Dynamic, 1> inf_vec(4);
  inf_vec << inf, inf, inf, inf;

  EXPECT_THROW(ordered_logistic_log(1, nan, c), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(1, inf, c), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(y, nan_vec, c), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(y, inf_vec, c), std::domain_error);

  Eigen::Matrix<double, Eigen::Dynamic, 1> cbad(2);
  cbad << 0.2, inf;
  EXPECT_THROW(ordered_logistic_log(1, 1.0, cbad), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(y, lambda, cbad), std::domain_error);
  cbad[1] = nan;
  EXPECT_THROW(ordered_logistic_log(1, 1.0, cbad), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(y, lambda, cbad), std::domain_error);

  Eigen::Matrix<double, Eigen::Dynamic, 1> cbad1(1);
  cbad1 << inf;
  EXPECT_THROW(ordered_logistic_log(1, 1.0, cbad1), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(y, lambda, cbad1), std::domain_error);
  cbad1[0] = nan;
  EXPECT_THROW(ordered_logistic_log(1, 1.0, cbad1), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(y, lambda, cbad1), std::domain_error);

  Eigen::Matrix<double, Eigen::Dynamic, 1> cbad3(3);
  cbad3 << 0.5, inf, 1.0;
  EXPECT_THROW(ordered_logistic_log(1, 1.0, cbad3), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(y, lambda, cbad3), std::domain_error);
  cbad3[1] = nan;
  EXPECT_THROW(ordered_logistic_log(1, 1.0, cbad3), std::domain_error);
  EXPECT_THROW(ordered_logistic_log(y, lambda, cbad3), std::domain_error);

  Eigen::Matrix<double, Eigen::Dynamic, 1> lambda_small(3);
  lambda_small << 1, 1, 1;
  EXPECT_THROW(ordered_logistic_log(y, lambda_small, c), std::invalid_argument);

  Eigen::Matrix<double, Eigen::Dynamic, 1> c_small(K - 2);
  c_small << -0.3, 0.1;
  std::vector<Eigen::Matrix<double, Eigen::Dynamic, 1>> c_small_vec(4);
  c_small_vec[0] = c;
  c_small_vec[1] = c;
  c_small_vec[2] = c_small;
  c_small_vec[3] = c;
  EXPECT_THROW(ordered_logistic_log(y, lambda, c_small_vec),
               std::invalid_argument);
}

TEST(ProbDistributionOrderedLogistic, error_check) {
  boost::random::mt19937 rng;
  double inf = std::numeric_limits<double>::infinity();
  Eigen::VectorXd c(4);
  c << -2, 2.0, 5, 10;
  EXPECT_NO_THROW(stan::math::ordered_logistic_rng(4.0, c, rng));

  EXPECT_THROW(
      stan::math::ordered_logistic_rng(stan::math::positive_infinity(), c, rng),
      std::domain_error);
  c << -inf, 2.0, -5, inf;
  EXPECT_THROW(stan::math::ordered_logistic_rng(4.0, c, rng),
               std::domain_error);

  c << -2, 5, 2.0, 10;
  EXPECT_THROW(stan::math::ordered_logistic_rng(4.0, c, rng),
               std::domain_error);
}

TEST(ProbDistributionOrderedLogistic, chiSquareGoodnessFitTest) {
  using stan::math::inv_logit;
  boost::random::mt19937 rng;
  int N = 10000;
  double eta = 1.0;
  Eigen::VectorXd theta(3);
  theta << -0.4, 4.0, 6.2;
  Eigen::VectorXd prob(4);
  prob(0) = 1 - inv_logit(eta - theta(0));
  prob(1) = inv_logit(eta - theta(0)) - inv_logit(eta - theta(1));
  prob(2) = inv_logit(eta - theta(1)) - inv_logit(eta - theta(2));
  prob(3) = inv_logit(eta - theta(2));
  int K = prob.rows();
  boost::math::chi_squared mydist(K - 1);

  Eigen::VectorXd loc(prob.rows());
  for (int i = 0; i < prob.rows(); i++)
    loc(i) = 0;

  for (int i = 0; i < prob.rows(); i++) {
    for (int j = i; j < prob.rows(); j++)
      loc(j) += prob(i);
  }

  int count = 0;
  int bin[K];
  double expect[K];
  for (int i = 0; i < K; i++) {
    bin[i] = 0;
    expect[i] = N * prob(i);
  }

  while (count < N) {
    int a = stan::math::ordered_logistic_rng(eta, theta, rng);
    bin[a - 1]++;
    count++;
  }

  double chi = 0;

  for (int j = 0; j < K; j++)
    chi += ((bin[j] - expect[j]) * (bin[j] - expect[j]) / expect[j]);
  EXPECT_TRUE(chi < quantile(complement(mydist, 1e-6)));
}

TEST(ProbDistributions, ordered_logistic_vecRNG) {
  using stan::math::ordered_logistic_rng;
  boost::random::mt19937 rng;
  Eigen::VectorXd eta_vec(3);
  eta_vec << -2.5, 1.2, 3.1;

  Eigen::VectorXd cut1(4);
  cut1 << -3.1, -1.5, 0.4, 1.2;
  Eigen::VectorXd cut2(4);
  cut2 << -1.2, -0.4, 1.6, 2.1;
  Eigen::VectorXd cut3(4);
  cut3 << -4.1, -3.5, 1.4, 4.2;

  std::vector<Eigen::VectorXd> cuts{cut1, cut2, cut3};

  std::vector<int> rng_vec_stvec = ordered_logistic_rng(eta_vec, cuts, rng);
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(rng_vec_stvec[i], ordered_logistic_rng(eta_vec[i], cuts[i], rng));
  }

  boost::random::mt19937 rng2;
  std::vector<int> rng_real_stvec
      = ordered_logistic_rng(eta_vec[0], cuts, rng2);
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(rng_real_stvec[i],
              ordered_logistic_rng(eta_vec[0], cuts[i], rng2));
  }

  boost::random::mt19937 rng3;
  std::vector<int> rng_vec_vec = ordered_logistic_rng(eta_vec, cuts[0], rng3);
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(rng_vec_vec[i], ordered_logistic_rng(eta_vec[i], cuts[0], rng3));
  }
}

TEST(ProbDistributions, ordered_logistic_vecRNG_throw) {
  using stan::math::ordered_logistic_rng;
  boost::random::mt19937 rng;

  Eigen::VectorXd eta_vec(3);
  eta_vec << -2.5, 1.2, 3.1;

  Eigen::VectorXd cut1(4);
  cut1 << -3.1, -1.5, 0.4, 1.2;

  std::vector<Eigen::VectorXd> cuts{cut1, cut1, cut1};

  Eigen::VectorXd eig_vec_zero(0);
  std::vector<Eigen::VectorXd> std_vec_zero(0);

  // Eta must have more than zero elements
  EXPECT_THROW(ordered_logistic_rng(eig_vec_zero, cuts, rng),
               std::domain_error);

  // Cut points must be larger than size zero
  EXPECT_THROW(ordered_logistic_rng(eta_vec, std_vec_zero, rng),
               std::domain_error);

  EXPECT_THROW(ordered_logistic_rng(eig_vec_zero, std_vec_zero, rng),
               std::domain_error);

  // Consistent sizes
  {
    Eigen::VectorXd etas(2);
    etas << 1.0, 2.0;
    Eigen::VectorXd cuts(4);
    cuts << 1.0, 2.0, 3.0, 4.0;
    std::vector<Eigen::VectorXd> svec_cuts = {cuts, cuts};
    EXPECT_NO_THROW(ordered_logistic_rng(etas, svec_cuts, rng));
    svec_cuts = {cuts, cuts, cuts};
    EXPECT_THROW(ordered_logistic_rng(etas, svec_cuts, rng),
                 std::invalid_argument);
  }

  // Size of cutpoints in vector
  {
    std::vector<Eigen::VectorXd> svec_zero = {eig_vec_zero};
    EXPECT_THROW(ordered_logistic_rng(1.0, svec_zero, rng), std::domain_error);
  }

  // Cutpoints ordered in vector
  {
    Eigen::VectorXd cuts(4);
    cuts << 1.0, 0.0, 2.0, 3.0;
    std::vector<Eigen::VectorXd> svec_cuts = {cuts};
    EXPECT_THROW(ordered_logistic_rng(1.0, svec_cuts, rng), std::domain_error);
  }

  double inf = std::numeric_limits<double>::infinity();

  // Cutpoints right end finite
  {
    Eigen::VectorXd cuts(4);
    cuts << 1.0, 2.0, 3.0, inf;
    std::vector<Eigen::VectorXd> svec_cuts = {cuts};
    EXPECT_THROW(ordered_logistic_rng(1.0, svec_cuts, rng), std::domain_error);
  }

  // Cutpoints left end finite
  {
    Eigen::VectorXd cuts(4);
    cuts << -inf, 1.0, 2.0, 3.0;
    std::vector<Eigen::VectorXd> svec_cuts = {cuts};
    EXPECT_THROW(ordered_logistic_rng(1.0, svec_cuts, rng), std::domain_error);
  }

  // Locations finite
  {
    Eigen::VectorXd etas(2);
    etas << inf, inf;
    Eigen::VectorXd cuts(4);
    cuts << 1.0, 2.0, 3.0, 4.0;
    std::vector<Eigen::VectorXd> svec_cuts = {cuts, cuts};
    EXPECT_THROW(ordered_logistic_rng(etas, svec_cuts, rng), std::domain_error);
  }
}
