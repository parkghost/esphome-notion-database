#include "stream_monitor.h"

using namespace esphome;

// Constructor
StreamMonitor::StreamMonitor(Stream &inner) : inner_(inner), bytes_read_(0), bytes_written_(0) {}

// Returns the number of bytes available
int StreamMonitor::available() {
  App.feed_wdt();
  return inner_.available();
}

// Reads a byte from the stream
int StreamMonitor::read() {
  App.feed_wdt();
  int result = inner_.read();
  // Increment bytes_read_ if a byte was read
  if (result >= 0) {
    bytes_read_++;
  }
  return result;
}

// Reads up to size bytes from the stream
int StreamMonitor::read(uint8_t *buf, size_t size) {
  App.feed_wdt();
  int result = inner_.readBytes(reinterpret_cast<char *>(buf), size);
  // Increment bytes_read_ by the number of bytes read
  if (result > 0) {
    bytes_read_ += result;
  }
  return result;
}

// Peeks at the next byte in the stream
int StreamMonitor::peek() {
  App.feed_wdt();
  return inner_.peek();
}

// Writes a single byte to the stream
size_t StreamMonitor::write(uint8_t byte) {
  App.feed_wdt();
  size_t res = inner_.write(byte);
  if (res > 0) bytes_written_ += res;
  return res;
}

// Writes multiple bytes to the stream
size_t StreamMonitor::write(const uint8_t *buf, size_t size) {
  App.feed_wdt();
  size_t res = inner_.write(buf, size);
  if (res > 0) bytes_written_ += res;
  return res;
}

// Returns the number of bytes read
size_t StreamMonitor::get_bytes_read() const { return bytes_read_; }

// Returns the number of bytes written
size_t StreamMonitor::get_bytes_written() const { return bytes_written_; }