#pragma once

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "esphome/components/http_request/http_request.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

namespace esphome {
namespace notion_database {

/// Wraps ESPHome's HttpContainer to provide high-level streaming methods
/// compatible with ArduinoJson's reader concept and streaming parse logic.
class HttpStreamAdapter {
 public:
  static constexpr const char *const TAG = "http_stream";
  static constexpr size_t DEFAULT_BUFFER_SIZE = 1024;
  static constexpr size_t MIN_BUFFER_SIZE = 64;
  static constexpr size_t MAX_BUFFER_SIZE = 4096;
  static constexpr size_t MAX_STRING_LENGTH = 1024;

  explicit HttpStreamAdapter(std::shared_ptr<http_request::HttpContainer> container,
                             size_t buffer_size = DEFAULT_BUFFER_SIZE,
                             uint32_t timeout_ms = 10000)
      : container_(std::move(container)), total_bytes_read_(0), eof_(false),
        timeout_ms_(timeout_ms), last_data_time_(millis()) {
    if (buffer_size < MIN_BUFFER_SIZE) buffer_size = MIN_BUFFER_SIZE;
    if (buffer_size > MAX_BUFFER_SIZE) buffer_size = MAX_BUFFER_SIZE;
    buf_.resize(buffer_size);
    read_pos_ = 0;
    write_pos_ = 0;
  }

  // Disable copy
  HttpStreamAdapter(const HttpStreamAdapter &) = delete;
  HttpStreamAdapter &operator=(const HttpStreamAdapter &) = delete;

  /// Read one byte. Returns -1 on EOF.
  /// This method satisfies ArduinoJson's reader concept.
  int read() {
    if (read_pos_ < write_pos_) {
      uint8_t byte = buf_[read_pos_++];
      total_bytes_read_++;
      return byte;
    }
    if (eof_) return -1;
    if (!fill_buffer_()) return -1;
    if (read_pos_ < write_pos_) {
      uint8_t byte = buf_[read_pos_++];
      total_bytes_read_++;
      return byte;
    }
    return -1;
  }

  /// Peek at next byte without consuming it.
  int peek() {
    if (read_pos_ < write_pos_) {
      return buf_[read_pos_];
    }
    if (eof_) return -1;
    if (!fill_buffer_()) return -1;
    if (read_pos_ < write_pos_) {
      return buf_[read_pos_];
    }
    return -1;
  }

  /// Returns number of bytes available in buffer (does not query underlying stream).
  int available() {
    return static_cast<int>(write_pos_ - read_pos_);
  }

  /// Search for target string in stream. Consumes all bytes up to and including target.
  /// Returns true if found, false if EOF reached.
  bool find(const char *target) {
    return findUntil(target, nullptr);
  }

  /// Read bytes until terminator is found. Returns accumulated string (excluding terminator).
  std::string readStringUntil(char terminator) {
    std::string result;
    result.reserve(128);

    int c;
    while ((c = read()) != -1) {
      if (static_cast<char>(c) == terminator) break;
      result += static_cast<char>(c);
      if (result.length() > MAX_STRING_LENGTH) {
        ESP_LOGW(TAG, "readStringUntil('%c') exceeded %zu chars, truncating",
                 terminator, MAX_STRING_LENGTH);
        break;
      }
    }
    return result;
  }

  /// Search for target but stop if terminator is found first.
  /// Returns true if target found before terminator.
  bool findUntil(const char *target, const char *terminator) {
    if (!target || !*target) return true;
    size_t target_len = strlen(target);
    size_t term_len = terminator ? strlen(terminator) : 0;
    size_t target_match = 0;
    size_t term_match = 0;

    while (true) {
      int c = read();
      if (c == -1) return false;
      char ch = static_cast<char>(c);

      // Check target match
      if (ch == target[target_match]) {
        target_match++;
        if (target_match == target_len) return true;
      } else {
        if (target_match > 0) {
          target_match = 0;
          if (ch == target[0]) target_match = 1;
        }
      }

      // Check terminator match
      if (term_len > 0) {
        if (ch == terminator[term_match]) {
          term_match++;
          if (term_match == term_len) return false;  // Terminator found first
        } else {
          if (term_match > 0) {
            term_match = 0;
            if (ch == terminator[0]) term_match = 1;
          }
        }
      }
    }
  }

  size_t getBytesRead() const { return total_bytes_read_; }

  void drainBuffer() {
    read_pos_ = write_pos_;  // Discard buffered data
  }

 private:
  bool fill_buffer_() {
    // Only compact when remaining space is less than half the buffer
    size_t space = buf_.size() - write_pos_;
    if (space < buf_.size() / 2 && read_pos_ > 0) {
      size_t remaining = write_pos_ - read_pos_;
      if (remaining > 0) {
        memmove(buf_.data(), buf_.data() + read_pos_, remaining);
      }
      write_pos_ = remaining;
      read_pos_ = 0;
      space = buf_.size() - write_pos_;
    }
    if (space == 0) return write_pos_ > read_pos_;

    while (true) {
      App.feed_wdt();
      yield();
      int bytes_read = container_->read(buf_.data() + write_pos_, space);
      auto result = http_request::http_read_loop_result(
          bytes_read, last_data_time_, timeout_ms_,
          container_->is_read_complete());

      switch (result) {
        case http_request::HttpReadLoopResult::DATA:
          write_pos_ += bytes_read;
          return true;
        case http_request::HttpReadLoopResult::COMPLETE:
          eof_ = true;
          return write_pos_ > read_pos_;
        case http_request::HttpReadLoopResult::RETRY:
          continue;
        case http_request::HttpReadLoopResult::ERROR:
        case http_request::HttpReadLoopResult::TIMEOUT:
          ESP_LOGW(TAG, "fill_buffer_ %s",
                   result == http_request::HttpReadLoopResult::ERROR ? "read error" : "timeout");
          eof_ = true;
          return write_pos_ > read_pos_;
      }
      // unreachable, but satisfy compiler
      return false;
    }
  }

  std::shared_ptr<http_request::HttpContainer> container_;
  std::vector<uint8_t> buf_;
  size_t read_pos_;
  size_t write_pos_;
  size_t total_bytes_read_;
  bool eof_;
  uint32_t timeout_ms_;
  uint32_t last_data_time_;
};

}  // namespace notion_database
}  // namespace esphome
