/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#include "DistrhoPluginInternal.h"

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// Static data

uint32_t d_lastBufferSize = 0;
double   d_lastSampleRate = 0.0;

// -------------------------------------------------
// Static, fallback data

const d_string        PluginInternal::fallbackString;
const ParameterRanges PluginInternal::fallbackRanges;

// -------------------------------------------------
// Plugin

Plugin::Plugin(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount)
{
    data = new PluginPrivateData;

    if (parameterCount > 0)
    {
        data->parameterCount = parameterCount;
        data->parameters = new Parameter [parameterCount];
    }

    if (programCount > 0)
    {
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        data->programCount = programCount;
        data->programNames = new d_string [programCount];
#endif
    }

    if (stateCount > 0)
    {
#if DISTRHO_PLUGIN_WANT_STATE
        data->stateCount = stateCount;
        data->stateKeys  = new d_string [stateCount];
#endif
    }
}

Plugin::~Plugin()
{
    delete data;
}

// -------------------------------------------------
// Host state

uint32_t Plugin::d_bufferSize() const
{
    return data->bufferSize;
}

double Plugin::d_sampleRate() const
{
    return data->sampleRate;
}

const TimePos& Plugin::d_timePos() const
{
    return data->timePos;
}

#if DISTRHO_PLUGIN_WANT_LATENCY
void Plugin::d_setLatency(uint32_t samples)
{
    data->latency = samples;
}
#endif

// -------------------------------------------------
// Callbacks

void Plugin::d_bufferSizeChanged(uint32_t)
{
}

void Plugin::d_sampleRateChanged(double)
{
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO
