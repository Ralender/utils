/*
 * type similar to std::function but non-owning.
 */

#ifndef UTILS_CALLABLE_REF_HPP
#define UTILS_CALLABLE_REF_HPP

#include <type_traits>
#include <memory>
#include <cassert>

#include "callable_utils.hpp"

namespace sg {

template<typename>
class any_callable_ref {};

template<typename R, typename ... Args>
class any_callable_ref<R(Args...)> {
 protected:
  R (* call_ptr)(void*, Args...) = nullptr;
  void* data = nullptr;
 public:
  any_callable_ref() noexcept = default;
  template<typename Callable, typename std::enable_if_t<
    !std::is_same_v<std::decay_t<Callable>, any_callable_ref>, int> = 0>
  explicit any_callable_ref(Callable&& func) noexcept {
    sig_asserts<typename std::decay<Callable>::type, R(Args...)> check;
    data = reinterpret_cast<void*>(std::addressof(func));
    call_ptr = [](void* func, Args... args) -> R {
      if constexpr (std::is_same_v<R, void>)
        (*reinterpret_cast<std::remove_reference_t<Callable>*>(func))(sg::propagate(args)...);
      else
        return (*reinterpret_cast<std::remove_reference_t<Callable>*>(func))(sg::propagate(args)...);
    };
  }
  any_callable_ref(const any_callable_ref&) noexcept = default;
  template<typename Callable, typename std::enable_if_t<
    !std::is_same_v<std::decay_t<Callable>, any_callable_ref>, int> = 0>
  any_callable_ref& operator =(Callable&& func) noexcept {
    *this = any_callable_ref(std::forward<Callable>(func));
    return *this;
  }
  any_callable_ref& operator =(const any_callable_ref&) noexcept = default;
  bool operator ==(any_callable_ref other) const noexcept {
    return call_ptr == other.call_ptr && data == other.data;
  }
  template<typename Callable, typename std::enable_if_t<
    !std::is_same_v<std::decay_t<Callable>, any_callable_ref>, int> = 0>
  bool operator ==(Callable&& func) const noexcept {
    return *this == any_callable_ref(std::forward<Callable>(func));
  }
  template<typename T>
  bool operator !=(T&& other) const noexcept {
    return !(*this == std::forward<T>(other));
  }
  R operator ()(Args... args) const {
    assert(call_ptr);
    assert(data);
    return call_ptr(data, propagate(args)...);
  }
  operator bool() const noexcept {
    return call_ptr;
  }
  ~any_callable_ref() noexcept = default;
};

}

#endif //UTILS_CALLABLE_REF_HPP
