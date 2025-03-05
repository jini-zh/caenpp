#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>

#include <cstdlib>
#include <cctype>
#include <cstring>

#include <getopt.h>
#include <unistd.h>

#include "comm.hpp"
#include "vme.hpp"

static void usage(const char* argv0) {
  std::cout
    << "This program reads and writes registers of a CAEN VME module\n"
       "Usage: " << argv0 << " [options] < registers\n"
       "Allowed options:\n"
       "  --address or -a <hexadecimal>: 16 most significant bits of the VME address (the value set by the rotary switches on the board). Can also be set through CAENPP_ADDRESS environment variable.\n"
       "  --bridge or -b <string>:      CAEN bridge name when connecting to a bridge. Pass `list' to see supported bridges. Can also be set through CAENPP_BRIDGE environment variable\n"
       "  --conet or -c <string>:       CAEN Conet adapter name when connecting through an adapter. Pass `list' to see supported conets. Can also be set through CAENPP_CONET environment variable\n"
       "  --help or -h:                 print this message\n"
       "  --ip or -i <string>:          IP address when connecting through Ethernet. Can also be set through CAENPP_IP environment variable\n"
       "  --link or -l <uint32>:        USB device number when connecting through USB or Conet PID when connecting through Conet. Can also be set through CAENPP_LINK environment variable\n"
       "  --local or -L:                connect to bridge local registers (experts only)\n"
       "  --node or -n <uint16_t>:      number of the device in the daisy chain. Can also be set through CAENPP_NODE environment variable\n"
       "  --access-mode or -d <16|32>:  default registers bits size. 16 bits if not specified. Can also be set through CAENPP_ACCESS_MODE environment variable\n"
       "Each input line should have the following syntax:\n"
       "  <access-mode>? <address> <value>?\n"
       "Where\n"
       "  <access-mode> specifies the register bit width and can be 'a' for 16 bits, 'A' for 32 bits or omitted for the default value\n"
       "  <address> is a 16-bit hexadecimal register address\n"
       "  <value> is the value to be written to the register and can be given in decimal (optinally with a '0d' prefix), hexadecimal (with a '0x' prefix), or binary (with a '0b' prefix)\n"
       "If <value> is present, it is written to the register at address <address>. If <value> is not present, the register contents is printed to the standard output in the following format:\n"
       "  <address> <hexadecimal> <decimal> <binary>\n"
       "The space between <access-mode> and <address> is optional\n"
   ;
};

template <typename... Args>
std::string concat(Args... args) {
  std::stringstream ss;
  (ss << ... << args);
  return ss.str();
};

template <typename... Args>
inline void fail(Args... args) __attribute__((noreturn));

template <typename... Args>
inline void fail(Args... args) {
  throw std::runtime_error(concat(args...));
};

static unsigned long str_to_ulong(const char* string, int base) {
  char* end;
  unsigned long result = std::strtoul(string, &end, base);
  if (!*end) return result;
  std::stringstream ss;
  ss << "invalid unsigned integer";
  if (base && base != 10) ss << " in base " << base;
  ss << ": " << string;
  throw std::runtime_error(ss.str());
};

#define def_str_to(uint) \
  static uint ## _t str_to_ ## uint (const char* string, int base = 0) { \
    unsigned long result = str_to_ulong(string, base); \
    if (result <= std::numeric_limits<uint ## _t>().max()) return result; \
    std::stringstream ss; \
    ss << #uint " overflow: " << string; \
    if (base && base != 10) ss << " (in base " << base << ')'; \
    throw std::runtime_error(ss.str()); \
  }
def_str_to(uint8);
def_str_to(uint16);
def_str_to(uint32);
#undef def_str_to

static void skip_whitespace(const std::string& line, size_t& pos) {
  while (pos < line.size() && std::isspace(line[pos])) ++pos;
};

static uint16_t parse_address(const std::string& line, size_t& pos) {
  if (pos + 1 < line.size() && line[pos] == '0' && line[pos+1] == 'x') pos += 2;
  if (pos == line.size()) fail("expected address");
  while (pos < line.size() && line[pos] == '0') ++pos;

  uint16_t address = 0;
  for (int n = 0; pos < line.size(); ++n) {
    int digit;
    char c = line[pos++];
    if (c >= '0' && c <= '9')
      digit = c - '0';
    else if (c >= 'a' && c <= 'f')
      digit = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      digit = c - 'A' + 10;
    else if (std::isspace(c))
      break;
    else
      fail(line, ": invalid address");
    if (n >= 4)
      fail(line, ": address is too large");
    address = address << 4 | digit;
  };

  return address;
};

