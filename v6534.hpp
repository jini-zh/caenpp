#pragma once

#include <algorithm>
#include <limits>
#include <string>

#include "caen.hpp"

namespace caen {

class V6534: public Device {
  public:
    class Error: public caen::Error {
      public:
        Error(const std::string& message): message(message) {};

        const char* what() const throw() {
          return message.c_str();
        };

      private:
        std::string message;
    };

    V6534(const Connection& connection);

    // Board maximum allowed voltage, V
    uint16_t vmax() const { return read16(0x0050); };
    float voltage_hwmax() const { return vmax(); };

    // Board maximum allowed current, μA
    uint16_t imax() const { return read16(0x0054); };
    // A
    float current_hwmax() const { return imax() * 1e-6; };

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
    uint16_t vset(uint8_t channel) const {
      return read_channel(channel, 0x80);
    };
    void set_vset(uint8_t channel, uint16_t voltage) {
      write_channel(channel, 0x80, std::min<uint16_t>(voltage, 60000));
    };

    // Channel voltage setting, V
    float voltage_setting(uint8_t channel) const {
      return vset(channel) * 0.1;
    };
    void set_voltage(uint8_t channel, float value) {
      set_vset(channel, value / 0.1);
    };

    // Channel current setting, 0.02 μA
    uint16_t iset(uint8_t channel) const {
      return read_channel(channel, 0x84);
    };
    void set_iset(uint8_t channel, uint16_t current) {
      write_channel(channel, 0x84, std::min<uint16_t>(current, 52500));
    };

    // Channel current setting, A
    uint16_t current_setting(uint8_t channel) const {
      return iset(channel) * 0.02e-6;
    };
    void set_current(uint8_t channel, float value) {
      set_iset(channel, value / 0.02e-6);
    };

    // Channel voltage  -- current value, 0.1 V
    uint16_t vmon(uint8_t channel) const {
      return read_channel(channel, 0x88);
    };

    // Channel voltage --- current value, V
    float voltage(uint8_t channel) const {
      return vmon(channel) * 0.1;
    };

    // Current monitor range control
    enum class IMonRange { high = 0, low = 1 };
    IMonRange imon_range(uint8_t channel) const {
      return static_cast<IMonRange>(read_channel(channel, 0xb4));
    };
    void set_imon_range(uint8_t channel, IMonRange range) {
      write_channel(channel, 0xb4, static_cast<uint16_t>(range));
    };

    // Channel current --- current value, 0.002 μA
    // Works when imon_range is set to low
    uint16_t imonL(uint8_t channel) const {
      return read_channel(channel, 0xb8);
    };
    // Channel current --- current value, 0.02 μA
    // Works when imon_range is set to high
    uint16_t imonH(uint8_t channel) const {
      return read_channel(channel, 0x8c);
    };

    float current(uint8_t channel) const {
      if (imon_range(channel) == IMonRange::high)
        return imonH(channel) * 0.02e-6;
      return imonL(channel) * 0.002e-6;
    };

    // Channel ON/OFF
    bool power(uint8_t channel) const {
      return read_channel(channel, 0x90);
    };
    void set_power(uint8_t channel, bool value) {
      write_channel(channel, 0x90, value);
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
    uint16_t status(uint8_t channel) const {
      return read_channel(channel, 0x94);
    };

    // TRIP time, 0.1 s. 10000 == infinite
    uint16_t trip_time(uint8_t channel) const {
      return read_channel(channel, 0x98);
    };
    void set_trip_time(uint8_t channel, uint16_t value) {
      write_channel(channel, 0x98, std::min<uint16_t>(value, 10000));
    };

    // TRIP time, s. Inf is inifnite
    float trip_t(uint8_t channel) const {
      uint16_t value = trip_time(channel);
      if (value >= 10000) return std::numeric_limits<float>().infinity();
      return value * 0.1;
    };
    void set_trip_t(uint8_t channel, float value) {
      set_trip_time(channel, value / 0.1);
    };

    // Software max voltage, 0.1 V
    uint16_t svmax(uint8_t channel) const {
      return read_channel(channel, 0x9c);
    };
    void set_svmax(uint8_t channel, uint16_t value) {
      write_channel(channel, 0x9c, std::min<uint16_t>(value, 60000));
    };

    // Software max voltage, V
    float voltage_max(uint8_t channel) const {
      return svmax(channel) * 0.1;
    };
    void set_voltage_max(uint8_t channel, float value) {
      set_svmax(channel, value / 0.1);
    };

    // Ramp down rate, V/s
    uint16_t ramp_down(uint8_t channel) const {
      return read_channel(channel, 0xa0);
    };
    void set_ramp_down(uint8_t channel, uint16_t value) {
      write_channel(channel, 0xa0, std::min<uint16_t>(value, 500));
    };

    // Ramp up rate, V/s
    uint16_t ramp_up(uint8_t channel) const {
      return read_channel(channel, 0xa4);
    };
    void set_ramp_up(uint8_t channel, uint16_t value) {
      write_channel(channel, 0xa4, std::min<uint16_t>(value, 500));
    };

    // Power down mode
    enum class PowerDownMode { kill = 0, ramp = 1 };
    PowerDownMode pwdown(uint8_t channel) const {
      return static_cast<PowerDownMode>(read_channel(channel, 0xa8));
    };
    void set_pwdown(int channel, PowerDownMode mode) {
      write_channel(channel, 0xa8, static_cast<uint16_t>(mode));
    };

    // Channel polarity: -1 or 1
    int8_t polarity(uint8_t channel) const {
      return read_channel(channel, 0xac) ? 1 : -1;
    };

    // Channel temperature, °C
    int16_t temperature(uint8_t channel) const {
      return read_channel(channel, 0xb0);
    };

    // Board description
    // For V6534 it is "6 Ch 6KV/1mA"
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

    // Number of channels
    uint16_t chnum() const {
      return read16(0x8100);
    };

    uint16_t nchannels() const { return 6; };

  private:
    uint16_t read_channel(uint8_t channel, uint8_t offset) const;

    void write_channel(uint8_t channel, uint8_t offset, uint16_t value) {
      if (channel > 5) return;
      write16(0x80 * channel + offset, value);
    };

    std::string read_string(uint16_t address, uint16_t size) const;
};

};
