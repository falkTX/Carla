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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaEngineInternal.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaMIDI.h"

#include "jackbridge/JackBridge.hpp"

#include <cmath>

#define URI_CANVAS_ICON "http://kxstudio.sf.net/ns/canvas/icon"

#ifdef USE_JUCE
#include "juce_audio_basics.h"

using juce::FloatVectorOperations;
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------------------------------------
// Fallback data

static const EngineEvent kFallbackJackEngineEvent;

// -----------------------------------------------------------------------
// Carla Engine JACK-Audio port

class CarlaEngineJackAudioPort : public CarlaEngineAudioPort
{
public:
    CarlaEngineJackAudioPort(const CarlaEngine& engine, const bool isInput, jack_client_t* const client, jack_port_t* const port)
        : CarlaEngineAudioPort(engine, isInput),
          fClient(client),
          fPort(port)
    {
        carla_debug("CarlaEngineJackAudioPort::CarlaEngineJackAudioPort(name:\"%s\", %s, %p, %p)", engine.getName(), bool2str(isInput), client, port);

        if (fEngine.getProccessMode() == ENGINE_PROCESS_MODE_SINGLE_CLIENT || fEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_ASSERT(client != nullptr && port != nullptr);
        }
        else
        {
            CARLA_ASSERT(client == nullptr && port == nullptr);
        }
    }

    ~CarlaEngineJackAudioPort() override
    {
        carla_debug("CarlaEngineJackAudioPort::~CarlaEngineJackAudioPort()");

        if (fClient != nullptr && fPort != nullptr)
            jackbridge_port_unregister(fClient, fPort);
    }

    void initBuffer() override
    {
        if (fPort == nullptr)
            return CarlaEngineAudioPort::initBuffer();

        const uint32_t bufferSize(fEngine.getBufferSize());

        fBuffer = (float*)jackbridge_port_get_buffer(fPort, bufferSize);

        if (! fIsInput)
        {
#ifdef USE_JUCE
           FloatVectorOperations::clear(fBuffer, bufferSize);
#else
           carla_zeroFloat(fBuffer, bufferSize);
#endif
        }
    }

private:
    jack_client_t* fClient;
    jack_port_t*   fPort;

    friend class CarlaEngineJack;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackAudioPort)
};

// -----------------------------------------------------------------------
// Carla Engine JACK-CV port

class CarlaEngineJackCVPort : public CarlaEngineCVPort
{
public:
    CarlaEngineJackCVPort(const CarlaEngine& engine, const bool isInput, jack_client_t* const client, jack_port_t* const port)
        : CarlaEngineCVPort(engine, isInput),
          fClient(client),
          fPort(port)
    {
        carla_debug("CarlaEngineJackCVPort::CarlaEngineJackCVPort(name:\"%s\", %s, %p, %p)", engine.getName(), bool2str(isInput), client, port);

        if (fEngine.getProccessMode() == ENGINE_PROCESS_MODE_SINGLE_CLIENT || fEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_ASSERT(client != nullptr && port != nullptr);
        }
        else
        {
            CARLA_ASSERT(client == nullptr && port == nullptr);
        }
    }

    ~CarlaEngineJackCVPort() override
    {
        carla_debug("CarlaEngineJackCVPort::~CarlaEngineJackCVPort()");

        if (fClient != nullptr && fPort != nullptr)
            jackbridge_port_unregister(fClient, fPort);
    }

    void initBuffer() override
    {
        if (fPort == nullptr)
            return CarlaEngineCVPort::initBuffer();

        const uint32_t bufferSize(fEngine.getBufferSize());

        if (fIsInput)
        {
            float* const jackBuffer((float*)jackbridge_port_get_buffer(fPort, bufferSize));
#ifdef USE_JUCE
            FloatVectorOperations::copy(fBuffer, jackBuffer, bufferSize);
#else
            carla_copyFloat(fBuffer, jackBuffer, bufferSize);
#endif
        }
        else
        {
#ifdef USE_JUCE
            FloatVectorOperations::clear(fBuffer, bufferSize);
#else
            carla_zeroFloat(fBuffer, bufferSize);
#endif
        }
    }

    void writeBuffer(const uint32_t frames, const uint32_t timeOffset) override
    {
        CARLA_SAFE_ASSERT_RETURN(! fIsInput,);

        float* const jackBuffer((float*)jackbridge_port_get_buffer(fPort, fEngine.getBufferSize()));

#ifdef USE_JUCE
        FloatVectorOperations::copy(jackBuffer+timeOffset, fBuffer, frames);
#else
        carla_copyFloat(jackBuffer+timeOffset, fBuffer, frames);
#endif
    }

private:
    jack_client_t* fClient;
    jack_port_t*   fPort;

    friend class CarlaEngineJack;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackCVPort)
};

// -----------------------------------------------------------------------
// Carla Engine JACK-Event port

class CarlaEngineJackEventPort : public CarlaEngineEventPort
{
public:
    CarlaEngineJackEventPort(const CarlaEngine& engine, const bool isInput, jack_client_t* const client, jack_port_t* const port)
        : CarlaEngineEventPort(engine, isInput),
          fClient(client),
          fPort(port),
          fJackBuffer(nullptr)
    {
        carla_debug("CarlaEngineJackEventPort::CarlaEngineJackEventPort(name:\"%s\", %s, %p, %p)", engine.getName(), bool2str(isInput), client, port);

        if (fEngine.getProccessMode() == ENGINE_PROCESS_MODE_SINGLE_CLIENT || fEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_ASSERT(client != nullptr && port != nullptr);
        }
        else
        {
            CARLA_ASSERT(client == nullptr && port == nullptr);
        }
    }

    ~CarlaEngineJackEventPort() override
    {
        carla_debug("CarlaEngineJackEventPort::~CarlaEngineJackEventPort()");

        if (fClient != nullptr && fPort != nullptr)
            jackbridge_port_unregister(fClient, fPort);
    }

    void initBuffer() override
    {
        if (fPort == nullptr)
            return CarlaEngineEventPort::initBuffer();

        fJackBuffer = jackbridge_port_get_buffer(fPort, fEngine.getBufferSize());

        if (! fIsInput)
            jackbridge_midi_clear_buffer(fJackBuffer);
    }

