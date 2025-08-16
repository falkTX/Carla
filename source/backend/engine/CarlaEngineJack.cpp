// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaEngineClient.hpp"
#include "CarlaEngineInit.hpp"
#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"
#include "CarlaPatchbayUtils.hpp"
#include "CarlaStringList.hpp"
#include "extra/ScopedPointer.hpp"

#include "jackey.h"

#ifdef __SSE2_MATH__
# include <xmmintrin.h>
#endif

// must be last
#include "jackbridge/JackBridge.hpp"

#ifdef JACKBRIDGE_DIRECT
# define JackPortIsCV 0x20
#endif

#define URI_CANVAS_ICON "http://kxstudio.sf.net/ns/canvas/icon"

#define URI_MAIN_CLIENT_NAME "https://kx.studio/ns/carla/main-client-name"
#define URI_POSITION         "https://kx.studio/ns/carla/position"
#define URI_PLUGIN_ICON      "https://kx.studio/ns/carla/plugin-icon"
#define URI_PLUGIN_ID        "https://kx.studio/ns/carla/plugin-id"

#define URI_TYPE_INTEGER "http://www.w3.org/2001/XMLSchema#integer"
#define URI_TYPE_STRING  "text/plain"

CARLA_BACKEND_START_NAMESPACE

class CarlaEngineJack;
class CarlaEngineJackClient;

#ifndef BUILD_BRIDGE
struct CarlaJackPortHints {
    bool isHardware : 1;
    bool isInput    : 1;
    bool isAudio    : 1;
    bool isMIDI     : 1;
    bool isCV       : 1;
    bool isOSC      : 1;

    // NOTE: assumes fThreadSafeMetadataMutex lock done from caller
    static CarlaJackPortHints fromPort(const jack_port_t* const jackPort)
    {
        CarlaJackPortHints ph = { false, false, false, false, false, false };

        const int portFlags = jackbridge_port_flags(jackPort);
        const char* const portType = jackbridge_port_type(jackPort);

        ph.isHardware = portFlags & JackPortIsPhysical;
        ph.isInput    = portFlags & JackPortIsInput;
        ph.isAudio    = portType != nullptr && std::strcmp(portType, JACK_DEFAULT_AUDIO_TYPE) == 0;
        ph.isMIDI     = portType != nullptr && std::strcmp(portType, JACK_DEFAULT_MIDI_TYPE) == 0;
        ph.isCV       = false;
        ph.isOSC      = false;

        if (ph.isAudio && portFlags & JackPortIsCV)
        {
            ph.isAudio = false;
            ph.isCV    = true;
        }

        if (const jack_uuid_t uuid = jackbridge_port_uuid(jackPort))
        {
            char* value = nullptr;
            char* type  = nullptr;

            if (jackbridge_get_property(uuid, JACKEY_SIGNAL_TYPE, &value, &type)
                && value != nullptr
                && type != nullptr
                && std::strcmp(type, URI_TYPE_STRING) == 0)
            {
                ph.isCV  = (std::strcmp(value, "CV") == 0);
                ph.isOSC = (std::strcmp(value, "OSC") == 0);

                if (ph.isCV)
                    ph.isAudio = false;
                if (ph.isOSC)
                    ph.isMIDI = false;

                jackbridge_free(value);
                jackbridge_free(type);
            }
        }

        return ph;
    }
};
#endif

// -----------------------------------------------------------------------
// Fallback data

#ifndef BUILD_BRIDGE
static const GroupNameToId  kGroupNameToIdFallback   = { 0, { '\0' } };
static /* */ PortNameToId   kPortNameToIdFallbackNC  = { 0, 0, { '\0' }, { '\0' }, { '\0' } };
static const PortNameToId   kPortNameToIdFallback    = { 0, 0, { '\0' }, { '\0' }, { '\0' } };
static const ConnectionToId kConnectionToIdFallback  = { 0, 0, 0, 0, 0 };
#endif
static EngineEvent kFallbackJackEngineEvent = {
    kEngineEventTypeNull, 0, 0, {{ kEngineControlEventTypeNull, 0, -1, 0.0f, true }}
};

// -----------------------------------------------------------------------
// Carla Engine Port removal helper

class CarlaEngineJackAudioPort;
class CarlaEngineJackCVPort;
class CarlaEngineJackEventPort;

struct JackPortDeletionCallback {
    virtual ~JackPortDeletionCallback() noexcept {}
    virtual void jackAudioPortDeleted(CarlaEngineJackAudioPort* const) noexcept = 0;
    virtual void jackCVPortDeleted(CarlaEngineJackCVPort* const) noexcept = 0;
    virtual void jackEventPortDeleted(CarlaEngineJackEventPort* const) noexcept = 0;
};

// -----------------------------------------------------------------------
// Carla Engine JACK-Audio port

class CarlaEngineJackAudioPort : public CarlaEngineAudioPort
{
public:
    CarlaEngineJackAudioPort(const CarlaEngineClient& client,
                             const bool isInputPort,
                             const uint32_t indexOffset,
                             jack_client_t* const jackClient,
                             jack_port_t* const jackPort,
                             CarlaRecursiveMutex& rmutex,
                             JackPortDeletionCallback* const delCallback) noexcept
        : CarlaEngineAudioPort(client, isInputPort, indexOffset),
          fJackClient(jackClient),
          fJackPort(jackPort),
          fThreadSafeMetadataMutex(rmutex),
          kDeletionCallback(delCallback)
    {
        carla_debug("CarlaEngineJackAudioPort::CarlaEngineJackAudioPort(%s, %p, %p)", bool2str(isInputPort), jackClient, jackPort);

        switch (kClient.getEngine().getProccessMode())
        {
        case ENGINE_PROCESS_MODE_SINGLE_CLIENT:
        case ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
            CARLA_SAFE_ASSERT_RETURN(jackClient != nullptr && jackPort != nullptr,);
            {
                const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

                if (const jack_uuid_t uuid = jackbridge_port_uuid(jackPort))
                    jackbridge_set_property(jackClient, uuid, JACKEY_SIGNAL_TYPE, "AUDIO", URI_TYPE_STRING);
            }
            break;

        default:
            CARLA_SAFE_ASSERT(jackClient == nullptr && jackPort == nullptr);
            break;
        }
    }

    ~CarlaEngineJackAudioPort() noexcept override
    {
        carla_debug("CarlaEngineJackAudioPort::~CarlaEngineJackAudioPort()");

        if (fJackClient != nullptr && fJackPort != nullptr)
        {
            try {
                jackbridge_port_unregister(fJackClient, fJackPort);
            } CARLA_SAFE_EXCEPTION("Audio port unregister");

            fJackClient = nullptr;
            fJackPort   = nullptr;
        }

        if (kDeletionCallback != nullptr)
            kDeletionCallback->jackAudioPortDeleted(this);
    }

    void initBuffer() noexcept override
    {
        if (fJackPort == nullptr)
            return CarlaEngineAudioPort::initBuffer();

        const uint32_t bufferSize(kClient.getEngine().getBufferSize());

        try {
            fBuffer = (float*)jackbridge_port_get_buffer(fJackPort, bufferSize);
        }
        catch(...) {
            fBuffer = nullptr;
            return;
        }

        if (! kIsInput)
            carla_zeroFloats(fBuffer, bufferSize);
    }

    void invalidate() noexcept
    {
        fJackClient = nullptr;
        fJackPort   = nullptr;
    }

    void setMetaData(const char* const key, const char* const value, const char* const type) override
    {
        if (fJackPort == nullptr)
            return CarlaEngineAudioPort::setMetaData(key, value, type);

        const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

        try {
            if (const jack_uuid_t uuid = jackbridge_port_uuid(fJackPort))
                jackbridge_set_property(fJackClient, uuid, key, value, type);
        } CARLA_SAFE_EXCEPTION("Port setMetaData");
    }

private:
    jack_client_t* fJackClient;
    jack_port_t*   fJackPort;

    CarlaRecursiveMutex& fThreadSafeMetadataMutex;
    JackPortDeletionCallback* const kDeletionCallback;

    friend class CarlaEngineJackClient;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackAudioPort)
};

// -----------------------------------------------------------------------
// Carla Engine JACK-CV port

class CarlaEngineJackCVPort : public CarlaEngineCVPort
{
public:
    CarlaEngineJackCVPort(const CarlaEngineClient& client,
                          const bool isInputPort,
                          const uint32_t indexOffset,
                          jack_client_t* const jackClient,
                          jack_port_t* const jackPort,
                          CarlaRecursiveMutex& rmutex,
                          JackPortDeletionCallback* const delCallback) noexcept
        : CarlaEngineCVPort(client, isInputPort, indexOffset),
          fJackClient(jackClient),
          fJackPort(jackPort),
          fThreadSafeMetadataMutex(rmutex),
          kDeletionCallback(delCallback)
    {
        carla_debug("CarlaEngineJackCVPort::CarlaEngineJackCVPort(%s, %p, %p)", bool2str(isInputPort), jackClient, jackPort);

        switch (kClient.getEngine().getProccessMode())
        {
        case ENGINE_PROCESS_MODE_SINGLE_CLIENT:
        case ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
            CARLA_SAFE_ASSERT_RETURN(jackClient != nullptr && jackPort != nullptr,);
            {
                const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

                if (const jack_uuid_t uuid = jackbridge_port_uuid(jackPort))
                    jackbridge_set_property(jackClient, uuid, JACKEY_SIGNAL_TYPE, "CV", URI_TYPE_STRING);
            }
            break;

        default:
            CARLA_SAFE_ASSERT(jackClient == nullptr && jackPort == nullptr);
            break;
        }
    }

    ~CarlaEngineJackCVPort() noexcept override
    {
        carla_debug("CarlaEngineJackCVPort::~CarlaEngineJackCVPort()");

        if (fJackClient != nullptr && fJackPort != nullptr)
        {
            try {
                jackbridge_port_unregister(fJackClient, fJackPort);
            } CARLA_SAFE_EXCEPTION("CV port unregister");

            fJackClient = nullptr;
            fJackPort   = nullptr;
        }

        if (kDeletionCallback != nullptr)
            kDeletionCallback->jackCVPortDeleted(this);
    }

    void initBuffer() noexcept override
    {
        if (fJackPort == nullptr)
            return CarlaEngineCVPort::initBuffer();

        const uint32_t bufferSize(kClient.getEngine().getBufferSize());

        try {
            fBuffer = (float*)jackbridge_port_get_buffer(fJackPort, bufferSize);
        }
        catch(...) {
            fBuffer = nullptr;
            return;
        }

        if (! kIsInput)
            carla_zeroFloats(fBuffer, bufferSize);
    }

    void invalidate() noexcept
    {
        fJackClient = nullptr;
        fJackPort   = nullptr;
    }

    void setMetaData(const char* const key, const char* const value, const char* const type) override
    {
        if (fJackPort == nullptr)
            return CarlaEngineCVPort::setMetaData(key, value, type);

        const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

        try {
            if (const jack_uuid_t uuid = jackbridge_port_uuid(fJackPort))
                jackbridge_set_property(fJackClient, uuid, key, value, type);
        } CARLA_SAFE_EXCEPTION("Port setMetaData");
    }

private:
    jack_client_t* fJackClient;
    jack_port_t*   fJackPort;

    CarlaRecursiveMutex& fThreadSafeMetadataMutex;
    JackPortDeletionCallback* const kDeletionCallback;

    friend class CarlaEngineJackClient;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackCVPort)
};

// -----------------------------------------------------------------------
// Carla Engine JACK-Event port

class CarlaEngineJackEventPort : public CarlaEngineEventPort
{
public:
    CarlaEngineJackEventPort(const CarlaEngineClient& client,
                             const bool isInputPort,
                             const uint32_t indexOffset,
                             jack_client_t* const jackClient,
                             jack_port_t* const jackPort,
                             CarlaRecursiveMutex& rmutex,
                             JackPortDeletionCallback* const delCallback) noexcept
        : CarlaEngineEventPort(client, isInputPort, indexOffset),
          fJackClient(jackClient),
          fJackPort(jackPort),
          fJackBuffer(nullptr),
          fRetEvent(kFallbackJackEngineEvent),
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
          fCvSourceEvents(nullptr),
          fCvSourceEventCount(0),
#endif
          fThreadSafeMetadataMutex(rmutex),
          kDeletionCallback(delCallback)
    {
        carla_debug("CarlaEngineJackEventPort::CarlaEngineJackEventPort(%s, %p, %p)", bool2str(isInputPort), jackClient, jackPort);

        switch (kClient.getEngine().getProccessMode())
        {
        case ENGINE_PROCESS_MODE_SINGLE_CLIENT:
        case ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
            CARLA_SAFE_ASSERT_RETURN(jackClient != nullptr && jackPort != nullptr,);
            break;
        default:
            CARLA_SAFE_ASSERT(jackClient == nullptr && jackPort == nullptr);
            break;
        }
    }

    ~CarlaEngineJackEventPort() noexcept override
    {
        carla_debug("CarlaEngineJackEventPort::~CarlaEngineJackEventPort()");

        if (fJackClient != nullptr && fJackPort != nullptr)
        {
            try {
                jackbridge_port_unregister(fJackClient, fJackPort);
            } CARLA_SAFE_EXCEPTION("Event port unregister");

            fJackClient = nullptr;
            fJackPort   = nullptr;
        }

        if (kDeletionCallback != nullptr)
            kDeletionCallback->jackEventPortDeleted(this);
    }

    void initBuffer() noexcept override
    {
        if (fJackPort == nullptr)
            return CarlaEngineEventPort::initBuffer();

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        fCvSourceEvents = nullptr;
        fCvSourceEventCount = 0;
#endif

        try {
            fJackBuffer = jackbridge_port_get_buffer(fJackPort, kClient.getEngine().getBufferSize());
        }
        catch(...) {
            fJackBuffer = nullptr;
            return;
        }

        if (! kIsInput)
            jackbridge_midi_clear_buffer(fJackBuffer);
    }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    void setCvSourceEvents(EngineEvent* const events, const uint32_t eventCount) noexcept
    {
        fCvSourceEvents = events;
        fCvSourceEventCount = eventCount;
    }
#endif

    uint32_t getEventCount() const noexcept override
    {
        if (fJackPort == nullptr)
            return CarlaEngineEventPort::getEventCount();

        CARLA_SAFE_ASSERT_RETURN(kIsInput, 0);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, 0);

        try {
            return jackbridge_midi_get_event_count(fJackBuffer)
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                    + fCvSourceEventCount
#endif
                    ;
        } CARLA_SAFE_EXCEPTION_RETURN("jack_midi_get_event_count", 0);
    }

    EngineEvent& getEvent(const uint32_t index) const noexcept override
    {
        if (fJackPort == nullptr)
            return CarlaEngineEventPort::getEvent(index);

        CARLA_SAFE_ASSERT_RETURN(kIsInput, kFallbackJackEngineEvent);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, kFallbackJackEngineEvent);

        return getEventUnchecked(index);
    }

    EngineEvent& getEventUnchecked(uint32_t index) const noexcept override
    {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (index < fCvSourceEventCount)
            return fCvSourceEvents[index];

        index -= fCvSourceEventCount;
#endif

        jack_midi_event_t jackEvent;

        bool test = false;

        try {
            test = jackbridge_midi_event_get(&jackEvent, fJackBuffer, index);
        } CARLA_SAFE_EXCEPTION_RETURN("jack_midi_event_get", kFallbackJackEngineEvent);

        if (! test)
            return kFallbackJackEngineEvent;

        CARLA_SAFE_ASSERT_RETURN(jackEvent.size < 0xFF /* uint8_t max */, kFallbackJackEngineEvent);

        uint8_t port;

        if (kIndexOffset < 0xFF /* uint8_t max */)
        {
            port = static_cast<uint8_t>(kIndexOffset);
        }
        else
        {
            port = 0;
            carla_safe_assert_uint("kIndexOffset < 0xFF", __FILE__, __LINE__, kIndexOffset);
        }

        fRetEvent.time = jackEvent.time;
        fRetEvent.fillFromMidiData(static_cast<uint8_t>(jackEvent.size), jackEvent.buffer, port);

        return fRetEvent;
    }

    bool writeControlEvent(const uint32_t time, const uint8_t channel,
                           const EngineControlEventType type,
                           const uint16_t param,
                           const int8_t midiValue,
                           const float value) noexcept override
    {
        if (fJackPort == nullptr)
            return CarlaEngineEventPort::writeControlEvent(time, channel, type, param, midiValue, value);

        CARLA_SAFE_ASSERT_RETURN(! kIsInput, false);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(type != kEngineControlEventTypeNull, false);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, false);
        CARLA_SAFE_ASSERT_RETURN(param < MAX_MIDI_VALUE, false);
        CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.0f);

        if (type == kEngineControlEventTypeParameter) {
            CARLA_SAFE_ASSERT(! MIDI_IS_CONTROL_BANK_SELECT(param));
        }

        uint8_t data[3] = { 0, 0, 0 };

        EngineControlEvent ctrlEvent = { type, param, midiValue, value, false };
        const uint8_t size = ctrlEvent.convertToMidiData(channel, data);

        if (size == 0)
            return false;

        try {
            return jackbridge_midi_event_write(fJackBuffer, time, data, size);
        } CARLA_SAFE_EXCEPTION_RETURN("jack_midi_event_write", false);
    }

    bool writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t size, const uint8_t* const data) noexcept override
    {
        if (fJackPort == nullptr)
            return CarlaEngineEventPort::writeMidiEvent(time, channel, size, data);

        CARLA_SAFE_ASSERT_RETURN(! kIsInput, false);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, false);
        CARLA_SAFE_ASSERT_RETURN(size > 0, false);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

        jack_midi_data_t jdata[size];

        jdata[0] = static_cast<jack_midi_data_t>(MIDI_GET_STATUS_FROM_DATA(data) + channel);

        for (uint8_t i=1; i < size; ++i)
            jdata[i] = data[i];

        try {
            return jackbridge_midi_event_write(fJackBuffer, time, jdata, size);
        } CARLA_SAFE_EXCEPTION_RETURN("jack_midi_event_write", false);
    }

    void invalidate() noexcept
    {
        fJackClient = nullptr;
        fJackPort   = nullptr;
    }

    void setMetaData(const char* const key, const char* const value, const char* const type) override
    {
        if (fJackPort == nullptr)
            return CarlaEngineEventPort::setMetaData(key, value, type);

        const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

        try {
            if (const jack_uuid_t uuid = jackbridge_port_uuid(fJackPort))
                jackbridge_set_property(fJackClient, uuid, key, value, type);
        } CARLA_SAFE_EXCEPTION("Port setMetaData");
    }

