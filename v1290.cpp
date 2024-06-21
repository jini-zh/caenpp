#include <algorithm>
#include <thread>
#include <chrono>

#include <cmath>

#include "v1290.hpp"

namespace caen {

const float V1290::single_resolution[4] = {
  800e-12,
  200e-12,
  100e-12,
  25e-12
};

const float V1290::pair_resolution[16] = {
  100e-12,
  200e-12,
  400e-12,
  800e-12,
  1.6e-9,
  3.2e-9,
  6.25e-9,
  12.5e-9,
  25e-9,
  50e-9,
  100e-9,
  200e-9,
  400e-9,
  800e-9,
  0,
  0
};

const float V1290::dead_times[4] = {
  5e-9,
  10e-9,
  30e-9,
  100e-9
};

V1290::V1290(const Connection& connection): Device(connection) {
  if (id() != 1290) throw WrongDevice(connection, "V1290");
  version_ = static_cast<Version>(read16(0x4030));
};

V1290::V1290(V1290&& device): Device(std::move(device)) {
  version_ = device.version_;
};

V1290::Resolution V1290::resolution() const {
  auto mode = edge_detection();

  micro_write(0x2600);
  uint16_t res = micro_read();

  Resolution result;
  if (mode & 3 == 3) {
    result.edge  = pair_resolution[res & 7];
    result.pulse = pair_resolution[res >> 3 & 0xF];
  } else {
    result.edge  = single_resolution[res & 3];
    result.pulse = 0;
  };
  return result;
};

static uint8_t find_nearest(
    const float* array, uint8_t size, float value, bool ascending = true
) {
  int i = (
      ascending
      ? std::upper_bound(array, array + size, value)
      : std::upper_bound(array, array + size, value, std::greater<float>())
  ) - array;
  if (i == 0) return 0;
  if (i == size) return i - 1;
  if (std::fabs(value - array[i-1]) < std::fabs(value - array[i])) return i - 1;
  return i;
};

void V1290::set_resolution(float edge, float pulse) {
  auto mode = edge_detection();

  if (mode & 3 == 3) {
    uint8_t iedge  = find_nearest(pair_resolution, 14, edge);
    uint8_t ipulse = find_nearest(pair_resolution, 14, pulse);
    micro_write(0x2500);
    micro_write(ipulse << 8 | iedge);
  } else {
    uint8_t iedge = find_nearest(single_resolution, 4, edge, false);
    micro_write(0x2400);
    micro_write(iedge);
  };
};

void V1290::set_dead_time(float time) {
  uint8_t t = find_nearest(dead_times, 4, time);
  micro_write(0x2800);
  micro_write(t);
};

V1290::TriggerConfiguration V1290::trigger_configuration() const {
  TriggerConfiguration result;
  micro_write(0x1600);
  result.window_width  = cycles_to_seconds(micro_read());
  result.window_offset = cycles_to_seconds(micro_read());
  result.search_margin = cycles_to_seconds(micro_read());
  result.reject_margin = cycles_to_seconds(micro_read());
  result.time_subtraction_enabled = micro_read() & 1;
  return result;
};

void V1290::set_edge_detection(bool leading, bool trailing) {
  uint16_t value = 0;
  if (trailing) value |= 1;
  if (leading)  value |= 3;

  micro_write(0x2200);
  micro_write(value);
};

int V1290::event_size() const {
  micro_write(0x3400);
  uint16_t code = micro_read();
  if (code == 9) return -1;
  if (code == 0) return 0;
  return 1 << code - 1;
};

static uint8_t log2_ceil(uint8_t x) {
#ifdef __cpp_lib_int_pow2
  uint8_t n = std::bit_width(x);
  if (x == 1 << n - 1) return n - 1;
  return n;
#else
  uint8_t n = 0;
  while (x > 1 << n) ++n;
  return n;
#endif
};

void V1290::set_event_size(int size) {
  uint16_t code;
  if (size < 0 || size > 128)
    code = 9;
  else if (size == 0)
    code = 0;
  else
    code = log2_ceil(size) + 1;

  micro_write(0x3300);
  micro_write(code);
};

void V1290::set_fifo_size(unsigned nwords) {
  uint8_t code;
  if (nwords <= 2)
    code = 0;
  else if (nwords >= 256)
    code = 7;
  else
    code = log2_ceil(nwords) - 1;

  micro_write(0x3B00);
  micro_write(code);
};

void V1290::enable_channels(uint32_t mask) {
  micro_write(0x4400);
  micro_write(mask & 0xFFFF);
  if (version_ == V1290A) micro_write(mask >> 16);
};

uint32_t V1290::enabled_channels() const {
  micro_write(0x4500);
  uint32_t result = micro_read();
  if (version_ == V1290A) result = micro_read() << 16 | result;
  return result;
};

uint32_t V1290::enabled_tdc_channels(uint8_t tdc) const {
  micro_write(0x4700 | tdc);
  uint32_t result = micro_read();
  result = micro_read() << 16 | result;
  return result;
};

void V1290::enable_tdc_channels(uint8_t tdc, uint32_t mask) {
  micro_write(0x4600 | tdc);
  micro_write(mask & 0xFFFF);
  micro_write(mask >> 16);
};

uint64_t V1290::tdc_status(uint8_t tdc) const {
  micro_write(0x7600 | tdc);
  uint64_t result = 0;
  for (int i = 0; i < 4; ++i) result = result << 16 | micro_read();
  return result;
};

void V1290::eeprom_write(uint16_t address, uint8_t byte) {
  micro_write(0xC000);
  micro_write(address);
  micro_write(byte);
};

uint8_t V1290::eeprom_read(uint16_t address) const {
  micro_write(0xC100);
  micro_write(address);
  return micro_read();
};

V1290::MicroRevision V1290::micro_revision_date() const {
  MicroRevision result;
  micro_write(0xC200);
  result.version = micro_read();
  result.day     = micro_read();
  result.month   = micro_read();
  result.year    = micro_read();
  return result;
};

void V1290::enable_test_mode(uint32_t test_word) {
  micro_write(0xC500);
  micro_write(test_word & 0xFFFF);
  micro_write(test_word >> 16);
};

void V1290::scan_path_read(uint8_t tdc, uint16_t* path) const {
  micro_write(0xC900 | tdc);
  for (int i = 0; i < scan_path_length; ++i) path[i] = micro_read();
};

void V1290::micro_wait(uint8_t bit) const {
  while (!(read16(0x1030) & bit))
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
};

uint16_t V1290::micro_read() const {
  micro_wait(2);
  return read16(0x102E);
};

void V1290::micro_write(uint16_t value) {
  micro_wait(1);
  write16(0x102E, value);
};

void V1290::micro_write(uint16_t value) const {
  const_cast<V1290*>(this)->micro_write(value);
};

};
