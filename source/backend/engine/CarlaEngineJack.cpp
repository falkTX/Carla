/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"
#include "CarlaPatchbayUtils.hpp"

#include "jackbridge/JackBridge.hpp"
#include "jackey.h"

#include "juce_audio_basics.h"

#ifdef __SSE2_MATH__
# include <xmmintrin.h>
#endif

#define URI_CANVAS_ICON "http://kxstudio.sf.net/ns/canvas/icon"

using juce::FloatVectorOperations;
using juce::String;
using juce::StringArray;

CARLA_BACKEND_START_NAMESPACE

class CarlaEngineJack;
class CarlaEngineJackClient;

// -----------------------------------------------------------------------
// Fallback data

static const EngineEvent kFallbackJackEngineEvent = { kEngineEventTypeNull, 0, 0, {{ kEngineControlEventTypeNull, 0, 0.0f }} };

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
    CarlaEngineJackAudioPort(const CarlaEngineClient& client, const bool isInputPort, jack_client_t* const jackClient, jack_port_t* const jackPort, JackPortDeletionCallback* const delCallback) noexcept
        : CarlaEngineAudioPort(client, isInputPort),
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
#ifndef CARLA_OS_WIN
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
            try {
                if (const jack_uuid_t uuid = jackbridge_port_uuid(fJackPort))
                    jackbridge_remove_property(fJackClient, uuid, JACKEY_SIGNAL_TYPE);
            } CARLA_SAFE_EXCEPTION("Audio port remove meta type");

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
            FloatVectorOperations::clear(fBuffer, static_cast<int>(bufferSize));
    }

    void invalidate() noexcept
    {
        fJackClient = nullptr;
        fJackPort   = nullptr;
    }

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
    CarlaEngineJackCVPort(const CarlaEngineClient& client, const bool isInputPort, jack_client_t* const jackClient, jack_port_t* const jackPort, JackPortDeletionCallback* const delCallback) noexcept
        : CarlaEngineCVPort(client, isInputPort),
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
#ifndef CARLA_OS_WIN
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
            try {
                if (const jack_uuid_t uuid = jackbridge_port_uuid(fJackPort))
                    jackbridge_remove_property(fJackClient, uuid, JACKEY_SIGNAL_TYPE);
            } CARLA_SAFE_EXCEPTION("CV port remove meta type");

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
            FloatVectorOperations::clear(fBuffer, static_cast<int>(bufferSize));
    }

    void invalidate() noexcept
    {
        fJackClient = nullptr;
        fJackPort   = nullptr;
    }

private:
    jack_client_t* fJackClient;
    jack_port_t*   fJackPort;

    JackPortDeletionCallback* const kDeletionCallback;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackCVPort)
};

// -----------------------------------------------------------------------
// Carla Engine JACK-Event port

class CarlaEngineJackEventPort : public CarlaEngineEventPort
{
public:
    CarlaEngineJackEventPort(const CarlaEngineClient& client, const bool isInputPort, jack_client_t* const jackClient, jack_port_t* const jackPort, JackPortDeletionCallback* const delCallback) noexcept
        : CarlaEngineEventPort(client, isInputPort),
          fJackClient(jackClient),
          fJackPort(jackPort),
          fJackBuffer(nullptr),
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

    uint32_t getEventCount() const noexcept override
    {
        if (fJackPort == nullptr)
            return CarlaEngineEventPort::getEventCount();

        CARLA_SAFE_ASSERT_RETURN(kIsInput, 0);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, 0);

        try {
            return jackbridge_midi_get_event_count(fJackBuffer);
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

    const EngineEvent& getEventUnchecked(const uint32_t index) const noexcept override
    {
        jack_midi_event_t jackEvent;

        bool test = false;

        try {
            test = jackbridge_midi_event_get(&jackEvent, fJackBuffer, index);
        } CARLA_SAFE_EXCEPTION_RETURN("jack_midi_event_get", kFallbackJackEngineEvent);

        if (! test)
            return kFallbackJackEngineEvent;

        CARLA_SAFE_ASSERT_RETURN(jackEvent.size < UINT8_MAX, kFallbackJackEngineEvent);

        fRetEvent.time = jackEvent.time;
        fRetEvent.fillFromMidiData(static_cast<uint8_t>(jackEvent.size), jackEvent.buffer);

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
        CARLA_SAFE_ASSERT_RETURN(param < 0x5F, false);
        CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.0f);

        if (type == kEngineControlEventTypeParameter) {
            CARLA_SAFE_ASSERT(! MIDI_IS_CONTROL_BANK_SELECT(param));
        }

        uint8_t size    = 0;
        uint8_t data[3] = { 0, 0, 0 };

        EngineControlEvent ctrlEvent = { type, param, value };
        ctrlEvent.convertToMidiData(channel, size, data);

        if (size == 0)
            return false;

        try {
            return jackbridge_midi_event_write(fJackBuffer, time, data, size);
        } CARLA_SAFE_EXCEPTION_RETURN("jack_midi_event_write", false);
    }

    bool writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t port, const uint8_t size, const uint8_t* const data) noexcept override
    {
        if (fJackPort == nullptr)
            return CarlaEngineEventPort::writeMidiEvent(time, channel, port, size, data);

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

private:
    jack_client_t* fJackClient;
    jack_port_t*   fJackPort;
    void*          fJackBuffer;

    mutable EngineEvent fRetEvent;

    JackPortDeletionCallback* const kDeletionCallback;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackEventPort)
};

// -----------------------------------------------------------------------
// Jack Engine client

