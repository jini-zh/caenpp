#include <cmath>

#include "v792.hpp"

namespace caen {

V792::V792(const Connection& connection): Device(connection) {
  if (id() != 792) throw WrongDevice(connection, "V792");
  init(version() == 0x11 ? V792A : V792N);
};

V792::V792(const Connection& connection, Version version): Device(connection) {
  if (id() != 792) throw WrongDevice(connection, "V792");
  init(version);
};

void V792::init(Version version) {
  channel_step_ = version == V792A ? 2 : 4;
};

float V792::fast_clear_window() const {
  return read16(0x102E) / 32e6 + 7e-6;
}

void V792::set_fast_clear_window(float window) {
  write16(0x102E, std::round((window - 7e-6) * 32e6));
};

void V792::test_memory_write(uint16_t address, uint32_t word) {
  write16(0x1036, address);
  write16(0x1038, word >> 16);
  write16(0x103A, word);
};

void V792::test_event_write(uint16_t events[32]) {
  for (int i = 0; i < 16; ++i) {
    write16(0x103E, events[i]);
    write16(0x103E, events[i + 16]);
  };
};

void V792::test_event_write(TestEvent events[32]) {
  test_event_write(reinterpret_cast<uint16_t*>(events));
};

};
