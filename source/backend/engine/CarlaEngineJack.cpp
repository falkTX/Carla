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

#include "jackbridge/JackBridge.hpp"

#include <QtCore/QStringList>

#define URI_CANVAS_CV   "http://kxstudio.sf.net/ns/canvas/cv"
#define URI_CANVAS_ICON "http://kxstudio.sf.net/ns/canvas/icon"

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

class CarlaEngineJack;
class CarlaEngineJackClient;

// -----------------------------------------------------------------------
// Fallback data

static const EngineEvent kFallbackJackEngineEvent = { kEngineEventTypeNull, 0, 0, {{ kEngineControlEventTypeNull, 0, 0.0f }} };

// -----------------------------------------------------------------------
// Carla Engine JACK-Audio port

class CarlaEngineJackAudioPort : public CarlaEngineAudioPort
{
public:
    CarlaEngineJackAudioPort(const CarlaEngine& engine, const bool isInputPort, jack_client_t* const client, jack_port_t* const port) noexcept
        : CarlaEngineAudioPort(engine, isInputPort),
          fClient(client),
          fPort(port)
    {
        carla_debug("CarlaEngineJackAudioPort::CarlaEngineJackAudioPort(%s, %p, %p)", bool2str(isInputPort), client, port);

        if (fEngine.getProccessMode() == ENGINE_PROCESS_MODE_SINGLE_CLIENT || fEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_SAFE_ASSERT_RETURN(client != nullptr && port != nullptr,);

            if (const jack_uuid_t uuid = jackbridge_port_uuid(port))
                jackbridge_set_property(client, uuid, URI_CANVAS_CV, "NO", "text/plain");
        }
        else
        {
            CARLA_SAFE_ASSERT(client == nullptr && port == nullptr);
        }
    }

    ~CarlaEngineJackAudioPort() noexcept override
    {
        carla_debug("CarlaEngineJackAudioPort::~CarlaEngineJackAudioPort()");

        if (fClient != nullptr && fPort != nullptr)
        {
            if (const jack_uuid_t uuid = jackbridge_port_uuid(fPort))
                jackbridge_remove_property(fClient, uuid, URI_CANVAS_CV);

            try {
                jackbridge_port_unregister(fClient, fPort);
            } catch(...) {}

            fClient = nullptr;
            fPort   = nullptr;
        }
    }

    void initBuffer() noexcept override
    {
        if (fPort == nullptr)
            return CarlaEngineAudioPort::initBuffer();

        const uint32_t bufferSize(fEngine.getBufferSize());

        try {
            fBuffer = (float*)jackbridge_port_get_buffer(fPort, bufferSize);
        }
        catch(...) {
            fBuffer = nullptr;
            return;
        }

        if (! fIsInput)
            FLOAT_CLEAR(fBuffer, bufferSize);
    }

private:
    jack_client_t* fClient;
    jack_port_t*   fPort;

    friend class CarlaEngineJack;
    friend class CarlaEngineJackClient;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackAudioPort)
};

// -----------------------------------------------------------------------
// Carla Engine JACK-CV port

class CarlaEngineJackCVPort : public CarlaEngineCVPort
{
public:
    CarlaEngineJackCVPort(const CarlaEngine& engine, const bool isInputPort, jack_client_t* const client, jack_port_t* const port) noexcept
        : CarlaEngineCVPort(engine, isInputPort),
          fClient(client),
          fPort(port)
    {
        carla_debug("CarlaEngineJackCVPort::CarlaEngineJackCVPort(%s, %p, %p)", bool2str(isInputPort), client, port);

        if (fEngine.getProccessMode() == ENGINE_PROCESS_MODE_SINGLE_CLIENT || fEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_SAFE_ASSERT_RETURN(client != nullptr && port != nullptr,);

            if (const jack_uuid_t uuid = jackbridge_port_uuid(port))
                jackbridge_set_property(client, uuid, URI_CANVAS_CV, "YES", "text/plain");
        }
        else
        {
            CARLA_SAFE_ASSERT(client == nullptr && port == nullptr);
        }
    }

    ~CarlaEngineJackCVPort() noexcept override
    {
        carla_debug("CarlaEngineJackCVPort::~CarlaEngineJackCVPort()");

        if (fClient != nullptr && fPort != nullptr)
        {
            if (const jack_uuid_t uuid = jackbridge_port_uuid(fPort))
                jackbridge_remove_property(fClient, uuid, URI_CANVAS_CV);

            try {
                jackbridge_port_unregister(fClient, fPort);
            } catch(...) {}

            fClient = nullptr;
            fPort   = nullptr;
        }
    }

    void initBuffer() noexcept override
    {
        if (fPort == nullptr)
            return CarlaEngineCVPort::initBuffer();

        const uint32_t bufferSize(fEngine.getBufferSize());

        try {
            fBuffer = (float*)jackbridge_port_get_buffer(fPort, bufferSize);
        }
        catch(...) {
            fBuffer = nullptr;
            return;
        }

        if (! fIsInput)
            FLOAT_CLEAR(fBuffer, bufferSize);
    }

private:
    jack_client_t* fClient;
    jack_port_t*   fPort;

    friend class CarlaEngineJack;
    friend class CarlaEngineJackClient;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackCVPort)
};

// -----------------------------------------------------------------------
// Carla Engine JACK-Event port

class CarlaEngineJackEventPort : public CarlaEngineEventPort
{
public:
    CarlaEngineJackEventPort(const CarlaEngine& engine, const bool isInputPort, jack_client_t* const client, jack_port_t* const port) noexcept
        : CarlaEngineEventPort(engine, isInputPort),
          fClient(client),
          fPort(port),
          fJackBuffer(nullptr)
    {
        carla_debug("CarlaEngineJackEventPort::CarlaEngineJackEventPort(%s, %p, %p)", bool2str(isInputPort), client, port);

        if (fEngine.getProccessMode() == ENGINE_PROCESS_MODE_SINGLE_CLIENT || fEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_SAFE_ASSERT(client != nullptr && port != nullptr);
        }
        else
        {
            CARLA_SAFE_ASSERT(client == nullptr && port == nullptr);
        }
    }

