#include <cstring>

#include "caen.hpp"

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

#define COMM(function, ...) \
  do { \
    CAENComm_ErrorCode status = CAENComm_ ## function(__VA_ARGS__); \
    if (status != CAENComm_Success) \
      throw Error(status); \
  } while (false)

Device::Device(
    CAENComm_ConnectionType link_type,
    uint32_t arg,
    uint32_t conet,
    uint32_t vme
) {
  COMM(OpenDevice2, link_type, &arg, conet, vme, &handle);
}

Device::Device(CAENComm_ConnectionType link_type, const char* ip) {
  COMM(OpenDevice2, link_type, ip, 0, 0, &handle);
}

Device::Device(CAENComm_ConnectionType link_type, const std::string& ip) {
  COMM(OpenDevice2, link_type, ip.c_str(), 0, 0, &handle);
}

Device::~Device() {
  if (handle >= 0) CAENComm_CloseDevice(handle);
}

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

} // namespace caen