class CarlaEngineJackClient : public CarlaEngineClient,
                                     JackPortDeletionCallback
{
public:
    CarlaEngineJackClient(const CarlaEngine& engine, jack_client_t* const jackClient)
        : CarlaEngineClient(engine),
          fJackClient(jackClient),
          fUseClient(engine.getProccessMode() == ENGINE_PROCESS_MODE_SINGLE_CLIENT || engine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
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

        if (kEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS && fJackClient != nullptr) // FIXME
            jackbridge_client_close(fJackClient);

        // ports must have been deleted by now!
        //fAudioPorts.clear();
        //fCVPorts.clear();
        //fEventPorts.clear();
    }

    void activate() noexcept override
    {
        carla_debug("CarlaEngineJackClient::activate()");

        if (kEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_SAFE_ASSERT_RETURN(fJackClient != nullptr && ! fActive,);

            try {
                jackbridge_activate(fJackClient);
            } catch(...) {}
        }

        CarlaEngineClient::activate();
    }

    void deactivate() noexcept override
    {
        carla_debug("CarlaEngineJackClient::deactivate()");

        if (kEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_SAFE_ASSERT_RETURN(fJackClient != nullptr && fActive,);

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

    CarlaEnginePort* addPort(const EnginePortType portType, const char* const name, const bool isInput) override
    {
        carla_debug("CarlaEngineJackClient::addPort(%i:%s, \"%s\", %s)", portType, EnginePortType2Str(portType), name, bool2str(isInput));

        jack_port_t* jackPort = nullptr;

        // Create JACK port first, if needed
        if (fUseClient && fJackClient != nullptr)
        {
            switch (portType)
            {
            case kEnginePortTypeNull:
                break;
            case kEnginePortTypeAudio:
                jackPort = jackbridge_port_register(fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            case kEnginePortTypeCV:
                jackPort = jackbridge_port_register(fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            case kEnginePortTypeEvent:
                jackPort = jackbridge_port_register(fJackClient, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
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
            CarlaEngineJackAudioPort* const enginePort(new CarlaEngineJackAudioPort(*this, isInput, fJackClient, jackPort, this));
            fAudioPorts.append(enginePort);
            return enginePort;
        }
        case kEnginePortTypeCV: {
            CarlaEngineJackCVPort* const enginePort(new CarlaEngineJackCVPort(*this, isInput, fJackClient, jackPort, this));
            fCVPorts.append(enginePort);
            return enginePort;
        }
        case kEnginePortTypeEvent: {
            CarlaEngineJackEventPort* const enginePort(new CarlaEngineJackEventPort(*this, isInput, fJackClient, jackPort, this));
            fEventPorts.append(enginePort);
            return enginePort;
        }
        }

        carla_stderr("CarlaEngineJackClient::addPort(%i, \"%s\", %s) - invalid type", portType, name, bool2str(isInput));
        return nullptr;
    }

    void invalidate() noexcept
    {
        for (LinkedList<CarlaEngineJackAudioPort*>::Itenerator it = fAudioPorts.begin(); it.valid(); it.next())
        {
            CarlaEngineJackAudioPort* const port(it.getValue());
            port->invalidate();
        }

        for (LinkedList<CarlaEngineJackCVPort*>::Itenerator it = fCVPorts.begin(); it.valid(); it.next())
        {
            CarlaEngineJackCVPort* const port(it.getValue());
            port->invalidate();
        }

        for (LinkedList<CarlaEngineJackEventPort*>::Itenerator it = fEventPorts.begin(); it.valid(); it.next())
        {
            CarlaEngineJackEventPort* const port(it.getValue());
            port->invalidate();
        }

        fJackClient = nullptr;
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

private:
    jack_client_t* fJackClient;
    const bool     fUseClient;

    LinkedList<CarlaEngineJackAudioPort*> fAudioPorts;
    LinkedList<CarlaEngineJackCVPort*>    fCVPorts;
    LinkedList<CarlaEngineJackEventPort*> fEventPorts;

    friend class CarlaEngineJack;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackClient)
};

// -----------------------------------------------------------------------
// Jack Engine

class CarlaEngineJack : public CarlaEngine
{
public:
    CarlaEngineJack()
        : CarlaEngine(),
          fClient(nullptr),
          fTransportState(JackTransportStopped),
          fFreewheel(false)
#ifdef BUILD_BRIDGE
        , fIsRunning(false)
#endif
    {
        carla_debug("CarlaEngineJack::CarlaEngineJack()");

#ifdef BUILD_BRIDGE
        pData->options.processMode = ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS;
#else
        carla_fill<jack_port_t*>(fRackPorts, nullptr, kRackPortCount);
#endif

        // FIXME: Always enable JACK transport for now
        pData->options.transportMode = ENGINE_TRANSPORT_MODE_JACK;

        carla_zeroStruct<jack_position_t>(fTransportPos);
    }

    ~CarlaEngineJack() noexcept override
    {
        carla_debug("CarlaEngineJack::~CarlaEngineJack()");
        CARLA_SAFE_ASSERT(fClient == nullptr);

#ifndef BUILD_BRIDGE
        fUsedGroups.clear();
        fUsedPorts.clear();
        fUsedConnections.clear();
        fNewGroups.clear();
#endif
    }

    // -------------------------------------------------------------------
    // Maximum values

    uint getMaxClientNameSize() const noexcept override
    {
        if (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT || pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            try {
                return static_cast<uint>(jackbridge_client_name_size());
            } CARLA_SAFE_EXCEPTION_RETURN("jack_client_name_size", 0);
        }

        return CarlaEngine::getMaxClientNameSize();
    }

    uint getMaxPortNameSize() const noexcept override
    {
        if (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT || pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            try {
                return static_cast<uint>(jackbridge_port_name_size());
            } CARLA_SAFE_EXCEPTION_RETURN("jack_port_name_size", 0);
        }

        return CarlaEngine::getMaxPortNameSize();
    }

    // -------------------------------------------------------------------
    // Virtual, per-engine type calls

    bool init(const char* const clientName) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient == nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
        carla_debug("CarlaEngineJack::init(\"%s\")", clientName);

        fFreewheel      = false;
        fTransportState = JackTransportStopped;

        carla_zeroStruct<jack_position_t>(fTransportPos);

#ifdef BUILD_BRIDGE
        if (pData->bufferSize == 0 || pData->sampleRate == 0.0)
        {
            // open temp client to get initial buffer-size and sample-rate values
            if (jack_client_t* const tmpClient = jackbridge_client_open(clientName, JackNullOption, nullptr))
            {
                pData->bufferSize = jackbridge_get_buffer_size(tmpClient);
                pData->sampleRate = jackbridge_get_sample_rate(tmpClient);

                jackbridge_client_close(tmpClient);
            }
        }

        fIsRunning = true;

        return CarlaEngine::init(clientName);
#else
        fClient = jackbridge_client_open(clientName, JackNullOption, nullptr);

        if (fClient == nullptr)
        {
            setLastError("Failed to create new JACK client");
            return false;
        }

        pData->bufferSize = jackbridge_get_buffer_size(fClient);
        pData->sampleRate = jackbridge_get_sample_rate(fClient);

        jackbridge_set_thread_init_callback(fClient, carla_jack_thread_init_callback, nullptr);
        jackbridge_set_buffer_size_callback(fClient, carla_jack_bufsize_callback, this);
        jackbridge_set_sample_rate_callback(fClient, carla_jack_srate_callback, this);
        jackbridge_set_freewheel_callback(fClient, carla_jack_freewheel_callback, this);
        jackbridge_set_latency_callback(fClient, carla_jack_latency_callback, this);
        jackbridge_set_process_callback(fClient, carla_jack_process_callback, this);
        jackbridge_on_shutdown(fClient, carla_jack_shutdown_callback, this);

        const char* const jackClientName(jackbridge_get_client_name(fClient));

        initJackPatchbay(jackClientName);

        jackbridge_set_client_registration_callback(fClient, carla_jack_client_registration_callback, this);
        jackbridge_set_port_registration_callback(fClient, carla_jack_port_registration_callback, this);
        jackbridge_set_port_connect_callback(fClient, carla_jack_port_connect_callback, this);
        jackbridge_set_port_rename_callback(fClient, carla_jack_port_rename_callback, this);

        if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        {
            fRackPorts[kRackPortAudioIn1]  = jackbridge_port_register(fClient, "audio-in1",  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            fRackPorts[kRackPortAudioIn2]  = jackbridge_port_register(fClient, "audio-in2",  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            fRackPorts[kRackPortAudioOut1] = jackbridge_port_register(fClient, "audio-out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
            fRackPorts[kRackPortAudioOut2] = jackbridge_port_register(fClient, "audio-out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
            fRackPorts[kRackPortEventIn]   = jackbridge_port_register(fClient, "events-in",  JACK_DEFAULT_MIDI_TYPE,  JackPortIsInput, 0);
            fRackPorts[kRackPortEventOut]  = jackbridge_port_register(fClient, "events-out", JACK_DEFAULT_MIDI_TYPE,  JackPortIsOutput, 0);

            pData->graph.create(true, pData->sampleRate, pData->bufferSize, 0, 0);
        }

        if (jackbridge_activate(fClient))
        {
            CarlaEngine::init(jackClientName);
            return true;
        }

        jackbridge_client_close(fClient);
        fClient = nullptr;

        setLastError("Failed to activate the JACK client");
        return false;
#endif
    }

    bool close() override
    {
        carla_debug("CarlaEngineJack::close()");

        CarlaEngine::close();

#ifdef BUILD_BRIDGE
        fClient    = nullptr;
        fIsRunning = false;
        return true;
#else
        if (jackbridge_deactivate(fClient))
        {
            if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
            {
                if (fRackPorts[0] != nullptr)
                {
                    jackbridge_port_unregister(fClient, fRackPorts[kRackPortAudioIn1]);
                    jackbridge_port_unregister(fClient, fRackPorts[kRackPortAudioIn2]);
                    jackbridge_port_unregister(fClient, fRackPorts[kRackPortAudioOut1]);
                    jackbridge_port_unregister(fClient, fRackPorts[kRackPortAudioOut2]);
                    jackbridge_port_unregister(fClient, fRackPorts[kRackPortEventIn]);
                    jackbridge_port_unregister(fClient, fRackPorts[kRackPortEventOut]);
                    carla_fill<jack_port_t*>(fRackPorts, nullptr, kRackPortCount);
                }

                pData->graph.destroy();
            }

            if (jackbridge_client_close(fClient))
            {
                fClient = nullptr;
                return true;
            }
            else
                setLastError("Failed to close the JACK client");
        }
        else
            setLastError("Failed to deactivate the JACK client");

        fClient = nullptr;

        fUsedGroups.clear();
        fUsedPorts.clear();
        fUsedConnections.clear();
        fNewGroups.clear();

        return false;
#endif
    }

#ifndef BUILD_BRIDGE
    void idle() noexcept override
    {
        CarlaEngine::idle();

        if (fNewGroups.count() == 0)
            return;

        LinkedList<uint> newPlugins;
        fNewGroups.spliceInsertInto(newPlugins);

        for (LinkedList<uint>::Itenerator it = newPlugins.begin(); it.valid(); it.next())
        {
            const uint groupId(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(groupId > 0);

            const char* const groupName(fUsedGroups.getGroupName(groupId));
            CARLA_SAFE_ASSERT_CONTINUE(groupName != nullptr && groupName[0] != '\0');

            int pluginId = -1;
            PatchbayIcon icon = PATCHBAY_ICON_PLUGIN;

            if (findPluginIdAndIcon(groupName, pluginId, icon))
                callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED, groupId, icon, pluginId, 0.0f, nullptr);
        }

        newPlugins.clear();
    }
#endif

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
            client = jackbridge_client_open(plugin->getName(), JackNullOption, nullptr);

            CARLA_SAFE_ASSERT_RETURN(client != nullptr, nullptr);

            jackbridge_set_thread_init_callback(client, carla_jack_thread_init_callback, nullptr);

#ifndef BUILD_BRIDGE
            jackbridge_set_latency_callback(client, carla_jack_latency_callback_plugin, plugin);
            jackbridge_set_process_callback(client, carla_jack_process_callback_plugin, plugin);
            jackbridge_on_shutdown(client, carla_jack_shutdown_callback_plugin, plugin);
#else
            fClient = client;
            pData->bufferSize = jackbridge_get_buffer_size(client);
            pData->sampleRate = jackbridge_get_sample_rate(client);

            jackbridge_set_buffer_size_callback(client, carla_jack_bufsize_callback, this);
            jackbridge_set_sample_rate_callback(client, carla_jack_srate_callback, this);
            jackbridge_set_freewheel_callback(client, carla_jack_freewheel_callback, this);
            jackbridge_set_latency_callback(client, carla_jack_latency_callback, this);
            jackbridge_set_process_callback(client, carla_jack_process_callback, this);
            jackbridge_on_shutdown(client, carla_jack_shutdown_callback, this);
#endif
        }

        return new CarlaEngineJackClient(*this, client);
    }

#ifndef BUILD_BRIDGE
    const char* renamePlugin(const uint id, const char* const newName) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->curPluginCount > 0, nullptr);
        CARLA_SAFE_ASSERT_RETURN(id < pData->curPluginCount, nullptr);
        CARLA_SAFE_ASSERT_RETURN(pData->plugins != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(newName != nullptr && newName[0] != '\0', nullptr);

        CarlaPlugin* const plugin(pData->plugins[id].plugin);

        if (plugin == nullptr)
        {
            carla_stderr("CarlaEngine::clonePlugin(%i) - could not find plugin", id);
            return nullptr;
        }

        CARLA_SAFE_ASSERT(plugin->getId() == id);

        CarlaString uniqueName;

        try {
            const char* const tmpName = getUniquePluginName(newName);
            uniqueName = tmpName;
            delete[] tmpName;
        } CARLA_SAFE_EXCEPTION("JACK renamePlugin");

        if (uniqueName.isEmpty())
        {
            setLastError("Failed to request new unique plugin name");
            return nullptr;
        }

        // single client always re-inits
        bool needsReinit = (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT);

        // rename in multiple client mode
        if (pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CarlaEngineJackClient* const client((CarlaEngineJackClient*)plugin->getEngineClient());

#if 0
            if (bridge.client_rename_ptr != nullptr)
            {
                newName = jackbridge_client_rename(client->fClient, newName);
            }
            else
#endif
            {
                // we should not be able to do this, jack really needs to allow client rename
                if (jack_client_t* const jackClient = jackbridge_client_open(uniqueName, JackNullOption, nullptr))
                {
                    // close old client
                    plugin->setEnabled(false);

                    if (client->isActive())
                        client->deactivate();

                    plugin->clearBuffers();

                    jackbridge_client_close(client->fJackClient);

                    // set new client data
                    uniqueName = jackbridge_get_client_name(jackClient);

                    jackbridge_set_thread_init_callback(jackClient, carla_jack_thread_init_callback, nullptr);
                    jackbridge_set_process_callback(jackClient, carla_jack_process_callback_plugin, plugin);
                    jackbridge_set_latency_callback(jackClient, carla_jack_latency_callback_plugin, plugin);
                    jackbridge_on_shutdown(jackClient, carla_jack_shutdown_callback_plugin, plugin);

                    client->fJackClient = jackClient;

                    needsReinit = true;
                }
                else
                {
                    setLastError("Failed to create new JACK client");
                    return nullptr;
                }
            }
        }

        // Rename
        plugin->setName(uniqueName);

        if (needsReinit)
        {
            // reload plugin to recreate its ports
            const StateSave& saveState(plugin->getStateSave());
            plugin->reload();
            plugin->loadStateSave(saveState);
        }

        return plugin->getName();
    }

    // -------------------------------------------------------------------
    // Patchbay

    bool patchbayConnect(const uint groupA, const uint portA, const uint groupB, const uint portB) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, false);

        const char* const fullPortNameA = fUsedPorts.getFullPortName(groupA, portA);
        CARLA_SAFE_ASSERT_RETURN(fullPortNameA != nullptr && fullPortNameA[0] != '\0', false);

        const char* const fullPortNameB = fUsedPorts.getFullPortName(groupB, portB);
        CARLA_SAFE_ASSERT_RETURN(fullPortNameB != nullptr && fullPortNameB[0] != '\0', false);

        carla_stdout("patchbayConnect(%u, %u, %u, %u => %s, %s)", groupA, portA, groupB, portB, fullPortNameA, fullPortNameB);

        if (! jackbridge_connect(fClient, fullPortNameA, fullPortNameB))
        {
            setLastError("JACK operation failed");
            return false;
        }

        return true;
    }

    bool patchbayDisconnect(const uint connectionId) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, false);

        for (LinkedList<ConnectionToId>::Itenerator it = fUsedConnections.list.begin(); it.valid(); it.next())
        {
            const ConnectionToId& connectionToId(it.getValue());

            if (connectionToId.id == connectionId)
            {
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
        }

        setLastError("Failed to find the requested connection");
        return false;
    }

    bool patchbayRefresh() override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, false);

        fUsedGroups.clear();
        fUsedPorts.clear();
        fUsedConnections.clear();
        fNewGroups.clear();

        initJackPatchbay(jackbridge_get_client_name(fClient));

        return true;
    }

    // -------------------------------------------------------------------
    // Transport

    void transportPlay() noexcept override
    {
        if (pData->options.transportMode == ENGINE_TRANSPORT_MODE_INTERNAL)
            return CarlaEngine::transportPlay();

        if (fClient != nullptr)
        {
            try {
                jackbridge_transport_start(fClient);
            } catch(...) {}
        }
    }

    void transportPause() noexcept override
    {
        if (pData->options.transportMode == ENGINE_TRANSPORT_MODE_INTERNAL)
            return CarlaEngine::transportPause();

        if (fClient != nullptr)
        {
            try {
                jackbridge_transport_stop(fClient);
            } catch(...) {}
        }
    }

    void transportRelocate(const uint64_t frame) noexcept override
    {
        if (pData->options.transportMode == ENGINE_TRANSPORT_MODE_INTERNAL)
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

    const char* const* getPatchbayConnections() const override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, nullptr);
        carla_debug("CarlaEngineJack::getPatchbayConnections()");

        LinkedList<const char*> connList;

        if (const char** const ports = jackbridge_get_ports(fClient, nullptr, nullptr, JackPortIsOutput))
        {
            for (int i=0; ports[i] != nullptr; ++i)
            {
                const jack_port_t* const jackPort(jackbridge_port_by_name(fClient, ports[i]));
                const char* const fullPortName(ports[i]);

                CARLA_SAFE_ASSERT_CONTINUE(jackPort != nullptr);

                if (const char** connections = jackbridge_port_get_all_connections(fClient, jackPort))
                {
                    for (int j=0; connections[j] != nullptr; ++j)
                    {
                        connList.append(carla_strdup(fullPortName));
                        connList.append(carla_strdup(connections[j]));
                    }

                    jackbridge_free(connections);
                }
            }

            jackbridge_free(ports);
        }

        const size_t connCount(connList.count());

        if (connCount == 0)
            return nullptr;

        const char** const retConns = new const char*[connCount+1];

        for (size_t i=0; i < connCount; ++i)
            retConns[i] = connList.getAt(i, nullptr);

        retConns[connCount] = nullptr;
        connList.clear();

        return retConns;
    }

    void restorePatchbayConnection(const char* const connSource, const char* const connTarget) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(connSource != nullptr && connSource[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(connTarget != nullptr && connTarget[0] != '\0',);
        carla_debug("CarlaEngineJack::restorePatchbayConnection(\"%s\", \"%s\")", connSource, connTarget);

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

        pData->bufferSize = newBufferSize;
        bufferSizeChanged(newBufferSize);
    }

    void handleJackSampleRateCallback(const double newSampleRate)
    {
        if (pData->sampleRate == newSampleRate)
            return;

        pData->sampleRate = newSampleRate;
        sampleRateChanged(newSampleRate);
    }

    void handleJackFreewheelCallback(const bool isFreewheel)
    {
        if (fFreewheel == isFreewheel)
            return;

        fFreewheel = isFreewheel;
        offlineModeChanged(isFreewheel);
    }

    void saveTransportInfo()
    {
        if (pData->options.transportMode != ENGINE_TRANSPORT_MODE_JACK)
            return;

        fTransportPos.unique_1 = fTransportPos.unique_2 + 1; // invalidate

        fTransportState = jackbridge_transport_query(fClient, &fTransportPos);

        pData->timeInfo.playing = (fTransportState == JackTransportRolling);

        if (fTransportPos.unique_1 == fTransportPos.unique_2)
        {
            pData->timeInfo.frame = fTransportPos.frame;
            pData->timeInfo.usecs = fTransportPos.usecs;

            if (fTransportPos.valid & JackPositionBBT)
            {
                pData->timeInfo.valid              = EngineTimeInfo::kValidBBT;
                pData->timeInfo.bbt.bar            = fTransportPos.bar;
                pData->timeInfo.bbt.beat           = fTransportPos.beat;
                pData->timeInfo.bbt.tick           = fTransportPos.tick;
                pData->timeInfo.bbt.barStartTick   = fTransportPos.bar_start_tick;
                pData->timeInfo.bbt.beatsPerBar    = fTransportPos.beats_per_bar;
                pData->timeInfo.bbt.beatType       = fTransportPos.beat_type;
                pData->timeInfo.bbt.ticksPerBeat   = fTransportPos.ticks_per_beat;
                pData->timeInfo.bbt.beatsPerMinute = fTransportPos.beats_per_minute;
            }
            else
                pData->timeInfo.valid = 0x0;
        }
        else
        {
            pData->timeInfo.frame = 0;
            pData->timeInfo.valid = 0x0;
        }
    }

    void handleJackProcessCallback(const uint32_t nframes)
    {
        saveTransportInfo();

        if (pData->curPluginCount == 0)
        {
#ifndef BUILD_BRIDGE
            // pass-through
            if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
            {
                float* const audioIn1  = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioIn1], nframes);
                float* const audioIn2  = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioIn2], nframes);
                float* const audioOut1 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut1], nframes);
                float* const audioOut2 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut2], nframes);
                void*  const eventOut  = jackbridge_port_get_buffer(fRackPorts[kRackPortEventOut], nframes);

                FloatVectorOperations::copy(audioOut1, audioIn1, static_cast<int>(nframes));
                FloatVectorOperations::copy(audioOut2, audioIn2, static_cast<int>(nframes));

                jackbridge_midi_clear_buffer(eventOut);
            }
#endif

            return runPendingRtEvents();
        }

#ifdef BUILD_BRIDGE
        CarlaPlugin* const plugin(pData->plugins[0].plugin);

        if (plugin != nullptr && plugin->isEnabled() && plugin->tryLock(fFreewheel))
        {
            plugin->initBuffers();
            processPlugin(plugin, nframes);
            plugin->unlock();
        }

        return runPendingRtEvents();
#else
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

            return runPendingRtEvents();
        }

        if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        {
            // get buffers from jack
            float* const audioIn1  = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioIn1], nframes);
            float* const audioIn2  = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioIn2], nframes);
            float* const audioOut1 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut1], nframes);
            float* const audioOut2 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut2], nframes);
            void* const  eventIn   = jackbridge_port_get_buffer(fRackPorts[kRackPortEventIn],  nframes);
            void* const  eventOut  = jackbridge_port_get_buffer(fRackPorts[kRackPortEventOut], nframes);

#if 0
            // assert buffers
            CARLA_SAFE_ASSERT(audioIn1 != nullptr);
            CARLA_SAFE_ASSERT(audioIn2 != nullptr);
            CARLA_SAFE_ASSERT(audioOut1 != nullptr);
            CARLA_SAFE_ASSERT(audioOut2 != nullptr);
            CARLA_SAFE_ASSERT(eventIn != nullptr);
            CARLA_SAFE_ASSERT(eventOut != nullptr);