    ~CarlaEngineJackEventPort() noexcept override
    {
        carla_debug("CarlaEngineJackEventPort::~CarlaEngineJackEventPort()");

        if (fClient != nullptr && fPort != nullptr)
        {
            try {
                jackbridge_port_unregister(fClient, fPort);
            } catch(...) {}

            fClient = nullptr;
            fPort   = nullptr;
        }
    }

    void initBuffer() noexcept override
    {
        if (fPort == nullptr)
            return CarlaEngineEventPort::initBuffer();

        try {
            fJackBuffer = jackbridge_port_get_buffer(fPort, fEngine.getBufferSize());
        }
        catch(...) {
            fJackBuffer = nullptr;
            return;
        }

        if (! fIsInput)
            jackbridge_midi_clear_buffer(fJackBuffer);
    }

    uint32_t getEventCount() const noexcept override
    {
        if (fPort == nullptr)
            return CarlaEngineEventPort::getEventCount();

        CARLA_SAFE_ASSERT_RETURN(fIsInput, 0);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, 0);

        uint32_t count = 0;

        try {
            count = jackbridge_midi_get_event_count(fJackBuffer);
        } catch(...) {}

        return count;
    }

    const EngineEvent& getEvent(const uint32_t index) const noexcept override
    {
        if (fPort == nullptr)
            return CarlaEngineEventPort::getEvent(index);

        CARLA_SAFE_ASSERT_RETURN(fIsInput, kFallbackJackEngineEvent);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, kFallbackJackEngineEvent);

        return getEventUnchecked(index);
    }

    const EngineEvent& getEventUnchecked(const uint32_t index) const noexcept override
    {
        jack_midi_event_t jackEvent;

        bool test = false;

        try {
            test = jackbridge_midi_event_get(&jackEvent, fJackBuffer, index);
        } catch(...) {}

        if (! test)
            return kFallbackJackEngineEvent;

        CARLA_SAFE_ASSERT_RETURN(jackEvent.size <= 0xFF /* uint8_t max */, kFallbackJackEngineEvent);

        fRetEvent.time = jackEvent.time;
        fRetEvent.fillFromMidiData(static_cast<uint8_t>(jackEvent.size), jackEvent.buffer);

        return fRetEvent;
    }

    bool writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const float value) noexcept override
    {
        if (fPort == nullptr)
            return CarlaEngineEventPort::writeControlEvent(time, channel, type, param, value);

        CARLA_SAFE_ASSERT_RETURN(! fIsInput, false);
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
        ctrlEvent.dumpToMidiData(channel, size, data);

        if (size == 0)
            return false;

        bool ret = false;

        try {
            ret = jackbridge_midi_event_write(fJackBuffer, time, data, size);
        } catch(...) {}

        return ret;
    }

    bool writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t port, const uint8_t size, const uint8_t* const data) noexcept override
    {
        if (fPort == nullptr)
            return CarlaEngineEventPort::writeMidiEvent(time, channel, port, size, data);

        CARLA_SAFE_ASSERT_RETURN(! fIsInput, false);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, false);
        CARLA_SAFE_ASSERT_RETURN(size > 0, false);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

        jack_midi_data_t jdata[size];

        jdata[0] = static_cast<jack_midi_data_t>(MIDI_GET_STATUS_FROM_DATA(data) + channel);

        for (uint8_t i=1; i < size; ++i)
            jdata[i] = data[i];

        bool ret = false;

        try {
            ret = jackbridge_midi_event_write(fJackBuffer, time, jdata, size);
        } catch(...) {}

        return ret;
    }

private:
    jack_client_t* fClient;
    jack_port_t*   fPort;
    void*          fJackBuffer;

    mutable EngineEvent fRetEvent;

    friend class CarlaEngineJack;
    friend class CarlaEngineJackClient;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackEventPort)
};

// -----------------------------------------------------------------------
// Jack Engine client

