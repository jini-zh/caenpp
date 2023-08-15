#pragma once

#include "caen.hpp"

#include <CAENDigitizer.h>

namespace caen {

class Digitizer {
  public:
    class Error: public caen::Error {
      public:
        Error(CAEN_DGTZ_ErrorCode code): code_(code) {};

        Error(const char* function, CAEN_DGTZ_ErrorCode code):
          code_(code), function(function)
        {};

        ~Error();

        const char* what() const throw();
        CAEN_DGTZ_ErrorCode code() const { return code_; };

      private:
        CAEN_DGTZ_ErrorCode code_;
        const char* function = nullptr;
        mutable char* message = nullptr;
    };

    class ReadoutBuffer {
      public:
        ReadoutBuffer() {};
        ReadoutBuffer(ReadoutBuffer&&);
        ReadoutBuffer(const Digitizer& digitizer) { allocate(digitizer); };
        ~ReadoutBuffer() { deallocate(); };

        void allocate(const Digitizer&);
        void deallocate();

      private:
        char* memory = nullptr;
        uint32_t size;

      friend class Digitizer;
    };

    class Event {
      public:
        virtual ~Event() {};
    };

    // Event class for the basic (D-WAVE) firmware
    template <typename T = void> class WaveEvent;
    
    // Events and waveforms for the DPP firmware
    template <typename T = void> class DPPEvents;
    template <typename T = void> class DPPWaveforms;

    // Type trait mapping CAEN event type to CAEN waveforms type
    template <typename Event> struct CAEN_Waveforms;

    Digitizer(
        CAEN_DGTZ_ConnectionType link, uint32_t arg, int conet, uint32_t vme
    );
    Digitizer(Digitizer&&);
    ~Digitizer();

    int handle() const { return digitizer; };
    const CAEN_DGTZ_BoardInfo_t& info() const { return info_; };

    uint8_t DPPFirmwareCode(uint8_t channel = 0) const;

    uint32_t readRegister(uint32_t address) const;
    void writeRegister(uint32_t address, uint32_t data);

    // read or write bits from start to end (inclusive)
    uint32_t readRegister(uint32_t address, uint8_t start, uint8_t end) const;
    void writeRegister(
        uint32_t address, uint32_t data, uint8_t start, uint8_t end
    );

    void reset();

    void clearData();

    void disableEventAlignedReadout();

    uint32_t getMaxNumEventsBLT() const;
    void     setMaxNumEventsBLT(uint32_t value);

    ReadoutBuffer mallocReadoutBuffer() const;

    void readData(CAEN_DGTZ_ReadMode_t, ReadoutBuffer&) const;

    uint32_t getNumEvents(const ReadoutBuffer&) const;

    Event* allocateEvent() const;

    void getEvent(const ReadoutBuffer&, int32_t number, WaveEvent<>&) const;
    void getEvents(const ReadoutBuffer&, DPPEvents<>&) const;

    void calibrate();

    uint32_t readTemperature(int32_t channel) const;

    void sendSWTrigger();

    CAEN_DGTZ_TriggerMode_t getSWTriggerMode() const;
    void                    setSWTriggerMode(CAEN_DGTZ_TriggerMode_t);

    CAEN_DGTZ_TriggerMode_t getExtTriggerInputMode() const;
    void                    setExtTriggerInputMode(CAEN_DGTZ_TriggerMode_t);

    CAEN_DGTZ_TriggerMode_t getChannelSelfTrigger(uint32_t channel) const;
    void setChannelSelfTrigger(
        uint32_t channel_mask, CAEN_DGTZ_TriggerMode_t mode
    );

    CAEN_DGTZ_TriggerMode_t getGroupSelfTrigger(uint32_t group) const;
    void setGroupSelfTrigger(uint32_t group_mask, CAEN_DGTZ_TriggerMode_t mode);

    uint32_t getChannelGroupMask(uint32_t group) const;
    void     setChannelGroupMask(uint32_t group, uint32_t channels);

    uint32_t getChannelTriggerThreshold(uint32_t channel) const;
    void setChannelTriggerThreshold(uint32_t channel, uint32_t threshold);

    uint32_t getGroupTriggerThreshold(uint32_t group) const;
    void     setGroupTriggerThreshold(uint32_t group, uint32_t threshold);

    CAEN_DGTZ_RunSyncMode_t getRunSynchronizationMode() const;
    void setRunSynchronizationMode(CAEN_DGTZ_RunSyncMode_t);

    CAEN_DGTZ_IOLevel_t getIOLevel() const;
    void                setIOLevel(CAEN_DGTZ_IOLevel_t);

    CAEN_DGTZ_TriggerPolarity_t getTriggerPolarity(uint32_t channel) const;
    void setTriggerPolarity(uint32_t channel, CAEN_DGTZ_TriggerPolarity_t);

