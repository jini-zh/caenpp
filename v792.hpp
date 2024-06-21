#pragma once

#include "caen.hpp"

namespace caen {

class V792: public Device {
  public:
    enum Version {
      V792A,
      V792N
    };

    // Status register 1 structure
    class Status1: public BitField<16> {
      public:
        Status1(uint16_t value): BitField<16>(value) {};

#define defbit(name, index) bool name() const { return bit(index); }
        defbit(data_ready,      0);
        defbit(global_ready,    1);
        defbit(busy,            2);
        defbit(global_busy,     3);
        defbit(amnesia,         4);
        defbit(purged,          5);
        defbit(termination_on,  6);
        defbit(termination_off, 7);
        defbit(events_ready,    8);
#undef defbit
    };

    // Control register 1 structure
    class Control1: public BitField<8> {
      public:
        Control1(uint8_t value): BitField<8>(value) {};

#define defbit(name, index) \
        bool name() const { return bit(index); }; \
        void set_ ## name(bool value) { set_bit(index, value); }

        defbit(block_readout,         2);
        defbit(panel_resets_software, 4);
        defbit(bus_error_enabled,     5);
        defbit(align_64,              6);
#undef defbit
    };

    // Status register 2 structure
    class Status2: public BitField<8> {
      public:
        Status2(uint8_t value): BitField<8>(value) {};

        bool    buffer_empty()    const { return bit(1);     };
        bool    buffer_full()     const { return bit(2);     };
        // type of the piggy-back plugged into the board
        uint8_t piggy_back_type() const { return bits(4, 7); };
    };

    // Bit set 2 register structure.
    // Bit set 2 operates through two registers: one reads the register and
    // allows for setting bits to 1, the other allows for clearing the bits.
    // Use the `set` function to set/clear bits in the bit field.
    class BitSet2: public BitField<16> {
      public:
        static const uint16_t mask = 0x79DF; // excludes reserved bits

        BitSet2(): BitField<16>(0) {};
        BitSet2(uint16_t value): BitField<16>(value & mask) {};

#define defbit(name, n, index) \
        bool name() const { return bit(n index); }; \
        void set_ ## name(bool value) { set_bit(index, n value); }
        defbit(test_memory,,                 0);
        defbit(offline,,                     1);
        defbit(clear_data,,                  2);
        defbit(overflow_enabled,,            3);
        defbit(threshold_enabled,!,          4);
        defbit(test_acquisition,,            6);
        defbit(slide_enabled,,               7);
        defbit(shift_threshold,,             8);
        defbit(auto_increment,,             11);
        defbit(empty_enabled,,              12);
        defbit(slide_subtraction_enabled,!, 13);
        defbit(all_triggers,,               14);
#undef defbit

        void set(bool value) {
          value_ = value ? mask : 0;
        };
    };

    // Test event write register structure
    class TestEvent: public BitField<16> {
      public:
        TestEvent(uint16_t value): BitField<16>(value) {};
        TestEvent(uint16_t value, bool overflow): BitField<16>(value & 0xFFF) {
          set_overflow(overflow);
        };

        uint16_t value()    const { return bits(0, 11); };
        void     set_value(uint16_t value)   { set_bits(0, 11, value); };
        bool     overflow() const { return bit(12); };
        void     set_overflow(bool overflow) { set_bit(12, overflow); };
    };

    class ChannelSettings: public BitField<16> {
      public:
        ChannelSettings(uint16_t value): BitField<16>(value) {};

        uint8_t threshold() const { return bits(0, 7); };
        void    set_threshold(uint8_t value) { set_bits(0, 7, value); };
        bool    disabled()  const { return bit(8); };
        void    set_disabled(bool value) { set_bit(8, value); };
    };

    // Data packets
    class Packet: public BitField<32> {
      public:
        enum Type {
          Header     = 0b010,
          Data       = 0b000,
          EndOfBlock = 0b100,
          Invalid    = 0b110
        };

