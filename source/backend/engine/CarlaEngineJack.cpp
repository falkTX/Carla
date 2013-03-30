/*
 * Carla JACK Engine
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifdef WANT_JACK

#include "CarlaEngineInternal.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaMIDI.h"

#include "jackbridge/jackbridge.h"

#include <cmath>
#include <QtCore/QStringList>

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------
// Plugin Helpers, defined in CarlaPlugin.cpp

extern CarlaEngine*          CarlaPluginGetEngine(CarlaPlugin* const plugin);
extern CarlaEngineAudioPort* CarlaPluginGetAudioInPort(CarlaPlugin* const plugin, uint32_t index);
extern CarlaEngineAudioPort* CarlaPluginGetAudioOutPort(CarlaPlugin* const plugin, uint32_t index);

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine JACK-Audio port

class CarlaEngineJackAudioPort : public CarlaEngineAudioPort
{
public:
    CarlaEngineJackAudioPort(const bool isInput, const ProcessMode processMode, jack_client_t* const client, jack_port_t* const port)
        : CarlaEngineAudioPort(isInput, processMode),
          kClient(client),
          kPort(port)
    {
        carla_debug("CarlaEngineJackAudioPort::CarlaEngineJackAudioPort(%s, %s, %p, %p)", bool2str(isInput), ProcessMode2Str(processMode), client, port);

        if (processMode == PROCESS_MODE_SINGLE_CLIENT || processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_ASSERT(client != nullptr && port != nullptr);
        }
        else
        {
            CARLA_ASSERT(client == nullptr && port == nullptr);
        }
    }

    ~CarlaEngineJackAudioPort()
    {
        carla_debug("CarlaEngineJackAudioPort::~CarlaEngineJackAudioPort()");

        if (kClient != nullptr && kPort != nullptr)
            jackbridge_port_unregister(kClient, kPort);
    }

    void initBuffer(CarlaEngine* const engine)
    {
        CARLA_ASSERT(engine != nullptr);

        if (engine == nullptr)
        {
            fBuffer = nullptr;
            return;
        }

        if (kPort == nullptr)
            return CarlaEngineAudioPort::initBuffer(engine);

        fBuffer = (float*)jackbridge_port_get_buffer(kPort, engine->getBufferSize());

        if (! kIsInput)
           carla_zeroFloat(fBuffer, engine->getBufferSize());
    }

private:
    jack_client_t* const kClient;
    jack_port_t*   const kPort;

    friend class CarlaEngineJack;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackAudioPort)
};

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine JACK-Event port

static const EngineEvent kFallbackJackEngineEvent;

class CarlaEngineJackEventPort : public CarlaEngineEventPort
{
public:
    CarlaEngineJackEventPort(const bool isInput, const ProcessMode processMode, jack_client_t* const client, jack_port_t* const port)
        : CarlaEngineEventPort(isInput, processMode),
          kClient(client),
          kPort(port),
          fJackBuffer(nullptr)
    {
        carla_debug("CarlaEngineJackEventPort::CarlaEngineJackEventPort(%s, %s, %p, %p)", bool2str(isInput), ProcessMode2Str(processMode), client, port);

        if (processMode == PROCESS_MODE_SINGLE_CLIENT || processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_ASSERT(client != nullptr && port != nullptr);
        }
        else
        {
            CARLA_ASSERT(client == nullptr && port == nullptr);
        }
    }

    ~CarlaEngineJackEventPort()
    {
        carla_debug("CarlaEngineJackEventPort::~CarlaEngineJackEventPort()");

        if (kClient != nullptr && kPort != nullptr)
            jackbridge_port_unregister(kClient, kPort);
    }

    void initBuffer(CarlaEngine* const engine)
    {
        CARLA_ASSERT(engine != nullptr);

        if (engine == nullptr)
        {
            fJackBuffer = nullptr;
            return;
        }

        if (kPort == nullptr)
            return CarlaEngineEventPort::initBuffer(engine);

        fJackBuffer = jackbridge_port_get_buffer(kPort, engine->getBufferSize());

        if (! kIsInput)
            jackbridge_midi_clear_buffer(fJackBuffer);
    }

    uint32_t getEventCount()
    {
        if (kPort == nullptr)
            return CarlaEngineEventPort::getEventCount();

        CARLA_ASSERT(kIsInput);
        CARLA_ASSERT(fJackBuffer != nullptr);

        if (! kIsInput)
            return 0;
        if (fJackBuffer == nullptr)
            return 0;

        return jackbridge_midi_get_event_count(fJackBuffer);
    }

    const EngineEvent& getEvent(const uint32_t index)
    {
        if (kPort == nullptr)
            return CarlaEngineEventPort::getEvent(index);

        CARLA_ASSERT(kIsInput);
        CARLA_ASSERT(fJackBuffer != nullptr);

        if (! kIsInput)
            return kFallbackJackEngineEvent;
        if (fJackBuffer == nullptr)
            return kFallbackJackEngineEvent;

        jack_midi_event_t jackEvent;

        if (jackbridge_midi_event_get(&jackEvent, fJackBuffer, index) != 0 || jackEvent.size > 3)
            return kFallbackJackEngineEvent;

        fRetEvent.clear();

        const uint8_t midiStatus  = MIDI_GET_STATUS_FROM_DATA(jackEvent.buffer);
        const uint8_t midiChannel = MIDI_GET_CHANNEL_FROM_DATA(jackEvent.buffer);

        fRetEvent.time    = jackEvent.time;
        fRetEvent.channel = midiChannel;

        if (MIDI_IS_STATUS_CONTROL_CHANGE(midiStatus))
        {
            const uint8_t midiControl = jackEvent.buffer[1];
            fRetEvent.type            = kEngineEventTypeControl;

            if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
            {
                const uint8_t midiBank = jackEvent.buffer[2];

                fRetEvent.ctrl.type  = kEngineControlEventTypeMidiBank;
                fRetEvent.ctrl.param = midiBank;
                fRetEvent.ctrl.value = 0.0;
            }
            else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
            {
                fRetEvent.ctrl.type  = kEngineControlEventTypeAllSoundOff;
                fRetEvent.ctrl.param = 0;
                fRetEvent.ctrl.value = 0.0;
            }
            else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
            {
                fRetEvent.ctrl.type  = kEngineControlEventTypeAllNotesOff;
                fRetEvent.ctrl.param = 0;
                fRetEvent.ctrl.value = 0.0;
            }
            else
            {
                const uint8_t midiValue = jackEvent.buffer[2];

                fRetEvent.ctrl.type  = kEngineControlEventTypeParameter;
                fRetEvent.ctrl.param = midiControl;
                fRetEvent.ctrl.value = double(midiValue)/127.0;
            }
        }
        else if (MIDI_IS_STATUS_PROGRAM_CHANGE(midiStatus))
        {
            const uint8_t midiProgram = jackEvent.buffer[1];
            fRetEvent.type            = kEngineEventTypeControl;

            fRetEvent.ctrl.type  = kEngineControlEventTypeMidiProgram;
            fRetEvent.ctrl.param = midiProgram;
            fRetEvent.ctrl.value = 0.0;
        }
        else
        {
            fRetEvent.type = kEngineEventTypeMidi;

            fRetEvent.midi.data[0] = midiStatus;
            fRetEvent.midi.data[1] = jackEvent.buffer[1];
            fRetEvent.midi.data[2] = jackEvent.buffer[2];
            fRetEvent.midi.size    = static_cast<uint8_t>(jackEvent.size);
        }

        return fRetEvent;
    }

    void writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const double value)
    {
        if (kPort == nullptr)
            return CarlaEngineEventPort::writeControlEvent(time, channel, type, param, value);

        CARLA_ASSERT(! kIsInput);
        CARLA_ASSERT(fJackBuffer != nullptr);
        CARLA_ASSERT(type != kEngineControlEventTypeNull);
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(param < MAX_MIDI_VALUE);
        CARLA_SAFE_ASSERT(value >= 0.0 && value <= 1.0);

        if (kIsInput)
            return;
        if (fJackBuffer == nullptr)
            return;
        if (type == kEngineControlEventTypeNull)
            return;
        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (param >= MAX_MIDI_VALUE)
            return;
        if (type == kEngineControlEventTypeParameter)
        {
            CARLA_ASSERT(! MIDI_IS_CONTROL_BANK_SELECT(param));
        }

        const double fixedValue = carla_fixValue<double>(0.0, 1.0, value);

        uint8_t data[3] = { 0 };
        uint8_t size    = 0;

        switch (type)
        {
        case kEngineControlEventTypeNull:
            break;
        case kEngineControlEventTypeParameter:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = static_cast<uint8_t>(param);
            data[2] = uint8_t(fixedValue * 127.0);
            size    = 3;
            break;
        case kEngineControlEventTypeMidiBank:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_BANK_SELECT;
            data[2] = static_cast<uint8_t>(param);
            size    = 3;
            break;
        case kEngineControlEventTypeMidiProgram:
            data[0] = MIDI_STATUS_PROGRAM_CHANGE + channel;
            data[1] = static_cast<uint8_t>(param);
            size    = 2;
            break;
        case kEngineControlEventTypeAllSoundOff:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
            size    = 2;
            break;
        case kEngineControlEventTypeAllNotesOff:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
            size    = 2;
            break;
        }

        if (size > 0)
            jackbridge_midi_event_write(fJackBuffer, time, data, size);
    }

    void writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t port, const uint8_t* const data, const uint8_t size)
    {
        if (kPort == nullptr)
            return CarlaEngineEventPort::writeMidiEvent(time, channel, port, data, size);

        CARLA_ASSERT(! kIsInput);
        CARLA_ASSERT(fJackBuffer != nullptr);
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(data != nullptr);
        CARLA_ASSERT(size > 0);

        if (kIsInput)
            return;
        if (fJackBuffer == nullptr)
            return;
        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (data == nullptr)
            return;
        if (size == 0)
            return;

        uint8_t jdata[size];
        carla_copy<uint8_t>(jdata, data, size);

        jdata[0] = data[0] + channel;

        jackbridge_midi_event_write(fJackBuffer, time, jdata, size);
    }

private:
    jack_client_t* const kClient;
    jack_port_t*   const kPort;

    void*       fJackBuffer;
    EngineEvent fRetEvent;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackEventPort)
};

// -------------------------------------------------------------------------------------------------------------------
// Jack Engine client

class CarlaEngineJackClient : public CarlaEngineClient
{
public:
    CarlaEngineJackClient(const EngineType engineType, const ProcessMode processMode, jack_client_t* const client)
        : CarlaEngineClient(engineType, processMode),
          kClient(client),
          kUseClient(processMode == PROCESS_MODE_SINGLE_CLIENT || processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
    {
        carla_debug("CarlaEngineJackClient::CarlaEngineJackClient(%s, %s, %p)", EngineType2Str(engineType), ProcessMode2Str(processMode), client);

        if (kUseClient)
        {
            CARLA_ASSERT(kClient != nullptr);
        }
        else
        {
            CARLA_ASSERT(kClient == nullptr);
        }
    }

    ~CarlaEngineJackClient()
    {
        carla_debug("CarlaEngineClient::~CarlaEngineClient()");

        if (kProcessMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            if (kClient)
                jackbridge_client_close(kClient);
        }
    }

    void activate()
    {
        carla_debug("CarlaEngineJackClient::activate()");

        if (kProcessMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_ASSERT(kClient && ! fActive);

            if (kClient && ! fActive)
                jackbridge_activate(kClient);
        }

        CarlaEngineClient::activate();
    }

    void deactivate()
    {
        carla_debug("CarlaEngineJackClient::deactivate()");

        if (kProcessMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_ASSERT(kClient && fActive);

            if (kClient && fActive)
                jackbridge_deactivate(kClient);
        }

        CarlaEngineClient::deactivate();
    }

    bool isOk() const
    {
        carla_debug("CarlaEngineJackClient::isOk()");

        if (kUseClient)
            return bool(kClient);

        return CarlaEngineClient::isOk();
    }

#if WANT_JACK_LATENCY
    void setLatency(const uint32_t samples)
    {
        CarlaEngineClient::setLatency(samples);

        if (kUseClient)
            jackbridge_recompute_total_latencies(kClient);
    }
#endif

    CarlaEnginePort* addPort(const EnginePortType portType, const char* const name, const bool isInput)
    {
        carla_debug("CarlaEngineJackClient::addPort(%s, \"%s\", %s)", EnginePortType2Str(portType), name, bool2str(isInput));

        jack_port_t* port = nullptr;

        // Create JACK port first, if needed
        if (kUseClient)
        {
            switch (portType)
            {
            case kEnginePortTypeNull:
                break;
            case kEnginePortTypeAudio:
                port = jackbridge_port_register(kClient, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            case kEnginePortTypeEvent:
                port = jackbridge_port_register(kClient, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            }
        }

        // Create Engine port
        switch (portType)
        {
        case kEnginePortTypeNull:
            break;
        case kEnginePortTypeAudio:
            return new CarlaEngineJackAudioPort(isInput, kProcessMode, kClient, port);
        case kEnginePortTypeEvent:
            return new CarlaEngineJackEventPort(isInput, kProcessMode, kClient, port);
        }

        carla_stderr("CarlaEngineJackClient::addPort(%s, \"%s\", %s) - invalid type", EnginePortType2Str(portType), name, bool2str(isInput));
        return nullptr;
    }

private:
    jack_client_t* const kClient;
    const bool           kUseClient;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackClient)
};

// -------------------------------------------------------------------------------------------------------------------
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
          fHasQuit(false)
#else
# ifndef QTCREATOR_TEST
          fRackPorts{nullptr},
          fLastGroupId(0),
# endif
          fLastPortId(0),
          fLastConnectionId(0)
#endif
    {
        carla_debug("CarlaEngineJack::CarlaEngineJack()");

#ifdef BUILD_BRIDGE
        fOptions.processMode   = PROCESS_MODE_MULTIPLE_CLIENTS;
#endif
        // FIXME: Always enable JACK transport for now
        fOptions.transportMode = TRANSPORT_MODE_JACK;

        carla_zeroStruct<jack_position_t>(fTransportPos);
    }

    ~CarlaEngineJack()
    {
        carla_debug("CarlaEngineJack::~CarlaEngineJack()");
        CARLA_ASSERT(fClient == nullptr);

#ifndef BUILD_BRIDGE
        fUsedGroupNames.clear();
        fUsedPortNames.clear();
        fUsedConnections.clear();
#endif
    }

    // -------------------------------------------------------------------
    // Maximum values

    unsigned int maxClientNameSize() const
    {
        if (fOptions.processMode == PROCESS_MODE_SINGLE_CLIENT || fOptions.processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
            return static_cast<unsigned int>(jackbridge_client_name_size());

        return CarlaEngine::maxClientNameSize();
    }

    unsigned int maxPortNameSize() const
    {
        if (fOptions.processMode == PROCESS_MODE_SINGLE_CLIENT || fOptions.processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
            return static_cast<unsigned int>(jackbridge_port_name_size());

        return CarlaEngine::maxPortNameSize();
    }

    // -------------------------------------------------------------------
    // Virtual, per-engine type calls

    bool init(const char* const clientName)
    {
        carla_debug("CarlaEngineJack::init(\"%s\")", clientName);

        fFreewheel      = false;
        fTransportState = JackTransportStopped;

        carla_zeroStruct<jack_position_t>(fTransportPos);

#ifndef BUILD_BRIDGE
        fLastGroupId = 0;
        fLastPortId  = 0;
        fLastConnectionId = 0;

        fUsedGroupNames.clear();
        fUsedPortNames.clear();
        fUsedConnections.clear();

        fClient = jackbridge_client_open(clientName, JackNullOption, nullptr);

        if (fClient != nullptr)
        {
            fBufferSize = jackbridge_get_buffer_size(fClient);
            fSampleRate = jackbridge_get_sample_rate(fClient);

            jackbridge_set_buffer_size_callback(fClient, carla_jack_bufsize_callback, this);
            jackbridge_set_sample_rate_callback(fClient, carla_jack_srate_callback, this);
            jackbridge_set_freewheel_callback(fClient, carla_jack_freewheel_callback, this);
            jackbridge_set_process_callback(fClient, carla_jack_process_callback, this);
            jackbridge_on_shutdown(fClient, carla_jack_shutdown_callback, this);
# if WANT_JACK_LATENCY
            jackbridge_set_latency_callback(fClient, carla_jack_latency_callback, this);
# endif

            const char* const jackClientName = jackbridge_get_client_name(fClient);

            initJackPatchbay(jackClientName);

            // TODO - update jackbridge
            jack_set_client_registration_callback(fClient, carla_jack_client_registration_callback, this);
            jack_set_port_registration_callback(fClient, carla_jack_port_registration_callback, this);
            jack_set_port_connect_callback(fClient, carla_jack_port_connect_callback, this);

#ifdef WANT_JACK_PORT_RENAME
            if (jack_set_port_rename_callback)
                jack_set_port_rename_callback(fClient, carla_jack_port_rename_callback, this);
#endif

            if (fOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
            {
                fRackPorts[rackPortAudioIn1]  = jackbridge_port_register(fClient, "audio-in1",  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                fRackPorts[rackPortAudioIn2]  = jackbridge_port_register(fClient, "audio-in2",  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                fRackPorts[rackPortAudioOut1] = jackbridge_port_register(fClient, "audio-out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                fRackPorts[rackPortAudioOut2] = jackbridge_port_register(fClient, "audio-out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                fRackPorts[rackPortEventIn]   = jackbridge_port_register(fClient, "events-in",  JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                fRackPorts[rackPortEventOut]  = jackbridge_port_register(fClient, "events-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
            }

            if (jackbridge_activate(fClient) == 0)
            {
                return CarlaEngine::init(jackClientName);
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
#else
        if (fBufferSize == 0 || fSampleRate == 0.0)
        {
            // open temp client to get initial buffer-size and sample-rate values
            if (jack_client_t* tmpClient = jackbridge_client_open(clientName, JackNullOption, nullptr))
            {
                fBufferSize = jackbridge_get_buffer_size(tmpClient);
                fSampleRate = jackbridge_get_sample_rate(tmpClient);

                jackbridge_client_close(tmpClient);
            }
        }

        return CarlaEngine::init(clientName);
#endif
    }

    bool close()
    {
        carla_debug("CarlaEngineJack::close()");
        CarlaEngine::close();

#ifdef BUILD_BRIDGE
        fClient  = nullptr;
        fHasQuit = true;
        return true;
#else
        if (jackbridge_deactivate(fClient) == 0)
        {
            if (fOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
            {
                jackbridge_port_unregister(fClient, fRackPorts[rackPortAudioIn1]);
                jackbridge_port_unregister(fClient, fRackPorts[rackPortAudioIn2]);
                jackbridge_port_unregister(fClient, fRackPorts[rackPortAudioOut1]);
                jackbridge_port_unregister(fClient, fRackPorts[rackPortAudioOut2]);
                jackbridge_port_unregister(fClient, fRackPorts[rackPortEventIn]);
                jackbridge_port_unregister(fClient, fRackPorts[rackPortEventOut]);
            }

            if (jackbridge_client_close(fClient) == 0)
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
#endif
        return false;
    }

    bool isRunning() const
    {
#ifdef BUILD_BRIDGE
        return (fClient != nullptr || ! fHasQuit);
#else
        return (fClient != nullptr);
#endif
    }

    bool isOffline() const
    {
        return fFreewheel;
    }

    EngineType type() const
    {
        return kEngineTypeJack;
    }

    CarlaEngineClient* addClient(CarlaPlugin* const plugin)
    {
        jack_client_t* client = nullptr;

#ifdef BUILD_BRIDGE
        client = fClient = jackbridge_client_open(plugin->name(), JackNullOption, nullptr);

        fBufferSize = jackbridge_get_buffer_size(client);
        fSampleRate = jackbridge_get_sample_rate(client);

        jackbridge_set_buffer_size_callback(client, carla_jack_bufsize_callback, this);
        jackbridge_set_sample_rate_callback(client, carla_jack_srate_callback, this);
        jackbridge_set_freewheel_callback(client, carla_jack_freewheel_callback, this);
        jackbridge_set_process_callback(client, carla_jack_process_callback, this);
        jackbridge_on_shutdown(client, carla_jack_shutdown_callback, this);
# if WANT_JACK_LATENCY
        jackbridge_set_latency_callback(client, carla_jack_latency_callback, this);
# endif
#else
        if (fOptions.processMode == PROCESS_MODE_SINGLE_CLIENT)
        {
            client = fClient;
        }
        else if (fOptions.processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            client = jackbridge_client_open(plugin->name(), JackNullOption, nullptr);
            jackbridge_set_process_callback(client, carla_jack_process_callback_plugin, plugin);
# if WANT_JACK_LATENCY
            jackbridge_set_latency_callback(client, carla_jack_latency_callback_plugin, plugin);
# endif
        }
#endif

        return new CarlaEngineJackClient(kEngineTypeJack, fOptions.processMode, client);
    }

#ifndef BUILD_BRIDGE
    // -------------------------------------------------------------------
    // Patchbay

    void patchbayConnect(int portA, int portB)
    {
        CARLA_ASSERT(fClient != nullptr);

        if (fClient == nullptr)
            return;

        const char* const portNameA = getFullPortName(portA).toUtf8().constData();
        const char* const portNameB = getFullPortName(portB).toUtf8().constData();

        jack_connect(fClient, portNameA, portNameB);
    }

    void patchbayDisconnect(int connectionId)
    {
        CARLA_ASSERT(fClient != nullptr);

        if (fClient == nullptr)
            return;

        for (int i=0, count=fUsedConnections.count(); i < count; i++)
        {
            if (fUsedConnections[i].id == connectionId)
            {
                const char* const portNameA = getFullPortName(fUsedConnections[i].portOut).toUtf8().constData();
                const char* const portNameB = getFullPortName(fUsedConnections[i].portIn).toUtf8().constData();

                jack_disconnect(fClient, portNameA, portNameB);
                break;
            }
        }
    }

    void patchbayRefresh()
    {
        CARLA_ASSERT(fClient != nullptr);

        if (fClient == nullptr)
            return;

        fLastGroupId = 0;
        fLastPortId  = 0;
        fLastConnectionId = 0;

        fUsedGroupNames.clear();
        fUsedPortNames.clear();
        fUsedConnections.clear();

        initJackPatchbay(jackbridge_get_client_name(fClient));
    }
#endif

    // -------------------------------------------------------------------
    // Transport

    void transportPlay()
    {
        if (fOptions.transportMode == TRANSPORT_MODE_INTERNAL)
            CarlaEngine::transportPlay();
        else if (fClient != nullptr)
            jackbridge_transport_start(fClient);
    }

    void transportPause()
    {
        if (fOptions.transportMode == TRANSPORT_MODE_INTERNAL)
            CarlaEngine::transportPause();
        else if (fClient != nullptr)
            jackbridge_transport_stop(fClient);
    }

    void transportRelocate(const uint32_t frame)
    {
        if (fOptions.transportMode == TRANSPORT_MODE_INTERNAL)
            CarlaEngine::transportRelocate(frame);
        else if (fClient != nullptr)
            jackbridge_transport_locate(fClient, frame);
    }

    // -------------------------------------

protected:
    void handleJackBufferSizeCallback(const uint32_t newBufferSize)
    {
        if (fBufferSize == newBufferSize)
            return;

        fBufferSize = newBufferSize;
        bufferSizeChanged(newBufferSize);
    }

    void handleJackSampleRateCallback(const double newSampleRate)
    {
        if (fSampleRate == newSampleRate)
            return;

        fSampleRate = newSampleRate;
        sampleRateChanged(newSampleRate);
    }

    void handleJackFreewheelCallback(const bool isFreewheel)
    {
        fFreewheel = isFreewheel;
    }

    void saveTransportInfo()
    {
        if (fOptions.transportMode != TRANSPORT_MODE_JACK)
            return;

        fTransportPos.unique_1 = fTransportPos.unique_2 + 1; // invalidate

        fTransportState = jackbridge_transport_query(fClient, &fTransportPos);

        fTimeInfo.playing = (fTransportState == JackTransportRolling);

        if (fTransportPos.unique_1 == fTransportPos.unique_2)
        {
            fTimeInfo.frame = fTransportPos.frame;
            fTimeInfo.time  = fTransportPos.usecs;

            if (fTransportPos.valid & JackPositionBBT)
            {
                fTimeInfo.valid              = EngineTimeInfo::ValidBBT;
                fTimeInfo.bbt.bar            = fTransportPos.bar;
                fTimeInfo.bbt.beat           = fTransportPos.beat;
                fTimeInfo.bbt.tick           = fTransportPos.tick;
                fTimeInfo.bbt.barStartTick   = fTransportPos.bar_start_tick;
                fTimeInfo.bbt.beatsPerBar    = fTransportPos.beats_per_bar;
                fTimeInfo.bbt.beatType       = fTransportPos.beat_type;
                fTimeInfo.bbt.ticksPerBeat   = fTransportPos.ticks_per_beat;
                fTimeInfo.bbt.beatsPerMinute = fTransportPos.beats_per_minute;
            }
            else
                fTimeInfo.valid = 0x0;
        }
        else
        {
            fTimeInfo.frame = 0;
            fTimeInfo.valid = 0x0;
        }
    }

    void handleJackProcessCallback(const uint32_t nframes)
    {
        saveTransportInfo();

#ifndef BUILD_BRIDGE
        if (kData->curPluginCount == 0)
        {
            // pass-through
            if (fOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
            {
                float* const audioIn1  = (float*)jackbridge_port_get_buffer(fRackPorts[rackPortAudioIn1], nframes);
                float* const audioIn2  = (float*)jackbridge_port_get_buffer(fRackPorts[rackPortAudioIn2], nframes);
                float* const audioOut1 = (float*)jackbridge_port_get_buffer(fRackPorts[rackPortAudioOut1], nframes);
                float* const audioOut2 = (float*)jackbridge_port_get_buffer(fRackPorts[rackPortAudioOut2], nframes);
                void*  const eventOut  = jackbridge_port_get_buffer(fRackPorts[rackPortEventOut], nframes);

                CARLA_ASSERT(audioIn1 != nullptr);
                CARLA_ASSERT(audioIn2 != nullptr);
                CARLA_ASSERT(audioOut1 != nullptr);
                CARLA_ASSERT(audioOut2 != nullptr);
                CARLA_ASSERT(eventOut != nullptr);

                carla_copyFloat(audioOut1, audioIn1, nframes);
                carla_copyFloat(audioOut2, audioIn2, nframes);
                jackbridge_midi_clear_buffer(eventOut);
            }

            return proccessPendingEvents();
        }
#endif

#ifdef BUILD_BRIDGE
        CarlaPlugin* const plugin = getPluginUnchecked(0);

        if (plugin && plugin->enabled() && plugin->tryLock())
        {
            plugin->initBuffers();
            processPlugin(plugin, nframes);
            plugin->unlock();
        }
#else
        if (fOptions.processMode == PROCESS_MODE_SINGLE_CLIENT)
        {
            for (unsigned int i=0; i < kData->curPluginCount; i++)
            {
                CarlaPlugin* const plugin = getPluginUnchecked(i);

                if (plugin && plugin->enabled() && plugin->tryLock())
                {
                    plugin->initBuffers();
                    processPlugin(plugin, nframes);
                    plugin->unlock();
                }
            }
        }
        else if (fOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
        {
            // get buffers from jack
            float* const audioIn1  = (float*)jackbridge_port_get_buffer(fRackPorts[rackPortAudioIn1], nframes);
            float* const audioIn2  = (float*)jackbridge_port_get_buffer(fRackPorts[rackPortAudioIn2], nframes);
            float* const audioOut1 = (float*)jackbridge_port_get_buffer(fRackPorts[rackPortAudioOut1], nframes);
            float* const audioOut2 = (float*)jackbridge_port_get_buffer(fRackPorts[rackPortAudioOut2], nframes);
            void* const  eventIn   = jackbridge_port_get_buffer(fRackPorts[rackPortEventIn],  nframes);
            void* const  eventOut  = jackbridge_port_get_buffer(fRackPorts[rackPortEventOut], nframes);

            // assert buffers
            CARLA_ASSERT(audioIn1 != nullptr);
            CARLA_ASSERT(audioIn2 != nullptr);
            CARLA_ASSERT(audioOut1 != nullptr);
            CARLA_ASSERT(audioOut2 != nullptr);
            CARLA_ASSERT(eventIn != nullptr);
            CARLA_ASSERT(eventOut != nullptr);

            // create audio buffers
            float* inBuf[2]  = { audioIn1, audioIn2 };
            float* outBuf[2] = { audioOut1, audioOut2 };

            // initialize input events
            carla_zeroStruct<EngineEvent>(kData->rack.in, RACK_EVENT_COUNT);
            {
                uint32_t engineEventIndex = 0;

                jack_midi_event_t jackEvent;
                const uint32_t jackEventCount = jackbridge_midi_get_event_count(eventIn);

                for (uint32_t jackEventIndex=0; jackEventIndex < jackEventCount; jackEventIndex++)
                {
                    if (jackbridge_midi_event_get(&jackEvent, eventIn, jackEventIndex) != 0)
                        continue;

                    EngineEvent* const engineEvent = &kData->rack.in[engineEventIndex++];
                    engineEvent->clear();

                    const uint8_t midiStatus  = MIDI_GET_STATUS_FROM_DATA(jackEvent.buffer);
                    const uint8_t midiChannel = MIDI_GET_CHANNEL_FROM_DATA(jackEvent.buffer);

                    engineEvent->time    = jackEvent.time;
                    engineEvent->channel = midiChannel;

                    if (MIDI_IS_STATUS_CONTROL_CHANGE(midiStatus))
                    {
                        const uint8_t midiControl = jackEvent.buffer[1];
                        engineEvent->type         = kEngineEventTypeControl;

                        if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
                        {
                            const uint8_t midiBank  = jackEvent.buffer[2];

                            engineEvent->ctrl.type  = kEngineControlEventTypeMidiBank;
                            engineEvent->ctrl.param = midiBank;
                            engineEvent->ctrl.value = 0.0;
                        }
                        else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
                        {
                            engineEvent->ctrl.type  = kEngineControlEventTypeAllSoundOff;
                            engineEvent->ctrl.param = 0;
                            engineEvent->ctrl.value = 0.0;
                        }
                        else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
                        {
                            engineEvent->ctrl.type  = kEngineControlEventTypeAllNotesOff;
                            engineEvent->ctrl.param = 0;
                            engineEvent->ctrl.value = 0.0;
                        }
                        else
                        {
                            const uint8_t midiValue = jackEvent.buffer[2];

                            engineEvent->ctrl.type  = kEngineControlEventTypeParameter;
                            engineEvent->ctrl.param = midiControl;
                            engineEvent->ctrl.value = double(midiValue)/127.0;
                        }
                    }
                    else if (MIDI_IS_STATUS_PROGRAM_CHANGE(midiStatus))
                    {
                        const uint8_t midiProgram = jackEvent.buffer[1];
                        engineEvent->type         = kEngineEventTypeControl;

                        engineEvent->ctrl.type  = kEngineControlEventTypeMidiProgram;
                        engineEvent->ctrl.param = midiProgram;
                        engineEvent->ctrl.value = 0.0;
                    }
                    else if (jackEvent.size <= 4)
                    {
                        engineEvent->type = kEngineEventTypeMidi;

                        carla_copy<uint8_t>(engineEvent->midi.data, jackEvent.buffer, jackEvent.size);

                        engineEvent->midi.data[0] = midiStatus;
                        engineEvent->midi.size    = static_cast<uint8_t>(jackEvent.size);
                    }

                    if (engineEventIndex >= RACK_EVENT_COUNT)
                        break;
                }
            }

            // process rack
            processRack(inBuf, outBuf, nframes);

            // output control
            {
                jackbridge_midi_clear_buffer(eventOut);

                for (unsigned short i=0; i < RACK_EVENT_COUNT; i++)
                {
                    EngineEvent* const engineEvent = &kData->rack.out[i];

                    uint8_t data[3] = { 0 };
                    uint8_t size    = 0;

                    switch (engineEvent->type)
                    {
                    case kEngineEventTypeNull:
                        break;

                    case kEngineEventTypeControl:
                    {
                        EngineControlEvent* const ctrlEvent = &engineEvent->ctrl;

                        if (ctrlEvent->type == kEngineControlEventTypeParameter && MIDI_IS_CONTROL_BANK_SELECT(ctrlEvent->param))
                        {
                            // FIXME?
                            ctrlEvent->type  = kEngineControlEventTypeMidiBank;
                            ctrlEvent->param = ctrlEvent->value;
                            ctrlEvent->value = 0.0;
                        }

                        switch (ctrlEvent->type)
                        {
                        case kEngineControlEventTypeNull:
                            break;
                        case kEngineControlEventTypeParameter:
                            data[0] = MIDI_STATUS_CONTROL_CHANGE + engineEvent->channel;
                            data[1] = static_cast<uint8_t>(ctrlEvent->param);
                            data[2] = uint8_t(ctrlEvent->value * 127.0);
                            size    = 3;
                            break;
                        case kEngineControlEventTypeMidiBank:
                            data[0] = MIDI_STATUS_CONTROL_CHANGE + engineEvent->channel;
                            data[1] = MIDI_CONTROL_BANK_SELECT;
                            data[2] = static_cast<uint8_t>(ctrlEvent->param);
                            size    = 3;
                            break;
                        case kEngineControlEventTypeMidiProgram:
                            data[0] = MIDI_STATUS_PROGRAM_CHANGE + engineEvent->channel;
                            data[1] = static_cast<uint8_t>(ctrlEvent->param);
                            size    = 2;
                            break;
                        case kEngineControlEventTypeAllSoundOff:
                            data[0] = MIDI_STATUS_CONTROL_CHANGE + engineEvent->channel;
                            data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                            size    = 2;
                            break;
                        case kEngineControlEventTypeAllNotesOff:
                            data[0] = MIDI_STATUS_CONTROL_CHANGE + engineEvent->channel;
                            data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                            size    = 2;
                            break;
                        }
                        break;
                    }

                    case kEngineEventTypeMidi:
                    {
                        EngineMidiEvent* const midiEvent = &engineEvent->midi;

                        data[0] = midiEvent->data[0];
                        data[1] = midiEvent->data[1];
                        data[2] = midiEvent->data[2];
                        size    = midiEvent->size;
                        break;
                    }
                    }

                    if (size > 0)
                        jackbridge_midi_event_write(eventOut, engineEvent->time, data, size);
                }
            }
        }
#endif // ! BUILD_BRIDGE

        proccessPendingEvents();
    }

#if WANT_JACK_LATENCY
    void handleJackLatencyCallback(const jack_latency_callback_mode_t mode)
    {
        if (fOptions.processMode != PROCESS_MODE_SINGLE_CLIENT)
            return;

        for (unsigned int i=0; i < kData->curPluginCount; i++)
        {
            CarlaPlugin* const plugin = getPluginUnchecked(i);

            if (plugin && plugin->enabled())
                latencyPlugin(plugin, mode);
        }
    }
#endif

#ifndef BUILD_BRIDGE
    void handleJackClientRegistrationCallback(const char* name, bool reg)
    {
        if (reg)
        {
            GroupNameToId groupNameToId;
            groupNameToId.id   = fLastGroupId;
            groupNameToId.name = name;

            callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, fLastGroupId, 0, 0.0f, name);
            fUsedGroupNames.append(groupNameToId);
            fLastGroupId++;
        }
        else
        {
            for (int i=0, count=fUsedGroupNames.count(); i < count; i++)
            {
                if (fUsedGroupNames[i].name == name)
                {
                    callback(CALLBACK_PATCHBAY_CLIENT_REMOVED, 0, fUsedGroupNames[i].id, 0, 0.0f, nullptr);
                    fUsedGroupNames.takeAt(i);
                    break;
                }
            }
        }
    }

    void handleJackPortRegistrationCallback(jack_port_id_t port, bool reg)
    {
        jack_port_t* jackPort = jack_port_by_id(fClient, port);

        QString fullName(jack_port_name(jackPort));
        QString groupName = fullName.split(":").at(0);
        int     groupId   = getGroupId(groupName);

        const char* portName = jack_port_short_name(jackPort);

        if (reg)
        {
            bool portIsInput = (jack_port_flags(jackPort) & JackPortIsInput);
            bool portIsAudio = (std::strcmp(jack_port_type(jackPort), JACK_DEFAULT_AUDIO_TYPE) == 0);

            unsigned int portFlags = 0x0;
            portFlags |= portIsInput ? PATCHBAY_PORT_IS_INPUT : PATCHBAY_PORT_IS_OUTPUT;
            portFlags |= portIsAudio ? PATCHBAY_PORT_IS_AUDIO : PATCHBAY_PORT_IS_MIDI;

            PortNameToId portNameToId;
            portNameToId.groupId  = groupId;
            portNameToId.portId   = fLastPortId;
            portNameToId.name     = portName;
            portNameToId.fullName = fullName;

            fUsedPortNames.append(portNameToId);
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, groupId, fLastPortId, portFlags, portName);
            fLastPortId++;
        }
        else
        {
            for (int i=0, count=fUsedPortNames.count(); i < count; i++)
            {
                if (fUsedPortNames[i].groupId == groupId && fUsedPortNames[i].name == portName)
                {
                    callback(CALLBACK_PATCHBAY_PORT_REMOVED, 0, fUsedPortNames[i].portId, 0, 0.0f, nullptr);
                    fUsedPortNames.takeAt(i);
                    break;
                }
            }
        }
    }

    void handleJackPortConnectCallback(jack_port_id_t a, jack_port_id_t b, bool connect)
    {
        jack_port_t* jackPortA = jack_port_by_id(fClient, a);
        jack_port_t* jackPortB = jack_port_by_id(fClient, b);

        int portIdA = getPortId(QString(jack_port_name(jackPortA)));
        int portIdB = getPortId(QString(jack_port_name(jackPortB)));

        if (connect)
        {
            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = portIdA;
            connectionToId.portIn  = portIdB;

            fUsedConnections.append(connectionToId);
            callback(CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, fLastConnectionId, portIdA, portIdB, nullptr);
            fLastConnectionId++;
        }
        else
        {
            for (int i=0, count=fUsedConnections.count(); i < count; i++)
            {
                if (fUsedConnections[i].portOut == portIdA && fUsedConnections[i].portIn == portIdB)
                {
                    callback(CALLBACK_PATCHBAY_CONNECTION_REMOVED, 0, fUsedConnections[i].id, 0, 0.0f, nullptr);
                    fUsedConnections.takeAt(i);
                    break;
                }
            }
        }
    }

    void handleJackPortRenameCallback(jack_port_id_t port, const char* oldName, const char* newName)
    {
        jack_port_t* jackPort = jack_port_by_id(fClient, port);

        QString fullName(oldName);
        QString groupName = fullName.split(":").at(0);
        int     groupId   = getGroupId(groupName);

        const char* portName = jack_port_short_name(jackPort);

        for (int i=0, count=fUsedPortNames.count(); i < count; i++)
        {
            if (fUsedPortNames[i].groupId == groupId && fUsedPortNames[i].name == portName)
            {
                callback(CALLBACK_PATCHBAY_PORT_RENAMED, 0, fUsedPortNames[i].portId, 0, 0.0f, newName);
                fUsedPortNames[i].name = newName;
                break;
            }
        }
    }
#endif

    void handleJackShutdownCallback()
    {
        for (unsigned int i=0; i < kData->curPluginCount; i++)
        {
            //CarlaPlugin* const plugin = getPluginUnchecked(i);

            //if (plugin)
            //    plugin->x_client = nullptr;
        }

        fClient = nullptr;
        callback(CALLBACK_QUIT, 0, 0, 0, 0.0f, nullptr);
    }

    // -------------------------------------

private:
    jack_client_t*         fClient;
    jack_position_t        fTransportPos;
    jack_transport_state_t fTransportState;
    bool fFreewheel;

    // -------------------------------------

#ifdef BUILD_BRIDGE
    bool fHasQuit;
#else
    enum RackPorts {
        rackPortAudioIn1  = 0,
        rackPortAudioIn2  = 1,
        rackPortAudioOut1 = 2,
        rackPortAudioOut2 = 3,
        rackPortEventIn   = 4,
        rackPortEventOut  = 5,
        rackPortCount     = 8
    };

    jack_port_t* fRackPorts[rackPortCount];

    struct GroupNameToId {
        int id;
        QString name;
    };

    struct PortNameToId {
        int groupId;
        int portId;
        QString name;
        QString fullName;
    };

    struct ConnectionToId {
        int id;
        int portOut;
        int portIn;
    };

    int fLastGroupId;
    int fLastPortId;
    int fLastConnectionId ;

    QList<GroupNameToId>  fUsedGroupNames;
    QList<PortNameToId>   fUsedPortNames;
    QList<ConnectionToId> fUsedConnections;

    int getGroupId(QString groupName)
    {
        for (int i=0, count=fUsedGroupNames.count(); i < count; i++)
        {
            if (fUsedGroupNames[i].name == groupName)
            {
                return fUsedGroupNames[i].id;
            }
        }
        return -1;
    }

    int getPortId(QString fullPortName)
    {
        QString groupName = fullPortName.split(":").at(0);
        QString portName  = fullPortName.replace(groupName+":", "");

        int groupId = getGroupId(groupName);

        for (int i=0, count=fUsedPortNames.count(); i < count; i++)
        {
            if (fUsedPortNames[i].groupId == groupId && fUsedPortNames[i].name == portName)
            {
                return fUsedPortNames[i].portId;
            }
        }

        return -1;
    }

    QString& getFullPortName(int portId)
    {
        static QString fallbackString;

        for (int i=0, count=fUsedPortNames.count(); i < count; i++)
        {
            if (fUsedPortNames[i].portId == portId)
            {
                return fUsedPortNames[i].fullName;
            }
        }

        return fallbackString;
    }

    void initJackPatchbay(const char* const ourName)
    {
        // query initial jack ports
        QList<QString> parsedGroups;

        // our client
        {
            GroupNameToId groupNameToId;
            groupNameToId.id   = fLastGroupId;
            groupNameToId.name = ourName;

            callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, fLastGroupId, 0, 0.0f, ourName);
            fUsedGroupNames.append(groupNameToId);
            fLastGroupId++;

            parsedGroups.append(QString(ourName));
        }

        if (const char** ports = jack_get_ports(fClient, nullptr, nullptr, 0))
        {
            for (int i=0; ports[i] != nullptr; i++)
            {
                jack_port_t* jackPort = jack_port_by_name(fClient, ports[i]);
                const char* portName  = jack_port_short_name(jackPort);

                QString fullName(ports[i]);
                QString groupName(fullName.split(":").at(0));
                int     groupId   = -1;

                if (groupName == ourName)
                    continue;

                if (parsedGroups.contains(groupName))
                {
                    groupId = getGroupId(groupName);
                }
                else
                {
                    groupId = fLastGroupId++;

                    GroupNameToId groupNameToId;
                    groupNameToId.id   = groupId;
                    groupNameToId.name = groupName;

                    fUsedGroupNames.append(groupNameToId);
                    parsedGroups.append(groupName);

                    callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, groupId, 0, 0.0f, groupName.toUtf8().constData());
                }

                bool portIsInput = (jack_port_flags(jackPort) & JackPortIsInput);
                bool portIsAudio = (std::strcmp(jack_port_type(jackPort), JACK_DEFAULT_AUDIO_TYPE) == 0);

                unsigned int portFlags = 0x0;
                portFlags |= portIsInput ? PATCHBAY_PORT_IS_INPUT : PATCHBAY_PORT_IS_OUTPUT;
                portFlags |= portIsAudio ? PATCHBAY_PORT_IS_AUDIO : PATCHBAY_PORT_IS_MIDI;

                PortNameToId portNameToId;
                portNameToId.groupId  = groupId;
                portNameToId.portId   = fLastPortId;
                portNameToId.name     = portName;
                portNameToId.fullName = fullName;

                fUsedPortNames.append(portNameToId);
                callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, groupId, fLastPortId, portFlags, portName);
                fLastPortId++;
            }

            jack_free(ports);
        }

        // query connections, after all ports are in place
        if (const char** ports = jack_get_ports(fClient, nullptr, nullptr, JackPortIsOutput))
        {
            for (int i=0; ports[i] != nullptr; i++)
            {
                jack_port_t* jackPort = jack_port_by_name(fClient, ports[i]);

                int thisPortId = getPortId(QString(ports[i]));

                if (const char** jackConnections = jack_port_get_connections(jackPort))
                {
                    for (int j=0; jackConnections[j] != nullptr; j++)
                    {
                        int targetPortId = getPortId(QString(jackConnections[j]));

                        ConnectionToId connectionToId;
                        connectionToId.id      = fLastConnectionId;
                        connectionToId.portOut = thisPortId;
                        connectionToId.portIn  = targetPortId;

                        fUsedConnections.append(connectionToId);
                        callback(CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, fLastConnectionId, thisPortId, targetPortId, nullptr);
                        fLastConnectionId++;
                    }

                    jack_free(jackConnections);
                }
            }

            jack_free(ports);
        }
    }
#endif

    // -------------------------------------

    void processPlugin(CarlaPlugin* const plugin, const uint32_t nframes)
    {
        const uint32_t inCount  = plugin->audioInCount();
        const uint32_t outCount = plugin->audioOutCount();

        float* inBuffer[inCount];
        float* outBuffer[outCount];

        float inPeaks[inCount];
        float outPeaks[outCount];

        if (inCount > 0)
            carla_zeroFloat(inPeaks, inCount);
        if (outCount > 0)
            carla_zeroFloat(outPeaks, outCount);

        for (uint32_t i=0; i < inCount; i++)
        {
            CarlaEngineAudioPort* const port = CarlaPluginGetAudioInPort(plugin, i);
            inBuffer[i] = port->getBuffer();
        }

        for (uint32_t i=0; i < outCount; i++)
        {
            CarlaEngineAudioPort* const port = CarlaPluginGetAudioOutPort(plugin, i);
            outBuffer[i] = port->getBuffer();
        }

        for (uint32_t i=0; i < inCount; i++)
        {
            for (uint32_t j=0; j < nframes; j++)
            {
                const float absV = std::fabs(inBuffer[i][j]);

                if (absV > inPeaks[i])
                    inPeaks[i] = absV;
            }
        }

        plugin->process(inBuffer, outBuffer, nframes);

        for (uint32_t i=0; i < outCount; i++)
        {
            for (uint32_t j=0; j < nframes; j++)
            {
                const float absV = std::fabs(outBuffer[i][j]);

                if (absV > outPeaks[i])
                    outPeaks[i] = absV;
            }
        }

        setPeaks(plugin->id(), inPeaks, outPeaks);
    }

#if WANT_JACK_LATENCY
    void latencyPlugin(CarlaPlugin* const plugin, jack_latency_callback_mode_t mode)
    {
        const uint32_t inCount  = plugin->audioInCount();
        const uint32_t outCount = plugin->audioOutCount();

        jack_latency_range_t range;
        uint32_t pluginLatency = plugin->latency();

        if (pluginLatency == 0)
            return;

        if (mode == JackCaptureLatency)
        {
            for (uint32_t i=0; i < inCount; i++)
            {
                uint32_t aOutI = (i >= outCount) ? outCount : i;
                jack_port_t* const portIn  = ((CarlaEngineJackAudioPort*)CarlaPluginGetAudioInPort(plugin, i))->kPort;
                jack_port_t* const portOut = ((CarlaEngineJackAudioPort*)CarlaPluginGetAudioOutPort(plugin, aOutI))->kPort;

                jackbridge_port_get_latency_range(portIn, mode, &range);
                range.min += pluginLatency;
                range.max += pluginLatency;
                jackbridge_port_set_latency_range(portOut, mode, &range);
            }
        }
        else
        {
            for (uint32_t i=0; i < outCount; i++)
            {
                uint32_t aInI = (i >= inCount) ? inCount : i;
                jack_port_t* const portIn  = ((CarlaEngineJackAudioPort*)CarlaPluginGetAudioInPort(plugin, aInI))->kPort;
                jack_port_t* const portOut = ((CarlaEngineJackAudioPort*)CarlaPluginGetAudioOutPort(plugin, i))->kPort;

                jackbridge_port_get_latency_range(portOut, mode, &range);
                range.min += pluginLatency;
                range.max += pluginLatency;
                jackbridge_port_set_latency_range(portIn, mode, &range);
            }
        }
    }
#endif

    // -------------------------------------

    #define handlePtr ((CarlaEngineJack*)arg)

    static int carla_jack_srate_callback(jack_nframes_t newSampleRate, void* arg)
    {
        handlePtr->handleJackSampleRateCallback(newSampleRate);
        return 0;
    }

    static int carla_jack_bufsize_callback(jack_nframes_t newBufferSize, void* arg)
    {
        handlePtr->handleJackBufferSizeCallback(newBufferSize);
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

#if WANT_JACK_LATENCY
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

    // -------------------------------------

#ifndef BUILD_BRIDGE
    static int carla_jack_process_callback_plugin(jack_nframes_t nframes, void* arg)
    {
        CarlaPlugin* const plugin = (CarlaPlugin*)arg;

        if (plugin != nullptr && plugin->enabled() && plugin->tryLock())
        {
            CarlaEngineJack* const engine = (CarlaEngineJack*)CarlaPluginGetEngine(plugin);

            plugin->initBuffers();
            engine->saveTransportInfo();
            engine->processPlugin(plugin, nframes);
            plugin->unlock();
        }
        else
            carla_stdout("Plugin not enabled or locked");

        return 0;
    }

# if WANT_JACK_LATENCY
    static void carla_jack_latency_callback_plugin(jack_latency_callback_mode_t mode, void* arg)
    {
        CarlaPlugin* const plugin = (CarlaPlugin*)arg;

        if (plugin != nullptr && plugin->enabled())
        {
            CarlaEngineJack* const engine = (CarlaEngineJack*)CarlaPluginGetEngine(plugin);

            engine->latencyPlugin(plugin, mode);
        }
    }
# endif
#endif

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJack)
};

// -----------------------------------------

CarlaEngine* CarlaEngine::newJack()
{
    return new CarlaEngineJack();
}

// -----------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_JACK