private:
    jack_client_t* fJackClient;
    jack_port_t*   fJackPort;
    void*          fJackBuffer;

    mutable EngineEvent fRetEvent;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    EngineEvent* fCvSourceEvents;
    uint32_t fCvSourceEventCount;
#endif

    CarlaRecursiveMutex& fThreadSafeMetadataMutex;
    JackPortDeletionCallback* const kDeletionCallback;

    friend class CarlaEngineJackClient;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackEventPort)
};

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
// -----------------------------------------------------------------------
// Jack Engine CV source ports

class CarlaEngineJackCVSourcePorts : public CarlaEngineCVSourcePorts
{
public:
    CarlaEngineJackCVSourcePorts(const bool useClient)
        : CarlaEngineCVSourcePorts(),
          fUseClient(useClient),
          fBuffer(nullptr),
          fBufferToDeleteLater(nullptr)
    {}

    ~CarlaEngineJackCVSourcePorts() override
    {
        if (fBufferToDeleteLater != nullptr)
        {
            delete[] fBufferToDeleteLater;
            fBufferToDeleteLater = nullptr;
        }
    }

    bool addCVSource(CarlaEngineCVPort* const port, const uint32_t portIndexOffset, const bool reconfigureNow) override
    {
        if (! fUseClient)
            return CarlaEngineCVSourcePorts::addCVSource(port, portIndexOffset, reconfigureNow);

        const CarlaRecursiveMutexLocker crml(pData->rmutex);

        if (! CarlaEngineCVSourcePorts::addCVSource(port, portIndexOffset, reconfigureNow))
            return false;

        if (pData->cvs.size() == 1 && fBuffer == nullptr)
        {
            EngineEvent* const buffer = new EngineEvent[kMaxEngineEventInternalCount];
            carla_zeroStructs(buffer, kMaxEngineEventInternalCount);

            fBuffer = buffer;
        }

        return true;
    }

    bool removeCVSource(const uint32_t portIndexOffset) override
    {
        if (! fUseClient)
            return CarlaEngineCVSourcePorts::removeCVSource(portIndexOffset);

        const CarlaRecursiveMutexLocker crml(pData->rmutex);

        if (! CarlaEngineCVSourcePorts::removeCVSource(portIndexOffset))
            return false;

        if (pData->cvs.size() == 0 && fBuffer != nullptr)
        {
            if (fBufferToDeleteLater != nullptr)
                delete[] fBufferToDeleteLater;

            fBufferToDeleteLater = fBuffer;
            fBuffer = nullptr;
        }

        return true;
    }