#endif

            // create audio buffers
            const float* inBuf[2]  = { audioIn1, audioIn2 };
                  float* outBuf[2] = { audioOut1, audioOut2 };

            // initialize events
            carla_zeroStruct<EngineEvent>(pData->events.in,  kMaxEngineEventInternalCount);
            carla_zeroStruct<EngineEvent>(pData->events.out, kMaxEngineEventInternalCount);

            {
                ushort engineEventIndex = 0;

                jack_midi_event_t jackEvent;
                const uint32_t jackEventCount(jackbridge_midi_get_event_count(eventIn));

                for (uint32_t jackEventIndex=0; jackEventIndex < jackEventCount; ++jackEventIndex)
                {
                    if (! jackbridge_midi_event_get(&jackEvent, eventIn, jackEventIndex))
                        continue;

                    CARLA_SAFE_ASSERT_CONTINUE(jackEvent.size <= 0xFF /* uint8_t max */);

                    EngineEvent& engineEvent(pData->events.in[engineEventIndex++]);

                    engineEvent.time = jackEvent.time;
                    engineEvent.fillFromMidiData(static_cast<uint8_t>(jackEvent.size), jackEvent.buffer);

                    if (engineEventIndex >= kMaxEngineEventInternalCount)
                        break;
                }
            }

            // process rack
            pData->graph.processRack(pData, inBuf, outBuf, nframes);

            // output control
            {
                jackbridge_midi_clear_buffer(eventOut);

                uint8_t        size    = 0;
                uint8_t        data[3] = { 0, 0, 0 };
                const uint8_t* dataPtr = data;

                for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
                {
                    const EngineEvent& engineEvent(pData->events.out[i]);

                    if (engineEvent.type == kEngineEventTypeNull)
                        break;

                    else if (engineEvent.type == kEngineEventTypeControl)
                    {
                        const EngineControlEvent& ctrlEvent(engineEvent.ctrl);
                        ctrlEvent.convertToMidiData(engineEvent.channel, size, data);
                        dataPtr = data;
                    }
                    else if (engineEvent.type == kEngineEventTypeMidi)
                    {
                        const EngineMidiEvent& midiEvent(engineEvent.midi);

                        size = midiEvent.size;

                        if (size > EngineMidiEvent::kDataSize && midiEvent.dataExt != nullptr)
                            dataPtr = midiEvent.dataExt;
                        else
                            dataPtr = midiEvent.data;
                    }
                    else
                    {
                        continue;
                    }

                    if (size > 0)
                        jackbridge_midi_event_write(eventOut, engineEvent.time, dataPtr, size);
                }
            }

            return runPendingRtEvents();
        }
