#include <sstream>

#include <cstring>

#include "comm.hpp"

namespace caen {

static const char* comm_strerror(CAENComm_ErrorCode code) {
  switch (code) {
    case CAENComm_Success:
      return "success";
    case CAENComm_VMEBusError:
      return "VME bus error";
    case CAENComm_CommError:
      return "communication error";
    case CAENComm_GenericError:
      return "generic error";
    case CAENComm_InvalidParam:
      return "invalid parameters";
    case CAENComm_InvalidLinkType:
      return "invalid link type";
    case CAENComm_InvalidHandler:
      return "invalid device handler";
    case CAENComm_CommTimeout:
      return "communication timeout";
    case CAENComm_DeviceNotFound:
      return "unable to open device";
    case CAENComm_MaxDevicesError:
      return "max. number of devices exceeded";
    case CAENComm_DeviceAlreadyOpen:
      return "device already open";
    case CAENComm_NotSupported:
      return "request not supported";
    case CAENComm_UnusedBridge:
      return "no boards are controlled by the bridge";
    case CAENComm_Terminated:
      return "communication terminated by the device";
    case CAENComm_UnsupportedBaseAddress:
      return "unsupported base address";
    default:
      return "unknown error";
  };
};

Device::Error::~Error() throw() {
  if (message) delete[] message;
}

// CAENComm_DecodeError does not recognize errors CommTimeout, DeviceNotFound,
// UnusedBridge, Terminated, UnsupportedBaseAddress
#if 0
const char* Device::Error::what() const throw() {
  if (!message) {
    // CAEN documentation does not specify the maximum length of error string
    char buf[256];
    memset(buf, 1, sizeof(buf));
    CAENComm_DecodeError(-3, buf);
    buf[255] = 0;

    size_t size = strlen(buf) + 1;
    if (strcmp(buf, "Unknown Error Code") == 0)
      size += sprintf(buf + size - 1, " (%d)", code_);

    message = new char[size];
    memcpy(message, buf, size);
  };

  return message;
}
#else
const char* Device::Error::what() const throw() {
  static const char* const unknown = comm_strerror(static_cast<CAENComm_ErrorCode>(1));
  if (message) return message;
  const char* msg = comm_strerror(code_);
  if (msg != unknown) return msg;

  char buf[50];
  int size = snprintf(buf, sizeof(buf), "%s (%d)", msg, code_);
  message = new char[size + 1];
  memcpy(message, buf, size + 1);
  return message;
};
#endif

const char* Device::WrongDevice::what() const noexcept {
  if (message.empty()) {
    try {
      std::stringstream ss;
      ss
        << "Device connected through "
        << connection_
        << " is not a "
        << expected_;
      message = ss.str();
    } catch (...) {
      return "caen::Device::WrongDevice::what: error while composing message";
    };
  };
  return message.c_str();
};

#define COMM(function, ...) \
  do { \
    CAENComm_ErrorCode status = CAENComm_ ## function(__VA_ARGS__); \
    if (status != CAENComm_Success) \
      throw Error(status); \
  } while (false)

static CAENComm_ConnectionType commConnectionType(
    const Connection& connection
) {
  if (!connection.ip.empty() && connection.link != 0)
    throw InvalidConnection(connection);

  if (connection.bridge == Connection::Bridge::V4718) {
    if (connection.ip.empty())
      return CAENComm_USB_V4718;
    return CAENComm_ETH_V4718;
  };

  switch (connection.conet) {
    case Connection::Conet::None:
      return CAENComm_USB;
    case Connection::Conet::Optical:
      return CAENComm_OpticalLink;
    case Connection::Conet::A4818:
      return CAENComm_USB_A4818;
  };

  throw InvalidConnection(connection);
};

Device::Device(const Connection& connection) {
  const void* arg;
  if (connection.ip.empty())
    arg = &connection.link;
  else
    arg = connection.ip.c_str();
  COMM(
      OpenDevice2,
      commConnectionType(connection),
      arg,
      connection.node,
      static_cast<uint32_t>(connection.address) << 16,
      &handle
  );
  own = true;

  try {
    if (!check()) throw WrongDevice(connection, kind());
  } catch (...) {
    CAENComm_CloseDevice(handle);
    throw;
  };
};

Device::Device(
    CAENComm_ConnectionType link,
    const void*             arg,
    int                     node,
    uint32_t                address
): own(true) {
  COMM(OpenDevice2, link, arg, node, address, &handle);
};

Device::Device(int handle, bool own): handle(handle), own(own) {};

Device& Device::operator=(Device&& device) {
  if (own) CAENComm_CloseDevice(handle);
  handle = device.handle;
  own    = device.own;
  device.own = false;
  return *this;
};

Device::~Device() {
  if (own) CAENComm_CloseDevice(handle);
}

int Device::vme_handle() const {
  int result;
  COMM(Info, handle, CAENComm_VMELIB_handle, &result);
  return result;
};

void Device::write32(uint32_t address, uint32_t data) {
  COMM(Write32, handle, address, data);
}

void Device::write16(uint32_t address, uint16_t data) {
  COMM(Write16, handle, address, data);
}

uint32_t Device::read32(uint32_t address) const {
  uint32_t result;
  COMM(Read32, handle, address, &result);
  return result;
}

uint16_t Device::read16(uint32_t address) const {
  uint16_t result;
  COMM(Read16, handle, address, &result);
  return result;
}

template <> uint16_t Device::read<16>(uint32_t address) const {
  return read16(address);
};

template <> uint32_t Device::read<32>(uint32_t address) const {
  return read32(address);
};

template <> void Device::write<16>(uint32_t address, uint16_t data) {
  write16(address, data);
};

template <> void Device::write<32>(uint32_t address, uint32_t data) {
  write32(address, data);
};

uint32_t Device::blt_read(
    uint32_t address, uint32_t* buffer, unsigned size
) const {
  int nwords;
  CAENComm_ErrorCode status = CAENComm_BLTRead(
      handle, address, buffer, size, &nwords
  );
  if (status != CAENComm_Success && status != CAENComm_Terminated)
    throw Error(status);
  return nwords;
};

uint32_t Device::mblt_read(
    uint32_t address, uint32_t* buffer, unsigned size
) const {
  int nwords;
  CAENComm_ErrorCode status = CAENComm_MBLTRead(
      handle, address, buffer, size, &nwords
  );
  if (status != CAENComm_Success && status != CAENComm_Terminated)
    throw Error(status);
  return nwords;
};

uint32_t Device::read(uint32_t address, unsigned nwords, uint32_t step) const {
  uint32_t result = 0;
  while (nwords--) {
    result = result << 8 | read16(address) & 0xFF;
    address += step;
  };
  return result;
};

} // namespace caen
