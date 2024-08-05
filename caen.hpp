#pragma once

#include <array>
#include <stdexcept>

#include <CAENComm.h>

namespace caen {

class Error: public std::exception {};

template <unsigned bits> struct UInt_overflow__ {
  static_assert(bits, "UInt: too many bits for an integer");
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

    value_type value_;

    BitField(value_type value = 0): value_(value) {};

    operator value_type() { return value_; };

    bool bit(unsigned index) const {
      return value_ & (static_cast<value_type>(1) << index);
    };

    void set_bit(unsigned index, bool bit) {
      value_type mask = static_cast<value_type>(1) << index;
      value_ = value_ & ~mask | static_cast<value_type>(bit) << index & mask;
    };

    value_type bits(unsigned start, unsigned end) const {
      return (value_ & ~(~static_cast<value_type>(1) << end)) >> start;
    };

    void set_bits(unsigned start, unsigned end, value_type bits) {
      value_type mask = ~(~static_cast<value_type>(1) << end - start) << start;
      value_ = value_ & ~mask | bits << start & mask;
    };
};

// A fixed size array with a fill pointer that specifies how many items are
// stored in the buffer. Provides std::array API.
template <typename Item, unsigned Size>
class Buffer {
  private:
    using array = std::array<Item, Size>;

  public:
    Buffer() {};

    Item& at(size_t index) {
      if (index >= fill_)
        throw std::out_of_range("caen::Buffer: at: out of range");
      return data_[index];
    };

    const Item& at(size_t index) const {
      if (index >= fill_)
        throw std::out_of_range("caen::Buffer: at: out of range");
      return data_[index];
    };

    Item& operator[](size_t index) {
      return data_[index];
    };

    const Item& operator[](size_t index) const {
      return data_[index];
    };

    Item& front() {
      return data_.front();
    };

    const Item& front() const {
      return data_.front();
    };

    Item& back() {
      return data_[fill_ - 1];
    };

    const Item& back() const {
      return data_[fill_ - 1];
    };

    Item* data() noexcept { return data_.data(); };
    const Item* data() const noexcept { return data_.data(); };

    typename array::iterator begin() noexcept {
      return data_.begin();
    };

    typename array::const_iterator cbegin() const noexcept {
      return data_.cbegin();
    };

    typename array::iterator end() noexcept {
      return data_.begin() + fill_;
    };

    typename array::const_iterator cend() const noexcept {
      return data_.cbegin() + fill_;
    };

    typename array::reverse_iterator rbegin() noexcept {
      return data_.rend() - fill_;
    };

    typename array::const_reverse_iterator crbegin() const noexcept {
      return data_.crend() - fill_;
    };

    typename array::reverse_iterator rend() noexcept {
      return data_.rend();
    };

    typename array::const_reverse_iterator crend() const noexcept {
      return data_.crend();
    };

    bool empty() const noexcept {
      return fill_ == 0;
    };

    size_t size() const noexcept {
      return fill_;
    };

    // adjust the fill pointer
    void resize(size_t size) {
      if (size > Size)
        throw std::out_of_range("caen::Buffer: resize: out of range");
      fill_ = size;
    };

    constexpr size_t max_size() const noexcept {
      return data_.size();
    };

    void fill(const Item& item) {
      data_.fill(item);
    };

    void swap(Buffer& buffer) {
      data_.swap(buffer.data_);
      std::swap(fill_, buffer.fill_);
    };

  protected:
    array  data_;
    size_t fill_;
};

class Device {
  public:
    struct Connection {
      CAENComm_ConnectionType link;
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

    int comm_handle() const { return handle; };
    int vme_handle()  const;

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

    // Read a number stored in big endian notation in lower 8 bits of `nwords`
    // sequential 16 bits registers separated by 4 bytes in the address space
    uint32_t read(
        uint32_t address,
        unsigned nwords /* must be 4 or less */,
        uint32_t step
    ) const;
};

template <> uint16_t Device::read<16>(uint32_t address) const;
template <> uint32_t Device::read<32>(uint32_t address) const;
template <> void Device::write<16>(uint32_t address, uint16_t data);
template <> void Device::write<32>(uint32_t address, uint32_t data);

} // namespace caen
