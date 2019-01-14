//
// Created by tyker on 12/8/18.
//

#ifndef UTILS_CALLABLE_REF_HPP
#define UTILS_CALLABLE_REF_HPP

#include <type_traits>
#include <memory>
#include <cassert>

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

template<typename>
class callable_ref {};

template<typename R, typename ... Args>
class callable_ref<R(Args...)> {
 protected:
  R (* call_ptr)(void*, Args...) = nullptr;
  void* data = nullptr;
 public:
  callable_ref() noexcept = default;
  template<typename Callable, typename std::enable_if_t<
    !std::is_same_v<std::decay_t<Callable>, callable_ref>, int> = 0>
  callable_ref(Callable&& func) noexcept {
    sig_asserts<typename std::decay<Callable>::type, R(Args...)> check;
    data = reinterpret_cast<void*>(std::addressof(func));
    call_ptr = [](void* func, Args... args) -> R {
      if constexpr (std::is_same_v<R, void>)
        (*reinterpret_cast<std::remove_reference_t<Callable>*>(func))(args...);
      else
        return (*reinterpret_cast<std::remove_reference_t<Callable>*>(func))(args...);
    };
  }
  callable_ref(const callable_ref&) noexcept = default;
  template<typename Callable, typename std::enable_if_t<
    !std::is_same_v<std::decay_t<Callable>, callable_ref>, int> = 0>
  callable_ref& operator =(Callable&& func) noexcept {
    *this = callable_ref(std::forward<Callable>(func));
    return *this;
  }
  callable_ref& operator =(const callable_ref&) noexcept = default;
  bool operator ==(callable_ref other) const noexcept {
    return call_ptr == other.call_ptr && data == other.data;
  }
  template<typename Callable, typename std::enable_if_t<
    !std::is_same_v<std::decay_t<Callable>, callable_ref>, int> = 0>
  bool operator ==(Callable&& func) const noexcept {
    return *this == callable_ref(std::forward<Callable>(func));
  }
  template<typename T>
  bool operator !=(T&& other) const noexcept {
    return !(*this == std::forward<T>(other));
  }
  R operator ()(Args... args) const {
    assert(call_ptr);
    assert(data);
    return call_ptr(data, args...);
  }
  operator bool() const noexcept {
    return call_ptr;
  }
  ~callable_ref() noexcept = default;
};

}

#endif //UTILS_CALLABLE_REF_HPP
