#pragma once

#include <ostream>
#include <string>
#include <cstring>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32
#include "esp_memory_utils.h"
#endif

namespace esphome {
namespace notion_database {

class PsramString {
 private:
  char *data_;
  size_t size_;
  size_t capacity_;
  // NONE flag = try PSRAM first, fall back to internal RAM
  RAMAllocator<char> allocator_{RAMAllocator<char>::NONE};

  void allocate_memory(size_t new_capacity) {
    if (new_capacity == 0) {
      deallocate_memory();
      return;
    }

    // If existing buffer is large enough, reuse it
    if (data_ && capacity_ >= new_capacity) {
      return;
    }

    char *new_data = allocator_.allocate(new_capacity);
    if (!new_data) {
      ESP_LOGE("psram_string", "Failed to allocate %zu bytes", new_capacity);
      return;
    }

    // Copy existing data if present
    if (data_ && size_ > 0 && new_capacity > size_) {
      memcpy(new_data, data_, size_);
    }

    deallocate_memory();
    data_ = new_data;
    capacity_ = new_capacity;
  }

  void deallocate_memory() {
    if (data_) {
      allocator_.deallocate(data_, capacity_);
      data_ = nullptr;
    }
    capacity_ = 0;
  }

 public:
  PsramString() : data_(nullptr), size_(0), capacity_(0) {}

  PsramString(const char *str) : data_(nullptr), size_(0), capacity_(0) {
    if (str) {
      size_t len = strlen(str);
      allocate_memory(len + 1);
      if (data_) {
        memcpy(data_, str, len);
        data_[len] = '\0';
        size_ = len;
      }
    }
  }

  PsramString(const std::string &str) : PsramString(str.c_str()) {}

  PsramString(const PsramString &other)
      : data_(nullptr), size_(0), capacity_(0) { *this = other; }

  PsramString(PsramString &&other) noexcept
      : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
  }

  PsramString &operator=(const PsramString &other) {
    if (this != &other) {
      if (other.data_ && other.size_ > 0) {
        allocate_memory(other.size_ + 1);
        if (data_) {
          memcpy(data_, other.data_, other.size_);
          data_[other.size_] = '\0';
          size_ = other.size_;
        }
      } else {
        deallocate_memory();
        size_ = 0;
      }
    }
    return *this;
  }

  PsramString &operator=(PsramString &&other) noexcept {
    if (this != &other) {
      deallocate_memory();
      data_ = other.data_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      other.data_ = nullptr;
      other.size_ = 0;
      other.capacity_ = 0;
    }
    return *this;
  }

  PsramString &operator=(const char *str) {
    if (str) {
      size_t len = strlen(str);
      allocate_memory(len + 1);
      if (data_) {
        memcpy(data_, str, len);
        data_[len] = '\0';
        size_ = len;
      }
    } else {
      deallocate_memory();
      size_ = 0;
    }
    return *this;
  }

  PsramString &operator=(const std::string &str) { return *this = str.c_str(); }

  ~PsramString() { deallocate_memory(); }

  const char *c_str() const { return data_ ? data_ : ""; }

  size_t size() const { return size_; }
  size_t length() const { return size_; }
  bool empty() const { return size_ == 0; }

  std::string to_std_string() const { return data_ ? std::string(data_, size_) : std::string(); }

  operator std::string() const { return to_std_string(); }

  bool operator==(const PsramString &other) const {
    if (size_ != other.size_)
      return false;
    if (size_ == 0)
      return true;
    return memcmp(data_, other.data_, size_) == 0;
  }

  bool operator==(const char *str) const {
    if (!str)
      return size_ == 0;
    return strcmp(c_str(), str) == 0;
  }

  bool operator==(const std::string &str) const { return strcmp(c_str(), str.c_str()) == 0; }

  size_t capacity() const { return capacity_; }

  bool is_using_psram() const {
#ifdef USE_ESP32
    if (!data_)
      return false;
    return esp_ptr_external_ram(data_);
#else
    return false;
#endif
  }
};

inline std::ostream &operator<<(std::ostream &os, const PsramString &str) {
  return os << str.c_str();
}

}  // namespace notion_database
}  // namespace esphome
