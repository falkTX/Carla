/*
 * DISTRHO Nekobi Plugin, based on Nekobee by Sean Bolton and others.
 * Copyright (C) 2004 Sean Bolton and others
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef DISTRHO_PLUGIN_3BANDEQ_HPP_INCLUDED
#define DISTRHO_PLUGIN_3BANDEQ_HPP_INCLUDED

#include "DistrhoPlugin.hpp"

extern "C" {
#include "nekobee-src/nekobee_types.h"
}

START_NAMESPACE_DISTRHO

class DistrhoPluginNekobi : public Plugin
{
public:
    enum Parameters
    {
        paramWaveform = 0,
        paramTuning,
        paramCutoff,
        paramResonance,
        paramEnvMod,
        paramDecay,
        paramAccent,
        paramVolume,
        paramCount
    };

    DistrhoPluginNekobi();
    ~DistrhoPluginNekobi();

protected:
    // ---------------------------------------------
    // Information

    const char* d_label() const
    {
        return "Nekobi";
    }

    const char* d_maker() const
    {
        return "DISTRHO";
    }

    const char* d_license() const
    {
        return "GPL v2+";
    }

    uint32_t d_version() const
    {
        return 0x1000;
    }

    long d_uniqueId() const
    {
        return d_cconst('D', 'N', 'e', 'k');
    }

    // ---------------------------------------------
    // Init

    void d_initParameter(uint32_t index, Parameter& parameter);

    // ---------------------------------------------
    // Internal data

    float d_parameterValue(uint32_t index);
    void  d_setParameterValue(uint32_t index, float value);

    // ---------------------------------------------
    // Process

    void d_activate();
    void d_deactivate();
    void d_run(float** inputs, float** outputs, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents);

    // ---------------------------------------------

private:
    struct ParamValues {
        float waveform;
        float tuning;
        float cutoff;
        float resonance;
        float envMod;
        float decay;
        float accent;
        float volume;
    } fParams;

    nekobee_synth_t* fSynth;
};

END_NAMESPACE_DISTRHO

#endif  // DISTRHO_PLUGIN_3BANDEQ_HPP_INCLUDED
