/*
 * DISTRHO Notes Plugin
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoPluginNotes.hpp"

#include <cmath>

START_NAMESPACE_DISTRHO

// -------------------------------------------------

DistrhoPluginNotes::DistrhoPluginNotes()
    : Plugin(1, 0, 103) // 1 parameter, 0 programs, 103 states
{
    fCurPage = 0;
}

DistrhoPluginNotes::~DistrhoPluginNotes()
{
}

// -------------------------------------------------
// Init

void DistrhoPluginNotes::d_initParameter(uint32_t index, Parameter& parameter)
{
    if (index != 0)
        return;

    parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_INTEGER;
    parameter.name       = "Page";
    parameter.symbol     = "page";
    parameter.ranges.def = 1;
    parameter.ranges.min = 1;
    parameter.ranges.max = 100;
    parameter.ranges.step = 1;
    parameter.ranges.stepSmall = 1;
    parameter.ranges.stepLarge = 10;
}

void DistrhoPluginNotes::d_initStateKey(uint32_t index, d_string& stateKey)
{
    switch (index)
    {
    case 0:
        stateKey = "readOnly";
        break;
    case 1 ... 100:
        stateKey = "pageText #" + d_string(index);
        break;
    case 101:
        stateKey = "guiWidth";
        break;
    case 102:
        stateKey = "guiHeight";
        break;
    }
}

// -------------------------------------------------
// Internal data

float DistrhoPluginNotes::d_parameterValue(uint32_t index)
{
    if (index != 0)
        return 0.0f;

    return fCurPage;
}

void DistrhoPluginNotes::d_setParameterValue(uint32_t index, float value)
{
    if (index != 0)
        return;

    fCurPage = int(value);
}

void DistrhoPluginNotes::d_setState(const char*, const char*)
{
    // do nothing, used only for UI state
}

// -------------------------------------------------
// Process

void DistrhoPluginNotes::d_run(float** inputs, float** outputs, uint32_t frames, uint32_t, const MidiEvent*)
{
    float* in1  = inputs[0];
    float* in2  = inputs[1];
    float* out1 = outputs[0];
    float* out2 = outputs[1];

    std::memcpy(out1, in1, sizeof(float)*frames);
    std::memcpy(out2, in2, sizeof(float)*frames);
}

// -------------------------------------------------

Plugin* createPlugin()
{
    return new DistrhoPluginNotes();
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO
