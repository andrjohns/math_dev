#ifndef STAN_MATH_PRIM_ARR_FUNCTOR_integrate_1d_tsc_HPP
#define STAN_MATH_PRIM_ARR_FUNCTOR_integrate_1d_tsc_HPP

#include <stan/math/prim/scal/meta/is_constant_struct.hpp>
#include <stan/math/prim/mat/fun/value_of.hpp>
#include <stan/math/rev/scal/fun/value_of.hpp>
#include <stan/math/rev/scal/fun/to_var.hpp>
#include <stan/math/rev/mat/fun/to_var.hpp>
#include <stan/math/rev/arr/fun/to_var.hpp>
#include <stan/math/prim/scal/err/check_finite.hpp>

#include <stan/math/prim/scal/meta/scalar_seq_view.hpp>
#include <stan/math/prim/scal/meta/operands_and_partials.hpp>
#include <stan/math/rev/mat/functor/de_integrator.hpp>
#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <cmath>
#include <ostream>
#include <vector>

namespace stan {

  namespace math {

    /**
     * Return the numeric integral of a function f given its gradient g.
     *
     * @tparam T Type of f.
     * @tparam G Type of g.
     * @param f a functor with signature
     * double (double, std::vector<T_param>) or with signature
     * double (double, T_param) where the first argument is one being
     * integrated and the second one is either an extra scalar or vector
     * being passed to f.
     * @param g a functor with signature
     * double (double, std::vector<T_param>, int, std::ostream*) or with
     * signature double (double, T_param, int, std::ostream*) where the
     * first argument is onebeing integrated and the second one is
     * either an extra scalar or vector being passed to f and the
     * third one selects which component of the gradient vector
     * is to be returned.
     * @param a lower limit of integration, must be double type.
     * @param b upper limit of integration, must be double type.
     * @param tre target relative error.
     * @param tae target absolute error.
     * @param param aditional parameters to be passed to f.
     * @param msgs stream.
     * @return numeric integral of function f.
     */
    template <typename F, typename G, typename T_param>
    inline typename scalar_type<T_param>::type
    integrate_1d_tsc_tscg(const F& f,  const G& g,  const double a,
                          const double b, const T_param& param,
                          std::ostream* msgs, const double tre = 1e-6,
                          const double tae = 1e-6) {
      check_finite("integrate_1d_tsc", "lower limit", a);
      check_finite("integrate_1d_tsc", "upper limit", b);

      using boost::lambda::_1;

      // hard case, we want a normalizing factor
      if (!is_constant_struct<T_param>::value) {
        size_t N = length(param);
        std::vector<double> grad(N);

        auto value_of_param = value_of(param);

        for (size_t n = 0; n < N; n++)
          grad[n] =
            de_integrator(boost::bind<double>(g, _1, value_of_param,
                                              static_cast<int>(n+1),
                                              msgs),
                          a, b, tre, tae);

        double val_ = de_integrator(boost::bind<double>(f, _1,
                                                        value_of_param,
                                                        msgs),
                                    a, b, tre, tae);

        operands_and_partials<T_param> ops_partials(param);
        for (size_t n = 0; n < N; n++)
          ops_partials.edge1_.partials_[n] += grad[n];

        return ops_partials.build(val_);
      // easy case, here we are calculating a normalizing constant,
      // not a normalizing factor, so g doesn't matter at all
      } else {
        return de_integrator(boost::bind<double>(f, _1, value_of(param),
                                                 msgs),
                             a, b, tre, tae);
      }
    }

    /**
     * Calculate gradient of f(x, param, std::ostream*)
     * with respect to param_n (which must be an element of param)
     */
    template <typename F, typename T_param>
    inline double
    gradient_of_f(const F& f, const double x, const T_param& param,
                  const var& param_n, std::ostream* msgs) {
      set_zero_all_adjoints_nested();
      f(x, param, msgs).grad();
      return param_n.adj();
    }

    /**
     * Return the numeric integral of a function f with its
     * gradients being infered automatically (but slowly).
     *
     * @tparam T Type of f.
     * @param f a functor with signature
     * double (double, std::vector<T_param>, std::ostream*) or with
     * signature double (double, T_param, std::ostream*) where the first
     * argument is one being integrated and the second one is either
     * an extra scalar or vector being passed to f.
     * @param a lower limit of integration, must be double type.
     * @param b upper limit of integration, must be double type.
     * @param tre target relative error.
     * @param tae target absolute error.
     * @param param aditional parameters to be passed to f.
     * @param msgs stream.
     * @return numeric integral of function f.
     */
    template <typename F, typename T_param>
    inline typename scalar_type<T_param>::type
    integrate_1d_tsc(const F& f, const double a, const double b,
                     const T_param& param, std::ostream* msgs,
                     const double tre = 1e-6, const double tae = 1e-6) {
      using boost::lambda::_1;

      stan::math::check_finite("integrate_1d_tsc", "lower limit", a);
      stan::math::check_finite("integrate_1d_tsc", "upper limit", b);

      double val_ = de_integrator(boost::bind<double>(f, _1,
                                                      value_of(param),
                                                      msgs),
                                  a, b, tre, tae);

      if (!is_constant_struct<T_param>::value) {
        size_t N = stan::length(param);
        std::vector<double> results(N);

        try {
          start_nested();

          auto clean_param = to_var(value_of(param));
          typedef decltype(clean_param) clean_T_param;

          scalar_seq_view<clean_T_param>
            clean_param_vec(clean_param);

          for (size_t n = 0; n < N; n++)
            results[n] =
              de_integrator(
                boost::bind<double>(gradient_of_f<F, clean_T_param>, f,
                                    _1, clean_param, clean_param_vec[n],
                                    msgs), a, b, tre, tae);
        } catch (const std::exception& e) {
          recover_memory_nested();
          throw;
        }
        recover_memory_nested();

        operands_and_partials<T_param> ops_partials(param);
        for (size_t n = 0; n < N; n++)
          ops_partials.edge1_.partials_[n] += results[n];

        return ops_partials.build(val_);
      } else {
        return val_;
      }
    }

  }

}

#endif