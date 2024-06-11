#pragma once

#include <stdexcept>

#include <CAENComm.h>

namespace caen {

class Error: public std::exception {};

class Device {
  public:
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
        WrongDevice(
            std::string expected,
            CAENComm_ConnectionType link_type,
            uint32_t arg,
            uint32_t conet = 0,
            uint32_t vme   = 0
        ): expected_(std::move(expected)),
           link_type_(link_type),
           arg_(arg),
           conet_(conet),
           vme_(vme)
        {};

        WrongDevice(
            std::string expected,
            CAENComm_ConnectionType link_type,
            std::string ip
        ): expected_(std::move(expected)),
           link_type_(link_type),
           ip_(std::move(ip))
        {};

        const char* what() const noexcept;

        const std::string&      expected()  const { return expected_;  };
        CAENComm_ConnectionType link_type() const { return link_type_; };
        uint32_t                arg()       const { return arg_;       };
        uint32_t                conet()     const { return conet_;     };
        uint32_t                vme()       const { return vme_;       };
        const std::string&      ip()        const { return ip_;        };

      private:
        std::string             expected_;
        CAENComm_ConnectionType link_type_;
        uint32_t                arg_;
        uint32_t                conet_;
        uint32_t                vme_;
        std::string             ip_;
        mutable std::string     message;
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
