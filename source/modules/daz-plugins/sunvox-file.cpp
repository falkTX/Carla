/*
 * Carla Native Plugins
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNative.hpp"

#ifndef CARLA_OS_WIN
# include <dlfcn.h>
#endif

#define SUNVOX_MAIN
#include "sunvox/sunvox.h"

class SunVoxFilePlugin : public PluginDescriptorClass
{
public:
    SunVoxFilePlugin(const HostDescriptor* const host)
        : PluginDescriptorClass(host),
          fSlot(sLastSlot++)
    {
        sv_open_slot(fSlot);
        sv_set_autostop(fSlot, 0);
    }

    ~SunVoxFilePlugin() override
    {
        sv_close_slot(fSlot);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin state calls

    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);

        if (std::strcmp(key, "file") != 0)
            return;

        sv_load(fSlot, value);
        sv_play_from_beginning(fSlot);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float** outBuf, const uint32_t frames, const uint32_t, const MidiEvent* const) override
    {
        const TimeInfo* const timePos = getTimeInfo();

        if (timePos->playing)
        {
            float svBuffer[frames*2];

            for (uint32_t i=0, j=0; i < frames; ++i)
            {
                svBuffer[j++] = outBuf[0][i];
                svBuffer[j++] = outBuf[1][i];
            }

            unsigned int ticks = sv_get_ticks();
            ticks += timePos->frame * sTicksPerFrame;

            sv_audio_callback(svBuffer, frames, 0, ticks+10000);
        }
        else
        {
           FloatVectorOperations::clear(outBuf[0], frames);
           FloatVectorOperations::clear(outBuf[1], frames);
        }
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (! show)
            return;

        if (const char* const filename = uiOpenFile(false, "Open Audio File", "MIDI Files *.mid;*.midi;;"))
        {
            uiCustomDataChanged("file", filename);
        }

        uiClosed();
    }

private:
    int fSlot;

    static int sInstanceCount;
    static int sLastSlot;
    static double sTicksPerFrame;

public:
    static PluginHandle _instantiate(HostDescriptor* host)
    {
        if (sInstanceCount == 0)
        {
            CARLA_ASSERT(sLastSlot == 0);
            CARLA_ASSERT(sTicksPerFrame == 0);

            if (sv_load_dll() != 0)
                return nullptr;

            const double sampleRate(host->get_sample_rate(host->handle));

            if (sv_init(nullptr, (int)sampleRate, 2, SV_INIT_FLAG_USER_AUDIO_CALLBACK|SV_INIT_FLAG_AUDIO_FLOAT32|SV_INIT_FLAG_ONE_THREAD) == 0)
                return nullptr;

            sTicksPerFrame = double(sv_get_ticks_per_second())/sampleRate;
        }

        sInstanceCount++;

        return new SunVoxFilePlugin(host);
    }

    static void _cleanup(PluginHandle handle)
    {
        delete (SunVoxFilePlugin*)handle;

        if (--sInstanceCount == 0)
        {
            CARLA_ASSERT(sLastSlot > 0);
            CARLA_ASSERT(sTicksPerFrame > 0.0);

            sLastSlot = 0;
            sTicksPerFrame = 0.0;

            sv_deinit();
            sv_unload_dll();
        }
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SunVoxFilePlugin)
};

int    SunVoxFilePlugin::sInstanceCount = 0;
int    SunVoxFilePlugin::sLastSlot = 0;
double SunVoxFilePlugin::sTicksPerFrame = 0.0;

// -----------------------------------------------------------------------

static const PluginDescriptor sunvoxfileDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_HAS_GUI),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 0,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "SunVox File",
    /* label     */ "sunvoxfile",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(SunVoxFilePlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_sunvoxfile()
{
    carla_register_native_plugin(&sunvoxfileDesc);
}

// -----------------------------------------------------------------------
