/*
 * Carla JACK Engine
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifdef CARLA_ENGINE_JACK

#include "carla_engine_internal.hpp"
#include "carla_backend_utils.hpp"
#include "carla_midi.h"

#include "jackbridge/jackbridge.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine JACK-Audio port

class CarlaEngineJackAudioPort : public CarlaEngineAudioPort
{
public:
    CarlaEngineJackAudioPort(const bool isInput, const ProcessMode processMode, jack_client_t* const client, jack_port_t* const port)
        : CarlaEngineAudioPort(isInput, processMode),
          m_client(client),
          m_port(port)
    {
        qDebug("CarlaEngineJackAudioPort::CarlaEngineJackAudioPort(%s, %s, %p, %p)", bool2str(isInput), ProcessMode2Str(processMode), client, port);

        if (processMode == PROCESS_MODE_SINGLE_CLIENT || processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_ASSERT(m_client && m_port);
        }
        else
        {
            CARLA_ASSERT(! (m_client || m_port));
        }
    }

    ~CarlaEngineJackAudioPort()
    {
        qDebug("CarlaEngineJackAudioPort::~CarlaEngineJackAudioPort()");

        if (m_client && m_port)
            jackbridge_port_unregister(m_client, m_port);
    }

    void initBuffer(CarlaEngine* const engine)
    {
        CARLA_ASSERT(engine);

        if (! engine)
        {
            buffer = nullptr;
            return;
        }

        if (! m_port)
            return CarlaEngineAudioPort::initBuffer(engine);

        buffer = jackbridge_port_get_buffer(m_port, engine->getBufferSize());
    }

    float* getBuffer() const
    {
        return (float*)buffer;
    }

private:
    jack_client_t* const m_client;
    jack_port_t* const m_port;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackAudioPort)
};

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine JACK-Event port

class CarlaEngineJackEventPort : public CarlaEngineEventPort
{
public:
    CarlaEngineJackEventPort(const bool isInput, const ProcessMode processMode, jack_client_t* const client, jack_port_t* const port)
        : CarlaEngineEventPort(isInput, processMode),
          m_client(client),
          m_port(port)
    {
        qDebug("CarlaEngineJackEventPort::CarlaEngineJackEventPort(%s, %s, %p, %p)", bool2str(isInput), ProcessMode2Str(processMode), client, port);

        if (processMode == PROCESS_MODE_SINGLE_CLIENT || processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_ASSERT(m_client && m_port);
        }
        else
        {
            CARLA_ASSERT(! (m_client || m_port));
        }
    }

    ~CarlaEngineJackEventPort()
    {
        qDebug("CarlaEngineJackEventPort::~CarlaEngineJackEventPort()");

        if (m_client && m_port)
            jackbridge_port_unregister(m_client, m_port);
    }

    void initBuffer(CarlaEngine* const engine)
    {
        CARLA_ASSERT(engine);

        if (! engine)
        {
            buffer = nullptr;
            return;
        }

        if (! m_port)
            return CarlaEngineEventPort::initBuffer(engine);

        buffer = jackbridge_port_get_buffer(m_port, engine->getBufferSize());

        if (! isInput)
            jackbridge_midi_clear_buffer(buffer);
    }

    uint32_t getEventCount()
    {
        if (! m_port)
            return CarlaEngineEventPort::getEventCount();

        if (! isInput)
            return 0;

        CARLA_ASSERT(buffer);

        if (! buffer)
            return 0;

        return jackbridge_midi_get_event_count(buffer);
    }

    const EngineEvent* getEvent(const uint32_t index)
    {
        if (! m_port)
            return CarlaEngineEventPort::getEvent(index);

        if (! isInput)
            return nullptr;

        CARLA_ASSERT(buffer);

        if (! buffer)
            return nullptr;

        jack_midi_event_t jackEvent;

        if (jackbridge_midi_event_get(&jackEvent, buffer, index) != 0 || jackEvent.size > 3)
            return nullptr;

        m_retEvent.clear();

        const uint8_t midiStatus  = MIDI_GET_STATUS_FROM_DATA(jackEvent.buffer);
        const uint8_t midiChannel = MIDI_GET_CHANNEL_FROM_DATA(jackEvent.buffer);

        m_retEvent.time    = jackEvent.time;
        m_retEvent.channel = midiChannel;

        if (MIDI_IS_STATUS_CONTROL_CHANGE(midiStatus))
        {
            const uint8_t midiControl = jackEvent.buffer[1];
            m_retEvent.type           = EngineEventTypeControl;

            if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
            {
                const uint8_t midiBank = jackEvent.buffer[2];

                m_retEvent.ctrl.type      = EngineControlEventTypeMidiBank;
                m_retEvent.ctrl.parameter = midiBank;
                m_retEvent.ctrl.value     = 0.0;
            }
            else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
            {
                m_retEvent.ctrl.type      = EngineControlEventTypeAllSoundOff;
                m_retEvent.ctrl.parameter = 0;
                m_retEvent.ctrl.value     = 0.0;
            }
            else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
            {
                m_retEvent.ctrl.type      = EngineControlEventTypeAllNotesOff;
                m_retEvent.ctrl.parameter = 0;
                m_retEvent.ctrl.value     = 0.0;
            }
            else
            {
                const uint8_t midiValue = jackEvent.buffer[2];

                m_retEvent.ctrl.type      = EngineControlEventTypeParameter;
                m_retEvent.ctrl.parameter = midiControl;
                m_retEvent.ctrl.value     = double(midiValue)/127;
            }
        }
        else if (MIDI_IS_STATUS_PROGRAM_CHANGE(midiStatus))
        {
            const uint8_t midiProgram = jackEvent.buffer[1];
            m_retEvent.type           = EngineEventTypeControl;

            m_retEvent.ctrl.type      = EngineControlEventTypeMidiProgram;
            m_retEvent.ctrl.parameter = midiProgram;
            m_retEvent.ctrl.value     = 0.0;
        }
        else
        {
            m_retEvent.type = EngineEventTypeMidi;

            m_retEvent.midi.data[0] = midiStatus;
            m_retEvent.midi.data[1] = jackEvent.buffer[1];
            m_retEvent.midi.data[2] = jackEvent.buffer[2];
            m_retEvent.midi.size    = jackEvent.size;
        }

        return &m_retEvent;
    }

    void writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t parameter, const double value)
    {
        if (! m_port)
            return CarlaEngineEventPort::writeControlEvent(time, channel, type, parameter, value);

        if (isInput)
            return;

        CARLA_ASSERT(buffer);
        CARLA_ASSERT(type != EngineControlEventTypeNull);
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);

        if (! buffer)
            return;
        if (type == EngineControlEventTypeNull)
            return;
        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (type == EngineControlEventTypeParameter)
        {
            CARLA_ASSERT(! MIDI_IS_CONTROL_BANK_SELECT(parameter));
        }

        uint8_t data[3] = { 0 };
        uint8_t size    = 0;

        switch (type)
        {
        case EngineControlEventTypeNull:
            break;
        case EngineControlEventTypeParameter:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = parameter;
            data[2] = value * 127;
            size    = 3;
            break;
        case EngineControlEventTypeMidiBank:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_BANK_SELECT;
            data[2] = value;
            size = 3;
            break;
        case EngineControlEventTypeMidiProgram:
            data[0] = MIDI_STATUS_PROGRAM_CHANGE + channel;
            data[1] = value;
            size    = 2;
            break;
        case EngineControlEventTypeAllSoundOff:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
            size    = 2;
            break;
        case EngineControlEventTypeAllNotesOff:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
            size    = 2;
            break;
        }

        if (size > 0)
            jackbridge_midi_event_write(buffer, time, data, size);
    }

    void writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t* const data, const uint8_t size)
    {
        if (! m_port)
            return CarlaEngineEventPort::writeMidiEvent(time, channel, data, size);

        if (isInput)
            return;

        CARLA_ASSERT(buffer);
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(data);
        CARLA_ASSERT(size > 0);

        if (! buffer)
            return;
        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (! (data && size > 0))
            return;

        uint8_t jdata[size];
        std::memcpy(jdata, data, sizeof(uint8_t)*size);

        jdata[0] = data[0] + channel;

        jackbridge_midi_event_write(buffer, time, jdata, size);
    }

private:
    jack_client_t* const m_client;
    jack_port_t* const m_port;

    EngineEvent m_retEvent;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackEventPort)
};

// -------------------------------------------------------------------------------------------------------------------
// Jack Engine client

class CarlaEngineJackClient : public CarlaEngineClient
{
public:
    CarlaEngineJackClient(const EngineType engineType, const ProcessMode processMode, jack_client_t* const client)
        : CarlaEngineClient(engineType, processMode),
          m_client(client),
          m_usesClient(processMode == PROCESS_MODE_SINGLE_CLIENT || processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
    {
        qDebug("CarlaEngineJackClient::CarlaEngineJackClient(%s, %s, %p)", EngineType2Str(engineType), ProcessMode2Str(processMode), client);

        if (m_usesClient)
            CARLA_ASSERT(m_client);
        else
            CARLA_ASSERT(! m_client);
    }

    ~CarlaEngineJackClient()
    {
        qDebug("CarlaEngineClient::~CarlaEngineClient()");

        if (processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            if (m_client)
                jackbridge_client_close(m_client);
        }
    }

    void activate()
    {
        qDebug("CarlaEngineClient::activate()");

        if (processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_ASSERT(m_client && ! isActive());

            if (m_client && ! isActive())
                jackbridge_activate(m_client);
        }

        CarlaEngineClient::activate();
    }

    void deactivate()
    {
        qDebug("CarlaEngineClient::deactivate()");

        if (processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CARLA_ASSERT(m_client && isActive());

            if (m_client && isActive())
                jackbridge_deactivate(m_client);
        }

        CarlaEngineClient::deactivate();
    }

    bool isOk() const
    {
        qDebug("CarlaEngineClient::isOk()");

        if (m_usesClient)
            return bool(m_client);

        return CarlaEngineClient::isOk();
    }

    void setLatency(const uint32_t samples)
    {
        CarlaEngineClient::setLatency(samples);

        if (m_usesClient)
            jackbridge_recompute_total_latencies(m_client);
    }

    const CarlaEnginePort* addPort(const EnginePortType portType, const char* const name, const bool isInput)
    {
        qDebug("CarlaJackEngineClient::addPort(%s, \"%s\", %s)", EnginePortType2Str(portType), name, bool2str(isInput));

        jack_port_t* port = nullptr;

        // Create Jack port if needed
        if (m_usesClient)
        {
            switch (portType)
            {
            case EnginePortTypeNull:
                break;
            case EnginePortTypeAudio:
                port = jackbridge_port_register(m_client, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            case EnginePortTypeEvent:
                port = jackbridge_port_register(m_client, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            }
        }

        // Create Engine port
        switch (portType)
        {
        case EnginePortTypeNull:
            break;
        case EnginePortTypeAudio:
            return new CarlaEngineJackAudioPort(isInput, processMode, m_client, port);
        case EnginePortTypeEvent:
            return new CarlaEngineJackEventPort(isInput, processMode, m_client, port);
        }

        qCritical("CarlaJackEngineClient::addPort(%s, \"%s\", %s) - invalid type", EnginePortType2Str(portType), name, bool2str(isInput));
        return nullptr;
    }

private:
    jack_client_t* const m_client;
    const bool m_usesClient;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJackClient)
};

// -------------------------------------------------------------------------------------------------------------------
// Jack Engine

class CarlaEngineJack : public CarlaEngine
{
public:
    CarlaEngineJack()
        : CarlaEngine()
#ifndef BUILD_BRIDGE
        , m_rackPorts{nullptr}
#endif
    {
        qDebug("CarlaEngineJack::CarlaEngineJack()");

        m_client    = nullptr;
        m_state     = JackTransportStopped;
        m_freewheel = false;

        memset(&m_pos, 0, sizeof(jack_position_t));

#ifdef BUILD_BRIDGE
        m_hasQuit = false;
#endif
    }

    ~CarlaEngineJack()
    {
        qDebug("CarlaEngineJack::~CarlaEngineJack()");
        CARLA_ASSERT(! m_client);
    }

    // -------------------------------------------------------------------
    // Maximum values

    int maxClientNameSize()
    {
#ifndef BUILD_BRIDGE
        if (options.processMode == PROCESS_MODE_SINGLE_CLIENT || options.processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
            return jackbridge_client_name_size() - 3; // reserve space for "_2" forced-stereo ports

        return CarlaEngine::maxClientNameSize();
    }

    int maxPortNameSize()
    {
#ifndef BUILD_BRIDGE
        if (options.processMode == PROCESS_MODE_SINGLE_CLIENT || options.processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
            return jackbridge_port_name_size();

        return CarlaEngine::maxPortNameSize();
    }

    // -------------------------------------------------------------------
    // Virtual, per-engine type calls

    bool init(const char* const clientName)
    {
        qDebug("CarlaEngineJack::init(\"%s\")", clientName);

        m_state     = JackTransportStopped;
        m_freewheel = false;

#ifndef BUILD_BRIDGE
        m_client = jackbridge_client_open(clientName, JackNullOption, nullptr);

        if (m_client)
        {
            bufferSize = jackbridge_get_buffer_size(m_client);
            sampleRate = jackbridge_get_sample_rate(m_client);

            jackbridge_set_buffer_size_callback(m_client, carla_jack_bufsize_callback, this);
            jackbridge_set_sample_rate_callback(m_client, carla_jack_srate_callback, this);
            jackbridge_set_freewheel_callback(m_client, carla_jack_freewheel_callback, this);
            jackbridge_set_process_callback(m_client, carla_jack_process_callback, this);
            jackbridge_set_latency_callback(m_client, carla_jack_latency_callback, this);
            jackbridge_on_shutdown(m_client, carla_jack_shutdown_callback, this);

            if (options.processMode == PROCESS_MODE_CONTINUOUS_RACK)
            {
                m_rackPorts[rackPortAudioIn1]  = jackbridge_port_register(m_client, "audio-in1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                m_rackPorts[rackPortAudioIn2]  = jackbridge_port_register(m_client, "audio-in2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                m_rackPorts[rackPortAudioOut1] = jackbridge_port_register(m_client, "audio-out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                m_rackPorts[rackPortAudioOut2] = jackbridge_port_register(m_client, "audio-out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                m_rackPorts[rackPortEventIn]   = jackbridge_port_register(m_client, "events-in",  JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                m_rackPorts[rackPortEventOut]  = jackbridge_port_register(m_client, "events-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
            }

            if (jackbridge_activate(m_client) == 0)
            {
                name = jackbridge_get_client_name(m_client);
                name.toBasic();

                CarlaEngine::init(name);
                return true;
            }
            else
            {
                setLastError("Failed to activate the JACK client");
                m_client = nullptr;
            }
        }
        else
            setLastError("Failed to create new JACK client");

        return false;
#else
        // open temp client to get initial buffer-size and sample-rate values
        if (bufferSize == 0 || sampleRate == 0.0)
        {
            m_client = jackbridge_client_open(clientName, JackNullOption, nullptr);

            if (m_client)
            {
                bufferSize = jackbridge_get_buffer_size(m_client);
                sampleRate = jackbridge_get_sample_rate(m_client);

                jackbridge_client_close(m_client);
                m_client = nullptr;
            }
        }

        name = clientName;
        name.toBasic();

        CarlaEngine::init(name);
        return true;
#endif
    }

    bool close()
    {
        qDebug("CarlaEngineJack::close()");
        CarlaEngine::close();

#ifdef BUILD_BRIDGE
        m_client  = nullptr;
        m_hasQuit = true;
        return true;
#else
        if (jackbridge_deactivate(m_client) == 0)
        {
            if (options.processMode == PROCESS_MODE_CONTINUOUS_RACK)
            {
                jackbridge_port_unregister(m_client, m_rackPorts[rackPortAudioIn1]);
                jackbridge_port_unregister(m_client, m_rackPorts[rackPortAudioIn2]);
                jackbridge_port_unregister(m_client, m_rackPorts[rackPortAudioOut1]);
                jackbridge_port_unregister(m_client, m_rackPorts[rackPortAudioOut2]);
                jackbridge_port_unregister(m_client, m_rackPorts[rackPortEventIn]);
                jackbridge_port_unregister(m_client, m_rackPorts[rackPortEventOut]);
            }

            if (jackbridge_client_close(m_client) == 0)
            {
                m_client = nullptr;
                return true;
            }
            else
                setLastError("Failed to close the JACK client");
        }
        else
            setLastError("Failed to deactivate the JACK client");

        m_client = nullptr;
#endif
        return false;
    }

    bool isRunning() const
    {
#ifdef BUILD_BRIDGE
        return bool(m_client || ! hasQuit);
#else
        return bool(m_client);
#endif
    }

    bool isOffline() const
    {
        return m_freewheel;
    }

    EngineType type() const
    {
        return EngineTypeJack;
    }

    CarlaEngineClient* addClient(CarlaPlugin* const plugin)
    {
        jack_client_t* client = nullptr;

#ifdef BUILD_BRIDGE
        client = m_client = jackbridge_client_open(plugin->name(), JackNullOption, nullptr);

        bufferSize = jackbridge_get_buffer_size(client);
        sampleRate = jackbridge_get_sample_rate(client);

        jackbridge_set_buffer_size_callback(client, carla_jack_bufsize_callback, this);
        jackbridge_set_sample_rate_callback(client, carla_jack_srate_callback, this);
        jackbridge_set_freewheel_callback(client, carla_jack_freewheel_callback, this);
        jackbridge_set_process_callback(client, carla_jack_process_callback, this);
        jackbridge_set_latency_callback(client, carla_jack_latency_callback, this);
        jackbridge_on_shutdown(client, carla_jack_shutdown_callback, this);
#else
        if (options.processMode == PROCESS_MODE_SINGLE_CLIENT)
        {
            client = m_client;
        }
        else if (options.processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            client = jackbridge_client_open(plugin->name(), JackNullOption, nullptr);
            jackbridge_set_process_callback(client, carla_jack_process_callback_plugin, plugin);
            jackbridge_set_latency_callback(client, carla_jack_latency_callback_plugin, plugin);
        }
#endif

#ifdef BUILD_BRIDGE
        return new CarlaEngineJackClient(EngineTypeJack, PROCESS_MODE_MULTIPLE_CLIENTS, client);
#else
        return new CarlaEngineJackClient(EngineTypeJack, options.processMode, client);
#endif
    }

    // -------------------------------------

protected:
    void handleJackBufferSizeCallback(const uint32_t newBufferSize)
    {
        bufferSize = newBufferSize;

#ifndef BUILD_BRIDGE
        if (options.processHighPrecision)
            return;
#endif

        bufferSizeChanged(newBufferSize);
    }

    void handleJackSampleRateCallback(const double newSampleRate)
    {
        sampleRate = newSampleRate;
    }

    void handleJackFreewheelCallback(const bool isFreewheel)
    {
        m_freewheel = isFreewheel;
    }

    void handleJackProcessCallback(const uint32_t nframes)
    {
#ifndef BUILD_BRIDGE
        if (maxPluginNumber() == 0)
            return;
#endif

        m_pos.unique_1 = m_pos.unique_2 + 1; // invalidate
        m_state = jackbridge_transport_query(m_client, &m_pos);

        timeInfo.playing = (m_state != JackTransportStopped);

        if (m_pos.unique_1 == m_pos.unique_2)
        {
            timeInfo.frame = m_pos.frame;
            timeInfo.time  = m_pos.usecs;

            if (m_pos.valid & JackPositionBBT)
            {
                timeInfo.valid    = timeInfo.ValidBBT;
                timeInfo.bbt.bar  = m_pos.bar;
                timeInfo.bbt.beat = m_pos.beat;
                timeInfo.bbt.tick = m_pos.tick;
                timeInfo.bbt.barStartTick   = m_pos.bar_start_tick;
                timeInfo.bbt.beatsPerBar    = m_pos.beats_per_bar;
                timeInfo.bbt.beatType       = m_pos.beat_type;
                timeInfo.bbt.ticksPerBeat   = m_pos.ticks_per_beat;
                timeInfo.bbt.beatsPerMinute = m_pos.beats_per_minute;
            }
            else
                timeInfo.valid = 0;
        }
        else
        {
            timeInfo.frame = 0;
            timeInfo.valid = 0;
        }

#if 0
#ifndef BUILD_BRIDGE
        if (options.processMode == PROCESS_MODE_SINGLE_CLIENT)
        {
            for (unsigned short i=0, max=maxPluginNumber(); i < max; i++)
            {
                CarlaPlugin* const plugin = getPluginUnchecked(i);
#else
                CarlaPlugin* const plugin = getPluginUnchecked(0);
#endif

                if (plugin && plugin->enabled())
                {
                    processLock();
                    plugin->initBuffers();
                    processPlugin(plugin, nframes);
                    processUnlock();
                }
#ifndef BUILD_BRIDGE
            }
        }
        else if (options.processMode == PROCESS_MODE_CONTINUOUS_RACK)
        {
            // get buffers from jack
            float* audioIn1  = (float*)jackbridge_port_get_buffer(m_rackPorts[rackPortAudioIn1], nframes);
            float* audioIn2  = (float*)jackbridge_port_get_buffer(m_rackPorts[rackPortAudioIn2], nframes);
            float* audioOut1 = (float*)jackbridge_port_get_buffer(m_rackPorts[rackPortAudioOut1], nframes);
            float* audioOut2 = (float*)jackbridge_port_get_buffer(m_rackPorts[rackPortAudioOut2], nframes);
            void* controlIn  = jackbridge_port_get_buffer(m_rackPorts[rackPortControlIn],  nframes);
            void* controlOut = jackbridge_port_get_buffer(m_rackPorts[rackPortControlOut], nframes);
            void* midiIn     = jackbridge_port_get_buffer(m_rackPorts[rackPortMidiIn],  nframes);
            void* midiOut    = jackbridge_port_get_buffer(m_rackPorts[rackPortMidiOut], nframes);

            // assert buffers
            CARLA_ASSERT(audioIn1);
            CARLA_ASSERT(audioIn2);
            CARLA_ASSERT(audioOut1);
            CARLA_ASSERT(audioOut2);
            CARLA_ASSERT(controlIn);
            CARLA_ASSERT(controlOut);
            CARLA_ASSERT(midiIn);
            CARLA_ASSERT(midiOut);

            // create audio buffers
            float* inBuf[2]  = { audioIn1, audioIn2 };
            float* outBuf[2] = { audioOut1, audioOut2 };

            // initialize control input
            memset(rackControlEventsIn, 0, sizeof(CarlaEngineControlEvent)*MAX_CONTROL_EVENTS);
            {
                jack_midi_event_t jackEvent;
                const uint32_t jackEventCount = jackbridge_midi_get_event_count(controlIn);

                uint32_t carlaEventIndex = 0;

                for (uint32_t jackEventIndex=0; jackEventIndex < jackEventCount; jackEventIndex++)
                {
                    if (jackbridge_midi_event_get(&jackEvent, controlIn, jackEventIndex) != 0)
                        continue;

                    CarlaEngineControlEvent* const carlaEvent = &rackControlEventsIn[carlaEventIndex++];

                    const uint8_t midiStatus  = jackEvent.buffer[0];
                    const uint8_t midiChannel = midiStatus & 0x0F;

                    carlaEvent->time    = jackEvent.time;
                    carlaEvent->channel = midiChannel;

                    if (MIDI_IS_STATUS_CONTROL_CHANGE(midiStatus))
                    {
                        const uint8_t midiControl = jackEvent.buffer[1];

                        if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
                        {
                            const uint8_t midiBank = jackEvent.buffer[2];
                            carlaEvent->type  = CarlaEngineMidiBankChangeEvent;
                            carlaEvent->value = midiBank;
                        }
                        else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
                        {
                            carlaEvent->type = CarlaEngineAllSoundOffEvent;
                        }
                        else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
                        {
                            carlaEvent->type = CarlaEngineAllNotesOffEvent;
                        }
                        else
                        {
                            const uint8_t midiValue = jackEvent.buffer[2];
                            carlaEvent->type      = CarlaEngineParameterChangeEvent;
                            carlaEvent->parameter = midiControl;
                            carlaEvent->value     = double(midiValue)/127;
                        }
                    }
                    else if (MIDI_IS_STATUS_PROGRAM_CHANGE(midiStatus))
                    {
                        const uint8_t midiProgram = jackEvent.buffer[1];
                        carlaEvent->type  = CarlaEngineMidiProgramChangeEvent;
                        carlaEvent->value = midiProgram;
                    }
                }
            }

            // initialize midi input
            memset(rackMidiEventsIn, 0, sizeof(CarlaEngineMidiEvent)*MAX_MIDI_EVENTS);
            {
                uint32_t i = 0, j = 0;
                jack_midi_event_t jackEvent;

                while (jackbridge_midi_event_get(&jackEvent, midiIn, j++) == 0)
                {
                    if (i == MAX_MIDI_EVENTS)
                        break;

                    if (jackEvent.size < 4)
                    {
                        rackMidiEventsIn[i].time = jackEvent.time;
                        rackMidiEventsIn[i].size = jackEvent.size;
                        memcpy(rackMidiEventsIn[i].data, jackEvent.buffer, jackEvent.size);
                        i += 1;
                    }
                }
            }

            // process rack
            processRack(inBuf, outBuf, nframes);

            // output control
            {
                jackbridge_midi_clear_buffer(controlOut);

                for (unsigned short i=0; i < MAX_CONTROL_EVENTS; i++)
                {
                    CarlaEngineControlEvent* const event = &rackControlEventsOut[i];

                    if (event->type == CarlaEngineParameterChangeEvent && MIDI_IS_CONTROL_BANK_SELECT(event->parameter))
                        event->type = CarlaEngineMidiBankChangeEvent;

                    uint8_t data[4] = { 0 };

                    switch (event->type)
                    {
                    case CarlaEngineNullEvent:
                        break;
                    case CarlaEngineParameterChangeEvent:
                        data[0] = MIDI_STATUS_CONTROL_CHANGE + event->channel;
                        data[1] = event->parameter;
                        data[2] = event->value * 127;
                        jackbridge_midi_event_write(controlOut, event->time, data, 3);
                        break;
                    case CarlaEngineMidiBankChangeEvent:
                        data[0] = MIDI_STATUS_CONTROL_CHANGE + event->channel;
                        data[1] = MIDI_CONTROL_BANK_SELECT;
                        data[2] = event->value;
                        jackbridge_midi_event_write(controlOut, event->time, data, 3);
                        break;
                    case CarlaEngineMidiProgramChangeEvent:
                        data[0] = MIDI_STATUS_PROGRAM_CHANGE + event->channel;
                        data[1] = event->value;
                        jackbridge_midi_event_write(controlOut, event->time, data, 2);
                        break;
                    case CarlaEngineAllSoundOffEvent:
                        data[0] = MIDI_STATUS_CONTROL_CHANGE + event->channel;
                        data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                        jackbridge_midi_event_write(controlOut, event->time, data, 2);
                        break;
                    case CarlaEngineAllNotesOffEvent:
                        data[0] = MIDI_STATUS_CONTROL_CHANGE + event->channel;
                        data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                        jackbridge_midi_event_write(controlOut, event->time, data, 2);
                        break;
                    }
                }
            }

            // output midi
            {
                jackbridge_midi_clear_buffer(midiOut);

                for (unsigned short i=0; i < MAX_MIDI_EVENTS; i++)
                {
                    if (rackMidiEventsOut[i].size == 0)
                        break;

                    jackbridge_midi_event_write(midiOut, rackMidiEventsOut[i].time, rackMidiEventsOut[i].data, rackMidiEventsOut[i].size);
                }
            }
        }
#endif
#endif
    }

    void handleJackLatencyCallback(const jack_latency_callback_mode_t mode)
    {
#ifndef BUILD_BRIDGE
        if (options.processMode != PROCESS_MODE_SINGLE_CLIENT)
            return;
#endif

#if 0
        for (unsigned short i=0, max=maxPluginNumber(); i < max; i++)
        {
            CarlaPlugin* const plugin = getPluginUnchecked(i);

            if (plugin && plugin->enabled())
                latencyPlugin(plugin, mode);
        }
#endif
    }

    void handleJackShutdownCallback()
    {
        for (unsigned short i=0, max=maxPluginNumber(); i < max; i++)
        {
            //CarlaPlugin* const plugin = getPluginUnchecked(i);

            //if (plugin)
            //    plugin->x_client = nullptr;
        }

        m_client = nullptr;
        callback(CALLBACK_QUIT, 0, 0, 0, 0.0, nullptr);
    }

    // -------------------------------------

private:
    jack_client_t* m_client;
    jack_transport_state_t m_state;
    jack_position_t m_pos;
    bool m_freewheel;

    // -------------------------------------

#ifdef BUILD_BRIDGE
    bool m_hasQuit;
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

    jack_port_t* m_rackPorts[rackPortCount];
#endif

    // -------------------------------------

    static void processPlugin(CarlaPlugin* const p, const uint32_t nframes)
    {
#if 0
        float* inBuffer[p->aIn.count];
        float* outBuffer[p->aOut.count];

        for (uint32_t i=0; i < p->aIn.count; i++)
            inBuffer[i] = ((CarlaEngineJackAudioPort*)p->aIn.ports[i])->getBuffer();

        for (uint32_t i=0; i < p->aOut.count; i++)
            outBuffer[i] = ((CarlaEngineJackAudioPort*)p->aOut.ports[i])->getBuffer();

        if (p->m_processHighPrecision)
        {
            float* inBuffer2[p->aIn.count];
            float* outBuffer2[p->aOut.count];

            for (uint32_t i=0, j; i < nframes; i += 8)
            {
                for (j=0; j < p->aIn.count; j++)
                    inBuffer2[j] = inBuffer[j] + i;

                for (j=0; j < p->aOut.count; j++)
                    outBuffer2[j] = outBuffer[j] + i;

                p->process(inBuffer2, outBuffer2, 8, i);
            }
        }
        else
            p->process(inBuffer, outBuffer, nframes);
#endif
    }

    static void latencyPlugin(CarlaPlugin* const p, jack_latency_callback_mode_t mode)
    {
#if 0
        jack_latency_range_t range;
        uint32_t pluginLatency = p->x_client->getLatency();

        if (pluginLatency == 0)
            return;

        if (mode == JackCaptureLatency)
        {
            for (uint32_t i=0; i < p->aIn.count; i++)
            {
                uint aOutI = (i >= p->aOut.count) ? p->aOut.count : i;
                jack_port_t* const portIn  = ((CarlaEngineJackAudioPort*)p->aIn.ports[i])->m_port;
                jack_port_t* const portOut = ((CarlaEngineJackAudioPort*)p->aOut.ports[aOutI])->m_port;

                jackbridge_port_get_latency_range(portIn, mode, &range);
                range.min += pluginLatency;
                range.max += pluginLatency;
                jackbridge_port_set_latency_range(portOut, mode, &range);
            }
        }
        else
        {
            for (uint32_t i=0; i < p->aOut.count; i++)
            {
                uint aInI = (i >= p->aIn.count) ? p->aIn.count : i;
                jack_port_t* const portIn  = ((CarlaEngineJackAudioPort*)p->aIn.ports[aInI])->m_port;
                jack_port_t* const portOut = ((CarlaEngineJackAudioPort*)p->aOut.ports[i])->m_port;

                jackbridge_port_get_latency_range(portOut, mode, &range);
                range.min += pluginLatency;
                range.max += pluginLatency;
                jackbridge_port_set_latency_range(portIn, mode, &range);
            }
        }
#endif
    }

    // -------------------------------------

    static int carla_jack_srate_callback(jack_nframes_t newSampleRate, void* arg)
    {
        if (CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg)
            _this_->handleJackSampleRateCallback(newSampleRate);
        return 0;
    }

    static int carla_jack_bufsize_callback(jack_nframes_t newBufferSize, void* arg)
    {
        if (CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg)
            _this_->handleJackBufferSizeCallback(newBufferSize);
        return 0;
    }

    static void carla_jack_freewheel_callback(int starting, void* arg)
    {
        if (CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg)
            _this_->handleJackFreewheelCallback(bool(starting));
    }

    static int carla_jack_process_callback(jack_nframes_t nframes, void* arg)
    {
        if (CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg)
            _this_->handleJackProcessCallback(nframes);
        return 0;
    }

    static void carla_jack_latency_callback(jack_latency_callback_mode_t mode, void* arg)
    {
        if (CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg)
            _this_->handleJackLatencyCallback(mode);
    }

    static void carla_jack_shutdown_callback(void* arg)
    {
        if (CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg)
            _this_->handleJackShutdownCallback();
    }

    // -------------------------------------

    static int carla_jack_process_callback_plugin(jack_nframes_t nframes, void* arg)
    {
        CarlaPlugin* const plugin = (CarlaPlugin*)arg;

        if (plugin && plugin->enabled())
        {
            plugin->engineProcessLock();
            plugin->initBuffers();
            processPlugin(plugin, nframes);
            plugin->engineProcessUnlock();
        }

        return 0;
    }

    static void carla_jack_latency_callback_plugin(jack_latency_callback_mode_t mode, void* arg)
    {
        CarlaPlugin* const plugin = (CarlaPlugin*)arg;

        if (plugin && plugin->enabled())
            latencyPlugin(plugin, mode);
    }

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
