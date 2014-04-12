/*
 * Segment Juice Plugin
 * Copyright (C) 2014 Andre Sklenar <andre.sklenar@gmail.com>, www.juicelab.cz
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

#ifndef SEGMENTJUICEPLUGIN_HPP_INCLUDED
#define SEGMENTJUICEPLUGIN_HPP_INCLUDED

#include "DistrhoPlugin.hpp"
#include <iostream>
#include "Synth.hxx"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class SegmentJuicePlugin : public Plugin
{
public:
    enum Parameters
    {
        paramWave1 = 0,
        paramWave2,
        paramWave3,
        paramWave4,
        paramWave5,
        paramWave6,
        paramFM1,
        paramFM2,
        paramFM3,
        paramFM4,
        paramFM5,
        paramFM6,
        paramPan1,
        paramPan2,
        paramPan3,
        paramPan4,
        paramPan5,
        paramPan6,
        paramAmp1,
        paramAmp2,
        paramAmp3,
        paramAmp4,
        paramAmp5,
        paramAmp6,
        paramAttack,
        paramDecay,
        paramSustain,
        paramRelease,
        paramStereo,
        paramTune,
        paramVolume,
        paramGlide,
        paramCount
    };

    SegmentJuicePlugin();
    ~SegmentJuicePlugin() override;

protected:
    // -------------------------------------------------------------------
    // Information

    const char* d_getLabel() const noexcept override
    {
        return "SegmentJuice";
    }

    const char* d_getMaker() const noexcept override
    {
        return "Andre Sklenar";
    }

    const char* d_getLicense() const noexcept override
    {
        return "GPL v2+";
    }

    uint32_t d_getVersion() const noexcept override
    {
        return 0x1000;
    }

    long d_getUniqueId() const noexcept override
    {
        return d_cconst('S', 'g', 't', 'J');
    }

    // -------------------------------------------------------------------
    // Init

    void d_initParameter(uint32_t index, Parameter& parameter) override;
    void d_initProgramName(uint32_t index, d_string& programName) override;

    // -------------------------------------------------------------------
    // Internal data

    float d_getParameterValue(uint32_t index) const override;
    void  d_setParameterValue(uint32_t index, float value) override;
    void  d_setProgram(uint32_t index) override;

    // -------------------------------------------------------------------
    // Process

    void d_activate() override;
    void d_deactivate() override;
    void d_run(float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override;

    // -------------------------------------------------------------------

private:
    CSynth synthL, synthR;
    float wave1, wave2, wave3, wave4, wave5, wave6;
    float FM1, FM2, FM3, FM4, FM5, FM6;
    float pan1, pan2, pan3, pan4, pan5, pan6;
    float amp1, amp2, amp3, amp4, amp5, amp6;
    float attack, decay, sustain, release, stereo, tune, volume, glide;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // SEGMENTJUICE_HPP_INCLUDED
