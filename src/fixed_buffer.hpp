/*
 * type similar to std::vector but for non-copyable and non-movable types. with the required adaptations
 */

#ifndef MT_TESTS_FIXED_BUFF_HPP
#define MT_TESTS_FIXED_BUFF_HPP

#include <cstdlib>
#include <memory>
#include <cstring>
#include <utility>
#include <type_traits>

namespace utils {

namespace detail {

auto build = [](auto) {};
using _build = decltype(build);

template<typename T>
struct fixed_buffer_base {
 public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = value_type*;
  using const_iterator = const value_type*;
 public:
  static constexpr bool is_noexcept_destructible = noexcept(std::declval<value_type>().~value_type());
 public:
  iterator _begin = nullptr;
  iterator _end = _begin;
};

}

using detail::build;

template<typename T>
class fixed_buffer : protected detail::fixed_buffer_base<T> {
 private:
  using base = detail::fixed_buffer_base<T>;
  using base::is_noexcept_destructible;
  using base::_begin;
  using base::_end;
 public:
  using typename base::value_type;
  using typename base::size_type;
  using typename base::difference_type;
  using typename base::reference;
  using typename base::const_reference;
  using typename base::iterator;
  using typename base::const_iterator;
 private:
  base& _data() {
    return *static_cast<base*>(this);
  }
  static base init(iterator ptr) {
    return base{ptr, ptr};
  }
  void destroy_at(iterator it) noexcept(is_noexcept_destructible) {
    it->~value_type();
  }
  void destroy() noexcept(is_noexcept_destructible) {
    if (_begin) {
      clear();
      std::free(_begin);
    }
  }
  template<typename ... Ts>
  void create_at(iterator it, Ts&& ... ts) noexcept(std::is_nothrow_constructible<value_type, Ts...>::value) {
    new(it) value_type(std::forward<Ts>(ts)...);
  }
  std::string out_range_msg(size_type pos, const std::string& position) {
    return ("out of range : " + std::to_string(pos) + " >= " + std::to_string(size()) + " in " + position);
  }
 public:
  fixed_buffer() noexcept = default;
  explicit fixed_buffer(size_type size) :
    base{init(static_cast<iterator>(std::malloc(sizeof(value_type) * size)))} {
  }
  template<typename C, typename ... Ts>
  explicit fixed_buffer(size_type size, C&& c, Ts&& ... ts) : fixed_buffer(size) {
    if (size > 0) {
      for (; _end < _begin + size - 1; ++_end) {
        create_at(_end, ts...);
        c(*_end);
      }
      create_at(_end, std::forward<Ts>(ts)...);
      std::forward<C>(c)(*_end);
      ++_end;
    }
  }
  fixed_buffer(const fixed_buffer&) = delete;
  fixed_buffer operator =(const fixed_buffer&) = delete;
  fixed_buffer(fixed_buffer&& other) noexcept {
    this->_data() = other._data();
    other._data() = {};
  }
  fixed_buffer& operator =(fixed_buffer&& other) noexcept(is_noexcept_destructible) {
    destroy();
    this->_data() = other._data();
    other._data() = {};
    return *this;
  }
  void swap(fixed_buffer& other) noexcept(is_noexcept_destructible) {
    fixed_buffer tmp = other._data();
    other._data() = this->_data();
    this->_data() = tmp;
  }
  reference operator [](size_type pos) noexcept {
    return _begin[pos];
  }
  const_reference operator [](size_type pos) const noexcept {
    return _begin[pos];
  }
  reference at(size_type pos) {
    if (pos < size())
      return this->operator [](pos);
    throw std::out_of_range(out_range_msg(pos, __PRETTY_FUNCTION__));
  }
  const_reference at(size_type pos) const {
    if (pos < size())
      return this->operator [](pos);
    throw std::out_of_range(out_range_msg(pos, __PRETTY_FUNCTION__));
  }
  template<typename ... Ts>
  reference remplace_at(size_type pos, Ts&& ...ts)
  noexcept(std::is_nothrow_constructible<value_type, Ts...>::value && is_noexcept_destructible) {
    destroy_at(_begin + pos);
    create_at(_begin + pos, std::forward<Ts>(ts)...);
    return _begin[pos];
  }
  template<typename ... Ts>
  reference emplace(Ts&& ... ts) noexcept(std::is_nothrow_constructible<value_type, Ts...>::value) {
    create_at(_end++, std::forward<Ts>(ts)...);
    return back();
  }
  void pop_back() noexcept(is_noexcept_destructible) {
    destroy_at(&back());
    _end--;
  }
  void clear() noexcept(is_noexcept_destructible) {
    _end--;
    for (; _end != _begin; --_end)
      destroy_at(_end);
  }
  void resize(size_type size) noexcept {
    _end = _begin + size;
  }
  size_type size() const noexcept {
    return _end - _begin;
  }
  iterator begin() noexcept {
    return _begin;
  }
  const_iterator begin() const noexcept {
    return _begin;
  }
  iterator end() noexcept {
    return _end;
  }
  const_iterator end() const noexcept {
    return _end;
  }
  reference front() noexcept {
    return *_begin;
  }
  const_reference front() const noexcept {
    return *_begin;
  }
  reference back() noexcept {
    return *(_end - 1);
  }
  const_reference back() const noexcept {
    return *(_end - 1);
  }
  ~fixed_buffer() noexcept(is_noexcept_destructible) {
    destroy();
  }
};

}

#endif //MT_TESTS_FIXED_BUFF_HPP