#endif // ! BUILD_BRIDGE

        runPendingRtEvents();
    }

    void handleJackLatencyCallback(const jack_latency_callback_mode_t /*mode*/)
    {
        // TODO
    }

#ifndef BUILD_BRIDGE
    void handleJackClientRegistrationCallback(const char* const name, const bool reg)
    {
        CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

        // do nothing on client registration, wait for first port
        if (reg) return;

        const uint groupId(fUsedGroups.getGroupId(name));

        // clients might have been registered without ports
        if (groupId == 0) return;

        GroupNameToId groupNameToId;
        groupNameToId.setData(groupId, name);

        callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED, groupNameToId.group, 0, 0, 0.0f, nullptr);
        fUsedGroups.list.removeOne(groupNameToId);
    }

    void handleJackPortRegistrationCallback(const jack_port_id_t port, const bool reg)
    {
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
                PatchbayIcon icon = (jackPortFlags & JackPortIsPhysical) ? PATCHBAY_ICON_HARDWARE : PATCHBAY_ICON_APPLICATION;

                GroupNameToId groupNameToId;
                groupNameToId.setData(groupId, groupName);

                callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, groupNameToId.group, icon, -1, 0.0f, groupNameToId.name);

                fNewGroups.append(groupId);
                fUsedGroups.list.append(groupNameToId);
            }

            addPatchbayJackPort(groupId, jackPort, shortPortName, fullPortName, jackPortFlags);
        }
        else
        {
            const PortNameToId& portNameToId(fUsedPorts.getPortNameToId(fullPortName));
            CARLA_SAFE_ASSERT_RETURN(portNameToId.group > 0 && portNameToId.port > 0,);

            callback(ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED, portNameToId.group, static_cast<int>(portNameToId.port), 0, 0.0f, nullptr);
            fUsedPorts.list.removeOne(portNameToId);
        }
    }

    void handleJackPortConnectCallback(const jack_port_id_t a, const jack_port_id_t b, const bool connect)
    {
        const jack_port_t* const jackPortA(jackbridge_port_by_id(fClient, a));
        CARLA_SAFE_ASSERT_RETURN(jackPortA != nullptr,);

        const jack_port_t* const jackPortB(jackbridge_port_by_id(fClient, b));
        CARLA_SAFE_ASSERT_RETURN(jackPortB != nullptr,);

        const char* const fullPortNameA(jackbridge_port_name(jackPortA));
        CARLA_SAFE_ASSERT_RETURN(fullPortNameA != nullptr && fullPortNameA[0] != '\0',);

        const char* const fullPortNameB(jackbridge_port_name(jackPortB));
        CARLA_SAFE_ASSERT_RETURN(fullPortNameB != nullptr && fullPortNameB[0] != '\0',);

        const PortNameToId& portNameToIdA(fUsedPorts.getPortNameToId(fullPortNameA));
        CARLA_SAFE_ASSERT_RETURN(portNameToIdA.group > 0 && portNameToIdA.port > 0,);

        const PortNameToId& portNameToIdB(fUsedPorts.getPortNameToId(fullPortNameB));
        CARLA_SAFE_ASSERT_RETURN(portNameToIdB.group > 0 && portNameToIdB.port > 0,);

        if (connect)
        {
            char strBuf[STR_MAX+1];
            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", portNameToIdA.group, portNameToIdA.port, portNameToIdB.group, portNameToIdB.port);
            strBuf[STR_MAX] = '\0';

            ConnectionToId connectionToId;
            connectionToId.setData(++fUsedConnections.lastId, portNameToIdA.group, portNameToIdA.port, portNameToIdB.group, portNameToIdB.port);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);
            fUsedConnections.list.append(connectionToId);
        }
        else
        {
            for (LinkedList<ConnectionToId>::Itenerator it = fUsedConnections.list.begin(); it.valid(); it.next())
            {
                const ConnectionToId& connectionToId(it.getValue());

                if (connectionToId.groupA == portNameToIdA.group && connectionToId.portA == portNameToIdA.port &&
                    connectionToId.groupB == portNameToIdB.group && connectionToId.portB == portNameToIdB.port)
                {
                    callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED, connectionToId.id, 0, 0, 0.0f, nullptr);
                    fUsedConnections.list.remove(it);
                    break;
                }
            }
        }
    }

    void handleJackClientRenameCallback(const char* const oldName, const char* const newName)
    {
        CARLA_SAFE_ASSERT_RETURN(oldName != nullptr && oldName[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(newName != nullptr && newName[0] != '\0',);

        for (LinkedList<GroupNameToId>::Itenerator it = fUsedGroups.list.begin(); it.valid(); it.next())
        {
            GroupNameToId& groupNameToId(it.getValue());

            if (std::strcmp(groupNameToId.name, oldName) == 0)
            {
                groupNameToId.rename(newName);
                callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED, groupNameToId.group, 0, 0, 0.0f, groupNameToId.name);
                break;
            }
        }
    }

    void handleJackPortRenameCallback(const jack_port_id_t port, const char* const oldFullName, const char* const newFullName)
    {
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

        for (LinkedList<PortNameToId>::Itenerator it = fUsedPorts.list.begin(); it.valid(); it.next())
        {
            PortNameToId& portNameToId(it.getValue());

            if (std::strcmp(portNameToId.fullName, oldFullName) == 0)
            {
                CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group == groupId);

                portNameToId.rename(shortPortName, newFullName);
                callback(ENGINE_CALLBACK_PATCHBAY_PORT_RENAMED, portNameToId.group, static_cast<int>(portNameToId.port), 0, 0.0f, portNameToId.name);
                break;
            }
        }
    }