    uint32_t getGroupFastTriggerThreshold(uint32_t group) const;
    void setGroupFastTriggerThreshold(uint32_t group, uint32_t threshold);

    uint32_t getGroupFastTriggerDCOffset(uint32_t group) const;
    void     setGroupFastTriggerDCOffset(uint32_t group, uint32_t offset);

    bool getFastTriggerDigitizing() const;
    void setFastTriggerDigitizing(bool);

    CAEN_DGTZ_TriggerMode_t getFastTriggerMode() const;
    void                    setFastTriggerMode(CAEN_DGTZ_TriggerMode_t);

    CAEN_DGTZ_DRS4Frequency_t getDRS4SamplingFrequency() const;
    void setDRS4SamplingFrequency(CAEN_DGTZ_DRS4Frequency_t);

    CAEN_DGTZ_OutputSignalMode_t getOutputSignalMode() const;
    void setOutputSignalMode(CAEN_DGTZ_OutputSignalMode_t);

    uint32_t getChannelEnableMask() const;
    void     setChannelEnableMask(uint32_t);

    uint32_t getGroupEnableMask() const;
    void     setGroupEnableMask(uint32_t);

    void SWStartAcquisition();
    void SWStopAcquisition();

    uint32_t getRecordLength() const;
    uint32_t getRecordLength(uint32_t channel) const;
    void     setRecordLength(uint32_t size);
    void     setRecordLength(uint32_t channel, uint32_t size);

    uint32_t getPostTriggerSize() const;
    void     setPostTriggerSize(uint32_t percent);

    CAEN_DGTZ_AcqMode_t getAcquisitionMode() const;
    void                setAcquisitionMode(CAEN_DGTZ_AcqMode_t);

    uint32_t getChannelDCOffset(uint32_t channel) const;
    void     setChannelDCOffset(uint32_t channel, uint32_t offset);

    uint32_t getGroupDCOffset(uint32_t group) const;
    void     setGroupDCOffset(uint32_t group, uint32_t offset);

    bool getDESMode() const;
    void setDESMode(bool);

    uint16_t getDecimationFactor() const;
    void     setDecimationFactor(uint16_t);

    CAEN_DGTZ_ZS_Mode_t getZeroSuppressionMode() const;
    void                setZeroSuppressionMode(CAEN_DGTZ_ZS_Mode_t);

    void getChannelZSParams(
        uint32_t channel,
        CAEN_DGTZ_ThresholdWeight_t* weight,
        int32_t* threshold,
        int32_t* nsamp
    ) const;

    void setChannelZSParams(
        uint32_t channel,
        CAEN_DGTZ_ThresholdWeight_t weight,
        int32_t threshold,
        int32_t nsamp
    );

    CAEN_DGTZ_AnalogMonitorOutputMode_t getAnalogMonOutput() const;
    void setAnalogMonOutput(CAEN_DGTZ_AnalogMonitorOutputMode_t);

    void getAnalogInspectionMonParams(
        uint32_t* channelmask,
        uint32_t* offset,
        CAEN_DGTZ_AnalogMonitorMagnify_t*,
        CAEN_DGTZ_AnalogMonitorInspectorInverter_t*
    ) const;

    void setAnalogInspectionMonParams(
        uint32_t channelmask,
        uint32_t offset,
        CAEN_DGTZ_AnalogMonitorMagnify_t,
        CAEN_DGTZ_AnalogMonitorInspectorInverter_t
    );

    bool getEventPackaging() const;
    void setEventPackaging(bool);

    void getDPPAcquisitionMode(
        CAEN_DGTZ_DPP_AcqMode_t*,
        CAEN_DGTZ_DPP_SaveParam_t*
    );
    void setDPPAcquisitionMode(
        CAEN_DGTZ_DPP_AcqMode_t,
        CAEN_DGTZ_DPP_SaveParam_t
    );

    void setDPPEventAggregation(int threshold = 0, int maxsize = 0);

    void setDPPParameters(uint32_t channels, CAEN_DGTZ_DPP_PSD_Params_t&);

    uint32_t getDPPPreTriggerSize(int channel) const;
    void     setDPPPreTriggerSize(int channel, uint32_t samples);

    int getDPPVirtualProbe(int trace);
    void setDPPVirtualProbe(int trace, int probe);

    CAEN_DGTZ_PulsePolarity_t getChannelPulsePolarity(uint32_t channel) const;
    void setChannelPulsePolarity(uint32_t channel, CAEN_DGTZ_PulsePolarity_t);

  private:
    int digitizer;
    CAEN_DGTZ_BoardInfo_t info_;

    Digitizer();
};

template <>
class Digitizer::WaveEvent<void> : public Digitizer::Event {
  public:
    CAEN_DGTZ_EventInfo_t info;

