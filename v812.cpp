#include <cmath>

#include "v812.hpp"

namespace caen {

V812::V812(const Connection& connection): Device(connection) {
  if (id() != 0x851) throw WrongDevice(connection, "V812");
};

void V812::set_threshold(uint8_t channel, float voltage) {
  uint8_t value;
  if (voltage < -255e-3)
    value = 255;
  else if (voltage > -1e-3)
    value = 0;
  else
    value = std::round(voltage / -1e-3);
  write16(channel << 1, value);
};

};
