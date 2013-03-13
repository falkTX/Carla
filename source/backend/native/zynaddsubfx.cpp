/*
 * Carla Native Plugins
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

// for UINT32_MAX
#define __STDC_LIMIT_MACROS
#include <cstdint>

#include "CarlaNative.hpp"
#include "CarlaMIDI.h"
#include "CarlaString.hpp"

#include "zynaddsubfx/Misc/Master.h"
#include "zynaddsubfx/Misc/Util.h"

#include <ctime>

// Dummy variables and functions for linking purposes
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
        : PluginDescriptorClass(host),
          kMaster(new Master()),
          kSampleRate(getSampleRate())
    {
        // refresh banks
        kMaster->bank.rescanforbanks();

        for (uint32_t i=0, size = kMaster->bank.banks.size(); i < size; i++)
        {
            if (kMaster->bank.banks[i].dir.empty())
                continue;

            kMaster->bank.loadbank(kMaster->bank.banks[i].dir);

            for (unsigned int instrument = 0; instrument < BANK_SIZE; instrument++)
            {
                const std::string insName(kMaster->bank.getname(instrument));

                if (insName.empty() || insName[0] == '\0' || insName[0] == ' ')
                    continue;

                ProgramInfo pInfo(i, instrument, insName.c_str());
                fPrograms.push_back(pInfo);
            }
        }
    }

    ~ZynAddSubFxPlugin()
    {
        //ensure that everything has stopped
        pthread_mutex_lock(&kMaster->mutex);
        pthread_mutex_unlock(&kMaster->mutex);

        fPrograms.clear();

        delete kMaster;
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
        CARLA_ASSERT(index < getParameterCount());

        switch (index)
        {
#if 0
        case PARAMETER_MASTER:
            return kMaster->Pvolume;
#endif
        default:
            return 0.0f;
        }
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount()
    {
        return fPrograms.size();
    }

    const MidiProgram* getMidiProgramInfo(const uint32_t index)
    {
        CARLA_ASSERT(index < getMidiProgramCount());

        if (index >= fPrograms.size())
            return nullptr;

        const ProgramInfo& pInfo(fPrograms[index]);

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
        CARLA_ASSERT(index < getParameterCount());

        switch (index)
        {
        }

        return;

        // unused, TODO
        (void)value;
    }

    void setMidiProgram(const uint32_t bank, const uint32_t program)
    {
        if (bank >= kMaster->bank.banks.size())
            return;
        if (program >= BANK_SIZE)
            return;

        const std::string bankdir(kMaster->bank.banks[bank].dir);

        if (! bankdir.empty())
        {
            pthread_mutex_lock(&kMaster->mutex);

            kMaster->bank.loadbank(bankdir);
            kMaster->bank.loadfromslot(program, kMaster->part[0]);

            pthread_mutex_unlock(&kMaster->mutex);
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate()
    {
        kMaster->setController(0, MIDI_CONTROL_ALL_SOUND_OFF, 0);
    }

    void process(float**, float** const outBuffer, const uint32_t frames, const uint32_t midiEventCount, const MidiEvent* const midiEvents)
    {
        if (pthread_mutex_trylock(&kMaster->mutex) != 0)
        {
            carla_zeroFloat(outBuffer[0], frames);
            carla_zeroFloat(outBuffer[1], frames);
            return;
        }

        for (uint32_t i=0; i < midiEventCount; i++)
        {
            const MidiEvent* const midiEvent = &midiEvents[i];

            const uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent->data);
            const uint8_t channel = MIDI_GET_CHANNEL_FROM_DATA(midiEvent->data);

            if (MIDI_IS_STATUS_NOTE_OFF(status))
            {
                const uint8_t note = midiEvent->data[1];

                kMaster->noteOff(channel, note);
            }
            else if (MIDI_IS_STATUS_NOTE_ON(status))
            {
                const uint8_t note = midiEvent->data[1];
                const uint8_t velo = midiEvent->data[2];

                kMaster->noteOn(channel, note, velo);
            }
            else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
            {
                const uint8_t note     = midiEvent->data[1];
                const uint8_t pressure = midiEvent->data[2];

                kMaster->polyphonicAftertouch(channel, note, pressure);
            }
        }

        kMaster->GetAudioOutSamples(frames, kSampleRate, outBuffer[0], outBuffer[1]);

        pthread_mutex_unlock(&kMaster->mutex);
    }

    // -------------------------------------------------------------------

private:
    struct ProgramInfo {
        uint32_t bank;
        uint32_t prog;
        CarlaString name;

        ProgramInfo(uint32_t bank_, uint32_t prog_, const char* name_)
          : bank(bank_),
            prog(prog_),
            name(name_) {}

        ProgramInfo() = delete;
    };

    std::vector<ProgramInfo> fPrograms;

    Master* const  kMaster;
    const unsigned kSampleRate;

public:
    static int sInstanceCount;

    static PluginHandle _instantiate(const PluginDescriptor*, HostDescriptor* host)
    {
        if (sInstanceCount++ == 0)
        {
            synth = new SYNTH_T;
            synth->buffersize = host->get_buffer_size(host->handle);
            synth->samplerate = host->get_sample_rate(host->handle);
            synth->alias();

            config.init();
            config.cfg.SoundBufferSize = synth->buffersize;
            config.cfg.SampleRate      = synth->samplerate;
            config.cfg.GzipCompression = 0;

            sprng(std::time(nullptr));
            denormalkillbuf = new float[synth->buffersize];
            for (int i=0; i < synth->buffersize; i++)
                denormalkillbuf[i] = (RND - 0.5f) * 1e-16;
        }

        return new ZynAddSubFxPlugin(host);
    }

    static void _cleanup(PluginHandle handle)
    {
        delete (ZynAddSubFxPlugin*)handle;

        if (--sInstanceCount == 0)
        {
            delete[] denormalkillbuf;
            denormalkillbuf = nullptr;

            delete synth;
            synth = nullptr;
        }
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZynAddSubFxPlugin)
};

int ZynAddSubFxPlugin::sInstanceCount = 0;

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
