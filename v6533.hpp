#pragma once

#include <limits>
#include <string>

#include <assert.h>

#include "caen.hpp"

namespace caen {

class V6533: public comm::Device {
  public:
    V6533(uint32_t vme, uint32_t usb = 0):
      comm::Device(CAENComm_USB, usb, 0, vme)
    {
      assert(chnum() == 6);
    };

    // Board maximum allowed voltage, V
    uint16_t vmax() const { return read16(0x0050); };

    // Board maximum allowed current, μA
    uint16_t imax() const { return read16(0x0054); };

    // Board status:
    // bit      meaning
    // 0        Channel 0 ALARM
    // 1        Channel 1 ALARM
    // 2        Channel 2 ALARM
    // 3        Channel 3 ALARM
    // 4        Channel 4 ALARM
    // 5        Channel 5 CLARM
    // 6        Reserved
    // 7        Reserved
    // 8        Board power fail
    // 9        Board overpower
    // 10       Board MAXV uncalibrated
    // 11       Board MAXI uncalibrated
    // 12 to 15 Reserved
    uint16_t status() const { return read16(0x0058); };

    // Firware release number
    uint16_t fwrel() const { return read16(0x005c); };

    // Channel voltage setting, 0.1 V
    uint16_t vset(int channel) const {
      return read16(0x80 * channel + 0x80);
    };
    void set_vset(int channel, uint16_t voltage) {
      assert(channel >= 0 && channel <= 5);
      assert(voltage <= 40000);
      write16(0x80 * channel + 0x80, voltage);
    };

    // Channel voltage setting, V
    float voltage_setting(int channel) const {
      return vset(channel) * 0.1;
    };
    void set_voltage(int channel, float value) {
      set_vset(channel, value / 0.1);
    };

    // Channel current setting, 0.05 μA
    uint16_t iset(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return read16(0x80 * channel + 0x84);
    };
    void set_iset(int channel, uint16_t current) {
      assert(channel >= 0 && channel <= 5);
      assert(current <= 62000);
      write16(0x80 * channel + 0x84, current);
    };

    // Channel current setting, A
    float current_setting(int channel) const {
      return iset(channel) * 0.05e-6;
    };
    void set_current(int channel, float value) {
      set_iset(channel, value / 0.05e-6);
    };

    // Channel voltage --- current value, 0.1 V
    uint16_t vmon(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return read16(0x80 * channel + 0x88);
    };

    // Channel voltage --- current value, V
    float voltage(int channel) const {
      return vmon(channel) * 0.1;
    };

    // Current monitor range control
    enum IMonRange { imrHigh = 0, imrLow = 1 };
    IMonRange imon_range(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return static_cast<IMonRange>(read16(0x80 * channel + 0xb4));
    };
    void set_imon_range(int channel, IMonRange range) {
      assert(channel >= 0 && channel <= 5);
      write16(0x80 * channel + 0xb4, range);
    };

    // Channel current --- current value, 0.005 μA
    // Works when imon_range is set to imrLow
    uint16_t imonL(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return read16(0x80 * channel + 0xb8);
    };
    // Channel current --- current value, 0.05 μA
    // Works when imon_range is set to imrHigh
    uint16_t imonH(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return read16(0x80 * channel + 0x8c);
    };

    // Channel current --- current value, A
    float current(int channel) const {
      if (imon_range(channel) == imrHigh)
        return imonH(channel) * 0.05e-6;
      else
        return imonL(channel) * 0.005e-6;
    };

    // Channel ON/OFF
    bool power(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return read16(0x80 * channel + 0x90);
    };
    void set_power(int channel, bool value) {
      assert(channel >= 0 && channel <= 5);
      write16(0x80 * channel + 0x90, value);
    };

