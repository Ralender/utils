/*
 * tests for any_callable
 */

#include <gtest/gtest.h>
#include "src/any_callable.hpp"

namespace {

void free_add1_ref(int& a) {
  a++;
}

int free_add1(int a) {
  return a + 1;
}

int free_add2(int a) {
  return a + 2;
}

std::string free_add_a(const std::string& str) {
  return str + 'a';
}

int global = 0;

void free_func() {
  global++;
}

template<typename T>
using any_callable = sg::any_callable<T, 8>;

static_assert(!any_callable<void()>::fit_sbo < std::string > );
// string is used as a non-trivial type that doesn't fit in sbo

}

TEST(callable, call_stateless_lambda_int) {
  any_callable<int(int)> add1([](int i) {
    return i + 1;
  });
  ASSERT_EQ(add1(1), 2);
  ASSERT_EQ(add1(add1(1)), 3);
}

TEST(callable, call_stateless_lambda_string) {
  any_callable<std::string(std::string)> add1([](std::string i) {
    return i + 'a';
  });
  ASSERT_EQ(add1(""), "a");
  ASSERT_EQ(add1(add1("b")), "baa");
}

TEST(callable, call_stateless_lambda_int_ref) {
  any_callable<void(int&)> add1([](int& i) {
    i = i + 1;
  });
  int i = 1;
  add1(i);
  ASSERT_EQ(i, 2);
  add1(i);
  ASSERT_EQ(i, 3);
}

TEST(callable, call_stateless_lambda_string_ref) {
  any_callable<void(std::string&)> add1([](std::string& i) {
    i += 'a';
  });
  std::string tmp = "b";
  ASSERT_EQ(tmp, "b");
  add1(tmp);
  ASSERT_EQ(tmp, "ba");
  add1(tmp);
  ASSERT_EQ(tmp, "baa");
}

TEST(callable, call_statefull_ref_lambda_int) {
  int i = 0;
  any_callable<void()> add1([&i]() {
    i++;
  });

  ASSERT_EQ(i, 0);
  add1();
  ASSERT_EQ(i, 1);
  add1();
  ASSERT_EQ(i, 2);
}

TEST(callable, call_statefull_ref_lambda_string) {
  std::string str;
  any_callable<void()> adda([&]() {
    str.push_back('a');
  });
  ASSERT_EQ(str, "");
  adda();
  ASSERT_EQ(str, "a");
  adda();
  ASSERT_EQ(str, "aa");
}

TEST(callable, call_statefull_copy_lambda_int) {
  int i = -2;
  any_callable<int()> ref([=]() {
    return i;
  });
  ASSERT_EQ(ref(), -2);
}

TEST(callable, call_statefull_copy_lambda_string) {
  std::string i = "a";
  any_callable<std::string()> ref([=]() {
    return i;
  });
  ASSERT_EQ(ref(), "a");
}

TEST(callable, call_func_ptr_int) {
  any_callable<int(int)> ref(&free_add1);
  ASSERT_EQ(ref(1), 2);
  ASSERT_EQ(ref(ref(1)), 3);
}

TEST(callable, call_func_ptr_int_ref) {
  any_callable<void(int&)> ref(&free_add1_ref);
  int tmp = 0;
  ASSERT_EQ(tmp, 0);
  ref(tmp);
  ASSERT_EQ(tmp, 1);
  ref(tmp);
  ASSERT_EQ(tmp, 2);
}

TEST(callable, call_func_ptr_global_state) {
  global = 0;
  any_callable<void()> ref(free_func);
  ASSERT_EQ(global, 0);
  ref();
  ASSERT_EQ(global, 1);
  ref();
  ASSERT_EQ(global, 2);
}

TEST(callable, move_call_stateless_lambda) {
  any_callable<int(int)> add1([](int i) {
    return i + 1;
  });
  any_callable<int(int)> cop(std::move(add1));
  ASSERT_EQ(cop(1), 2);
  ASSERT_EQ(cop(cop(1)), 3);
}

TEST(callable, move_func_ptr_int) {
  any_callable<int(int)> ref(&free_add1);

  any_callable<int(int)> cop(std::move(ref));
  ASSERT_EQ(cop(1), 2);
  ASSERT_EQ(cop(cop(1)), 3);
}