    void initPortBuffers(const float* const* const buffers,
                         const uint32_t frames,
                         const bool sampleAccurate,
                         CarlaEngineEventPort* const eventPort) override
    {
        if (! fUseClient)
            return CarlaEngineCVSourcePorts::initPortBuffers(buffers, frames, sampleAccurate, eventPort);

        CARLA_SAFE_ASSERT_RETURN(buffers != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(eventPort != nullptr,);

        const CarlaRecursiveMutexTryLocker crmtl(pData->rmutex);

        if (! crmtl.wasLocked())
            return;

        const int numCVs = pData->cvs.size();

        if (numCVs == 0)
            return;

        EngineEvent* const buffer = fBuffer;
        CARLA_SAFE_ASSERT_RETURN(buffer != nullptr,);

        uint32_t eventCount = 0;
        float v, min, max;

        for (int i = 0; i < numCVs && eventCount < kMaxEngineEventInternalCount; ++i)
        {
            CarlaEngineEventCV& ecv(pData->cvs.getReference(i));
            CARLA_SAFE_ASSERT_CONTINUE(ecv.cvPort != nullptr);
            CARLA_SAFE_ASSERT_CONTINUE(buffers[i] != nullptr);

            float previousValue = ecv.previousValue;
            ecv.cvPort->getRange(min, max);

            v = buffers[i][0];

            if (carla_isNotEqual(v, previousValue))
            {
                previousValue = v;

                EngineEvent& event(buffer[eventCount++]);

                event.type    = kEngineEventTypeControl;
                event.time    = 0;
                event.channel = kEngineEventNonMidiChannel;

                event.ctrl.type            = kEngineControlEventTypeParameter;
                event.ctrl.param           = static_cast<uint16_t>(ecv.indexOffset);
                event.ctrl.midiValue       = -1;
                event.ctrl.normalizedValue = carla_fixedValue(0.0f, 1.0f, (v - min) / (max - min));
            }

            ecv.previousValue = previousValue;
        }

        if (eventCount != 0)
            if (CarlaEngineJackEventPort* const jackEventPort = dynamic_cast<CarlaEngineJackEventPort*>(eventPort))
                jackEventPort->setCvSourceEvents(buffer, eventCount);
    }

    CarlaRecursiveMutex& getMutex() const noexcept
    {
        return pData->rmutex;
    }

    uint32_t getPortCount() const noexcept
    {
        return static_cast<uint32_t>(pData->cvs.size());
    }

    CarlaEngineCVPort* getPort(const uint32_t portIndexOffset) const
    {
        const int ioffset = static_cast<int>(portIndexOffset);
        return pData->cvs[ioffset].cvPort;
    }

    void resetGraphAndPlugin() noexcept
    {
        pData->graph = nullptr;
        pData->plugin.reset();
    }

    void setGraphAndPlugin(PatchbayGraph* const graph, const CarlaPluginPtr plugin) noexcept
    {
        pData->graph = graph;
        pData->plugin = plugin;
    }

private:
    const bool fUseClient;
    EngineEvent* fBuffer;
    EngineEvent* fBufferToDeleteLater;

    CARLA_DECLARE_NON_COPYABLE(CarlaEngineJackCVSourcePorts)
};
#endif

// -----------------------------------------------------------------------
// Jack Engine client

class CarlaEngineJackClient : public CarlaEngineClientForSubclassing,
                              private JackPortDeletionCallback
{
public:
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CarlaEngineJackClient(const CarlaEngine& engine,
                          EngineInternalGraph& egraph,
                          CarlaRecursiveMutex& rmutex,
                          const CarlaPluginPtr plugin,
                          const String& mainClientName,
                          jack_client_t* const jackClient)
        : CarlaEngineClientForSubclassing(engine, egraph, plugin),
#else
    CarlaEngineJackClient(const CarlaEngine& engine,
                          CarlaRecursiveMutex& rmutex,
                          const String& mainClientName,
                          jack_client_t* const jackClient)
        : CarlaEngineClientForSubclassing(engine),
#endif
          fJackClient(jackClient),
          fUseClient(engine.getProccessMode() == ENGINE_PROCESS_MODE_SINGLE_CLIENT ||
                     engine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS),
          fAudioPorts(),
          fCVPorts(),
          fEventPorts(),
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
          fCVSourcePorts(fUseClient),
          fPreRenameMutex(),
          fPreRenameConnections(),
          fPreRenamePluginId(),
          fPreRenamePluginIcon(),
          fReservedPluginPtr(),
#endif
          fThreadSafeMetadataMutex(rmutex),
          fMainClientName(mainClientName)
    {
        carla_debug("CarlaEngineJackClient::CarlaEngineJackClient(%p)", jackClient);

        if (fUseClient)
        {
            CARLA_SAFE_ASSERT(jackClient != nullptr);
        }
        else
        {
            CARLA_SAFE_ASSERT(jackClient == nullptr);
        }
    }

    ~CarlaEngineJackClient() noexcept override
    {
        carla_debug("CarlaEngineJackClient::~CarlaEngineJackClient()");

        if (getProcessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS && fJackClient != nullptr) // FIXME
            jackbridge_client_close(fJackClient);

        // ports must have been deleted by now!
        //fAudioPorts.clear();
        //fCVPorts.clear();
        //fEventPorts.clear();

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        const CarlaMutexLocker cml(fPreRenameMutex);

        fPreRenameConnections.clear();
        fPreRenamePluginId.clear();
        fPreRenamePluginIcon.clear();
#endif
    }

    void activate() noexcept override
    {
        carla_debug("CarlaEngineJackClient::activate()");

        if (getProcessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_SAFE_ASSERT_RETURN(fJackClient != nullptr && ! isActive(),);

            try {
                jackbridge_activate(fJackClient);
            } catch(...) {}
        }

        CarlaEngineClient::activate();

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        const CarlaMutexLocker cml(fPreRenameMutex);

        if (fJackClient != nullptr)
        {
            // restore pre-rename connections
            const char* portNameA = nullptr;
            const char* portNameB = nullptr;
            bool doConnection = false;

            for (CarlaStringList::Itenerator it = fPreRenameConnections.begin2(); it.valid(); it.next())
            {
                const bool connectNow = doConnection;
                doConnection = !doConnection;

                if (connectNow)
                    portNameB = it.getValue(nullptr);
                else
                    portNameA = it.getValue(nullptr);

                if (! connectNow)
                    continue;

                CARLA_SAFE_ASSERT_CONTINUE(portNameA != nullptr && portNameA[0] != '\0');
                CARLA_SAFE_ASSERT_CONTINUE(portNameB != nullptr && portNameB[0] != '\0');

                jackbridge_connect(fJackClient, portNameA, portNameB);
            }

            if (fPreRenamePluginId.isNotEmpty())
            {
                const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

                if (char* const uuidstr = jackbridge_client_get_uuid(fJackClient))
                {
                    jack_uuid_t uuid;

                    if (jackbridge_uuid_parse(uuidstr, &uuid))
                    {
                        jackbridge_set_property(fJackClient, uuid,
                                                URI_MAIN_CLIENT_NAME,
                                                fMainClientName,
                                                URI_TYPE_STRING);

                        jackbridge_set_property(fJackClient, uuid,
                                                URI_PLUGIN_ID,
                                                fPreRenamePluginId,
                                                URI_TYPE_INTEGER);

                        if (fPreRenamePluginIcon.isNotEmpty())
                            jackbridge_set_property(fJackClient, uuid,
                                                    URI_PLUGIN_ICON,
                                                    fPreRenamePluginIcon,
                                                    URI_TYPE_STRING);
                    }

                    jackbridge_free(uuidstr);
                }
            }
        }

        fPreRenameConnections.clear();
        fPreRenamePluginId.clear();
        fPreRenamePluginIcon.clear();
#endif
    }

    void deactivate(const bool willClose) noexcept override
    {
        carla_debug("CarlaEngineJackClient::deactivate(%s)", bool2str(willClose));

        if (getProcessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            if (fJackClient != nullptr && isActive())
            {
                try {
                    jackbridge_deactivate(fJackClient);
                } catch(...) {}
            }
        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (willClose)
        {
            fCVSourcePorts.resetGraphAndPlugin();
            fReservedPluginPtr = nullptr;
        }
#endif

        CarlaEngineClient::deactivate(willClose);
    }

    bool isOk() const noexcept override
    {
        if (fUseClient)
            return (fJackClient != nullptr);

        return CarlaEngineClient::isOk();
    }

    CarlaEnginePort* addPort(const EnginePortType portType, const char* const name, const bool isInput, const uint32_t indexOffset) override
    {
        carla_debug("CarlaEngineJackClient::addPort(%i:%s, \"%s\", %s)", portType, EnginePortType2Str(portType), name, bool2str(isInput));

        jack_port_t* jackPort = nullptr;
        const char* realName = name;

        // Create JACK port first, if needed
        if (fUseClient)
        {
            CARLA_SAFE_ASSERT_RETURN(fJackClient != nullptr, nullptr);

            realName = pData->getUniquePortName(name);

            switch (portType)
            {
            case kEnginePortTypeNull:
                break;
            case kEnginePortTypeAudio:
                jackPort = jackbridge_port_register(fJackClient,
                                                    realName,
                                                    JACK_DEFAULT_AUDIO_TYPE,
                                                    isInput ? JackPortIsInput : JackPortIsOutput,
                                                    0);
                break;
            case kEnginePortTypeCV:
                jackPort = jackbridge_port_register(fJackClient,
                                                    realName,
                                                    JACK_DEFAULT_AUDIO_TYPE,
                                                    static_cast<uint64_t>(JackPortIsCV |
                                                                          (isInput ? JackPortIsInput
                                                                                   : JackPortIsOutput)),
                                                    0);
                break;
            case kEnginePortTypeEvent:
                jackPort = jackbridge_port_register(fJackClient,
                                                    realName,
                                                    JACK_DEFAULT_MIDI_TYPE,
                                                    isInput ? JackPortIsInput : JackPortIsOutput,
                                                    0);
                break;
            }

            CARLA_SAFE_ASSERT_RETURN(jackPort != nullptr, nullptr);
        }

        // Create Engine port
        switch (portType)
        {
        case kEnginePortTypeNull:
            break;
        case kEnginePortTypeAudio: {
            pData->addAudioPortName(isInput, realName);
            if (realName != name) delete[] realName;
            CarlaEngineJackAudioPort* const enginePort(new CarlaEngineJackAudioPort(*this,
                                                                                    isInput,
                                                                                    indexOffset,
                                                                                    fJackClient,
                                                                                    jackPort,
                                                                                    fThreadSafeMetadataMutex,
                                                                                    this));
            fAudioPorts.append(enginePort);
            return enginePort;
        }
        case kEnginePortTypeCV: {
            pData->addCVPortName(isInput, realName);
            if (realName != name) delete[] realName;
            CarlaEngineJackCVPort* const enginePort(new CarlaEngineJackCVPort(*this,
                                                                              isInput,
                                                                              indexOffset,
                                                                              fJackClient,
                                                                              jackPort,
                                                                              fThreadSafeMetadataMutex,
                                                                              this));
            fCVPorts.append(enginePort);
            return enginePort;
        }
        case kEnginePortTypeEvent: {
            pData->addEventPortName(isInput, realName);
            if (realName != name) delete[] realName;
            CarlaEngineJackEventPort* const enginePort(new CarlaEngineJackEventPort(*this,
                                                                                    isInput,
                                                                                    indexOffset,
                                                                                    fJackClient,
                                                                                    jackPort,
                                                                                    fThreadSafeMetadataMutex,
                                                                                    this));
            fEventPorts.append(enginePort);
            return enginePort;
        }
        }

        carla_stderr("CarlaEngineJackClient::addPort(%i, \"%s\", %s) - invalid type", portType, name, bool2str(isInput));
        return nullptr;
    }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CarlaEngineCVSourcePorts* createCVSourcePorts() override
    {
        fCVSourcePorts.setGraphAndPlugin(getPatchbayGraphOrNull(), getPlugin());
        return &fCVSourcePorts;
    }

    CarlaEngineJackCVSourcePorts& getCVSourcePorts() noexcept
    {
        return fCVSourcePorts;
    }
#endif

    void invalidate() noexcept
    {
        for (LinkedList<CarlaEngineJackAudioPort*>::Itenerator it = fAudioPorts.begin2(); it.valid(); it.next())
        {
            CarlaEngineJackAudioPort* const port(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(port != nullptr);

            port->invalidate();
        }

        for (LinkedList<CarlaEngineJackCVPort*>::Itenerator it = fCVPorts.begin2(); it.valid(); it.next())
        {
            CarlaEngineJackCVPort* const port(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(port != nullptr);

            port->invalidate();
        }

        for (LinkedList<CarlaEngineJackEventPort*>::Itenerator it = fEventPorts.begin2(); it.valid(); it.next())
        {
            CarlaEngineJackEventPort* const port(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(port != nullptr);

            port->invalidate();
        }

        fJackClient = nullptr;
        CarlaEngineClient::deactivate(true);
    }

    const char* getJackClientName() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fJackClient != nullptr, nullptr);

        try {
            return jackbridge_get_client_name(fJackClient);
        } CARLA_SAFE_EXCEPTION_RETURN("jack_get_client_name", nullptr);
    }

    void jackAudioPortDeleted(CarlaEngineJackAudioPort* const port) noexcept override
    {
        fAudioPorts.removeAll(port);
    }

    void jackCVPortDeleted(CarlaEngineJackCVPort* const port) noexcept override
    {
        fCVPorts.removeAll(port);
    }

    void jackEventPortDeleted(CarlaEngineJackEventPort* const port) noexcept override
    {
        fEventPorts.removeAll(port);
    }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    bool renameInSingleClient(const String& newClientName)
    {
        const String clientNamePrefix(newClientName + ":");

        return _renamePorts(fAudioPorts, clientNamePrefix) &&
               _renamePorts(fCVPorts,    clientNamePrefix) &&
               _renamePorts(fEventPorts, clientNamePrefix);
    }

    void closeForRename(jack_client_t* const newClient, const String& newClientName) noexcept
    {
        if (fJackClient != nullptr)
        {
            if (isActive())
            {
                {
                    const String clientNamePrefix(newClientName + ":");

                    // store current client connections
                    const CarlaMutexLocker cml(fPreRenameMutex);

                    fPreRenameConnections.clear();
                    fPreRenamePluginId.clear();
                    fPreRenamePluginIcon.clear();

                    _savePortsConnections(fAudioPorts, clientNamePrefix);
                    _savePortsConnections(fCVPorts,    clientNamePrefix);
                    _savePortsConnections(fEventPorts, clientNamePrefix);
                    _saveProperties();
                }

                try {
                    jackbridge_deactivate(fJackClient);
                } catch(...) {}
            }

            try {
                jackbridge_client_close(fJackClient);
            } catch(...) {}

            invalidate();
        }

        fAudioPorts.clear();
        fCVPorts.clear();
        fEventPorts.clear();
        pData->clearPorts();

        fJackClient = newClient;
    }

    void reservePluginPtr(CarlaPluginPtr* const pluginPtr)
    {
        fReservedPluginPtr = pluginPtr;
    }

    void setNewPluginId(const uint id) const
    {
        CARLA_SAFE_ASSERT_RETURN(fJackClient != nullptr,);

        // NOTE: no fThreadSafeMetadataMutex lock here, assumed done from caller

        if (char* const uuidstr = jackbridge_client_get_uuid(fJackClient))
        {
            jack_uuid_t uuid;

            if (jackbridge_uuid_parse(uuidstr, &uuid))
            {
                char buf[32];
                std::snprintf(buf, 31, "%u", id);
                buf[31] = '\0';
                jackbridge_set_property(fJackClient, uuid,
                                        URI_PLUGIN_ID,
                                        buf,
                                        URI_TYPE_INTEGER);
            }

            jackbridge_free(uuidstr);
        }
    }
#endif

private:
    jack_client_t* fJackClient;
    const bool     fUseClient;

    LinkedList<CarlaEngineJackAudioPort*> fAudioPorts;
    LinkedList<CarlaEngineJackCVPort*>    fCVPorts;
    LinkedList<CarlaEngineJackEventPort*> fEventPorts;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CarlaEngineJackCVSourcePorts fCVSourcePorts;

    CarlaMutex      fPreRenameMutex;
    CarlaStringList fPreRenameConnections;
    String fPreRenamePluginId;
    String fPreRenamePluginIcon;

    ScopedPointer<CarlaPluginPtr> fReservedPluginPtr;

    template<typename T>
    bool _renamePorts(const LinkedList<T*>& t, const String& clientNamePrefix)
    {
        for (typename LinkedList<T*>::Itenerator it = t.begin2(); it.valid(); it.next())
        {
            T* const port(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(port != nullptr);
            CARLA_SAFE_ASSERT_CONTINUE(port->fJackPort != nullptr);

            const char* shortPortName(jackbridge_port_short_name(port->fJackPort));
            CARLA_SAFE_ASSERT_CONTINUE(shortPortName != nullptr && shortPortName[0] != '\0');

            const char* const oldClientNameSep(std::strstr(shortPortName, ":"));
            CARLA_SAFE_ASSERT_CONTINUE(oldClientNameSep != nullptr && oldClientNameSep[0] != '\0' && oldClientNameSep[1] != '\0');

            shortPortName += oldClientNameSep-shortPortName + 1;

            const String newPortName(clientNamePrefix + shortPortName);

            if (! jackbridge_port_rename(fJackClient, port->fJackPort, newPortName))
                return false;
        }

        return true;
    }

    template<typename T>
    void _savePortsConnections(const LinkedList<T*>& t, const String& clientNamePrefix)
    {
        for (typename LinkedList<T*>::Itenerator it = t.begin2(); it.valid(); it.next())
        {
            T* const port(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(port != nullptr);
            CARLA_SAFE_ASSERT_CONTINUE(port->fJackPort != nullptr);

            const char* const shortPortName(jackbridge_port_short_name(port->fJackPort));
            CARLA_SAFE_ASSERT_CONTINUE(shortPortName != nullptr && shortPortName[0] != '\0');

            const String portName(clientNamePrefix + shortPortName);

            if (const char** const connections = jackbridge_port_get_all_connections(fJackClient, port->fJackPort))
            {
                for (int i=0; connections[i] != nullptr; ++i)
                {
                    if (port->kIsInput)
                    {
                        fPreRenameConnections.append(connections[i]);
                        fPreRenameConnections.append(portName);
                    }
                    else
                    {
                        fPreRenameConnections.append(portName);
                        fPreRenameConnections.append(connections[i]);
                    }
                }

                jackbridge_free(connections);
            }
        }
    }

    void _saveProperties()
    {
        const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

        if (char* const uuidstr = jackbridge_client_get_uuid(fJackClient))
        {
            jack_uuid_t uuid;

            const bool parsed = jackbridge_uuid_parse(uuidstr, &uuid);
            jackbridge_free(uuidstr);

            CARLA_CUSTOM_SAFE_ASSERT_ONCE_RETURN("JACK meta-data support unavailable", parsed,);

            char* value = nullptr;
            char* type = nullptr;

            CARLA_SAFE_ASSERT_RETURN(jackbridge_get_property(uuid,
                                                             URI_PLUGIN_ID,
                                                             &value,
                                                             &type),);
            CARLA_SAFE_ASSERT_RETURN(type != nullptr,);
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(type, URI_TYPE_INTEGER) == 0,);
            fPreRenamePluginId = value;

            jackbridge_free(value);
            jackbridge_free(type);
            value = type = nullptr;

            if (jackbridge_get_property(uuid, URI_PLUGIN_ICON, &value, &type))
            {
                CARLA_SAFE_ASSERT_RETURN(type != nullptr,);
                CARLA_SAFE_ASSERT_RETURN(std::strcmp(type, URI_TYPE_STRING) == 0,);
                fPreRenamePluginIcon = value;

                jackbridge_free(value);
                jackbridge_free(type);
            }
        }
    }
#endif // BUILD_BRIDGE_ALTERNATIVE_ARCH

    CarlaRecursiveMutex& fThreadSafeMetadataMutex;
    const String& fMainClientName;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackClient)
};

// -----------------------------------------------------------------------
// Jack Engine

class CarlaEngineJack : public CarlaEngine
#ifndef BUILD_BRIDGE
                      , private CarlaThread
#endif
{
public:
    CarlaEngineJack()
        : CarlaEngine(),
#ifndef BUILD_BRIDGE
          CarlaThread("CarlaEngineJackCallbacks"),
#endif
          fClient(nullptr),
          fExternalPatchbayHost(true),
          fExternalPatchbayOsc(true),
          fFreewheel(false),
          fClientName(),
          fThreadSafeMetadataMutex(),
#ifdef BUILD_BRIDGE
          fIsRunning(false)
#else
          fClientNamePrefix(),
          fTimebaseMaster(false),
          fTimebaseRolling(false),
          fTimebaseUsecs(0),
          fUsedGroups(),
          fUsedPorts(),
          fUsedConnections(),
          fPatchbayProcThreadProtectionMutex(),
          fRetConns(),
          fLastPatchbaySetGroupPos(),
          fPostPonedEvents(),
          fPostPonedEventsMutex(),
          fIsInternalClient(false)
#endif
    {
        carla_debug("CarlaEngineJack::CarlaEngineJack()");

#ifdef BUILD_BRIDGE
        pData->options.processMode = ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS;
#else
        carla_zeroPointers(fRackPorts, kRackPortCount);
#endif
    }

    ~CarlaEngineJack() noexcept override
    {
        carla_debug("CarlaEngineJack::~CarlaEngineJack()");
        CARLA_SAFE_ASSERT(fClient == nullptr);

#ifndef BUILD_BRIDGE
        fUsedGroups.clear();
        fUsedPorts.clear();
        fUsedConnections.clear();
        CARLA_SAFE_ASSERT(fPostPonedEvents.count() == 0);
#endif
    }

    // -------------------------------------------------------------------
    // Maximum values

    uint getMaxClientNameSize() const noexcept override
    {
#ifndef BUILD_BRIDGE
        if (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT ||
            pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
        {
            try {
                return static_cast<uint>(jackbridge_client_name_size()-1);
            } CARLA_SAFE_EXCEPTION_RETURN("jack_client_name_size", 32);
        }

        return CarlaEngine::getMaxClientNameSize();
    }

    uint getMaxPortNameSize() const noexcept override
    {
        if (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT || pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            try {
                return static_cast<uint>(jackbridge_port_name_size()-1);
            } CARLA_SAFE_EXCEPTION_RETURN("jack_port_name_size", 255);
        }

        return CarlaEngine::getMaxPortNameSize();
    }

    // -------------------------------------------------------------------
    // Virtual, per-engine type calls

    bool init(const char* const clientName) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr || (clientName != nullptr && clientName[0] != '\0'), false);
        carla_debug("CarlaEngineJack::init(\"%s\")", clientName);

        if (!jackbridge_is_ok())
        {
            setLastError("JACK is not available or installed");
            return false;
        }

        fFreewheel = false;
        fExternalPatchbayHost = true;
        fExternalPatchbayOsc  = true;

        String truncatedClientName;

        if (fClient == nullptr && clientName != nullptr)
        {
            truncatedClientName = clientName;
            truncatedClientName.truncate(getMaxClientNameSize());
        }

#ifdef BUILD_BRIDGE
        fIsRunning = true;

        if (! pData->init(truncatedClientName))
        {
            close();
            setLastError("Failed to init internal data");
            return false;
        }

        if (pData->bufferSize == 0 || carla_isEqual(pData->sampleRate, 0.0))
        {
            // open temp client to get initial buffer-size and sample-rate values
            if (jack_client_t* const tmpClient = jackbridge_client_open(truncatedClientName, JackNoStartServer, nullptr))
            {
                pData->bufferSize = jackbridge_get_buffer_size(tmpClient);
                pData->sampleRate = jackbridge_get_sample_rate(tmpClient);

                jackbridge_client_close(tmpClient);
            }
            else
            {
                close();
                setLastError("Failed to init temporary jack client");
                return false;
            }
        }

        return true;

#else // BUILD_BRIDGE
        if (fClient == nullptr && clientName != nullptr)
            fClient = jackbridge_client_open(truncatedClientName, JackNoStartServer, nullptr);

        if (fClient == nullptr)
        {
            setLastError("Failed to create new JACK client");
            return false;
        }

        const char* const jackClientName = jackbridge_get_client_name(fClient);

        if (! pData->init(jackClientName))
        {
            jackbridge_client_close(fClient);
            fClient = nullptr;
            setLastError("Failed to init internal data");
            return false;
        }

        // if main jack client does not match our requested one, setup client name prefix as needed
        if (truncatedClientName != jackClientName)
        {
            if (const char* const suffix = std::strrchr(jackClientName, '-'))
            {
                if (fClientNamePrefix.isNotEmpty())
                {
                    fClientNamePrefix.truncate(fClientNamePrefix.rfind('.') + 1);
                }
                else
                {
                    fClientNamePrefix = truncatedClientName;
                    fClientNamePrefix += ".";
                }

                fClientNamePrefix += suffix + 1;
                fClientNamePrefix += "/";
            }
        }

        fClientName = jackClientName;

        const EngineOptions& opts(pData->options);

        pData->bufferSize = jackbridge_get_buffer_size(fClient);
        pData->sampleRate = jackbridge_get_sample_rate(fClient);
        pData->initTime(opts.transportExtra);

        jackbridge_set_thread_init_callback(fClient, carla_jack_thread_init_callback, nullptr);
        jackbridge_set_buffer_size_callback(fClient, carla_jack_bufsize_callback, this);
        jackbridge_set_sample_rate_callback(fClient, carla_jack_srate_callback, this);
        jackbridge_set_freewheel_callback(fClient, carla_jack_freewheel_callback, this);
        // jackbridge_set_latency_callback(fClient, carla_jack_latency_callback, this);
        jackbridge_set_process_callback(fClient, carla_jack_process_callback, this);
        jackbridge_on_shutdown(fClient, carla_jack_shutdown_callback, this);

        fTimebaseRolling = false;

        if (opts.transportMode == ENGINE_TRANSPORT_MODE_JACK)
            fTimebaseMaster = jackbridge_set_timebase_callback(fClient, true, carla_jack_timebase_callback, this);
        else
            fTimebaseMaster = false;

        initJackPatchbay(true, false, jackClientName, pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY);

        jackbridge_set_client_registration_callback(fClient, carla_jack_client_registration_callback, this);
        jackbridge_set_port_registration_callback(fClient, carla_jack_port_registration_callback, this);
        jackbridge_set_port_connect_callback(fClient, carla_jack_port_connect_callback, this);
        jackbridge_set_port_rename_callback(fClient, carla_jack_port_rename_callback, this);
        jackbridge_set_property_change_callback(fClient, carla_jack_property_change_callback, this);
        jackbridge_set_xrun_callback(fClient, carla_jack_xrun_callback, this);

        if (opts.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || opts.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        {
            fRackPorts[kRackPortAudioIn1]  = jackbridge_port_register(fClient, "audio-in1",  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            fRackPorts[kRackPortAudioIn2]  = jackbridge_port_register(fClient, "audio-in2",  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            fRackPorts[kRackPortAudioOut1] = jackbridge_port_register(fClient, "audio-out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
            fRackPorts[kRackPortAudioOut2] = jackbridge_port_register(fClient, "audio-out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
            fRackPorts[kRackPortEventIn]   = jackbridge_port_register(fClient, "events-in",  JACK_DEFAULT_MIDI_TYPE,  JackPortIsInput, 0);
            fRackPorts[kRackPortEventOut]  = jackbridge_port_register(fClient, "events-out", JACK_DEFAULT_MIDI_TYPE,  JackPortIsOutput, 0);

            if (opts.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
            {
                // FIXME?
                pData->graph.create(0, 0, 0, 0);
            }
            else
            {
                pData->graph.create(2, 2, 0, 0);
                // pData->graph.setUsingExternalHost(true);
                // pData->graph.setUsingExternalOSC(true);
                patchbayRefresh(true, false, false);
            }
        }

# ifdef HAVE_LIBLO
        {
            const String& tcp(pData->osc.getServerPathTCP());
            const String& udp(pData->osc.getServerPathUDP());

            if (tcp.isNotEmpty() || udp.isNotEmpty())
            {
                const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

                if (char* const uuidstr = jackbridge_client_get_uuid(fClient))
                {
                    jack_uuid_t uuid;

                    if (jackbridge_uuid_parse(uuidstr, &uuid))
                    {
                        if (tcp.isNotEmpty())
                            jackbridge_set_property(fClient, uuid,
                                                    "https://kx.studio/ns/carla/osc-tcp", tcp, URI_TYPE_STRING);

                        if (udp.isNotEmpty())
                            jackbridge_set_property(fClient, uuid,
                                                    "https://kx.studio/ns/carla/osc-udp", udp, URI_TYPE_STRING);
                    }

                    jackbridge_free(uuidstr);
                }
            }
        }
# endif

        if (jackbridge_activate(fClient))
        {
            if (opts.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK ||
                opts.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
            {
                if (pData->options.audioDevice != nullptr &&
                    std::strcmp(pData->options.audioDevice, "Auto-Connect ON") == 0 &&
                    std::getenv("LADISH_APP_NAME") == nullptr &&
                    std::getenv("NSM_URL") == nullptr)
                {
                    char strBuf[STR_MAX];

                    if (jackbridge_port_by_name(fClient, "system:capture_1") != nullptr)
                    {
                        std::snprintf(strBuf, STR_MAX, "%s:audio-in1", jackClientName);
                        strBuf[STR_MAX-1] = '\0';

                        jackbridge_connect(fClient, "system:capture_1", strBuf);

                        std::snprintf(strBuf, STR_MAX, "%s:audio-in2", jackClientName);
                        strBuf[STR_MAX-1] = '\0';

                        if (jackbridge_port_by_name(fClient, "system:capture_2") != nullptr)
                            jackbridge_connect(fClient, "system:capture_2", strBuf);
                        else
                            jackbridge_connect(fClient, "system:capture_1", strBuf);
                    }

                    if (jackbridge_port_by_name(fClient, "system:playback_1") != nullptr)
                    {
                        std::snprintf(strBuf, STR_MAX, "%s:audio-out1", jackClientName);
                        strBuf[STR_MAX-1] = '\0';

                        jackbridge_connect(fClient, strBuf, "system:playback_1");

                        std::snprintf(strBuf, STR_MAX, "%s:audio-out2", jackClientName);
                        strBuf[STR_MAX-1] = '\0';

                        if (jackbridge_port_by_name(fClient, "system:playback_2") != nullptr)
                            jackbridge_connect(fClient, strBuf, "system:playback_2");
                        else
                            jackbridge_connect(fClient, strBuf, "system:playback_1");
                    }
                }
            }

            if (fIsInternalClient)
                startThread();

            callback(true, true,
                     ENGINE_CALLBACK_ENGINE_STARTED, 0,
                     opts.processMode,
                     opts.transportMode,
                     static_cast<int>(pData->bufferSize),
                     static_cast<float>(pData->sampleRate),
                     getCurrentDriverName());
            return true;
        }

        if (opts.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK ||
            opts.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        {
            pData->graph.destroy();
        }

        pData->close();
        jackbridge_client_close(fClient);
        fClient = nullptr;

        setLastError("Failed to activate the JACK client");
        return false;
#endif // BUILD_BRIDGE
    }

#ifndef BUILD_BRIDGE
    bool initInternal(jack_client_t* const client)
    {
        fClient = client;
        fIsInternalClient = true;

        return init(nullptr);
    }
#endif

    bool close() override
    {
        carla_debug("CarlaEngineJack::close()");

#ifdef BUILD_BRIDGE
        fClient    = nullptr;
        fIsRunning = false;
        CarlaEngine::close();
        return true;
#else
        if (fIsInternalClient)
            stopThread(-1);

        // deactivate client ASAP
        if (fClient != nullptr)
            jackbridge_deactivate(fClient);

        // clear engine data
        CarlaEngine::close();

        // now close client
        if (fClient != nullptr)
        {
            jackbridge_client_close(fClient);
            fClient = nullptr;
        }

        fClientName.clear();
        fUsedGroups.clear();
        fUsedPorts.clear();
        fUsedConnections.clear();
        fPostPonedEvents.clear();

        // clear rack/patchbay stuff
        if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK ||
            pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        {
            carla_zeroPointers(fRackPorts, kRackPortCount);
            pData->graph.destroy();
        }

        return true;
#endif
    }

    bool hasIdleOnMainThread() const noexcept override
    {
       #ifndef BUILD_BRIDGE
        return !fIsInternalClient;
       #else
        return true;
       #endif
    }

    bool isRunning() const noexcept override
    {
#ifdef BUILD_BRIDGE
        return (fClient != nullptr || fIsRunning);
#else
        return (fClient != nullptr);
#endif
    }

    bool isOffline() const noexcept override
    {
        return fFreewheel;
    }

    EngineType getType() const noexcept override
    {
        return kEngineTypeJack;
    }

    const char* getCurrentDriverName() const noexcept override
    {
        return "JACK";
    }

#ifndef BUILD_BRIDGE
    float getDSPLoad() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, 0.0f);

        return jackbridge_cpu_load(fClient);
    }

    void callback(const bool sendHost, const bool sendOsc,
                  const EngineCallbackOpcode action, const uint pluginId,
                  const int value1, const int value2, const int value3,
                  const float valuef, const char* const valueStr) noexcept override
    {
        if (action == ENGINE_CALLBACK_PROJECT_LOAD_FINISHED)
        {
            if (fTimebaseMaster)
            {
                // project finished loading, need to set bpm here, so we force an update of timebase master
                transportRelocate(pData->timeInfo.frame);
            }
        }

        CarlaEngine::callback(sendHost, sendOsc, action, pluginId, value1, value2, value3, valuef, valueStr);
    }

    void idle() noexcept override
    {
        LinkedList<PostPonedJackEvent> events;
        const PostPonedJackEvent nullEvent = {};

        {
            const CarlaMutexLocker cml(fPostPonedEventsMutex);

            if (fPostPonedEvents.count() > 0)
                fPostPonedEvents.moveTo(events);
        }

        for (LinkedList<PostPonedJackEvent>::Itenerator it = events.begin2(); it.valid(); it.next())
        {
            const PostPonedJackEvent& ev(it.getValue(nullEvent));
            CARLA_SAFE_ASSERT_CONTINUE(ev.type != PostPonedJackEvent::kTypeNull);

            switch (ev.type)
            {
            case PostPonedJackEvent::kTypeNull:
                break;
            case PostPonedJackEvent::kTypeClientUnregister:
                handleJackClientUnregistrationCallback(ev.clientUnregister.name);
                break;
            case PostPonedJackEvent::kTypePortRegister:
                handleJackPortRegistrationCallback(ev.portRegister.fullName,
                                                   ev.portRegister.shortName,
                                                   ev.portRegister.hints);
                break;
            case PostPonedJackEvent::kTypePortUnregister:
                handleJackPortUnregistrationCallback(ev.portUnregister.fullName);
                break;
            case PostPonedJackEvent::kTypePortConnect:
                handleJackPortConnectCallback(ev.portConnect.portNameA,
                                              ev.portConnect.portNameB);
                break;
            case PostPonedJackEvent::kTypePortDisconnect:
                handleJackPortDisconnectCallback(ev.portDisconnect.portNameA,
                                                 ev.portDisconnect.portNameB);
                break;
            case PostPonedJackEvent::kTypePortRename:
                handleJackPortRenameCallback(ev.portRename.oldFullName,
                                             ev.portRename.newFullName,
                                             ev.portRename.newShortName);
                break;
            case PostPonedJackEvent::kTypeClientPositionChange:
                handleJackClientPositionChangeCallback(ev.clientPositionChange.uuid);
                break;
            }
        }

        events.clear();

        CarlaEngine::idle();
    }
#endif

    bool setBufferSizeAndSampleRate(const uint bufferSize, const double sampleRate) override
    {
        CARLA_SAFE_ASSERT_RETURN(carla_isEqual(pData->sampleRate, sampleRate), false);
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, false);

        try {
            return jackbridge_set_buffer_size(fClient, bufferSize);
        } CARLA_SAFE_EXCEPTION_RETURN("setBufferSizeAndSampleRate", false);
    }

    EngineTimeInfo getTimeInfo() const noexcept override
    {
        if (pData->options.transportMode != ENGINE_TRANSPORT_MODE_JACK)
            return CarlaEngine::getTimeInfo();
        if (pData->options.processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
            return CarlaEngine::getTimeInfo();

        jack_position_t jpos;

        // invalidate
        jpos.unique_1 = 1;
        jpos.unique_2 = 2;

        EngineTimeInfo timeInfo;
        const bool playing = jackbridge_transport_query(fClient, &jpos) == JackTransportRolling;

        if (jpos.unique_1 != jpos.unique_2)
        {
            timeInfo.playing = false;
            timeInfo.frame = 0;
            timeInfo.usecs = 0;
            timeInfo.bbt.valid = false;
            return timeInfo;
        }

        timeInfo.playing = playing;
        timeInfo.frame   = jpos.frame;
        timeInfo.usecs   = jpos.usecs;

        if (jpos.valid & JackPositionBBT)
        {
            timeInfo.bbt.valid          = true;
            timeInfo.bbt.bar            = jpos.bar;
            timeInfo.bbt.beat           = jpos.beat;
#ifdef JACK_TICK_DOUBLE
            if (jpos.valid & JackTickDouble)
                timeInfo.bbt.tick       = jpos.tick_double;
            else
#endif
                timeInfo.bbt.tick       = jpos.tick;
            timeInfo.bbt.barStartTick   = jpos.bar_start_tick;
            timeInfo.bbt.beatsPerBar    = jpos.beats_per_bar;
            timeInfo.bbt.beatType       = jpos.beat_type;
            timeInfo.bbt.ticksPerBeat   = jpos.ticks_per_beat;
            timeInfo.bbt.beatsPerMinute = jpos.beats_per_minute;
        }
        else
        {
            timeInfo.bbt.valid = false;
        }

        return timeInfo;
    }

#ifndef BUILD_BRIDGE
    void setOption(const EngineOption option, const int value, const char* const valueStr) noexcept override
    {
        if (option == ENGINE_OPTION_TRANSPORT_MODE && fClient != nullptr)
        {
            CARLA_SAFE_ASSERT_INT_RETURN(value >= ENGINE_TRANSPORT_MODE_DISABLED &&
                                          value <= ENGINE_TRANSPORT_MODE_JACK, value,);

            if (value == ENGINE_TRANSPORT_MODE_JACK)
            {
                fTimebaseMaster = jackbridge_set_timebase_callback(fClient, true, carla_jack_timebase_callback, this);
            }
            else
            {
                // jack transport cannot be disabled in multi-client
                callback(true, true,
                         ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED, 0,
                         ENGINE_TRANSPORT_MODE_JACK,
                         0, 0, 0.0f,
                         pData->options.transportExtra);
                CARLA_SAFE_ASSERT_RETURN(pData->options.processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS,);

                jackbridge_release_timebase(fClient);
                fTimebaseMaster = false;
            }
        }
        else if (option == ENGINE_OPTION_CLIENT_NAME_PREFIX)
        {
            fClientNamePrefix = valueStr;

            if (fClientNamePrefix.isNotEmpty())
            {
                if (! fClientNamePrefix.contains('.'))
                    fClientNamePrefix += ".0";
                if (! fClientNamePrefix.endsWith('/'))
                    fClientNamePrefix += "/";
            }
        }

        CarlaEngine::setOption(option, value, valueStr);
    }
#endif

    CarlaEngineClient* addClient(CarlaPluginPtr plugin) override
    {
        jack_client_t* client = nullptr;

#ifndef BUILD_BRIDGE
        CARLA_CUSTOM_SAFE_ASSERT_RETURN("Not connected to JACK", fClient != nullptr, nullptr);

        CarlaPluginPtr* pluginReserve = nullptr;

        if (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
        {
            client = fClient;
        }
        else if (pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
        {
#ifndef BUILD_BRIDGE
            if (fClientNamePrefix.isNotEmpty())
            {
                client = jackbridge_client_open(fClientNamePrefix + plugin->getName(), JackNoStartServer, nullptr);
            }
            else
#endif
            {
                client = jackbridge_client_open(plugin->getName(), JackNoStartServer, nullptr);
            }
            CARLA_CUSTOM_SAFE_ASSERT_RETURN("Failure to open client", client != nullptr, nullptr);

            jackbridge_set_thread_init_callback(client, carla_jack_thread_init_callback, nullptr);

            const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

            if (char* const uuidstr = jackbridge_client_get_uuid(client))
            {
                jack_uuid_t uuid;

                if (jackbridge_uuid_parse(uuidstr, &uuid))
                {
                    char strBufId[24];
                    std::snprintf(strBufId, 23, "%u", plugin->getId());
                    strBufId[23] = '\0';

#ifndef BUILD_BRIDGE
                    jackbridge_set_property(client, uuid,
                                            URI_MAIN_CLIENT_NAME,
                                            fClientName,
                                            URI_TYPE_STRING);

                    jackbridge_set_property(client, uuid,
                                            URI_PLUGIN_ID,
                                            strBufId,
                                            URI_TYPE_INTEGER);
#endif

                    if (const char* const pluginIcon = plugin->getIconName())
                        jackbridge_set_property(client, uuid,
                                                URI_PLUGIN_ICON,
                                                pluginIcon,
                                                URI_TYPE_STRING);
                }

                jackbridge_free(uuidstr);
            }

#ifndef BUILD_BRIDGE
            pluginReserve = new CarlaPluginPtr(plugin);
            /*
            jackbridge_set_buffer_size_callback(fClient, carla_jack_bufsize_callback_plugin, pluginReserve);
            jackbridge_set_sample_rate_callback(fClient, carla_jack_srate_callback_plugin, pluginReserve);
            */
            // jackbridge_set_latency_callback(client, carla_jack_latency_callback_plugin, pluginReserve);
            jackbridge_set_process_callback(client, carla_jack_process_callback_plugin, pluginReserve);
            jackbridge_on_shutdown(client, carla_jack_shutdown_callback_plugin, pluginReserve);
#else
            fClient = client;
            pData->bufferSize = jackbridge_get_buffer_size(client);
            pData->sampleRate = jackbridge_get_sample_rate(client);
            pData->initTime(nullptr);

            jackbridge_set_buffer_size_callback(client, carla_jack_bufsize_callback, this);
            jackbridge_set_sample_rate_callback(client, carla_jack_srate_callback, this);
            jackbridge_set_freewheel_callback(client, carla_jack_freewheel_callback, this);
            // jackbridge_set_latency_callback(client, carla_jack_latency_callback, this);
            jackbridge_set_process_callback(client, carla_jack_process_callback, this);
            jackbridge_on_shutdown(client, carla_jack_shutdown_callback, this);

            fClientName = jackbridge_get_client_name(client);
#endif
        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        carla_debug("new CarlaEngineJackClient");
        CarlaEngineJackClient* const jclient = new CarlaEngineJackClient(*this,
                                                                         pData->graph,
                                                                         fThreadSafeMetadataMutex,
                                                                         plugin, fClientName, client);
# ifndef BUILD_BRIDGE
        if (pluginReserve != nullptr)
            jclient->reservePluginPtr(pluginReserve);
# endif
        return jclient;

#else
        return new CarlaEngineJackClient(*this, fThreadSafeMetadataMutex, fClientName, client);
#endif
    }

#ifndef BUILD_BRIDGE
    bool removePlugin(const uint id) override
    {
        if (! CarlaEngine::removePlugin(id))
            return false;

        if (pData->options.processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
            return true;

        const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

        for (uint i=id; i < pData->curPluginCount; ++i)
        {
            const CarlaPluginPtr plugin = pData->plugins[i].plugin;
            CARLA_SAFE_ASSERT_BREAK(plugin.get() != nullptr);

            CarlaEngineJackClient* const client = dynamic_cast<CarlaEngineJackClient*>(plugin->getEngineClient());
            CARLA_SAFE_ASSERT_BREAK(client != nullptr);

            client->setNewPluginId(i);
        }

        return true;
    }

    bool switchPlugins(const uint idA, const uint idB) noexcept override
    {
        if (! CarlaEngine::switchPlugins(idA, idB))
            return false;

        if (pData->options.processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
            return true;

        CarlaPluginPtr newPluginA = pData->plugins[idA].plugin;
        CARLA_SAFE_ASSERT_RETURN(newPluginA.get() != nullptr, true);

        CarlaPluginPtr newPluginB = pData->plugins[idB].plugin;
        CARLA_SAFE_ASSERT_RETURN(newPluginB.get() != nullptr, true);

        CarlaEngineJackClient* const clientA = dynamic_cast<CarlaEngineJackClient*>(newPluginA->getEngineClient());
        CARLA_SAFE_ASSERT_RETURN(clientA != nullptr, true);

        CarlaEngineJackClient* const clientB = dynamic_cast<CarlaEngineJackClient*>(newPluginB->getEngineClient());
        CARLA_SAFE_ASSERT_RETURN(clientB != nullptr, true);

        {
            const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);
            clientA->setNewPluginId(idA);
            clientB->setNewPluginId(idB);
        }

        return true;
    }

    bool renamePlugin(const uint id, const char* const newName) override
    {
        if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK ||
            pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        {
            return CarlaEngine::renamePlugin(id, newName);
        }

        CARLA_SAFE_ASSERT_RETURN(pData->plugins != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(pData->curPluginCount != 0, false);
        CARLA_SAFE_ASSERT_RETURN(id < pData->curPluginCount, false);
        CARLA_SAFE_ASSERT_RETURN(newName != nullptr && newName[0] != '\0', false);

        CarlaPluginPtr plugin = pData->plugins[id].plugin;
        CARLA_SAFE_ASSERT_RETURN_ERR(plugin.get() != nullptr, "Could not find plugin to rename");
        CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data");

        // before we stop the engine thread we might need to get the plugin data
        const bool needsReinit = (pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS);
        const CarlaStateSave* saveStatePtr = nullptr;

        if (needsReinit)
        {
            const CarlaStateSave& saveState(plugin->getStateSave());
            saveStatePtr = &saveState;
        }

        String uniqueName;

        try {
            const char* const tmpName = getUniquePluginName(newName);
            uniqueName = tmpName;
            delete[] tmpName;
        } CARLA_SAFE_EXCEPTION("JACK renamePlugin getUniquePluginName");

        if (uniqueName.isEmpty())
        {
            setLastError("Failed to request new unique plugin name");
            return false;
        }

        const ScopedRunnerStopper srs(this);

        // rename on client client mode, just rename the ports
        if (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
        {
            CarlaEngineJackClient* const client((CarlaEngineJackClient*)plugin->getEngineClient());

            if (! client->renameInSingleClient(uniqueName))
            {
                setLastError("Failed to rename some JACK ports, does your JACK version support proper port renaming?");
                return false;
            }
        }
        // rename in multiple client mode
        else if (pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CarlaEngineJackClient* const client((CarlaEngineJackClient*)plugin->getEngineClient());

            // we should not be able to do this, jack really needs to allow client rename
            if (jack_client_t* const jackClient = jackbridge_client_open(fClientNamePrefix + uniqueName,
                                                                         JackNoStartServer, nullptr))
            {
                // get new client name
                fClientName = jackbridge_get_client_name(jackClient);

                if (fClientNamePrefix.isNotEmpty())
                    uniqueName = fClientName.buffer() + fClientNamePrefix.length();
                else
                    uniqueName = fClientName;

                // close client
                client->closeForRename(jackClient, fClientName);

                // disable plugin
                plugin->setEnabled(false);

                // set new client data
                CarlaPluginPtr* const pluginReserve = new CarlaPluginPtr(plugin);
                client->reservePluginPtr(pluginReserve);
                // jackbridge_set_latency_callback(jackClient, carla_jack_latency_callback_plugin, pluginReserve);
                jackbridge_set_process_callback(jackClient, carla_jack_process_callback_plugin, pluginReserve);
                jackbridge_on_shutdown(jackClient, carla_jack_shutdown_callback_plugin, pluginReserve);

                // NOTE: jack1 locks up here
                if (jackbridge_get_version_string() != nullptr)
                    jackbridge_set_thread_init_callback(jackClient, carla_jack_thread_init_callback, nullptr);

                /* The following code is because of a tricky situation.
                   We cannot lock or do jack operations during jack callbacks on jack1. jack2 events are asynchronous.
                   When we close the client jack will trigger unregister-port callbacks, which we handle on a separate thread ASAP.
                   But before that happens we already registered a new client with the same ports (the "renamed" one),
                    and at this point the port we receive during that callback is actually the new one from the new client..
                   JACK2 seems to be reusing ports to save space, which is understandable.
                   Anyway, this means we have to remove all our port-related data before the new client ports are created.
                   (we also stop the separate jack-events thread to avoid any race conditions while modying our port data) */

                if (fIsInternalClient)
                    stopThread(-1);

                LinkedList<PortNameToId> ports;
                LinkedList<ConnectionToId> conns;

                {
                    const CarlaMutexLocker cml1(fUsedGroups.mutex);

                    if (const uint groupId = fUsedGroups.getGroupId(plugin->getName()))
                    {
                        const CarlaMutexLocker cml2(fUsedPorts.mutex);

                        for (LinkedList<PortNameToId>::Itenerator it = fUsedPorts.list.begin2(); it.valid(); it.next())
                        {
                            const PortNameToId& portNameToId(it.getValue(kPortNameToIdFallback));
                            CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group != 0);

                            if (portNameToId.group != groupId)
                                continue;

                            ports.append(portNameToId);
                            fUsedPorts.list.remove(it);
                        }

                        const CarlaMutexLocker cml3(fUsedConnections.mutex);

                        for (LinkedList<ConnectionToId>::Itenerator it = fUsedConnections.list.begin2(); it.valid(); it.next())
                        {
                            const ConnectionToId& connectionToId = it.getValue(kConnectionToIdFallback);
                            CARLA_SAFE_ASSERT_CONTINUE(connectionToId.id != 0);

                            if (connectionToId.groupA != groupId && connectionToId.groupB != groupId)
                                continue;

                            conns.append(connectionToId);
                            fUsedConnections.list.remove(it);
                        }
                    }
                }

                for (LinkedList<ConnectionToId>::Itenerator it = conns.begin2(); it.valid(); it.next())
                {
                    const ConnectionToId& connectionToId = it.getValue(kConnectionToIdFallback);
                    CARLA_SAFE_ASSERT_CONTINUE(connectionToId.id != 0);

                    callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                              ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED,
                              connectionToId.id,
                              0, 0, 0, 0.0f, nullptr);
                }

                for (LinkedList<PortNameToId>::Itenerator it = ports.begin2(); it.valid(); it.next())
                {
                    const PortNameToId& portNameToId(it.getValue(kPortNameToIdFallback));
                    CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group != 0);

                    callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                              ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED,
                              portNameToId.group,
                              static_cast<int>(portNameToId.port),
                              0, 0, 0.0f, nullptr);
                }

                ports.clear();
                conns.clear();

                if (fIsInternalClient)
                    startThread();
            }
            else
            {
                setLastError("Failed to create new JACK client");
                return false;
            }
        }

        // Rename
        plugin->setName(uniqueName);

        if (needsReinit)
        {
            // reload plugin to recreate its ports
            plugin->reload();
            plugin->loadStateSave(*saveStatePtr);
            plugin->setEnabled(true);
        }

        callback(true, true, ENGINE_CALLBACK_PLUGIN_RENAMED, id, 0, 0, 0, 0.0f, uniqueName);
        return true;
    }

    // -------------------------------------------------------------------
    // Patchbay

    bool patchbayConnect(const bool external,
                         const uint groupA, const uint portA, const uint groupB, const uint portB) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, false);

        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY && ! external)
            return CarlaEngine::patchbayConnect(false, groupA, portA, groupB, portB);

        const CarlaMutexLocker cml(fUsedPorts.mutex);

        const char* const fullPortNameA = fUsedPorts.getFullPortName(groupA, portA);
        CARLA_SAFE_ASSERT_RETURN(fullPortNameA != nullptr && fullPortNameA[0] != '\0', false);

        const char* const fullPortNameB = fUsedPorts.getFullPortName(groupB, portB);
        CARLA_SAFE_ASSERT_RETURN(fullPortNameB != nullptr && fullPortNameB[0] != '\0', false);

        if (! jackbridge_connect(fClient, fullPortNameA, fullPortNameB))
        {
            setLastError("JACK operation failed");
            return false;
        }

        return true;
    }

    bool patchbayDisconnect(const bool external, const uint connectionId) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, false);

        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY && ! external)
            return CarlaEngine::patchbayDisconnect(false, connectionId);

        ConnectionToId connectionToId = { 0, 0, 0, 0, 0 };

        {
            const CarlaMutexLocker cml(fUsedConnections.mutex);

            for (LinkedList<ConnectionToId>::Itenerator it = fUsedConnections.list.begin2(); it.valid(); it.next())
            {
                connectionToId = it.getValue(kConnectionToIdFallback);
                CARLA_SAFE_ASSERT_CONTINUE(connectionToId.id != 0);

                if (connectionToId.id == connectionId)
                    break;
            }
        }

        if (connectionToId.id == 0 || connectionToId.id != connectionId)
        {
            setLastError("Failed to find the requested connection");
            return false;
        }

        const CarlaMutexLocker cml(fUsedPorts.mutex);

        const char* const fullPortNameA = fUsedPorts.getFullPortName(connectionToId.groupA, connectionToId.portA);
        CARLA_SAFE_ASSERT_RETURN(fullPortNameA != nullptr && fullPortNameA[0] != '\0', false);

        const char* const fullPortNameB = fUsedPorts.getFullPortName(connectionToId.groupB, connectionToId.portB);
        CARLA_SAFE_ASSERT_RETURN(fullPortNameB != nullptr && fullPortNameB[0] != '\0', false);

        if (! jackbridge_disconnect(fClient, fullPortNameA, fullPortNameB))
        {
            setLastError("JACK operation failed");
            return false;
        }

        return true;
    }

    bool patchbaySetGroupPos(const bool sendHost, const bool sendOSC, const bool external,
                             const uint groupId, const int x1, const int y1, const int x2, const int y2) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(! pData->loadingProject, false);
        carla_debug("CarlaEngineJack::patchbaySetGroupPos(%u, %i, %i, %i, %i)", groupId, x1, y1, x2, y2);

        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY && ! external)
            return CarlaEngine::patchbaySetGroupPos(sendHost, sendOSC, false, groupId, x1, y1, x2, y2);

        const char* groupName;

        {
            const CarlaMutexLocker cml(fUsedGroups.mutex);

            groupName = fUsedGroups.getGroupName(groupId);
            CARLA_SAFE_ASSERT_RETURN(groupName != nullptr && groupName[0] != '\0', false);
        }

        bool ok;

        {
            const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

            jack_uuid_t uuid;
            {
                char* const uuidstr = jackbridge_get_uuid_for_client_name(fClient, groupName);
                CARLA_SAFE_ASSERT_RETURN(uuidstr != nullptr && uuidstr[0] != '\0', false);

                const bool parsed = jackbridge_uuid_parse(uuidstr, &uuid);
                jackbridge_free(uuidstr);

                /* if parsing fails, meta-data is not available..
                   this could be because JACK version is old, or perhaps this is an internal client */
                if (! parsed)
                    return false;
            }

            fLastPatchbaySetGroupPos.set(x1, y1, x2, y2);

            char valueStr[STR_MAX];
            std::snprintf(valueStr, STR_MAX-1, "%i:%i:%i:%i", x1, y1, x2, y2);
            valueStr[STR_MAX-1] = '\0';

            ok = jackbridge_set_property(fClient, uuid, URI_POSITION, valueStr, URI_TYPE_STRING);
        }

        callback(sendHost, sendOSC,
                 ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED,
                 groupId, x1, y1, x2, static_cast<float>(y2),
                 nullptr);

        return ok;
    }

    bool patchbayRefresh(const bool sendHost, const bool sendOSC, const bool external) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, false);
        carla_debug("patchbayRefresh(%s, %s, %s)", bool2str(sendHost), bool2str(sendOSC), bool2str(external));

        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        {
            if (sendHost)
            {
                fExternalPatchbayHost = external;
                pData->graph.setUsingExternalHost(external);
            }
            if (sendOSC)
            {
                fExternalPatchbayOsc = external;
                pData->graph.setUsingExternalOSC(external);
            }

            if (! external)
                CarlaEngine::patchbayRefresh(sendHost, sendOSC, false);
        }

        initJackPatchbay(sendHost, sendOSC, jackbridge_get_client_name(fClient),
                         pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY && !external);
        return true;
    }