    WaveEvent() {};
    WaveEvent(WaveEvent&&);
    WaveEvent(const Digitizer& digitizer) { allocate(digitizer); };
    ~WaveEvent() { deallocate(); };

    void allocate(const Digitizer&);
    void deallocate();

    void* data() { return data_; };

  private:
    void* data_ = nullptr;
    int digitizer;

  friend class Digitizer;
};

template <typename T>
class Digitizer::WaveEvent : public Digitizer::WaveEvent<void> {
  public:
    WaveEvent() {};
    WaveEvent(WaveEvent&& event): WaveEvent<void>(event) {};
    WaveEvent(const Digitizer& digitizer): WaveEvent<void>(digitizer) {};

    T* data() { return static_cast<T*>(WaveEvent<void>::data()); };
};

template <typename T>
struct Digitizer::CAEN_Waveforms {
  using type = void;
};

#define CAEN_DGTZ_MAP_EVENT_TYPE(firmware) \
  template <> \
  struct Digitizer::CAEN_Waveforms<CAEN_DGTZ_DPP_ ## firmware ## _Event_t> { \
    using type = CAEN_DGTZ_DPP_ ## firmware ## _Waveforms_t; \
  }
  CAEN_DGTZ_MAP_EVENT_TYPE(PHA);
  CAEN_DGTZ_MAP_EVENT_TYPE(PSD);
  CAEN_DGTZ_MAP_EVENT_TYPE(CI);
  CAEN_DGTZ_MAP_EVENT_TYPE(QDC);
#undef CAEN_DGTZ_MAP_EVENT_TYPE

template <>
class Digitizer::DPPWaveforms<void> {
  public:
    DPPWaveforms() {};
    DPPWaveforms(DPPWaveforms&&);
    DPPWaveforms(const Digitizer& digitizer) { allocate(digitizer); };
    ~DPPWaveforms() { deallocate(); };

    void allocate(const Digitizer&);
    void deallocate();

    void* waveforms() { return waveforms_; };

  private:
    void* waveforms_ = nullptr;
    int digitizer;
    uint32_t size;

  friend class DPPEvent;
};

template <typename Waveforms>
class Digitizer::DPPWaveforms : public Digitizer::DPPWaveforms<void> {
  public:
    DPPWaveforms() {};

    DPPWaveforms(DPPWaveforms&& waveforms):
      DPPWaveforms<void>(std::move(waveforms))
    {};

    DPPWaveforms(const Digitizer& digitizer): DPPWaveforms<void>(digitizer) {};

    Waveforms* waveforms() {
      return static_cast<Waveforms*>(
          Digitizer::DPPWaveforms<void>::waveforms()
      );
    };
};

template <>
class Digitizer::DPPEvents<void> : public Digitizer::Event {
  public:
    using waveforms_t = void;

    DPPEvents() {};
    DPPEvents(DPPEvents&&);
    DPPEvents(const Digitizer& digitizer) { allocate(digitizer); };
    ~DPPEvents() { deallocate(); };

    void allocate(const Digitizer&);
    void deallocate();

    template <typename E>
    E* event(uint32_t channel, uint32_t number) const {
      return static_cast<E*>(events_[channel]) + number;
    };

    template <typename E>
    E* begin(uint32_t channel) const {
      return event<E>(channel, 0);
    };

    template <typename E>
    E* end(uint32_t channel) const {
      return event<E>(channel, nevents_[channel]);
    };

    uint32_t nevents(uint32_t channel) const { return nevents_[channel]; };

    void decode(void* event, DPPWaveforms<void>& waveforms) const {
      decode(event, waveforms.waveforms());
    };

    void decode(void* event, void* waveforms) const;

  private:
    void*    events_[CAEN_DGTZ_MAX_CHANNEL];
    uint32_t nevents_[CAEN_DGTZ_MAX_CHANNEL];
    uint32_t size = 0;
    int      digitizer;

  friend class Digitizer;
};

template <typename E>
class Digitizer::DPPEvents : public Digitizer::DPPEvents<void> {
  public:
    using waveforms_t = typename Digitizer::CAEN_Waveforms<E>::type;

    DPPEvents() {};
    DPPEvents(DPPEvents&& events): DPPEvents<void>(std::move(events)) {};
    DPPEvents(const Digitizer& digitizer): DPPEvents<void>(digitizer) {};

    E* event(uint32_t channel, uint32_t number) const {
      return DPPEvents<void>::event<E>(channel, number);
    };

    E* begin(uint32_t channel) const {
      return static_cast<E*>(DPPEvents<void>::begin<E>(channel));
    };

    E* end(uint32_t channel) const {
      return static_cast<E*>(DPPEvents<void>::end<E>(channel));
    };

    void decode(E* event, DPPWaveforms<waveforms_t>& waveforms) const {
      DPPEvents<void>::decode(event, waveforms.waveforms());
    };
};

}; // namespace caen
