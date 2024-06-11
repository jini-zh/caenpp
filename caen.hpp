#pragma once

#include <stdexcept>

#include <CAENComm.h>

namespace caen {

class Error: public std::exception {};

class Device {
  public:
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

    Device(
        CAENComm_ConnectionType link_type,
        uint32_t arg,
        uint32_t conet = 0,
        uint32_t vme   = 0
    );
    Device(CAENComm_ConnectionType link_type, const char* ip);
    Device(CAENComm_ConnectionType link_type, const std::string& ip);

    Device(Device&& device): handle(device.handle) {
      device.handle = -1;
    };

    virtual ~Device();

    uint16_t read16(uint32_t address) const;
    uint32_t read32(uint32_t address) const;
    void write16(uint32_t address, uint16_t data);
    void write32(uint32_t address, uint32_t data);

  protected:
    int handle;
};

} // namespace caen