    // -------------------------------------------------------------------
    // Transport

    void transportPlay() noexcept override
    {
        if (pData->options.transportMode != ENGINE_TRANSPORT_MODE_JACK)
            return CarlaEngine::transportPlay();

        if (fClient != nullptr)
        {
            if (! pData->timeInfo.bbt.valid)
            {
                // old timebase master no longer active, make ourselves master again
                pData->time.setNeedsReset();
                fTimebaseMaster = jackbridge_set_timebase_callback(fClient, true, carla_jack_timebase_callback, this);
            }

            try {
                jackbridge_transport_start(fClient);
            } catch(...) {}
        }
    }

    void transportPause() noexcept override
    {
        if (pData->options.transportMode != ENGINE_TRANSPORT_MODE_JACK)
            return CarlaEngine::transportPause();

        if (fClient != nullptr)
        {
            try {
                jackbridge_transport_stop(fClient);
            } catch(...) {}
        }
    }

    void transportBPM(const double bpm) noexcept override
    {
        if (pData->options.transportMode != ENGINE_TRANSPORT_MODE_JACK || fTimebaseMaster)
            return CarlaEngine::transportBPM(bpm);

        if (fClient == nullptr)
            return;

        jack_position_t jpos;

        // invalidate
        jpos.unique_1 = 1;
        jpos.unique_2 = 2;
        jackbridge_transport_query(fClient, &jpos);

        if (jpos.unique_1 == jpos.unique_2 && (jpos.valid & JackPositionBBT) != 0)
        {
            carla_stdout("NOTE: Changing BPM without being JACK timebase master");
            jpos.beats_per_minute = bpm;

            try {
                jackbridge_transport_reposition(fClient, &jpos);
            } catch(...) {}
        }
    }

