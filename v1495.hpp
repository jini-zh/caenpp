#pragma once

#include "comm.hpp"

namespace caen {

class V1495: public Device {
  public:
    static const uint16_t buffer_size = 0x1000 / sizeof(uint32_t);

    V1495(const Connection& connection): Device(connection) {};

    V1495(V1495&& device): Device(std::move(device)) {};

    V1495& operator=(V1495&& device) {
      Device::operator=(std::move(device));
      return *this;
    };

    const char* kind() const { return "V1495"; };

    uint32_t readout(uint32_t* buffer, unsigned size = buffer_size) const {
      return mblt_read(0, buffer, size);
    };

    uint32_t rom_checksum() const {
      return read32(0x8100);
    };

    // guessed
    uint32_t rom_checksum_length() const {
      return read(0x8104, 3, 4);
    };

    // guessed
    uint32_t rom_constant() const {
      return read(0x8110, 3, 4);
    };

    uint32_t rom_c_code() const {
      return read32(0x811C);
    };

    uint32_t rom_r_code() const {
      return read32(0x8120);
    };

    // Manufacturer identifier --- should be 0x40E6
    uint32_t oui() const {
      return read(0x8124, 3, 4);
    };

    uint32_t version() const {
      return read32(0x8130);
    };

    // Board ID: 0x05D7 (1495)
    uint32_t id() const {
      return read(0x8134, 3, 4);
    };

    uint32_t revision() const {
      return read(0x8140, 4, 4);
    };

    uint32_t serial_number() const {
      return read(0x8180, 2, 4);
    };

    uint8_t geo() const {
      return read16(0x8008) & 0x1F;
    };

    void reset() {
      write16(0x800C, 1);
    };

    uint16_t firmware_revision() const {
      return read16(0x800C);
    };

    uint16_t scratch16() const {
      return read16(0x8018);
    };

    void set_scratch16(uint16_t value) {
      write16(0x8018, value);
    };

    uint32_t scratch32() const {
      return read32(0x8020);
    };

    void set_scratch32(uint32_t value) {
      write32(0x8020, value);
    };

    void reload() {
      write16(0x8016, 1);
    };

  private:
    bool check() const;
};

};
