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

#include "SegmentJuicePlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

SegmentJuicePlugin::SegmentJuicePlugin()
    : Plugin(paramCount, 1, 0) // 1 program, 0 states
{
    // set default values
    d_setProgram(0);

    // reset
    d_deactivate();
}

SegmentJuicePlugin::~SegmentJuicePlugin()
{
}

// -----------------------------------------------------------------------
// Init

void SegmentJuicePlugin::d_initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
    case paramWave1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Wave1";
        parameter.symbol     = "w1";
        parameter.unit       = "";
        parameter.ranges.def = 3.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 4.0f;
        break;
    case paramWave2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Wave2";
        parameter.symbol     = "w2";
        parameter.unit       = "";
        parameter.ranges.def = 3.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 4.0f;
        break;
    case paramWave3:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Wave3";
        parameter.symbol     = "w3";
        parameter.unit       = "";
        parameter.ranges.def = 3.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 4.0f;
        break;
    case paramWave4:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Wave4";
        parameter.symbol     = "w4";
        parameter.unit       = "";
        parameter.ranges.def = 3.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 4.0f;
        break;
    case paramWave5:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Wave5";
        parameter.symbol     = "w5";
        parameter.unit       = "";
        parameter.ranges.def = 3.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 4.0f;
        break;
    case paramWave6:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "w6";
        parameter.symbol     = "";
        parameter.unit       = "";
        parameter.ranges.def = 3.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 4.0f;
        break;
    case paramFM1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "FM1";
        parameter.symbol     = "f1";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramFM2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "FM2";
        parameter.symbol     = "f2";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramFM3:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "FM3";
        parameter.symbol     = "f3";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramFM4:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "FM4";
        parameter.symbol     = "f4";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramFM5:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "FM5";
        parameter.symbol     = "f5";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramFM6:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "FM6";
        parameter.symbol     = "f6";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -1.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramPan1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Pan1";
        parameter.symbol     = "p1";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -1.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramPan2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Pan2";
        parameter.symbol     = "p2";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -1.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramPan3:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Pan3";
        parameter.symbol     = "p3";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -1.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramPan4:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Pan4";
        parameter.symbol     = "p4";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -1.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramPan5:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Pan5";
        parameter.symbol     = "p5";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -1.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramPan6:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "p6";
        parameter.symbol     = "p6";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -1.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramAmp1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Amp1";
        parameter.symbol     = "a1";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramAmp2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Amp2";
        parameter.symbol     = "a2";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramAmp3:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Amp3";
        parameter.symbol     = "a3";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramAmp4:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Amp4";
        parameter.symbol     = "a4";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramAmp5:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Amp5";
        parameter.symbol     = "a5";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramAmp6:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Amp6";
        parameter.symbol     = "a6";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramAttack:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Attack";
        parameter.symbol     = "att";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramDecay:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Decay";
        parameter.symbol     = "dec";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramSustain:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Sustain";
        parameter.symbol     = "sus";
        parameter.unit       = "";
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramRelease:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Release";
        parameter.symbol     = "rel";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramStereo:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Stereo";
        parameter.symbol     = "str";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -1.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramTune:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Tune";
        parameter.symbol     = "tun";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -1.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramVolume:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Volume";
        parameter.symbol     = "vol";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramGlide:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Glide";
        parameter.symbol     = "gld";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    }
}

