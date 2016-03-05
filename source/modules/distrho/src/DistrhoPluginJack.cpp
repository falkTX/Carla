/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#include "DistrhoPluginInternal.hpp"

#if DISTRHO_PLUGIN_HAS_UI && ! DISTRHO_PLUGIN_HAS_EMBED_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIInternal.hpp"
#else
# include "../extra/Sleep.hpp"
#endif

#include "jack/jack.h"
#include "jack/midiport.h"
#include "jack/transport.h"

#ifndef DISTRHO_OS_WINDOWS
# include <signal.h>
#endif

// -----------------------------------------------------------------------

START_NAMESPACE_DISTRHO

#if DISTRHO_PLUGIN_HAS_UI && ! DISTRHO_PLUGIN_WANT_STATE
static const setStateFunc setStateCallback = nullptr;
#endif

// -----------------------------------------------------------------------

static volatile bool gCloseSignalReceived = false;

#ifdef DISTRHO_OS_WINDOWS
static BOOL WINAPI winSignalHandler(DWORD dwCtrlType) noexcept
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        gCloseSignalReceived = true;
        return TRUE;
    }
    return FALSE;
}

static void initSignalHandler()
{
    SetConsoleCtrlHandler(winSignalHandler, TRUE);
}
#else
static void closeSignalHandler(int) noexcept
{
    gCloseSignalReceived = true;
}

static void initSignalHandler()
{
    struct sigaction sint;
    struct sigaction sterm;

    sint.sa_handler  = closeSignalHandler;
    sint.sa_flags    = SA_RESTART;
    sint.sa_restorer = nullptr;
    sigemptyset(&sint.sa_mask);
    sigaction(SIGINT, &sint, nullptr);

    sterm.sa_handler  = closeSignalHandler;
    sterm.sa_flags    = SA_RESTART;
    sterm.sa_restorer = nullptr;
    sigemptyset(&sterm.sa_mask);
    sigaction(SIGTERM, &sterm, nullptr);
}
#endif

// -----------------------------------------------------------------------

