#ifndef STAN_MATH_PRIM_MAT_FUN_LOG_MIX_HPP
#define STAN_MATH_PRIM_MAT_FUN_LOG_MIX_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/mat/fun/log_sum_exp.hpp>
#include <stan/math/prim/mat/fun/value_of.hpp>
#include <stan/math/prim/scal/err/check_bounded.hpp>
#include <stan/math/prim/scal/err/check_not_nan.hpp>
#include <stan/math/prim/scal/err/check_consistent_sizes.hpp>
#include <stan/math/prim/scal/err/check_finite.hpp>
#include <vector>

namespace stan {
namespace math {
/**
 * Return the log mixture density given specified mixing proportions
 * and array of log density vectors.
 *
 * \f[
 * \frac{\partial }{\partial p_x}\left[
 * \log\left(\exp^{\log\left(p_1\right)+d_1}+\cdot\cdot\cdot+
 * \exp^{\log\left(p_n\right)+d_n}\right)+
 * \log\left(\exp^{\log\left(p_1\right)+f_1}+\cdot\cdot\cdot+
 * \exp^{\log\left(p_n\right)+f_n}\right)\right]
 * =\frac{e^{d_x}}{e^{d_1}p_1+\cdot\cdot\cdot+e^{d_m}p_m}+
 * \frac{e^{f_x}}{e^{f_1}p_1+\cdot\cdot\cdot+e^{f_m}p_m}
 * \f]
 *
 * \f[
 * \frac{\partial }{\partial d_x}\left[
 * \log\left(\exp^{\log\left(p_1\right)+d_1}+\cdot\cdot\cdot+
 * \exp^{\log\left(p_n\right)+d_n}\right)
 * +\log\left(\exp^{\log\left(p_1\right)+f_1}+\cdot\cdot\cdot+
 * \exp^{\log\left(p_n\right)+f_n}\right)\right]
 * =\frac{e^{d_x}p_x}{e^{d_1}p_1+\cdot\cdot\cdot+e^{d_m}p_m}
 * \f]
 *
 * @param theta vector of mixing proportions in [0, 1].
 * @param lambda array containing vectors of log densities.
 * @return log mixture of densities in specified proportion
 */
template <typename T_theta, typename T_lam>
return_type_t<T_theta, T_lam>
log_mix(const T_theta& theta, const T_lam& lambda) {
  static const char* function = "log_mix";
  typedef typename stan::partials_return_type<T_theta, T_lam>
      T_partials_return;
  typedef typename Eigen::Matrix<T_partials_return, -1, 1> T_partials_vec;
  typedef typename Eigen::Matrix<T_partials_return, -1, -1> T_partials_mat;

  const int N = length_mvt(lambda);
  const int M = length(theta);

  scalar_seq_view<T_theta> theta_vec(theta);
  T_partials_vec theta_dbl(M);
  for (int m = 0; m < M; ++m)
    theta_dbl[m] = value_of(theta_vec[m]);

  T_partials_mat lam_dbl(M, N);
  vector_seq_view<T_lam> lam_vec(lambda);
  for (int n = 0; n < N; ++n)
    for (int m = 0; m < M; ++m)
      lam_dbl(m, n) = value_of(lam_vec[n][m]);

  check_bounded(function, "theta", theta, 0, 1);
  check_not_nan(function, "theta", theta);
  check_finite(function, "theta", theta);
  for (int n = 0; n < N; ++n) {
    check_not_nan(function, "lambda", lam_vec[n]);
    check_finite(function, "lambda", lam_vec[n]);
    check_consistent_sizes(function, "theta", theta, "lambda", lam_vec[n]);
  }

  T_partials_vec logp(N);
  for (int n = 0; n < N; ++n)
    logp[n] = log_sum_exp((lam_dbl.array().colwise()
                            + theta_dbl.array().log()).matrix().col(n).eval());

  operands_and_partials<T_theta, T_lam> ops_partials(theta, lambda);
  if (!is_constant_all<T_theta>::value || !is_constant_all<T_lam>::value) {
    T_partials_mat derivs = (lam_dbl.array().rowwise()
                              - logp.transpose().array()).exp();
    if (!is_constant_all<T_theta>::value) {
        Eigen::Map<T_partials_vec>(ops_partials.edge1_.partials_.data(), M)
          = derivs.rowwise().sum();
    }

    if (!is_constant_all<T_lam>::value) {
      for (int n = 0; n < N; ++n)
        Eigen::Map<T_partials_vec>
          (ops_partials.edge2_.partials_vec_[n].data(), M)
            += derivs.col(n).cwiseProduct(theta_dbl);
    }
  }
  return ops_partials.build(logp.sum());
}
}  // namespace math
}  // namespace stan
#endif
