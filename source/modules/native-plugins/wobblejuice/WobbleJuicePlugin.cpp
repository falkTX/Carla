/*
 * Wobble Juice Plugin
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

#include "WobbleJuicePlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

WobbleJuicePlugin::WobbleJuicePlugin()
    : Plugin(paramCount, 1, 0) // 1 program, 0 states
{
    // set default values
    d_setProgram(0);
}

// -----------------------------------------------------------------------
// Init

void WobbleJuicePlugin::d_initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
    case paramDivision:
        parameter.hints      = PARAMETER_IS_AUTOMABLE|PARAMETER_IS_INTEGER;
        parameter.name       = "Division";
        parameter.symbol     = "div";
        parameter.unit       = "x";
        parameter.ranges.def = 4.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 16.0f;
        break;
    case paramReso:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Resonance";
        parameter.symbol     = "reso";
        parameter.unit       = "";
        parameter.ranges.def = 0.1f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 0.2f;
        break;
    case paramRange:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Range";
        parameter.symbol     = "rng";
        parameter.unit       = "Hz";
        parameter.ranges.def = 16000.0f;
        parameter.ranges.min = 500.0f;
        parameter.ranges.max = 16000.0f;
        break;
    case paramPhase:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Phase";
        parameter.symbol     = "phs";
        parameter.unit       = "Deg";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -1.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramWave:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Wave";
        parameter.symbol     = "wav";
        parameter.unit       = "";
        parameter.ranges.def = 2.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 4.0f;
        break;
    case paramDrive:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Drive";
        parameter.symbol     = "drv";
        parameter.unit       = "";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    }
}

void WobbleJuicePlugin::d_initProgramName(uint32_t index, d_string& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

// -----------------------------------------------------------------------
// Internal data

float WobbleJuicePlugin::d_getParameterValue(uint32_t index) const
{
    switch (index)
    {
    case paramDivision:
        return division;
    case paramReso:
        return reso;
    case paramRange:
        return range;
    case paramPhase:
        return phase;
    case paramWave:
        return wave;
    case paramDrive:
        return drive;
    default:
        return 0.0f;
    }
}

void WobbleJuicePlugin::d_setParameterValue(uint32_t index, float value)
{
    switch (index)
    {
    case paramDivision:
        division = value;
        break;
    case paramReso:
        reso = value;
        break;
    case paramRange:
        range = value;
        break;
    case paramPhase:
        phase = value;
        break;
    case paramWave:
        wave = value;
        break;
    case paramDrive:
        drive = value;
        break;
    }
}

void WobbleJuicePlugin::d_setProgram(uint32_t index)
{
    if (index != 0)
        return;

    /* Default parameter values */
    division = 4.0f;
    reso     = 0.1f;
    range    = 16000.0f;
    phase    = 0.0f;
    wave     = 2.0f;
    drive    = 0.5f;

    /* Default variable values */
    bar=tick=tickOffset=percentage=phaseOffset=currentPhaseL=0.0f;
    currentPhaseR=posL=posR=cutoffL=cutoffR=0.0f;
    waveType=2.0f;

    /* reset filter values */
    d_activate();
}

// -----------------------------------------------------------------------
// Process

void WobbleJuicePlugin::d_activate()
{
    sinePos = 0.0;
}

void WobbleJuicePlugin::d_run(const float** inputs, float** outputs, uint32_t frames)
{
    //fetch the timepos struct from host;
    const TimePos& time(d_getTimePos());

    /* sample count for one bar */
    bar = ((120.0/(time.bbt.valid ? time.bbt.beatsPerMinute : 120.0))*(d_getSampleRate())); //ONE, two, three, four
    tick = bar/(std::round(division)); //size of one target wob
    phaseOffset = phase*M_PI; //2pi = 1 whole cycle

    /* if rolling then sync to timepos */
    if (time.playing)
    {
        tickOffset = time.frame-std::floor(time.frame/tick)*tick; //how much after last tick

        if (tickOffset!=0) {
            //TODO: why do we need this??
            percentage = tickOffset/tick;
        } else {
            percentage = 0;
        }
        sinePos = (M_PI*2)*percentage;

        if (sinePos>2*M_PI) {
            //TODO: can this ever happen??
            sinePos = 0;
        }
    }
    /* else just keep on wobblin' */
    else
    {
        sinePos += (M_PI)/(tick/2000); //wtf, but works
        if (sinePos>2*M_PI) {
            sinePos = 0;
        }
    }

    /* phase of 0..1 filter = 500..16k */
    currentPhaseL = getBlendedPhase(sinePos+phaseOffset, wave);
    currentPhaseR = getBlendedPhase(sinePos-phaseOffset, wave);

    /* logarithmic */
    cutoffL = std::exp((std::log(range)-std::log(500))*currentPhaseL+std::log(500));
    cutoffR = std::exp((std::log(range)-std::log(500))*currentPhaseR+std::log(500));

    //output filtered signal
    filterL.recalc(cutoffL, reso*4, d_getSampleRate(), drive);
    filterR.recalc(cutoffR, reso*4, d_getSampleRate(), drive);
    filterL.process(frames, inputs[0], outputs[0]);
    filterR.process(frames, inputs[1], outputs[1]);
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new WobbleJuicePlugin();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
