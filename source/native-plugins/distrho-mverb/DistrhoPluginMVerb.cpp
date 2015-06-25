/*
 * DISTRHO MVerb, a DPF'ied MVerb.
 * Copyright (c) 2010 Martin Eastwood
 * Copyright (C) 2015 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the LICENSE file.
 */

#include "DistrhoPluginMVerb.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

DistrhoPluginMVerb::DistrhoPluginMVerb()
    : Plugin(MVerb<float>::NUM_PARAMS, 5, 0) // 5 program, 0 states
{
    fVerb.setSampleRate(getSampleRate());

    // set initial values
    loadProgram(0);
}

// -----------------------------------------------------------------------
// Init

void DistrhoPluginMVerb::initParameter(uint32_t index, Parameter& parameter)
{
    parameter.unit       = "%";
    parameter.ranges.min = 0.0f;
    parameter.ranges.max = 100.0f;

    // default values taken from 1st preset
    switch (index)
    {
    case MVerb<float>::DAMPINGFREQ:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "Damping";
        parameter.symbol     = "damping";
        parameter.ranges.def = 0.5f * 100.0f;
        break;
    case MVerb<float>::DENSITY:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "Density";
        parameter.symbol     = "density";
        parameter.ranges.def = 0.5f * 100.0f;
        break;
    case MVerb<float>::BANDWIDTHFREQ:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "Bandwidth";
        parameter.symbol     = "bandwidth";
        parameter.ranges.def = 0.5f * 100.0f;
        break;
    case MVerb<float>::DECAY:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "Decay";
        parameter.symbol     = "decay";
        parameter.ranges.def = 0.5f * 100.0f;
        break;
    case MVerb<float>::PREDELAY:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "Predelay";
        parameter.symbol     = "predelay";
        parameter.ranges.def = 0.5f * 100.0f;
        break;
    case MVerb<float>::SIZE:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "Size";
        parameter.symbol     = "size";
        parameter.ranges.def = 0.75f * 100.0f;
        parameter.ranges.min = 0.05f * 100.0f;
        break;
    case MVerb<float>::GAIN:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "Gain";
        parameter.symbol     = "gain";
        parameter.ranges.def = 1.0f * 100.0f;
        break;
    case MVerb<float>::MIX:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "Mix";
        parameter.symbol     = "mix";
        parameter.ranges.def = 0.5f * 100.0f;
        break;
    case MVerb<float>::EARLYMIX:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "Early/Late Mix";
        parameter.symbol     = "earlymix";
        parameter.ranges.def = 0.5f * 100.0f;
        break;
    }
}

void DistrhoPluginMVerb::initProgramName(uint32_t index, String& programName)
{
    switch(index)
    {
    case 0:
        programName = "Halves";
        break;
    case 1:
        programName = "Dark";
        break;
    case 2:
        programName = "Cupboard";
        break;
    case 3:
        programName = "Stadium";
        break;
    case 4:
        programName = "Subtle";
        break;
    }
}

// -----------------------------------------------------------------------
// Internal data

float DistrhoPluginMVerb::getParameterValue(uint32_t index) const
{
    return fVerb.getParameter(static_cast<int>(index)) * 100.0f;
}

void DistrhoPluginMVerb::setParameterValue(uint32_t index, float value)
{
    fVerb.setParameter(static_cast<int>(index), value / 100.0f);
}

