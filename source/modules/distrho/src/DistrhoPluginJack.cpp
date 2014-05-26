/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#if ! DISTRHO_PLUGIN_HAS_UI
# error JACK export requires an UI
#endif

#include "DistrhoUIInternal.hpp"

#include "jack/jack.h"
#include "jack/midiport.h"
#include "jack/transport.h"

// -----------------------------------------------------------------------

START_NAMESPACE_DISTRHO

#if ! DISTRHO_PLUGIN_WANT_STATE
static const setStateFunc setStateCallback = nullptr;
#endif

// -----------------------------------------------------------------------

class PluginJack : public IdleCallback
{
public:
    PluginJack(jack_client_t* const client)
        : fPlugin(),
          fUI(this, 0, nullptr, setParameterValueCallback, setStateCallback, nullptr, setSizeCallback, fPlugin.getInstancePointer()),
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
            fPlugin.setProgram(0);
#endif

        if (const uint32_t count = fPlugin.getParameterCount())
        {
            fLastOutputValues = new float[count];

            for (uint32_t i=0; i < count; ++i)
            {
                if (fPlugin.isParameterOutput(i))
                    fLastOutputValues[i] = fPlugin.getParameterValue(i);
                else
                    fLastOutputValues[i] = 0.0f;
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

        jack_activate(fClient);

        if (const char* const name = jack_get_client_name(fClient))
            fUI.setTitle(name);
        else
            fUI.setTitle(DISTRHO_PLUGIN_NAME);

        fUI.exec(this);
    }

    ~PluginJack()
    {
        if (fClient == nullptr)
            return;

        jack_deactivate(fClient);

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
    void idleCallback() override
    {
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
        fTimePos.playing = (jack_transport_query(fClient, &pos) == JackTransportRolling);

        if (pos.unique_1 == pos.unique_2)
        {
            if (pos.valid & JackTransportPosition)
                fTimePos.frame = pos.frame;
            else
                fTimePos.frame = 0;

            if (pos.valid & JackTransportBBT)
            {
                fTimePos.bbt.valid = true;

                fTimePos.bbt.bar  = pos.bar;
                fTimePos.bbt.beat = pos.beat;
                fTimePos.bbt.tick = pos.tick;
                fTimePos.bbt.barStartTick = pos.bar_start_tick;

                fTimePos.bbt.beatsPerBar = pos.beats_per_bar;
                fTimePos.bbt.beatType    = pos.beat_type;

                fTimePos.bbt.ticksPerBeat   = pos.ticks_per_beat;
                fTimePos.bbt.beatsPerMinute = pos.beats_per_minute;
            }
            else
                fTimePos.bbt.valid = false;
        }
        else
        {
            fTimePos.bbt.valid = false;
            fTimePos.frame = 0;
        }

        fPlugin.setTimePos(fTimePos);
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
                if (jevent.size > 4)
                    continue;

                MidiEvent& midiEvent(midiEvents[midiEventCount++]);

                midiEvent.frame = jevent.time;
                midiEvent.size  = jevent.size;
                std::memcpy(midiEvent.buf, jevent.buffer, jevent.size);
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
        fUI.quit();
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

    void setSize(const uint width, const uint height)
    {
        fUI.setSize(width, height);
    }

    // -------------------------------------------------------------------

private:
    PluginExporter fPlugin;
    UIExporter     fUI;

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
    TimePos fTimePos;
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

    static void setSizeCallback(void* ptr, uint width, uint height)
    {
        uiPtr->setSize(width, height);
    }

    #undef uiPtr
};

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

int main()
{
    jack_status_t  status = jack_status_t(0x0);
    jack_client_t* client = jack_client_open(DISTRHO_PLUGIN_NAME, JackNoStartServer, &status);

    if (client == nullptr)
    {
        d_string errorString;

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

    d_lastBufferSize = jack_get_buffer_size(client);
    d_lastSampleRate = jack_get_sample_rate(client);
    d_lastUiSampleRate = d_lastSampleRate;

    const PluginJack p(client);

    return 0;
}

// -----------------------------------------------------------------------
