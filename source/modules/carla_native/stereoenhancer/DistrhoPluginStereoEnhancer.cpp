/*
 * DISTRHO StereoEnhancer Plugin, based on StereoEnhancer by Michael Gruhn
 * Copyright (C) 2007 Michael Gruhn <michael-gruhn@web.de>
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the license see the doc/LGPL.txt file.
 */

#include "DistrhoPluginStereoEnhancer.hpp"

#include <cmath>

static const float kDC_ADD = 1e-30f;
static const float kPI     = 3.141592654f;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

DistrhoPluginStereoEnhancer::DistrhoPluginStereoEnhancer()
    : Plugin(paramCount, 1, 0) // 1 program, 0 states
{
    // set default values
    d_setProgram(0);

    // reset
    d_deactivate();
}

DistrhoPluginStereoEnhancer::~DistrhoPluginStereoEnhancer()
{
}

// -----------------------------------------------------------------------
// Init

void DistrhoPluginStereoEnhancer::d_initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
    case paramWidthLows:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Width Lows";
        parameter.symbol     = "width_lows";
        parameter.unit       = "%";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 100.0f;
        parameter.ranges.max = 200.0f;
        break;

    case paramWidthHighs:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "WidthHighs";
        parameter.symbol     = "width_highs";
        parameter.unit       = "%";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 100.0f;
        parameter.ranges.max = 200.0f;
        break;

    case paramCrossover:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Crossover";
        parameter.symbol     = "crossover";
        parameter.unit       = "%";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 27.51604f;
        parameter.ranges.max = 100.0f;
        break;
    }
}

void DistrhoPluginStereoEnhancer::d_initProgramName(uint32_t index, d_string& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

// -----------------------------------------------------------------------
// Internal data

float DistrhoPluginStereoEnhancer::d_getParameterValue(uint32_t index) const
{
    switch (index)
    {
    case paramWidthLows:
        return widthLP;
    case paramWidthHighs:
        return widthHP;
    case paramCrossover:
        return freqHP;
    default:
        return 0.0f;
    }
}

void DistrhoPluginStereoEnhancer::d_setParameterValue(uint32_t index, float value)
{
    if (d_getSampleRate() <= 0.0)
        return;

    switch (index)
    {
    case paramWidthLows:
        widthLP = value;
        widthCoeffLP = std::fmax(widthLP, 1.0f);
        break;
    case paramWidthHighs:
        widthHP = value;
        widthCoeffHP = std::fmax(widthHP, 1.0f);
        break;
    case paramCrossover:
        freqHPFader = value;
        freqHP = freqHPFader*freqHPFader*freqHPFader*24000.0f;
        xHP  = std::exp(-2.0f * kPI * freqHP / (float)d_getSampleRate());
        a0HP = 1.0f-xHP;
        b1HP = -xHP;
        break;
    }
}

void DistrhoPluginStereoEnhancer::d_setProgram(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    widthLP = 100.0f;
    widthHP = 100.0f;
    freqHPFader = 27.51604f;

    // Internal stuff
    widthCoeffLP = 1.0f;
    widthCoeffHP = 1.0f;
    freqHP = 500.0f;

    // reset filter values
    d_activate();
}

// -----------------------------------------------------------------------
// Process

void DistrhoPluginStereoEnhancer::d_activate()
{
    xHP  = std::exp(-2.0f * kPI * freqHP/ (float)d_getSampleRate());
    a0HP = 1.0f-xHP;
    b1HP = -xHP;
}

void DistrhoPluginStereoEnhancer::d_deactivate()
{
    out1HP = out2HP = 0.0f;
    tmp1HP = tmp2HP = 0.0f;
}

void DistrhoPluginStereoEnhancer::d_run(float** inputs, float** outputs, uint32_t frames, const MidiEvent*, uint32_t)
{
    float* in1  = inputs[0];
    float* in2  = inputs[1];
    float* out1 = outputs[0];
    float* out2 = outputs[1];

    for (uint32_t i=0; i < frames; ++i)
    {
        out1HP = in1[i];
        out2HP = in2[i];

        in1[i] = (tmp1HP = a0HP * in1[i] - b1HP * tmp1HP + kDC_ADD);
        in2[i] = (tmp2HP = a0HP * in2[i] - b1HP * tmp2HP + kDC_ADD);

        out1HP -= in1[i] - kDC_ADD;
        out2HP -= in2[i] - kDC_ADD;

        monoLP   = (in1[i] + in2[i]) / 2.0f;
        stereoLP = in1[i] - in2[i];
        (*in1) = (monoLP + stereoLP * widthLP) / widthCoeffLP;
        (*in2) = (monoLP - stereoLP * widthLP) / widthCoeffLP;

        monoHP   = (out1HP + out2HP) / 2.0f;
        stereoHP = out1HP - out2HP;

        out1HP = (monoHP + stereoHP * widthHP) / widthCoeffHP;
        out2HP = (monoHP - stereoHP * widthHP) / widthCoeffHP;

        out1[i] = in1[i] + out1HP;
        out2[i] = in2[i] + out2HP;
    }
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new DistrhoPluginStereoEnhancer();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