void DistrhoPluginMVerb::loadProgram(uint32_t index)
{
    // NOTE: DAMPINGFREQ is reversed

    switch(index)
    {
    case 0:
        fVerb.setParameter(MVerb<float>::DAMPINGFREQ, 0.5f);
        fVerb.setParameter(MVerb<float>::DENSITY, 0.5f);
        fVerb.setParameter(MVerb<float>::BANDWIDTHFREQ, 0.5f);
        fVerb.setParameter(MVerb<float>::DECAY, 0.5f);
        fVerb.setParameter(MVerb<float>::PREDELAY, 0.5f);
        fVerb.setParameter(MVerb<float>::SIZE, 0.75f);
        fVerb.setParameter(MVerb<float>::GAIN, 1.0f);
        fVerb.setParameter(MVerb<float>::MIX, 0.5f);
        fVerb.setParameter(MVerb<float>::EARLYMIX, 0.5f);
        break;
    case 1:
        fVerb.setParameter(MVerb<float>::DAMPINGFREQ, 0.1f);
        fVerb.setParameter(MVerb<float>::DENSITY, 0.5f);
        fVerb.setParameter(MVerb<float>::BANDWIDTHFREQ, 0.1f);
        fVerb.setParameter(MVerb<float>::DECAY, 0.5f);
        fVerb.setParameter(MVerb<float>::PREDELAY, 0.0f);
        fVerb.setParameter(MVerb<float>::SIZE, 0.5f);
        fVerb.setParameter(MVerb<float>::GAIN, 1.0f);
        fVerb.setParameter(MVerb<float>::MIX, 0.5f);
        fVerb.setParameter(MVerb<float>::EARLYMIX, 0.75f);
        break;
    case 2:
        fVerb.setParameter(MVerb<float>::DAMPINGFREQ, 1.0f);
        fVerb.setParameter(MVerb<float>::DENSITY, 0.5f);
        fVerb.setParameter(MVerb<float>::BANDWIDTHFREQ, 1.0f);
        fVerb.setParameter(MVerb<float>::DECAY, 0.5f);
        fVerb.setParameter(MVerb<float>::PREDELAY, 0.0f);
        fVerb.setParameter(MVerb<float>::SIZE, 0.25f);
        fVerb.setParameter(MVerb<float>::GAIN, 1.0f);
        fVerb.setParameter(MVerb<float>::MIX, 0.35f);
        fVerb.setParameter(MVerb<float>::EARLYMIX, 0.75f);
        break;
    case 3:
        fVerb.setParameter(MVerb<float>::DAMPINGFREQ, 1.0f);
        fVerb.setParameter(MVerb<float>::DENSITY, 0.5f);
        fVerb.setParameter(MVerb<float>::BANDWIDTHFREQ, 1.0f);
        fVerb.setParameter(MVerb<float>::DECAY, 0.5f);
        fVerb.setParameter(MVerb<float>::PREDELAY, 0.0f);
        fVerb.setParameter(MVerb<float>::SIZE, 1.0f);
        fVerb.setParameter(MVerb<float>::GAIN, 1.0f);
        fVerb.setParameter(MVerb<float>::MIX, 0.35f);
        fVerb.setParameter(MVerb<float>::EARLYMIX, 0.75f);
        break;
    case 4:
        fVerb.setParameter(MVerb<float>::DAMPINGFREQ, 1.0f);
        fVerb.setParameter(MVerb<float>::DENSITY, 0.5f);
        fVerb.setParameter(MVerb<float>::BANDWIDTHFREQ, 1.0f);
        fVerb.setParameter(MVerb<float>::DECAY, 0.5f);
        fVerb.setParameter(MVerb<float>::PREDELAY, 0.0f);
        fVerb.setParameter(MVerb<float>::SIZE, 0.5f);
        fVerb.setParameter(MVerb<float>::GAIN, 1.0f);
        fVerb.setParameter(MVerb<float>::MIX, 0.15f);
        fVerb.setParameter(MVerb<float>::EARLYMIX, 0.75f);
        break;
    }

    fVerb.reset();
}

// -----------------------------------------------------------------------
// Process

void DistrhoPluginMVerb::activate()
{
    fVerb.reset();
}

void DistrhoPluginMVerb::run(const float** inputs, float** outputs, uint32_t frames)
{
    fVerb.process(inputs, outputs, static_cast<int>(frames));
}

// -----------------------------------------------------------------------
// Callbacks

void DistrhoPluginMVerb::sampleRateChanged(double newSampleRate)
{
    fVerb.setSampleRate(newSampleRate);
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new DistrhoPluginMVerb();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