#endif

    void handleJackShutdownCallback()
    {
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

#ifndef BUILD_BRIDGE
        carla_fill<jack_port_t*>(fRackPorts, nullptr, kRackPortCount);
#endif

        fClient = nullptr;
        callback(ENGINE_CALLBACK_QUIT, 0, 0, 0, 0.0f, nullptr);
    }

    // -------------------------------------------------------------------

private:
    jack_client_t*         fClient;
    jack_position_t        fTransportPos;
    jack_transport_state_t fTransportState;
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

    PatchbayGroupList      fUsedGroups;
    PatchbayPortList       fUsedPorts;
    PatchbayConnectionList fUsedConnections;
    LinkedList<uint>       fNewGroups;

    bool findPluginIdAndIcon(const char* const clientName, int& pluginId, PatchbayIcon& icon) noexcept
    {
        carla_debug("CarlaEngineJack::findPluginIdAndIcon(\"%s\", ...)", clientName);

        // TODO - this currently only works in multi-client mode
        if (pData->options.processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
            return false;

        for (uint i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin == nullptr || ! plugin->isEnabled())
                continue;

            const CarlaEngineJackClient* const engineClient((const CarlaEngineJackClient*)plugin->getEngineClient());
            CARLA_SAFE_ASSERT_CONTINUE(engineClient != nullptr);

            const char* const engineClientName(engineClient->getJackClientName());
            CARLA_SAFE_ASSERT_CONTINUE(engineClientName != nullptr && engineClientName[0] != '\0');

            if (std::strcmp(clientName, engineClientName) != 0)
                continue;

            pluginId = static_cast<int>(i);
            icon     = PATCHBAY_ICON_PLUGIN;

            if (const char* const pluginIcon = plugin->getIconName())
            {
                if (pluginIcon[0] == '\0')
                    pass();
                else if (std::strcmp(pluginIcon, "app") == 0 || std::strcmp(pluginIcon, "application") == 0)
                    icon = PATCHBAY_ICON_APPLICATION;
                else if (std::strcmp(pluginIcon, "plugin") == 0)
                    icon = PATCHBAY_ICON_PLUGIN;
                else if (std::strcmp(pluginIcon, "hardware") == 0)
                    icon = PATCHBAY_ICON_HARDWARE;
                else if (std::strcmp(pluginIcon, "carla") == 0)
                    icon = PATCHBAY_ICON_CARLA;
                else if (std::strcmp(pluginIcon, "distrho") == 0)
                    icon = PATCHBAY_ICON_DISTRHO;
                else if (std::strcmp(pluginIcon, "file") == 0)
                    icon = PATCHBAY_ICON_FILE;
            }

            return true;
        }

        return false;
    }

    void initJackPatchbay(const char* const ourName)
    {
        CARLA_SAFE_ASSERT_RETURN(ourName != nullptr && ourName[0] != '\0',);

        StringArray parsedGroups;

        // add our client first
        {
            parsedGroups.add(String(ourName));

            GroupNameToId groupNameToId;
            groupNameToId.setData(++fUsedGroups.lastId, ourName);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, groupNameToId.group, PATCHBAY_ICON_CARLA, -1, 0.0f, groupNameToId.name);
            fUsedGroups.list.append(groupNameToId);
        }

        // query all jack ports
        if (const char** const ports = jackbridge_get_ports(fClient, nullptr, nullptr, 0))
        {
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

                String jGroupName(groupName.buffer());

                if (parsedGroups.contains(jGroupName))
                {
                    groupId = fUsedGroups.getGroupId(groupName);
                    CARLA_SAFE_ASSERT_CONTINUE(groupId > 0);
                }
                else
                {
                    groupId = ++fUsedGroups.lastId;
                    parsedGroups.add(jGroupName);

                    int pluginId = -1;
                    PatchbayIcon icon = (jackPortFlags & JackPortIsPhysical) ? PATCHBAY_ICON_HARDWARE : PATCHBAY_ICON_APPLICATION;

                    findPluginIdAndIcon(groupName, pluginId, icon);

                    GroupNameToId groupNameToId;
                    groupNameToId.setData(groupId, groupName);

                    callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, groupNameToId.group, icon, pluginId, 0.0f, groupNameToId.name);
                    fUsedGroups.list.append(groupNameToId);
                }

                addPatchbayJackPort(groupId, jackPort, shortPortName, fullPortName, jackPortFlags);
            }

            jackbridge_free(ports);
        }

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

                        char strBuf[STR_MAX+1];
                        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", thisPort.group, thisPort.port, targetPort.group, targetPort.port);
                        strBuf[STR_MAX] = '\0';

                        ConnectionToId connectionToId;
                        connectionToId.setData(++fUsedConnections.lastId, thisPort.group, thisPort.port, targetPort.group, targetPort.port);

                        callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);
                        fUsedConnections.list.append(connectionToId);
                    }

                    jackbridge_free(connections);
                }
            }

            jackbridge_free(ports);
        }
    }

    void addPatchbayJackPort(const uint groupId, const jack_port_t* const jackPort, const char* const shortPortName, const char* const fullPortName, const int jackPortFlags)
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

            if (jackbridge_get_property(uuid, JACKEY_SIGNAL_TYPE, &value, &type) && value != nullptr && type != nullptr && std::strcmp(type, "text/plain") == 0)
            {
                portIsCV  = (std::strcmp(value, "CV") == 0);
                portIsOSC = (std::strcmp(value, "OSC") == 0);
            }
        }

        uint canvasPortFlags = 0x0;
        canvasPortFlags |= portIsInput ? PATCHBAY_PORT_IS_INPUT : 0x0;

        if (portIsCV)
            canvasPortFlags |= PATCHBAY_PORT_TYPE_CV;
        else if (portIsAudio)
            canvasPortFlags |= PATCHBAY_PORT_TYPE_AUDIO;
        else if (portIsMIDI)
            canvasPortFlags |= PATCHBAY_PORT_TYPE_MIDI;

        PortNameToId portNameToId;
        portNameToId.setData(groupId, ++fUsedPorts.lastId, shortPortName, fullPortName);

        callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, portNameToId.group, static_cast<int>(portNameToId.port), static_cast<int>(canvasPortFlags), 0.0f, portNameToId.name);
        fUsedPorts.list.append(portNameToId);

        return; // unused
        (void)portIsOSC;
    }
