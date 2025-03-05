#pragma once

#include <stdexcept>

#include <cstdint>

namespace caen {

class Error: public std::exception {};

// The purpose of Connection is to provide means for general handling of
// connections to CAEN devices. If you want to hardcode connection to a
// specific kind of devices, use a specialized constructor in a derivative of
// caen::Bridge or caen::Device
//
// Note that there are two CAEN libraries, CAENVME and CAENComm, used to
// communicate to devices. CAENVME is used to communicate to bridges, CAENComm
// --- to modules. Connection is designed to accomodate both.
struct Connection {
  enum class Bridge {
    None,
    V1718,
    V2718,
    V3718,
    V4718,
    A2719,
    Invalid
  };

  enum class Conet {
    None,
    Optical, // Special value to accomodate CAENComm_OpticalLink
    A2818,
    A3818,
    A4818,
#if CAENVME_VERSION_NUMBER >= 40000
    A5818,
#endif
    Invalid
  };

  Bridge      bridge  = Bridge::None;
  Conet       conet   = Conet::None;
  uint32_t    link    = 0;     // device usb number or PID of the conet adapter
  std::string ip;              // when connecting through ethernet
  short       node    = 0;     // Conet daisy chain node number
  bool        local   = false; // when connecting to internal registers of a bridge
  uint16_t    address = 0;     // most significant bits of the VME address

  static const char* bridgeName(Bridge);
  static const char* conetName(Conet);

  static Bridge strToBridge(const char*);
  static Conet  strToConet(const char*);

  const char* bridgeName() const { return bridgeName(bridge); };
  const char* conetName()  const { return conetName(conet); };
};

class InvalidConnection: public Error {
  public:
    InvalidConnection(Connection connection): connection_(connection) {};

    const char* what() const throw();
    Connection connection() { return connection_; };

  private:
    Connection connection_;
    mutable std::string message;
};

std::ostream& operator<<(std::ostream&, const Connection&);

// Database of known devices and their properties
// (experimental, to be extended with more properties when needed)
// Last entry's name is nullptr
struct DeviceDB {
  const char* name;
  bool is_bridge;
};

extern DeviceDB deviceDB[];

};
