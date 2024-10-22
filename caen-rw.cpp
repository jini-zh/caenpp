#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <cstdlib>
#include <cctype>
#include <cstring>

#include <getopt.h>

#include "caen.hpp"

static void usage(const char* argv0) {
  std::cout
    << "This program reads and writes registers of a CAEN VME module\n"
       "Usage: " << argv0 << " [options] < registers\n"
       "Allowed options:\n"
       "  --link  or -l <string>:      CAEN connection link type. Pass `list' to see supported links\n"
       "  --arg   or -a <arg>:         CAEN connection argument. USB device number for USB links, A4818 PID for A4818 links, IP address for Ethernet links\n"
       "  --conet or -c <integer>:     CAEN Conet connection daisy chain number\n"
       "  --vme   or -v <hexadecimal>: 16 most significant bits of the VME address (the value set by the rotary switches on the board)\n"
       "  --access-mode or -d <16|32>: default registers bits size. 16 bits if not specified\n"
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

static CAENComm_ConnectionType str_to_link(const char* string) {
  auto result = caen::str_to_link(string);
  if (result != -1) return result;
  fail("invalid connection type: ", string);
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
  for (int n = 0; ; ++n) {
    int digit;
    char c = line[pos];
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
    if (++pos >= line.size()) break;
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

int main(int argc, char** argv) {
  try {
    const char* link        = nullptr;
    const char* arg         = nullptr;
    const char* conet       = nullptr;
    const char* vme         = nullptr;
    const char* access_mode = nullptr;
    bool        read        = false;
    bool        write       = false;
    while (true) {
      static option options[] = {
        { "arg",         required_argument, nullptr, 'a' },
        { "conet",       required_argument, nullptr, 'c' },
        { "help",        no_argument,       nullptr, 'h' },
        { "link",        required_argument, nullptr, 'l' },
        { "access-mode", required_argument, nullptr, 'd' },
        { "vme",         required_argument, nullptr, 'v' },
      };

      int c = getopt_long(argc, argv, "a:c:d:hl:v:", options, nullptr);
      if (c == -1) break;

      switch (c) {
        case 'a':
          arg = optarg;
          break;
        case 'c':
          conet = optarg;
          break;
        case 'd':
          access_mode = optarg;
          break;
        case 'h':
          usage(argv[0]);
          return 0;
        case 'l':
          link = optarg;
          break;
        case 'v':
          vme = optarg;
          break;
        default:
          return 1;
      };
    };

    if (link && std::strcmp(link, "list") == 0) {
      for (auto& type : caen::Device::Connection::types)
        std::cout << type.name << '\n';
      return 0;
    };

    if (!link)  link  = getenv("CAENPP_LINK");
    if (!arg)   arg   = getenv("CAENPP_ARG");
    if (!conet) conet = getenv("CAENPP_CONET");
    if (!vme)   vme   = getenv("CAENPP_VME");
    if (!access_mode) access_mode = getenv("CAENPP_ACCESS_MODE");

    caen::Device::Connection connection;
    connection.link = link ? str_to_link(link) : CAENComm_USB;
    if (connection.is_ethernet())
      connection.ip = arg;
    else
      connection.arg = arg ? str_to_uint32(arg) : 0;
    connection.conet = conet ? str_to_uint32(conet)   : 0;
    if (vme) {
      connection.vme = str_to_uint16(vme, 16);
      connection.vme <<= 16;
    } else
      connection.vme = 0;

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
          return 1;
      };

    caen::Device device(connection);
    std::string line;
    while (true) {
      std::getline(std::cin, line);
      if (!std::cin) return 0;
      try {
        size_t pos = 0;
        skip_whitespace(line, pos);
        if (pos >= line.size()) continue;

        bool w;
        switch (line[pos]) {
          case 'a':
            w = false;
            skip_whitespace(line, ++pos);
            break;
          case 'A':
            w = true;
            skip_whitespace(line, ++pos);
            break;
          default:
            w = wide;
        };

        uint16_t address = parse_address(line, pos);
        skip_whitespace(line, pos);
        if (pos < line.size()) {
          size_t value_pos = pos;
          uint32_t value = parse_value(line, pos);
          if (w)
            device.write32(address, value);
          else if (value < std::numeric_limits<uint16_t>().max())
            device.write16(address, value);
          else
            fail(line, ": value is too large for 16 bits");
        } else if (w)
          printf(
              "%1$04x %2$08x %2$010u %2$032b\n",
              address,
              device.read32(address)
          );
        else
          printf(
              "%1$04x %2$04x %2$05u %2$016b\n",
              address,
              device.read16(address)
          );
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
