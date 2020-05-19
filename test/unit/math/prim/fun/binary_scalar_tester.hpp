#include <stan/math/prim.hpp>
#include <gtest/gtest.h>

namespace stan {
namespace test {

/**
 * Implementation function which checks that the binary vectorisation
 * framework returns the same value as the function with scalar inputs,
 * for all valid combinations of scalar/vector/nested vector.
 *
 * @tparam F Type of functor to apply.
 * @tparam T1 Type of first vector.
 * @tparam T2 Type of second vector.
 * @param x First vector input to which operation is applied.
 * @param y Second vector input to which operation is applied.
 * @param f functor to apply to inputs.
 */
template <typename F, typename T1, typename T2>
void binary_scalar_tester_impl(const F& f, const T1& x, const T2& y) {
  auto vec_vec = f(x, y);
  auto vec_scal = f(x, y[1]);
  auto scal_vec = f(x[1], y);
  for (int i = 0; i < x.size(); ++i) {
    EXPECT_FLOAT_EQ(f(x[i], y[i]), vec_vec[i]);
    EXPECT_FLOAT_EQ(f(x[i], y[1]), vec_scal[i]);
    EXPECT_FLOAT_EQ(f(x[1], y[i]), scal_vec[i]);
  }
  std::vector<T1> nest_x{x, x, x};
  std::vector<T2> nest_y{y, y, y};
  auto nestvec_nestvec = f(nest_x, nest_y);
  auto nestvec_scal = f(nest_x, y[1]);
  auto scal_nestvec = f(x[1], nest_y);
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < x.size(); ++j) {
      EXPECT_FLOAT_EQ(f(nest_x[i][j], nest_y[i][j]), nestvec_nestvec[i][j]);
      EXPECT_FLOAT_EQ(f(nest_x[i][j], y[1]), nestvec_scal[i][j]);
      EXPECT_FLOAT_EQ(f(x[1], nest_y[i][j]), scal_nestvec[i][j]);
    }
  }
}

/**
 * Testing framework for checking that the vectorisation of binary
 * functions returns the same results as the binary function with
 * scalar inputs. This framework takes two Eigen column vectors of
 * inputs which are tested, and then transformed to Eigen row vectors
 * and std::vectors to also be tested.
 *
 * @tparam F Type of functor to apply.
 * @tparam T1 Type of first Eigen column-vector.
 * @tparam T2 Type of second Eigen column-vector.
 * @param x First Eigen column-vector input to which operation is applied.
 * @param y Second Eigen column-vector input to which operation is applied.
 * @param f functor to apply to inputs.
 */
template <typename F, typename T1, typename T2,
          require_all_eigen_col_vector_t<T1, T2>...>
void binary_scalar_tester(const F& f, const T1& x, const T2& y) {
  binary_scalar_tester_impl(f, x, y);
  binary_scalar_tester_impl(f, x.transpose(), y.transpose());
  binary_scalar_tester_impl(
      f, std::vector<typename T1::Scalar>(x.data(), x.data() + x.size()),
      std::vector<typename T2::Scalar>(y.data(), y.data() + y.size()));
}

}  // namespace test
}  // namespace stan
