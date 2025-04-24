#pragma once

#include "esphome.h"

/**
 * @brief Monitors data stream of a Stream.
 *
 * Wraps a Stream and tracks bytes read/written.
 */
class StreamMonitor : public Stream {
 public:
  // Constructor
  StreamMonitor(Stream &inner);

  // Returns the number of bytes available
  int available() override;
  // Reads a byte from the stream
  int read() override;
  // Reads up to size bytes from the stream
  int read(uint8_t *buf, size_t size);
  // Peeks at the next byte in the stream
  int peek() override;
  // Writes a single byte to the stream
  size_t write(uint8_t byte) override;
  // Writes multiple bytes to the stream
  size_t write(const uint8_t *buf, size_t size) override;
  // Returns the number of bytes read
  size_t get_bytes_read() const;
  // Returns the number of bytes written
  size_t get_bytes_written() const;

 private:
  Stream &inner_;
  size_t bytes_read_;
  size_t bytes_written_;
};