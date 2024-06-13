#pragma once

#include <stdexcept>

#include <CAENComm.h>

namespace caen {

class Error: public std::exception {};

template <int bits> struct UInt_overflow__ {
  static_assert(false, "UInt: too many bits for an integer");
};

template <unsigned bits>
using UInt = typename std::conditional<
  bits <= 8,
  uint8_t,
  typename std::conditional<
    bits <= 16,
    uint16_t,
    typename std::conditional<
      bits <= 32,
      uint32_t,
      typename std::conditional<
        bits <= 64,
        uint64_t,
        UInt_overflow__<bits>
      >::type
    >::type
  >::type
>::type;

template <unsigned NBits>
class BitField {
  public:
    using value_type = UInt<NBits>;

    BitField(value_type value): value_(value) {};

    operator value_type() { return value_; };

  protected:
    value_type value_;

    bool bit(unsigned index) const {
      return value_ & (static_cast<value_type>(1) << index);
    };

    void set_bit(unsigned index, bool bit) {
      value_type mask = static_cast<value_type>(1) << index;
      value_ = value_ & ~mask | static_cast<value_type>(bit) << index & mask;
    };

    value_type bits(unsigned start, unsigned end) const {
      return (value_ & (static_cast<value_type>(1) << end + 1) - 1) >> start;
    };

    void set_bits(unsigned start, unsigned end, value_type bits) {
      value_type mask = ~(~static_cast<value_type>(1) << end - start) << start;
      value_ = value_ & ~mask | bits << start & mask;
    };
};

class Device {
  public:
    struct Connection {
      CAENComm_ConnectionType type;
      uint32_t                arg;
      uint32_t                conet;
      uint32_t                vme;
      std::string             ip;

      bool is_ethernet() const;
    };

    // CAEN device errors (with the error code)
    class Error: public caen::Error {
      public:
        Error(CAENComm_ErrorCode code): code_(code) {};
        ~Error() throw();

        const char* what() const throw();
        CAENComm_ErrorCode code() const { return code_; };

      private:
        CAENComm_ErrorCode code_;
        mutable char* message = nullptr;
    };

    // Connected to a wrong device --- identification data does not match
    class WrongDevice: public caen::Error {
      public:
        WrongDevice(Connection connection, std::string expected):
          connection_(std::move(connection)),
          expected_(std::move(expected))
        {};

        const std::string& expected()   const { return expected_;   };
        const Connection&  connection() const { return connection_; };

        const char* what() const noexcept;

      private:
        Connection  connection_;
        std::string expected_;

        mutable std::string message;
    };

    Device(const Connection& connection);

    Device(Device&& device): handle(device.handle) {
      device.handle = -1;
    };

    virtual ~Device();

    // These templates are implemented in terms of the functions below. They
    // are intended for generic programming; use the functions if it's more
    // convenient.
    template <unsigned NBits> UInt<NBits> read(uint32_t address) const;
    template <unsigned NBits> void write(uint32_t address, UInt<NBits> data);

    // Read a register
    uint16_t read16(uint32_t address) const;
    uint32_t read32(uint32_t address) const;

    // Write a register
    void write16(uint32_t address, uint16_t data);
    void write32(uint32_t address, uint32_t data);

    // Read a block of data using a BLT (32-bit) cycle.
    // Returns the number of words read.
    uint32_t blt_read(uint32_t address, uint32_t* buffer, unsigned size) const;
    // Read a block of data using a MBLT (64-bit) cycle.
    // Returns the number of words read.
    uint32_t mblt_read(uint32_t address, uint32_t* buffer, unsigned size) const;

  protected:
    int handle;
};

template <> uint16_t Device::read<16>(uint32_t address) const;
template <> uint32_t Device::read<32>(uint32_t address) const;
template <> void Device::write<16>(uint32_t address, uint16_t data);
template <> void Device::write<32>(uint32_t address, uint32_t data);

} // namespace caen
