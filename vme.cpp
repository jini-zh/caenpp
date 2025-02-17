#include <sstream>

#include "vme.hpp"

namespace caen {

const char* Bridge::Connection::bridgeTypeName(BridgeType type) {
  switch (type) {
    case BridgeType::V1718:
      return "V1718";
    case BridgeType::V2718:
      return "V2718";
    case BridgeType::V3718:
      return "V3718";
    case BridgeType::A2719:
      return "A2719";
    case BridgeType::None:
      return "None";
    default:
      return "invalid";
  };
};

const char* Bridge::Connection::conetTypeName(ConetType type) {
  switch (type) {
    case ConetType::A2818:
      return "A2818";
    case ConetType::A3818:
      return "A3818";
    case ConetType::A4818:
      return "A4818";
    case ConetType::A5818:
      return "A5818";
    case ConetType::None:
      return "None";
    default:
      return "invalid";
  };
};

const char* Bridge::InvalidConnection::what() const throw() {
  if (message.empty()) {
    try {
      std::stringstream ss;
      ss
        << "caen::Bridge: invalid connection parameters: "
        << connection_.bridgeName()
        << " via "
        << connection_.conetName()
        << ", local = " << (connection_.local ? "true" : "false");
      if (connection_.ip.empty())
        ss << ", link = " << connection_.link;
      else
        ss << ", ip = " << connection_.ip;
      ss << ", node = " << connection_.node;
      message = ss.str();
    } catch (std::exception&) {
      return "Bridge::InvalidConnection::what: error while printing connection parameters";
    };
  };
  return message.c_str();
};

const char* Bridge::Error::what() const throw() {
  return CAENVME_DecodeError(code_);
};

#define VME(function, ...) \
  do { \
    CVErrorCodes status__ = CAENVME_ ## function(__VA_ARGS__); \
    if (status__ != cvSuccess) \
      throw Bridge::Error(status__); \
  } while (false)

static CVBoardTypes vmeConnectionType(
    Bridge::Connection::BridgeType bridge,
    Bridge::Connection::ConetType  conet,
    bool local,
    bool ethernet
) {
#define bt Bridge::Connection::BridgeType
#define ct Bridge::Connection::ConetType
  switch (bridge) {
    case bt::V1718:
      return cvV1718;
    case bt::V2718:
      switch (conet) {
        case ct::None:
          return cvV2718;
        case ct::A4818:
          return local ? cvUSB_A4818_V2718_LOCAL : cvUSB_A4818_V2718;
      };
      break;
    case bt::V3718:
      switch (conet) {
        case ct::None:
          return local ? cvUSB_V3718_LOCAL : cvUSB_V3718;
        case ct::A2818:
          return local ? cvPCI_A2818_V3718_LOCAL : cvPCI_A2818_V3718;
        case ct::A3818:
          return local ? cvPCIE_A3818_V3718_LOCAL : cvPCIE_A3818_V3718;
        case ct::A4818:
          return local ? cvUSB_A4818_V3718_LOCAL : cvUSB_A4818_V3718;
        case ct::A5818:
          return local ? cvPCIE_A5818_V3718_LOCAL : cvPCIE_A5818_V3718;
      };
      break;
    case bt::V4718:
      switch (conet) {
        case ct::None:
          return ethernet
               ? (local ? cvETH_V4718_LOCAL : cvETH_V4718)
               : (local ? cvUSB_V4718_LOCAL : cvUSB_V4718);
        case ct::A2818:
          return local ? cvPCI_A2818_V4718_LOCAL : cvPCI_A2818_V4718;
        case ct::A3818:
          return local ? cvPCIE_A3818_V4718_LOCAL : cvPCIE_A3818_V4718;
        case ct::A4818:
          return local ? cvUSB_A4818_V4718_LOCAL : cvUSB_A4818_V4718;
        case ct::A5818:
          return local ? cvPCIE_A5818_V4718_LOCAL : cvPCIE_A5818_V4718;
      };
      break;
    case bt::A2719:
      switch (conet) {
        case ct::None:
          return cvA2719;
        case ct::A4818:
          return cvUSB_A4818_A2719_LOCAL;
      };
      break;
    case bt::None:
      switch (conet) {
        case ct::A2818:
          return cvA2818;
        case ct::A3818:
          return cvA3818;
        case ct::A4818:
          return local ? cvUSB_A4818_LOCAL : cvUSB_A4818;
        case ct::A5818:
          return cvA5818;
      };
      break;
  };
  return cvInvalid;
#undef ct
#undef bt
};

Bridge::Bridge(
    Connection::BridgeType bridge,
    Connection::ConetType  conet,
    uint32_t               link,
    const char*            ip,
    short                  node,
    bool                   local
): own(true) {
  CVBoardTypes type = vmeConnectionType(bridge, conet, local, ip);
  if (type == cvInvalid)
    throw InvalidConnection(
        Connection {
          .bridge = bridge,
          .conet  = conet,
          .link   = link,
          .ip     = ip,
          .node   = node,
          .local  = local
        }
    );

  const void* arg;
  if (ip)
    arg = &link;
  else
    arg = ip;

  VME(Init2, type, arg, node, &handle);
};

Bridge::Bridge(const Connection& connection):
  Bridge(
      connection.bridge,
      connection.conet,
      connection.link,
      connection.ip.c_str(),
      connection.node,
      connection.local
  )
{};

Bridge::Bridge(
    Connection::BridgeType bridge,
    Connection::ConetType  conet,
    uint32_t               link,
    short                  node,
    bool                   local
): Bridge(bridge, conet, link, nullptr, node, local)
{};

Bridge::Bridge(
    Connection::BridgeType bridge,
    Connection::ConetType  conet,
    const char*            ip,
    short                  node,
    bool                   local
): Bridge(bridge, conet, 0, ip, node, local)
{};

Bridge::Bridge(
    Connection::BridgeType bridge,
    uint32_t               link,
    short                  node,
    bool                   local
): Bridge(bridge, Connection::ConetType::None, link, nullptr, node, local)
{};

Bridge::Bridge(
    Connection::BridgeType bridge,
    const char*            ip,
    short                  node,
    bool                   local
): Bridge(bridge, Connection::ConetType::None, 0, ip, node, local)
{};

Bridge::Bridge(CVBoardTypes type, const void* arg, short conet): own(true) {
  VME(Init2, type, arg, conet, &handle);
};

Bridge::~Bridge() {
  if (own) CAENVME_End(handle);
};

std::string Bridge::firmwareRelease() const {
  char s[16];
  VME(BoardFWRelease, handle, s);
  return s;
};

std::string Bridge::softwareRelease() {
  char s[16];
  VME(SWRelease, s);
  return s;
};

std::string Bridge::driverRelease() const {
  char s[16];
  VME(DriverRelease, handle, s);
  return s;
};

void Bridge::deviceReset() {
  VME(DeviceReset, handle);
};

unsigned Bridge::readRegister(uint8_t address) const {
  unsigned result;
  VME(ReadRegister, handle, static_cast<CVRegisters>(address), &result);
  return result;
};

void Bridge::writeRegister(uint8_t address, unsigned value) {
  VME(WriteRegister, handle, static_cast<CVRegisters>(address), value);
};

void Bridge::readCycle(
    uint32_t          address,
    CVAddressModifier modifier,
    CVDataWidth       width,
    void*             data
) const {
  VME(ReadCycle, handle, address, data, modifier, width);
};

void Bridge::writeCycle(
    uint32_t          address,
    CVAddressModifier modifier,
    CVDataWidth       width,
    void*             data
) {
  VME(WriteCycle, handle, address, data, modifier, width);
};

void Bridge::RMWCycle(
    uint32_t address, CVAddressModifier modifier, CVDataWidth width, void* data
) {
  VME(RMWCycle, handle, address, data, modifier, width);
};

void Bridge::multiRead(
    int                ncycles,
    uint32_t*          addresses,
    CVAddressModifier* modifiers,
    CVDataWidth*       widths,
    uint32_t*          buffer,
    CVErrorCodes*      codes
) const {
  VME(MultiRead, handle, addresses, buffer, ncycles, modifiers, widths, codes);
};

void Bridge::multiWrite(
    int                ncycles,
    uint32_t*          addresses,
    CVAddressModifier* modifiers,
    CVDataWidth*       widths,
    uint32_t*          buffer,
    CVErrorCodes*      codes
) {
  VME(MultiWrite, handle, addresses, buffer, ncycles, modifiers, widths, codes);
};

int Bridge::BLTReadCycle(
    uint32_t          address,
    CVAddressModifier modifier,
    CVDataWidth       width,
    void*             buffer,
    int               size
) const {
  int count;
  VME(BLTReadCycle, handle, address, buffer, size, modifier, width, &count);
  return count;
};

int Bridge::BLTWriteCycle(
    uint32_t          address,
    CVAddressModifier modifier,
    CVDataWidth       width,
    void*             buffer,
    int               size
) {
  int count;
  VME(BLTWriteCycle, handle, address, buffer, size, modifier, width, &count);
  return count;
};

int Bridge::MBLTReadCycle(
    uint32_t          address,
    CVAddressModifier modifier,
    void*             buffer,
    int               size
) const {
  int count;
  VME(MBLTReadCycle, handle, address, buffer, size, modifier, &count);
  return count;
};

int Bridge::MBLTWriteCycle(
    uint32_t          address,
    CVAddressModifier modifier,
    void*             buffer,
    int               size
) {
  int count;
  VME(MBLTWriteCycle, handle, address, buffer, size, modifier, &count);
  return count;
};

int Bridge::FIFOBLTReadCycle(
    uint32_t          address,
    CVAddressModifier modifier,
    CVDataWidth       width,
    void*             buffer,
    int               size
) const {
  int count;
  VME(FIFOBLTReadCycle, handle, address, buffer, size, modifier, width, &count);
  return count;
};

int Bridge::FIFOBLTWriteCycle(
    uint32_t          address,
    CVAddressModifier modifier,
    CVDataWidth       width,
    void*             buffer,
    int               size
) {
  int count;
  VME(FIFOBLTWriteCycle, handle, address, buffer, size, modifier, width, &count);
  return count;
};

int Bridge::FIFOMBLTReadCycle(
    uint32_t          address,
    CVAddressModifier modifier,
    void*             buffer,
    int               size
) const {
  int count;
  VME(FIFOMBLTReadCycle, handle, address, buffer, size, modifier, &count);
  return count;
};

int Bridge::FIFOMBLTWriteCycle(
    uint32_t          address,
    CVAddressModifier modifier,
    void*             buffer,
    int               size
) {
  int count;
  VME(FIFOMBLTWriteCycle, handle, address, buffer, size, modifier, &count);
  return count;
};

void Bridge::ADOCycle(uint32_t address, CVAddressModifier modifier) {
  VME(ADOCycle, handle, address, modifier);
};

void Bridge::ADOHCycle(uint32_t address, CVAddressModifier modifier) {
  VME(ADOHCycle, handle, address, modifier);
};

#define defparameter_ex(type, getter, setter, vme_getter, vme_setter) \
  type Bridge::getter() const { \
    type value; \
    VME(vme_getter, handle, &value); \
    return value; \
  }; \
  void Bridge::setter(type value) { \
    VME(vme_setter, handle, value); \
  }

#define defparameter(type, name, Name) \
  defparameter_ex(type, name, set ## Name, Get ## Name, Set ## Name)

defparameter(CVArbiterTypes,   arbiterType,   ArbiterType);
defparameter(CVRequesterTypes, requesterType, RequesterType);
defparameter(CVReleaseTypes,   releaseType,   ReleaseType);
defparameter(CVBusReqLevels,   busReqLevel,   BusReqLevel);
defparameter(CVVMETimeouts,    timeout,       Timeout);

bool Bridge::FIFOMode() const {
  short enabled;
  VME(GetFIFOMode, handle, &enabled);
  return enabled;
};

void Bridge::setFIFOMode(bool enabled) {
  VME(SetFIFOMode, handle, enabled);
};

void Bridge::readDisplay(CVDisplay* display) const {
  VME(ReadDisplay, handle, display);
};

void Bridge::setLocationMonitor(
    uint32_t          address,
    CVAddressModifier modifier,
    short             write,
    short             lword,
    short             iack
) {
  VME(SetLocationMonitor, handle, address, modifier, write, lword, iack);
};

void Bridge::reset() {
  VME(SystemReset, handle);
};

void Bridge::BLTReadAsync(
    uint32_t          address,
    CVAddressModifier modifier,
    CVDataWidth       width,
    void*             buffer,
    int               size
) const {
  VME(BLTReadAsync, handle, address, buffer, size, modifier, width);
};

int Bridge::BLTReadWait() const {
  int count;
  VME(BLTReadWait, handle, &count);
  return count;
};

void Bridge::IACKCycle(CVIRQLevels level, void* vector, CVDataWidth width) {
  VME(IACKCycle, handle, level, vector, width);
};

uint8_t Bridge::IRQCheck() const {
  uint8_t mask;
  VME(IRQCheck, handle, &mask);
  return mask;
};

void Bridge::IRQEnable(uint32_t mask) {
  VME(IRQEnable, handle, mask);
};

void Bridge::IRQDisable(uint32_t mask) {
  VME(IRQDisable, handle, mask);
};

void Bridge::IRQWait(uint32_t mask, uint32_t timeout) const {
  VME(IRQWait, handle, mask, timeout);
};

Bridge::PulserConf Bridge::pulserConf(CVPulserSelect pulser) const {
  PulserConf conf;
  getPulserConf(pulser, conf);
  return conf;
};

void Bridge::getPulserConf(CVPulserSelect pulser, PulserConf& conf) const {
  VME(
      GetPulserConf,
      handle,
      pulser,
      &conf.period,
      &conf.width,
      &conf.unit,
      &conf.number,
      &conf.start,
      &conf.reset
  );
};

void Bridge::setPulserConf(CVPulserSelect pulser, const PulserConf& conf) {
  VME(
      SetPulserConf,
      handle,
      pulser,
      conf.period,
      conf.width,
      conf.unit,
      conf.number,
      conf.start,
      conf.reset
  );
};

void Bridge::startPulser(CVPulserSelect pulser) {
  VME(StartPulser, handle, pulser);
};

void Bridge::stopPulser(CVPulserSelect pulser) {
  VME(StopPulser, handle, pulser);
};

Bridge::ScalerConf Bridge::scalerConf() const {
  ScalerConf conf;
  getScalerConf(conf);
  return conf;
};

void Bridge::getScalerConf(ScalerConf& conf) const {
  VME(
      GetScalerConf,
      handle,
      &conf.limit,
      &conf.autoReset,
      &conf.hit,
      &conf.gate,
      &conf.reset
  );
};

void Bridge::setScalerConf(const ScalerConf& conf) {
  VME(
      SetScalerConf,
      handle,
      conf.limit,
      conf.autoReset,
      conf.hit,
      conf.gate,
      conf.reset
  );
};

void Bridge::resetScalerCount() {
  VME(ResetScalerCount, handle);
};

void Bridge::enableScalerGate() {
  VME(EnableScalerGate, handle);
};

void Bridge::disableScalerGate() {
  VME(DisableScalerGate, handle);
};

#define define_scaler_parameter(type, name) \
  defparameter_ex( \
      type, \
      scaler     ## name, \
      setScaler  ## name, \
      GetScaler_ ## name, \
      SetScaler_ ## name  \
  )

define_scaler_parameter(CVScalerMode,   Mode);
define_scaler_parameter(CVScalerSource, InputSource);
define_scaler_parameter(CVScalerSource, GateSource);
define_scaler_parameter(CVScalerSource, StartSource);

bool Bridge::scalerContinuousRun() const {
  CVContinuosRun value;
  VME(GetScaler_ContinuousRun, handle, &value);
  return value == cvOn;
};

void Bridge::setScalerContinuousRun(bool value) {
  VME(SetScaler_ContinuousRun, handle, value ? cvOn : cvOff);
};

define_scaler_parameter(uint16_t, MaxHits);
define_scaler_parameter(uint16_t, DWellTime);

void Bridge::scalerStop() {
  VME(SetScaler_SWStop, handle);
};

void Bridge::scalerReset() {
  VME(SetScaler_SWReset, handle);
};

void Bridge::scalerOpenGate() {
  VME(SetScaler_SWOpenGate, handle);
};

void Bridge::scalerCloseGate() {
  VME(SetScaler_SWCloseGate, handle);
};

Bridge::OutputConf Bridge::outputConf(CVOutputSelect output) const {
  OutputConf conf;
  getOutputConf(output, conf);
  return conf;
};

void Bridge::getOutputConf(CVOutputSelect output, OutputConf& conf) const {
  VME(
      GetOutputConf,
      handle,
      output,
      &conf.polarity,
      &conf.led_polarity,
      &conf.source
  );
};

void Bridge::setOutputConf(CVOutputSelect output, const OutputConf& conf) {
  VME(
      SetOutputConf,
      handle,
      output,
      conf.polarity,
      conf.led_polarity,
      conf.source
  );
};

void Bridge::setOutputRegister(uint16_t mask) {
  VME(SetOutputRegister, handle, mask);
};

void Bridge::clearOutputRegister(uint16_t mask) {
  VME(ClearOutputRegister, handle, mask);
};

void Bridge::pulseOutputRegister(uint16_t mask) {
  VME(PulseOutputRegister, handle, mask);
};

Bridge::InputConf Bridge::inputConf(CVInputSelect input) const {
  InputConf conf;
  getInputConf(input, conf);
  return conf;
};

void Bridge::getInputConf(CVInputSelect input, InputConf& conf) const {
  VME(GetInputConf, handle, input, &conf.polarity, &conf.led_polarity);
};

void Bridge::setInputConf(CVInputSelect input, const InputConf& conf) {
  VME(SetInputConf, handle, input, conf.polarity, conf.led_polarity);
};

};