#if DISTRHO_PLUGIN_HAS_UI
class PluginJack : public IdleCallback
#else
class PluginJack
#endif
{
public:
    PluginJack(jack_client_t* const client)
        : fPlugin(),
#if DISTRHO_PLUGIN_HAS_UI
          fUI(this, 0, nullptr, setParameterValueCallback, setStateCallback, nullptr, setSizeCallback, fPlugin.getInstancePointer()),
#endif
          fClient(client)
    {
        char strBuf[0xff+1];
        strBuf[0xff] = '\0';

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
        {
            std::snprintf(strBuf, 0xff, "in%i", i+1);
            fPortAudioIns[i] = jack_port_register(fClient, strBuf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        }
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
        {
            std::snprintf(strBuf, 0xff, "out%i", i+1);
            fPortAudioOuts[i] = jack_port_register(fClient, strBuf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        }
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
        fPortMidiIn = jack_port_register(fClient, "midi-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
#endif

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (fPlugin.getProgramCount() > 0)
        {
            fPlugin.loadProgram(0);
# if DISTRHO_PLUGIN_HAS_UI
            fUI.programLoaded(0);
# endif
        }
#endif

        if (const uint32_t count = fPlugin.getParameterCount())
        {
            fLastOutputValues = new float[count];

            for (uint32_t i=0; i < count; ++i)
            {
                if (fPlugin.isParameterOutput(i))
                {
                    fLastOutputValues[i] = fPlugin.getParameterValue(i);
                }
                else
                {
                    fLastOutputValues[i] = 0.0f;
# if DISTRHO_PLUGIN_HAS_UI
                    fUI.parameterChanged(i, fPlugin.getParameterValue(i));
# endif
                }
            }
        }
        else
        {
            fLastOutputValues = nullptr;
        }

        jack_set_buffer_size_callback(fClient, jackBufferSizeCallback, this);
        jack_set_sample_rate_callback(fClient, jackSampleRateCallback, this);
        jack_set_process_callback(fClient, jackProcessCallback, this);
        jack_on_shutdown(fClient, jackShutdownCallback, this);

        fPlugin.activate();

        jack_activate(fClient);

#if DISTRHO_PLUGIN_HAS_UI
        if (const char* const name = jack_get_client_name(fClient))
            fUI.setWindowTitle(name);
        else
            fUI.setWindowTitle(fPlugin.getName());

        fUI.exec(this);
#else
        while (! gCloseSignalReceived)
            d_sleep(1);
#endif
    }

    ~PluginJack()
    {
        if (fClient != nullptr)
            jack_deactivate(fClient);

        if (fLastOutputValues != nullptr)
        {
            delete[] fLastOutputValues;
            fLastOutputValues = nullptr;
        }

        fPlugin.deactivate();

        if (fClient == nullptr)
            return;

#if DISTRHO_PLUGIN_IS_SYNTH
        jack_port_unregister(fClient, fPortMidiIn);
        fPortMidiIn = nullptr;
#endif

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
        {
            jack_port_unregister(fClient, fPortAudioIns[i]);
            fPortAudioIns[i] = nullptr;
        }
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
        {
            jack_port_unregister(fClient, fPortAudioOuts[i]);
            fPortAudioOuts[i] = nullptr;
        }
#endif

        jack_client_close(fClient);
    }

    // -------------------------------------------------------------------

protected:
#if DISTRHO_PLUGIN_HAS_UI
    void idleCallback() override
    {
        if (gCloseSignalReceived)
            return fUI.quit();

        float value;

        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (! fPlugin.isParameterOutput(i))
                continue;

            value = fPlugin.getParameterValue(i);

            if (fLastOutputValues[i] == value)
                continue;

            fLastOutputValues[i] = value;
            fUI.parameterChanged(i, value);
        }

        fUI.exec_idle();
    }
#endif

    void jackBufferSize(const jack_nframes_t nframes)
    {
        fPlugin.setBufferSize(nframes, true);
    }

    void jackSampleRate(const jack_nframes_t nframes)
    {
        fPlugin.setSampleRate(nframes, true);
    }

    void jackProcess(const jack_nframes_t nframes)
    {
#if DISTRHO_PLUGIN_NUM_INPUTS > 0
        const float* audioIns[DISTRHO_PLUGIN_NUM_INPUTS];

        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
            audioIns[i] = (const float*)jack_port_get_buffer(fPortAudioIns[i], nframes);
#else
        static const float** audioIns = nullptr;
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        float* audioOuts[DISTRHO_PLUGIN_NUM_OUTPUTS];

        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            audioOuts[i] = (float*)jack_port_get_buffer(fPortAudioOuts[i], nframes);
#else
        static float** audioOuts = nullptr;
#endif

#if DISTRHO_PLUGIN_WANT_TIMEPOS
        jack_position_t pos;
        fTimePosition.playing = (jack_transport_query(fClient, &pos) == JackTransportRolling);

        if (pos.unique_1 == pos.unique_2)
        {
            fTimePosition.frame = pos.frame;

            if (pos.valid & JackTransportBBT)
            {
                fTimePosition.bbt.valid = true;

                fTimePosition.bbt.bar  = pos.bar;
                fTimePosition.bbt.beat = pos.beat;
                fTimePosition.bbt.tick = pos.tick;
                fTimePosition.bbt.barStartTick = pos.bar_start_tick;

                fTimePosition.bbt.beatsPerBar = pos.beats_per_bar;
                fTimePosition.bbt.beatType    = pos.beat_type;

                fTimePosition.bbt.ticksPerBeat   = pos.ticks_per_beat;
                fTimePosition.bbt.beatsPerMinute = pos.beats_per_minute;
            }
            else
                fTimePosition.bbt.valid = false;
        }
        else
        {
            fTimePosition.bbt.valid = false;
            fTimePosition.frame = 0;
        }

        fPlugin.setTimePosition(fTimePosition);
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
        void* const midiBuf = jack_port_get_buffer(fPortMidiIn, nframes);

        if (const uint32_t eventCount = jack_midi_get_event_count(midiBuf))
        {
            uint32_t  midiEventCount = 0;
            MidiEvent midiEvents[eventCount];

            jack_midi_event_t jevent;

            for (uint32_t i=0; i < eventCount; ++i)
            {
                if (jack_midi_event_get(&jevent, midiBuf, i) != 0)
                    break;

                MidiEvent& midiEvent(midiEvents[midiEventCount++]);

                midiEvent.frame = jevent.time;
                midiEvent.size  = jevent.size;

                if (midiEvent.size > MidiEvent::kDataSize)
                    midiEvent.dataExt = jevent.buffer;
                else
                    std::memcpy(midiEvent.data, jevent.buffer, midiEvent.size);
            }

            fPlugin.run(audioIns, audioOuts, nframes, midiEvents, midiEventCount);
        }
        else
        {
            fPlugin.run(audioIns, audioOuts, nframes, nullptr, 0);
        }
#else
        fPlugin.run(audioIns, audioOuts, nframes);
#endif
    }

    void jackShutdown()
    {
        d_stderr("jack has shutdown, quitting now...");
        fClient = nullptr;
#if DISTRHO_PLUGIN_HAS_UI
        fUI.quit();
#endif
    }

    // -------------------------------------------------------------------

    void setParameterValue(const uint32_t index, const float value)
    {
        fPlugin.setParameterValue(index, value);
    }

#if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* const key, const char* const value)
    {
        fPlugin.setState(key, value);
    }
#endif

#if DISTRHO_PLUGIN_HAS_UI
    void setSize(const uint width, const uint height)
    {
        fUI.setWindowSize(width, height);
    }
#endif

    // -------------------------------------------------------------------

private:
    PluginExporter fPlugin;
#if DISTRHO_PLUGIN_HAS_UI
    UIExporter     fUI;
#endif

    jack_client_t* fClient;

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
    jack_port_t* fPortAudioIns[DISTRHO_PLUGIN_NUM_INPUTS];
#endif
#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    jack_port_t* fPortAudioOuts[DISTRHO_PLUGIN_NUM_OUTPUTS];
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
    jack_port_t* fPortMidiIn;
#endif
#if DISTRHO_PLUGIN_WANT_TIMEPOS
    TimePosition fTimePosition;
#endif

    // Temporary data
    float* fLastOutputValues;

    // -------------------------------------------------------------------
    // Callbacks

    #define uiPtr ((PluginJack*)ptr)

    static int jackBufferSizeCallback(jack_nframes_t nframes, void* ptr)
    {
        uiPtr->jackBufferSize(nframes);
        return 0;
    }

    static int jackSampleRateCallback(jack_nframes_t nframes, void* ptr)
    {
        uiPtr->jackSampleRate(nframes);
        return 0;
    }

    static int jackProcessCallback(jack_nframes_t nframes, void* ptr)
    {
        uiPtr->jackProcess(nframes);
        return 0;
    }

    static void jackShutdownCallback(void* ptr)
    {
        uiPtr->jackShutdown();
    }

    static void setParameterValueCallback(void* ptr, uint32_t index, float value)
    {
        uiPtr->setParameterValue(index, value);
    }

#if DISTRHO_PLUGIN_WANT_STATE
    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        uiPtr->setState(key, value);
    }
#endif

#if DISTRHO_PLUGIN_HAS_UI
    static void setSizeCallback(void* ptr, uint width, uint height)
    {
        uiPtr->setSize(width, height);
    }
#endif

    #undef uiPtr
};

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

int main()
{
    USE_NAMESPACE_DISTRHO;

    jack_status_t  status = jack_status_t(0x0);
    jack_client_t* client = jack_client_open(DISTRHO_PLUGIN_NAME, JackNoStartServer, &status);

    if (client == nullptr)
    {
        String errorString;

        if (status & JackFailure)
            errorString += "Overall operation failed;\n";
        if (status & JackInvalidOption)
            errorString += "The operation contained an invalid or unsupported option;\n";
        if (status & JackNameNotUnique)
            errorString += "The desired client name was not unique;\n";
        if (status & JackServerStarted)
            errorString += "The JACK server was started as a result of this operation;\n";
        if (status & JackServerFailed)
            errorString += "Unable to connect to the JACK server;\n";
        if (status & JackServerError)
            errorString += "Communication error with the JACK server;\n";
        if (status & JackNoSuchClient)
            errorString += "Requested client does not exist;\n";
        if (status & JackLoadFailure)
            errorString += "Unable to load internal client;\n";
        if (status & JackInitFailure)
            errorString += "Unable to initialize client;\n";
        if (status & JackShmFailure)
            errorString += "Unable to access shared memory;\n";
        if (status & JackVersionError)
            errorString += "Client's protocol version does not match;\n";
        if (status & JackBackendError)
            errorString += "Backend Error;\n";
        if (status & JackClientZombie)
            errorString += "Client is being shutdown against its will;\n";

        if (errorString.isNotEmpty())
        {
            errorString[errorString.length()-2] = '.';
            d_stderr("Failed to create jack client, reason was:\n%s", errorString.buffer());
        }
        else
            d_stderr("Failed to create jack client, cannot continue!");

        return 1;
    }

    USE_NAMESPACE_DISTRHO;

    initSignalHandler();

    d_lastBufferSize = jack_get_buffer_size(client);
    d_lastSampleRate = jack_get_sample_rate(client);
#if DISTRHO_PLUGIN_HAS_UI
    d_lastUiSampleRate = d_lastSampleRate;
#endif

    const PluginJack p(client);

    return 0;
}

// -----------------------------------------------------------------------