static uint32_t parse_value(const std::string& line, size_t& pos) {
  size_t value_pos = pos;

  uint32_t base = 0;
  if (pos + 1 < line.size() && line[pos] == '0') {
    switch (line[pos+1]) {
      case 'b':
        base = 2;
        pos += 2;
        break;
      case 'd':
        base = 10;
        pos += 2;
        break;
      case 'x':
        base = 16;
        pos += 2;
        break;
    };
  };
  if (base == 0) base = 10;

  if (pos == line.size())
    fail(line, ": expected value");

  uint32_t result = 0;
  while (pos < line.size()) {
    char c = line[pos++];
    if (isspace(c)) break;
    uint32_t x;
    switch (c) {
      case '0' ... '9':
        x = c - '0';
        break;
      case 'a' ... 'z':
        x = c - 'a';
        break;
      case 'A' ... 'Z':
        x = c - 'A';
        break;
      default:
        goto fail_;
    };
    if (x >= base) goto fail_;
    x += result * base;
    if (x < result) goto fail_;
    result = x;
  };

  return result;

fail_:
  fail(line, ": invalid value");
};

static void list_bridges() {
  for (int bridge = static_cast<int>(caen::Connection::Bridge::None);
       ++bridge != static_cast<int>(caen::Connection::Bridge::Invalid);)
    std::cout
      << caen::Connection::bridgeName(
          static_cast<caen::Connection::Bridge>(bridge)
         )
      << '\n';
};

static void list_conets() {
  for (int conet = static_cast<int>(caen::Connection::Conet::None);
       ++conet != static_cast<int>(caen::Connection::Conet::Invalid);)
    std::cout
      << caen::Connection::conetName(
          static_cast<caen::Connection::Conet>(conet)
         )
      << '\n';
};

static void connect(
    const caen::Connection& connection,
    bool wide,
    std::function<uint32_t (uint16_t)>& read,
    std::function<void (uint16_t, uint32_t)>& write
) {
  if (connection.is_bridge()) {
    auto bridge = std::make_shared<caen::Bridge>(connection);
    read = [bridge](uint16_t address) -> uint32_t {
      if (address > std::numeric_limits<uint8_t>().max())
        fail("address is too big for 8 bits: ", address);
      return bridge->readRegister(address);
    };
    write = [bridge](uint16_t address, uint32_t value) {
      if (address > std::numeric_limits<uint8_t>().max())
        fail("address is too big for 8 bits: ", address);
      bridge->writeRegister(address, value);
    };
  } else {
    auto device = std::make_shared<caen::Device>(connection);
    if (wide) {
      read = [device](uint16_t address) -> uint32_t {
        return device->read32(address);
      };
      write = [device](uint16_t address, uint32_t value) {
        device->write32(address, value);
      };
    } else {
      read = [device](uint16_t address) -> uint32_t {
        return device->read16(address);
      };
      write = [device](uint16_t address, uint32_t value) {
        if (value > std::numeric_limits<uint16_t>().max())
          fail("value is too big for 16 bits: ", value);
        device->write16(address, value);
      };
    };
  };
};

struct Options {
  caen::Connection connection;
  bool wide;
};

