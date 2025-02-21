#include <utility>
#include <sstream>

#include <cstring>

#include "caen.hpp"

namespace caen {

DeviceDB deviceDB[] = {
  { "V792",  false },
  { "V812",  false },
  { "V1290", false },
  { "V1495", false },
  { "V3718", true  },
  { "V6534", false },
  { nullptr, false }
};

static const char* bridge_names[] = {
  "None", "V1718", "V2718", "V3718", "V4718", "A2719"
};

static const char* conet_names[] = {
  "None", "Optical", "A2818", "A3818", "A4818", "A5818"
};

#define defenum(type, Type) \
  const char* Connection::type ## Name(Type type) { \
    int index = static_cast<int>(type); \
    auto count = sizeof(type ## _names) / sizeof(*type ## _names); \
    if (index < 0 || index > count) return "invalid"; \
    return type ## _names[index]; \
  }; \
  Connection::Type Connection::strTo ## Type(const char* name) { \
    auto count = sizeof(type ## _names) / sizeof(*type ## _names); \
    for (int i = 0; i < count; ++i) \
      if (strcasecmp(type ## _names[i], name) == 0) \
        return static_cast<Connection::Type>(i); \
    return Connection::Type::Invalid; \
  }; \
  static bool valid ## Type(Connection::Type type) { \
    return type > Connection::Type::None && type < Connection::Type::Invalid; \
  }

defenum(bridge, Bridge);
defenum(conet, Conet);

#undef defenum

const char* InvalidConnection::what() const throw() {
  if (message.empty()) {
    try {
      std::stringstream ss;
      ss << "caen: invalid connection: " << connection_;
      message = ss.str();
    } catch (...) {
      return "caen::InvalidConnection::what: error while printing connection parameters";
    };
  };
  return message.c_str();
};

std::ostream& operator<<(std::ostream& stream, const Connection& connection) {
  bool first = true;

  if (validBridge(connection.bridge)) {
    stream << connection.bridgeName();
    first = false;
  };

  if (validConet(connection.conet)) {
    if (!first) stream << " via ";
    first = false;
    stream << connection.conetName();
  };

  if (connection.link) {
    if (!validConet(connection.conet)) {
      if (!first) stream << ", ";
      first = false;
      stream << "USB device ";
    };
    stream << connection.link;
  };

  if (!connection.ip.empty()) {
    if (!first) stream << ", ";
    first = false;
    stream << "IP " << connection.ip;
  };

  if (connection.node) {
    if (!first) stream << ", ";
    first = false;
    stream << "daisy chain node " << connection.node;
  };

  if (connection.local) {
    if (!first) stream << ", ";
    first = false;
    stream << "local";
  };

  if (connection.address) {
    if (!first) stream << ", ";
    first = false;
    stream << "VME address 0x" << std::hex << connection.address << std::dec;
  };

  if (first) stream << "<unspecified>";

  return stream;
};

};
