/*
 * Carla Native Plugins
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
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

// for UINT32_MAX
#define __STDC_LIMIT_MACROS
#include <cstdint>

#include "carla_midi.h"
#include "carla_native.hpp"

#include "zynaddsubfx/Misc/Master.h"
#include "zynaddsubfx/Misc/Util.h"

//Dummy variables and functions for linking purposes
class WavFile;
namespace Nio {
   bool start(void){return 1;}
   void stop(void){}
   void waveNew(WavFile*){}
   void waveStart(void){}
   void waveStop(void){}
   void waveEnd(void){}
}

SYNTH_T* synth = nullptr;

class ZynAddSubFxPlugin : public PluginDescriptorClass
{
public:
    enum Parameters {
        PARAMETER_COUNT = 0
    };

    ZynAddSubFxPlugin(const HostDescriptor* const host)
        : PluginDescriptorClass(host)
    {
        m_master = new Master;

        // refresh banks
        m_master->bank.rescanforbanks();

        for (uint32_t i=0, size = m_master->bank.banks.size(); i < size; i++)
        {
            if (m_master->bank.banks[i].dir.empty())
                continue;

            m_master->bank.loadbank(m_master->bank.banks[i].dir);

            for (unsigned int instrument = 0; instrument < BANK_SIZE; instrument++)
            {
                const std::string insName(m_master->bank.getname(instrument));

                if (insName.empty() || insName[0] == '\0' || insName[0] == ' ')
                    continue;

                ProgramInfo pInfo = { i, instrument, CarlaString(insName.c_str()) };
                m_programs.push_back(pInfo);
            }
        }
    }

    ~ZynAddSubFxPlugin()
    {
        //ensure that everything has stopped with the mutex wait
        pthread_mutex_lock(&m_master->mutex);
        pthread_mutex_unlock(&m_master->mutex);

        m_programs.clear();

        delete m_master;
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount()
    {
        return PARAMETER_COUNT;
    }

    const Parameter* getParameterInfo(const uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());

        //if (index >= PARAMETER_COUNT)
        return nullptr;

        static Parameter param;

        param.ranges.step      = PARAMETER_RANGES_DEFAULT_STEP;
        param.ranges.stepSmall = PARAMETER_RANGES_DEFAULT_STEP_SMALL;
        param.ranges.stepLarge = PARAMETER_RANGES_DEFAULT_STEP_LARGE;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
#if 0
        case PARAMETER_MASTER:
            param.hints = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
            param.name  = "Master Volume";
            param.unit  = nullptr;
            param.ranges.min = 0.0f;
            param.ranges.max = 100.0f;
            param.ranges.def = 100.0f;
            break;
#endif
        }

        return &param;
    }

    float getParameterValue(const uint32_t index)
    {
        switch (index)
        {
#if 0
        case PARAMETER_MASTER:
            return m_master->Pvolume;
#endif
        default:
            return 0.0f;
        }
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount()
    {
        return m_programs.size();
    }

    const MidiProgram* getMidiProgramInfo(const uint32_t index)
    {
        CARLA_ASSERT(index < getMidiProgramCount());

        if (index >= m_programs.size())
            return nullptr;

        const ProgramInfo& pInfo(m_programs[index]);

        static MidiProgram midiProgram;
        midiProgram.bank    = pInfo.bank;
        midiProgram.program = pInfo.prog;
        midiProgram.name    = pInfo.name;

        return &midiProgram;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value)
    {
        switch (index)
        {
        }
        Q_UNUSED(value);
    }

    void setMidiProgram(const uint32_t bank, const uint32_t program)
    {
        if (bank >= m_master->bank.banks.size())
            return;
        if (program >= BANK_SIZE)
            return;

        const std::string bankdir = m_master->bank.banks[bank].dir;

        if (! bankdir.empty())
        {
            pthread_mutex_lock(&m_master->mutex);

            m_master->bank.loadbank(bankdir);
            m_master->bank.loadfromslot(program, m_master->part[0]);

            pthread_mutex_unlock(&m_master->mutex);
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate()
    {
        m_master->setController(0, MIDI_CONTROL_ALL_SOUND_OFF, 0);
    }

    void process(float**, float** const outBuffer, const uint32_t frames, const uint32_t midiEventCount, const MidiEvent* const midiEvents)
    {
        uint32_t fromFrame      = 0;
        uint32_t eventIndex     = 0;
        uint32_t nextEventFrame = 0;
        uint32_t toFrame = 0;
        pthread_mutex_lock(&m_master->mutex);

        do {
            // Find the time of the next event, if any
            if (eventIndex >= midiEventCount)
                nextEventFrame = UINT32_MAX;
            else
                nextEventFrame = midiEvents[eventIndex].time;

            // find the end of the sub-sample to be processed this time round...
            // if the next event falls within the desired sample interval...
            if ((nextEventFrame < frames) && (nextEventFrame >= toFrame))
                // set the end to be at that event
                toFrame = nextEventFrame;
            else
                // ...else go for the whole remaining sample
                toFrame = frames;

            if (fromFrame < toFrame)
            {
                // call master to fill from `fromFrame` to `toFrame`:
                m_master->GetAudioOutSamples(toFrame - fromFrame, (unsigned)getSampleRate(), &outBuffer[0][fromFrame], &outBuffer[1][fromFrame]);
                // next sub-sample please...
                fromFrame = toFrame;
            }

            // Now process any event(s) at the current timing point
            while (eventIndex < midiEventCount && midiEvents[eventIndex].time == toFrame)
            {
                uint8_t status  = midiEvents[eventIndex].data[0];
                uint8_t channel = status & 0x0F;

                if (MIDI_IS_STATUS_NOTE_OFF(status))
                {
                    uint8_t note = midiEvents[eventIndex].data[1];

                    m_master->noteOff(channel, note);
                }
                else if (MIDI_IS_STATUS_NOTE_ON(status))
                {
                    uint8_t note = midiEvents[eventIndex].data[1];
                    uint8_t velo = midiEvents[eventIndex].data[2];

                    m_master->noteOn(channel, note, velo);
                }
                else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                {
                    uint8_t note     = midiEvents[eventIndex].data[1];
                    uint8_t pressure = midiEvents[eventIndex].data[2];

                    m_master->polyphonicAftertouch(channel, note, pressure);
                }

                eventIndex++;
            }

        // Keep going until we have the desired total length of sample...
        } while (toFrame < frames);

        pthread_mutex_unlock(&m_master->mutex);
    }

    // -------------------------------------------------------------------

private:
    struct ProgramInfo {
        uint32_t bank;
        uint32_t prog;
        CarlaString name;
    };
    std::vector<ProgramInfo> m_programs;

    Master* m_master;

public:
    static int s_instanceCount;

    static PluginHandle _instantiate(const PluginDescriptor*, HostDescriptor* host)
    {
        if (s_instanceCount++ == 0)
        {
            synth = new SYNTH_T;
            synth->buffersize = host->get_buffer_size(host->handle);
            synth->samplerate = host->get_sample_rate(host->handle);
            synth->alias();

            config.init();
            config.cfg.SoundBufferSize = synth->buffersize;
            config.cfg.SampleRate      = synth->samplerate;
            config.cfg.GzipCompression = 0;

            sprng(time(NULL));
            denormalkillbuf = new float [synth->buffersize];
            for (int i=0; i < synth->buffersize; i++)
                denormalkillbuf[i] = (RND - 0.5f) * 1e-16;
        }

        return new ZynAddSubFxPlugin(host);
    }

    static void _cleanup(PluginHandle handle)
    {
        delete (ZynAddSubFxPlugin*)handle;

        if (--s_instanceCount == 0)
        {
            delete[] denormalkillbuf;
            denormalkillbuf = nullptr;

            delete synth;
            synth = nullptr;
        }
    }
};

int ZynAddSubFxPlugin::s_instanceCount = 0;

// -----------------------------------------------------------------------

static const PluginDescriptor zynAddSubFxDesc = {
    /* category  */ PLUGIN_CATEGORY_SYNTH,
    /* hints     */ PLUGIN_IS_SYNTH,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ ZynAddSubFxPlugin::PARAMETER_COUNT,
    /* paramOuts */ 0,
    /* name      */ "ZynAddSubFX",
    /* label     */ "zynaddsubfx",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(ZynAddSubFxPlugin)
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_zynaddsubfx()
{
    carla_register_native_plugin(&zynAddSubFxDesc);
}

// -----------------------------------------------------------------------
