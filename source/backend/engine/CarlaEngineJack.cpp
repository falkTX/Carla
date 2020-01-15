/*
 * Carla Plugin Host
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaEngineClient.hpp"
#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"
#include "CarlaPatchbayUtils.hpp"
#include "CarlaStringList.hpp"

#include "jackey.h"

#ifdef USING_JUCE
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Weffc++"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wundef"
# endif

# include "AppConfig.h"
# include "juce_events/juce_events.h"

# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic pop
# endif
#endif

#ifdef __SSE2_MATH__
# include <xmmintrin.h>
#endif

// must be last
#include "jackbridge/JackBridge.hpp"

#define URI_CANVAS_ICON "http://kxstudio.sf.net/ns/canvas/icon"

CARLA_BACKEND_START_NAMESPACE

class CarlaEngineJack;
class CarlaEngineJackClient;

// -----------------------------------------------------------------------
// Fallback data

static const EngineEvent kFallbackJackEngineEvent = { kEngineEventTypeNull, 0, 0, {{ kEngineControlEventTypeNull, 0, 0.0f }} };
//static CarlaEngineEventCV kFallbackEngineEventCV = { nullptr, (uint32_t)-1, 0.0f };

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
    CarlaEngineJackAudioPort(const CarlaEngineClient& client, const bool isInputPort, const uint32_t indexOffset, jack_client_t* const jackClient, jack_port_t* const jackPort, JackPortDeletionCallback* const delCallback) noexcept
        : CarlaEngineAudioPort(client, isInputPort, indexOffset),
          fJackClient(jackClient),
          fJackPort(jackPort),
          kDeletionCallback(delCallback)
    {
        carla_debug("CarlaEngineJackAudioPort::CarlaEngineJackAudioPort(%s, %p, %p)", bool2str(isInputPort), jackClient, jackPort);

        switch (kClient.getEngine().getProccessMode())
        {
        case ENGINE_PROCESS_MODE_SINGLE_CLIENT:
        case ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
            CARLA_SAFE_ASSERT_RETURN(jackClient != nullptr && jackPort != nullptr,);
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            if (const jack_uuid_t uuid = jackbridge_port_uuid(jackPort))
                jackbridge_set_property(jackClient, uuid, JACKEY_SIGNAL_TYPE, "AUDIO", "text/plain");
#endif
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
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            try {
                if (const jack_uuid_t uuid = jackbridge_port_uuid(fJackPort))
                    jackbridge_remove_property(fJackClient, uuid, JACKEY_SIGNAL_TYPE);
            } CARLA_SAFE_EXCEPTION("Audio port remove meta type");
#endif

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

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    void setMetaData(const char* const key, const char* const value, const char* const type) override
    {
        if (fJackPort == nullptr)
            return CarlaEngineJackAudioPort::setMetaData(key, value, type);

        try {
            if (const jack_uuid_t uuid = jackbridge_port_uuid(fJackPort))
                jackbridge_set_property(fJackClient, uuid, key, value, type);
        } CARLA_SAFE_EXCEPTION("Port setMetaData");
    }
#endif

private:
    jack_client_t* fJackClient;
    jack_port_t*   fJackPort;

    JackPortDeletionCallback* const kDeletionCallback;

    friend class CarlaEngineJackClient;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackAudioPort)
};

// -----------------------------------------------------------------------
// Carla Engine JACK-CV port

class CarlaEngineJackCVPort : public CarlaEngineCVPort
{
public:
    CarlaEngineJackCVPort(const CarlaEngineClient& client, const bool isInputPort, const uint32_t indexOffset, jack_client_t* const jackClient, jack_port_t* const jackPort, JackPortDeletionCallback* const delCallback) noexcept
        : CarlaEngineCVPort(client, isInputPort, indexOffset),
          fJackClient(jackClient),
          fJackPort(jackPort),
          kDeletionCallback(delCallback)
    {
        carla_debug("CarlaEngineJackCVPort::CarlaEngineJackCVPort(%s, %p, %p)", bool2str(isInputPort), jackClient, jackPort);

        switch (kClient.getEngine().getProccessMode())
        {
        case ENGINE_PROCESS_MODE_SINGLE_CLIENT:
        case ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
            CARLA_SAFE_ASSERT_RETURN(jackClient != nullptr && jackPort != nullptr,);
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            if (const jack_uuid_t uuid = jackbridge_port_uuid(jackPort))
                jackbridge_set_property(jackClient, uuid, JACKEY_SIGNAL_TYPE, "CV", "text/plain");
#endif
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
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            try {
                if (const jack_uuid_t uuid = jackbridge_port_uuid(fJackPort))
                    jackbridge_remove_property(fJackClient, uuid, JACKEY_SIGNAL_TYPE);
            } CARLA_SAFE_EXCEPTION("CV port remove meta type");
#endif

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

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    void setMetaData(const char* const key, const char* const value, const char* const type) override
    {
        if (fJackPort == nullptr)
            return CarlaEngineCVPort::setMetaData(key, value, type);

        try {
            if (const jack_uuid_t uuid = jackbridge_port_uuid(fJackPort))
                jackbridge_set_property(fJackClient, uuid, key, value, type);
        } CARLA_SAFE_EXCEPTION("Port setMetaData");
    }
#endif

private:
    jack_client_t* fJackClient;
    jack_port_t*   fJackPort;

    JackPortDeletionCallback* const kDeletionCallback;

    friend class CarlaEngineJackClient;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackCVPort)
};

// -----------------------------------------------------------------------
// Carla Engine JACK-Event port

class CarlaEngineJackEventPort : public CarlaEngineEventPort
{
public:
    CarlaEngineJackEventPort(const CarlaEngineClient& client, const bool isInputPort, const uint32_t indexOffset, jack_client_t* const jackClient, jack_port_t* const jackPort, JackPortDeletionCallback* const delCallback) noexcept
        : CarlaEngineEventPort(client, isInputPort, indexOffset),
          fJackClient(jackClient),
          fJackPort(jackPort),
          fJackBuffer(nullptr),
          fRetEvent(kFallbackJackEngineEvent),
          fCvSourceEvents(nullptr),
          fCvSourceEventCount(0),
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

        fCvSourceEvents = nullptr;
        fCvSourceEventCount = 0;

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

    void setCvSourceEvents(EngineEvent* const events, const uint32_t eventCount) noexcept
    {
        fCvSourceEvents = events;
        fCvSourceEventCount = eventCount;
    }

    uint32_t getEventCount() const noexcept override
    {
        if (fJackPort == nullptr)
            return CarlaEngineEventPort::getEventCount();

        CARLA_SAFE_ASSERT_RETURN(kIsInput, 0);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, 0);

        try {
            return jackbridge_midi_get_event_count(fJackBuffer) + fCvSourceEventCount;
        } CARLA_SAFE_EXCEPTION_RETURN("jack_midi_get_event_count", 0);
    }

    const EngineEvent& getEvent(const uint32_t index) const noexcept override
    {
        if (fJackPort == nullptr)
            return CarlaEngineEventPort::getEvent(index);

        CARLA_SAFE_ASSERT_RETURN(kIsInput, kFallbackJackEngineEvent);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, kFallbackJackEngineEvent);

        return getEventUnchecked(index);
    }

    const EngineEvent& getEventUnchecked(uint32_t index) const noexcept override
    {
        if (index < fCvSourceEventCount)
            return fCvSourceEvents[index];

        index -= fCvSourceEventCount;

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

    bool writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const float value) noexcept override
    {
        if (fJackPort == nullptr)
            return CarlaEngineEventPort::writeControlEvent(time, channel, type, param, value);

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

        EngineControlEvent ctrlEvent = { type, param, value };
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

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    void setMetaData(const char* const key, const char* const value, const char* const type) override
    {
        if (fJackPort == nullptr)
            return CarlaEngineJackEventPort::setMetaData(key, value, type);

        try {
            if (const jack_uuid_t uuid = jackbridge_port_uuid(fJackPort))
                jackbridge_set_property(fJackClient, uuid, key, value, type);
        } CARLA_SAFE_EXCEPTION("Port setMetaData");
    }
#endif

private:
    jack_client_t* fJackClient;
    jack_port_t*   fJackPort;
    void*          fJackBuffer;

    mutable EngineEvent fRetEvent;

    EngineEvent* fCvSourceEvents;
    uint32_t fCvSourceEventCount;

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

    bool addCVSource(CarlaEngineCVPort* const port, const uint32_t portIndexOffset) override
    {
        if (! fUseClient)
            return CarlaEngineCVSourcePorts::addCVSource(port, portIndexOffset);

        const CarlaRecursiveMutexLocker crml(pData->rmutex);

        if (! CarlaEngineCVSourcePorts::addCVSource(port, portIndexOffset))
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

                event.ctrl.type  = kEngineControlEventTypeParameter;
                event.ctrl.param = static_cast<uint16_t>(ecv.indexOffset);
                event.ctrl.value = carla_fixedValue(0.0f, 1.0f, (v - min) / (max - min));
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

    void setGraphAndPlugin(PatchbayGraph* const graph, CarlaPlugin* const plugin) noexcept
    {
        pData->graph = graph;
        pData->plugin = plugin;
    }

private:
    const bool fUseClient;
    EngineEvent* fBuffer;
    EngineEvent* fBufferToDeleteLater;

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEngineJackCVSourcePorts)
};
#endif

// -----------------------------------------------------------------------
// Jack Engine client

class CarlaEngineJackClient : public CarlaEngineClientForSubclassing,
                              private JackPortDeletionCallback
{
public:
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CarlaEngineJackClient(const CarlaEngine& engine, EngineInternalGraph& egraph, CarlaPlugin* const plugin, jack_client_t* const jackClient)
        : CarlaEngineClientForSubclassing(engine, egraph, plugin),
#else
    CarlaEngineJackClient(const CarlaEngine& engine, jack_client_t* const jackClient)
        : CarlaEngineClientForSubclassing(engine),
#endif
          fJackClient(jackClient),
          fUseClient(engine.getProccessMode() == ENGINE_PROCESS_MODE_SINGLE_CLIENT || engine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS),
          fAudioPorts(),
          fCVPorts(),
          fEventPorts(),
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
          fCVSourcePorts(fUseClient),
#endif
          fPreRenameMutex(),
          fPreRenameConnections()
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
        }

        fPreRenameConnections.clear();
    }

    void deactivate() noexcept override
    {
        carla_debug("CarlaEngineJackClient::deactivate()");

        if (getProcessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_SAFE_ASSERT_RETURN(fJackClient != nullptr && isActive(),);

            try {
                jackbridge_deactivate(fJackClient);
            } catch(...) {}
        }

        CarlaEngineClient::deactivate();
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
                jackPort = jackbridge_port_register(fJackClient, realName, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            case kEnginePortTypeCV:
                jackPort = jackbridge_port_register(fJackClient, realName, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            case kEnginePortTypeEvent:
                jackPort = jackbridge_port_register(fJackClient, realName, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
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
            CarlaEngineJackAudioPort* const enginePort(new CarlaEngineJackAudioPort(*this, isInput, indexOffset, fJackClient, jackPort, this));
            fAudioPorts.append(enginePort);
            return enginePort;
        }
        case kEnginePortTypeCV: {
            pData->addCVPortName(isInput, realName);
            if (realName != name) delete[] realName;
            CarlaEngineJackCVPort* const enginePort(new CarlaEngineJackCVPort(*this, isInput, indexOffset, fJackClient, jackPort, this));
            fCVPorts.append(enginePort);
            return enginePort;
        }
        case kEnginePortTypeEvent: {
            pData->addEventPortName(isInput, realName);
            if (realName != name) delete[] realName;
            CarlaEngineJackEventPort* const enginePort(new CarlaEngineJackEventPort(*this, isInput, indexOffset, fJackClient, jackPort, this));
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
        fCVSourcePorts.setGraphAndPlugin(getPatchbayGraph(), getPlugin());
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
        CarlaEngineClient::deactivate();
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

    bool renameInSingleClient(const CarlaString& newClientName)
    {
        const CarlaString clientNamePrefix(newClientName + ":");

        return _renamePorts(fAudioPorts, clientNamePrefix) &&
               _renamePorts(fCVPorts,    clientNamePrefix) &&
               _renamePorts(fEventPorts, clientNamePrefix);
    }

    void closeForRename(jack_client_t* const newClient, const CarlaString& newClientName) noexcept
    {
        if (fJackClient != nullptr)
        {
            if (isActive())
            {
                {
                    const CarlaString clientNamePrefix(newClientName + ":");

                    // store current client connections
                    const CarlaMutexLocker cml(fPreRenameMutex);

                    fPreRenameConnections.clear();

                    _savePortsConnections(fAudioPorts, clientNamePrefix);
                    _savePortsConnections(fCVPorts,    clientNamePrefix);
                    _savePortsConnections(fEventPorts, clientNamePrefix);
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

private:
    jack_client_t* fJackClient;
    const bool     fUseClient;

    LinkedList<CarlaEngineJackAudioPort*> fAudioPorts;
    LinkedList<CarlaEngineJackCVPort*>    fCVPorts;
    LinkedList<CarlaEngineJackEventPort*> fEventPorts;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CarlaEngineJackCVSourcePorts fCVSourcePorts;
#endif

    CarlaMutex      fPreRenameMutex;
    CarlaStringList fPreRenameConnections;

    template<typename T>
    bool _renamePorts(const LinkedList<T*>& t, const CarlaString& clientNamePrefix)
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

            const CarlaString newPortName(clientNamePrefix + shortPortName);

            if (! jackbridge_port_rename(fJackClient, port->fJackPort, newPortName))
                return false;
        }

        return true;
    }

    template<typename T>
    void _savePortsConnections(const LinkedList<T*>& t, const CarlaString& clientNamePrefix)
    {
        for (typename LinkedList<T*>::Itenerator it = t.begin2(); it.valid(); it.next())
        {
            T* const port(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(port != nullptr);
            CARLA_SAFE_ASSERT_CONTINUE(port->fJackPort != nullptr);

            const char* const shortPortName(jackbridge_port_short_name(port->fJackPort));
            CARLA_SAFE_ASSERT_CONTINUE(shortPortName != nullptr && shortPortName[0] != '\0');

            const CarlaString portName(clientNamePrefix + shortPortName);

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
#ifdef BUILD_BRIDGE
          fIsRunning(false)
#else
          fTimebaseMaster(false),
          fTimebaseRolling(false),
          fTimebaseUsecs(0),
          fUsedGroups(),
          fUsedPorts(),
          fUsedConnections(),
          fPatchbayProcThreadProtectionMutex(),
          fRetConns(),
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
        if (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT || pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
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
        CARLA_SAFE_ASSERT_RETURN(jackbridge_is_ok(), false);
        carla_debug("CarlaEngineJack::init(\"%s\")", clientName);

        fFreewheel = false;
        fExternalPatchbayHost = true;
        fExternalPatchbayOsc  = true;

        CarlaString truncatedClientName;

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
#else
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

        const EngineOptions& opts(pData->options);

        pData->bufferSize = jackbridge_get_buffer_size(fClient);
        pData->sampleRate = jackbridge_get_sample_rate(fClient);
        pData->initTime(opts.transportExtra);

        jackbridge_set_thread_init_callback(fClient, carla_jack_thread_init_callback, nullptr);
        jackbridge_set_buffer_size_callback(fClient, carla_jack_bufsize_callback, this);
        jackbridge_set_sample_rate_callback(fClient, carla_jack_srate_callback, this);
        jackbridge_set_freewheel_callback(fClient, carla_jack_freewheel_callback, this);
        jackbridge_set_latency_callback(fClient, carla_jack_latency_callback, this);
        jackbridge_set_process_callback(fClient, carla_jack_process_callback, this);
        jackbridge_on_shutdown(fClient, carla_jack_shutdown_callback, this);

        fTimebaseRolling = false;

        if (opts.transportMode == ENGINE_TRANSPORT_MODE_JACK)
            fTimebaseMaster = jackbridge_set_timebase_callback(fClient, true, carla_jack_timebase_callback, this);
        else
            fTimebaseMaster = false;

        if (opts.processMode != ENGINE_PROCESS_MODE_PATCHBAY)
            initJackPatchbay(true, false, jackClientName);

        jackbridge_set_client_registration_callback(fClient, carla_jack_client_registration_callback, this);
        jackbridge_set_port_registration_callback(fClient, carla_jack_port_registration_callback, this);
        jackbridge_set_port_connect_callback(fClient, carla_jack_port_connect_callback, this);
        jackbridge_set_port_rename_callback(fClient, carla_jack_port_rename_callback, this);
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

        if (const char* const uuidchar = jackbridge_client_get_uuid(fClient))
        {
            jack_uuid_t uuid;

            if (jackbridge_uuid_parse(uuidchar, &uuid))
            {
#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
                const CarlaString& tcp(pData->osc.getServerPathTCP());
                const CarlaString& udp(pData->osc.getServerPathUDP());

                if (tcp.isNotEmpty())
                    jackbridge_set_property(fClient, uuid,
                                            "https://kx.studio/ns/carla/osc-tcp", tcp.buffer(), "text/plain");

                if (tcp.isNotEmpty())
                    jackbridge_set_property(fClient, uuid,
                                            "https://kx.studio/ns/carla/osc-udp", udp.buffer(), "text/plain");
#endif
            }
        }

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
#endif
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
        stopThread(-1);
        fPostPonedEvents.clear();

        CARLA_SAFE_ASSERT_RETURN_ERR(fClient != nullptr, "JACK Client is null");

        // deactivate and close client
        jackbridge_deactivate(fClient);
        jackbridge_client_close(fClient);

        // clear engine data
        CarlaEngine::close();

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

        fClient = nullptr;
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
        if (action == ENGINE_CALLBACK_PROJECT_LOAD_FINISHED && fTimebaseMaster)
        {
            // project finished loading, need to set bpm here, so we force an update of timebase master
            transportRelocate(pData->timeInfo.frame);
        }

        CarlaEngine::callback(sendHost, sendOsc, action, pluginId, value1, value2, value3, valuef, valueStr);
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
            timeInfo.bbt.tick           = jpos.tick;
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
            CARLA_SAFE_ASSERT_RETURN(value >= ENGINE_TRANSPORT_MODE_DISABLED && value <= ENGINE_TRANSPORT_MODE_JACK,);

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

        CarlaEngine::setOption(option, value, valueStr);
    }
#endif

    CarlaEngineClient* addClient(CarlaPlugin* const plugin) override
    {
        jack_client_t* client = nullptr;

#ifndef BUILD_BRIDGE
        if (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
        {
            client = fClient;
        }
        else if (pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
        {
            client = jackbridge_client_open(plugin->getName(), JackNoStartServer, nullptr);

            CARLA_SAFE_ASSERT_RETURN(client != nullptr, nullptr);

            jackbridge_set_thread_init_callback(client, carla_jack_thread_init_callback, nullptr);

#ifndef BUILD_BRIDGE
            /*
            jackbridge_set_buffer_size_callback(fClient, carla_jack_bufsize_callback_plugin, plugin);
            jackbridge_set_sample_rate_callback(fClient, carla_jack_srate_callback_plugin, plugin);
            */
            jackbridge_set_latency_callback(client, carla_jack_latency_callback_plugin, plugin);
            jackbridge_set_process_callback(client, carla_jack_process_callback_plugin, plugin);
            jackbridge_on_shutdown(client, carla_jack_shutdown_callback_plugin, plugin);

            if (const char* const uuidchar = jackbridge_client_get_uuid(client))
            {
                jack_uuid_t uuid;

                if (jackbridge_uuid_parse(uuidchar, &uuid))
                {
                    char strBufId[24];
                    std::snprintf(strBufId, 24, "%u", plugin->getId());
                    strBufId[23] = '\0';

                    jackbridge_set_property(client, uuid,
                                            "https://kx.studio/ns/carla/plugin-id",
                                            strBufId,
                                            "http://www.w3.org/2001/XMLSchema#integer");

                    if (const char* const pluginIcon = plugin->getIconName())
                        jackbridge_set_property(client, uuid,
                                                "https://kx.studio/ns/carla/plugin-icon",
                                                pluginIcon,
                                                "text/plain");
                }
            }