    uint32_t getEventCount() const override
    {
        if (fPort == nullptr)
            return CarlaEngineEventPort::getEventCount();

        CARLA_SAFE_ASSERT_RETURN(fIsInput, 0);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, 0);

        return jackbridge_midi_get_event_count(fJackBuffer);
    }

    const EngineEvent& getEvent(const uint32_t index) override
    {
        if (fPort == nullptr)
            return CarlaEngineEventPort::getEvent(index);

        CARLA_SAFE_ASSERT_RETURN(fIsInput, kFallbackJackEngineEvent);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, kFallbackJackEngineEvent);

        return getEventUnchecked(index);
    }

    const EngineEvent& getEventUnchecked(const uint32_t index) override
    {
        jack_midi_event_t jackEvent;
        carla_zeroStruct<jack_midi_event_t>(jackEvent);

        if (! jackbridge_midi_event_get(&jackEvent, fJackBuffer, index))
            return kFallbackJackEngineEvent;
        if (jackEvent.size == 0 || jackEvent.size > 4)
            return kFallbackJackEngineEvent;

        fRetEvent.clear();

        const uint8_t midiStatus  = MIDI_GET_STATUS_FROM_DATA(jackEvent.buffer);
        const uint8_t midiChannel = MIDI_GET_CHANNEL_FROM_DATA(jackEvent.buffer);

        fRetEvent.time    = jackEvent.time;
        fRetEvent.channel = midiChannel;

        if (midiStatus == MIDI_STATUS_CONTROL_CHANGE)
        {
            CARLA_ASSERT(jackEvent.size == 2 || jackEvent.size == 3);

            const uint8_t midiControl = jackEvent.buffer[1];
            fRetEvent.type            = kEngineEventTypeControl;

            if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
            {
                const uint8_t midiBank = jackEvent.buffer[2];

                fRetEvent.ctrl.type  = kEngineControlEventTypeMidiBank;
                fRetEvent.ctrl.param = midiBank;
                fRetEvent.ctrl.value = 0.0f;
            }
            else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
            {
                fRetEvent.ctrl.type  = kEngineControlEventTypeAllSoundOff;
                fRetEvent.ctrl.param = 0;
                fRetEvent.ctrl.value = 0.0f;
            }
            else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
            {
                fRetEvent.ctrl.type  = kEngineControlEventTypeAllNotesOff;
                fRetEvent.ctrl.param = 0;
                fRetEvent.ctrl.value = 0.0f;
            }
            else
            {
                CARLA_ASSERT(jackEvent.size == 3);

                const uint8_t midiValue = jackEvent.buffer[2];

                fRetEvent.ctrl.type  = kEngineControlEventTypeParameter;
                fRetEvent.ctrl.param = midiControl;
                fRetEvent.ctrl.value = float(midiValue)/127.0f;
            }
        }
        else if (midiStatus == MIDI_STATUS_PROGRAM_CHANGE)
        {
            CARLA_SAFE_ASSERT_INT2(jackEvent.size == 2, jackEvent.size, jackEvent.buffer[1]);

            const uint8_t midiProgram = jackEvent.buffer[1];
            fRetEvent.type            = kEngineEventTypeControl;

            fRetEvent.ctrl.type  = kEngineControlEventTypeMidiProgram;
            fRetEvent.ctrl.param = midiProgram;
            fRetEvent.ctrl.value = 0.0f;
        }
        else
        {
            fRetEvent.type = kEngineEventTypeMidi;

            fRetEvent.midi.port  = 0;
            fRetEvent.midi.size  = static_cast<uint8_t>(jackEvent.size);

            fRetEvent.midi.data[0] = midiStatus;

            for (uint8_t i=1; i < fRetEvent.midi.size; ++i)
                fRetEvent.midi.data[i] = jackEvent.buffer[i];
        }

        return fRetEvent;
    }

    bool writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const float value) override
    {
        if (fPort == nullptr)
            return CarlaEngineEventPort::writeControlEvent(time, channel, type, param, value);

        CARLA_SAFE_ASSERT_RETURN(! fIsInput, false);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(type != kEngineControlEventTypeNull, false);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, false);
        CARLA_SAFE_ASSERT_RETURN(param < 0x5F, false);
        CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.0f);

        if (type == kEngineControlEventTypeParameter)
        {
            CARLA_ASSERT(! MIDI_IS_CONTROL_BANK_SELECT(param));
        }

        const float fixedValue(carla_fixValue<float>(0.0f, 1.0f, value));

        size_t size = 0;
        jack_midi_data_t data[4] = { 0, 0, 0, 0 };

        switch (type)
        {
        case kEngineControlEventTypeNull:
            break;
        case kEngineControlEventTypeParameter:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = param;
            data[2] = jack_midi_data_t(fixedValue * 127.0f);
            size    = 3;
            break;
        case kEngineControlEventTypeMidiBank:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_BANK_SELECT;
            data[2] = param;
            size    = 3;
            break;
        case kEngineControlEventTypeMidiProgram:
            data[0] = MIDI_STATUS_PROGRAM_CHANGE + channel;
            data[1] = param;
            size    = 2;
            break;
        case kEngineControlEventTypeAllSoundOff:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
            data[2] = 0;
            size    = 3;
            break;
        case kEngineControlEventTypeAllNotesOff:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
            data[2] = 0;
            size    = 3;
            break;
        }

        if (size == 0)
            return false;

        return jackbridge_midi_event_write(fJackBuffer, time, data, size);
    }

    bool writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t port, const uint8_t* const data, const uint8_t size) override
    {
        if (fPort == nullptr)
            return CarlaEngineEventPort::writeMidiEvent(time, channel, port, data, size);

        CARLA_SAFE_ASSERT_RETURN(! fIsInput, false);
        CARLA_SAFE_ASSERT_RETURN(fJackBuffer != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, false);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(size > 0, false);

        jack_midi_data_t jdata[size];
        std::memset(jdata, 0, sizeof(jack_midi_data_t)*size);

        jdata[0]  = MIDI_GET_STATUS_FROM_DATA(data);
        jdata[0] += channel;

        for (uint8_t i=1; i < size; ++i)
            jdata[i] = data[i];

        return jackbridge_midi_event_write(fJackBuffer, time, jdata, size);
    }

