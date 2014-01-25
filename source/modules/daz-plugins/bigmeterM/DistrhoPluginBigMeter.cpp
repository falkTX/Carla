/*
 * DISTRHO BigMeter Plugin
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

#include "DistrhoPluginBigMeter.hpp"

#include <cmath>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

DistrhoPluginBigMeter::DistrhoPluginBigMeter()
    : Plugin(1+DISTRHO_PLUGIN_NUM_INPUTS, 0, 0), // many parameters, 0 programs, 0 states
      fColor(0)
{
    std::memset(fOuts, 0, sizeof(float)*DISTRHO_PLUGIN_NUM_INPUTS);
}

DistrhoPluginBigMeter::~DistrhoPluginBigMeter()
{
}

// -----------------------------------------------------------------------
// Init

void DistrhoPluginBigMeter::d_initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
    case 0:
        parameter.hints  = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_INTEGER;
        parameter.name   = "Color";
        parameter.symbol = "color";
        parameter.ranges.def = 0;
        parameter.ranges.min = 0;
        parameter.ranges.max = 1;
        parameter.ranges.step = 1;
        parameter.ranges.stepSmall = 1;
        parameter.ranges.stepLarge = 1;
        break;
    default:
        parameter.hints  = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_OUTPUT;
        parameter.name   = "Out " + d_string(index);
        parameter.symbol = "out"  + d_string(index);
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    }
}

// -----------------------------------------------------------------------
// Internal data

float DistrhoPluginBigMeter::d_getParameterValue(uint32_t index) const
{
    switch (index)
    {
    case 0:
        return (float)fColor;
    default:
        return fOuts[index-1];
    }
}

void DistrhoPluginBigMeter::d_setParameterValue(uint32_t index, float value)
{
    switch (index)
    {
    case 0:
        fColor = (int)value;
        break;
    default:
        fOuts[index-1] = value;
        break;
    }
}

// -----------------------------------------------------------------------
// Process

void DistrhoPluginBigMeter::d_run(float** inputs, float**, uint32_t frames, const MidiEvent*, uint32_t)
{
    float tmp, tmp32[DISTRHO_PLUGIN_NUM_INPUTS];

    std::memset(tmp32, 0, sizeof(float)*DISTRHO_PLUGIN_NUM_INPUTS);

    for (uint32_t i=0; i < frames; ++i)
    {
        for (int j=0; j < DISTRHO_PLUGIN_NUM_INPUTS; ++j)
        {
            tmp = std::abs(inputs[j][i]);

            if (tmp > tmp32[j])
                tmp32[j] = tmp;
        }
    }

    std::memcpy(fOuts, tmp32, sizeof(float)*DISTRHO_PLUGIN_NUM_INPUTS);
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new DistrhoPluginBigMeter();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
