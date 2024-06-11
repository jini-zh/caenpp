#include <cstring>

#include "v6534.hpp"

namespace caen {

V6534::V6534(const Connection& connection): Device(connection) {
  std::string model = this->model();
  if (strncmp(model.c_str(), "V6534", 5) != 0)
    throw WrongDevice(connection, "V6534");
};

uint16_t V6534::read_channel(uint8_t channel, uint8_t offset) const {
  if (channel > 5) throw Error("bad channel: " + std::to_string(channel));
  return read16(0x80 * channel + offset);
};

std::string V6534::read_string(uint16_t address, uint16_t size) const {
  std::string string(size, 0);
  for (uint16_t i = 0; i < size;) {
    uint16_t x = read16(address);
    address += 2;
    string[i++] = x & 0xff;
    string[i++] = x >> 8;
  };
  return string;
};

};
