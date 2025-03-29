#pragma once

#include "esphome/core/helpers.h"
#include <memory>

namespace esphome {
namespace notion_database {

extern RAMAllocator<uint8_t> ALLOCATOR;

template <typename T>
struct Allocator {
  typedef T value_type;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;

  template <typename U>
  struct rebind {
    typedef Allocator<U> other;
  };

  Allocator() = default;

  template <typename U>
  Allocator(const Allocator<U>&) {}

  ~Allocator() {}

  T* allocate(size_t n) {
    void* mem = ALLOCATOR.allocate(n * sizeof(T), alignof(T));
    return static_cast<T*>(mem);
  }

  void deallocate(T* p, size_t n) {
    ALLOCATOR.deallocate(static_cast<uint8_t*>(static_cast<void*>(p)), n * sizeof(T));
  }

  template <typename U>
  bool operator==(const Allocator<U>&) const { return true; }

  template <typename U>
  bool operator!=(const Allocator<U>&) const { return false; }
};

}  // namespace notion_database
}  // namespace esphome