#include <cmath>

#include <CAENVMElib.h>

#include "v792.hpp"

namespace caen {

V792::V792(const Connection& connection): Device(connection) {
  // Known versions: 0x11 (V792AA), 0x13 (V792AC), 0xE1 (V792NA), 0xE3 (V792NC)
  init(connection, version() & 0xF0 == 0xE0 ? V792N : V792A);
};

void V792::init(const Connection& connection, Version version) {
  channel_step_ = version == V792A ? 2 : 4;
  vme_handle_   = vme_handle();
  vme_address_  = connection.address;
};

bool V792::check() const {
  return oui() == OUI && id() == 792;
};

float V792::fast_clear_window() const {
  return read16(0x102E) / 32e6 + 7e-6;
}

void V792::set_fast_clear_window(float window) {
  write16(0x102E, std::round((window - 7e-6) * 32e6));
};

void V792::test_memory_write(uint16_t address, uint32_t word) {
  write16(0x1036, address);
  write16(0x1038, word >> 16);
  write16(0x103A, word);
};

void V792::test_event_write(uint16_t events[32]) {
  for (int i = 0; i < 16; ++i) {
    write16(0x103E, events[i]);
    write16(0x103E, events[i + 16]);
  };
};

void V792::test_event_write(TestEvent events[32]) {
  test_event_write(reinterpret_cast<uint16_t*>(events));
};

// This is a workaround for the packet duplication problem. CAENComm_BLTRead
// calls CAENVME_FIFO_BLTReadCycle under the hood with the cvA32_U_BLT as the
// address modifier. We change the modifier, but the board stops asserting the
// bus error when there is no more data to transfer
uint32_t V792::readout_wa(uint32_t* buffer, uint32_t size) {
  int result;
  CVErrorCodes status = CAENVME_FIFOBLTReadCycle(
      vme_handle_,
      vme_address_,
      buffer,
      size * sizeof(uint32_t),
      cvA32_U_DATA,
      cvD32,
      &result
  );

  if (status != cvSuccess && status != cvBusError) {
    // Map VME error to COMM error
    CAENComm_ErrorCode comm;
    switch (status) {
      case (cvCommError):
        comm = CAENComm_CommError;
        break;
      case (cvGenericError):
        comm = CAENComm_GenericError;
        break;
      case (cvInvalidParam):
        comm = CAENComm_InvalidParam;
        break;
      case (cvTimeoutError):
        comm = CAENComm_CommTimeout;
        break;
      case (cvAlreadyOpenError):
        comm = CAENComm_DeviceAlreadyOpen;
        break;
      case (cvMaxBoardCountError):
        comm = CAENComm_MaxDevicesError;
        break;
      case (cvNotSupported):
        comm = CAENComm_NotSupported;
        break;
      default:
        comm = static_cast<CAENComm_ErrorCode>(static_cast<int>(status) - 100);
    };
    throw Device::Error(comm);
  };

  return result / 4;
};

};
