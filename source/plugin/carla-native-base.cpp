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

#include "CarlaNative.h"
#include "RtList.hpp"

#include "lv2/lv2.h"

// -----------------------------------------------------------------------
// Plugin register calls

extern "C" {

// Simple plugins (C)
void carla_register_native_plugin_bypass();
void carla_register_native_plugin_lfo();
void carla_register_native_plugin_midiGain();
void carla_register_native_plugin_midiSplit();
void carla_register_native_plugin_midiThrough();
void carla_register_native_plugin_midiTranspose();
void carla_register_native_plugin_nekofilter();

// Simple plugins (C++)
void carla_register_native_plugin_vex_fx();
void carla_register_native_plugin_vex_synth();

#ifdef WANT_AUDIOFILE
// AudioFile
void carla_register_native_plugin_audiofile();
#endif

#ifdef WANT_MIDIFILE
// MidiFile
void carla_register_native_plugin_midifile();
#endif

#ifdef WANT_OPENGL
// DISTRHO plugins (OpenGL)
void carla_register_native_plugin_3BandEQ();
void carla_register_native_plugin_3BandSplitter();
void carla_register_native_plugin_Nekobi();
void carla_register_native_plugin_PingPongPan();
// void carla_register_native_plugin_StereoEnhancer();
#endif

// DISTRHO plugins (PyQt)
void carla_register_native_plugin_BigMeter();
void carla_register_native_plugin_BigMeterM();
void carla_register_native_plugin_Notes();

#ifdef WANT_ZYNADDSUBFX
// ZynAddSubFX
void carla_register_native_plugin_zynaddsubfx_fx();
void carla_register_native_plugin_zynaddsubfx_synth();
#endif
}

// -----------------------------------------------------------------------
// Plugin List

struct PluginListManager {
    PluginListManager()
    {
        // Simple plugins (C)
        carla_register_native_plugin_bypass();
        carla_register_native_plugin_lfo();
        carla_register_native_plugin_midiGain();
        carla_register_native_plugin_midiSplit();
        carla_register_native_plugin_midiThrough();
        carla_register_native_plugin_midiTranspose();
        carla_register_native_plugin_nekofilter();

        // Simple plugins (C++)
        carla_register_native_plugin_vex_fx();
        carla_register_native_plugin_vex_synth();

#ifdef WANT_AUDIOFILE
        // AudioFile
        carla_register_native_plugin_audiofile();
#endif

#ifdef WANT_MIDIFILE
        // MidiFile
        carla_register_native_plugin_midifile();
#endif

#ifdef WANT_OPENGL
        // DISTRHO plugins (OpenGL)
        carla_register_native_plugin_3BandEQ();
        carla_register_native_plugin_3BandSplitter();
        carla_register_native_plugin_Nekobi();
        carla_register_native_plugin_PingPongPan();
        //carla_register_native_plugin_StereoEnhancer(); // unfinished
#endif

        // DISTRHO plugins (PyQt)
        carla_register_native_plugin_BigMeter();
        carla_register_native_plugin_BigMeterM();
        carla_register_native_plugin_Notes(); // unfinished

#ifdef WANT_ZYNADDSUBFX
        // ZynAddSubFX
        carla_register_native_plugin_zynaddsubfx_fx();
        carla_register_native_plugin_zynaddsubfx_synth();
#endif
    }

    ~PluginListManager()
    {
        for (NonRtList<const LV2_Descriptor*>::Itenerator it = lv2Descs.begin(); it.valid(); it.next())
        {
            const LV2_Descriptor*& lv2Desc(*it);
            delete[] lv2Desc->URI;
            delete lv2Desc;
        }

        descs.clear();
        lv2Descs.clear();
    }

    NonRtList<const PluginDescriptor*> descs;
    NonRtList<const LV2_Descriptor*> lv2Descs;
};

static PluginListManager sPluginDescsMgr;

void carla_register_native_plugin(const PluginDescriptor* desc)
{
#ifdef CARLA_NATIVE_PLUGIN_LV2
    // LV2 MIDI Out and Open/Save are not implemented yet
    if (desc->midiOuts > 0 || (desc->hints & PLUGIN_NEEDS_UI_OPEN_SAVE) != 0)
        return;
#endif
    sPluginDescsMgr.descs.append(desc);
}

// -----------------------------------------------------------------------
