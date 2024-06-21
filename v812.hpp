#pragma once

#include <bitset>

#include "caen.hpp"

namespace caen {

class V812: public Device {
  public:

    V812(const Connection&);
    V812(V812&& device): Device(std::move(device)) {};

    // -1 to -255 mV, in volts
    void set_threshold(uint8_t channel, float voltage);

    void enable_channels(uint16_t mask) {
      write16(0x4A, mask);
    };

    void enable_channels(std::bitset<16> mask) {
      write16(0x4A, mask.to_ulong());
    };

    // channels_set: 0 for channels 0 to 7, 1 for channels 8 to 15
    // value: 0 -> 12 ns, 255 -> 206 ns, non-linear relation in between
    void set_output_width(uint8_t channels_set, uint8_t value) {
      write16(0x40 + ((channels_set & 1) << 1), value);
    };

    // channels_set: 0 for channels 0 to 7, 1 for channels 8 to 15
    // value: 0 -> 118 ns, 255 -> 1625 ns; is it linear in between?
    void set_dead_time(uint8_t channels_set, uint8_t value) {
      write16(0x44 + ((channels_set & 1) << 1), value);
    };

    void set_majority_threshold(uint8_t value) {
      write16(0x48, value);
    };

    void test_pulse() {
      write16(0x4C, 1);
    };

    uint16_t serial() const {
      return read16(0xFE) & 0xFFF;
    };

    uint8_t version() const {
      return read16(0xFE) >> 12;
    };

    uint16_t id() const {
      return read16(0xFC);
    };

    // 0xFAF5
    uint16_t constant() const {
      return read16(0xFA);
    };
};

};
