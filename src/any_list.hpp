/*
 * experimantal type-ereased list where the node and the data it contains are in the same allocation
 */


#ifndef SG_ANY_LIST_H
#define SG_ANY_LIST_H

#include <memory>
#include <utility>
#include <cassert>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <iterator>

namespace sg {

class any_list {
  template<typename>
  struct node;
  class itrerator;
  struct node_base {
   private:
    struct deleter {
      void operator ()(node_base* base) {
        base->destroy();
      }
    };
    using ptr_type = std::unique_ptr<node_base, deleter>;
    std::type_index _idx;
    ptr_type _next;
    node_base* _prev;
    void (* _deleter)(node_base*);
    void (* _copy)(node_base*, node_base*);
    void destroy() {
      _deleter(this);
    }
    template<typename T>
    node_base(T&&, node_base* prev, node_base::ptr_type next)
      : _idx{typeid(T)}, _next{std::move(next)}, _prev{prev} {
      _deleter = [](node_base* base) {
        delete (base->get_as<T>());
      };
    }
    template<typename T>
    node<T>* get_as() {
      return static_cast<node<T>*>(this);
    }
    template<typename T>
    const node<T>* get_as() const {
      return static_cast<node<T>*>(this);
    }
   public:
    template<typename T>
    bool constexpr check() const {
      return (_idx.hash_code() == typeid(T).hash_code() && _idx == typeid(T));
    }
    template<typename T>
    T& as() {
      return get_as<T>()->_data;
    }
    template<typename T>
    const T& as() const {
      return get_as<T>()->_data;
    }
    friend class any_list;
  };
  template<typename T>
  struct node : node_base {
    T _data;
    template<typename ... F>
    node(node_base* prev, node_base::ptr_type next, F&& ... data)
      : node_base(T(std::forward<F>(data)...), prev, std::move(next)), _data{std::forward<F>(data)...} {}
  };
  node_base::ptr_type _begin;
  node_base* _end;
  static node_base::ptr_type null() {
    return {nullptr};
  }
 public:
  class iterator {
    node_base* _ptr = nullptr;
    iterator(node_base* ptr) : _ptr{ptr} {}
   public:
    using value_type = node_base;
    using reference = node_base&;
    using pointer = node_base*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;
    iterator() = default;
    iterator(const iterator&) = default;
    iterator& operator =(const iterator&) = default;
    node_base& operator *() {
      return (*_ptr);
    }
    node_base* operator ->() {
      return (_ptr);
    }
    const node_base& operator *() const {
      return (*_ptr);
    }
    const node_base* operator ->() const {
      return (_ptr);
    }
    iterator& operator ++() {
      _ptr = _ptr->_next.get();
      return (*this);
    }
    iterator operator ++(int) {
      _ptr = _ptr->_next.get();
      return (*this);
    }
    iterator& operator --() {
      _ptr = _ptr->_prev;
      return (*this);
    }
    iterator operator --(int) {
      _ptr = _ptr->_prev;
      return (*this);
    }
    bool operator ==(iterator other) const {
      return _ptr == other._ptr;
    }
    bool operator !=(iterator other) const {
      return _ptr != other._ptr;
    }
    operator bool() const {
      return _ptr;
    }
    ~iterator() noexcept = default;
    friend class any_list;
  };
  using const_iterator = const iterator;
  any_list() : _begin{nullptr}, _end{nullptr} {}
  template<typename T, typename ... Ts>
  void emplace_back(Ts&& ... ts) {
    node_base::ptr_type tmp = node_base::ptr_type(new node<T>(_end, null(), std::forward<Ts>(ts)...));
    if (_begin == nullptr) {
      assert(!_end);
      _begin = std::move(tmp);
      _end = _begin.get();
      return;
    } else {
      assert(_end);
      _end->_next = std::move(tmp);
      _end = _end->_next.get();
    }
  }
  template<typename T, typename ... Ts>
  void emplace_front(Ts&& ... ts) {
    node_base::ptr_type tmp = node_base::ptr_type(new node<T>(nullptr, _begin, std::forward<Ts>(ts)...));
    if (_begin == nullptr) {
      assert(!_end);
      _begin = std::move(tmp);
      _end = _begin.get();
      return;
    } else {
      assert(_end);
      _begin = std::move(tmp);
      if (_begin->_next)
        _begin->_next->_prev = _begin.get();
    }
  }
  template<typename T, typename ... Ts>
  void emplace_next(const_iterator it, Ts&& ... ts) {
    assert(it);
    assert(_begin);
    assert(_end);
    node_base* ptr = it._ptr;
    node_base::ptr_type tmp = node_base::ptr_type(new node<T>(ptr, std::move(ptr->_next), std::forward<Ts>(ts)...));
    ptr->_next = std::move(tmp);
    if (ptr->_next->_next)
      ptr->_next->_next->_prev = ptr->_next.get();
    else
      _end = ptr->_next.get();
  }
  template<typename T, typename ... Ts>
  void emplace_prev(const_iterator it, Ts&& ... ts) {
    assert(it);
    assert(_begin);
    assert(_end);
    node_base* ptr = it._ptr;
    if (ptr->_prev) {
      node_base::ptr_type
        tmp = node_base::ptr_type(new node<T>(ptr->_prev, std::move(ptr->_prev->_next), std::forward<Ts>(ts)...));
      ptr->_prev->_next = std::move(tmp);
      ptr->_prev = ptr->_prev->_next.get();
    } else {
      node_base::ptr_type tmp = node_base::ptr_type(new node<T>(nullptr, std::move(_begin), std::forward<Ts>(ts)...));
      _begin = std::move(tmp);
      _begin->_next->_prev = _begin.get();
    }
  }
  template<typename T>
  void push_front(T&& elem) {
    emplace_front<typename std::remove_cv<typename std::remove_reference<T>::type>::type>(std::forward<T>(elem));
  }
  template<typename T>
  void push_back(T&& elem) {
    emplace_back<typename std::remove_cv<typename std::remove_reference<T>::type>::type>(std::forward<T>(elem));
  }
  template<typename T>
  void push_next(iterator it, T&& elem) {
    emplace_next<typename std::remove_cv<typename std::remove_reference<T>::type>::type>(it, std::forward<T>(elem));
  }
  template<typename T>
  void push_prev(iterator it, T&& elem) {
    emplace_prev<typename std::remove_cv<typename std::remove_reference<T>::type>::type>(it, std::forward<T>(elem));
  }
  template<typename T>
  [[nodiscard]] bool check_front() const {
    return _begin->check<T>();
  }
  template<typename T>
  [[nodiscard]] bool check_back() const {
    return _end->check<T>();
  }
  template<typename T>
  [[nodiscard]] T& front() {
    assert(_begin);
    assert(_end);
    return _begin->get_as<T>()->_data;
  }
  template<typename T>
  [[nodiscard]] T& back() {
    assert(_begin);
    assert(_end);
    return _end->get_as<T>()->_data;
  }
  void pop_front() {
    assert(_begin);
    assert(_end);
    _begin = std::move(_begin->_next);
    if (_begin)
      _begin->_prev = nullptr;
    else
      _end = nullptr;
  }
  void pop_back() {
    assert(_begin);
    assert(_end);
    _end = _end->_prev;
    if (_end)
      _end->_next = null();
    else
      _begin = null();
  }
  void pop_next(iterator it) {
    assert(it);
    assert(_begin);
    assert(_end);
    assert(it._ptr->_next);
    node_base* ptr = it._ptr;
    ptr->_next = std::move(ptr->_next->_next);
    if (ptr->_next)
      ptr->_next->_prev = ptr;
    else
      _end = ptr->_next.get();
  }
  void pop_prev(iterator it) {
    assert(it);
    assert(_begin);
    assert(_end);
    assert(it._ptr->_prev);
    node_base* ptr = it._ptr;
    if (ptr->_prev->_prev) {
      node_base::ptr_type tmp = std::move(ptr->_prev->_prev->_next);
      tmp->_prev->_next = std::move(tmp->_next);
      ptr->_prev = tmp->_prev;
    } else {
      _begin = std::move(ptr->_prev->_next);
      _begin->_next->_prev = _begin.get();
    }
  }
  void pop(iterator it) {
    assert(_begin);
    assert(_end);
    assert(it);
    if (it->_next)
      pop_prev(++it);
    else if (it->_prev)
      pop_next(--it);
    else
      clear();
  }
  std::reverse_iterator<iterator> rbegin() {
    return std::make_reverse_iterator(iterator(_end));
  }
  std::reverse_iterator<iterator> rend() {
    return std::make_reverse_iterator(iterator());
  }
  const_iterator begin() const {
    return iterator(_begin.get());
  }
  const_iterator end() const {
    return iterator();
  }
  void clear() noexcept {
    _begin = null();
    _end = nullptr;
  }
};

};

#endif