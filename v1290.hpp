#pragma once

#include "caen.hpp"

namespace caen {

class V1290: public Device {
  public:
    enum Version {
      V1290A = 0,
      V1290N = 2
    };

    // Control register structure
    class Control: public BitField<16> {
      public:
        Control(uint16_t value): BitField<16>(value) {};

#define defbit(name, index) \
        bool name() const { return bit(index); }; \
        void set_ ## name(bool value) { set_bit(index, value); }

        // When data is exhausted during a block transfer,
        //   false: fill the rest with fillers (default)
        //   true:  generate bus error to interrupt the transfer
        defbit(bus_error_enabled, 0);

        // Software terminated status, works if the next bit is set
        defbit(sw_termination, 1);

        // Termination is selected
        //    false: via dip-switch
        //    true:  via software, see sw_termination()
        defbit(sw_termination_enabled, 2);

        // Write global header and trailer packets when there is no data
        // (default: false)
        defbit(emit_empty_events, 3);

        // Align data to 64 bit boundary on readout (default: false)
        defbit(align_64, 4);

        // Enable compensation of the INL (default: true)
        defbit(compensation_enabled, 5);

        // Test mode (default: false)
        defbit(test_fifo_enabled, 6);

        // SRAM compensation table available for the readout (default: false)
        defbit(read_compensation_sram_enabled, 7);

        // Event FIFO enabled (default: false)
        defbit(event_fifo_enabled, 8);

        // Extended trigger time tag enabled (default: false)
        defbit(ettt_enabled, 9);

        // MEB access with 16 Mb address range in BLT/MBLT/2eVME/2eSST enabled
        // (requires firmware rev. 0.C and later)
        defbit(meb_access_16mb_enabled, 12);
#undef defbit
    };

    // Status register structure
    class Status: public BitField<16> {
      public:
        Status(uint16_t value): BitField<16>(value) {};
#define defbit(name, index) bool name() const { return bit(index); }

        // There is at least 1 event in the output buffer
        defbit(data_ready, 0);

        // Almost full level has been met
        defbit(almost_full, 1);

        // Output buffer is full
        defbit(full, 2);

        // Operating mode:
        //   false: continuous storage
        //   true:  trigger matching
        defbit(triggered_mode, 3);

        // Whether TDC header and trailer packets are enabled
        defbit(tdc_headers_enabled, 4);

        // false: all control bus terminations are off
        // true:  all control bus terminations are on
        defbit(terminations, 5);

        // TDC error status
        uint8_t tdc_error() const { return bits(6, 9); };

        // Bus error occured
        defbit(bus_error, 10);

        // Board has been purged: it either has no data or has transferred all
        // its data during a CBLT and the CBLT has not ended yet
        defbit(purged, 11);

        // Time resolution in seconds
        float resolution() const { return single_resolution[bits(12, 13)]; };

        // Module in pair mode
        defbit(pair_mode, 14);

        // At least one trigger hasn't been sent to TDC. The value of this bit is
        // reset when the status register is read out
        defbit(trigger_lost, 15);
#undef defbit
    };

    // Micro Handshake register structure
    class MicroHandshake: public BitField<16> {
      public:
        MicroHandshake(uint16_t value): BitField<16>(value) {};

        // a write operation to Micro register is allowed
        bool write_ok() const { return bit(0); };
        // a read operation from Micro register is allowed
        bool read_ok()  const { return bit(1); };
    };

    // Event FIFO status register structure
    class EventFIFOStatus: public BitField<16> {
      public:
        EventFIFOStatus(uint16_t value): BitField<16>(value) {};

        // There are data in the event FIFO
        bool data_ready() const { return bit(0); };
        // Event FIFO is full (1024 32-bit words)
        bool full()       const { return bit(1); };
    };

    struct TriggerConfiguration {
      float window_width;
      float window_offset;
      float search_margin;
      float reject_margin;
      bool  time_subtraction_enabled;
    };

    class EdgeDetection: public BitField<16> {
      public:
        EdgeDetection(uint16_t value): BitField<16>(value) {};

        bool trailing() const { return bit(0); };
        void set_trailing(bool value) { set_bit(0, value); };

        bool leading()  const { return bit(1); };
        void set_leading(bool value) { set_bit(1, value); };
    };