void SegmentJuicePlugin::d_initProgramName(uint32_t index, d_string& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

// -----------------------------------------------------------------------
// Internal data

float SegmentJuicePlugin::d_getParameterValue(uint32_t index) const
{
    switch (index)
    {
    case paramWave1:
        return wave1;
    case paramWave2:
        return wave2;
    case paramWave3:
        return wave3;
    case paramWave4:
        return wave4;
    case paramWave5:
        return wave5;
    case paramWave6:
        return wave6;
    case paramFM1:
        return FM1;
    case paramFM2:
        return FM2;
    case paramFM3:
        return FM3;
    case paramFM4:
        return FM4;
    case paramFM5:
        return FM5;
    case paramFM6:
        return FM6;
    case paramPan1:
        return pan1;
    case paramPan2:
        return pan2;
    case paramPan3:
        return pan3;
    case paramPan4:
        return pan4;
    case paramPan5:
        return pan5;
    case paramPan6:
        return pan6;
    case paramAmp1:
        return amp1;
    case paramAmp2:
        return amp2;
    case paramAmp3:
        return amp3;
    case paramAmp4:
        return amp4;
    case paramAmp5:
        return amp5;
    case paramAmp6:
        return amp6;
    case paramAttack:
        return attack;
    case paramDecay:
        return decay;
    case paramSustain:
        return sustain;
    case paramRelease:
        return release;
    case paramStereo:
        return stereo;
    case paramTune:
        return tune;
    case paramVolume:
        return volume;
    case paramGlide:
        return glide;

    default:
        return 0.0f;
    }
}

void SegmentJuicePlugin::d_setParameterValue(uint32_t index, float value)
{
    switch (index)
    {
    case paramWave1:
        synthL.setWave(0, value);
        synthR.setWave(0, value);
        wave1 = value;
        break;
    case paramWave2:
        synthL.setWave(1, value);
        synthR.setWave(1, value);
        wave2 = value;
        break;
    case paramWave3:
        synthL.setWave(2, value);
        synthR.setWave(2, value);
        wave3 = value;
        break;
    case paramWave4:
        synthL.setWave(3, value);
        synthR.setWave(3, value);
        wave4 = value;
        break;
    case paramWave5:
        synthL.setWave(4, value);
        synthR.setWave(4, value);
        wave5 = value;
        break;
    case paramWave6:
        synthL.setWave(5, value);
        synthR.setWave(5, value);
        wave6 = value;
        break;
    case paramFM1:
        synthL.setFM(0, value);
        synthR.setFM(0, value);
        FM1 = value;
        break;
    case paramFM2:
        synthL.setFM(1, value);
        synthR.setFM(1, value);
        FM2 = value;
        break;
    case paramFM3:
        synthL.setFM(2, value);
        synthR.setFM(2, value);
        FM3 = value;
        break;
    case paramFM4:
        synthL.setFM(3, value);
        synthR.setFM(3, value);
        FM4 = value;
        break;
    case paramFM5:
        synthL.setFM(4, value);
        synthR.setFM(4, value);
        FM5 = value;
        break;
    case paramFM6:
        synthL.setFM(5, value);
        synthR.setFM(5, value);
        FM6 = value;
        break;
    case paramPan1:
        synthL.setPan(0, -value);
        synthR.setPan(0, value);
        pan1 = value;
        break;
    case paramPan2:
        synthL.setPan(1, -value);
        synthR.setPan(1, value);
        pan2 = value;
        break;
    case paramPan3:
        synthL.setPan(2, -value);
        synthR.setPan(2, value);
        pan3 = value;
        break;
    case paramPan4:
        synthL.setPan(3, -value);
        synthR.setPan(3, value);
        pan4 = value;
        break;
    case paramPan5:
        synthL.setPan(4, -value);
        synthR.setPan(4, value);
        pan5 = value;
        break;
    case paramPan6:
        synthL.setPan(5, -value);
        synthR.setPan(5, value);
        pan6 = value;
        break;
    case paramAmp1:
        synthL.setAmp(0, value);
        synthR.setAmp(0, value);
        amp1 = value;
        break;
    case paramAmp2:
        synthL.setAmp(1, value);
        synthR.setAmp(1, value);
        amp2 = value;
        break;
    case paramAmp3:
        synthL.setAmp(2, value);
        synthR.setAmp(2, value);
        amp3 = value;
        break;
    case paramAmp4:
        synthL.setAmp(3, value);
        synthR.setAmp(3, value);
        amp4 = value;
        break;
    case paramAmp5:
        synthL.setAmp(4, value);
        synthR.setAmp(4, value);
        amp5 = value;
        break;
    case paramAmp6:
        synthL.setAmp(5, value);
        synthR.setAmp(5, value);
        amp6 = value;
        break;
    case paramAttack:
        synthL.setAttack(value);
        synthR.setAttack(value);
        attack = value;
        break;
    case paramDecay:
        synthL.setDecay(value);
        synthR.setDecay(value);
        attack = value;
        break;
    case paramSustain:
        synthL.setSustain(value);
        synthR.setSustain(value);
        attack = value;
        break;
    case paramRelease:
        synthL.setRelease(value);
        synthR.setRelease(value);
        attack = value;
        break;
    case paramStereo:
        synthL.setStereo(-value);
        synthR.setStereo(value);
        attack = value;
        break;
    case paramTune:
        synthL.setTune(value);
        synthR.setTune(value);
        attack = value;
        break;
    case paramVolume:
        synthL.setMAmp(value);
        synthR.setMAmp(value);
        attack = value;
    case paramGlide:
        synthL.setGlide(value);
        synthR.setGlide(value);
        glide = value;
        break;
    }
}

void SegmentJuicePlugin::d_setProgram(uint32_t index)
{
    if (index != 0)
        return;

    /* Default parameter values */
    wave1=wave2=wave3=wave4=wave5=wave6=3.0f;
    FM1=FM2=FM3=FM4=FM5=FM6=0.5f;
    pan1=pan2=pan3=pan4=pan5=pan6=0.0f;
    amp1=amp2=amp3=amp4=amp5=amp6=0.5f;
    attack=decay=release=stereo=tune=glide=0.0f;
    sustain=1.0f;
    volume=0.5f;

    for (int i=0; i<6; i++) {
        synthL.setFM(i, 0.5f);
        synthR.setFM(i, 0.5f);

        synthL.setPan(i, 0.0f);
        synthR.setPan(i, 0.0f);

        synthL.setAmp(i, 0.5f);
        synthR.setAmp(i, 0.5f);
    }

    /* Default variable values */

    synthL.setSampleRate(d_getSampleRate());
    synthR.setSampleRate(d_getSampleRate());
    synthL.setMAmp(0.5);
    synthR.setMAmp(0.5);

    synthL.setTune(0.0f);
    synthR.setTune(0.0f);

    synthL.setStereo(0.0f);
    synthR.setStereo(0.0f);

    synthL.setGlide(0.0f);
    synthR.setGlide(0.0f);

    synthL.setAttack(0.0f);
    synthR.setAttack(0.0f);

    synthL.setDecay(0.0f);
    synthR.setDecay(0.0f);

    synthL.setSustain(1.0f);
    synthR.setSustain(1.0f);

    synthL.setRelease(0.0f);
    synthR.setRelease(0.0f);

    /* reset filter values */
    d_activate();
}

// -----------------------------------------------------------------------
// Process

void SegmentJuicePlugin::d_activate()
{
}

void SegmentJuicePlugin::d_deactivate()
{
    // all values to zero
}

void SegmentJuicePlugin::d_run(float**, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount)
{
    for (uint32_t i = 0; i<midiEventCount; i++) {
        int mType = midiEvents[i].buf[0] & 0xF0;
        //int mChan = midiEvents[i].buf[0] & 0x0F;
        int mNum = midiEvents[i].buf[1];
        if (mType == 0x90) {
            //note on
            synthL.play(mNum);
            synthR.play(mNum);
        } else if (mType == 0x80) {
            //note off
            synthL.stop(mNum);
            synthR.stop(mNum);
        }
    }

    for (uint32_t i = 0; i<frames; i++) {
        outputs[0][i] = synthL.run();
        outputs[1][i] = synthR.run();
    }
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new SegmentJuicePlugin();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
