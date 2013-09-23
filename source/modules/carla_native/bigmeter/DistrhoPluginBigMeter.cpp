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
    : Plugin(3, 0, 0), // 3 parameters, 0 programs, 0 states
      fColor(0),
      fOutLeft(0.0f),
      fOutRight(0.0f)
{
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
    case 1:
        parameter.hints  = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_OUTPUT;
        parameter.name   = "Out Left";
        parameter.symbol = "outleft";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case 2:
        parameter.hints  = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_OUTPUT;
        parameter.name   = "Out Right";
        parameter.symbol = "outright";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    default:
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
    case 1:
        return fOutLeft;
    case 2:
        return fOutRight;
    default:
        return 0.0f;
    }
}

void DistrhoPluginBigMeter::d_setParameterValue(uint32_t index, float value)
{
    switch (index)
    {
    case 0:
        fColor = (int)value;
        break;
    case 1:
        fOutLeft = value;
        break;
    case 2:
        fOutRight = value;
        break;
    default:
        break;
    }
}

// -----------------------------------------------------------------------
// Process

void DistrhoPluginBigMeter::d_run(float** inputs, float**, uint32_t frames, const MidiEvent*, uint32_t)
{
    float tmp, tmpLeft, tmpRight;
    tmpLeft = tmpRight = 0.0f;

    for (uint32_t i=0; i < frames; ++i)
    {
        tmp = std::abs(inputs[0][i]);

        if (tmp > tmpLeft)
            tmpLeft = tmp;

        tmp = std::abs(inputs[1][i]);

        if (tmp > tmpRight)
            tmpRight = tmp;
    }

    fOutLeft  = tmpLeft;
    fOutRight = tmpRight;
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new DistrhoPluginBigMeter();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