        static const uint32_t invalid = 0x60000;

        Packet(uint32_t value = invalid): BitField<32>(value) {};

        uint8_t type() const { return bits(24, 26); };
        template <typename P> P as() const {
          static_assert(std::is_base_of<Packet, P>::value);
          return P(value_);
        };
    };

    class Header: public Packet {
      public:
        Header(uint32_t value): Packet(value) {};

        uint8_t count() const { return bits( 8, 13); };
        uint8_t crate() const { return bits(16, 23); };
        uint8_t type()  const { return bits(24, 26); };
        uint8_t geo()   const { return bits(27, 31); };
    };

    // XXX: V792 and V792N have slightly different data formats: bit width of
    // the `channel` field differs incompatibly. Use the proper packet type
    // when readout, or shift the channel number one bit to the right.
    class Data: public Packet {
      public:
        Data(uint32_t value): Packet(value) {};

        uint16_t value()     const { return bits(0, 11);  };
        bool     overflow()  const { return bit(12);      };
        bool     underflow() const { return bit(13);      };
        uint8_t  channel()   const { return bits(16, 20); };
        uint8_t  type()      const { return bits(24, 26); };
        uint8_t  geo()       const { return bits(27, 31); };
    };

    // For V792N, see the comment above.
    class NData: public Packet {
      public:
        NData(uint32_t value): Packet(value) {};

        uint16_t value()     const { return bits(0, 11);  };
        bool     overflow()  const { return bit(12);      };
        bool     underflow() const { return bit(13);      };
        uint8_t  channel()   const { return bits(17, 20); };
        uint8_t  type()      const { return bits(24, 26); };
        uint8_t  geo()       const { return bits(27, 31); };
    };

    class EndOfBlock: public Packet {
      public:
        EndOfBlock(uint32_t value): Packet(value) {};

        uint32_t event() const { return bits( 0, 23); };
        uint8_t  type()  const { return bits(24, 26); };
        uint8_t  geo()   const { return bits(27, 31); };
    };

    class Invalid: public Packet {
      public:
        Invalid(uint32_t value): Packet(value) {};

        uint8_t type() const { return bits(24, 26); };
    };

    class Buffer: public caen::Buffer<Packet, 34 * 32> {
      public:
        // Use this function to avoid tiresome casting. Packet is just a
        // uint32_t under the hood.
        uint32_t* raw() noexcept {
          return reinterpret_cast<uint32_t*>(data());
        };

        const uint32_t* raw() const noexcept {
          return reinterpret_cast<const uint32_t*>(data());
        };
    };


    V792(const Connection&);
    // Use this constructor to override the board version
    // XXX: I don't know the identifier of the V792N board version
    V792(const Connection&, Version);

    uint16_t firmware_revision() const {
      return read16(0x1000);
    };

    uint8_t geo_address() const {
      return read16(0x1002);
    };

    void set_geo_address(uint8_t address) {
      write16(0x1002, address);
    };

    uint8_t mcst_address() const {
      return read16(0x1004);
    };

    void set_mcst_address(uint8_t address) {
      write16(0x1004, address);
    };
    
    bool bus_error() const {
      return read16(0x1006) & 0x8;
    };

    void set_bus_error(bool value) {
      write16(value ? 0x1006 : 0x1008, 0x8);
    };

    bool sw_address_enabled() const {
      return read16(0x1006) & 0x10;
    };

    void set_sw_address_enabled(bool value) {
      return write16(value ? 0x1006 : 0x1008, 0x10);
    };

    // use `reset` to reset the board
    bool software_reset() const {
      return read16(0x1006) & 0x20;
    };

    void set_software_reset(bool value) {
      write16(value ? 0x1006 : 0x1008, 0x20);
    };

    void reset() {
      write16(0x1006, 0x20);
      write16(0x1008, 0x20);
    };

    uint8_t interrupt_level() const {
      return read16(0x100A);
    };

    void set_interrupt_level(uint8_t level) {
      write16(0x100A, level);
    };