    void transportRelocate(const uint64_t frame) noexcept override
    {
        if (pData->options.transportMode != ENGINE_TRANSPORT_MODE_JACK)
            return CarlaEngine::transportRelocate(frame);

        if (fClient != nullptr)
        {
            try {
                jackbridge_transport_locate(fClient, static_cast<jack_nframes_t>(frame));
            } catch(...) {}
        }
    }

    // -------------------------------------------------------------------
    // Patchbay stuff

    const char* const* getPatchbayConnections(const bool external) const override
    {
        CARLA_CUSTOM_SAFE_ASSERT_RETURN("Not connected to JACK, will not save patchbay connections",
                                        fClient != nullptr, nullptr);
        carla_debug("CarlaEngineJack::getPatchbayConnections(%s)", bool2str(external));

        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY && ! external)
            return CarlaEngine::getPatchbayConnections(external);

        CarlaStringList connList;

        if (const char** const ports = jackbridge_get_ports(fClient, nullptr, nullptr, JackPortIsOutput))
        {
            for (int i=0; ports[i] != nullptr; ++i)
            {
                const jack_port_t* const jackPort(jackbridge_port_by_name(fClient, ports[i]));
                const char* const fullPortName(ports[i]);

                CARLA_SAFE_ASSERT_CONTINUE(jackPort != nullptr);

                if (const char** const connections = jackbridge_port_get_all_connections(fClient, jackPort))
                {
                    for (int j=0; connections[j] != nullptr; ++j)
                    {
                        connList.append(fullPortName);
                        connList.append(connections[j]);
                    }

                    jackbridge_free(connections);
                }
            }

            jackbridge_free(ports);
        }

        if (connList.count() == 0)
            return nullptr;

        fRetConns = connList.toCharStringListPtr();

        return fRetConns;
    }

    const PatchbayPosition* getPatchbayPositions(const bool external, uint& count) const override
    {
        CARLA_CUSTOM_SAFE_ASSERT_RETURN("Not connected to JACK, will not save patchbay positions",
                                        fClient != nullptr, nullptr);
        carla_debug("CarlaEngineJack::getPatchbayPositions(%s)", bool2str(external));

        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY && ! external)
            return CarlaEngine::getPatchbayPositions(external, count);

        const CarlaMutexLocker cml(fUsedGroups.mutex);
        const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

        if (const std::size_t maxCount = fUsedGroups.list.count())
        {
            PatchbayPosition* ret;

            try {
                ret = new CarlaEngine::PatchbayPosition[maxCount];
            } CARLA_SAFE_EXCEPTION_RETURN("new CarlaEngine::PatchbayPosition", nullptr);

            count = 0;

            GroupNameToId groupNameToId;

            for (LinkedList<GroupNameToId>::Itenerator it = fUsedGroups.list.begin2(); it.valid(); it.next())
            {
                groupNameToId = it.getValue(kGroupNameToIdFallback);
                CARLA_SAFE_ASSERT_CONTINUE(groupNameToId.group != 0);

                jack_uuid_t uuid;
                {
                    char* const uuidstr = jackbridge_get_uuid_for_client_name(fClient, groupNameToId.name);

                    if (uuidstr == nullptr || uuidstr[0] == '\0')
                        continue;

                    const bool parsed = jackbridge_uuid_parse(uuidstr, &uuid);
                    jackbridge_free(uuidstr);

                    /* if parsing fails, meta-data is not available..
                       this could be because JACK version is old, or perhaps this is an internal client */
                    if (! parsed)
                        continue;
                }

                char* value = nullptr;
                char* type  = nullptr;

                if (jackbridge_get_property(uuid, URI_POSITION, &value, &type)
                    && value != nullptr
                    && type != nullptr
                    && std::strcmp(type, URI_TYPE_STRING) == 0)
                {
                    CarlaEngine::PatchbayPosition& ppos(ret[count++]);

                    ppos.name = carla_strdup_safe(groupNameToId.name);
                    ppos.dealloc = true;
                    ppos.pluginId = -1;

                    if (char* sep1 = std::strstr(value, ":"))
                    {
                        *sep1++ = '\0';
                        ppos.x1 = std::atoi(value);

                        if (char* sep2 = std::strstr(sep1, ":"))
                        {
                            *sep2++ = '\0';
                            ppos.y1 = std::atoi(sep1);

                            if (char* sep3 = std::strstr(sep2, ":"))
                            {
                                *sep3++ = '\0';
                                ppos.x2 = std::atoi(sep2);
                                ppos.y2 = std::atoi(sep3);
                            }
                        }
                    }

                    jackbridge_free(value);
                    jackbridge_free(type);
                    value = type = nullptr;

                    const bool clientBelongsToUs = (jackbridge_get_property(uuid, URI_MAIN_CLIENT_NAME, &value, &type)
                                                    && value != nullptr
                                                    && type != nullptr
                                                    && std::strcmp(type, URI_TYPE_STRING) == 0
                                                    && fClientName == value);
                    jackbridge_free(value);
                    jackbridge_free(type);
                    value = type = nullptr;

                    if (! clientBelongsToUs)
                        continue;

                    if (jackbridge_get_property(uuid, URI_PLUGIN_ID, &value, &type)
                                                && value != nullptr
                                                && type != nullptr
                                                && std::strcmp(type, URI_TYPE_INTEGER) == 0)
                    {
                        ppos.pluginId = std::atoi(value);
                    }

                    jackbridge_free(value);
                    jackbridge_free(type);
                }
            }

            return ret;
        }

        return nullptr;
    }

    void restorePatchbayConnection(const bool external,
                                   const char* const connSource, const char* const connTarget) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(connSource != nullptr && connSource[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(connTarget != nullptr && connTarget[0] != '\0',);
        carla_debug("CarlaEngineJack::restorePatchbayConnection(%s, \"%s\", \"%s\")",
                    bool2str(external), connSource, connTarget);

        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY && ! external)
            return CarlaEngine::restorePatchbayConnection(external, connSource, connTarget);

        if (const jack_port_t* const port = jackbridge_port_by_name(fClient, connSource))
        {
            if (jackbridge_port_by_name(fClient, connTarget) == nullptr)
                return;

            if (! jackbridge_port_connected_to(port, connTarget))
                jackbridge_connect(fClient, connSource, connTarget);
        }
    }

    bool restorePatchbayGroupPosition(const bool external, PatchbayPosition& ppos) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, false);
        carla_debug("CarlaEngineJack::restorePatchbayGroupPosition(%s, {%i, %i, %i, %i, %i, \"%s\"})",
                    bool2str(external), ppos.pluginId, ppos.x1, ppos.y1, ppos.x2, ppos.y2, ppos.name);

        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY && ! external)
            return CarlaEngine::restorePatchbayGroupPosition(external, ppos);

        uint groupId = 0;
        const char* const orig_name = ppos.name;

        /* NOTE: When loading a project, it might take a bit to receive plugins' jack client registration callbacks.
         *       We try to wait a little for it, but not too much.
         */
        if (ppos.pluginId >= 0)
        {
            // strip client name prefix if already in place
            if (const char* const rname2 = std::strstr(ppos.name, "."))
                if (const char* const rname3 = std::strstr(rname2 + 1, "/"))
                    ppos.name = rname3 + 1;

            if (pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
            {
                if (fClientNamePrefix.isNotEmpty())
                {
                    char* nname = (char*)std::malloc(fClientNamePrefix.length() + std::strlen(ppos.name) + 1);
                    std::strcpy(nname, fClientNamePrefix.buffer());
                    std::strcat(nname, ppos.name);

                    ppos.name = nname;
                    ppos.dealloc = true;
                }

                for (int i=20; --i >=0;)
                {
                    {
                        const CarlaMutexLocker cml(fUsedGroups.mutex);

                        if (fUsedGroups.list.count() == 0)
                            break;

                        groupId = fUsedGroups.getGroupId(ppos.name);
                    }

                    if (groupId != 0)
                        break;

                    d_msleep(100);
                    callback(true, true, ENGINE_CALLBACK_IDLE, 0, 0, 0, 0, 0.0f, nullptr);
                }
            }
        }
        else
        {
            const CarlaMutexLocker cml(fUsedGroups.mutex);

            if (fUsedGroups.list.count() != 0)
                groupId = fUsedGroups.getGroupId(ppos.name);
        }

        if (groupId == 0)
        {
            if (ppos.pluginId < 0 || pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
                carla_stdout("Previously saved client '%s' not found", ppos.name);
        }
        else
        {
            for (;;)
            {
                const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

                jack_uuid_t uuid;
                {
                    char* const uuidstr = jackbridge_get_uuid_for_client_name(fClient, ppos.name);
                    CARLA_SAFE_ASSERT_BREAK(uuidstr != nullptr && uuidstr[0] != '\0');

                    const bool parsed = jackbridge_uuid_parse(uuidstr, &uuid);
                    jackbridge_free(uuidstr);
                    CARLA_CUSTOM_SAFE_ASSERT_ONCE_BREAK("JACK meta-data support unavailable", parsed);
                }

                char valueStr[STR_MAX];
                std::snprintf(valueStr, STR_MAX-1, "%i:%i:%i:%i", ppos.x1, ppos.y1, ppos.x2, ppos.y2);
                valueStr[STR_MAX-1] = '\0';

                jackbridge_set_property(fClient, uuid, URI_POSITION, valueStr, URI_TYPE_STRING);
                break;
            }

            callback(true, true,
                    ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED,
                    groupId, ppos.x1, ppos.y1, ppos.x2, static_cast<float>(ppos.y2),
                    nullptr);
        }

        return ppos.name != orig_name;
    }
#endif

    // -------------------------------------------------------------------

protected:
    void handleJackBufferSizeCallback(const uint32_t newBufferSize)
    {
        if (pData->bufferSize == newBufferSize)
            return;

#ifndef BUILD_BRIDGE
        const CarlaMutexLocker cml(fPatchbayProcThreadProtectionMutex);
#endif

        pData->bufferSize = newBufferSize;
        bufferSizeChanged(newBufferSize);
    }

    void handleJackSampleRateCallback(const double newSampleRate)
    {
        if (carla_isEqual(pData->sampleRate, newSampleRate))
            return;

#ifndef BUILD_BRIDGE
        const CarlaMutexLocker cml(fPatchbayProcThreadProtectionMutex);
#endif

        pData->sampleRate = newSampleRate;
        sampleRateChanged(newSampleRate);
    }

    void handleJackFreewheelCallback(const bool isFreewheel)
    {
        if (fFreewheel == isFreewheel)
            return;

#ifndef BUILD_BRIDGE
        const CarlaMutexLocker cml(fPatchbayProcThreadProtectionMutex);
#endif

        fFreewheel = isFreewheel;
        offlineModeChanged(isFreewheel);
    }

    void handleJackProcessCallback(const uint32_t nframes)
    {
        const PendingRtEventsRunner prt(this, nframes);

        CARLA_SAFE_ASSERT_INT2_RETURN(nframes == pData->bufferSize, nframes, pData->bufferSize,);

#ifdef BUILD_BRIDGE
        CarlaPluginPtr plugin = pData->plugins[0].plugin;

        if (plugin.get() != nullptr && plugin->isEnabled() && plugin->tryLock(fFreewheel))
        {
            plugin->initBuffers();
            processPlugin(plugin, nframes);
            plugin->unlock();
        }
#else
        if (pData->options.transportMode == ENGINE_TRANSPORT_MODE_JACK && !fTimebaseMaster)
        {
            jack_position_t jpos;

            // invalidate
            jpos.unique_1 = 1;
            jpos.unique_2 = 2;

            EngineTimeInfo timeInfo;
            const bool playing = jackbridge_transport_query(fClient, &jpos) == JackTransportRolling;

            if (jpos.unique_1 != jpos.unique_2)
            {
                timeInfo.playing = false;
                timeInfo.frame = 0;
                timeInfo.usecs = 0;
                timeInfo.bbt.valid = false;
            }
            else
            {
                timeInfo.playing = playing;
                timeInfo.frame   = jpos.frame;
                timeInfo.usecs   = jpos.usecs;

                if (jpos.valid & JackPositionBBT)
                {
                    timeInfo.bbt.valid          = true;
                    timeInfo.bbt.bar            = jpos.bar;
                    timeInfo.bbt.beat           = jpos.beat;
#ifdef JACK_TICK_DOUBLE
                    if (jpos.valid & JackTickDouble)
                        timeInfo.bbt.tick       = jpos.tick_double;
                    else
#endif
                        timeInfo.bbt.tick       = jpos.tick;
                    timeInfo.bbt.barStartTick   = jpos.bar_start_tick;
                    timeInfo.bbt.beatsPerBar    = jpos.beats_per_bar;
                    timeInfo.bbt.beatType       = jpos.beat_type;
                    timeInfo.bbt.ticksPerBeat   = jpos.ticks_per_beat;
                    timeInfo.bbt.beatsPerMinute = jpos.beats_per_minute;
                }
                else
                {
                    timeInfo.bbt.valid = false;
                }
            }

            pData->timeInfo = timeInfo;
        }

        if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        {
            if (pData->aboutToClose)
            {
                if (float* const audioOut1 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut1], nframes))
                    carla_zeroFloats(audioOut1, nframes);

                if (float* const audioOut2 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut2], nframes))
                    carla_zeroFloats(audioOut2, nframes);
            }
            else if (pData->curPluginCount == 0)
            {
                float* const audioIn1  = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioIn1], nframes);
                float* const audioIn2  = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioIn2], nframes);
                float* const audioOut1 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut1], nframes);
                float* const audioOut2 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut2], nframes);

                // assert buffers
                CARLA_SAFE_ASSERT_RETURN(audioIn1 != nullptr,);
                CARLA_SAFE_ASSERT_RETURN(audioIn2 != nullptr,);
                CARLA_SAFE_ASSERT_RETURN(audioOut1 != nullptr,);
                CARLA_SAFE_ASSERT_RETURN(audioOut2 != nullptr,);

                // pass-through
                carla_copyFloats(audioOut1, audioIn1, nframes);
                carla_copyFloats(audioOut2, audioIn2, nframes);

                // TODO pass-through MIDI as well
                if (void* const eventOut = jackbridge_port_get_buffer(fRackPorts[kRackPortEventOut], nframes))
                    jackbridge_midi_clear_buffer(eventOut);

                return;
            }
        }

        if (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
        {
            for (uint i=0; i < pData->curPluginCount; ++i)
            {
                if (CarlaPluginPtr plugin = pData->plugins[i].plugin)
                {
                    if (plugin->isEnabled() && plugin->tryLock(fFreewheel))
                    {
                        plugin->initBuffers();
                        processPlugin(plugin, nframes);
                        plugin->unlock();
                    }
                }
            }
        }
        else if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK ||
                 pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        {
            CARLA_SAFE_ASSERT_RETURN(pData->events.in  != nullptr,);
            CARLA_SAFE_ASSERT_RETURN(pData->events.out != nullptr,);

            // get buffers from jack
            float* const audioIn1  = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioIn1], nframes);
            float* const audioIn2  = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioIn2], nframes);
            float* const audioOut1 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut1], nframes);
            float* const audioOut2 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut2], nframes);
            void* const  eventIn   = jackbridge_port_get_buffer(fRackPorts[kRackPortEventIn],  nframes);
            void* const  eventOut  = jackbridge_port_get_buffer(fRackPorts[kRackPortEventOut], nframes);

            // assert buffers
            CARLA_SAFE_ASSERT_RETURN(audioIn1 != nullptr,);
            CARLA_SAFE_ASSERT_RETURN(audioIn2 != nullptr,);
            CARLA_SAFE_ASSERT_RETURN(audioOut1 != nullptr,);
            CARLA_SAFE_ASSERT_RETURN(audioOut2 != nullptr,);

            // create audio buffers
            const float* inBuf[2]  = { audioIn1, audioIn2 };
            /**/  float* outBuf[2] = { audioOut1, audioOut2 };

            // initialize events
            carla_zeroStructs(pData->events.in,  kMaxEngineEventInternalCount);
            carla_zeroStructs(pData->events.out, kMaxEngineEventInternalCount);

            if (eventIn != nullptr)
            {
                ushort engineEventIndex = 0;

                jack_midi_event_t jackEvent;
                const uint32_t jackEventCount(jackbridge_midi_get_event_count(eventIn));

                for (uint32_t jackEventIndex=0; jackEventIndex < jackEventCount; ++jackEventIndex)
                {
                    if (! jackbridge_midi_event_get(&jackEvent, eventIn, jackEventIndex))
                        continue;

                    CARLA_SAFE_ASSERT_CONTINUE(jackEvent.size < 0xFF /* uint8_t max */);

                    EngineEvent& engineEvent(pData->events.in[engineEventIndex++]);

                    engineEvent.time = jackEvent.time;
                    engineEvent.fillFromMidiData(static_cast<uint8_t>(jackEvent.size), jackEvent.buffer, 0);

                    if (engineEventIndex >= kMaxEngineEventInternalCount)
                        break;
                }
            }

            if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
            {
                pData->graph.processRack(pData, inBuf, outBuf, nframes);
            }
            else
            {
                const CarlaMutexLocker cml(fPatchbayProcThreadProtectionMutex);
                pData->graph.process(pData, inBuf, outBuf, nframes);
            }

            // output control
            if (eventOut != nullptr)
            {
                jackbridge_midi_clear_buffer(eventOut);

                uint8_t  size     = 0;
                uint8_t  mdata[3] = { 0, 0, 0 };
                uint8_t  mdataTmp[EngineMidiEvent::kDataSize];
                const uint8_t* mdataPtr;

                for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
                {
                    const EngineEvent& engineEvent(pData->events.out[i]);

                    /**/ if (engineEvent.type == kEngineEventTypeNull)
                    {
                        break;
                    }
                    else if (engineEvent.type == kEngineEventTypeControl)
                    {
                        const EngineControlEvent& ctrlEvent(engineEvent.ctrl);

                        size = ctrlEvent.convertToMidiData(engineEvent.channel, mdata);
                        mdataPtr = mdata;
                    }
                    else if (engineEvent.type == kEngineEventTypeMidi)
                    {
                        const EngineMidiEvent& midiEvent(engineEvent.midi);

                        size = midiEvent.size;
                        CARLA_SAFE_ASSERT_CONTINUE(size > 0);

                        if (size > EngineMidiEvent::kDataSize)
                        {
                            CARLA_SAFE_ASSERT_CONTINUE(midiEvent.dataExt != nullptr);
                            mdataPtr = midiEvent.dataExt;
                        }
                        else
                        {
                            // set first byte
                            mdataTmp[0] = static_cast<uint8_t>(midiEvent.data[0] | (engineEvent.channel & MIDI_CHANNEL_BIT));

                            if (size > 1)
                            {
                                // copy rest
                                carla_copy<uint8_t>(mdataTmp+1, midiEvent.data+1, size-1U);
                            }

                            // done
                            mdataPtr = mdataTmp;
                        }
                    }
                    else
                    {
                        continue;
                    }

                    if (size > 0)
                        jackbridge_midi_event_write(eventOut, engineEvent.time, mdataPtr, size);
                }
            }
        }

        if (pData->options.transportMode == ENGINE_TRANSPORT_MODE_JACK)
        {
            if (fTimebaseMaster)
            {
                const bool playing = jackbridge_transport_query(fClient, nullptr) == JackTransportRolling;

                if (fTimebaseRolling != playing)
                {
                    fTimebaseRolling = playing;
                    pData->timeInfo.playing = playing;
                }

                // Check if we are no longer timebase master
                if (playing && fTimebaseUsecs != 0 && fTimebaseUsecs == pData->timeInfo.usecs)
                {
                    carla_debug("No longer timerbase master");
                    fTimebaseMaster = false;
                }
            }

            fTimebaseUsecs = pData->timeInfo.usecs;
        }