static Options parse_options(int argc, char** argv) {
  const char* bridge      = nullptr;
  const char* conet       = nullptr;
  const char* link        = nullptr;
  const char* ip          = nullptr;
  const char* node        = nullptr;
  bool local              = false;
  const char* address     = nullptr;
  const char* access_mode = nullptr;
  while (true) {
    static option options[] = {
      { "address",     required_argument, nullptr, 'a' },
      { "bridge",      required_argument, nullptr, 'b' },
      { "conet",       required_argument, nullptr, 'c' },
      { "help",        no_argument,       nullptr, 'h' },
      { "ip",          required_argument, nullptr, 'i' },
      { "link",        required_argument, nullptr, 'l' },
      { "local",       no_argument,       nullptr, 'L' },
      { "node",        required_argument, nullptr, 'n' },
      { "access-mode", required_argument, nullptr, 'd' },
    };

    int c = getopt_long(argc, argv, "a:b:c:hi:l:Ln:d:", options, nullptr);
    if (c == -1) break;

    switch (c) {
      case 'a':
        address = optarg;
        break;
      case 'b':
        bridge = optarg;
        break;
      case 'c':
        conet = optarg;
        break;
      case 'd':
        access_mode = optarg;
        break;
      case 'h':
        usage(argv[0]);
        exit(0);
      case 'i':
        ip = optarg;
        break;
      case 'l':
        link = optarg;
        break;
      case 'L':
        local = true;
        break;
      case 'n':
        node = optarg;
        break;
      default:
        exit(1);
    };
  };

  if (bridge && strcmp(bridge, "list") == 0) {
    list_bridges();
    exit(0);
  };

  if (conet && strcmp(conet, "list") == 0) {
    list_conets();
    exit(0);
  };

  if (!bridge)      bridge      = getenv("CAENPP_BRIDGE");
  if (!conet)       conet       = getenv("CAENPP_CONET");
  if (!ip)          ip          = getenv("CAENPP_IP");
  if (!link)        link        = getenv("CAENPP_LINK");
  if (!node)        node        = getenv("CAENPP_NODE");
  if (!address)     address     = getenv("CAENPP_ADDRESS");
  if (!access_mode) access_mode = getenv("CAENPP_ACCESS_MODE");

  caen::Connection connection;

  if (bridge) {
    connection.bridge = caen::Connection::strToBridge(bridge);
    if (connection.bridge == caen::Connection::Bridge::Invalid) {
      std::cerr << argv[0] << ": invalid bridge: " << bridge << '\n';
      exit(1);
    };
  };

  if (conet) {
    connection.conet = caen::Connection::strToConet(conet);
    if (connection.conet == caen::Connection::Conet::Invalid) {
      std::cerr << argv[0] << ": invalid conet: " << conet << '\n';
      exit(1);
    };
  };

  if (link)    connection.link    = str_to_uint32(link);
  if (ip)      connection.ip      = ip;
  if (node)    connection.node    = str_to_uint16(node);
  connection.local = local;
  if (address) connection.address = str_to_uint16(address);

  bool wide = false;
  if (access_mode)
    switch (str_to_uint8(access_mode)) {
      case 16:
        break;
      case 32:
        wide = true;
        break;
      default:
        std::cerr
          << argv[0]
          << ": invalid access mode: "
          << access_mode
          << ", expected 16 or 32\n";
        exit(1);
    };

  return { std::move(connection), wide };
};

void print(
    uint16_t address,
    uint32_t value,
    int hex_width,
    int dec_width,
    int bin_width
) {
  std::cout
    << std::setfill('0') << std::hex << std::setw(4) << address
    << ' ' << std::setw(hex_width) << value
    << ' ' << std::setfill(' ') << std::setw(dec_width) << std::dec << value
    << ' ';

  char buf[bin_width + 1];
  buf[bin_width] = 0;
  int n = bin_width;
  for (; value; value >>= 1) buf[--n] = '0' + (value & 1);
  while (n > 0) buf[--n] = '0';
  std::cout << buf << '\n';
};

int main(int argc, char** argv) {
  try {
    auto options = parse_options(argc, argv);

    std::function<uint32_t (uint16_t)>       read;
    std::function<void (uint16_t, uint32_t)> write;
    connect(options.connection, options.wide, read, write);

    bool terminal = isatty(0);

    caen::Device device(options.connection);
    std::string line;
    while (true) {
      if (terminal) std::cout << "> ";
      std::getline(std::cin, line);
      if (!std::cin) {
        if (terminal) std::cout << '\n';
        return 0;
      };
      try {
        size_t pos = 0;
        skip_whitespace(line, pos);
        if (pos >= line.size()) continue;

        bool w = options.wide;
        switch (line[pos]) {
          case 'a':
            w = false;
            skip_whitespace(line, ++pos);
            break;
          case 'A':
            w = true;
            skip_whitespace(line, ++pos);
            break;
        };

        uint16_t address = parse_address(line, pos);
        skip_whitespace(line, pos);
        if (pos < line.size()) {
          size_t value_pos = pos;
          uint32_t value = parse_value(line, pos);
          write(address, value);
        } else {
          uint32_t value = read(address);
          if (w)
            print(address, value, 8, 10, 32);
          else
            print(address, value, 4, 5, 16);
        };
      } catch (std::exception& e) {
        std::cerr << argv[0] << ": " << e.what() << '\n';
      };
    };

    return 0;
  } catch (std::exception& e) {
    std::cerr << argv[0] << ": " << e.what() << '\n';
    return 1;
  };
};
