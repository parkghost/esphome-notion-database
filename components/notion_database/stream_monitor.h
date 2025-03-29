#pragma once

#include <WiFiClient.h>

#include "esphome.h"

/**
 * @brief Monitors data stream of a WiFiClient.
 *
 * Wraps a WiFiClient and tracks bytes read/written.
 */
class StreamMonitor : public WiFiClient {
 public:
  // Constructor
  StreamMonitor(WiFiClient &client);

  // Returns the number of bytes available
  int available() override;
  // Reads a byte from the stream
  int read() override;
  // Reads up to size bytes from the stream
  int read(uint8_t *buf, size_t size) override;
  // Peeks at the next byte in the stream
  int peek() override;
  // Writes a byte to the stream
  size_t write(uint8_t b) override;
  // Writes up to size bytes to the stream
  size_t write(const uint8_t *buf, size_t size) override;
  // Flushes the stream
  void flush() override;
  // Stops the stream
  void stop() override;
  // Returns the connection status
  uint8_t connected() override;

  // Returns the number of bytes read
  size_t get_bytes_read() const;
  // Returns the number of bytes written
  size_t get_bytes_written() const;

 private:
  WiFiClient &client_;
  size_t bytes_read_;
  size_t bytes_written_;
};