#endif // ! BUILD_BRIDGE
    }

    void handleJackLatencyCallback(const jack_latency_callback_mode_t /*mode*/)
    {
        // TODO
    }

#ifndef BUILD_BRIDGE
    void handleJackTimebaseCallback(jack_nframes_t nframes, jack_position_t* const pos, const int new_pos)
    {
        if (new_pos)
            pData->time.setNeedsReset();

        pData->timeInfo.playing = fTimebaseRolling;
        pData->timeInfo.frame = pos->frame;
        pData->timeInfo.usecs = pos->usecs;
        pData->time.fillJackTimeInfo(pos, nframes);
    }

    void handleJackClientUnregistrationCallback(const char* const name)
    {
        CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

        uint groupId;

        {
            const CarlaMutexLocker cml(fUsedGroups.mutex);
            groupId = fUsedGroups.getGroupId(name);

            // clients might have been registered without ports
            if (groupId == 0) return;

            GroupNameToId groupNameToId;
            groupNameToId.setData(groupId, name);

            fUsedGroups.list.removeOne(groupNameToId);
        }

        // ignore callback if on internal patchbay mode
#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (! (fExternalPatchbayHost || (fExternalPatchbayOsc && pData->osc.isControlRegisteredForTCP()))) return;
#else
        if (! fExternalPatchbayHost) return;
#endif

        callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                 ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED,
                 groupId,
                 0, 0, 0, 0.0f, nullptr);
    }

    void handleJackClientPositionChangeCallback(const jack_uuid_t uuid)
    {
        // ignore this if on internal patchbay mode
#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (! (fExternalPatchbayHost || (fExternalPatchbayOsc && pData->osc.isControlRegisteredForTCP()))) return;
#else
        if (! fExternalPatchbayHost) return;
#endif

        const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

        char uuidstr[JACK_UUID_STRING_SIZE] = {};
        jackbridge_uuid_unparse(uuid, uuidstr);

        if (char* const clientName = jackbridge_get_client_name_by_uuid(fClient, uuidstr))
        {
            CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0',);

            uint groupId;

            {
                const CarlaMutexLocker cml(fUsedGroups.mutex);
                groupId = fUsedGroups.getGroupId(clientName);
            }

            jackbridge_free(clientName);
            CARLA_SAFE_ASSERT_RETURN(groupId != 0,);

            char* value = nullptr;
            char* type  = nullptr;

            if (jackbridge_get_property(uuid, URI_POSITION, &value, &type)
                && value != nullptr
                && type != nullptr
                && std::strcmp(type, URI_TYPE_STRING) == 0)
            {
                if (char* sep1 = std::strstr(value, ":"))
                {
                    LastPatchbaySetGroupPos pos;
                    *sep1++ = '\0';
                    pos.x1 = std::atoi(value);

                    if (char* sep2 = std::strstr(sep1, ":"))
                    {
                        *sep2++ = '\0';
                        pos.y1 = std::atoi(sep1);

                        if (char* sep3 = std::strstr(sep2, ":"))
                        {
                            *sep3++ = '\0';
                            pos.x2 = std::atoi(sep2);
                            pos.y2 = std::atoi(sep3);
                        }

                        if (fLastPatchbaySetGroupPos != pos)
                        {
                            fLastPatchbaySetGroupPos.clear();

                            callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                                        ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED,
                                        groupId, pos.x1, pos.y1, pos.x2, static_cast<float>(pos.y2),
                                        nullptr);
                        }
                    }
                }

                jackbridge_free(value);
                jackbridge_free(type);
            }
        }
    }

    void handleJackPortRegistrationCallback(const char* const portName,
                                            const char* const shortPortName,
                                            const CarlaJackPortHints& jackPortHints)
    {
        bool groupFound;
        String groupName(portName);
        groupName.truncate(groupName.rfind(shortPortName, &groupFound)-1);
        CARLA_SAFE_ASSERT_RETURN(groupFound,);

        groupFound = false;
        GroupToIdData groupData;
        PortToIdData portData;

        {
            const CarlaMutexLocker cml1(fUsedGroups.mutex);

            groupData.id = fUsedGroups.getGroupId(groupName);

            if (groupData.id == 0)
            {
                groupData.id = ++fUsedGroups.lastId;

                GroupNameToId groupNameToId;
                groupNameToId.setData(groupData.id, groupName);

                int pluginId = -1;
                PatchbayIcon icon = jackPortHints.isHardware ? PATCHBAY_ICON_HARDWARE : PATCHBAY_ICON_APPLICATION;

                findPluginIdAndIcon(groupName, pluginId, icon);

                fUsedGroups.list.append(groupNameToId);

                groupFound = true;
                groupData.icon = icon;
                groupData.pluginId = pluginId;
                std::strncpy(groupData.strVal, groupName, STR_MAX-1);
                groupData.strVal[STR_MAX-1] = '\0';
            }
        }

        // ignore the rest if on internal patchbay mode
#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (! (fExternalPatchbayHost || (fExternalPatchbayOsc && pData->osc.isControlRegisteredForTCP()))) return;
#else
        if (! fExternalPatchbayHost) return;
#endif

        {
            uint canvasPortFlags = 0x0;
            canvasPortFlags |= jackPortHints.isInput ? PATCHBAY_PORT_IS_INPUT : 0x0;

            /**/ if (jackPortHints.isCV)
                canvasPortFlags |= PATCHBAY_PORT_TYPE_CV;
            else if (jackPortHints.isOSC)
                canvasPortFlags |= PATCHBAY_PORT_TYPE_OSC;
            else if (jackPortHints.isAudio)
                canvasPortFlags |= PATCHBAY_PORT_TYPE_AUDIO;
            else if (jackPortHints.isMIDI)
                canvasPortFlags |= PATCHBAY_PORT_TYPE_MIDI;

            const CarlaMutexLocker cml2(fUsedPorts.mutex);

            portData.group = groupData.id;
            portData.port = ++fUsedPorts.lastId;
            portData.flags = canvasPortFlags;
            std::strncpy(portData.strVal, shortPortName, STR_MAX-1);
            portData.strVal[STR_MAX-1] = '\0';

            PortNameToId portNameToId;
            portNameToId.setData(portData.group, portData.port, shortPortName, portName);
            fUsedPorts.list.append(portNameToId);
        }

        if (groupFound)
        {
            callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                      ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED,
                      groupData.id,
                      groupData.icon,
                      groupData.pluginId,
                      0, 0.0f,
                      groupData.strVal);
        }

        callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                 ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                 portData.group,
                 static_cast<int>(portData.port),
                 static_cast<int>(portData.flags),
                 0, 0.0f,
                 portData.strVal);
    }

    void handleJackPortUnregistrationCallback(const char* const portName)
    {
        // ignore this if on internal patchbay mode
#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (! (fExternalPatchbayHost || (fExternalPatchbayOsc && pData->osc.isControlRegisteredForTCP()))) return;
#else
        if (! fExternalPatchbayHost) return;
#endif

        uint groupId, portId;

        {
            const CarlaMutexLocker cml(fUsedPorts.mutex);

            const PortNameToId& portNameToId(fUsedPorts.getPortNameToId(portName));

            /* NOTE: Due to JACK2 async behaviour the port we get here might be the same of a previous rename-plugin request.
                     See the comment on CarlaEngineJack::renamePlugin() for more information. */
            if (portNameToId.group <= 0 || portNameToId.port <= 0) return;

            groupId = portNameToId.group;
            portId = portNameToId.port;

            fUsedPorts.list.removeOne(portNameToId);
        }

        callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                  ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED,
                  groupId,
                  static_cast<int>(portId),
                  0, 0, 0.0f, nullptr);
    }

    void handleJackPortConnectCallback(const char* const portNameA, const char* const portNameB)
    {
        // ignore this if on internal patchbay mode
#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (! (fExternalPatchbayHost || (fExternalPatchbayOsc && pData->osc.isControlRegisteredForTCP()))) return;
#else
        if (! fExternalPatchbayHost) return;
#endif

        char strBuf[STR_MAX];
        uint connectionId;

        {
            const CarlaMutexLocker cml1(fUsedPorts.mutex);

            const PortNameToId& portNameToIdA(fUsedPorts.getPortNameToId(portNameA));
            const PortNameToId& portNameToIdB(fUsedPorts.getPortNameToId(portNameB));

            /* NOTE: Due to JACK2 async behaviour the port we get here might be the same of a previous rename-plugin request.
                     See the comment on CarlaEngineJack::renamePlugin() for more information. */
            if (portNameToIdA.group <= 0 || portNameToIdA.port <= 0) return;
            if (portNameToIdB.group <= 0 || portNameToIdB.port <= 0) return;

            const CarlaMutexLocker cml2(fUsedConnections.mutex);

            std::snprintf(strBuf, STR_MAX-1, "%i:%i:%i:%i",
                          portNameToIdA.group, portNameToIdA.port,
                          portNameToIdB.group, portNameToIdB.port);
            strBuf[STR_MAX-1] = '\0';

            connectionId = ++fUsedConnections.lastId;

            ConnectionToId connectionToId;
            connectionToId.setData(connectionId,
                                   portNameToIdA.group, portNameToIdA.port,
                                   portNameToIdB.group, portNameToIdB.port);

            fUsedConnections.list.append(connectionToId);
        }

        callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                  ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED,
                  connectionId,
                  0, 0, 0, 0.0f,
                  strBuf);
    }

    void handleJackPortDisconnectCallback(const char* const portNameA, const char* const portNameB)
    {
        // ignore this if on internal patchbay mode
#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (! (fExternalPatchbayHost || (fExternalPatchbayOsc && pData->osc.isControlRegisteredForTCP()))) return;
#else
        if (! fExternalPatchbayHost) return;
#endif

        uint connectionId = 0;

        {
            const CarlaMutexLocker cml1(fUsedPorts.mutex);

            const PortNameToId& portNameToIdA(fUsedPorts.getPortNameToId(portNameA));
            const PortNameToId& portNameToIdB(fUsedPorts.getPortNameToId(portNameB));

            /* NOTE: Due to JACK2 async behaviour the port we get here might be the same of a previous rename-plugin request.
                     See the comment on CarlaEngineJack::renamePlugin() for more information. */
            if (portNameToIdA.group <= 0 || portNameToIdA.port <= 0) return;
            if (portNameToIdB.group <= 0 || portNameToIdB.port <= 0) return;

            const CarlaMutexLocker cml2(fUsedConnections.mutex);

            for (LinkedList<ConnectionToId>::Itenerator it = fUsedConnections.list.begin2(); it.valid(); it.next())
            {
                const ConnectionToId& connectionToId = it.getValue(kConnectionToIdFallback);
                CARLA_SAFE_ASSERT_CONTINUE(connectionToId.id != 0);

                if (connectionToId.groupA == portNameToIdA.group && connectionToId.portA == portNameToIdA.port &&
                    connectionToId.groupB == portNameToIdB.group && connectionToId.portB == portNameToIdB.port)
                {
                    connectionId = connectionToId.id;
                    fUsedConnections.list.remove(it);
                    break;
                }
            }
        }

        if (connectionId != 0) {
            callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                      ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED,
                      connectionId,
                      0, 0, 0, 0.0f, nullptr);
        }
    }

    void handleJackPortRenameCallback(const char* const oldFullName,
                                      const char* const newFullName,
                                      const char* const newShortName)
    {
        // ignore this if on internal patchbay mode
#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (! (fExternalPatchbayHost || (fExternalPatchbayOsc && pData->osc.isControlRegisteredForTCP()))) return;
#else
        if (! fExternalPatchbayHost) return;
#endif

        CARLA_SAFE_ASSERT_RETURN(oldFullName != nullptr && oldFullName[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(newFullName != nullptr && newFullName[0] != '\0',);

        bool found;
        String groupName(newFullName);
        groupName.truncate(groupName.rfind(newShortName, &found)-1);
        CARLA_SAFE_ASSERT_RETURN(found,);

        uint groupId, portId = 0;
        char portName[STR_MAX];
        found = false;

        {
            const CarlaMutexLocker cml1(fUsedGroups.mutex);

            groupId = fUsedGroups.getGroupId(groupName);
            CARLA_SAFE_ASSERT_RETURN(groupId != 0,);

            const CarlaMutexLocker cml2(fUsedPorts.mutex);

            for (LinkedList<PortNameToId>::Itenerator it = fUsedPorts.list.begin2(); it.valid(); it.next())
            {
                PortNameToId& portNameToId(it.getValue(kPortNameToIdFallbackNC));
                CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group != 0);

                if (std::strncmp(portNameToId.fullName, oldFullName, STR_MAX) == 0)
                {
                    CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group == groupId);

                    found = true;
                    portId = portNameToId.port;
                    std::strncpy(portName, newShortName, STR_MAX-1);
                    portName[STR_MAX-1] = '\0';

                    portNameToId.rename(newShortName, newFullName);
                    break;
                }
            }
        }

        if (found)
        {
            callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                      ENGINE_CALLBACK_PATCHBAY_PORT_CHANGED,
                      groupId,
                      static_cast<int>(portId),
                      0, 0, 0.0f,
                      portName);
        }
    }
