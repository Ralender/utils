/*
 * type to be inherited from that handle an sbo buffer
 */

#ifndef UTILS_SBO_BASE_HPP
#define UTILS_SBO_BASE_HPP

#include <cstddef>
#include <cstdlib>
#include <memory>

namespace sg {

struct malloc_allocator {
  void* allocate(std::size_t size) noexcept {
    return std::malloc(size);
  }
  void deallocate(void* ptr) noexcept {
    std::free(ptr);
  }
};

template<std::size_t sbo_buff_size, std::size_t sbo_buff_align = alignof(max_align_t), typename Allocator = malloc_allocator>
class sbo_base : private Allocator {
 protected:
  union {
    alignas(sbo_buff_align) std::byte _sbo_buff[sbo_buff_size];
    void* _alloced_ptr = nullptr;
  };
  std::size_t _size = 0;
  void destroy() noexcept {
    if (!is_sbo())
      this->deallocate(_alloced_ptr);
  }
  sbo_base() = default;
  sbo_base(const sbo_base&) = delete;
  sbo_base& operator =(const sbo_base&) = delete;
  void* alloc(std::size_t size) noexcept {
    destroy();
    _size = size;
    if (size > sbo_buff_size) {
      _alloced_ptr = this->allocate(size);
      return _alloced_ptr;
    }
    return (void*) &(_sbo_buff[0]);
  }
  void* ptr() const noexcept {
    if (_size > sbo_buff_size)
      return _alloced_ptr;
    return (void*) &(_sbo_buff[0]);
  }
  std::size_t size() const noexcept {
    return _size;
  }
  void free() noexcept {
    destroy();
    _size = 0;
    _alloced_ptr = nullptr;
  }
  bool is_sbo() const noexcept {
    return _size <= sbo_buff_size;
  }
  ~sbo_base() noexcept {
    destroy();
  }
};

}

#endif //UTILS_SBO_BASE_HPP