#else
            fClient = client;
            pData->bufferSize = jackbridge_get_buffer_size(client);
            pData->sampleRate = jackbridge_get_sample_rate(client);
            pData->initTime(nullptr);

            jackbridge_set_buffer_size_callback(client, carla_jack_bufsize_callback, this);
            jackbridge_set_sample_rate_callback(client, carla_jack_srate_callback, this);
            jackbridge_set_freewheel_callback(client, carla_jack_freewheel_callback, this);
            jackbridge_set_latency_callback(client, carla_jack_latency_callback, this);
            jackbridge_set_process_callback(client, carla_jack_process_callback, this);
            jackbridge_on_shutdown(client, carla_jack_shutdown_callback, this);
#endif
        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        return new CarlaEngineJackClient(*this, pData->graph, plugin, client);
#else
        return new CarlaEngineJackClient(*this, client);
#endif
    }

#ifndef BUILD_BRIDGE
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

        CarlaPlugin* const plugin(pData->plugins[id].plugin);
        CARLA_SAFE_ASSERT_RETURN_ERR(plugin != nullptr, "Could not find plugin to rename");
        CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data");

        // before we stop the engine thread we might need to get the plugin data
        const bool needsReinit = (pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS);
        const CarlaStateSave* saveStatePtr = nullptr;

        if (needsReinit)
        {
            const CarlaStateSave& saveState(plugin->getStateSave());
            saveStatePtr = &saveState;
        }

        CarlaString uniqueName;

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

        const ScopedThreadStopper sts(this);

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
            if (jack_client_t* const jackClient = jackbridge_client_open(uniqueName, JackNoStartServer, nullptr))
            {
                // get new client name
                uniqueName = jackbridge_get_client_name(jackClient);

                // close client
                client->closeForRename(jackClient, uniqueName);

                // disable plugin
                plugin->setEnabled(false);

                // set new client data
                jackbridge_set_latency_callback(jackClient, carla_jack_latency_callback_plugin, plugin);
                jackbridge_set_process_callback(jackClient, carla_jack_process_callback_plugin, plugin);
                jackbridge_on_shutdown(jackClient, carla_jack_shutdown_callback_plugin, plugin);

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

                stopThread(-1);

                const uint groupId(fUsedGroups.getGroupId(plugin->getName()));

                if (groupId > 0)
                {
                    for (LinkedList<PortNameToId>::Itenerator it = fUsedPorts.list.begin2(); it.valid(); it.next())
                    {
                        for (LinkedList<ConnectionToId>::Itenerator it2 = fUsedConnections.list.begin2(); it2.valid(); it2.next())
                        {
                            static ConnectionToId connectionFallback = { 0, 0, 0, 0, 0 };

                            ConnectionToId& connectionToId = it2.getValue(connectionFallback);
                            CARLA_SAFE_ASSERT_CONTINUE(connectionToId.id != 0);

                            if (connectionToId.groupA != groupId && connectionToId.groupB != groupId)
                                continue;

                            callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                                     ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED,
                                     connectionToId.id,
                                     0, 0, 0, 0.0f, nullptr);
                            fUsedConnections.list.remove(it2);
                        }

                        static PortNameToId portNameFallback = { 0, 0, { '\0' }, { '\0' } };

                        PortNameToId& portNameToId(it.getValue(portNameFallback));
                        CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group != 0);

                        if (portNameToId.group != groupId)
                            continue;

                        callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                                 ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED,
                                 portNameToId.group,
                                 static_cast<int>(portNameToId.port),
                                 0, 0, 0.0f, nullptr);
                        fUsedPorts.list.remove(it);
                    }
                }

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
                connectionToId = it.getValue(connectionToId);
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
                return CarlaEngine::patchbayRefresh(sendHost, sendOSC, false);
        }

        fUsedGroups.clear();
        fUsedPorts.clear();
        fUsedConnections.clear();

        initJackPatchbay(sendHost, sendOSC, jackbridge_get_client_name(fClient));
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
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, nullptr);
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

    void restorePatchbayConnection(const bool external, const char* const connSource, const char* const connTarget) override
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
        CarlaPlugin* const plugin(pData->plugins[0].plugin);

        if (plugin != nullptr && plugin->isEnabled() && plugin->tryLock(fFreewheel))
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
                    timeInfo.bbt.tick           = jpos.tick;
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
                CarlaPlugin* const plugin(pData->plugins[i].plugin);

                if (plugin != nullptr && plugin->isEnabled() && plugin->tryLock(fFreewheel))
                {
                    plugin->initBuffers();
                    processPlugin(plugin, nframes);
                    plugin->unlock();
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

                            // copy rest
                            carla_copy<uint8_t>(mdataTmp+1, midiEvent.data+1, size-1U);

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

    void handleJackClientRegistrationCallback(const char* const name, const bool reg)
    {
        CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

        // ignore this if on internal patchbay mode
#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (! (fExternalPatchbayHost || (fExternalPatchbayOsc && pData->osc.isControlRegisteredForTCP()))) return;
#else
        if (! fExternalPatchbayHost) return;
#endif

        // do nothing on client registration, wait for first port
        if (reg) return;

        const uint groupId(fUsedGroups.getGroupId(name));

        // clients might have been registered without ports
        if (groupId == 0) return;

        GroupNameToId groupNameToId;
        groupNameToId.setData(groupId, name);

        callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                 ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED, groupNameToId.group, 0, 0, 0, 0.0f, nullptr);

        fUsedGroups.list.removeOne(groupNameToId);
    }

    void handleJackPortRegistrationCallback(const jack_port_id_t port, const bool reg)
    {
        // ignore this if on internal patchbay mode
#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (! (fExternalPatchbayHost || (fExternalPatchbayOsc && pData->osc.isControlRegisteredForTCP()))) return;
#else
        if (! fExternalPatchbayHost) return;
#endif

        const jack_port_t* const jackPort(jackbridge_port_by_id(fClient, port));
        CARLA_SAFE_ASSERT_RETURN(jackPort != nullptr,);

        const char* const fullPortName(jackbridge_port_name(jackPort));
        CARLA_SAFE_ASSERT_RETURN(fullPortName != nullptr && fullPortName[0] != '\0',);

        if (reg)
        {
            const char* const shortPortName(jackbridge_port_short_name(jackPort));
            CARLA_SAFE_ASSERT_RETURN(shortPortName != nullptr && shortPortName[0] != '\0',);

            bool found;
            CarlaString groupName(fullPortName);
            groupName.truncate(groupName.rfind(shortPortName, &found)-1);

            CARLA_SAFE_ASSERT_RETURN(found,);

            const int jackPortFlags(jackbridge_port_flags(jackPort));

            uint groupId(fUsedGroups.getGroupId(groupName));

            if (groupId == 0)
            {
                groupId = ++fUsedGroups.lastId;

                GroupNameToId groupNameToId;
                groupNameToId.setData(groupId, groupName);

                int pluginId = -1;
                PatchbayIcon icon = (jackPortFlags & JackPortIsPhysical) ? PATCHBAY_ICON_HARDWARE : PATCHBAY_ICON_APPLICATION;

                findPluginIdAndIcon(groupName, pluginId, icon);

                callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                         ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED,
                         groupNameToId.group,
                         icon,
                         pluginId,
                         0, 0.0f,
                         groupNameToId.name);

                fUsedGroups.list.append(groupNameToId);
            }

            addPatchbayJackPort(fExternalPatchbayHost, fExternalPatchbayOsc,
                                groupId, jackPort, shortPortName, fullPortName, jackPortFlags);
        }
        else
        {
            const PortNameToId& portNameToId(fUsedPorts.getPortNameToId(fullPortName));

            /* NOTE: Due to JACK2 async behaviour the port we get here might be the same of a previous rename-plugin request.
                     See the comment on CarlaEngineJack::renamePlugin() for more information. */
            if (portNameToId.group <= 0 || portNameToId.port <= 0) return;

            callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                     ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED,
                     portNameToId.group,
                     static_cast<int>(portNameToId.port),
                     0, 0, 0.0f, nullptr);
            fUsedPorts.list.removeOne(portNameToId);
        }
    }

    void handleJackPortConnectCallback(const jack_port_id_t a, const jack_port_id_t b, const bool connect)
    {
        // ignore this if on internal patchbay mode
#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (! (fExternalPatchbayHost || (fExternalPatchbayOsc && pData->osc.isControlRegisteredForTCP()))) return;
#else
        if (! fExternalPatchbayHost) return;
#endif

        const jack_port_t* const jackPortA(jackbridge_port_by_id(fClient, a));
        CARLA_SAFE_ASSERT_RETURN(jackPortA != nullptr,);

        const jack_port_t* const jackPortB(jackbridge_port_by_id(fClient, b));
        CARLA_SAFE_ASSERT_RETURN(jackPortB != nullptr,);

        const char* const fullPortNameA(jackbridge_port_name(jackPortA));
        CARLA_SAFE_ASSERT_RETURN(fullPortNameA != nullptr && fullPortNameA[0] != '\0',);

        const char* const fullPortNameB(jackbridge_port_name(jackPortB));
        CARLA_SAFE_ASSERT_RETURN(fullPortNameB != nullptr && fullPortNameB[0] != '\0',);

        const PortNameToId& portNameToIdA(fUsedPorts.getPortNameToId(fullPortNameA));
        const PortNameToId& portNameToIdB(fUsedPorts.getPortNameToId(fullPortNameB));

        /* NOTE: Due to JACK2 async behaviour the port we get here might be the same of a previous rename-plugin request.
                 See the comment on CarlaEngineJack::renamePlugin() for more information. */
        if (portNameToIdA.group <= 0 || portNameToIdA.port <= 0) return;
        if (portNameToIdB.group <= 0 || portNameToIdB.port <= 0) return;

        if (connect)
        {
            char strBuf[STR_MAX+1];
            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", portNameToIdA.group, portNameToIdA.port, portNameToIdB.group, portNameToIdB.port);
            strBuf[STR_MAX] = '\0';

            ConnectionToId connectionToId;
            connectionToId.setData(++fUsedConnections.lastId, portNameToIdA.group, portNameToIdA.port, portNameToIdB.group, portNameToIdB.port);

            callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                     ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED,
                     connectionToId.id,
                     0, 0, 0, 0.0f,
                     strBuf);
            fUsedConnections.list.append(connectionToId);
        }
        else
        {
            ConnectionToId connectionToId = { 0, 0, 0, 0, 0 };
            bool found = false;

            {
                const CarlaMutexLocker cml(fUsedConnections.mutex);

                for (LinkedList<ConnectionToId>::Itenerator it = fUsedConnections.list.begin2(); it.valid(); it.next())
                {
                    connectionToId = it.getValue(connectionToId);
                    CARLA_SAFE_ASSERT_CONTINUE(connectionToId.id != 0);

                    if (connectionToId.groupA == portNameToIdA.group && connectionToId.portA == portNameToIdA.port &&
                        connectionToId.groupB == portNameToIdB.group && connectionToId.portB == portNameToIdB.port)
                    {
                        found = true;
                        fUsedConnections.list.remove(it);
                        break;
                    }
                }
            }

            if (found) {
                callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                         ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED,
                         connectionToId.id,
                         0, 0, 0, 0.0f, nullptr);
            }
        }
    }

    void handleJackPortRenameCallback(const jack_port_id_t port, const char* const oldFullName, const char* const newFullName)
    {
        // ignore this if on internal patchbay mode
#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (! (fExternalPatchbayHost || (fExternalPatchbayOsc && pData->osc.isControlRegisteredForTCP()))) return;
#else
        if (! fExternalPatchbayHost) return;
#endif

        CARLA_SAFE_ASSERT_RETURN(oldFullName != nullptr && oldFullName[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(newFullName != nullptr && newFullName[0] != '\0',);

        const jack_port_t* const jackPort(jackbridge_port_by_id(fClient, port));
        CARLA_SAFE_ASSERT_RETURN(jackPort != nullptr,);

        const char* const shortPortName(jackbridge_port_short_name(jackPort));
        CARLA_SAFE_ASSERT_RETURN(shortPortName != nullptr && shortPortName[0] != '\0',);

        bool found;
        CarlaString groupName(newFullName);
        groupName.truncate(groupName.rfind(shortPortName, &found)-1);

        CARLA_SAFE_ASSERT_RETURN(found,);

        const uint groupId(fUsedGroups.getGroupId(groupName));
        CARLA_SAFE_ASSERT_RETURN(groupId > 0,);

        for (LinkedList<PortNameToId>::Itenerator it = fUsedPorts.list.begin2(); it.valid(); it.next())
        {
            static PortNameToId portNameFallback = { 0, 0, { '\0' }, { '\0' } };

            PortNameToId& portNameToId(it.getValue(portNameFallback));
            CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group != 0);

            if (std::strncmp(portNameToId.fullName, oldFullName, STR_MAX) == 0)
            {
                CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group == groupId);

                portNameToId.rename(shortPortName, newFullName);
                callback(fExternalPatchbayHost, fExternalPatchbayOsc,
                         ENGINE_CALLBACK_PATCHBAY_PORT_CHANGED,
                         portNameToId.group,
                         static_cast<int>(portNameToId.port),
                         0, 0, 0.0f,
                         portNameToId.name);
                break;
            }
        }
    }