TEST(callable, move_call_statefull_ref_lambda_int) {
  int i = 0;
  any_callable<void()> add1([&i]() {
    i++;
  });
  any_callable<void()> cop(std::move(add1));
  ASSERT_EQ(i, 0);
  cop();
  ASSERT_EQ(i, 1);
  cop();
  ASSERT_EQ(i, 2);
}

TEST(callable, move_call_statefull_ref_lambda_string) {
  std::string str;
  any_callable<void()> adda([&]() {
    str.push_back('a');
  });
  any_callable<void()> cop(std::move(adda));
  ASSERT_EQ(str, "");
  cop();
  ASSERT_EQ(str, "a");
  cop();
  ASSERT_EQ(str, "aa");
}

TEST(callable, move_call_statefull_copy_lambda_int) {
  int i = -2;
  any_callable<int()> ref([=]() {
    return i;
  });
  any_callable<int()> cop(std::move(ref));
  ASSERT_EQ(cop(), -2);
}

TEST(callable, move_call_statefull_copy_lambda_string) {
  std::string i = "a";
  any_callable<std::string()> ref([=]() {
    return i;
  });
  any_callable<std::string()> cop(std::move(ref));
  ASSERT_EQ(cop(), "a");
}

TEST(callable, compare_wrapped_func_ptr_int_true) {
  any_callable<int(int)> a(&free_add1);

  any_callable<int(int)> b(&free_add1);
  ASSERT_EQ(a == b, true);
  ASSERT_EQ(a != b, false);
  ASSERT_EQ(b == a, true);
  ASSERT_EQ(b != a, false);
}

TEST(callable, compare_wrapped_func_ptr_int_false) {
  any_callable<int(int)> a(&free_add1);

  any_callable<int(int)> b(&free_add2);
  ASSERT_EQ(a == b, false);
  ASSERT_EQ(a != b, true);
  ASSERT_EQ(b == a, false);
  ASSERT_EQ(b != a, true);
}

TEST(callable, compare_wrapped_stateless_lambda_true) {
  auto labmda = [](int a) { return a + 1; };
  any_callable<int(int)> a(labmda);

  any_callable<int(int)> b(labmda);
  ASSERT_EQ(a == b, true);
  ASSERT_EQ(a != b, false);
  ASSERT_EQ(b == a, true);
  ASSERT_EQ(b != a, false);
}

TEST(callable, compare_wrapped_stateless_lambda_false) {
  auto labmda1 = [](int a) { return a + 1; };
  auto labmda2 = [](int a) { return a + 2; };
  any_callable<int(int)> a(labmda1);

  any_callable<int(int)> b(labmda2);
  ASSERT_EQ(a == b, false);
  ASSERT_EQ(a != b, true);
  ASSERT_EQ(b == a, false);
  ASSERT_EQ(b != a, true);
}

TEST(callable, compare_wrapped_statefull_lambda_true) {
  int i = 1;
  auto labmda = [i](int a) { return a + i; };
  any_callable<int(int)> a(labmda);

  any_callable<int(int)> b(labmda);
  ASSERT_EQ(a == b, true);
  ASSERT_EQ(a != b, false);
  ASSERT_EQ(b == a, true);
  ASSERT_EQ(b != a, false);
}

TEST(callable, compare_wrapped_statefull_lambda_false) {
  int i = 0;
  auto labmda1 = [i](int a) { return a + i; };
  auto labmda2 = [](int a) { return a + 2; };
  any_callable<int(int)> a(labmda1);

  any_callable<int(int)> b(labmda2);
  ASSERT_EQ(a == b, false);
  ASSERT_EQ(a != b, true);
  ASSERT_EQ(b == a, false);
  ASSERT_EQ(b != a, true);
}

TEST(callable, compare_unwrapped_func_ptr_int_true) {
  any_callable<int(int)> a(&free_add1);

  ASSERT_EQ(a == &free_add1, true);
  ASSERT_EQ(a != &free_add1, false);
  ASSERT_EQ(&free_add1 == a, true);
  ASSERT_EQ(&free_add1 != a, false);
}

