#pragma once

#include <stdexcept>

#include <CAENComm.h>

namespace caen {

class Error: public std::exception {};

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

    uint16_t read16(uint32_t address) const;
    uint32_t read32(uint32_t address) const;
    void write16(uint32_t address, uint16_t data);
    void write32(uint32_t address, uint32_t data);

  protected:
    int handle;
};

} // namespace caen
