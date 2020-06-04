#include <test/unit/math/test_ad.hpp>
#include <complex>
#include <limits>
#include <vector>

/*
TEST(mathMixMatFun, acos) {
  using stan::test::expect_unary_vectorized;
  auto f = [](const auto& x1) {
    using stan::math::acos;
    return acos(x1);
  };
  // can't autodiff acos through integers
  for (auto x : stan::test::internal::common_nonzero_args())
    stan::test::expect_unary_vectorized(f, x);
  expect_unary_vectorized(f, -2.2, -0.8, 0.5,
                          1 + std::numeric_limits<double>::epsilon(), 1.5, 3,
                          3.4, 4);
  for (double re : std::vector<double>{-0.2, 0, 0.3}) {
    for (double im : std::vector<double>{-0.3, 0, 0.2}) {
      stan::test::expect_ad(f, std::complex<double>{re, im});
    }
  }
}
*/

TEST(mathMixMatFun, acos) {
  // segfaults
  stan::math::vector_v vec_v = Eigen::VectorXd::Random(10000);
  auto out_v = stan::math::acos(vec_v);

  // Working
  stan::math::vector_fd vec_fd = Eigen::VectorXd::Random(10000);
  auto out_fd = stan::math::acos(vec_fd);

  stan::math::vector_ffd vec_ffd = Eigen::VectorXd::Random(10000);
  auto out_ffd = stan::math::acos(vec_ffd);
}