#endif

    // -------------------------------------------------------------------

    void processPlugin(CarlaPlugin* const plugin, const uint32_t nframes)
    {
        const uint32_t inCount(plugin->getAudioInCount());
        const uint32_t outCount(plugin->getAudioOutCount());

        float* inBuffer[inCount];
        float* outBuffer[outCount];

        float inPeaks[2] = { 0.0f };
        float outPeaks[2] = { 0.0f };

        for (uint32_t i=0; i < inCount; ++i)
        {
            CarlaEngineAudioPort* const port(plugin->getAudioInPort(i));
            inBuffer[i] = port->getBuffer();
        }

        for (uint32_t i=0; i < outCount; ++i)
        {
            CarlaEngineAudioPort* const port(plugin->getAudioOutPort(i));
            outBuffer[i] = port->getBuffer();
        }

        for (uint32_t i=0; i < inCount && i < 2; ++i)
        {
            for (uint32_t j=0; j < nframes; ++j)
            {
                const float absV(std::abs(inBuffer[i][j]));

                if (absV > inPeaks[i])
                    inPeaks[i] = absV;
            }
        }

        plugin->process(inBuffer, outBuffer, nframes);

        for (uint32_t i=0; i < outCount && i < 2; ++i)
        {
            for (uint32_t j=0; j < nframes; ++j)
            {
                const float absV(std::abs(outBuffer[i][j]));

                if (absV > outPeaks[i])
                    outPeaks[i] = absV;
            }
        }

        setPluginPeaks(plugin->getId(), inPeaks, outPeaks);
    }

    // -------------------------------------------------------------------

    #define handlePtr ((CarlaEngineJack*)arg)

    static void carla_jack_thread_init_callback(void*)
    {
#ifdef __SSE2_MATH__
        // Set FTZ and DAZ flags
        _mm_setcsr(_mm_getcsr() | 0x8040);
#endif
    }

    static int carla_jack_bufsize_callback(jack_nframes_t newBufferSize, void* arg)
    {
        handlePtr->handleJackBufferSizeCallback(newBufferSize);
        return 0;
    }

    static int carla_jack_srate_callback(jack_nframes_t newSampleRate, void* arg)
    {
        handlePtr->handleJackSampleRateCallback(newSampleRate);
        return 0;
    }

    static void carla_jack_freewheel_callback(int starting, void* arg)
    {
        handlePtr->handleJackFreewheelCallback(bool(starting));
    }

    static int carla_jack_process_callback(jack_nframes_t nframes, void* arg)
    {
        handlePtr->handleJackProcessCallback(nframes);
        return 0;
    }

    static void carla_jack_latency_callback(jack_latency_callback_mode_t mode, void* arg)
    {
        handlePtr->handleJackLatencyCallback(mode);
    }

