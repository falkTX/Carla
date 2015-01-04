/*
 * DISTRHO Nekobi Plugin, based on Nekobee by Sean Bolton and others.
 * Copyright (C) 2004 Sean Bolton and others
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the LICENSE file.
 */

#ifndef DISTRHO_PLUGIN_NEKOBI_HPP_INCLUDED
#define DISTRHO_PLUGIN_NEKOBI_HPP_INCLUDED

#include "DistrhoPlugin.hpp"

extern "C" {
#include "nekobee-src/nekobee_synth.h"
}

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

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
    ~DistrhoPluginNekobi() override;

protected:
    // -------------------------------------------------------------------
    // Information

    const char* d_getLabel() const noexcept override
    {
        return "Nekobi";
    }

    const char* d_getMaker() const noexcept override
    {
        return "Sean Bolton, falkTX";
    }

    const char* d_getLicense() const noexcept override
    {
        return "GPL v2+";
    }

    uint32_t d_getVersion() const noexcept override
    {
        return 0x1000;
    }

    int64_t d_getUniqueId() const noexcept override
    {
        return d_cconst('D', 'N', 'e', 'k');
    }

    // -------------------------------------------------------------------
    // Init

    void d_initParameter(uint32_t index, Parameter& parameter) override;

    // -------------------------------------------------------------------
    // Internal data

    float d_getParameterValue(uint32_t index) const override;
    void  d_setParameterValue(uint32_t index, float value) override;

    // -------------------------------------------------------------------
    // Process

    void d_activate() override;
    void d_deactivate() override;
    void d_run(const float**, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override;

    // -------------------------------------------------------------------

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

    nekobee_synth_t fSynth;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistrhoPluginNekobi)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // DISTRHO_PLUGIN_NEKOBI_HPP_INCLUDED