#endif

    void handleJackShutdownCallback()
    {
#ifndef BUILD_BRIDGE
        signalThreadShouldExit();
#endif

        const PendingRtEventsRunner prt(this, pData->bufferSize);

        for (uint i=0; i < pData->curPluginCount; ++i)
        {
            if (CarlaPlugin* const plugin = pData->plugins[i].plugin)
            {
                plugin->tryLock(true);

                if (CarlaEngineJackClient* const client = (CarlaEngineJackClient*)plugin->getEngineClient())
                    client->invalidate();

                plugin->unlock();
            }
        }

        fClient = nullptr;
#ifndef BUILD_BRIDGE
        carla_zeroPointers(fRackPorts, kRackPortCount);
#endif

        callback(true, true, ENGINE_CALLBACK_QUIT, 0, 0, 0, 0, 0.0f, nullptr);
    }

    // -------------------------------------------------------------------

    void handlePluginJackShutdownCallback(CarlaPlugin* const plugin)
    {
        CarlaEngineJackClient* const engineClient((CarlaEngineJackClient*)plugin->getEngineClient());
        CARLA_SAFE_ASSERT_RETURN(engineClient != nullptr,);

        plugin->tryLock(true);
        engineClient->invalidate();
        plugin->unlock();

        //if (pData->nextAction.pluginId == plugin->getId())
        //    pData->nextAction.clearAndReset();

        callback(true, true, ENGINE_CALLBACK_PLUGIN_UNAVAILABLE, plugin->getId(), 0, 0, 0, 0.0f, "Killed by JACK");
    }

    // -------------------------------------------------------------------