#endif

    void handleJackShutdownCallback()
    {
#ifndef BUILD_BRIDGE
        if (fIsInternalClient)
            stopThread(-1);
#endif

        {
            const PendingRtEventsRunner prt(this, pData->bufferSize);

            for (uint i=0; i < pData->curPluginCount; ++i)
            {
                if (const CarlaPluginPtr plugin = pData->plugins[i].plugin)
                {
                    plugin->tryLock(true);

                    if (CarlaEngineJackClient* const client = (CarlaEngineJackClient*)plugin->getEngineClient())
                        client->invalidate();

                    plugin->unlock();
                }
            }
        }

        pData->runner.stopRunner();
        fClient = nullptr;

#ifdef BUILD_BRIDGE
        fIsRunning = false;
#else
        carla_zeroPointers(fRackPorts, kRackPortCount);
#endif

        callback(true, true, ENGINE_CALLBACK_ERROR, 0, 0, 0, 0, 0.0f,
                 "Carla has been killed by JACK, or JACK has stopped.\n"
                 "You can still save if you want, but you will lose patchbay connections and positions.");
    }

    // -------------------------------------------------------------------

    void handlePluginJackShutdownCallback(const CarlaPluginPtr plugin)
    {
        CarlaEngineJackClient* const engineClient((CarlaEngineJackClient*)plugin->getEngineClient());
        CARLA_SAFE_ASSERT_RETURN(engineClient != nullptr,);

        plugin->tryLock(true);
        engineClient->invalidate();
        plugin->unlock();

        callback(true, true, ENGINE_CALLBACK_PLUGIN_UNAVAILABLE, plugin->getId(), 0, 0, 0, 0.0f, "Killed by JACK");
    }

    // -------------------------------------------------------------------

private:
    jack_client_t* fClient;
    bool fExternalPatchbayHost;
    bool fExternalPatchbayOsc;
    bool fFreewheel;

    String fClientName;
    CarlaRecursiveMutex fThreadSafeMetadataMutex;

    // -------------------------------------------------------------------

#ifdef BUILD_BRIDGE
    bool fIsRunning;
#else
    String fClientNamePrefix;

    enum RackPorts {
        kRackPortAudioIn1  = 0,
        kRackPortAudioIn2  = 1,
        kRackPortAudioOut1 = 2,
        kRackPortAudioOut2 = 3,
        kRackPortEventIn   = 4,
        kRackPortEventOut  = 5,
        kRackPortCount     = 6
    };

    jack_port_t* fRackPorts[kRackPortCount];

    bool fTimebaseMaster;
    bool fTimebaseRolling;
    uint64_t fTimebaseUsecs;

    PatchbayGroupList      fUsedGroups;
    PatchbayPortList       fUsedPorts;
    PatchbayConnectionList fUsedConnections;
    CarlaMutex             fPatchbayProcThreadProtectionMutex;

    mutable CharStringListPtr fRetConns;

    void findPluginIdAndIcon(const char* const clientName, int& pluginId, PatchbayIcon& icon) const noexcept
    {
        carla_debug("CarlaEngineJack::findPluginIdAndIcon(\"%s\", ...)", clientName);

        // TODO - this currently only works in multi-client mode
        if (pData->options.processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
            return;

        const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

        jack_uuid_t uuid;
        {
            char* const uuidstr = jackbridge_get_uuid_for_client_name(fClient, clientName);
            CARLA_SAFE_ASSERT_RETURN(uuidstr != nullptr && uuidstr[0] != '\0',);

            const bool parsed = jackbridge_uuid_parse(uuidstr, &uuid);
            jackbridge_free(uuidstr);

            /* if parsing fails, meta-data is not available..
               this could be because JACK version is old, or perhaps this is an internal client */
            if (! parsed)
                return;
        }

        bool clientBelongsToUs;

        {
            char* value = nullptr;
            char* type = nullptr;

            if (! jackbridge_get_property(uuid, URI_MAIN_CLIENT_NAME, &value, &type))
                return;

            CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
            CARLA_SAFE_ASSERT_RETURN(value != nullptr && value[0] != '\0',);
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(type, URI_TYPE_STRING) == 0,);

            clientBelongsToUs = fClientName == value;

            jackbridge_free(value);
            jackbridge_free(type);
        }

        {
            char* value = nullptr;
            char* type = nullptr;

            if (! jackbridge_get_property(uuid, URI_PLUGIN_ID, &value, &type))
                return;

            CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
            CARLA_SAFE_ASSERT_RETURN(value != nullptr && value[0] != '\0',);
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(type, URI_TYPE_INTEGER) == 0,);

            if (clientBelongsToUs)
                pluginId = std::atoi(value);

            icon = PATCHBAY_ICON_PLUGIN;

            jackbridge_free(value);
            jackbridge_free(type);
        }

        {
            char* value = nullptr;
            char* type = nullptr;

            if (! jackbridge_get_property(uuid, URI_PLUGIN_ICON, &value, &type))
                return;

            CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
            CARLA_SAFE_ASSERT_RETURN(value != nullptr && value[0] != '\0',);
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(type, URI_TYPE_STRING) == 0,);

            /**/ if (std::strcmp(value, "app") == 0)
                icon = PATCHBAY_ICON_APPLICATION;
            else if (std::strcmp(value, "application") == 0)
                icon = PATCHBAY_ICON_APPLICATION;
            else if (std::strcmp(value, "plugin") == 0)
                icon = PATCHBAY_ICON_PLUGIN;
            else if (std::strcmp(value, "hardware") == 0)
                icon = PATCHBAY_ICON_HARDWARE;
            else if (std::strcmp(value, "carla") == 0)
                icon = PATCHBAY_ICON_CARLA;
            else if (std::strcmp(value, "distrho") == 0)
                icon = PATCHBAY_ICON_DISTRHO;
            else if (std::strcmp(value, "file") == 0)
                icon = PATCHBAY_ICON_FILE;

            jackbridge_free(value);
            jackbridge_free(type);
        }
    }

    // prevent recursion on patchbay group position changes
    struct LastPatchbaySetGroupPos {
        int x1, y1, x2, y2;

        LastPatchbaySetGroupPos()
            : x1(0), y1(0), x2(0), y2(0) {}

        void clear() noexcept
        {
            x1 = y1 = x2 = y2 = 0;
        }

        void set(const int _x1, const int _y1, const int _x2, const int _y2) noexcept
        {
            x1 = _x1;
            y1 = _y1;
            x2 = _x2;
            y2 = _y2;
        }

        bool operator!=(const LastPatchbaySetGroupPos& pos) const noexcept
        {
            return pos.x1 != x1 || pos.y1 != y1 || pos.x2 != x2 || pos.y2 != y2;
        }
    } fLastPatchbaySetGroupPos;

    // handy stuff only needed for initJackPatchbay
    struct GroupToIdData {
        uint id;
        PatchbayIcon icon;
        int pluginId;
        char strVal[STR_MAX];
    };
    struct PortToIdData {
        uint group;
        uint port;
        uint flags;
        char strVal[STR_MAX];
    };
    struct ConnectionToIdData {
        uint id;
        char strVal[STR_MAX];
    };

    void initJackPatchbay(const bool sendHost, const bool sendOSC, const char* const ourName, const bool groupsOnly)
    {
        CARLA_SAFE_ASSERT_RETURN(ourName != nullptr && ourName[0] != '\0',);

        fLastPatchbaySetGroupPos.clear();

        uint id, carlaId;
        CarlaStringList parsedGroups;
        LinkedList<GroupToIdData> groupCallbackData;
        LinkedList<PortToIdData> portsCallbackData;
        LinkedList<ConnectionToIdData> connCallbackData;

        {
            const CarlaMutexLocker cml1(fUsedGroups.mutex);
            const CarlaMutexLocker cml2(fUsedPorts.mutex);
            const CarlaMutexLocker cml3(fUsedConnections.mutex);
            const CarlaMutexLocker cml4(fPostPonedEventsMutex);

            fUsedGroups.clear();
            fUsedPorts.clear();
            fUsedConnections.clear();
            fPostPonedEvents.clear();

            // add our client first
            {
                carlaId = ++fUsedGroups.lastId;
                parsedGroups.append(ourName);

                GroupNameToId groupNameToId;
                groupNameToId.setData(carlaId, ourName);
                fUsedGroups.list.append(groupNameToId);
            }

            // query all jack ports
            {
                const char** const ports = jackbridge_get_ports(fClient, nullptr, nullptr, 0);
                CARLA_SAFE_ASSERT_RETURN(ports != nullptr,);

                const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

                for (int i=0; ports[i] != nullptr; ++i)
                {
                    const char* const fullPortName(ports[i]);
                    CARLA_SAFE_ASSERT_CONTINUE(fullPortName != nullptr && fullPortName[0] != '\0');

                    const jack_port_t* const jackPort(jackbridge_port_by_name(fClient, fullPortName));
                    CARLA_SAFE_ASSERT_CONTINUE(jackPort != nullptr);

                    const char* const shortPortName(jackbridge_port_short_name(jackPort));
                    CARLA_SAFE_ASSERT_CONTINUE(shortPortName != nullptr && shortPortName[0] != '\0');

                    const CarlaJackPortHints jackPortHints(CarlaJackPortHints::fromPort(jackPort));

                    uint groupId = 0;

                    bool found;
                    String groupName(fullPortName);
                    groupName.truncate(groupName.rfind(shortPortName, &found)-1);

                    CARLA_SAFE_ASSERT_CONTINUE(found);

                    if (parsedGroups.contains(groupName))
                    {
                        groupId = fUsedGroups.getGroupId(groupName);
                        CARLA_SAFE_ASSERT_CONTINUE(groupId > 0);
                    }
                    else
                    {
                        groupId = ++fUsedGroups.lastId;
                        parsedGroups.append(groupName);

                        GroupNameToId groupNameToId;
                        groupNameToId.setData(groupId, groupName);

                        int pluginId = -1;
                        PatchbayIcon icon = jackPortHints.isHardware ? PATCHBAY_ICON_HARDWARE : PATCHBAY_ICON_APPLICATION;

                        findPluginIdAndIcon(groupName, pluginId, icon);

                        fUsedGroups.list.append(groupNameToId);

                        if (! groupsOnly)
                        {
                            GroupToIdData groupData;
                            groupData.id = groupId;
                            groupData.icon = icon;
                            groupData.pluginId = pluginId;
                            std::strncpy(groupData.strVal, groupName, STR_MAX-1);
                            groupData.strVal[STR_MAX-1] = '\0';
                            groupCallbackData.append(groupData);
                        }
                    }

                    if (groupsOnly)
                        continue;

                    uint canvasPortFlags = 0x0;
                    canvasPortFlags |= jackPortHints.isInput ? PATCHBAY_PORT_IS_INPUT : 0x0;

                    /**/ if (jackPortHints.isCV)
                        canvasPortFlags |= PATCHBAY_PORT_TYPE_CV;
                    else if (jackPortHints.isOSC)
                        canvasPortFlags |= PATCHBAY_PORT_TYPE_OSC;
                    else if (jackPortHints.isAudio)
                        canvasPortFlags |= PATCHBAY_PORT_TYPE_AUDIO;
                    else if (jackPortHints.isMIDI)
                        canvasPortFlags |= PATCHBAY_PORT_TYPE_MIDI;

                    id = ++fUsedPorts.lastId;

                    PortNameToId portNameToId;
                    portNameToId.setData(groupId, id, shortPortName, fullPortName);
                    fUsedPorts.list.append(portNameToId);

                    PortToIdData portData;
                    portData.group = groupId;
                    portData.port = id;
                    portData.flags = canvasPortFlags;
                    std::strncpy(portData.strVal, shortPortName, STR_MAX-1);
                    portData.strVal[STR_MAX-1] = '\0';
                    portsCallbackData.append(portData);
                }

                jackbridge_free(ports);
            }

            if (groupsOnly)
                return;

            // query connections, after all ports are in place
            if (const char** const ports = jackbridge_get_ports(fClient, nullptr, nullptr, JackPortIsOutput))
            {
                for (int i=0; ports[i] != nullptr; ++i)
                {
                    const char* const fullPortName(ports[i]);
                    CARLA_SAFE_ASSERT_CONTINUE(fullPortName != nullptr && fullPortName[0] != '\0');

                    const jack_port_t* const jackPort(jackbridge_port_by_name(fClient, fullPortName));
                    CARLA_SAFE_ASSERT_CONTINUE(jackPort != nullptr);

                    const PortNameToId& thisPort(fUsedPorts.getPortNameToId(fullPortName));

                    CARLA_SAFE_ASSERT_CONTINUE(thisPort.group > 0);
                    CARLA_SAFE_ASSERT_CONTINUE(thisPort.port > 0);

                    if (const char** const connections = jackbridge_port_get_all_connections(fClient, jackPort))
                    {
                        for (int j=0; connections[j] != nullptr; ++j)
                        {
                            const char* const connection(connections[j]);
                            CARLA_SAFE_ASSERT_CONTINUE(connection != nullptr && connection[0] != '\0');

                            const PortNameToId& targetPort(fUsedPorts.getPortNameToId(connection));

                            CARLA_SAFE_ASSERT_CONTINUE(targetPort.group > 0);
                            CARLA_SAFE_ASSERT_CONTINUE(targetPort.port > 0);

                            id = ++fUsedConnections.lastId;

                            ConnectionToId connectionToId;
                            connectionToId.setData(id, thisPort.group, thisPort.port, targetPort.group, targetPort.port);
                            fUsedConnections.list.append(connectionToId);

                            ConnectionToIdData connData;
                            connData.id = id;
                            std::snprintf(connData.strVal, STR_MAX-1, "%i:%i:%i:%i",
                                          thisPort.group, thisPort.port, targetPort.group, targetPort.port);
                            connData.strVal[STR_MAX-1] = '\0';
                            connCallbackData.append(connData);
                        }

                        jackbridge_free(connections);
                    }
                }

                jackbridge_free(ports);
            }
        }

        const GroupToIdData groupFallback = { 0, PATCHBAY_ICON_PLUGIN, -1, { '\0' } };
        const PortToIdData portFallback = { 0, 0, 0, { '\0' } };
        const ConnectionToIdData connFallback = { 0, { '\0' } };

        callback(sendHost, sendOSC,
                 ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED,
                 carlaId,
                 PATCHBAY_ICON_CARLA,
                 MAIN_CARLA_PLUGIN_ID,
                 0, 0.0f,
                 ourName);

        {
            const CarlaRecursiveMutexLocker crml(fThreadSafeMetadataMutex);

            for (LinkedList<GroupToIdData>::Itenerator it = groupCallbackData.begin2(); it.valid(); it.next())
            {
                const GroupToIdData& group(it.getValue(groupFallback));

                callback(sendHost, sendOSC,
                        ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED,
                        group.id,
                        group.icon,
                        group.pluginId,
                        0, 0.0f,
                        group.strVal);

                jack_uuid_t uuid;
                {
                    char* const uuidstr = jackbridge_get_uuid_for_client_name(fClient, group.strVal);
                    CARLA_SAFE_ASSERT_CONTINUE(uuidstr != nullptr && uuidstr[0] != '\0');

                    const bool parsed = jackbridge_uuid_parse(uuidstr, &uuid);
                    jackbridge_free(uuidstr);

                    /* if parsing fails, meta-data is not available..
                       this could be because JACK version is old, or perhaps this is an internal client */
                    if (! parsed)
                        continue;
                }

                char* value = nullptr;
                char* type  = nullptr;

                if (jackbridge_get_property(uuid, URI_POSITION, &value, &type)
                    && value != nullptr
                    && type != nullptr
                    && std::strcmp(type, URI_TYPE_STRING) == 0)
                {
                    if (char* sep1 = std::strstr(value, ":"))
                    {
                        int x1, y1 = 0, x2 = 0, y2 = 0;
                        *sep1++ = '\0';
                        x1 = std::atoi(value);

                        if (char* sep2 = std::strstr(sep1, ":"))
                        {
                            *sep2++ = '\0';
                            y1 = std::atoi(sep1);

                            if (char* sep3 = std::strstr(sep2, ":"))
                            {
                                *sep3++ = '\0';
                                x2 = std::atoi(sep2);
                                y2 = std::atoi(sep3);
                            }
                        }

                        jackbridge_free(value);
                        jackbridge_free(type);

                        callback(sendHost, sendOSC,
                                ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED,
                                group.id, x1, y1, x2, static_cast<float>(y2),
                                nullptr);
                    }
                }
            }
        }

        for (LinkedList<PortToIdData>::Itenerator it = portsCallbackData.begin2(); it.valid(); it.next())
        {
            const PortToIdData& port(it.getValue(portFallback));

            callback(sendHost, sendOSC,
                     ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                     port.group,
                     static_cast<int>(port.port),
                     static_cast<int>(port.flags),
                     0, 0.0f,
                     port.strVal);
        }

        for (LinkedList<ConnectionToIdData>::Itenerator it = connCallbackData.begin2(); it.valid(); it.next())
        {
            const ConnectionToIdData& conn(it.getValue(connFallback));

            callback(sendHost, sendOSC,
                     ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED,
                     conn.id,
                     0, 0, 0, 0.0f,
                     conn.strVal);
        }

        groupCallbackData.clear();
        portsCallbackData.clear();
        connCallbackData.clear();
    }
