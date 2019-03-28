/*
 * type similar to std::function but move-only and the size of sbo is configurable
 */

#ifndef UTILS_ANY_CALLABLE_HPP
#define UTILS_ANY_CALLABLE_HPP

#include <utility>
#include <type_traits>
#include <cassert>

#include "sbo_base.hpp"
#include "callable_utils.hpp"

namespace sg {

template<typename Signature, std::size_t = 32, bool = false>
class any_callable;

template<typename R, typename ... Args, std::size_t sbo_size>
class any_callable<R(Args...) noexcept, sbo_size> : any_callable<R(Args...), sbo_size, true> {};

template<typename R, typename ... Args, std::size_t sbo_size, bool is_noexcept>
class any_callable<R(Args...), sbo_size, is_noexcept> : sbo_base<sbo_size> {
 private:
  using invoke_ptr_type = R(*)(void*, Args...);
  using destroy_ptr_type = void (*)(void*);
  using move_ptr_type = void (*)(void*, void*);
  invoke_ptr_type invoke_ptr = nullptr;
  destroy_ptr_type destroy_ptr = nullptr;
  move_ptr_type move_ptr = nullptr;
  template<typename T>
  static R invoke_func(void* data, Args... args) {
    return (*static_cast<T*>(data))(propagate(args)...);
  }
  template<typename T>
  static void destroy_func(void* data) {
    static_cast<T*>(data)->~T();
  }
  template<typename T>
  static void move_func(void* from, void* to) {
    new(to) T(std::move(*static_cast<T*>(from)));
  }
  template<typename T>
  void setup(T&& invokale) {
    sig_asserts<typename std::decay<T>::type, R(Args...)> check;
    static_assert(std::is_nothrow_move_constructible_v<T>);
    using inplace_type = std::decay_t<T>;

    if (!this->alloc(sizeof(inplace_type)))
      throw std::bad_alloc();
    new(this->ptr()) inplace_type(std::forward<T>(invokale));
    invoke_ptr = invoke_func<inplace_type>;
    destroy_ptr = destroy_func<inplace_type>;
    move_ptr = move_func<inplace_type>;
  }
  void destroy() {
    if (!this->is_empty()) {
      destroy_ptr(this->ptr());
      this->free();
    }
  }
  template<typename T>
  void move_to_self(T&& other) noexcept {
    static_assert(is_same_sig<std::decay_t<decltype(*this)>, std::decay_t<T>>::value, "signature don't match");
    static_assert(std::decay_t<T>::buff_size <= sbo_size, "noexcept can't be garenteed");
    //because sbo can be to small for the current type but fit in the other type so allocation could be necessary and fail
    this->_size = other._size;
    invoke_ptr = other.invoke_ptr;
    destroy_ptr = other.destroy_ptr;
    move_ptr = other.move_ptr;
    if (other.is_sbo()) {
      void* ptr = this->alloc(other.size());
      assert(ptr && "shouldn't fail as object fits in sbo");
      move_ptr(other.ptr(), ptr);
    } else {
      this->_alloced_ptr = other._alloced_ptr;
    }
    other._size = 0;
    other.invoke_ptr = nullptr;
  }
 public:
  template<typename T>
  constexpr static bool fit_sbo = sizeof(std::decay_t<T>) <= sbo_size;
  constexpr static std::size_t buff_size = sbo_size;
  any_callable() = default;
  template<typename T, std::enable_if_t<!sg::is_instance_of<std::decay_t<T>, any_callable>::value, int> = 0>
  explicit any_callable(T&& invokale) {
    setup(std::forward<T>(invokale));
  }
  template<typename T, std::enable_if_t<!sg::is_instance_of<std::decay_t<T>, any_callable>::value, int> = 0>
  any_callable& operator =(T&& invokale) {
    destroy();
    setup(std::forward(invokale));
    return (*this);
  }
  template<typename T, std::enable_if_t<sg::is_instance_of<std::decay_t<T>, any_callable>::value, int> = 0>
  explicit any_callable(T&& other) noexcept {
    move_to_self(std::forward<T>(other));
  }
  template<typename T, std::enable_if_t<sg::is_instance_of<std::decay_t<T>, any_callable>::value, int> = 0>
  any_callable& operator =(T&& other) noexcept {
    destroy();
    move_to_self(std::forward<T>(other));
    return (*this);
  }
  R operator ()(Args... args) const noexcept(is_noexcept) {
    assert(invoke_ptr);
    return invoke_ptr(this->ptr(), propagate(args)...);
  }
  [[nodiscard]] explicit operator bool() const noexcept {
    return invoke_ptr;
  }
  [[nodiscard]] bool is_empty() const noexcept {
    return invoke_ptr == nullptr;
  }
  template<typename T, std::enable_if_t<sg::is_instance_of<std::decay_t<T>, any_callable>::value, int> = 0>
  [[nodiscard]] bool operator ==(const T& other) const noexcept {
    if (invoke_ptr == &invoke_func<R(*)(Args...)> && other.invoke_ptr == invoke_ptr) {
      return (*reinterpret_cast<R(**)(Args...)>(this->ptr())) == (*reinterpret_cast<R(**)(Args...)>(other.ptr()));
    } else
      return invoke_ptr == other.invoke_ptr;
  }
  template<typename T, std::enable_if_t<!sg::is_instance_of<std::decay_t<T>, any_callable>::value, int> = 0>
  [[nodiscard]] friend bool operator ==(const any_callable<R(Args...), sbo_size>& first, const T& value) noexcept {
    sig_asserts<typename std::decay<T>::type, R(Args...)> check;
    static_assert(std::is_nothrow_move_constructible_v<T>);
    using inplace_type = std::decay_t<T>;

    if constexpr (std::is_constructible_v<inplace_type, R(*)(Args...)>) {
      if (first.invoke_ptr == &invoke_func<R(*)(Args...)>)
        return (*reinterpret_cast<R(**)(Args...)>(first.ptr())) == static_cast<R(*)(Args...)>(value);
    }
    return first.invoke_ptr == &invoke_func<inplace_type>;
  }
  template<typename T, std::enable_if_t<!sg::is_instance_of<std::decay_t<T>, any_callable>::value, int> = 0>
  [[nodiscard]] friend bool operator ==(const T& value, const any_callable<R(Args...), sbo_size>& first) noexcept {
    return (first == value);
  }
  template<typename T>
  [[nodiscard]] friend bool operator !=(const any_callable<R(Args...), sbo_size>& first, const T& value) noexcept {
    return !(first == value);
  }
  template<typename T, std::enable_if_t<!sg::is_instance_of<std::decay_t<T>, any_callable>::value, int> = 0>
  [[nodiscard]] friend bool operator !=(const T& value, const any_callable<R(Args...), sbo_size>& first) noexcept {
    return !(first == value);
  }
  [[nodiscard]] friend bool operator ==(const any_callable<R(Args...), sbo_size>& first, std::nullptr_t) noexcept {
    return first.invoke_ptr == nullptr;
  }
  [[nodiscard]] friend bool operator ==(std::nullptr_t, const any_callable<R(Args...), sbo_size>& first) noexcept {
    return first.invoke_ptr == nullptr;
  }
  any_callable(any_callable&) = delete;
  any_callable& operator =(any_callable&) = delete;
  ~any_callable() {
    destroy();
  }
};

}

#endif