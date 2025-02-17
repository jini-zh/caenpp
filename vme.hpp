#pragma once

#include <string>

#include <CAENVMElib.h>

#include "caen.hpp"

namespace caen {

class Bridge {
  public:
    // The purpose of Connection is to provide means for general handling of
    // connections to bridges. If you want to hardcode connection to a specific
    // kind of devices, use a specialized constructor below or a derived class
    struct Connection {
      enum class BridgeType {
        V1718,
        V2718,
        V3718,
        V4718,
        A2719,
        None
      };

      enum class ConetType {
        A2818,
        A3818,
        A4818,
        A5818,
        None
      };

      BridgeType  bridge;
      ConetType   conet;
      uint32_t    link;
      std::string ip;
      short       node;
      bool        local;

      const char* bridgeName() const { return bridgeTypeName(bridge); };
      const char* conetName()  const { return conetTypeName(conet);   };

      static const char* bridgeTypeName(BridgeType);
      static const char* conetTypeName(ConetType);
    };

    class InvalidConnection: public caen::Error {
      public:
        InvalidConnection(Connection connection): connection_(connection) {};

        const char* what() const throw();
        Connection connection() { return connection_; };

      private:
        Connection connection_;
        mutable std::string message;
    };

    // VME errors (with the error code)
    class Error: public caen::Error {
      public:
        Error(CVErrorCodes code): code_(code) {};

        const char* what() const throw();
        CVErrorCodes code() const { return code_; };

      private:
        CVErrorCodes code_;
    };

    struct PulserConf {
      uint8_t     period;
      uint8_t     width;
      CVTimeUnits unit;
      uint8_t     number;
      CVIOSources start;
      CVIOSources reset;
    };

    struct ScalerConf {
      short       limit;
      short       autoReset;
      CVIOSources hit;
      CVIOSources gate;
      CVIOSources reset;
    };

    struct OutputConf {
      CVIOPolarity  polarity;
      CVLEDPolarity led_polarity;
      CVIOSources   source;
    };

    struct InputConf {
      CVIOPolarity  polarity;
      CVLEDPolarity led_polarity;
    };

    Bridge(const Connection&);

    // Convenience constructors
    Bridge(
        Connection::BridgeType bridge,
        Connection::ConetType  conet = Connection::ConetType::None,
        uint32_t               link  = 0,
        short                  node  = 0,
        bool                   local = false
    );

    Bridge(
        Connection::BridgeType bridge,
        Connection::ConetType  conet,
        const char*            ip,
        short                  node = 0,
        bool                   local = false
    );

    Bridge(
        Connection::BridgeType bridge,
        uint32_t               link,
        short                  node = 0,
        bool                   local = false
    );

    Bridge(
        Connection::BridgeType bridge,
        const char*            ip,
        short                  node = 0,
        bool                   local = false
    );

    // thin wrapper over CAENVME_Init2
    Bridge(CVBoardTypes, const void* arg, short conet); 

    // in case you already have a handle
    Bridge(int32_t handle, bool own): handle(handle), own(own) {};

    Bridge(Bridge&& bridge): handle(bridge.handle), own(bridge.own) {
      bridge.own = false;
    };

    virtual ~Bridge();

    Bridge& operator=(Bridge&& bridge) {
      handle = bridge.handle;
      own = bridge.own;
      bridge.own = false;
      return *this;
    };

    int32_t vme_handle() const { return handle; };

    std::string firmwareRelease() const;
    static std::string softwareRelease();
    std::string driverRelease() const;

    void deviceReset();

    unsigned readRegister(uint8_t address) const;
    void writeRegister(uint8_t address, unsigned value);

    void readCycle(
        uint32_t          address,
        CVAddressModifier modifier,
        CVDataWidth       width,
        void*             data
    ) const;

    void writeCycle(
        uint32_t          address,
        CVAddressModifier modifier,
        CVDataWidth       width,
        void*             data
    );

    void RMWCycle(
        uint32_t          address,
        CVAddressModifier modifier,
        CVDataWidth       width,
        void*             data
    );

    void multiRead(
        int                ncycles,
        uint32_t*          addresses,
        CVAddressModifier* modifiers,
        CVDataWidth*       widths,
        uint32_t*          buffer,
        CVErrorCodes*      codes
    ) const;

    void multiWrite(
        int                ncycles,
        uint32_t*          addresses,
        CVAddressModifier* modifiers,
        CVDataWidth*       widths,
        uint32_t*          buffer,
        CVErrorCodes*      codes
    );

    int BLTReadCycle(
        uint32_t          address,
        CVAddressModifier modifier,
        CVDataWidth       width,
        void*             buffer,
        int               size
    ) const;