private:
    jack_client_t* fClient;
    bool fExternalPatchbayHost;
    bool fExternalPatchbayOsc;
    bool fFreewheel;

    // -------------------------------------------------------------------

#ifdef BUILD_BRIDGE
    bool fIsRunning;
#else
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

        const char* const uuidstr = jackbridge_get_uuid_for_client_name(fClient, clientName);
        CARLA_SAFE_ASSERT_RETURN(uuidstr != nullptr && uuidstr[0] != '\0',);

        jack_uuid_t uuid;
        CARLA_SAFE_ASSERT_RETURN(jackbridge_uuid_parse(uuidstr, &uuid),);

        {
            char* value = nullptr;
            char* type = nullptr;

            if (! jackbridge_get_property(uuid, "https://kx.studio/ns/carla/plugin-id", &value, &type))
                return;

            CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
            CARLA_SAFE_ASSERT_RETURN(value != nullptr && value[0] != '\0',);
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(type, "http://www.w3.org/2001/XMLSchema#integer") == 0,);

            pluginId = std::atoi(value);
            icon     = PATCHBAY_ICON_PLUGIN;
        }

        {
            char* value = nullptr;
            char* type = nullptr;

            if (! jackbridge_get_property(uuid, "https://kx.studio/ns/carla/plugin-icon", &value, &type))
                return;

            CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
            CARLA_SAFE_ASSERT_RETURN(value != nullptr && value[0] != '\0',);
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(type, "text/plain") == 0,);

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
        }
    }

    void initJackPatchbay(const bool sendHost, const bool sendOSC, const char* const ourName)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options.processMode != ENGINE_PROCESS_MODE_PATCHBAY ||
                                 (fExternalPatchbayHost && sendHost) || (fExternalPatchbayOsc && sendOSC),);
        CARLA_SAFE_ASSERT_RETURN(ourName != nullptr && ourName[0] != '\0',);

        CarlaStringList parsedGroups;

        // add our client first
        {
            parsedGroups.append(ourName);

            GroupNameToId groupNameToId;
            groupNameToId.setData(++fUsedGroups.lastId, ourName);

            callback(sendHost, sendOSC,
                     ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED,
                     groupNameToId.group,
                     PATCHBAY_ICON_CARLA,
                     MAIN_CARLA_PLUGIN_ID,
                     0, 0.0f,
                     groupNameToId.name);
            fUsedGroups.list.append(groupNameToId);
        }

        // query all jack ports
        {
            const char** const ports = jackbridge_get_ports(fClient, nullptr, nullptr, 0);
            CARLA_SAFE_ASSERT_RETURN(ports != nullptr,);

            for (int i=0; ports[i] != nullptr; ++i)
            {
                const char* const fullPortName(ports[i]);
                CARLA_SAFE_ASSERT_CONTINUE(fullPortName != nullptr && fullPortName[0] != '\0');

                const jack_port_t* const jackPort(jackbridge_port_by_name(fClient, fullPortName));
                CARLA_SAFE_ASSERT_CONTINUE(jackPort != nullptr);

                const char* const shortPortName(jackbridge_port_short_name(jackPort));
                CARLA_SAFE_ASSERT_CONTINUE(shortPortName != nullptr && shortPortName[0] != '\0');

                const int jackPortFlags(jackbridge_port_flags(jackPort));

                uint groupId = 0;

                bool found;
                CarlaString groupName(fullPortName);
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
                    PatchbayIcon icon = (jackPortFlags & JackPortIsPhysical) ? PATCHBAY_ICON_HARDWARE : PATCHBAY_ICON_APPLICATION;

                    findPluginIdAndIcon(groupName, pluginId, icon);

                    callback(sendHost, sendOSC,
                             ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED,
                             groupNameToId.group,
                             icon,
                             pluginId,
                             0, 0.0f,
                             groupNameToId.name);
                    fUsedGroups.list.append(groupNameToId);
                }

                addPatchbayJackPort(sendHost, sendOSC, groupId, jackPort, shortPortName, fullPortName, jackPortFlags);
            }

            jackbridge_free(ports);
        }

        // query connections, after all ports are in place
        if (const char** const ports = jackbridge_get_ports(fClient, nullptr, nullptr, JackPortIsOutput))
        {
            char strBuf[STR_MAX+1];

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

                        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", thisPort.group, thisPort.port, targetPort.group, targetPort.port);
                        strBuf[STR_MAX] = '\0';

                        ConnectionToId connectionToId;
                        connectionToId.setData(++fUsedConnections.lastId, thisPort.group, thisPort.port, targetPort.group, targetPort.port);

                        callback(sendHost, sendOSC,
                                 ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED,
                                 connectionToId.id,
                                 0, 0, 0, 0.0f,
                                 strBuf);
                        fUsedConnections.list.append(connectionToId);
                    }

                    jackbridge_free(connections);
                }
            }

            jackbridge_free(ports);
        }
    }

    void addPatchbayJackPort(const bool sendHost, const bool sendOSC,
                             const uint groupId, const jack_port_t* const jackPort,
                             const char* const shortPortName, const char* const fullPortName, const int jackPortFlags)
    {
        bool portIsInput = (jackPortFlags & JackPortIsInput);
        bool portIsAudio = (std::strcmp(jackbridge_port_type(jackPort), JACK_DEFAULT_AUDIO_TYPE) == 0);
        bool portIsMIDI  = (std::strcmp(jackbridge_port_type(jackPort), JACK_DEFAULT_MIDI_TYPE) == 0);
        bool portIsCV    = false;
        bool portIsOSC   = false;

        if (const jack_uuid_t uuid = jackbridge_port_uuid(jackPort))
        {
            char* value = nullptr;
            char* type  = nullptr;

            if (jackbridge_get_property(uuid, JACKEY_SIGNAL_TYPE, &value, &type)
                && value != nullptr
                && type != nullptr
                && std::strcmp(type, "text/plain") == 0)
            {
                portIsCV  = (std::strcmp(value, "CV") == 0);
                portIsOSC = (std::strcmp(value, "OSC") == 0);
            }
        }

        uint canvasPortFlags = 0x0;
        canvasPortFlags |= portIsInput ? PATCHBAY_PORT_IS_INPUT : 0x0;

        /**/ if (portIsCV)
            canvasPortFlags |= PATCHBAY_PORT_TYPE_CV;
        else if (portIsOSC)
            canvasPortFlags |= PATCHBAY_PORT_TYPE_OSC;
        else if (portIsAudio)
            canvasPortFlags |= PATCHBAY_PORT_TYPE_AUDIO;
        else if (portIsMIDI)
            canvasPortFlags |= PATCHBAY_PORT_TYPE_MIDI;

        PortNameToId portNameToId;
        portNameToId.setData(groupId, ++fUsedPorts.lastId, shortPortName, fullPortName);

        callback(sendHost, sendOSC,
                 ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                 portNameToId.group,
                 static_cast<int>(portNameToId.port),
                 static_cast<int>(canvasPortFlags),
                 0, 0.0f,
                 portNameToId.name);
        fUsedPorts.list.append(portNameToId);
    }
