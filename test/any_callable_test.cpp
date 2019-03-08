//
// Created by tyker on 2/13/19.
//

#include <gtest/gtest.h>
#include "src/any_callable.hpp"

void free_add1_ref(int& a);
int free_add1(int a);
std::string free_add_a(const std::string& str);
extern int global;
void free_func();

TEST(callable, call_stateless_lambda_int) {
  sg::any_callable<int(int)> add1([](int i) {
    return i + 1;
  });
  ASSERT_EQ(add1(1), 2);
  ASSERT_EQ(add1(add1(1)), 3);
}

TEST(callable, call_stateless_lambda_string) {
  sg::any_callable<std::string(std::string)> add1([](std::string i) {
    return i + 'a';
  });
  ASSERT_EQ(add1(""), "a");
  ASSERT_EQ(add1(add1("b")), "baa");
}

TEST(callable, call_stateless_lambda_int_ref) {
  sg::any_callable<void(int&)> add1([](int& i) {
    i = i + 1;
  });
  int i = 1;
  add1(i);
  ASSERT_EQ(i, 2);
  add1(i);
  ASSERT_EQ(i, 3);
}

TEST(callable, call_stateless_lambda_string_ref) {
  sg::any_callable<void(std::string&)> add1([](std::string& i) {
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
  sg::any_callable<void()> add1([&i]() {
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
  sg::any_callable<void()> adda([&]() {
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
  sg::any_callable<int()> ref([=]() {
    return i;
  });
  ASSERT_EQ(ref(), -2);
}

TEST(callable, call_statefull_copy_lambda_string) {
  std::string i = "a";
  sg::any_callable<std::string()> ref([=]() {
    return i;
  });
  ASSERT_EQ(ref(), "a");
}

TEST(callable, call_func_ptr_int) {
  sg::any_callable<int(int)> ref(&free_add1);
  ASSERT_EQ(ref(1), 2);
  ASSERT_EQ(ref(ref(1)), 3);
}

TEST(callable, call_func_ptr_int_ref) {
  sg::any_callable<void(int&)> ref(&free_add1_ref);
  int tmp = 0;
  ASSERT_EQ(tmp, 0);
  ref(tmp);
  ASSERT_EQ(tmp, 1);
  ref(tmp);
  ASSERT_EQ(tmp, 2);
}

TEST(callable, call_func_ptr_global_state) {
  global = 0;
  sg::any_callable<void()> ref(free_func);
  ASSERT_EQ(global, 0);
  ref();
  ASSERT_EQ(global, 1);
  ref();
  ASSERT_EQ(global, 2);
}

TEST(callable, move_call_stateless_lambda) {
  sg::any_callable<int(int)> add1([](int i) {
    return i + 1;
  });
  sg::any_callable<int(int)> cop(std::move(add1));
  ASSERT_EQ(cop(1), 2);
  ASSERT_EQ(cop(cop(1)), 3);
}

TEST(callable, move_call_statefull_ref_lambda_int) {
  int i = 0;
  sg::any_callable<void()> add1([&i]() {
    i++;
  });
  sg::any_callable<void()> cop(std::move(add1));
  ASSERT_EQ(i, 0);
  cop();
  ASSERT_EQ(i, 1);
  cop();
  ASSERT_EQ(i, 2);
}

TEST(callable, move_call_statefull_ref_lambda_string) {
  std::string str;
  sg::any_callable<void()> adda([&]() {
    str.push_back('a');
  });
  sg::any_callable<void()> cop(std::move(adda));
  ASSERT_EQ(str, "");
  cop();
  ASSERT_EQ(str, "a");
  cop();
  ASSERT_EQ(str, "aa");
}

TEST(callable, move_call_statefull_copy_lambda_int) {
  int i = -2;
  sg::any_callable<int()> ref([=]() {
    return i;
  });
  sg::any_callable<int()> cop(std::move(ref));
  ASSERT_EQ(cop(), -2);
}

TEST(callable, move_call_statefull_copy_lambda_string) {
  std::string i = "a";
  sg::any_callable<std::string()> ref([=]() {
    return i;
  });
  sg::any_callable<std::string()> cop(std::move(ref));
  ASSERT_EQ(cop(), "a");
}