#ifndef BUILD_BRIDGE
    static void carla_jack_client_registration_callback(const char* name, int reg, void* arg)
    {
        handlePtr->handleJackClientRegistrationCallback(name, (reg != 0));
    }

    static void carla_jack_port_registration_callback(jack_port_id_t port, int reg, void* arg)
    {
        handlePtr->handleJackPortRegistrationCallback(port, (reg != 0));
    }

    static void carla_jack_port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
    {
        handlePtr->handleJackPortConnectCallback(a, b, (connect != 0));
    }

    static int carla_jack_client_rename_callback(const char* oldName, const char* newName, void* arg)
    {
        handlePtr->handleJackClientRenameCallback(oldName, newName);
        return 0;
    }

    static int carla_jack_port_rename_callback(jack_port_id_t port, const char* oldName, const char* newName, void* arg)
    {
        handlePtr->handleJackPortRenameCallback(port, oldName, newName);
        return 0;
    }
#endif

    static void carla_jack_shutdown_callback(void* arg)
    {
        handlePtr->handleJackShutdownCallback();
    }

    #undef handlePtr

    // -------------------------------------------------------------------

#ifndef BUILD_BRIDGE
    static int carla_jack_process_callback_plugin(jack_nframes_t nframes, void* arg)
    {
        CarlaPlugin* const plugin((CarlaPlugin*)arg);

        if (plugin != nullptr && plugin->isEnabled())
        {
            CarlaEngineJack* const engine((CarlaEngineJack*)plugin->getEngine());
            CARLA_SAFE_ASSERT_RETURN(engine != nullptr,0);

            if (plugin->tryLock(engine->fFreewheel))
            {
                plugin->initBuffers();
                engine->saveTransportInfo();
                engine->processPlugin(plugin, nframes);
                plugin->unlock();
            }
        }

        return 0;
    }

    static void carla_jack_latency_callback_plugin(jack_latency_callback_mode_t /*mode*/, void* /*arg*/)
    {
        // TODO
    }

    static void carla_jack_shutdown_callback_plugin(void* arg)
    {
        CarlaPlugin* const plugin((CarlaPlugin*)arg);

        if (plugin != nullptr /*&& plugin->isEnabled()*/)
        {
            CarlaEngine* const engine(plugin->getEngine());
            CARLA_SAFE_ASSERT_RETURN(engine != nullptr,);

            CarlaEngineJackClient* const engineClient((CarlaEngineJackClient*)plugin->getEngineClient());
            CARLA_SAFE_ASSERT_RETURN(engineClient != nullptr,);

            plugin->tryLock(true);
            engineClient->invalidate();
            plugin->unlock();

            engine->callback(ENGINE_CALLBACK_PLUGIN_UNAVAILABLE, plugin->getId(), 0, 0, 0.0f, "Killed by JACK");
        }
    }
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
