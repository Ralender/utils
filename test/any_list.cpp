/*
 * non-complete test for any_list
 */

#include "src/any_list.hpp"
#include <tuple>
#include <gtest/gtest.h>

template<typename ... Ts, typename F, std::size_t ... idx>
void for_tuple_impl(const std::tuple<Ts...>& tuple, F&& func, std::index_sequence<idx...>) {
  (func(std::get<idx>(tuple), idx),...);
}

template<typename ... Ts, typename F>
void for_tuple(const std::tuple<Ts...>& tuple, F&& func) {
  for_tuple_impl(tuple, std::forward<F>(func), std::make_index_sequence<sizeof...(Ts)>{});
}

template<typename ...>
using void_t = int;

TEST(any_list, basic) {
  sg::any_list list;
  std::stringstream ss;
  std::tuple<int, float, std::string> t{0, 0.1f, "bob"};

  for_tuple(t, [&](auto elem, auto) {
    list.push_back(elem);
  });
  for_tuple(t, [&](auto elem, auto idx) {
    sg::any_list::iterator it = list.begin();
    std::advance(it, idx);
    ASSERT_EQ(it->template check<decltype(elem)>(), true)
    ss << it->as<decltype(elem)>() << " ";
  });
  std::cout << ss.str() << std::endl;
}