    struct Resolution {
      float edge;
      float pulse;
    };

    // TDC internal error types. See opcode 39xx.
    class InternalErrors: public BitField<16> {
      public:
        InternalErrors(uint16_t value): BitField<16>(value) {};

#define defbit(name, index) \
        bool name() const { return bit(index); }; \
        void set_ ## name(bool value) { set_bit(index, value); }

        // Vernier error (DLL unlocked or excessive jitter)
        defbit(vernier, 0);

        // Coarse error (parity error on coarse count)
        defbit(coarse, 1);

        // Channel select error (synchronisation error)
        defbit(channel, 2);

        // L1 buffer parity error
        defbit(l1_parity, 3);

        // Trigger FIFO parity error
        defbit(trigger_fifo, 4);

        // Trigger matching error (state error)
        defbit(trigger, 5);

        // Readout FIFO parity error
        defbit(readout_fifo, 6);

        // Readout state error
        defbit(readout, 7);

        // Set up parity error
        defbit(setup, 8);

        // Control parity error
        defbit(control, 9);

        // Jtag instruction parity error
        defbit(jtag, 10);
#undef defbit
    };

    // Global time offset. See opcode 51xx
    struct GlobalOffset {
      uint16_t coarse;
      uint8_t  fine;
    };

    // Microcontroller firmware revision and date
    struct MicroRevision {
      uint16_t version;
      uint16_t day;
      uint16_t month;
      uint16_t year;
    };

    // Data packets
    class Packet: public BitField<32> {
      public:
        enum Type: uint8_t {
          GlobalHeader           = 0b01000,
          TDCHeader              = 0b00001,
          TDCMeasurement         = 0b00000,
          TDCTrailer             = 0b00011,
          TDCError               = 0b00100,
          GlobalTrailer          = 0b10000,
          ExtendedTriggerTimeTag = 0b10001,
          Filler                 = 0b11000
        };

        static const uint32_t filler = 0xC0000000U;

        Packet(uint32_t value = filler): BitField<32>(value) {};

        uint8_t type() const { return bits(27, 31); };

        template <typename P> P as() const {
          static_assert(std::is_base_of<Packet, P>::value);
          return P(value_);
        };
    };

    class GlobalHeader: public Packet {
      public:
        GlobalHeader(uint32_t value): Packet(value) {};

        uint8_t  geo()     const { return bits( 0,  4); };
        uint32_t nevents() const { return bits( 5, 26); };
        uint8_t  type()    const { return bits(27, 31); };
    };

    class TDCHeader: public Packet {
      public:
        TDCHeader(uint32_t value): Packet(value) {};

        uint16_t bunch() const { return bits( 0, 11); };
        uint16_t event() const { return bits(12, 23); };
        uint8_t  tdc()   const { return bits(24, 25); };
        uint8_t  type()  const { return bits(27, 31); };
    };

    class TDCMeasurement: public Packet {
      public:
        TDCMeasurement(uint32_t value): Packet(value) {};

        uint32_t value()    const { return bits( 0, 20); };
        uint8_t  channel()  const { return bits(21, 25); };
        bool     trailing() const { return bit(26);      };
        uint8_t  type()     const { return bits(27, 31); };
    };

    class TDCTrailer: public Packet {
      public:
        TDCTrailer(uint32_t value): Packet(value) {};

        uint16_t nwords() const { return bits( 0, 11); };
        uint16_t event()  const { return bits(12, 23); };
        uint8_t  tdc()    const { return bits(24, 25); };
        uint8_t  type()   const { return bits(27, 31); };
    };

    class TDCError: public Packet {
      public:
        TDCError(uint32_t value): Packet(value) {};

        uint16_t errors() const { return bits( 0, 14); };
        uint8_t  tdc()    const { return bits(24, 25); };
        uint8_t  type()   const { return bits(27, 31); };
    };

    class ExtendedTriggerTimeTag: public Packet {
      public:
        ExtendedTriggerTimeTag(uint32_t value): Packet(value) {};

        uint32_t value() const { return bits( 0, 26); };
        uint8_t  type()  const { return bits(27, 31); };
    };

    class GlobalTrailer: public Packet {
      public:
        GlobalTrailer(uint32_t value): Packet(value) {};