    uint8_t interrupt_vector() const {
      return read16(0x100C);
    };

    void set_interrupt_vector(uint8_t vector) {
      write16(0x100C, vector);
    };

    Status1 status1() const {
      return read16(0x100E);
    };

    // These functions get values available through the Status register 1. Each
    // call reads the register. For a more efficient approach use the `status1`
    // function above.
#define defbit(name) bool name() const { return status1().name(); }
    defbit(data_ready);
    defbit(global_ready);
    defbit(busy);
    defbit(global_busy);
    defbit(amnesia);
    defbit(purged);
    defbit(termination_on);
    defbit(termination_off);
    defbit(events_ready);
#undef defbit

    Control1 control1() const {
      return read16(0x1010);
    };

    void set_control1(Control1 value) {
      write16(0x1010, value);
    };

    // These functions get and set values available through the Control 1
    // register. Each call to a getter reads the register; each call to a
    // setter both reads and writes the register flipping the required bits.
    // For a more efficient approach use `contol1` and `set_control1` functions.
#define defbit(name) \
    bool name() const { return control1().name(); }; \
    void set_ ## name(bool value) { \
      Control1 c = control1(); \
      c.set_ ## name(value); \
      set_control1(c); \
    }

    defbit(block_readout);
    defbit(panel_resets_software);
    defbit(bus_error_enabled);
    defbit(align_64);
#undef defbit

    uint16_t address() const {
      return read(0x1012, 2, 2);
    };

    void set_address(uint16_t address) {
      write16(0x1012, address >> 8);
      write16(0x1014, address & 0xFF);
    };

    void single_shot_reset() {
      write16(0x1016, 1);
    };

    void set_mcst_control(bool first, bool last) {
      write16(0x101A, (first & 1) << 1 | last & 1);
    };

    uint8_t event_trigger() const {
      return read16(0x1020);
    };

    void set_event_trigger(uint8_t value) {
      write16(0x1020, value);
    };

    Status2 status2() const {
      return read16(0x1022);
    };

    // These functions get values available through the Status register 2. Each
    // call reads the register. For a more efficient approach use the `status2`
    // function above.
    bool    buffer_empty()    const { return status2().buffer_empty();    };
    bool    buffer_full()     const { return status2().buffer_full();     };
    uint8_t piggy_back_type() const { return status2().piggy_back_type(); };

    uint32_t event_counter() const {
      return read16(0x1024) | read16(0x1026) << 16;
    };

    void increment_event() {
      write16(0x1028, 1);
    };

    void increment_offset() {
      write16(0x102A, 1);
    };

    float fast_clear_window() const;
    void set_fast_clear_window(float window);

    BitSet2 bitset2() const {
      return read16(0x1032);
    };

    void set_bitset2(BitSet2 value) {
      write16(0x1032, value);
    };

    void clear_bitset2(BitSet2 value) {
      write16(0x1034, value);
    };

    // These functions get and set values available through the Bit Set 2 and
    // Bit Clear 2 registers. Each call to a getter reads a register; each call
    // to a setter both reads and writes to a register. For a more efficient
    // approach use the `bitset2`, `set_bitset2` and `clear_bitset2` functions.
#define defbit(name) \
    bool name() const { return bitset2().name(); }; \
    void set_ ## name(bool value) { \
      BitSet2 b; \
      b.set_ ## name(true); \
      write16(value ? 0x1032 : 0x1034, b); \
    }
    defbit(test_memory);
    defbit(offline);
    defbit(clear_data);
    defbit(overflow_enabled);
    defbit(threshold_enabled);
    defbit(test_acquisition);
    defbit(slide_enabled);
    defbit(shift_threshold);
    defbit(auto_increment);
    defbit(empty_enabled);
    defbit(slide_subtraction_enabled);
    defbit(all_triggers);
#undef defbit

    void clear() {
      write16(0x1032, 4);
      write16(0x1034, 4);
    };

    void test_memory_write(uint16_t address, uint32_t word);

    uint8_t crate_number() const {
      return read16(0x103C);
    };

    void set_crate_number(uint8_t n) {
      write16(0x103C, n);
    };

    void test_event_write(uint16_t events[32]);
    void test_event_write(TestEvent events[32]);

    void reset_event_counter() {
      write16(0x1040, 1);
    };

    // XXX: pedestal step is not defined
    uint8_t current_pedestal() const {
      return read16(0x1060);
    };

    void set_current_pedestal(uint8_t pedestal) {
      write16(0x1060, pedestal);
    };

    void set_test_memory_read_address(uint16_t address) {
      write16(0x1064, address);
    };

    uint16_t test_register() const {
      return read16(0x1068);
    };

    void set_test_register(uint16_t value) {
      write16(0x1068, value);
    };

    uint8_t slide_constant() const {
      return read16(0x106A);
    };

    void set_slide_constant(uint8_t value) {
      write16(0x106A, value);
    };

    uint16_t AAD() const {
      return read16(0x1070);
    };

    uint16_t BAD() const {
      return read16(0x1072);
    };

    ChannelSettings channel_settings(uint8_t channel) const {
      return read16(0x1080 + channel * channel_step_);
    };

    void set_channel_settings(uint8_t channel, ChannelSettings settings) {
      write16(0x1080 + channel * channel_step_, settings);
    };

    void set_channel_settings(
        uint8_t channel, uint8_t threshold, bool enabled
    ) {
      write16(
          0x1080 + channel * channel_step_,
          threshold & 0x7F | enabled << 7
      );
    };

    uint8_t channel_threshold(uint8_t channel) const {
      return channel_settings(channel).threshold();
    };

    void set_channel_threshold(uint8_t channel, uint8_t threshold) {
      ChannelSettings s = channel_settings(channel);
      s.set_threshold(threshold);
      set_channel_settings(channel, s);
    };

    bool channel_enabled(uint8_t channel) const {
      return !channel_settings(channel).disabled();
    };

    void set_channel_enabled(uint8_t channel, bool value) {
      ChannelSettings s = channel_settings(channel);
      s.set_disabled(!value);
      set_channel_settings(channel, s);
    };

    // Manufacturer identifier (OUI) --- should be 0x40E6
    uint32_t oui() const {
      return read(0x8026, 3, 4);
    };

    uint8_t version() const {
      return read16(0x8032);
    };

    // Board ID: 792
    uint32_t id() const {
      return read(0x8036, 3, 4);
    };

    uint16_t revision() const {
      return read16(0x804E);
    };

    uint16_t serial() const {
      return read(0x8F02, 2, 4);
    };

    uint32_t readout(uint32_t* buffer, uint32_t size) {
      return blt_read(0, buffer, size);
    };

    uint32_t readout(Packet* buffer, uint32_t size) {
      return readout(reinterpret_cast<uint32_t*>(buffer), size);
    };

    void readout(Buffer& buffer) {
      buffer.resize(readout(buffer.raw(), buffer.max_size()));
    };

    // My board V792AA (board revision 4, firmware revision 0x501) duplicates
    // packets and corrupts the event structure with `readout`. If yours does
    // so too, consider using this function. Unfortunately, the board does not
    // assert the bus error when opearting in this mode, so
    // `set_bus_error_enabled` is useless in this case.
    // `wa` stands for workaround.
    uint32_t readout_wa(uint32_t* buffer, uint32_t size);

    uint32_t readout_wa(Packet* buffer, uint32_t size) {
      return readout_wa(reinterpret_cast<uint32_t*>(buffer), size);
    };

    void readout_wa(Buffer& buffer) {
      buffer.resize(readout_wa(buffer.raw(), buffer.max_size()));
    };

  private:
    int      vme_handle_;
    uint32_t vme_address_;
    uint8_t  channel_step_;

    void init(const Connection&, Version);
};

};