#endif

    // -------------------------------------------------------------------

    void processPlugin(CarlaPlugin* const plugin, const uint32_t nframes)
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
            kTypeClientRegister,
            kTypePortRegister,
            kTypePortConnect,
            kTypePortRename
        };

        Type type;
        bool action; // register or connect
        jack_port_id_t port1;
        jack_port_id_t port2;
        char name1[STR_MAX+1];
        char name2[STR_MAX+1];
    };

    LinkedList<PostPonedJackEvent> fPostPonedEvents;
    CarlaMutex fPostPonedEventsMutex;

    bool fIsInternalClient;

    void postPoneJackCallback(const PostPonedJackEvent& ev)
    {
        const CarlaMutexLocker cml(fPostPonedEventsMutex);
        fPostPonedEvents.append(ev);
    }

    void run() override
    {
        LinkedList<PostPonedJackEvent> events;

        PostPonedJackEvent nullEvent;
        carla_zeroStruct(nullEvent);

        for (; ! shouldThreadExit();)
        {
            if (fIsInternalClient)
                idle();

            {
                const CarlaMutexLocker cml(fPostPonedEventsMutex);

                if (fPostPonedEvents.count() > 0)
                    fPostPonedEvents.moveTo(events);
            }

            if (fClient == nullptr)
                break;

            if (events.count() == 0)
            {
                carla_msleep(fIsInternalClient ? 50 : 200);
                continue;
            }

            for (LinkedList<PostPonedJackEvent>::Itenerator it = events.begin2(); it.valid(); it.next())
            {
                const PostPonedJackEvent& ev(it.getValue(nullEvent));
                CARLA_SAFE_ASSERT_CONTINUE(ev.type != PostPonedJackEvent::kTypeNull);

                switch (ev.type)
                {
                case PostPonedJackEvent::kTypeNull:
                    break;
                case PostPonedJackEvent::kTypeClientRegister:
                    handleJackClientRegistrationCallback(ev.name1, ev.action);
                    break;
                case PostPonedJackEvent::kTypePortRegister:
                    handleJackPortRegistrationCallback(ev.port1, ev.action);
                    break;
                case PostPonedJackEvent::kTypePortConnect:
                    handleJackPortConnectCallback(ev.port1, ev.port2, ev.action);
                    break;
                case PostPonedJackEvent::kTypePortRename:
                    handleJackPortRenameCallback(ev.port1, ev.name1, ev.name2);
                    break;
                }
            }

            events.clear();
        }

        events.clear();
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
        PostPonedJackEvent ev;
        carla_zeroStruct(ev);
        ev.type   = PostPonedJackEvent::kTypeClientRegister;
        ev.action = (reg != 0);
        std::strncpy(ev.name1, name, STR_MAX);
        handlePtr->postPoneJackCallback(ev);
    }

    static void JACKBRIDGE_API carla_jack_port_registration_callback(jack_port_id_t port, int reg, void* arg)
    {
        PostPonedJackEvent ev;
        carla_zeroStruct(ev);
        ev.type   = PostPonedJackEvent::kTypePortRegister;
        ev.action = (reg != 0);
        ev.port1  = port;
        handlePtr->postPoneJackCallback(ev);
    }

    static void JACKBRIDGE_API carla_jack_port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
    {
        PostPonedJackEvent ev;
        carla_zeroStruct(ev);
        ev.type   = PostPonedJackEvent::kTypePortConnect;
        ev.action = (connect != 0);
        ev.port1  = a;
        ev.port2  = b;
        handlePtr->postPoneJackCallback(ev);
    }

    static void JACKBRIDGE_API carla_jack_port_rename_callback(jack_port_id_t port, const char* oldName, const char* newName, void* arg)
    {
        PostPonedJackEvent ev;
        carla_zeroStruct(ev);
        ev.type  = PostPonedJackEvent::kTypePortRename;
        ev.port1 = port;
        std::strncpy(ev.name1, oldName, STR_MAX);
        std::strncpy(ev.name2, newName, STR_MAX);
        handlePtr->postPoneJackCallback(ev);
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
        CarlaPlugin* const plugin((CarlaPlugin*)arg);
        CARLA_SAFE_ASSERT_RETURN(plugin != nullptr && plugin->isEnabled(), 0);

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
        CarlaPlugin* const plugin((CarlaPlugin*)arg);
        CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

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

CarlaEngine* CarlaEngine::newJack()
{
    carla_debug("CarlaEngine::newJack()");
    return new CarlaEngineJack();
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#if defined(JACKBRIDGE_DIRECT) && !defined(BUILD_BRIDGE)
// -----------------------------------------------------------------------
// internal jack client

CARLA_EXPORT
int jack_initialize (jack_client_t *client, const char *load_init);

CARLA_EXPORT
void jack_finish(void *arg);

#ifdef CARLA_OS_UNIX
# include "ThreadSafeFFTW.hpp"
static ThreadSafeFFTW sThreadSafeFFTW;
#endif

// -----------------------------------------------------------------------

CARLA_EXPORT
int jack_initialize(jack_client_t* const client, const char* const load_init)
{
    CARLA_BACKEND_USE_NAMESPACE

    EngineProcessMode mode;
    if (load_init != nullptr && std::strcmp(load_init, "rack") == 0)
        mode = ENGINE_PROCESS_MODE_CONTINUOUS_RACK;
    else
        mode = ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS;

#ifdef USING_JUCE
    juce::initialiseJuce_GUI();
#endif

    CarlaEngineJack* const engine = new CarlaEngineJack();

    engine->setOption(ENGINE_OPTION_FORCE_STEREO, 1, nullptr);
    engine->setOption(ENGINE_OPTION_AUDIO_DRIVER, 0, "JACK");
    engine->setOption(ENGINE_OPTION_AUDIO_DEVICE, 0, "Auto-Connect ON");
    engine->setOption(ENGINE_OPTION_OSC_ENABLED,  1, nullptr);
    engine->setOption(ENGINE_OPTION_OSC_PORT_TCP, 22752, nullptr);
    engine->setOption(ENGINE_OPTION_OSC_PORT_UDP, 22752, nullptr);

    engine->setOption(ENGINE_OPTION_PROCESS_MODE, mode, nullptr);
    engine->setOption(ENGINE_OPTION_TRANSPORT_MODE, ENGINE_TRANSPORT_MODE_JACK, nullptr);

    // FIXME
    engine->setOption(ENGINE_OPTION_PATH_BINARIES, 0, "/usr/lib/carla");
    engine->setOption(ENGINE_OPTION_PATH_RESOURCES, 0, "/usr/share/resources");

    if (engine->initInternal(client))
    {
#ifdef CARLA_OS_UNIX
        sThreadSafeFFTW.init();
#endif
        return 0;
    }
    else
    {
        delete engine;
#ifdef USING_JUCE
        juce::shutdownJuce_GUI();
#endif
        return 1;
    }
}

CARLA_EXPORT
void jack_finish(void *arg)
{
    CARLA_BACKEND_USE_NAMESPACE

    CarlaEngineJack* const engine = (CarlaEngineJack*)arg;;
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr,);

#ifdef CARLA_OS_UNIX
    const ThreadSafeFFTW::Deinitializer tsfftwde(sThreadSafeFFTW);
#endif

    engine->setAboutToClose();
    engine->removeAllPlugins();
    engine->close();
    delete engine;

#ifdef USING_JUCE
    juce::shutdownJuce_GUI();
#endif
}

// -----------------------------------------------------------------------
#endif // defined(JACKBRIDGE_DIRECT) && !defined(BUILD_BRIDGE)