        uint8_t  geo()          const { return bits( 0,  4); };
        uint16_t nwords()       const { return bits(5, 20);  };
        bool     errors()       const { return bit(24);      };
        bool     overflow()     const { return bit(25);      };
        bool     trigger_lost() const { return bit(26);      };
        uint8_t  type()         const { return bits(27, 31); };
    };

    class Filler: public Packet {
      public:
        Filler(uint32_t value): Packet(value) {};

        uint8_t type() const { return bits(27, 31); };
    };

    class Buffer: public caen::Buffer<Packet, 32 * 1024> {
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

    // TDC time resolution in single mode (only the leading or the trailing
    // edge of the signal is detected) in seconds. Descending order.
    static const float single_resolution[4];
    // TDC time resolution in pair mode (both the leading and the trailing edge
    // of the signal is detected, pulse width is measured) in seconds.
    // Ascending order.
    // XXX: The last two values are 0 (invalid).
    static const float pair_resolution[16];
    // TDC double hit time resolution (dead time between two subsequent hits)
    // in seconds.  Ascending order.
    static const float dead_times[4];

    V1290(const Connection& connection);
    V1290(V1290&& device);

    uint16_t rom_checksum() const {
      return read16(0x4000);
    };

    // should be 0x20
    uint32_t rom_checksum_length() const {
      return read(0x4004, 3, 4);
    };

    // A constant stored in ROM --- should be 0x838401
    uint32_t rom_constant() const {
      return read(0x4010, 3, 4);
    };

    // ROM C code --- should be 0x43
    uint16_t rom_c_code() const {
      return read16(0x401C);
    };

    // ROM R code --- should be 0x52
    uint16_t rom_r_code() const {
      return read16(0x4020);
    };

    // Manufacturer identifier (OUI) --- should be 0x40E6
    uint32_t oui() const {
      return read(0x4024, 3, 4);
    };

    // Board version
    Version version() const { return version_; };

    // Board ID: 0x050A (1290)
    uint32_t id() const {
      return read(0x4034, 3, 4);
    };

    // Board revision
    uint32_t revision() const {
      return read(0x4040, 4, 4);
    };

    // Board serial number
    uint32_t serial() const {
      return read(0x4080, 2, 4);
    };

    // Control register
    Control control() const {
      return read16(0x1000);
    };
    void set_control(Control value) {
      write16(0x1000, value);
    };

    // These functions get and set values available through the Control
    // register. Each call to a getter reads the register; each call to a
    // setter both reads and writes the register flipping the required bits.
    // For a more efficient approach use `contol` and `set_control` functions.
    // For the description of each value see the V1290::Control class.
#define defbit(name) \
    bool name() const { return control().name(); }; \
    void set_ ## name(bool value) { \
      Control c = control(); \
      c.set_ ## name(value); \
      set_control(c); \
    }

    defbit(bus_error_enabled);
    defbit(sw_termination);
    defbit(sw_termination_enabled);
    defbit(emit_empty_events);
    defbit(align_64);
    defbit(compensation_enabled);
    defbit(test_fifo_enabled);
    defbit(read_compensation_sram_enabled);
    defbit(event_fifo_enabled);
    defbit(ettt_enabled);
    defbit(meb_access_16mb_enabled);
#undef defbit

    // Status register
    Status status() const {
      return read16(0x1002);
    };

    // These functions get values available through the Status register. Each
    // call reads the register. For a more efficient approach use the `status`
    // function above. For the description of each value see the V1290::Status
    // class.
    // XXX: Note that the value of the `trigger_lost` field is reset each time
    // the register is read.
#define defbit(name) bool name() const { return status().name(); }
    defbit(data_ready);
    defbit(almost_full);
    defbit(full);

    // skipped in favour of the opcode, see `triggered_mode` below
    // defbit(triggered_mode);

    defbit(tdc_headers_enabled);
    defbit(terminations);
    uint8_t tdc_error() const { return status().tdc_error(); };
    defbit(bus_error);
    defbit(purged);

    // skipped in favour of the opcode, see `resolution` below
    // defbit(resolution);

    defbit(pair_mode);
    defbit(trigger_lost);
#undef defbit

    // VME address
    uint32_t address() const {
      return read(0x1004, 2, 2) << 16;
    };
    void set_address(uint32_t value) {
      write16(0x1004, value >> 24);
      write16(0x1006, value >> 16 & 0xFF);
    };

