
#ifndef UTILS_ANY_CALLABLE_HPP
#define UTILS_ANY_CALLABLE_HPP

#include <utility>
#include <type_traits>
#include <cassert>

#include "sbo_base.hpp"
#include "callable_utils.hpp"

namespace sg {

template<typename, std::size_t sbo_size = 32>
class any_callable;

template<typename R, typename ... Args, std::size_t sbo_size>
class any_callable<R(Args...), sbo_size> : sbo_base<sbo_size> {
 private:
  using base = sbo_base<sbo_size>;
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
    static_assert(std::decay_t<T>::buff_size <= sbo_size, "noexcept can't be garenteed");
    //because sbo can be to small for the current type but fit in the other type so and allocation could be necessary
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
  }
 public:
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
  any_callable& operator =(T&& other) {
    destroy();
    move_to_self(std::forward<T>(other));
    return (*this);
  }
  R operator ()(Args... args) const {
    return invoke_ptr(this->ptr(), propagate(args)...);
  }
  explicit operator bool() const noexcept {
    return invoke_ptr;
  }
  bool is_empty() const noexcept {
    return invoke_ptr;
  }
  bool is_sbo() const noexcept {
    return this->sbo_base<sbo_size>::is_sbo();
  }
  bool operator ==(const any_callable& other) {
    return invoke_ptr == other.invoke_ptr &&
      destroy_ptr == other.destroy_ptr &&
      move_ptr == other.move_ptr;
  }
  any_callable(any_callable&) = delete;
  any_callable& operator =(any_callable&) = delete;
  ~any_callable() {
    destroy();
  }
};

}

#endif