TEST(callable, compare_unwrapped_func_ptr_int_false) {
  any_callable<int(int)> a(&free_add1);
  ASSERT_EQ(a == &free_add2, false);
  ASSERT_EQ(a != &free_add2, true);
  ASSERT_EQ(&free_add2 == a, false);
  ASSERT_EQ(&free_add2 != a, true);
}

TEST(callable, compare_unwrapped_stateless_lambda_true) {
  auto labmda = [](int a) { return a + 1; };
  any_callable<int(int)> a(labmda);

  ASSERT_EQ(a == labmda, true);
  ASSERT_EQ(a != labmda, false);
  ASSERT_EQ(labmda == a, true);
  ASSERT_EQ(labmda != a, false);
}

TEST(callable, compare_unwrapped_stateless_lambda_false) {
  auto labmda1 = [](int a) { return a + 1; };
  auto labmda2 = [](int a) { return a + 2; };
  any_callable<int(int)> a(labmda1);

  ASSERT_EQ(a == labmda2, false);
  ASSERT_EQ(a != labmda2, true);
  ASSERT_EQ(labmda2 == a, false);
  ASSERT_EQ(labmda2 != a, true);
}

TEST(callable, compare_unwrapped_statefull_lambda_true) {
  int i = 1;
  auto labmda = [i](int a) { return a + i; };
  any_callable<int(int)> a(labmda);

  ASSERT_EQ(a == labmda, true);
  ASSERT_EQ(a != labmda, false);
  ASSERT_EQ(labmda == a, true);
  ASSERT_EQ(labmda != a, false);
}

TEST(callable, compare_unwrapped_statefull_lambda_false) {
  int i = 0;
  auto labmda1 = [i](int a) { return a + i; };
  auto labmda2 = [](int a) { return a + 2; };
  any_callable<int(int)> a(labmda1);

  ASSERT_EQ(a == labmda2, false);
  ASSERT_EQ(a != labmda2, true);
  ASSERT_EQ(labmda2 == a, false);
  ASSERT_EQ(labmda2 != a, true);
}

TEST(callable, compare_nullptr_true) {
  any_callable<int(int)> a;

  ASSERT_EQ(a == nullptr, true);
  ASSERT_EQ(a != nullptr, false);
  ASSERT_EQ(nullptr == a, true);
  ASSERT_EQ(nullptr != a, false);
}

TEST(callable, compare_nullptr_false) {
  any_callable<int(int)> a([](int a) { return a; });

  ASSERT_EQ(a == nullptr, false);
  ASSERT_EQ(a != nullptr, true);
  ASSERT_EQ(nullptr == a, false);
  ASSERT_EQ(nullptr != a, true);
}

TEST(callable, operator_bool_false) {
  any_callable<int()> a;

  ASSERT_EQ(static_cast<bool>(a), false);
}

TEST(callable, operator_bool_true) {
  int i = 0;
  any_callable<int(int)> a([=](int v) { return v + i; });

  ASSERT_EQ(static_cast<bool>(a), true);
}

TEST(callable, is_empty_true) {
  any_callable<int()> a;

  ASSERT_EQ(a.is_empty(), true);
}

TEST(callable, is_empty_false) {
  int i = 0;
  any_callable<int(int)> a([=](int v) { return v + i; });

  ASSERT_EQ(a.is_empty(), false);
}

TEST(callable, type_traits) {
  static_assert(std::is_nothrow_move_constructible_v<any_callable<int()>>);
  static_assert(std::is_nothrow_move_constructible_v<any_callable<std::string(std::string)>>);
  static_assert(std::is_nothrow_move_assignable_v<any_callable<int()>>);
  static_assert(std::is_nothrow_move_assignable_v<any_callable<std::string(std::string)>>);
  static_assert(std::is_same_v<int, std::decay_t<decltype(std::declval<const any_callable<int()>>()())>>);
  static_assert(std::is_same_v<std::string,
                               std::decay_t<decltype(std::declval<const any_callable<std::string(std::string)>>()(std::declval<
                                 std::string>()))>>);
}