    // false: base address is selected via rotary switches on the board (default)
    // true:  the address set via `set_address` is used
    void sw_address_enabled(bool value) {
      write16(0x1008, value);
    };

    // Interrupt level
    uint8_t interrupt_level() const {
      return read16(0x100A) & 0x7;
    };
    void set_interrupt_level(uint8_t level) {
      write16(0x100A, level);
    };

    // Interrupt vector --- STATUS/ID that the board places on VME data bus
    // during the interrupt Acknowledge cycle. Default is 0xDD
    uint8_t interrupt_vector() const {
      return read16(0x100C);
    };
    void set_interrupt_vector(uint8_t vector) {
      write16(0x100C, vector);
    };

    // GEO address --- transferred to header and trailer words for data
    // identification. Default is 0x1F
    uint8_t geo_address() const {
      return read16(0x100E) & 0x1F;
    };
    void set_geo_address(uint8_t address) {
      write16(0x100E, address);
    };

    // MSCT base address (most significant bits)
    uint8_t mcst_base_address() const {
      return read16(0x1010);
    };
    void set_mcst_base_address(uint8_t address) {
      write16(0x1010, address);
    };

    // MSCT/CBLT control register
    // 0: disabled
    // 1: last
    // 2: first
    // 3: intermediate
    uint8_t mcst_control() const {
      return read16(0x1012) & 0x3;
    };
    void set_mcst_control(uint8_t value) {
      write16(0x1012, value);
    };

    // Module reset
    void reset() {
      write16(0x1014, 1);
    };

    // Software clear
    void clear() {
      write16(0x1016, 1);
    };

    // Software event reset
    void reset_event() {
      write16(0x1018, 1);
    };

    // Software trigger
    void trigger() {
      write16(0x101A, 1);
    };

    // Event counter
    uint32_t event_counter() const {
      return read32(0x101C);
    };

    // Event stored --- number of events currently stored in the output buffer
    uint16_t event_stored() const {
      return read16(0x1020);
    };

    // Almost full level --- when should status() report almost_full
    uint16_t almost_full_level() const {
      return read16(0x1022);
    };
    void set_almost_full_level(uint16_t level) {
      write16(0x1022, level);
    };

    // BLT event number
    uint8_t blt_event_number() const {
      return read16(0x1024);
    };
    void set_blt_event_number(uint8_t number) {
      write16(0x1024, number);
    };

    // Firmware revision
    uint8_t firmware_revision() const {
      return read16(0x1026) & 0xFF;
    };

    // Test register
    uint32_t test() const {
      return read32(0x1028);
    };
    void set_test(uint32_t value) {
      write32(0x1028, value);
    };

    // OUT_PROG control. Sets the function of the OUT_PROG ECL output on the control connector to
    //   0: data ready
    //   1: full
    //   2: almost full
    //   3: error
    uint8_t out_prog() const {
      return read16(0x102C) & 0x7;
    };
    void set_out_prog(uint8_t value) {
      write16(0x102C, value);
    };

    // Micro Handshake: all read and write operations with the Micro register
    // can be performed, respectively, when the bit `read_ok` or `write_ok` is
    // set
    MicroHandshake micro_handshake() const {
      return read16(0x1030) & 0x3;
    };

    // These functions get values available through the Micro Handshake
    // register. Each call reads the register. For a more efficient approach
    // use the `micro_handshake` function above. For the description, see the
    // V1290::MicroHandshake class.
    bool micro_write_ok() const { return micro_handshake().write_ok(); };
    bool micro_read_ok()  const { return micro_handshake().read_ok();  };

    // Dummy32 --- for testing
    uint32_t dummy32() const {
      return read32(0x1200);
    };
    void set_dummy32(uint32_t value) {
      write32(0x1200, value);
    };

    // Dummy16 --- for testing
    uint16_t dummy16() const {
      return read16(0x1204);
    };
    void set_dummy16(uint16_t value) {
      write16(0x1204, value);
    };

    // Select flash
    bool flash_selected() const {
      return !(read16(0x1034) & 1);
    };
    void select_flash(bool select) {
      write16(0x1034, !select);
    };

    // XXX: Flash memory access is not implemented yet
    // XXX: SRAM compensation page support is not implemented yet
    // XXX: Event FIFO support is not implemented yet