private:
    jack_client_t* fClient;
    jack_port_t*   fPort;

    void*       fJackBuffer;
    EngineEvent fRetEvent;

    friend class CarlaEngineJack;

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
        carla_debug("CarlaEngineJackClient::CarlaEngineJackClient(name:\"%s\", %p)", engine.getName(), client);

        if (fUseClient)
        {
            CARLA_ASSERT(fClient != nullptr);
        }
        else
        {
            CARLA_ASSERT(fClient == nullptr);
        }
    }

    ~CarlaEngineJackClient() override
    {
        carla_debug("CarlaEngineClient::~CarlaEngineClient()");

        if (fEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS && fClient != nullptr)
            jackbridge_client_close(fClient);
    }

    void activate() override
    {
        carla_debug("CarlaEngineJackClient::activate()");

        if (fEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_SAFE_ASSERT_RETURN(fClient != nullptr && ! fActive,);

            jackbridge_activate(fClient);
        }

        CarlaEngineClient::activate();
    }

    void deactivate() override
    {
        carla_debug("CarlaEngineJackClient::deactivate()");

        if (fEngine.getProccessMode() == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_SAFE_ASSERT_RETURN(fClient != nullptr && fActive,);

            jackbridge_deactivate(fClient);
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

    CarlaEnginePort* addPort(const EnginePortType portType, const char* const name, const bool isInput) override
    {
        carla_debug("CarlaEngineJackClient::addPort(%s, \"%s\", %s)", EnginePortType2Str(portType), name, bool2str(isInput));

        jack_port_t* port = nullptr;

        // Create JACK port first, if needed
        if (fUseClient && fClient != nullptr)
        {
            switch (portType)
            {
            case kEnginePortTypeNull:
                break;
            case kEnginePortTypeAudio:
                port = jackbridge_port_register(fClient, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            case kEnginePortTypeCV:
                port = jackbridge_port_register(fClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsControlVoltage | (isInput ? JackPortIsInput : JackPortIsOutput), 0);
                break;
            case kEnginePortTypeEvent:
                port = jackbridge_port_register(fClient, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            case kEnginePortTypeOSC:
                break;
            }
        }

        // Create Engine port
        switch (portType)
        {
        case kEnginePortTypeNull:
            break;
        case kEnginePortTypeAudio:
            return new CarlaEngineJackAudioPort(fEngine, isInput, fClient, port);
        case kEnginePortTypeCV:
            return new CarlaEngineJackCVPort(fEngine, isInput, fClient, port);
        case kEnginePortTypeEvent:
            return new CarlaEngineJackEventPort(fEngine, isInput, fClient, port);
        case kEnginePortTypeOSC:
            break;
        }

        carla_stderr("CarlaEngineJackClient::addPort(%s, \"%s\", %s) - invalid type", EnginePortType2Str(portType), name, bool2str(isInput));
        return nullptr;
    }

private:
    jack_client_t* fClient;
    const bool     fUseClient;

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
          fHasQuit(false)
#else
          fLastGroupId(0),
          fLastPortId(0),
          fLastConnectionId(0)
#endif
    {
        carla_debug("CarlaEngineJack::CarlaEngineJack()");

#ifdef BUILD_BRIDGE
        fOptions.processMode = ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS;
#else
        carla_fill<jack_port_t*>(fRackPorts, kRackPortCount, nullptr);
#endif

        // FIXME: Always enable JACK transport for now
        fOptions.transportMode = ENGINE_TRANSPORT_MODE_JACK;

        carla_zeroStruct<jack_position_t>(fTransportPos);
    }

    ~CarlaEngineJack() override
    {
        carla_debug("CarlaEngineJack::~CarlaEngineJack()");
        CARLA_ASSERT(fClient == nullptr);

#ifndef BUILD_BRIDGE
        fUsedGroupNames.clear();
        fUsedPortNames.clear();
        fUsedConnections.clear();
        fGroupIconsChanged.clear();
#endif
    }

    // -------------------------------------------------------------------
    // Maximum values

    unsigned int getMaxClientNameSize() const noexcept override
    {
        if (fOptions.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT || fOptions.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
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
        if (fOptions.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT || fOptions.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
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

#ifndef BUILD_BRIDGE
        fLastGroupId = 0;
        fLastPortId  = 0;
        fLastConnectionId = 0;

        fUsedGroupNames.clear();
        fUsedPortNames.clear();
        fUsedConnections.clear();
        fGroupIconsChanged.clear();

        fClient = jackbridge_client_open(clientName, JackNullOption, nullptr);

        if (fClient != nullptr)
        {
            fBufferSize = jackbridge_get_buffer_size(fClient);
            fSampleRate = jackbridge_get_sample_rate(fClient);

            jackbridge_custom_publish_data(fClient, URI_CANVAS_ICON, "carla", 6);
            jackbridge_custom_set_data_appearance_callback(fClient, carla_jack_custom_appearance_callback, this);

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

            if (fOptions.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
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

    bool close() override
    {
        carla_debug("CarlaEngineJack::close()");
        CarlaEngine::close();

#ifdef BUILD_BRIDGE
        fClient  = nullptr;
        fHasQuit = true;
        return true;
#else
        if (jackbridge_deactivate(fClient))
        {
            if (fOptions.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
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
        fGroupIconsChanged.clear();
#endif
        return false;
    }

#ifndef BUILD_BRIDGE
    void idle() override
    {
        CarlaEngine::idle();

        if (fGroupIconsChanged.count() == 0)
            return;

        static bool checkIcons = false;

        if (! checkIcons)
        {
            checkIcons = true; // check them next time
            return;
        }

        checkIcons = false;

        void*  data;
        size_t dataSize;

        List<int> groupIconsCopy;
        fGroupIconsChanged.spliceAppend(groupIconsCopy, true);

        for (List<int>::Itenerator it = groupIconsCopy.begin(); it.valid(); it.next())
        {
            const int& groupId(*it);
            const char* const groupName(getGroupName(groupId));

            data = nullptr;
            dataSize = 0;

            if (jackbridge_custom_get_data(fClient, groupName, URI_CANVAS_ICON, &data, &dataSize) && data != nullptr && dataSize != 0)
            {
                const char* const icon((const char*)data);
                CARLA_ASSERT(std::strlen(icon)+1 == dataSize);

                callback(ENGINE_CALLBACK_PATCHBAY_ICON_CHANGED, 0, groupId, 0, 0.0f, icon);

                jackbridge_free(data);
            }
        }

        groupIconsCopy.clear();
    }
#endif

    bool isRunning() const noexcept override
    {
#ifdef BUILD_BRIDGE
        return (fClient != nullptr || ! fHasQuit);
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
        const char* const iconName(plugin->getIconName());
        jack_client_t* client = nullptr;

#ifdef BUILD_BRIDGE
        client = fClient = jackbridge_client_open(plugin->getName(), JackNullOption, nullptr);

        CARLA_SAFE_ASSERT_RETURN(client != nullptr, nullptr);

        fBufferSize = jackbridge_get_buffer_size(client);
        fSampleRate = jackbridge_get_sample_rate(client);

        jackbridge_custom_publish_data(client, URI_CANVAS_ICON, iconName, std::strlen(iconName)+1);

        jackbridge_set_buffer_size_callback(client, carla_jack_bufsize_callback, this);
        jackbridge_set_sample_rate_callback(client, carla_jack_srate_callback, this);
        jackbridge_set_freewheel_callback(client, carla_jack_freewheel_callback, this);
        jackbridge_set_latency_callback(client, carla_jack_latency_callback, this);
        jackbridge_set_process_callback(client, carla_jack_process_callback, this);
        jackbridge_on_shutdown(client, carla_jack_shutdown_callback, this);
#else
        if (fOptions.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
        {
            client = fClient;
        }
        else if (fOptions.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            client = jackbridge_client_open(plugin->getName(), JackNullOption, nullptr);

            CARLA_SAFE_ASSERT_RETURN(client != nullptr, nullptr);

            jackbridge_custom_publish_data(client, URI_CANVAS_ICON, iconName, std::strlen(iconName)+1);

            jackbridge_set_latency_callback(client, carla_jack_latency_callback_plugin, plugin);
            jackbridge_set_process_callback(client, carla_jack_process_callback_plugin, plugin);
        }
#endif

        return new CarlaEngineJackClient(*this, client);
    }

#ifndef BUILD_BRIDGE
    const char* renamePlugin(const unsigned int id, const char* const newName) override
    {
        CARLA_ASSERT(pData->curPluginCount > 0);
        CARLA_ASSERT(id < pData->curPluginCount);
        CARLA_ASSERT(pData->plugins != nullptr);
        CARLA_ASSERT(newName != nullptr);

        if (pData->plugins == nullptr)
        {
            setLastError("Critical error: no plugins are currently loaded!");
            return nullptr;
        }

        CarlaPlugin* const plugin(pData->plugins[id].plugin);

        if (plugin == nullptr)
        {
            carla_stderr("CarlaEngine::clonePlugin(%i) - could not find plugin", id);
            return nullptr;
        }

        CARLA_ASSERT(plugin->getId() == id);

        bool needsReinit = (fOptions.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT);
        const char* name = getUniquePluginName(newName);

        // TODO - use rename port if single-client

        // JACK client rename
        if (fOptions.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
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

                if (jack_client_t* jclient = jackbridge_client_open(name, JackNullOption, nullptr))
                {
                    const char* const iconName(plugin->getIconName());
                    jackbridge_custom_publish_data(jclient, URI_CANVAS_ICON, iconName, std::strlen(iconName)+1);

                    // close old client
                    plugin->setEnabled(false);

                    if (client->isActive())
                        client->deactivate();

                    plugin->clearBuffers();

                    jackbridge_client_close(client->fClient);

                    // set new client data
                    name = jackbridge_get_client_name(jclient);

                    jackbridge_set_process_callback(jclient, carla_jack_process_callback_plugin, plugin);
                    jackbridge_set_latency_callback(jclient, carla_jack_latency_callback_plugin, plugin);

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
        CARLA_ASSERT(fClient != nullptr);

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

    bool patchbayDisconnect(int connectionId) override
    {
        CARLA_ASSERT(fClient != nullptr);

        if (fClient == nullptr)
        {
            setLastError("Invalid JACK client");
            return false;
        }

        for (List<ConnectionToId>::Itenerator it = fUsedConnections.begin(); it.valid(); it.next())
        {
            const ConnectionToId& connectionToId(*it);

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

    void patchbayRefresh() override
    {
        CARLA_SAFE_ASSERT_RETURN(fClient != nullptr,);

        fLastGroupId = 0;
        fLastPortId  = 0;
        fLastConnectionId = 0;

        fUsedGroupNames.clear();
        fUsedPortNames.clear();
        fUsedConnections.clear();
        fGroupIconsChanged.clear();

        initJackPatchbay(jackbridge_get_client_name(fClient));
    }
#endif

    // -------------------------------------------------------------------
    // Transport

    void transportPlay() override
    {
        if (fOptions.transportMode == ENGINE_TRANSPORT_MODE_INTERNAL)
            CarlaEngine::transportPlay();
        else if (fClient != nullptr)
            jackbridge_transport_start(fClient);
    }

    void transportPause() override
    {
        if (fOptions.transportMode == ENGINE_TRANSPORT_MODE_INTERNAL)
            CarlaEngine::transportPause();
        else if (fClient != nullptr)
            jackbridge_transport_stop(fClient);
    }

    void transportRelocate(const uint32_t frame) override
    {
        if (fOptions.transportMode == ENGINE_TRANSPORT_MODE_INTERNAL)
            CarlaEngine::transportRelocate(frame);
        else if (fClient != nullptr)
            jackbridge_transport_locate(fClient, frame);
    }

    // -------------------------------------------------------------------

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
        if (fFreewheel == isFreewheel)
            return;

        fFreewheel = isFreewheel;
        offlineModeChanged(isFreewheel);
    }

    void saveTransportInfo()
    {
        if (fOptions.transportMode != ENGINE_TRANSPORT_MODE_JACK)
            return;

        fTransportPos.unique_1 = fTransportPos.unique_2 + 1; // invalidate

        fTransportState = jackbridge_transport_query(fClient, &fTransportPos);

        fTimeInfo.playing = (fTransportState == JackTransportRolling);

        if (fTransportPos.unique_1 == fTransportPos.unique_2)
        {
            fTimeInfo.frame = fTransportPos.frame;
            fTimeInfo.usecs = fTransportPos.usecs;

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

        if (pData->curPluginCount == 0)
        {
#ifndef BUILD_BRIDGE
            // pass-through
            if (fOptions.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
            {
                float* const audioIn1  = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioIn1], nframes);
                float* const audioIn2  = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioIn2], nframes);
                float* const audioOut1 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut1], nframes);
                float* const audioOut2 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut2], nframes);
                void*  const eventOut  = jackbridge_port_get_buffer(fRackPorts[kRackPortEventOut], nframes);

#ifdef USE_JUCE
                FloatVectorOperations::copy(audioOut1, audioIn1, nframes);
                FloatVectorOperations::copy(audioOut2, audioIn2, nframes);
#else
#endif
                jackbridge_midi_clear_buffer(eventOut);
            }
#endif

            return runPendingRtEvents();
        }

#ifdef BUILD_BRIDGE
        CarlaPlugin* const plugin(pData->plugins[0].plugin);

        if (plugin != nullptr && plugin->isEnabled() && plugin->tryLock())
        {
            plugin->initBuffers();
            processPlugin(plugin, nframes);
            plugin->unlock();
        }

        return runPendingRtEvents();
#else
        if (fOptions.processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
        {
            for (unsigned int i=0; i < pData->curPluginCount; ++i)
            {
                CarlaPlugin* const plugin(pData->plugins[i].plugin);

                if (plugin != nullptr && plugin->isEnabled() && plugin->tryLock())
                {
                    plugin->initBuffers();
                    processPlugin(plugin, nframes);
                    plugin->unlock();
                }
            }

            return runPendingRtEvents();
        }

        if (fOptions.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        {
            // get buffers from jack
            float* const audioIn1  = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioIn1], nframes);
            float* const audioIn2  = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioIn2], nframes);
            float* const audioOut1 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut1], nframes);
            float* const audioOut2 = (float*)jackbridge_port_get_buffer(fRackPorts[kRackPortAudioOut2], nframes);
            void* const  eventIn   = jackbridge_port_get_buffer(fRackPorts[kRackPortEventIn],  nframes);
            void* const  eventOut  = jackbridge_port_get_buffer(fRackPorts[kRackPortEventOut], nframes);

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
            carla_zeroStruct<EngineEvent>(pData->bufEvents.in, kEngineMaxInternalEventCount);
            {
                uint32_t engineEventIndex = 0;

                jack_midi_event_t jackEvent;
                const uint32_t jackEventCount(jackbridge_midi_get_event_count(eventIn));

                for (uint32_t jackEventIndex=0; jackEventIndex < jackEventCount; ++jackEventIndex)
                {
                    if (! jackbridge_midi_event_get(&jackEvent, eventIn, jackEventIndex))
                        continue;

                    EngineEvent* const engineEvent(&pData->bufEvents.in[engineEventIndex++]);
                    engineEvent->clear();

                    const uint8_t midiStatus  = MIDI_GET_STATUS_FROM_DATA(jackEvent.buffer);
                    const uint8_t midiChannel = MIDI_GET_CHANNEL_FROM_DATA(jackEvent.buffer);

                    engineEvent->time    = jackEvent.time;
                    engineEvent->channel = midiChannel;

                    if (midiStatus == MIDI_STATUS_CONTROL_CHANGE)
                    {
                        CARLA_ASSERT(jackEvent.size == 2 || jackEvent.size == 3);

                        const uint8_t midiControl = jackEvent.buffer[1];
                        engineEvent->type         = kEngineEventTypeControl;

                        if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
                        {
                            const uint8_t midiBank  = jackEvent.buffer[2];

                            engineEvent->ctrl.type  = kEngineControlEventTypeMidiBank;
                            engineEvent->ctrl.param = midiBank;
                            engineEvent->ctrl.value = 0.0f;
                        }
                        else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
                        {
                            engineEvent->ctrl.type  = kEngineControlEventTypeAllSoundOff;
                            engineEvent->ctrl.param = 0;
                            engineEvent->ctrl.value = 0.0f;
                        }
                        else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
                        {
                            engineEvent->ctrl.type  = kEngineControlEventTypeAllNotesOff;
                            engineEvent->ctrl.param = 0;
                            engineEvent->ctrl.value = 0.0f;
                        }
                        else
                        {
                            CARLA_ASSERT(jackEvent.size == 3);

                            const uint8_t midiValue = jackEvent.buffer[2];

                            engineEvent->ctrl.type  = kEngineControlEventTypeParameter;
                            engineEvent->ctrl.param = midiControl;
                            engineEvent->ctrl.value = float(midiValue)/127.0f;
                        }
                    }
                    else if (midiStatus == MIDI_STATUS_PROGRAM_CHANGE)
                    {
                        CARLA_ASSERT(jackEvent.size == 2);

                        const uint8_t midiProgram = jackEvent.buffer[1];
                        engineEvent->type         = kEngineEventTypeControl;

                        engineEvent->ctrl.type  = kEngineControlEventTypeMidiProgram;
                        engineEvent->ctrl.param = midiProgram;
                        engineEvent->ctrl.value = 0.0f;
                    }
                    else if (jackEvent.size <= 4)
                    {
                        engineEvent->type = kEngineEventTypeMidi;

                        engineEvent->midi.data[0] = midiStatus;
                        engineEvent->midi.size    = static_cast<uint8_t>(jackEvent.size);

                        if (jackEvent.size > 1)
                            carla_copy<uint8_t>(engineEvent->midi.data+1, jackEvent.buffer+1, jackEvent.size-1);
                    }

                    if (engineEventIndex >= kEngineMaxInternalEventCount)
                        break;
                }
            }

            // process rack
            processRack(inBuf, outBuf, nframes);

            // output control
            {
                jackbridge_midi_clear_buffer(eventOut);

                for (unsigned short i=0; i < kEngineMaxInternalEventCount; ++i)
                {
                    EngineEvent* const engineEvent = &pData->bufEvents.out[i];

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
                            ctrlEvent->value = 0.0f;
                        }

                        switch (ctrlEvent->type)
                        {
                        case kEngineControlEventTypeNull:
                            break;
                        case kEngineControlEventTypeParameter:
                            data[0] = MIDI_STATUS_CONTROL_CHANGE + engineEvent->channel;
                            data[1] = static_cast<uint8_t>(ctrlEvent->param);
                            data[2] = uint8_t(ctrlEvent->value * 127.0f);
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

            return runPendingRtEvents();
        }
#endif // ! BUILD_BRIDGE

        runPendingRtEvents();
    }

    void handleJackLatencyCallback(const jack_latency_callback_mode_t mode)
    {
        if (fOptions.processMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            return;

        for (unsigned int i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin != nullptr && plugin->isEnabled())
                latencyPlugin(plugin, mode);
        }
    }

#ifndef BUILD_BRIDGE
    void handleCustomAppearanceCallback(const char* client_name, const char* key, jack_custom_change_t change)
    {
        if ((change == JackCustomAdded || change == JackCustomReplaced) && std::strcmp(key, URI_CANVAS_ICON) == 0)
        {
            const int groupId (getGroupId(client_name));

            if (groupId == -1)
                return;

            fGroupIconsChanged.append(groupId);
        }
    }

    void handleJackClientRegistrationCallback(const char* const name, const bool reg)
    {
        // do nothing on client registration, wait for first port
        if (reg) return;

        const int id(getGroupId(name)); // also checks name nullness

        if (id == -1)
            return;

        GroupNameToId groupNameId(id, name);
        fUsedGroupNames.removeAll(groupNameId);

        callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED, 0, id, 0, 0.0f, name);
    }

    void handleJackPortRegistrationCallback(const jack_port_id_t port, const bool reg)
    {
        jack_port_t* const jackPort(jackbridge_port_by_id(fClient, port));
        const char*  const portName(jackbridge_port_short_name(jackPort));
        const char*  const fullPortName(jackbridge_port_name(jackPort));

        CARLA_ASSERT(jackPort != nullptr);
        CARLA_ASSERT(portName != nullptr);
        CARLA_ASSERT(fullPortName != nullptr);

        if (jackPort == nullptr)
            return;
        if (portName == nullptr)
            return;
        if (fullPortName == nullptr)
            return;

        CarlaString groupName(fullPortName);
        groupName.truncate(groupName.rfind(portName)-1);

        int groupId = getGroupId(groupName);

        if (reg)
        {
            const int jackPortFlags(jackbridge_port_flags(jackPort));

            if (groupId == -1)
            {
                groupId = fLastGroupId++;

                GroupNameToId groupNameToId(groupId, groupName);
                fUsedGroupNames.append(groupNameToId);

                if (jackPortFlags & JackPortIsPhysical)
                {
                    callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, 0, groupId, 0, 0.0f, groupName);
                    // hardware
                }
                else
                {
                    callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, 0, groupId, 0, 0.0f, groupName);
                    fGroupIconsChanged.append(groupId);
                    // "application"
                }
            }

            bool portIsInput = (jackPortFlags & JackPortIsInput);
            bool portIsAudio = (std::strcmp(jackbridge_port_type(jackPort), JACK_DEFAULT_AUDIO_TYPE) == 0);
            bool portIsCV    = (jackPortFlags & JackPortIsControlVoltage);

            unsigned int canvasPortFlags = 0x0;
            canvasPortFlags |= portIsInput ? PATCHBAY_PORT_IS_INPUT : PATCHBAY_PORT_IS_OUTPUT;
            canvasPortFlags |= portIsAudio ? PATCHBAY_PORT_IS_AUDIO : PATCHBAY_PORT_IS_MIDI;

            if (portIsAudio && portIsCV)
                canvasPortFlags |= PATCHBAY_PORT_IS_CV;

            PortNameToId portNameToId(groupId, fLastPortId++, portName, fullPortName);
            fUsedPortNames.append(portNameToId);

            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, 0, groupId, portNameToId.portId, canvasPortFlags, portName);
        }
        else
        {
            const int portId(getPortId(fullPortName));

            CARLA_ASSERT(groupId != -1);
            CARLA_ASSERT(portId != -1);

            if (groupId == -1 || portId == -1)
                return;

            PortNameToId portNameId(groupId, portId, portName, fullPortName);
            fUsedPortNames.removeOne(portNameId);

            callback(ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED, 0, groupId, portId, 0.0f, portName);
        }
    }

    void handleJackPortConnectCallback(const jack_port_id_t a, const jack_port_id_t b, const bool connect)
    {
        jack_port_t* const jackPortA(jackbridge_port_by_id(fClient, a));
        jack_port_t* const jackPortB(jackbridge_port_by_id(fClient, b));
        const char*  const fullPortNameA(jackbridge_port_name(jackPortA));
        const char*  const fullPortNameB(jackbridge_port_name(jackPortB));

        CARLA_ASSERT(jackPortA != nullptr);
        CARLA_ASSERT(jackPortB != nullptr);
        CARLA_ASSERT(fullPortNameA != nullptr);
        CARLA_ASSERT(fullPortNameB != nullptr);

        if (jackPortA == nullptr)
            return;
        if (jackPortB == nullptr)
            return;
        if (fullPortNameA == nullptr)
            return;
        if (fullPortNameB == nullptr)
            return;

        const int portIdA(getPortId(fullPortNameA));
        const int portIdB(getPortId(fullPortNameB));

        if (portIdA == -1 || portIdB == -1)
            return;

        if (connect)
        {
            ConnectionToId connectionToId(fLastConnectionId++, portIdA, portIdB);
            fUsedConnections.append(connectionToId);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, connectionToId.id, portIdA, portIdB, nullptr);
        }
        else
        {
            for (List<ConnectionToId>::Itenerator it = fUsedConnections.begin(); it.valid(); it.next())
            {
                const ConnectionToId& connectionToId(*it);

                if (connectionToId.portOut == portIdA && connectionToId.portIn == portIdB)
                {
                    callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED, 0, connectionToId.id, 0, 0.0f, nullptr);
                    fUsedConnections.remove(it);
                    break;
                }
            }
        }
    }

    void handleJackClientRenameCallback(const char* const oldName, const char* const newName)
    {
        for (List<GroupNameToId>::Itenerator it = fUsedGroupNames.begin(); it.valid(); it.next())
        {
            GroupNameToId& groupNameToId(*it);

            if (std::strcmp(groupNameToId.name, oldName) == 0)
            {
                groupNameToId.rename(newName);
                callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED, 0, groupNameToId.id, 0, 0.0f, newName);
                break;
            }
        }
    }

    void handleJackPortRenameCallback(const jack_port_id_t port, const char* const oldName, const char* const newName)
    {
        jack_port_t* const jackPort(jackbridge_port_by_id(fClient, port));
        const char*  const portName(jackbridge_port_short_name(jackPort));

        CARLA_ASSERT(jackPort != nullptr);
        CARLA_ASSERT(portName != nullptr);

        if (jackPort == nullptr)
            return;
        if (portName == nullptr)
            return;

        CarlaString groupName(newName);
        groupName.truncate(groupName.rfind(portName)-1);

        const int groupId(getGroupId(groupName));

        CARLA_ASSERT(groupId != -1);

        if (groupId == -1)
            return;

        for (List<PortNameToId>::Itenerator it = fUsedPortNames.begin(); it.valid(); it.next())
        {
            PortNameToId& portNameId(*it);

            if (std::strcmp(portNameId.fullName, oldName) == 0)
            {
                CARLA_ASSERT(portNameId.groupId == groupId);
                portNameId.rename(portName, newName);
                callback(ENGINE_CALLBACK_PATCHBAY_PORT_RENAMED, 0, groupId, portNameId.portId, 0.0f, newName);
                break;
            }
        }
    }
#endif

    void handleJackShutdownCallback()
    {
        for (unsigned int i=0; i < pData->curPluginCount; ++i)
        {
            //CarlaPlugin* const plugin(pData->plugins[i].plugin);

            //if (plugin)
            //    plugin->x_client = nullptr;
        }

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
    bool fHasQuit;
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

        GroupNameToId()
        {
            id = -1;
            name[0] = '\0';
        }

        GroupNameToId(const int id, const char name[])
        {
            this->id = id;
            std::strncpy(this->name, name, STR_MAX);
            this->name[STR_MAX] = '\0';
        }

        void rename(const char name[])
        {
            std::strncpy(this->name, name, STR_MAX);
            this->name[STR_MAX] = '\0';
        }

        bool operator==(const GroupNameToId& groupNameId)
        {
            if (groupNameId.id != id)
                return false;
            if (std::strcmp(groupNameId.name, name) != 0)
                return false;
            return true;
        }
    };

    struct PortNameToId {
        int  groupId;
        int  portId;
        char name[STR_MAX+1];
        char fullName[STR_MAX+1]; // unique

        PortNameToId()
        {
            groupId = -1;
            portId  = -1;
            name[0] = '\0';
            fullName[0] = '\0';
        }

        PortNameToId(const int groupId, const int portId, const char name[], const char fullName[])
        {
            this->groupId = groupId;
            this->portId  = portId;
            std::strncpy(this->name, name, STR_MAX);
            this->name[STR_MAX] = '\0';
            std::strncpy(this->fullName, fullName, STR_MAX);
            this->fullName[STR_MAX] = '\0';
        }

        void rename(const char name[], const char fullName[])
        {
            std::strncpy(this->name, name, STR_MAX);
            this->name[STR_MAX] = '\0';
            std::strncpy(this->fullName, fullName, STR_MAX);
            this->fullName[STR_MAX] = '\0';
        }

        bool operator==(const PortNameToId& portNameId)
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
    };

    struct ConnectionToId {
        int id;
        int portOut;
        int portIn;

        ConnectionToId()
        {
            id = -1;
            portOut = -1;
            portIn  = -1;
        }

        ConnectionToId(const int id, const int portOut, const int portIn)
        {
            this->id = id;
            this->portOut = portOut;
            this->portIn  = portIn;
        }

        bool operator==(const ConnectionToId& connectionId)
        {
            if (connectionId.id != id)
                return false;
            if (connectionId.portOut != portOut)
                return false;
            if (connectionId.portIn != portIn)
                return false;
            return true;
        }
    };

    int fLastGroupId;
    int fLastPortId;
    int fLastConnectionId;

    List<GroupNameToId>  fUsedGroupNames;
    List<PortNameToId>   fUsedPortNames;
    List<ConnectionToId> fUsedConnections;
    List<int>            fGroupIconsChanged;

    int getGroupId(const char* const name)
    {
        CARLA_ASSERT(name != nullptr);

        if (name == nullptr)
            return -1;

        for (List<GroupNameToId>::Itenerator it = fUsedGroupNames.begin(); it.valid(); it.next())
        {
            const GroupNameToId& groupNameId(*it);

            if (std::strcmp(groupNameId.name, name) == 0)
                return groupNameId.id;
        }

        return -1;
    }

    const char* getGroupName(const int groupId)
    {
        CARLA_ASSERT(groupId >= 0);

        static const char fallback[1] = { '\0' };

        if (groupId < 0)
            return fallback;

        for (List<GroupNameToId>::Itenerator it = fUsedGroupNames.begin(); it.valid(); it.next())
        {
            const GroupNameToId& groupNameId(*it);

            if (groupNameId.id == groupId)
                return groupNameId.name;
        }

        return fallback;
    }

    int getPortId(const char* const fullName)
    {
        CARLA_ASSERT(fullName != nullptr);

        if (fullName == nullptr)
            return -1;

        for (List<PortNameToId>::Itenerator it = fUsedPortNames.begin(); it.valid(); it.next())
        {
            const PortNameToId& portNameId(*it);

            if (std::strcmp(portNameId.fullName, fullName) == 0)
                return portNameId.portId;
        }

        return -1;
    }

    void getFullPortName(const int portId, char nameBuf[STR_MAX+1])
    {
        for (List<PortNameToId>::Itenerator it = fUsedPortNames.begin(); it.valid(); it.next())
        {
            const PortNameToId& portNameId(*it);

            if (portNameId.portId == portId)
            {
                std::strncpy(nameBuf, portNameId.fullName, STR_MAX);
                nameBuf[STR_MAX] = '\0';
                return;
            }
        }

        nameBuf[0] = '\0';
    }

    void initJackPatchbay(const char* const ourName)
    {
        CARLA_ASSERT(fLastGroupId == 0);
        CARLA_ASSERT(fLastPortId == 0);
        CARLA_ASSERT(fLastConnectionId == 0);
        CARLA_ASSERT(ourName != nullptr);

#ifdef USE_JUCE
        using namespace juce;

        // query initial jack ports
        StringArray parsedGroups;

        // our client
        if (ourName != nullptr)
        {
            parsedGroups.add(String(ourName));

            GroupNameToId groupNameToId(fLastGroupId++, ourName);
            fUsedGroupNames.append(groupNameToId);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, 0, groupNameToId.id, 0, 0.0f, ourName);
            // carla
        }

        if (const char** ports = jackbridge_get_ports(fClient, nullptr, nullptr, 0))
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

                CarlaString groupName(fullPortName);
                groupName.truncate(groupName.rfind(portName)-1);

                String qGroupName(groupName);

                if (parsedGroups.contains(qGroupName))
                {
                    groupId = getGroupId(groupName);
                    CARLA_ASSERT(groupId != -1);
                }
                else
                {
                    groupId = fLastGroupId++;
                    parsedGroups.add(qGroupName);

                    GroupNameToId groupNameToId(groupId, groupName);
                    fUsedGroupNames.append(groupNameToId);

                    void*  data = nullptr;
                    size_t dataSize = 0;

                    if (jackPortFlags & JackPortIsPhysical)
                    {
                        // "hardware"
                    }
                    else if (jackbridge_custom_get_data(fClient, groupName, URI_CANVAS_ICON, &data, &dataSize) && data != nullptr && dataSize != 0)
                    {
                        //const char* const icon((const char*)data);
                        //CARLA_ASSERT(std::strlen(icon)+1 == dataSize);
                        // icon
                    }

                    callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, 0, groupId, 0, 0.0f, groupName);
                }

                bool portIsInput = (jackPortFlags & JackPortIsInput);
                bool portIsAudio = (std::strcmp(jackbridge_port_type(jackPort), JACK_DEFAULT_AUDIO_TYPE) == 0);
                bool portIsCV    = (jackPortFlags & JackPortIsControlVoltage);

                unsigned int canvasPortFlags = 0x0;
                canvasPortFlags |= portIsInput ? PATCHBAY_PORT_IS_INPUT : PATCHBAY_PORT_IS_OUTPUT;
                canvasPortFlags |= portIsAudio ? PATCHBAY_PORT_IS_AUDIO : PATCHBAY_PORT_IS_MIDI;

                if (portIsAudio && portIsCV)
                    canvasPortFlags |= PATCHBAY_PORT_IS_CV;

                PortNameToId portNameToId(groupId, fLastPortId++, portName, fullPortName);
                fUsedPortNames.append(portNameToId);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, 0, groupId, portNameToId.portId, canvasPortFlags, portName);
            }

