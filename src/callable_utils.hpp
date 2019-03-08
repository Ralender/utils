//
// Created by tyker on 3/7/19.
//

#ifndef UTILS_INVOKABLE_TRAITS_HPP
#define UTILS_INVOKABLE_TRAITS_HPP

#include <type_traits>

namespace sg {

template<typename...>
struct sig_asserts {};

template<typename Rp, typename ... Argsp, typename R, typename ... Args>
struct sig_asserts<Rp(*)(Argsp...), R(Args...)> {
  static_assert(std::is_same_v<R, Rp>, "return type of signature and callable don't match");
  static_assert((std::is_same_v<Args, Argsp> && ...), "argument type of signature and callable don't match");
};

template<typename T, typename R, typename ... Args>
struct sig_asserts<T, R(Args...)> {
  static_assert(std::is_invocable_r_v<R, T, Args...>, "signature and callable don't match");
  static_assert(std::is_same_v<R, decltype(std::declval<T>()(std::declval<Args>()...))>,
                "return type of signature and callable don't match");
};

template<typename, template<typename, auto> class>
struct is_instance_of : std::false_type {};

template<template<typename, auto> class M, typename T, auto N>
struct is_instance_of<M<T, N>, M> : std::true_type {};

/**
 * @brief similar to std::forward but adapted to a context where the real type can't be deduced
 *        because the function can't be template or must match and exact signature
 * @tparam T type of the value to propagate
 * @param t value to propagate
 * @return T&& an lvalue reference if it receives and lvalue reference an rvalue reference otherwise
 */
template<typename T, std::enable_if_t<!std::is_lvalue_reference_v<T>, int> = 0>
T&& propagate(T&& t) {
  return std::move(t);
}

template<typename T, std::enable_if_t<std::is_lvalue_reference_v<T>, int> = 0>
T& propagate(T&& t) {
  return t;
}

}

#endif //UTILS_INVOKABLE_TRAITS_HPP
