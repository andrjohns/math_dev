#ifndef STAN_MATH_PRIM_FUNCTOR_ROWWISE_HPP
#define STAN_MATH_PRIM_FUNCTOR_ROWWISE_HPP

#include <stan/math/prim/meta.hpp>
#include <vector>

namespace stan {
namespace math {
namespace internal {

template <template <class> class Check>
constexpr size_t type_count(size_t x) {
  return x;
}
template <template <class> class Check, typename T, typename... TArgs>
constexpr size_t type_count(size_t x) {
  return Check<T>::value ? type_count<Check, TArgs...>(x + 1) : x;
}
template <template <class> class Check, typename... TArgs>
constexpr size_t type_count() {
    return type_count<Check, TArgs...>(0);
}

template <typename T1, typename... Ts>
inline size_t rows_equal(const T1& x1, const Ts&... xs) {
  auto v = {rows(x1), rows(xs)...};
  return std::all_of(v.begin(), v.end(),
                     [&](int i){ return i == *v.begin(); });
}

template <typename TupleT>
decltype(auto) row_index(const TupleT& x, size_t i) {
  return apply([&](auto&&... args){
    return std::forward_as_tuple(row(args, i + 1)...);
  }, x);
}

template <std::size_t O, std::size_t ... Is>
std::index_sequence<(O + Is)...>
  add_offset(std::index_sequence<Is...>) {
    return {};
}

template<typename Tuple, std::size_t... Ints>
auto subset_tuple(Tuple&& tuple, std::index_sequence<Ints...>) {
 return std::forward_as_tuple(std::get<Ints>(std::forward<Tuple>(tuple))...);
}

template <typename T>
using is_stan_type = disjunction<is_stan_scalar<T>, is_container<T>>;

template <bool colwise, typename T>
using apply_return_t
  = std::conditional_t<is_stan_scalar<T>::value,
                       Eigen::Matrix<T,
                                     colwise ? 1 : Eigen::Dynamic,
                                     colwise ? Eigen::Dynamic : 1>,
                       Eigen::Matrix<scalar_type_t<T>,
                                     Eigen::Dynamic,
                                     Eigen::Dynamic>>;
} // namespace internal

template <typename... TArgs>
auto rowwise(const TArgs&... args) {
  // Get location of functor in parameter pack
  constexpr size_t pos = internal::type_count<internal::is_stan_type,
                                              TArgs...>();
  constexpr size_t arg_size = sizeof...(TArgs);
  decltype(auto) args_tuple = std::forward_as_tuple(args...);
  using TupleT = decltype(args_tuple);

  // Split parameter pack into two tuples, separated by the functor
  decltype(auto) t1 = internal::subset_tuple(std::forward<TupleT>(args_tuple),
                         std::make_index_sequence<pos>{});
  decltype(auto) t2 = internal::subset_tuple(std::forward<TupleT>(args_tuple),
                         internal::add_offset<pos+1>(
                          std::make_index_sequence<arg_size-pos-1>{}));

  // Check that inputs to be iterated have the same number of rows
  bool eqrows = apply([&](auto&&... args) {
                        return internal::rows_equal(args...); },
                      std::forward<decltype(t1)>(t1));

  if (!eqrows) {
    std::ostringstream msg;
    msg << "Inputs to be iterated over must have the same number of rows!";
    throw std::invalid_argument(msg.str());
  }

  size_t rs = std::get<0>(std::forward<decltype(t1)>(t1)).rows();

  // Extract functor from parameter pack
  decltype(auto) f = std::get<pos>(args_tuple);

  // Evaluate first iteration, needed to determine type and size of return
  decltype(auto) iter_0
    = apply([&](auto&&... args) { return f(args...); },
            std::tuple_cat(internal::row_index(std::forward<decltype(t1)>(t1), 0),
                           std::forward<decltype(t2)>(t2)));

  internal::apply_return_t<0, decltype(iter_0)> rtn(rs, stan::math::size(iter_0));
  rtn.row(0) = as_row_vector(std::move(iter_0));

  for(size_t i = 1; i < rs; ++i) {
    rtn.row(i) = as_row_vector(
      apply([&](auto&&... args) { return f(args...); },
        std::tuple_cat(internal::row_index(std::forward<decltype(t1)>(t1), i),
                        std::forward<decltype(t2)>(t2)))
    );
  }

  return rtn;
}
}  // namespace math
}  // namespace stan
#endif