#endif

    // -------------------------------------------------------------------

    void processPlugin(CarlaPluginPtr& plugin, const uint32_t nframes)
    {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        CarlaEngineJackClient* const client = (CarlaEngineJackClient*)plugin->getEngineClient();
        CarlaEngineJackCVSourcePorts& cvSourcePorts(client->getCVSourcePorts());

        const CarlaRecursiveMutexTryLocker crmtl(cvSourcePorts.getMutex(), fFreewheel);
        // const CarlaRecursiveMutexLocker crml(cvSourcePorts.getMutex());
#endif

        /*
        const uint32_t audioInCount  = client->getPortCount(kEnginePortTypeAudio, true);
        const uint32_t audioOutCount = client->getPortCount(kEnginePortTypeAudio, false);
        const uint32_t cvInCount     = client->getPortCount(kEnginePortTypeCV, true);
        const uint32_t cvOutCount    = client->getPortCount(kEnginePortTypeCV, false);
        */

        const uint32_t audioInCount  = plugin->getAudioInCount();
        const uint32_t audioOutCount = plugin->getAudioOutCount();
        const uint32_t cvInCount     = plugin->getCVInCount();
        const uint32_t cvOutCount    = plugin->getCVOutCount();
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        const uint32_t cvsInCount    = crmtl.wasLocked() ? cvSourcePorts.getPortCount() : 0;
#else
        const uint32_t cvsInCount    = 0;
#endif

        const float* audioIn[audioInCount];
        /* */ float* audioOut[audioOutCount];
        const float* cvIn[cvInCount+cvsInCount];
        /* */ float* cvOut[cvOutCount];

        for (uint32_t i=0; i < audioInCount; ++i)
        {
            if (CarlaEngineAudioPort* const port = plugin->getAudioInPort(i))
                audioIn[i] = port->getBuffer();
            else
                audioIn[i] = nullptr;
        }

        for (uint32_t i=0; i < audioOutCount; ++i)
        {
            if (CarlaEngineAudioPort* const port = plugin->getAudioOutPort(i))
                audioOut[i] = port->getBuffer();
            else
                audioOut[i] = nullptr;
        }

        for (uint32_t i=0; i < cvInCount; ++i)
        {
            if (CarlaEngineCVPort* const port = plugin->getCVInPort(i))
                cvIn[i] = port->getBuffer();
            else
                cvIn[i] = nullptr;
        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        for (uint32_t i=cvInCount, j=0; j < cvsInCount; ++i, ++j)
        {
            if (CarlaEngineCVPort* const port = cvSourcePorts.getPort(j))
            {
                port->initBuffer();
                cvIn[i] = port->getBuffer();
            }
            else
            {
                cvIn[i] = nullptr;
            }
        }
#endif

        for (uint32_t i=0; i < cvOutCount; ++i)
        {
            if (CarlaEngineCVPort* const port = plugin->getCVOutPort(i))
                cvOut[i] = port->getBuffer();
            else
                cvOut[i] = nullptr;
        }

        float inPeaks[2] = { 0.0f };
        float outPeaks[2] = { 0.0f };

        for (uint32_t i=0; i < audioInCount && i < 2; ++i)
        {
            for (uint32_t j=0; j < nframes; ++j)
            {
                const float absV(std::abs(audioIn[i][j]));

                if (absV > inPeaks[i])
                    inPeaks[i] = absV;
            }
        }

        plugin->process(audioIn, audioOut, cvIn, cvOut, nframes);

        for (uint32_t i=0; i < audioOutCount && i < 2; ++i)
        {
            for (uint32_t j=0; j < nframes; ++j)
            {
                const float absV(std::abs(audioOut[i][j]));

                if (absV > outPeaks[i])
                    outPeaks[i] = absV;
            }
        }

        setPluginPeaksRT(plugin->getId(), inPeaks, outPeaks);
    }

#ifndef BUILD_BRIDGE
    // -------------------------------------------------------------------

    struct PostPonedJackEvent {
        enum Type {
            kTypeNull = 0,
            kTypeClientUnregister,
            kTypeClientPositionChange,
            kTypePortRegister,
            kTypePortUnregister,
            kTypePortConnect,
            kTypePortDisconnect,
            kTypePortRename
        };

        Type type;

        union {
            struct {
                char name[STR_MAX+1];
            } clientRegister;
            struct {
                char name[STR_MAX+1];
            } clientUnregister;
            struct {
                jack_uuid_t uuid;
            } clientPositionChange;
            struct {
                char shortName[STR_MAX+1];
                char fullName[STR_MAX+1];
                CarlaJackPortHints hints;
            } portRegister;
            struct {
                char fullName[STR_MAX+1];
            } portUnregister;
            struct {
                char oldFullName[STR_MAX+1];
                char newFullName[STR_MAX+1];
                char newShortName[STR_MAX+1];
            } portRename;
            struct {
                char portNameA[STR_MAX+1];
                char portNameB[STR_MAX+1];
            } portConnect;
            struct {
                char portNameA[STR_MAX+1];
                char portNameB[STR_MAX+1];
            } portDisconnect;
        };
    };

    LinkedList<PostPonedJackEvent> fPostPonedEvents;
    CarlaMutex fPostPonedEventsMutex;

    bool fIsInternalClient;

    void postponeJackCallback(PostPonedJackEvent& ev)
    {
        const CarlaMutexLocker cml(fPostPonedEventsMutex);
        fPostPonedEvents.append(ev);
    }

    void run() override
    {
        for (; ! shouldThreadExit();)
        {
            if (fIsInternalClient)
                idle();

            if (fClient == nullptr)
                break;

            d_msleep(200);
        }
    }
#endif //  BUILD_BRIDGE

    // -------------------------------------------------------------------

// disable -Wattributes warnings
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wattributes"
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wattributes"
#endif

    #define handlePtr ((CarlaEngineJack*)arg)

    static void JACKBRIDGE_API carla_jack_thread_init_callback(void*)
    {
#ifdef __SSE2_MATH__
        // Set FTZ and DAZ flags
        _mm_setcsr(_mm_getcsr() | 0x8040);
#endif
    }

    static int JACKBRIDGE_API carla_jack_bufsize_callback(jack_nframes_t newBufferSize, void* arg)
    {
        handlePtr->handleJackBufferSizeCallback(newBufferSize);
        return 0;
    }

    static int JACKBRIDGE_API carla_jack_srate_callback(jack_nframes_t newSampleRate, void* arg)
    {
        handlePtr->handleJackSampleRateCallback(newSampleRate);
        return 0;
    }

    static void JACKBRIDGE_API carla_jack_freewheel_callback(int starting, void* arg)
    {
        handlePtr->handleJackFreewheelCallback(bool(starting));
    }

    static void JACKBRIDGE_API carla_jack_latency_callback(jack_latency_callback_mode_t mode, void* arg)
    {
        handlePtr->handleJackLatencyCallback(mode);
    }

    static int JACKBRIDGE_API carla_jack_process_callback(jack_nframes_t nframes, void* arg) __attribute__((annotate("realtime")))
    {
        handlePtr->handleJackProcessCallback(nframes);
        return 0;
    }

#ifndef BUILD_BRIDGE
    static void JACKBRIDGE_API carla_jack_timebase_callback(jack_transport_state_t, jack_nframes_t nframes, jack_position_t* pos, int new_pos, void* arg) __attribute__((annotate("realtime")))
    {
        handlePtr->handleJackTimebaseCallback(nframes, pos, new_pos);
    }

    static void JACKBRIDGE_API carla_jack_client_registration_callback(const char* name, int reg, void* arg)
    {
        if (reg == 0)
            return;

        PostPonedJackEvent ev;
        carla_zeroStruct(ev);
        ev.type = PostPonedJackEvent::kTypeClientUnregister;
        std::strncpy(ev.clientUnregister.name, name, STR_MAX);
        handlePtr->postponeJackCallback(ev);
    }

    static void JACKBRIDGE_API carla_jack_port_registration_callback(jack_port_id_t port_id, int reg, void* arg)
    {
        const jack_port_t* const port = jackbridge_port_by_id(handlePtr->fClient, port_id);
        CARLA_SAFE_ASSERT_RETURN(port != nullptr,);

        const char* const fullName = jackbridge_port_name(port);
        CARLA_SAFE_ASSERT_RETURN(fullName != nullptr && fullName[0] != '\0',);

        PostPonedJackEvent ev;
        carla_zeroStruct(ev);

        if (reg != 0)
        {
            const char* const shortName = jackbridge_port_short_name(port);
            CARLA_SAFE_ASSERT_RETURN(shortName != nullptr && shortName[0] != '\0',);

            ev.type = PostPonedJackEvent::kTypePortRegister;
            std::strncpy(ev.portRegister.fullName, fullName, STR_MAX);
            std::strncpy(ev.portRegister.shortName, shortName, STR_MAX);

            const CarlaRecursiveMutexLocker crml(handlePtr->fThreadSafeMetadataMutex);
            ev.portRegister.hints = CarlaJackPortHints::fromPort(port);
        }
        else
        {
            ev.type = PostPonedJackEvent::kTypePortUnregister;
            std::strncpy(ev.portUnregister.fullName, fullName, STR_MAX);
        }

        handlePtr->postponeJackCallback(ev);
    }

    static void JACKBRIDGE_API carla_jack_port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
    {
        const jack_port_t* const portA = jackbridge_port_by_id(handlePtr->fClient, a);
        CARLA_SAFE_ASSERT_RETURN(portA != nullptr,);

        const jack_port_t* const portB = jackbridge_port_by_id(handlePtr->fClient, b);
        CARLA_SAFE_ASSERT_RETURN(portB != nullptr,);

        const char* const fullNameA = jackbridge_port_name(portA);
        CARLA_SAFE_ASSERT_RETURN(fullNameA != nullptr && fullNameA[0] != '\0',);

        const char* const fullNameB = jackbridge_port_name(portB);
        CARLA_SAFE_ASSERT_RETURN(fullNameB != nullptr && fullNameB[0] != '\0',);

        PostPonedJackEvent ev;
        carla_zeroStruct(ev);

        if (connect != 0)
        {
            ev.type = PostPonedJackEvent::kTypePortConnect;
            std::strncpy(ev.portConnect.portNameA, fullNameA, STR_MAX);
            std::strncpy(ev.portConnect.portNameB, fullNameB, STR_MAX);
        }
        else
        {
            ev.type = PostPonedJackEvent::kTypePortDisconnect;
            std::strncpy(ev.portDisconnect.portNameA, fullNameA, STR_MAX);
            std::strncpy(ev.portDisconnect.portNameB, fullNameB, STR_MAX);
        }

        handlePtr->postponeJackCallback(ev);
    }

    static void JACKBRIDGE_API carla_jack_port_rename_callback(jack_port_id_t port_id, const char* oldName, const char* newName, void* arg)
    {
        const jack_port_t* const port = jackbridge_port_by_id(handlePtr->fClient, port_id);
        CARLA_SAFE_ASSERT_RETURN(port != nullptr,);

        const char* const shortName = jackbridge_port_short_name(port);
        CARLA_SAFE_ASSERT_RETURN(shortName != nullptr && shortName[0] != '\0',);

        PostPonedJackEvent ev;
        carla_zeroStruct(ev);
        ev.type = PostPonedJackEvent::kTypePortRename;
        std::strncpy(ev.portRename.oldFullName, oldName, STR_MAX);
        std::strncpy(ev.portRename.newFullName, newName, STR_MAX);
        std::strncpy(ev.portRename.newShortName, shortName, STR_MAX);
        handlePtr->postponeJackCallback(ev);
    }

    static void carla_jack_property_change_callback(jack_uuid_t subject, const char* key, jack_property_change_t change, void* arg)
    {
        if (change != PropertyChanged)
            return;
        if (std::strcmp(key, URI_POSITION) != 0)
            return;

        PostPonedJackEvent ev;
        carla_zeroStruct(ev);
        ev.type = PostPonedJackEvent::kTypeClientPositionChange;
        ev.clientPositionChange.uuid = subject;
        handlePtr->postponeJackCallback(ev);
    }

    static int JACKBRIDGE_API carla_jack_xrun_callback(void* arg)
    {
        ++(handlePtr->pData->xruns);
        return 0;
    }
#endif

    static void JACKBRIDGE_API carla_jack_shutdown_callback(void* arg)
    {
        handlePtr->handleJackShutdownCallback();
    }

    #undef handlePtr

    // -------------------------------------------------------------------

#ifndef BUILD_BRIDGE
    static int JACKBRIDGE_API carla_jack_process_callback_plugin(jack_nframes_t nframes, void* arg) __attribute__((annotate("realtime")))
    {
        CarlaPluginPtr* const pluginPtr = static_cast<CarlaPluginPtr*>(arg);
        CARLA_SAFE_ASSERT_RETURN(pluginPtr != nullptr, 0);

        CarlaPluginPtr plugin = *pluginPtr;
        CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr && plugin->isEnabled(), 0);

        CarlaEngineJack* const engine((CarlaEngineJack*)plugin->getEngine());
        CARLA_SAFE_ASSERT_RETURN(engine != nullptr, 0);

        if (plugin->tryLock(engine->fFreewheel))
        {
            plugin->initBuffers();
            engine->processPlugin(plugin, nframes);
            plugin->unlock();
        }

        return 0;
    }

    /*
    static int JACKBRIDGE_API carla_jack_bufsize_callback_plugin(jack_nframes_t nframes, void* arg)
    {
        CarlaPlugin* const plugin((CarlaPlugin*)arg);
        CARLA_SAFE_ASSERT_RETURN(plugin != nullptr && plugin->isEnabled(), 0);

        plugin->bufferSizeChanged(nframes);
        return 1;
    }

    static int JACKBRIDGE_API carla_jack_srate_callback_plugin(jack_nframes_t nframes, void* arg)
    {
        CarlaPlugin* const plugin((CarlaPlugin*)arg);
        CARLA_SAFE_ASSERT_RETURN(plugin != nullptr && plugin->isEnabled(), 0);

        plugin->sampleRateChanged(nframes);
        return 1;
    }
    */

    static void JACKBRIDGE_API carla_jack_latency_callback_plugin(jack_latency_callback_mode_t /*mode*/, void* /*arg*/)
    {
        // TODO
    }

    static void JACKBRIDGE_API carla_jack_shutdown_callback_plugin(void* arg)
    {
        CarlaPluginPtr* const pluginPtr = static_cast<CarlaPluginPtr*>(arg);
        CARLA_SAFE_ASSERT_RETURN(pluginPtr != nullptr,);

        CarlaPluginPtr plugin = *pluginPtr;
        CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr,);

        CarlaEngineJack* const engine((CarlaEngineJack*)plugin->getEngine());
        CARLA_SAFE_ASSERT_RETURN(engine != nullptr,);

        engine->handlePluginJackShutdownCallback(plugin);
    }
#endif

// enable -Wattributes again
#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJack)
};

// -----------------------------------------------------------------------

namespace EngineInit {

CarlaEngine* newJack()
{
    carla_debug("EngineInit::newJack()");
    return new CarlaEngineJack();
}

}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#if defined(JACKBRIDGE_DIRECT) && !defined(BUILD_BRIDGE)
// -----------------------------------------------------------------------
// internal jack client

CARLA_PLUGIN_EXPORT
int jack_initialize(jack_client_t *client, const char *load_init);

CARLA_PLUGIN_EXPORT
void jack_finish(void *arg);

#ifdef CARLA_OS_UNIX
# include "ThreadSafeFFTW.hpp"
#endif

// -----------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
int jack_initialize(jack_client_t* const client, const char* const load_init)
{
    CARLA_BACKEND_USE_NAMESPACE

#ifdef CARLA_OS_UNIX
    static const ThreadSafeFFTW sThreadSafeFFTW;
#endif

    EngineProcessMode mode;
    if (load_init != nullptr && std::strcmp(load_init, "rack") == 0)
        mode = ENGINE_PROCESS_MODE_CONTINUOUS_RACK;
    else
        mode = ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS;

    CarlaEngineJack* const engine = new CarlaEngineJack();

    engine->setOption(ENGINE_OPTION_FORCE_STEREO, 1, nullptr);
    engine->setOption(ENGINE_OPTION_AUDIO_DRIVER, 0, "JACK");
    engine->setOption(ENGINE_OPTION_AUDIO_DEVICE, 0, "Auto-Connect ON");
    engine->setOption(ENGINE_OPTION_OSC_ENABLED,  1, nullptr);
    engine->setOption(ENGINE_OPTION_OSC_PORT_TCP, 22752, nullptr);
    engine->setOption(ENGINE_OPTION_OSC_PORT_UDP, 22752, nullptr);

    engine->setOption(ENGINE_OPTION_PROCESS_MODE, mode, nullptr);
    engine->setOption(ENGINE_OPTION_TRANSPORT_MODE, ENGINE_TRANSPORT_MODE_JACK, nullptr);

#ifdef __MOD_DEVICES__
    engine->setOption(ENGINE_OPTION_FILE_PATH, FILE_AUDIO, "/data/user-files/Audio Loops:/data/user-files/Audio Tracks");
    engine->setOption(ENGINE_OPTION_FILE_PATH, FILE_MIDI, "/data/user-files/MIDI Songs");
#endif

    // FIXME
    engine->setOption(ENGINE_OPTION_PATH_BINARIES, 0, "/usr/lib/carla");
    engine->setOption(ENGINE_OPTION_PATH_RESOURCES, 0, "/usr/share/resources");

    if (engine->initInternal(client))
        return 0;

    delete engine;

    return 1;
}

CARLA_PLUGIN_EXPORT
void jack_finish(void *arg)
{
    CARLA_BACKEND_USE_NAMESPACE

    CarlaEngineJack* const engine = (CarlaEngineJack*)arg;;
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr,);

    engine->setAboutToClose();
    engine->removeAllPlugins();
    engine->close();
    delete engine;
}

// -----------------------------------------------------------------------
#endif // defined(JACKBRIDGE_DIRECT) && !defined(BUILD_BRIDGE)