    // Channel status
    // bit      meaning
    // 0        ON
    // 1        RAMP UP
    // 2        RAMP DOWN
    // 3        OVER CURRENT
    // 4        OVER VOLTAGE
    // 5        UNDER VOLTAGE
    // 6        MAXV
    // 7        MAXI
    // 8        TRIP
    // 9        OVER POWER
    // 10       OVER TEMPERATURE
    // 11       DISABLED
    // 12       INTERLOCK
    // 13       UNCALIBRATED
    // 14 to 15 Reserved
    uint16_t status(int channel) {
      assert(channel >= 0 && channel <= 5);
      return read16(0x80 * channel + 0x94);
    };

    // TRIP time, 0.1 s. 10000 == infinite
    uint16_t trip_time(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return read16(0x80 * channel + 0x98);
    };
    uint16_t set_trip_time(int channel, uint16_t value) {
      assert(channel >= 0 && channel <= 5);
      assert(value <= 10000);
      write16(0x80 * channel + 0x98, value);
    };

    // TRIP time, s. Inf is infinite
    float trip_t(int channel) const {
      uint16_t value = trip_time(channel);
      if (value == 10000) return std::numeric_limits<float>().infinity();
      return value * 0.1;
    };
    void set_trip_t(int channel, float value) {
      set_trip_time(channel, value >= 1e3 ? 10000 : value / 0.1);
    };

    // Software max voltage, 0.1 V
    uint16_t svmax(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return read16(0x80 * channel + 0x9c);
    };
    void set_svmax(int channel, uint16_t value) {
      assert(channel >= 0 && channel <= 5);
      assert(value <= 40000);
      write16(0x80 * channel + 0x9c, value);
    };

    // Software max voltage, V
    float voltage_max(int channel) const {
      return svmax(channel) * 0.1;
    };
    void set_voltage_max(int channel, float value) {
      set_svmax(channel, value / 0.1);
    };

    // Ramp down rate, V/s
    uint16_t ramp_down(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return read16(0x80 * channel + 0xa0);
    };
    void set_ramp_down(int channel, uint16_t value) {
      assert(channel >= 0 && channel <= 5);
      assert(value <= 500);
      write16(0x80 * channel + 0xa0, value);
    };

    // Ramp up rate, V/s
    uint16_t ramp_up(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return read16(0x80 * channel + 0xa4);
    };
    void set_ramp_up(int channel, uint16_t value) {
      assert(channel >= 0 && channel <= 5);
      assert(value <= 500);
      write16(0x80 * channel + 0xa4, value);
    };

    // Power down mode
    enum PowerDownMode { pdmKill = 0, pdmRamp = 1 };
    PowerDownMode pwdown(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return static_cast<PowerDownMode>(read16(0x80 * channel + 0xa8));
    };
    void set_pwdown(int channel, PowerDownMode mode) {
      assert(channel >= 0 && channel <= 5);
      write16(0x80 * channel + 0xa8, mode);
    };

    // Channel polarity: -1 or 1
    int polarity(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return read16(0x80 * channel + 0xac) ? 1 : -1;
    };

    // Channel temperature, °C
    int16_t temperature(int channel) const {
      assert(channel >= 0 && channel <= 5);
      return read16(0x80 * channel + 0xb0);
    };

    // Board description
    // For V6533 it is "6 Ch 4KV/3mA"
    std::string description() const {
      return read_string(0x8102, 20);
    };

    std::string model() const {
      return read_string(0x8116, 8);
    };

    uint16_t serial_number() const {
      return read16(0x811e);
    };

    uint16_t vme_fwrel() const {
      return read16(0x8120);
    };

  private:
    uint16_t chnum() const {
      return read16(0x8100);
    };

    std::string read_string(uint16_t offset, uint16_t size) const {
      std::string string(size, 0);
      for (uint16_t i = 0; i < size;) {
        uint16_t x = read16(offset);
        offset += 2;
        string[i++] = x & 0xff;
        string[i++] = x >> 8;
      };
      return string;
    };
};

}