class CarlaEngineJackClient : public CarlaEngineClient
{
public:
    CarlaEngineJackClient(const CarlaEngine& engine, jack_client_t* const client)
        : CarlaEngineClient(engine),
          fClient(client),
          fUseClient(engine.getProccessMode() == ENGINE_PROCESS_MODE_SINGLE_CLIENT || engine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
    {
        carla_debug("CarlaEngineJackClient::CarlaEngineJackClient(%p)", client);

        if (fUseClient)
        {
            CARLA_SAFE_ASSERT(fClient != nullptr);
        }
        else
        {
            CARLA_SAFE_ASSERT(fClient == nullptr);
        }
    }

    ~CarlaEngineJackClient() override
    {
        carla_debug("CarlaEngineClient::~CarlaEngineClient()");

        if (fEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS && fClient != nullptr)
            jackbridge_client_close(fClient);

        fAudioPorts.clear();
        fCVPorts.clear();
        fEventPorts.clear();
    }

    void activate() noexcept override
    {
        carla_debug("CarlaEngineJackClient::activate()");

        if (fEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_SAFE_ASSERT_RETURN(fClient != nullptr && ! fActive,);

            try {
                jackbridge_activate(fClient);
            } catch(...) {}
        }

        CarlaEngineClient::activate();
    }

    void deactivate() noexcept override
    {
        carla_debug("CarlaEngineJackClient::deactivate()");

        if (fEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_SAFE_ASSERT_RETURN(fClient != nullptr && fActive,);

            try {
                jackbridge_deactivate(fClient);
            } catch(...) {}
        }

        CarlaEngineClient::deactivate();
    }

    bool isOk() const noexcept override
    {
        if (fUseClient)
            return (fClient != nullptr);

        return CarlaEngineClient::isOk();
    }

#if 0
    void setLatency(const uint32_t samples) noexcept override
    {
        CarlaEngineClient::setLatency(samples);

        if (fUseClient && fClient != nullptr)
        {
            // try etc
            jackbridge_recompute_total_latencies(fClient);
        }
    }
#endif

    CarlaEnginePort* addPort(const EnginePortType portType, const char* const name, const bool isInput) /*noexcept*/ override
    {
        carla_debug("CarlaEngineJackClient::addPort(%s, \"%s\", %s)", EnginePortType2Str(portType), name, bool2str(isInput));

        jack_port_t* port = nullptr;

        // Create JACK port first, if needed
        if (fUseClient && fClient != nullptr)
        {
            try {
                switch (portType)
                {
                case kEnginePortTypeNull:
                    break;
                case kEnginePortTypeAudio:
                    port = jackbridge_port_register(fClient, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                    break;
                case kEnginePortTypeCV:
                    port = jackbridge_port_register(fClient, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                    break;
                case kEnginePortTypeEvent:
                    port = jackbridge_port_register(fClient, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                    break;
                }
            } catch(...) {}

            if (port == nullptr)
            {
                carla_stderr("CarlaEngineJackClient::addPort(%s, \"%s\", %s) - failed to create JACK port", EnginePortType2Str(portType), name, bool2str(isInput));
                return nullptr;
            }
        }

        // Create Engine port
        switch (portType)
        {
        case kEnginePortTypeNull:
            break;
        case kEnginePortTypeAudio: {
            CarlaEngineJackAudioPort* const enginePort(new CarlaEngineJackAudioPort(fEngine, isInput, fClient, port));
            fAudioPorts.append(enginePort);
            return enginePort;
        }
        case kEnginePortTypeCV: {
            CarlaEngineJackCVPort* const enginePort(new CarlaEngineJackCVPort(fEngine, isInput, fClient, port));
            fCVPorts.append(enginePort);
            return enginePort;
        }
        case kEnginePortTypeEvent: {
            CarlaEngineJackEventPort* const enginePort(new CarlaEngineJackEventPort(fEngine, isInput, fClient, port));
            fEventPorts.append(enginePort);
            return enginePort;
        }
        }

        carla_stderr("CarlaEngineJackClient::addPort(%s, \"%s\", %s) - invalid type", EnginePortType2Str(portType), name, bool2str(isInput));
        return nullptr;
    }

    void invalidate()
    {
        for (LinkedList<CarlaEngineJackAudioPort*>::Itenerator it = fAudioPorts.begin(); it.valid(); it.next())
        {
            CarlaEngineJackAudioPort* const port(it.getValue());
            port->fClient = nullptr;
            port->fPort   = nullptr;
        }

        for (LinkedList<CarlaEngineJackCVPort*>::Itenerator it = fCVPorts.begin(); it.valid(); it.next())
        {
            CarlaEngineJackCVPort* const port(it.getValue());
            port->fClient = nullptr;
            port->fPort   = nullptr;
        }

        for (LinkedList<CarlaEngineJackEventPort*>::Itenerator it = fEventPorts.begin(); it.valid(); it.next())
        {
            CarlaEngineJackEventPort* const port(it.getValue());
            port->fClient = nullptr;
            port->fPort   = nullptr;
        }

        fClient = nullptr;
    }

private:
    jack_client_t* fClient;
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
          fFreewheel(false),
#ifdef BUILD_BRIDGE
          fIsRunning(false)
#else
          fLastGroupId(0),
          fLastPortId(0),
          fLastConnectionId(0)
#endif
    {
        carla_debug("CarlaEngineJack::CarlaEngineJack()");

#ifdef BUILD_BRIDGE
        pData->options.processMode = ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS;
#else
        carla_fill<jack_port_t*>(fRackPorts, kRackPortCount, nullptr);
#endif

        // FIXME: Always enable JACK transport for now
        pData->options.transportMode = ENGINE_TRANSPORT_MODE_JACK;

        carla_zeroStruct<jack_position_t>(fTransportPos);
    }

    ~CarlaEngineJack() override
    {
        carla_debug("CarlaEngineJack::~CarlaEngineJack()");
        CARLA_SAFE_ASSERT(fClient == nullptr);

#ifndef BUILD_BRIDGE
        fUsedGroupNames.clear();
        fUsedPortNames.clear();
        fUsedConnections.clear();
        fNewGroups.clear();
#endif
    }

    // -------------------------------------------------------------------
    // Maximum values

    unsigned int getMaxClientNameSize() const noexcept override
    {
        if (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT || pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            unsigned int ret = 0;

            try {
                ret = static_cast<unsigned int>(jackbridge_client_name_size());
            }
            catch (...) {}

            return ret;
        }

        return CarlaEngine::getMaxClientNameSize();
    }

    unsigned int getMaxPortNameSize() const noexcept override
    {
        if (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT || pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            unsigned int ret = 0;

            try {
                ret = static_cast<unsigned int>(jackbridge_port_name_size());
            }
            catch (...) {}

            return ret;
        }

        return CarlaEngine::getMaxPortNameSize();
    }

    // -------------------------------------------------------------------
    // Virtual, per-engine type calls

    bool init(const char* const clientName) override
    {
        carla_debug("CarlaEngineJack::init(\"%s\")", clientName);

        fFreewheel      = false;
        fTransportState = JackTransportStopped;

        carla_zeroStruct<jack_position_t>(fTransportPos);

#ifdef BUILD_BRIDGE
        if (pData->bufferSize == 0 || pData->sampleRate == 0.0)
        {
            // open temp client to get initial buffer-size and sample-rate values
            if (jack_client_t* tmpClient = jackbridge_client_open(clientName, JackNullOption, nullptr))
            {
                pData->bufferSize = jackbridge_get_buffer_size(tmpClient);
                pData->sampleRate = jackbridge_get_sample_rate(tmpClient);

                jackbridge_client_close(tmpClient);
            }
        }

        fIsRunning = true;

        return CarlaEngine::init(clientName);
#else
        fLastGroupId = 0;
        fLastPortId  = 0;
        fLastConnectionId = 0;

        fUsedGroupNames.clear();
        fUsedPortNames.clear();
        fUsedConnections.clear();
        fNewGroups.clear();

        fClient = jackbridge_client_open(clientName, JackNullOption, nullptr);

        if (fClient != nullptr)
        {
            pData->bufferSize = jackbridge_get_buffer_size(fClient);
            pData->sampleRate = jackbridge_get_sample_rate(fClient);

            jackbridge_set_buffer_size_callback(fClient, carla_jack_bufsize_callback, this);
            jackbridge_set_sample_rate_callback(fClient, carla_jack_srate_callback, this);
            jackbridge_set_freewheel_callback(fClient, carla_jack_freewheel_callback, this);
            //jackbridge_set_latency_callback(fClient, carla_jack_latency_callback, this);
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
                fRackPorts[kRackPortEventIn]   = jackbridge_port_register(fClient, "events-in",  JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                fRackPorts[kRackPortEventOut]  = jackbridge_port_register(fClient, "events-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
            }

            if (jackbridge_activate(fClient))
            {
                CarlaEngine::init(jackClientName);
                return true;
            }
            else
            {
                setLastError("Failed to activate the JACK client");
                jackbridge_client_close(fClient);
                fClient = nullptr;
            }
        }
        else
            setLastError("Failed to create new JACK client");

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
            if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && fRackPorts[0] != nullptr)
            {
                jackbridge_port_unregister(fClient, fRackPorts[kRackPortAudioIn1]);
                jackbridge_port_unregister(fClient, fRackPorts[kRackPortAudioIn2]);
                jackbridge_port_unregister(fClient, fRackPorts[kRackPortAudioOut1]);
                jackbridge_port_unregister(fClient, fRackPorts[kRackPortAudioOut2]);
                jackbridge_port_unregister(fClient, fRackPorts[kRackPortEventIn]);
                jackbridge_port_unregister(fClient, fRackPorts[kRackPortEventOut]);
                carla_fill<jack_port_t*>(fRackPorts, kRackPortCount, nullptr);
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

        fUsedGroupNames.clear();
        fUsedPortNames.clear();
        fUsedConnections.clear();
        fNewGroups.clear();

        return false;
#endif
    }

#ifndef BUILD_BRIDGE
    void idle() override
    {

        CarlaEngine::idle();

        if (fNewGroups.count() == 0)
            return;

        LinkedList<int> newPlugins;
        fNewGroups.spliceInsert(newPlugins, true);

        for (LinkedList<int>::Itenerator it = newPlugins.begin(); it.valid(); it.next())
        {
            const int groupId(it.getValue());
            const char* const groupName(getGroupName(groupId));
            CARLA_SAFE_ASSERT_CONTINUE(groupId > 0 && groupName != nullptr && groupName[0] != '\0');

            int pluginId = -1;
            PatchbayIcon icon = PATCHBAY_ICON_PLUGIN;

            if (findPluginIdAndIcon(groupName, pluginId, icon))
                callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED, static_cast<uint>(groupId), icon, pluginId, 0.0f, nullptr);
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

#ifdef BUILD_BRIDGE
        client = fClient = jackbridge_client_open(plugin->getName(), JackNullOption, nullptr);

        CARLA_SAFE_ASSERT_RETURN(client != nullptr, nullptr);

        pData->bufferSize = jackbridge_get_buffer_size(client);
        pData->sampleRate = jackbridge_get_sample_rate(client);

        jackbridge_set_buffer_size_callback(client, carla_jack_bufsize_callback, this);
        jackbridge_set_sample_rate_callback(client, carla_jack_srate_callback, this);
        jackbridge_set_freewheel_callback(client, carla_jack_freewheel_callback, this);
        //jackbridge_set_latency_callback(client, carla_jack_latency_callback, this);
        jackbridge_set_process_callback(client, carla_jack_process_callback, this);
        jackbridge_on_shutdown(client, carla_jack_shutdown_callback, this);
#else
        if (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
        {
            client = fClient;
        }
        else if (pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            client = jackbridge_client_open(plugin->getName(), JackNullOption, nullptr);

            CARLA_SAFE_ASSERT_RETURN(client != nullptr, nullptr);

            //jackbridge_set_latency_callback(client, carla_jack_latency_callback_plugin, plugin);
            jackbridge_set_process_callback(client, carla_jack_process_callback_plugin, plugin);
            jackbridge_on_shutdown(fClient, carla_jack_shutdown_callback_plugin, plugin);
        }
#endif
        return new CarlaEngineJackClient(*this, client);
    }

#ifndef BUILD_BRIDGE
    const char* renamePlugin(const unsigned int id, const char* const newName) override
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

        bool needsReinit = (pData->options.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT);
        const char* name = getUniquePluginName(newName);

        // TODO - use rename port if single-client and JACK2

        // JACK client rename
        if (pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CarlaEngineJackClient* const client((CarlaEngineJackClient*)plugin->getEngineClient());

#if 0
            if (bridge.client_rename_ptr != nullptr)
            {
                name = bridge.client_rename_ptr(client->fClient, name);
            }
            else
#endif
            {
                // we should not be able to do this, jack really needs to allow client rename
                needsReinit = true;

                if (jack_client_t* const jclient = jackbridge_client_open(name, JackNullOption, nullptr))
                {
                    // close old client
                    plugin->setEnabled(false);

                    if (client->isActive())
                        client->deactivate();

                    plugin->clearBuffers();

                    jackbridge_client_close(client->fClient);

                    // set new client data
                    name = jackbridge_get_client_name(jclient);

                    jackbridge_set_process_callback(jclient, carla_jack_process_callback_plugin, plugin);
                    //jackbridge_set_latency_callback(jclient, carla_jack_latency_callback_plugin, plugin);
                    jackbridge_on_shutdown(jclient, carla_jack_shutdown_callback_plugin, plugin);

                    client->fClient = jclient;
                }
            }
        }

        if (name == nullptr)
            return nullptr;

        // Rename
        plugin->setName(name);

        if (needsReinit)
        {
            // reload plugin to recreate its ports
            const SaveState& saveState(plugin->getSaveState());
            plugin->reload();
            plugin->loadSaveState(saveState);
        }

        return name;
    }

    // -------------------------------------------------------------------
    // Patchbay

    bool patchbayConnect(int portA, int portB) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, false);

        if (fClient == nullptr)
        {
            setLastError("Invalid JACK client");
            return false;
        }

        char portNameA[STR_MAX+1];
        char portNameB[STR_MAX+1];
        getFullPortName(portA, portNameA);
        getFullPortName(portB, portNameB);

        if (! jackbridge_connect(fClient, portNameA, portNameB))
        {
            setLastError("JACK operation failed");
            return false;
        }

        return true;
    }

    bool patchbayDisconnect(uint connectionId) override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, false);

        for (LinkedList<ConnectionToId>::Itenerator it = fUsedConnections.begin(); it.valid(); it.next())
        {
            const ConnectionToId& connectionToId(it.getValue());

            if (connectionToId.id == connectionId)
            {
                char portNameOut[STR_MAX+1];
                char portNameIn[STR_MAX+1];

                getFullPortName(connectionToId.portOut, portNameOut);
                getFullPortName(connectionToId.portIn, portNameIn);

                if (! jackbridge_disconnect(fClient, portNameOut, portNameIn))
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

        fLastGroupId = 0;
        fLastPortId  = 0;
        fLastConnectionId = 0;

        fUsedGroupNames.clear();
        fUsedPortNames.clear();
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
                jack_port_t* const jackPort(jackbridge_port_by_name(fClient, ports[i]));
                const char*  const fullPortName(ports[i]);

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
            retConns[i] = connList.getAt(i);

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

        if (jack_port_t* const port = jackbridge_port_by_name(fClient, connSource))
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

                FLOAT_COPY(audioOut1, audioIn1, nframes);
                FLOAT_COPY(audioOut2, audioIn2, nframes);

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
            for (unsigned int i=0; i < pData->curPluginCount; ++i)
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
            float* inBuf[2]  = { audioIn1, audioIn2 };
            float* outBuf[2] = { audioOut1, audioOut2 };

            // initialize input events
            carla_zeroStruct<EngineEvent>(pData->bufEvents.in, kMaxEngineEventInternalCount);
            {
                uint32_t engineEventIndex = 0;

                jack_midi_event_t jackEvent;
                const uint32_t jackEventCount(jackbridge_midi_get_event_count(eventIn));

                for (uint32_t jackEventIndex=0; jackEventIndex < jackEventCount; ++jackEventIndex)
                {
                    if (! jackbridge_midi_event_get(&jackEvent, eventIn, jackEventIndex))
                        continue;

                    CARLA_SAFE_ASSERT_CONTINUE(jackEvent.size <= 0xFF /* uint8_t max */);

                    EngineEvent& engineEvent(pData->bufEvents.in[engineEventIndex++]);

                    engineEvent.time = jackEvent.time;
                    engineEvent.fillFromMidiData(static_cast<uint8_t>(jackEvent.size), jackEvent.buffer);

                    if (engineEventIndex >= kMaxEngineEventInternalCount)
                        break;
                }
            }

            // process rack
            pData->processRack(inBuf, outBuf, nframes, fFreewheel);

            // output control
            {
                jackbridge_midi_clear_buffer(eventOut);

                for (unsigned short i=0; i < kMaxEngineEventInternalCount; ++i)
                {
                    const EngineEvent& engineEvent(pData->bufEvents.out[i]);

                    uint8_t        size    = 0;
                    uint8_t        data[3] = { 0, 0, 0 };
                    const uint8_t* dataPtr = data;

                    switch (engineEvent.type)
                    {
                    case kEngineEventTypeNull:
                        break;

                    case kEngineEventTypeControl:
                    {
                        const EngineControlEvent& ctrlEvent(engineEvent.ctrl);
                        ctrlEvent.dumpToMidiData(engineEvent.channel, size, data);
                        break;
                    }

                    case kEngineEventTypeMidi:
                    {
                        const EngineMidiEvent& midiEvent(engineEvent.midi);

                        size = midiEvent.size;

                        if (size > EngineMidiEvent::kDataSize && midiEvent.dataExt != nullptr)
                            dataPtr = midiEvent.dataExt;
                        else
                            dataPtr = midiEvent.dataExt;

                        break;
                    }
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

#if 0
    void handleJackLatencyCallback(const jack_latency_callback_mode_t mode)
    {
        if (pData->options.processMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            return;

        for (unsigned int i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin != nullptr && plugin->isEnabled())
                latencyPlugin(plugin, mode);
        }
    }
#endif

#ifndef BUILD_BRIDGE
    void handleJackClientRegistrationCallback(const char* const name, const bool reg)
    {
        // do nothing on client registration, wait for first port
        if (reg) return;

        const int id(getGroupId(name)); // also checks name nullness

        if (id < 0)
            return;

        GroupNameToId groupNameToId;
        groupNameToId.setData(id, name);
        fUsedGroupNames.removeAll(groupNameToId);

        callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED, static_cast<uint>(id), 0, 0, 0.0f, nullptr);
    }

    void handleJackPortRegistrationCallback(const jack_port_id_t port, const bool reg)
    {
        jack_port_t* const jackPort(jackbridge_port_by_id(fClient, port));
        const char*  const portName(jackbridge_port_short_name(jackPort));
        const char*  const fullPortName(jackbridge_port_name(jackPort));

        CARLA_SAFE_ASSERT_RETURN(jackPort != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(portName != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fullPortName != nullptr,);

        CarlaString groupName(fullPortName);
        groupName.truncate(groupName.rfind(portName)-1);

        int groupId = getGroupId(groupName);

        if (reg)
        {
            const int jackPortFlags(jackbridge_port_flags(jackPort));

            if (groupId < 0)
            {
                groupId = fLastGroupId++;
                CARLA_SAFE_ASSERT_RETURN(groupId >= 0,);

                GroupNameToId groupNameToId;
                groupNameToId.setData(groupId, groupName);
                fUsedGroupNames.append(groupNameToId);

                PatchbayIcon icon = (jackPortFlags & JackPortIsPhysical) ? PATCHBAY_ICON_HARDWARE : PATCHBAY_ICON_APPLICATION;

                callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, static_cast<uint>(groupId), icon, -1, 0.0f, groupName);

                fNewGroups.append(groupId);
            }

            bool portIsInput = (jackPortFlags & JackPortIsInput);
            bool portIsAudio = (std::strcmp(jackbridge_port_type(jackPort), JACK_DEFAULT_AUDIO_TYPE) == 0);
            bool portIsCV    = false;

            if (const jack_uuid_t uuid = jackbridge_port_uuid(jackPort))
            {
                char* value = nullptr;
                char* type  = nullptr;

                if (jackbridge_get_property(uuid, URI_CANVAS_CV, &value, &type) && value != nullptr && type != nullptr && std::strcmp(type, "text/plain") == 0)
                    portIsCV = (std::strcmp(value, "YES") == 0);
            }

            unsigned int canvasPortFlags = 0x0;
            canvasPortFlags |= portIsInput ? PATCHBAY_PORT_IS_INPUT : 0x0;
            canvasPortFlags |= portIsAudio ? PATCHBAY_PORT_TYPE_AUDIO : PATCHBAY_PORT_TYPE_MIDI;

            if (portIsAudio && portIsCV)
                canvasPortFlags |= PATCHBAY_PORT_TYPE_CV;

            PortNameToId portNameToId;
            portNameToId.setData(groupId, fLastPortId++, portName, fullPortName);
            fUsedPortNames.append(portNameToId);

            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, static_cast<uint>(groupId), portNameToId.portId, static_cast<int>(canvasPortFlags), 0.0f, portName);
        }
        else
        {
            const int portId(getPortId(fullPortName));

            CARLA_SAFE_ASSERT_RETURN(groupId >= 0,);
            CARLA_SAFE_ASSERT_RETURN(portId >= 0,);

            PortNameToId portNameToId;
            portNameToId.setData(groupId, portId, portName, fullPortName);
            fUsedPortNames.removeOne(portNameToId);

            callback(ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED, static_cast<uint>(groupId), portId, 0, 0.0f, nullptr);
        }
    }

    void handleJackPortConnectCallback(const jack_port_id_t a, const jack_port_id_t b, const bool connect)
    {
        jack_port_t* const jackPortA(jackbridge_port_by_id(fClient, a));
        jack_port_t* const jackPortB(jackbridge_port_by_id(fClient, b));
        const char*  const fullPortNameA(jackbridge_port_name(jackPortA));
        const char*  const fullPortNameB(jackbridge_port_name(jackPortB));

        CARLA_SAFE_ASSERT_RETURN(jackPortA != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(jackPortB != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fullPortNameA != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fullPortNameB != nullptr,);

        const int portIdA(getPortId(fullPortNameA));
        const int portIdB(getPortId(fullPortNameB));

        if (portIdA == -1 || portIdB == -1)
            return;

        if (connect)
        {
            ConnectionToId connectionToId;
            connectionToId.setData(fLastConnectionId++, portIdA, portIdB);
            fUsedConnections.append(connectionToId);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);
        }
        else
        {
            for (LinkedList<ConnectionToId>::Itenerator it = fUsedConnections.begin(); it.valid(); it.next())
            {
                const ConnectionToId& connectionToId(it.getValue());

                if (connectionToId.portOut == portIdA && connectionToId.portIn == portIdB)
                {
                    callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED, static_cast<uint>(connectionToId.id), connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);
                    fUsedConnections.remove(it);
                    break;
                }
            }
        }
    }

    void handleJackClientRenameCallback(const char* const oldName, const char* const newName)
    {
        for (LinkedList<GroupNameToId>::Itenerator it = fUsedGroupNames.begin(); it.valid(); it.next())
        {
            GroupNameToId& groupNameToId(it.getValue());

            if (std::strcmp(groupNameToId.name, oldName) == 0)
            {
                groupNameToId.rename(newName);
                callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED, static_cast<uint>(groupNameToId.id), 0, 0, 0.0f, newName);
                break;
            }
        }
    }

    void handleJackPortRenameCallback(const jack_port_id_t port, const char* const oldName, const char* const newName)
    {
        jack_port_t* const jackPort(jackbridge_port_by_id(fClient, port));
        const char*  const portName(jackbridge_port_short_name(jackPort));

        CARLA_SAFE_ASSERT_RETURN(jackPort != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(portName != nullptr,);

        CarlaString groupName(newName);
        groupName.truncate(groupName.rfind(portName)-1);

        const int groupId(getGroupId(groupName));

        CARLA_SAFE_ASSERT_RETURN(groupId >= 0,);

        for (LinkedList<PortNameToId>::Itenerator it = fUsedPortNames.begin(); it.valid(); it.next())
        {
            PortNameToId& portNameId(it.getValue());

            if (std::strcmp(portNameId.fullName, oldName) == 0)
            {
                CARLA_SAFE_ASSERT(portNameId.groupId == groupId);
                portNameId.rename(portName, newName);
                callback(ENGINE_CALLBACK_PATCHBAY_PORT_RENAMED, static_cast<uint>(groupId), portNameId.portId, 0, 0.0f, newName);
                break;
            }
        }
    }
#endif

    void handleJackShutdownCallback()
    {
        for (unsigned int i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin != nullptr)
            {
                if (CarlaEngineJackClient* const client = (CarlaEngineJackClient*)plugin->getEngineClient())
                    client->fClient = nullptr;
            }
        }

#ifndef BUILD_BRIDGE
        carla_fill<jack_port_t*>(fRackPorts, kRackPortCount, nullptr);
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

    struct GroupNameToId {
        int  id;
        char name[STR_MAX+1];

        void clear() noexcept
        {
            id = -1;
            name[0] = '\0';
        }

        void setData(const int i, const char n[]) noexcept
        {
            id = i;
            rename(n);
        }

        void rename(const char n[]) noexcept
        {
            std::strncpy(name, n, STR_MAX);
            name[STR_MAX] = '\0';
        }

        bool operator==(const GroupNameToId& groupNameId) const noexcept
        {
            if (groupNameId.id != id)
                return false;
            if (std::strcmp(groupNameId.name, name) != 0)
                return false;
            return true;
        }

        bool operator!=(const GroupNameToId& groupNameId) const noexcept
        {
            return !operator==(groupNameId);
        }
    };

    struct PortNameToId {
        int  groupId;
        int  portId;
        char name[STR_MAX+1];
        char fullName[STR_MAX+1]; // unique

        void clear() noexcept
        {
            groupId = -1;
            portId  = -1;
            name[0]     = '\0';
            fullName[0] = '\0';
        }

        void setData(const int gId, const int pId, const char n[], const char fn[]) noexcept
        {
            groupId = gId;
            portId  = pId;
            rename(n, fn);
        }

        void rename(const char n[], const char fn[]) noexcept
        {
            std::strncpy(name, n, STR_MAX);
            name[STR_MAX] = '\0';

            std::strncpy(fullName, fn, STR_MAX);
            fullName[STR_MAX] = '\0';
        }

        bool operator==(const PortNameToId& portNameId) noexcept
        {
            if (portNameId.groupId != groupId)
                return false;
            if (portNameId.portId != portId)
                return false;
            if (std::strcmp(portNameId.name, name) != 0)
                return false;
            if (std::strcmp(portNameId.fullName, fullName) != 0)
                return false;
            return true;
        }

        bool operator!=(const PortNameToId& portNameId) noexcept
        {
            return !operator==(portNameId);
        }
    };

    struct ConnectionToId {
        uint id;
        int  portOut;
        int  portIn;

        void clear() noexcept
        {
            id      = 0;
            portOut = -1;
            portIn  = -1;
        }

        void setData(const uint i, const int out, const int in) noexcept
        {
            id      = i;
            portOut = out;
            portIn  = in;
        }

        bool operator==(const ConnectionToId& connectionId) const noexcept
        {
            if (connectionId.id != id)
                return false;
            if (connectionId.portOut != portOut)
                return false;
            if (connectionId.portIn != portIn)
                return false;
            return true;
        }

        bool operator!=(const ConnectionToId& connectionId) const noexcept
        {
            return !operator==(connectionId);
        }
    };

    int  fLastGroupId;
    int  fLastPortId;
    uint fLastConnectionId;

    LinkedList<GroupNameToId>  fUsedGroupNames;
    LinkedList<PortNameToId>   fUsedPortNames;
    LinkedList<ConnectionToId> fUsedConnections;
    LinkedList<int>            fNewGroups;

    int getGroupId(const char* const name)
    {
        CARLA_SAFE_ASSERT_RETURN(name != nullptr, -1);

        for (LinkedList<GroupNameToId>::Itenerator it = fUsedGroupNames.begin(); it.valid(); it.next())
        {
            const GroupNameToId& groupNameId(it.getValue());

            if (std::strcmp(groupNameId.name, name) == 0)
                return groupNameId.id;
        }

        return -1;
    }

    const char* getGroupName(const int groupId)
    {
        static const char fallback[1] = { '\0' };

        CARLA_SAFE_ASSERT_RETURN(groupId >= 0, fallback);

        for (LinkedList<GroupNameToId>::Itenerator it = fUsedGroupNames.begin(); it.valid(); it.next())
        {
            const GroupNameToId& groupNameId(it.getValue());

            if (groupNameId.id == groupId)
                return groupNameId.name;
        }

        return fallback;
    }

    int getPortId(const char* const fullName)
    {
        CARLA_SAFE_ASSERT_RETURN(fullName != nullptr, -1);

        for (LinkedList<PortNameToId>::Itenerator it = fUsedPortNames.begin(); it.valid(); it.next())
        {
            const PortNameToId& portNameId(it.getValue());

            if (std::strcmp(portNameId.fullName, fullName) == 0)
                return portNameId.portId;
        }

        return -1;
    }

    void getFullPortName(const int portId, char nameBuf[STR_MAX+1])
    {
        for (LinkedList<PortNameToId>::Itenerator it = fUsedPortNames.begin(); it.valid(); it.next())
        {
            const PortNameToId& portNameId(it.getValue());

            if (portNameId.portId == portId)
            {
                std::strncpy(nameBuf, portNameId.fullName, STR_MAX);
                nameBuf[STR_MAX] = '\0';
                return;
            }
        }

        nameBuf[0] = '\0';
    }

    bool findPluginIdAndIcon(const char* const clientName, int& pluginId, PatchbayIcon& icon)
    {
        carla_debug("CarlaEngineJack::findPluginIdAndIcon(\"%s\", ...)", clientName);

        for (uint i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin == nullptr || ! plugin->isEnabled())
                continue;

            const CarlaEngineJackClient* const engClient((const CarlaEngineJackClient*)plugin->getEngineClient());
            CARLA_SAFE_ASSERT_CONTINUE(engClient != nullptr && engClient->fClient != nullptr);

            const char* const thisClientName(jackbridge_get_client_name(engClient->fClient));
            CARLA_SAFE_ASSERT_CONTINUE(thisClientName != nullptr && thisClientName[0] != '\0');

            if (std::strcmp(clientName, thisClientName) == 0)
            {
                pluginId = static_cast<int>(i);
                icon = PATCHBAY_ICON_PLUGIN;

                if (const char* const pluginIcon = plugin->getIconName())
                {
                    if (std::strcmp(pluginIcon, "app") == 0 || std::strcmp(pluginIcon, "application") == 0)
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
        }
        return false;
    }

    void initJackPatchbay(const char* const ourName)
    {
        CARLA_SAFE_ASSERT_RETURN(fLastGroupId == 0,);
        CARLA_SAFE_ASSERT_RETURN(fLastPortId == 0,);
        CARLA_SAFE_ASSERT_RETURN(fLastConnectionId == 0,);
        CARLA_SAFE_ASSERT_RETURN(ourName != nullptr,);

        // query initial jack ports
        QStringList parsedGroups;

        // our client
        {
            parsedGroups.append(QString(ourName));

            GroupNameToId groupNameToId;
            groupNameToId.setData(fLastGroupId++, ourName);
            fUsedGroupNames.append(groupNameToId);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, 0 /* our client */, PATCHBAY_ICON_CARLA, -1, 0.0f, ourName);
        }

        if (const char** const ports = jackbridge_get_ports(fClient, nullptr, nullptr, 0))
        {
            for (int i=0; ports[i] != nullptr; ++i)
            {
                jack_port_t* const jackPort(jackbridge_port_by_name(fClient, ports[i]));
                const char*  const portName(jackbridge_port_short_name(jackPort));
                const char*  const fullPortName(ports[i]);

                CARLA_SAFE_ASSERT_CONTINUE(jackPort != nullptr);
                CARLA_SAFE_ASSERT_CONTINUE(portName != nullptr);

                const int jackPortFlags(jackbridge_port_flags(jackPort));

                int groupId = -1;

                bool found;
                CarlaString groupName(fullPortName);
                groupName.truncate(groupName.rfind(portName, &found)-1);

                CARLA_SAFE_ASSERT(found);

                QString qGroupName(groupName.buffer());

                if (parsedGroups.contains(qGroupName))
                {
                    groupId = getGroupId(groupName);
                    CARLA_SAFE_ASSERT(groupId >= 0);
                }
                else
                {
                    groupId = fLastGroupId++;
                    parsedGroups.append(qGroupName);

                    GroupNameToId groupNameToId;
                    groupNameToId.setData(groupId, groupName);
                    fUsedGroupNames.append(groupNameToId);

                    int pluginId = -1;
                    PatchbayIcon icon = (jackPortFlags & JackPortIsPhysical) ? PATCHBAY_ICON_HARDWARE : PATCHBAY_ICON_APPLICATION;

                    findPluginIdAndIcon(groupName, pluginId, icon);

                    callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, static_cast<uint>(groupId), icon, pluginId, 0.0f, groupName);
                }

                bool portIsInput = (jackPortFlags & JackPortIsInput);
                bool portIsAudio = (std::strcmp(jackbridge_port_type(jackPort), JACK_DEFAULT_AUDIO_TYPE) == 0);
                bool portIsCV    = false;

                if (const jack_uuid_t uuid = jackbridge_port_uuid(jackPort))
                {
                    char* value = nullptr;
                    char* type  = nullptr;

                    if (jackbridge_get_property(uuid, URI_CANVAS_CV, &value, &type) && value != nullptr && type != nullptr && std::strcmp(type, "text/plain") == 0)
                        portIsCV = (std::strcmp(value, "YES") == 0);
                }

                unsigned int canvasPortFlags = 0x0;
                canvasPortFlags |= portIsInput ? PATCHBAY_PORT_IS_INPUT : 0x0;
                canvasPortFlags |= portIsAudio ? PATCHBAY_PORT_TYPE_AUDIO : PATCHBAY_PORT_TYPE_MIDI;

                if (portIsCV)
                    canvasPortFlags |= PATCHBAY_PORT_TYPE_CV;

                PortNameToId portNameToId;
                portNameToId.setData(groupId, fLastPortId++, portName, fullPortName);
                fUsedPortNames.append(portNameToId);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, static_cast<uint>(groupId), portNameToId.portId, static_cast<int>(canvasPortFlags), 0.0f, portName);
            }

            jackbridge_free(ports);
        }

        // query connections, after all ports are in place
        if (const char** const ports = jackbridge_get_ports(fClient, nullptr, nullptr, JackPortIsOutput))
        {
            for (int i=0; ports[i] != nullptr; ++i)
            {
                jack_port_t* const jackPort(jackbridge_port_by_name(fClient, ports[i]));
                const char*  const fullPortName(ports[i]);

                const int thisPortId(getPortId(fullPortName));

                CARLA_SAFE_ASSERT_CONTINUE(jackPort != nullptr);
                CARLA_SAFE_ASSERT_CONTINUE(thisPortId != -1);

                if (const char** connections = jackbridge_port_get_all_connections(fClient, jackPort))
                {
                    for (int j=0; connections[j] != nullptr; ++j)
                    {
                        const int targetPortId(getPortId(connections[j]));

                        ConnectionToId connectionToId;
                        connectionToId.setData(fLastConnectionId++, thisPortId, targetPortId);
                        fUsedConnections.append(connectionToId);

                        callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);
                    }

                    jackbridge_free(connections);
                }
            }

            jackbridge_free(ports);
        }
    }
#endif

    // -------------------------------------

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

#if 0
    void latencyPlugin(CarlaPlugin* const plugin, jack_latency_callback_mode_t mode)
    {
        //const uint32_t inCount(plugin->audioInCount());
        //const uint32_t outCount(plugin->audioOutCount());
        const uint32_t latency(plugin->getLatencyInFrames());

        if (latency == 0)
            return;

        //jack_latency_range_t range;

        // TODO

        if (mode == JackCaptureLatency)
        {
            for (uint32_t i=0; i < inCount; ++i)
            {
                uint32_t aOutI = (i >= outCount) ? outCount : i;
                jack_port_t* const portIn  = ((CarlaEngineJackAudioPort*)CarlaPluginGetAudioInPort(plugin, i))->kPort;
                jack_port_t* const portOut = ((CarlaEngineJackAudioPort*)CarlaPluginGetAudioOutPort(plugin, aOutI))->kPort;

                jackbridge_port_get_latency_range(portIn, mode, &range);
                range.min += latency;
                range.max += latency;
                jackbridge_port_set_latency_range(portOut, mode, &range);
            }
        }
        else
        {
            for (uint32_t i=0; i < outCount; ++i)
            {
                uint32_t aInI = (i >= inCount) ? inCount : i;
                jack_port_t* const portIn  = ((CarlaEngineJackAudioPort*)CarlaPluginGetAudioInPort(plugin, aInI))->kPort;
                jack_port_t* const portOut = ((CarlaEngineJackAudioPort*)CarlaPluginGetAudioOutPort(plugin, i))->kPort;

                jackbridge_port_get_latency_range(portOut, mode, &range);
                range.min += latency;
                range.max += latency;
                jackbridge_port_set_latency_range(portIn, mode, &range);
            }
        }
    }
#endif

    // -------------------------------------

    #define handlePtr ((CarlaEngineJack*)arg)

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

#if 0
    static void carla_jack_latency_callback(jack_latency_callback_mode_t mode, void* arg)
    {
        handlePtr->handleJackLatencyCallback(mode);
    }
#endif

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

# if 0
    static void carla_jack_latency_callback_plugin(jack_latency_callback_mode_t mode, void* arg)
    {
        CarlaPlugin* const plugin((CarlaPlugin*)arg);

        if (plugin != nullptr && plugin->isEnabled())
        {
            CarlaEngineJack* const engine((CarlaEngineJack*)plugin->getEngine());
            CARLA_SAFE_ASSERT_RETURN(engine != nullptr,);

            engine->latencyPlugin(plugin, mode);
        }
    }
# endif

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
