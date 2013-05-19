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

#ifndef __DISTRHO_PLUGIN_HPP__
#define __DISTRHO_PLUGIN_HPP__

#include "DistrhoUtils.hpp"

#include <cstdint>

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// Parameter Hints

const uint32_t PARAMETER_IS_AUTOMABLE   = 1 << 0;
const uint32_t PARAMETER_IS_BOOLEAN     = 1 << 1;
const uint32_t PARAMETER_IS_INTEGER     = 1 << 2;
const uint32_t PARAMETER_IS_LOGARITHMIC = 1 << 3;
const uint32_t PARAMETER_IS_OUTPUT      = 1 << 4;

// -------------------------------------------------
// Parameter Ranges

struct ParameterRanges {
    float def;
    float min;
    float max;
    float step;
    float stepSmall;
    float stepLarge;

    ParameterRanges()
        : def(0.0f),
          min(0.0f),
          max(1.0f),
          step(0.001f),
          stepSmall(0.00001f),
          stepLarge(0.01f) {}

    ParameterRanges(float def, float min, float max)
        : step(0.001f),
          stepSmall(0.00001f),
          stepLarge(0.01f)
    {
        this->def = def;
        this->min = min;
        this->max = max;
    }

    ParameterRanges(float def, float min, float max, float step, float stepSmall, float stepLarge)
    {
        this->def = def;
        this->min = min;
        this->max = max;
        this->step = step;
        this->stepSmall = stepSmall;
        this->stepLarge = stepLarge;
    }

    void fixValue(float& value) const
    {
        if (value < min)
            value = min;
        else if (value > max)
            value = max;
    }

    float fixValue(const float& value) const
    {
        if (value < min)
            return min;
        else if (value > max)
            return max;
        return value;
    }

    float normalizeValue(const float& value) const
    {
        float newValue = (value - min) / (max - min);

        if (newValue < 0.0f)
            newValue = 0.0f;
        else if (newValue > 1.0f)
            newValue = 1.0f;

        return newValue;
    }

    float unnormalizeValue(const float& value) const
    {
        return value * (max - min) + min;
    }
};

// -------------------------------------------------
// Parameter

struct Parameter {
    uint32_t hints;
    d_string name;
    d_string symbol;
    d_string unit;
    ParameterRanges ranges;

    Parameter()
        : hints(0x0) {}
};

// -------------------------------------------------
// MidiEvent

struct MidiEvent {
    uint32_t frame;
    uint8_t  buf[4];
    uint8_t  size;

    MidiEvent()
    {
        clear();
    }

    void clear()
    {
        frame  = 0;
        buf[0] = 0;
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
        size   = 0;
    }
};

// -------------------------------------------------
// TimePos

struct TimePos {
    bool playing;
    uint64_t frame;
    double bpm;

    TimePos()
        : playing(false),
          frame(0),
          bpm(120.0) {}
};

// -------------------------------------------------
// Plugin

class Plugin
{
public:
    Plugin(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount);
    virtual ~Plugin();

    // ---------------------------------------------
    // Host state

    uint32_t       d_bufferSize() const;
    double         d_sampleRate() const;
    const TimePos& d_timePos()    const;
#if DISTRHO_PLUGIN_WANT_LATENCY
    void           d_setLatency(uint32_t frames);
#endif

protected:
    // ---------------------------------------------
    // Information

    virtual const char* d_name() const { return DISTRHO_PLUGIN_NAME; }
    virtual const char* d_label() const = 0;
    virtual const char* d_maker() const = 0;
    virtual const char* d_license() const = 0;
    virtual uint32_t    d_version() const = 0;
    virtual long        d_uniqueId() const = 0;

    // ---------------------------------------------
    // Init

    virtual void d_initParameter(uint32_t index, Parameter& parameter) = 0;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    virtual void d_initProgramName(uint32_t index, d_string& programName) = 0;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    virtual void d_initStateKey(uint32_t index, d_string& stateKey) = 0;
#endif

    // ---------------------------------------------
    // Internal data

    virtual float d_parameterValue(uint32_t index) = 0;
    virtual void  d_setParameterValue(uint32_t index, float value) = 0;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    virtual void  d_setProgram(uint32_t index) = 0;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    virtual void  d_setState(const char* key, const char* value) = 0;
#endif

    // ---------------------------------------------
    // Process

    virtual void d_activate() {}
    virtual void d_deactivate() {}
    virtual void d_run(float** inputs, float** outputs, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents) = 0;

    // ---------------------------------------------
    // Callbacks (optional)

    virtual void d_bufferSizeChanged(uint32_t newBufferSize);
    virtual void d_sampleRateChanged(double   newSampleRate);

    // ---------------------------------------------

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class PluginInternal;
};

// -------------------------------------------------

Plugin* createPlugin();

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_PLUGIN_HPP__
