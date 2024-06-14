#include <cmath>

#include "v812.hpp"

namespace caen {

V812::V812(const Connection& connection): Device(connection) {
  if (id() != 0x851) throw WrongDevice(connection, "V812");
};

void V812::set_threshold_v(uint8_t channel, float voltage) {
  write16(channel << 1, std::round(voltage / -1e-3));
};

};