    int BLTWriteCycle(
        uint32_t          address,
        CVAddressModifier modifier,
        CVDataWidth       width,
        void*             buffer,
        int               size
    );

    int MBLTReadCycle(
        uint32_t          address,
        CVAddressModifier modifier,
        void*             buffer,
        int               size
    ) const;

    int MBLTWriteCycle(
        uint32_t          address,
        CVAddressModifier modifier,
        void*             buffer,
        int               size
    );

    int FIFOBLTReadCycle(
        uint32_t          address,
        CVAddressModifier modifier,
        CVDataWidth       width,
        void*             buffer,
        int               size
    ) const;

    int FIFOBLTWriteCycle(
        uint32_t          address,
        CVAddressModifier modifier,
        CVDataWidth       width,
        void*             buffer,
        int               size
    );

    int FIFOMBLTReadCycle(
        uint32_t          address,
        CVAddressModifier modifier,
        void*             buffer,
        int               size
    ) const;

    int FIFOMBLTWriteCycle(
        uint32_t          address,
        CVAddressModifier modifier,
        void*             buffer,
        int               size
    );

    void ADOCycle(uint32_t address, CVAddressModifier modifier);
    void ADOHCycle(uint32_t address, CVAddressModifier modifier);

    CVArbiterTypes arbiterType() const;
    void setArbiterType(CVArbiterTypes);

    CVRequesterTypes requesterType() const;
    void setRequesterType(CVRequesterTypes);

    CVReleaseTypes releaseType() const;
    void setReleaseType(CVReleaseTypes);

    CVBusReqLevels busReqLevel() const;
    void setBusReqLevel(CVBusReqLevels);

    CVVMETimeouts timeout() const;
    void setTimeout(CVVMETimeouts);

    bool FIFOMode() const;
    void setFIFOMode(bool enable);

    void readDisplay(CVDisplay* display) const;

    void setLocationMonitor(
        uint32_t          address,
        CVAddressModifier modifier,
        short             write,
        short             lword,
        short             iack
    );

    void reset();

    void BLTReadAsync(
        uint32_t          address,
        CVAddressModifier modifier,
        CVDataWidth       width,
        void*             buffer,
        int               size
    ) const;

    int BLTReadWait() const;

    void IACKCycle(CVIRQLevels level, void* vector, CVDataWidth);
    uint8_t IRQCheck() const;
    void IRQEnable(uint32_t mask);
    void IRQDisable(uint32_t mask);
    void IRQWait(uint32_t mask, uint32_t timeout) const;

    PulserConf pulserConf(CVPulserSelect pulser) const;
    void getPulserConf(CVPulserSelect pulser, PulserConf&) const;
    void setPulserConf(CVPulserSelect pulser, const PulserConf&);

    void startPulser(CVPulserSelect pulser);
    void stopPulser(CVPulserSelect pulser);

    ScalerConf scalerConf() const;
    void getScalerConf(ScalerConf&) const;
    void setScalerConf(const ScalerConf&);

    void resetScalerCount();

    void enableScalerGate();
    void disableScalerGate();

    CVScalerMode scalerMode() const;
    void setScalerMode(CVScalerMode);

    CVScalerSource scalerInputSource() const;
    void setScalerInputSource(CVScalerSource);

    CVScalerSource scalerGateSource() const;
    void setScalerGateSource(CVScalerSource);

    CVScalerSource scalerStartSource() const;
    void setScalerStartSource(CVScalerSource);

    bool scalerContinuousRun() const;
    void setScalerContinuousRun(bool);

    uint16_t scalerMaxHits() const;
    void setScalerMaxHits(uint16_t);

    uint16_t scalerDWellTime() const;
    void setScalerDWellTime(uint16_t);

    void scalerStop();
    void scalerReset();
    void scalerOpenGate();
    void scalerCloseGate();

    OutputConf outputConf(CVOutputSelect) const;
    void getOutputConf(CVOutputSelect, OutputConf&) const;
    void setOutputConf(CVOutputSelect, const OutputConf&);

    void setOutputRegister(uint16_t mask);
    void clearOutputRegister(uint16_t mask);
    void pulseOutputRegister(uint16_t mask);

    InputConf inputConf(CVInputSelect) const;
    void getInputConf(CVInputSelect, InputConf&) const;
    void setInputConf(CVInputSelect, const InputConf&);

  protected:
    int32_t handle;

  private:
    bool own;

    Bridge(
        Connection::BridgeType bridge,
        Connection::ConetType  conet,
        uint32_t               link,
        const char*            ip,
        short                  node,
        bool                   local
    );
};

};
