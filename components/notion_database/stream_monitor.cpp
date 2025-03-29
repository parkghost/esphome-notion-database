#include "stream_monitor.h"

using namespace esphome;

// Constructor
StreamMonitor::StreamMonitor(WiFiClient &client) : client_(client), bytes_read_(0), bytes_written_(0) {}

// Returns the number of bytes available
int StreamMonitor::available() {
  App.feed_wdt();
  return client_.available();
}

// Reads a byte from the stream
int StreamMonitor::read() {
  App.feed_wdt();
  int result = client_.read();
  // Increment bytes_read_ if a byte was read
  if (result >= 0) {
    bytes_read_++;
  }
  return result;
}

// Reads up to size bytes from the stream
int StreamMonitor::read(uint8_t *buf, size_t size) {
  App.feed_wdt();
  int result = client_.read(buf, size);
  // Increment bytes_read_ by the number of bytes read
  if (result > 0) {
    bytes_read_ += result;
  }
  return result;
}

// Peeks at the next byte in the stream
int StreamMonitor::peek() {
  App.feed_wdt();
  return client_.peek();
}

// Writes a byte to the stream
size_t StreamMonitor::write(uint8_t b) {
  App.feed_wdt();
  size_t result = client_.write(b);
  bytes_written_ += result;
  return result;
}

// Writes up to size bytes to the stream
size_t StreamMonitor::write(const uint8_t *buf, size_t size) {
  App.feed_wdt();
  size_t result = client_.write(buf, size);
  bytes_written_ += result;
  return result;
}

// Flushes the stream
void StreamMonitor::flush() {
  App.feed_wdt();
  client_.flush();
}

// Stops the stream
void StreamMonitor::stop() {
  App.feed_wdt();
  client_.stop();
}

// Returns the connection status
uint8_t StreamMonitor::connected() {
  App.feed_wdt();
  return client_.connected();
}

// Returns the number of bytes read
size_t StreamMonitor::get_bytes_read() const { return bytes_read_; }

// Returns the number of bytes written
size_t StreamMonitor::get_bytes_written() const { return bytes_written_; }