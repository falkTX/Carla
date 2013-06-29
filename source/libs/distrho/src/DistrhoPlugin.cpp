/*
 * DISTRHO Plugin Toolkit (DPT)
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
 * For a full copy of the license see the LGPL.txt file
 */

#include "DistrhoPluginInternal.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// Static data

uint32_t d_lastBufferSize = 0;
double   d_lastSampleRate = 0.0;

// -------------------------------------------------
// Static, fallback data

const d_string        PluginInternal::sFallbackString;
const ParameterRanges PluginInternal::sFallbackRanges;

// -------------------------------------------------
// Plugin

Plugin::Plugin(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount)
    : pData(new PrivateData())
{
    if (parameterCount > 0)
    {
        pData->parameterCount = parameterCount;
        pData->parameters = new Parameter[parameterCount];
    }

    if (programCount > 0)
    {
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        pData->programCount = programCount;
        pData->programNames = new d_string[programCount];
#endif
    }

    if (stateCount > 0)
    {
#if DISTRHO_PLUGIN_WANT_STATE
        pData->stateCount = stateCount;
        pData->stateKeys  = new d_string[stateCount];
#endif
    }
}

Plugin::~Plugin()
{
    delete pData;
}

// -------------------------------------------------
// Host state

uint32_t Plugin::d_bufferSize() const
{
    return pData->bufferSize;
}

double Plugin::d_sampleRate() const
{
    return pData->sampleRate;
}

#if DISTRHO_PLUGIN_WANT_TIMEPOS
const TimePos& Plugin::d_timePos() const
{
    return pData->timePos;
}
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
void Plugin::d_setLatency(uint32_t frames)
{
    pData->latency = frames;
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