    // Number of events stored in Event FIFO
    uint16_t event_fifo_stored() const {
      return read16(0x103C) & 0x3ff;
    };

    EventFIFOStatus event_fifo_status() const {
      return read16(0x103E) & 3;
    };

    // These functions get values available through the Event FIFO Status
    // register. Each call reads the register. For a more efficient approach
    // use the `event_fifo_status` function above. For the description, see the
    // V1290::EventFIFOStatus class.
    bool event_fifo_ready() const { return event_fifo_status().data_ready(); };
    bool event_fifo_full()  const { return event_fifo_status().full();       };

    void set_triggered_mode(bool enabled) {
      micro_write(enabled ? 0x0000 : 0x0100);
    };

    bool triggered_mode() const {
      micro_write(0x0200);
      return micro_read() & 1;
    };

    void set_keep_token(bool keep) {
      micro_write(keep ? 0x0300 : 0x0400);
    };

    void load_default_configuration() {
      micro_write(0x0500);
    };

    void save_user_configuration() {
      micro_write(0x0600);
    };

    void load_user_configuration() {
      micro_write(0x0700);
    };

    void set_autoload_user_configuration(bool load) {
      micro_write(load ? 0x0800 : 0x0900);
    };

    void set_window_width(float seconds) {
      set_time_value(0x1000, seconds);
    };

    void set_window_offset(float seconds) {
      set_time_value(0x1100, seconds);
    };

    void set_search_margin(float seconds) {
      set_time_value(0x1200, seconds);
    };

    void set_reject_margin(float seconds) {
      set_time_value(0x1300, seconds);
    };

    void set_trigger_time_subtraction(bool enabled) {
      micro_write(enabled ? 0x1400 : 0x1500);
    };

    TriggerConfiguration trigger_configuration() const;

    EdgeDetection edge_detection() const {
      micro_write(0x2300);
      return micro_read();
    };

    void set_edge_detection(bool leading, bool trailing);

    void set_edge_detection(EdgeDetection detection) {
      micro_write(0x2200);
      micro_write(detection);
    };

    Resolution resolution() const;
    void set_resolution(float edge, float pulse = 0);
    void set_resolution(Resolution r) {
      set_resolution(r.edge, r.pulse);
    };

    float dead_time() const {
      micro_write(0x2900);
      return dead_times[micro_read() & 3];
    };

    void set_dead_time(float time);

    // Whether TDC's header and trailer packets are added to the data
    bool header_and_trailer_enabled() const {
      micro_write(0x3200);
      return micro_read();
    };

    void set_header_and_trailer_enabled(bool enabled) {
      micro_write(enabled ? 0x3000 : 0x3100);
    };

    // < 0: unlimited
    int event_size() const;
    // < 0 or > 128: unlimited
    void set_event_size(int size);

    // Put an error mark in the data when a global error occurs (default)
    void enable_error_mark(bool enable) {
      micro_write(enable ? 0x3500 : 0x3600);
    };

    // Enable TDCs' bypass when a global error occurs
    void enable_error_bypass(bool enable) {
      micro_write(enable ? 0x3700 : 0x3800);
    };

    InternalErrors internal_errors() const {
      micro_write(0x3A00);
      return micro_read();
    };

    void set_internal_errors(InternalErrors errors) {
      micro_write(0x3900);
      micro_write(errors);
    };

    unsigned fifo_size() const {
      micro_write(0x3C00);
      return 2 << micro_read();
    };

    void set_fifo_size(unsigned nwords);

    void set_channel_enabled(uint8_t channel, bool enabled) {
      micro_write((enabled ? 0x4000 : 0x4100) | channel);
    };

    void set_channels_enabled(bool enabled) {
      micro_write(enabled ? 0x4200 : 0x4300);
    };

    uint32_t enabled_channels() const;
    void enable_channels(uint32_t mask);

    // Operate on individual channels on each TDC chip. Normally each
    // measurement is performed by 4 cascaded channels, only one of which is
    // connected to the input. This function allows disabling some of these
    // channels, reducing the time resolution. See Appendix B of the manual.
    // We may need a better name for this function
    uint32_t enabled_tdc_channels(uint8_t tdc) const;
    void enable_tdc_channels(uint8_t tdc, uint32_t mask);

    GlobalOffset global_offset() const {
      GlobalOffset result;
      micro_write(0x5100);
      result.coarse = micro_read();
      result.fine   = micro_read();
      return result;
    };

    void set_global_offset(uint16_t coarse, uint8_t fine) {
      micro_write(0x5000);
      micro_write(coarse);
      micro_write(fine);
    };

    void set_global_offset(GlobalOffset o) {
      set_global_offset(o.coarse, o.fine);
    };

    uint8_t channel_adjust(uint8_t channel) const {
      micro_write(0x5300 | channel);
      return micro_read();
    };

    void adjust_channel(uint8_t channel, uint8_t value) {
      micro_write(0x5200 | channel);
      micro_write(value);
    };

    uint16_t rc_adjust(uint8_t tdc) const {
      micro_write(0x5500 | tdc);
      return micro_read();
    };

    void adjust_rc(uint8_t tdc, uint16_t set) {
      micro_write(0x5400);
      micro_write(set);
    };

    void save_rc_adjust() {
      micro_write(0x5600);
    };

    uint16_t tdc_id(uint8_t tdc) const {
      micro_write(0x6000 | tdc);
      return micro_read();
    };

    uint16_t micro_revision() const {
      micro_write(0x6100);
      return micro_read();
    };

    // Resets TDCs' PLL (Phase Locked Loop) and DLL (Delay Locked Loop)
    void reset_timers() {
      micro_write(0x6200);
    };

    void scan_path_write(uint8_t address, uint16_t word) {
      micro_write(0x7000 | address);
      micro_write(word);
    };

    uint16_t scan_path_read(uint8_t address) {
      micro_write(0x7100 | address);
      return micro_read();
    };

    void scan_path_load() {
      micro_write(0x7200);
    };

    void scan_path_reload() {
      micro_write(0x7300);
    };

    InternalErrors tdc_errors(uint8_t tdc) const {
      micro_write(0x7400 | tdc);
      return micro_read();
    };

    bool dll_locked(uint8_t tdc) const {
      micro_write(0x7500 | tdc);
      return micro_read() & 1;
    };

    uint64_t tdc_status(uint8_t tdc) const;

    void scan_path_load(uint8_t tdc) {
      micro_write(0x7700 | tdc);
    };

    void eeprom_write(uint16_t address, uint8_t byte);
    uint8_t eeprom_read(uint16_t address) const;

    MicroRevision micro_revision_date() const;

    void spare_write(uint16_t value) {
      micro_write(0xC300);
      micro_write(value);
    };

    uint16_t spare_read() const {
      micro_write(0xC400);
      return micro_read();
    };

    void enable_test_mode(uint32_t test_word);

    void disable_test_mode() {
      micro_write(0xC600);
    };

    void tdc_test_output(uint8_t tdc, uint8_t output) {
      micro_write(0xC700 | tdc);
      micro_write(output);
    };

    // 0 direct 40 MHz clock (low resolution)
    // 1 PLL 40 MHz clock (low resolution)
    // 2 PLL 160 MHz clock (medium resolution)
    // 3 PLL 320 MHz clock (high resolution)
    void set_dll_clock(uint8_t clock) {
      micro_write(0xC800);
      micro_write(clock);
    };

    static const unsigned scan_path_length = 41; // of uint16_t
    void scan_path_read(uint8_t tdc, uint16_t* scan_path) const;

    uint32_t readout(uint32_t* buffer, uint16_t size) {
      return blt_read(0, buffer, size);
    };

    uint32_t readout(Packet* buffer, uint16_t size) {
      return readout(reinterpret_cast<uint32_t*>(buffer), size);
    };

    void readout(Buffer& buffer) {
      buffer.resize(readout(buffer.raw(), buffer.max_size()));
    };

  private:
    Version version_;

    void micro_wait(uint8_t bit) const;
    uint16_t micro_read() const;
    void micro_write(uint16_t value);
    // Not every call to `micro_write` changes the visible state of the board
    void micro_write(uint16_t value) const;

    float cycles_to_seconds(int16_t cycles) const {
      return cycles * 25e-9;
    };

    int16_t seconds_to_cycles(float seconds) const;

    void set_time_value(uint16_t opcode, float seconds) {
      micro_write(opcode);
      micro_write(seconds_to_cycles(seconds));
    };
};

};