#if 0
            jackbridge_free(ports);
        }

        // query connections, after all ports are in place
        if (const char** ports = jackbridge_get_ports(fClient, nullptr, nullptr, JackPortIsOutput))
        {
#endif
            for (int i=0; ports[i] != nullptr; ++i)
            {
                jack_port_t* const jackPort(jackbridge_port_by_name(fClient, ports[i]));
                const char*  const fullPortName(ports[i]);

                CARLA_SAFE_ASSERT_CONTINUE(jackPort != nullptr);

#if 1
                const int jackPortFlags(jackbridge_port_flags(jackPort));

                if (jackPortFlags & JackPortIsInput)
                    continue;
#endif

                const int thisPortId(getPortId(fullPortName));

                if (thisPortId == -1)
                    continue;

                if (const char** connections = jackbridge_port_get_connections(jackPort))
                {
                    for (int j=0; connections[j] != nullptr; ++j)
                    {
                        const int targetPortId(getPortId(connections[j]));

                        ConnectionToId connectionToId(fLastConnectionId++, thisPortId, targetPortId);
                        fUsedConnections.append(connectionToId);

                        callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, connectionToId.id, thisPortId, targetPortId, nullptr);
                    }

                    jackbridge_free(connections);
                }
                else
                    carla_stderr("jack_port_get_connections failed for port %s", fullPortName);
            }

            jackbridge_free(ports);
        }
#endif
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
#if 0
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
#endif
        }
        else
        {
#if 0
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
#endif
        }
    }

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

    static void carla_jack_latency_callback(jack_latency_callback_mode_t mode, void* arg)
    {
        handlePtr->handleJackLatencyCallback(mode);
    }

#ifndef BUILD_BRIDGE
    static void carla_jack_custom_appearance_callback(const char* client_name, const char* key, jack_custom_change_t change, void* arg)
    {
        handlePtr->handleCustomAppearanceCallback(client_name, key, change);
    }

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

        if (plugin != nullptr && plugin->isEnabled() && plugin->tryLock())
        {
            CarlaEngineJack* const engine((CarlaEngineJack*)plugin->getEngine());
            CARLA_SAFE_ASSERT_RETURN(engine != nullptr,0);

            plugin->initBuffers();
            engine->saveTransportInfo();
            engine->processPlugin(plugin, nframes);
            plugin->unlock();
        }

        return 0;
    }

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
