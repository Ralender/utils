/*
 * tests for any_callable_ref
 */

#include <gtest/gtest.h>
#include "src/any_callable_ref.hpp"

namespace {

void free_add1_ref(int& a) {
  a++;
}

int free_add1(int a) {
  return a + 1;
}

std::string free_add_a(const std::string& str) {
  return str + 'a';
}

int global = 0;

void free_func() {
  global++;
}

}

TEST(callable_ref, call_stateless_lambda_int) {
  sg::any_callable_ref<int(int)> add1([](int i) {
    return i + 1;
  });
  ASSERT_EQ(add1(1), 2);
  ASSERT_EQ(add1(add1(1)), 3);
}

TEST(callable_ref, call_stateless_lambda_string) {
  sg::any_callable_ref<std::string(std::string)> add1([](std::string i) {
    return i + 'a';
  });
  ASSERT_EQ(add1(""), "a");
  ASSERT_EQ(add1(add1("b")), "baa");
}

TEST(callable_ref, call_stateless_lambda_int_ref) {
  sg::any_callable_ref<void(int&)> add1([](int& i) {
    i = i + 1;
  });
  int i = 1;
  add1(i);
  ASSERT_EQ(i, 2);
  add1(i);
  ASSERT_EQ(i, 3);
}

TEST(callable_ref, call_stateless_lambda_string_ref) {
  sg::any_callable_ref<void(std::string&)> add1([](std::string& i) {
    i += 'a';
  });
  std::string tmp = "b";
  ASSERT_EQ(tmp, "b");
  add1(tmp);
  ASSERT_EQ(tmp, "ba");
  add1(tmp);
  ASSERT_EQ(tmp, "baa");
}

TEST(callable_ref, call_statefull_ref_lambda_int) {
  int i = 0;
  auto pinned_callback = [&i]() {
    i++;
  };
  sg::any_callable_ref<void()> add1(pinned_callback);
  ASSERT_EQ(i, 0);
  add1();
  ASSERT_EQ(i, 1);
  add1();
  ASSERT_EQ(i, 2);
}

TEST(callable_ref, call_statefull_ref_lambda_string) {
  std::string str;
  auto pinned_callback = [&]() {
    str.push_back('a');
  };
  sg::any_callable_ref<void()> adda(pinned_callback);
  ASSERT_EQ(str, "");
  adda();
  ASSERT_EQ(str, "a");
  adda();
  ASSERT_EQ(str, "aa");
}

TEST(callable_ref, call_statefull_copy_lambda_int) {
  int i = -2;
  auto pinned_callback = [=]() {
    return i;
  };
  sg::any_callable_ref<int()> ref(pinned_callback);
  ASSERT_EQ(ref(), -2);
}

TEST(callable_ref, call_statefull_copy_lambda_string) {
  std::string i = "a";
  auto pinned_callback = [=]() {
    return i;
  };
  sg::any_callable_ref<std::string()> ref(pinned_callback);
  ASSERT_EQ(ref(), "a");
}

TEST(callable_ref, call_func_ptr_int) {
  int (* pinning)(int) = &free_add1;
  sg::any_callable_ref<int(int)> ref(pinning);
  ASSERT_EQ(ref(1), 2);
  ASSERT_EQ(ref(ref(1)), 3);
}

TEST(callable_ref, call_func_ptr_int_ref) {
  void (* pinning)(int&) = &free_add1_ref;
  sg::any_callable_ref<void(int&)> ref(pinning);
  int tmp = 0;
  ASSERT_EQ(tmp, 0);
  ref(tmp);
  ASSERT_EQ(tmp, 1);
  ref(tmp);
  ASSERT_EQ(tmp, 2);
}

TEST(callable_ref, call_func_ptr_global_state) {
  global = 0;
  sg::any_callable_ref<void()> ref(free_func);
  ASSERT_EQ(global, 0);
  ref();
  ASSERT_EQ(global, 1);
  ref();
  ASSERT_EQ(global, 2);
}

TEST(callable_ref, copy_call_stateless_lambda) {
  sg::any_callable_ref<int(int)> add1([](int i) {
    return i + 1;
  });
  sg::any_callable_ref<int(int)> cop(add1);
  ASSERT_EQ(cop(1), 2);
  ASSERT_EQ(cop(cop(1)), 3);
}

TEST(callable_ref, copy_call_statefull_ref_lambda_int) {
  int i = 0;
  auto pinned_callback = [&i]() {
    i++;
  };
  sg::any_callable_ref<void()> add1(pinned_callback);
  sg::any_callable_ref<void()> cop(add1);
  ASSERT_EQ(i, 0);
  cop();
  ASSERT_EQ(i, 1);
  cop();
  ASSERT_EQ(i, 2);
}

TEST(callable_ref, copy_call_statefull_ref_lambda_string) {
  std::string str;
  auto pinned_callback = [&]() {
    str.push_back('a');
  };
  sg::any_callable_ref<void()> adda(pinned_callback);
  sg::any_callable_ref<void()> cop(adda);
  ASSERT_EQ(str, "");
  cop();
  ASSERT_EQ(str, "a");
  cop();
  ASSERT_EQ(str, "aa");
}

TEST(callable_ref, copy_call_statefull_copy_lambda_int) {
  int i = -2;
  auto pinned_callback = [=]() {
    return i;
  };
  sg::any_callable_ref<int()> ref(pinned_callback);
  sg::any_callable_ref<int()> cop(ref);
  ASSERT_EQ(cop(), -2);
}

TEST(callable_ref, copy_call_statefull_copy_lambda_string) {
  std::string i = "a";
  auto pinned_callback = [=]() {
    return i;
  };
  sg::any_callable_ref<std::string()> ref(pinned_callback);
  sg::any_callable_ref<std::string()> cop(ref);
  ASSERT_EQ(ref(), "a");
}

