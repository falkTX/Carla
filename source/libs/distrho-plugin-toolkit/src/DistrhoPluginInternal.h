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

#ifndef __DISTRHO_PLUGIN_INTERNAL_H__
#define __DISTRHO_PLUGIN_INTERNAL_H__

#include "DistrhoPlugin.h"

#include <cassert>

START_NAMESPACE_DISTRHO

// -------------------------------------------------

#define MAX_MIDI_EVENTS 512

extern uint32_t d_lastBufferSize;
extern double   d_lastSampleRate;

struct PluginPrivateData {
    uint32_t bufferSize;
    double   sampleRate;

    uint32_t   parameterCount;
    Parameter* parameters;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t  programCount;
    d_string* programNames;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    uint32_t  stateCount;
    d_string* stateKeys;
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t latency;
#endif
    TimePos  timePos;

    PluginPrivateData()
        : bufferSize(d_lastBufferSize),
          sampleRate(d_lastSampleRate),
          parameterCount(0),
          parameters(nullptr),
#if DISTRHO_PLUGIN_WANT_PROGRAMS
          programCount(0),
          programNames(nullptr),
#endif
#if DISTRHO_PLUGIN_WANT_STATE
          stateCount(0),
          stateKeys(nullptr),
#endif
#if DISTRHO_PLUGIN_WANT_LATENCY
          latency(0),
#endif
          timePos()
    {
        assert(d_lastBufferSize != 0);
        assert(d_lastSampleRate != 0.0);
    }

    ~PluginPrivateData()
    {
        if (parameterCount > 0 && parameters)
            delete[] parameters;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (programCount > 0 && programNames)
            delete[] programNames;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        if (stateCount > 0 && stateKeys)
            delete[] stateKeys;
#endif
    }
};

// -------------------------------------------------

class PluginInternal
{
public:
    PluginInternal()
        : plugin(createPlugin()),
          data(plugin ? plugin->data : nullptr)
    {
        assert(plugin);

        if (! plugin)
            return;

        for (uint32_t i=0, count=data->parameterCount; i < count; i++)
            plugin->d_initParameter(i, data->parameters[i]);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        for (uint32_t i=0, count=data->programCount; i < count; i++)
            plugin->d_initProgramName(i, data->programNames[i]);
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        for (uint32_t i=0, count=data->stateCount; i < count; i++)
            plugin->d_initStateKey(i, data->stateKeys[i]);
#endif
    }

    ~PluginInternal()
    {
        if (plugin)
            delete plugin;
    }

    // ---------------------------------------------

    const char* name() const
    {
        assert(plugin);
        return plugin ? plugin->d_name() : "";
    }

    const char* label() const
    {
        assert(plugin);
        return plugin ? plugin->d_label() : "";
    }

    const char* maker() const
    {
        assert(plugin);
        return plugin ? plugin->d_maker() : "";
    }

    const char* license() const
    {
        assert(plugin);
        return plugin ? plugin->d_license() : "";
    }

    uint32_t version() const
    {
        assert(plugin);
        return plugin ? plugin->d_version() : 1000;
    }

    long uniqueId() const
    {
        assert(plugin);
        return plugin ? plugin->d_uniqueId() : 0;
    }

    // ---------------------------------------------

#if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t latency() const
    {
        assert(data);
        return data ? data->latency : 0;
    }
#endif

    uint32_t parameterCount() const
    {
        assert(data);
        return data ? data->parameterCount : 0;
    }

    uint32_t parameterHints(uint32_t index) const
    {
        assert(data && index < data->parameterCount);
        return (data && index < data->parameterCount) ? data->parameters[index].hints : 0x0;
    }

    bool parameterIsOutput(uint32_t index) const
    {
        uint32_t hints = parameterHints(index);
        return (hints & PARAMETER_IS_OUTPUT);
    }

    const d_string& parameterName(uint32_t index) const
    {
        assert(data && index < data->parameterCount);
        return (data && index < data->parameterCount) ? data->parameters[index].name : fallbackString;
    }

    const d_string& parameterSymbol(uint32_t index) const
    {
        assert(data && index < data->parameterCount);
        return (data && index < data->parameterCount) ? data->parameters[index].symbol : fallbackString;
    }

    const d_string& parameterUnit(uint32_t index) const
    {
        assert(data && index < data->parameterCount);
        return (data && index < data->parameterCount) ? data->parameters[index].unit : fallbackString;
    }

    const ParameterRanges& parameterRanges(uint32_t index) const
    {
        assert(data && index < data->parameterCount);
        return (data && index < data->parameterCount) ? data->parameters[index].ranges : fallbackRanges;
    }

    float parameterValue(uint32_t index)
    {
        assert(plugin && index < data->parameterCount);
        return (plugin && index < data->parameterCount) ? plugin->d_parameterValue(index) : 0.0f;
    }

    void setParameterValue(uint32_t index, float value)
    {
        assert(plugin && index < data->parameterCount);

        if (plugin && index < data->parameterCount)
            plugin->d_setParameterValue(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t programCount() const
    {
        assert(data);
        return data ? data->programCount : 0;
    }

    const d_string& programName(uint32_t index) const
    {
        assert(data && index < data->programCount);
        return (data && index < data->programCount) ? data->programNames[index] : fallbackString;
    }

    void setProgram(uint32_t index)
    {
        assert(plugin && index < data->programCount);

        if (plugin && index < data->programCount)
            plugin->d_setProgram(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    uint32_t stateCount() const
    {
        assert(data);
        return data ? data->stateCount : 0;
    }

    const d_string& stateKey(uint32_t index) const
    {
        assert(data && index < data->stateCount);
        return (data && index < data->stateCount) ? data->stateKeys[index] : fallbackString;
    }

    void setState(const char* key, const char* value)
    {
        assert(plugin && key && value);

        if (plugin && key && value)
            plugin->d_setState(key, value);
    }
#endif

    // ---------------------------------------------

    void activate()
    {
        assert(plugin);

        if (plugin)
            plugin->d_activate();
    }

    void deactivate()
    {
        assert(plugin);

        if (plugin)
            plugin->d_deactivate();
    }

    void run(float** inputs, float** outputs, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents)
    {
        assert(plugin);

        if (plugin)
            plugin->d_run(inputs, outputs, frames, midiEventCount, midiEvents);
    }

    // ---------------------------------------------

    void setBufferSize(uint32_t bufferSize, bool callback = false)
    {
        assert(data && plugin && bufferSize >= 2);

        if (data)
        {
            data->bufferSize = bufferSize;

            if (callback && data->bufferSize == bufferSize)
                callback = false;
        }

        if (plugin && callback)
        {
            plugin->d_deactivate();
            plugin->d_bufferSizeChanged(bufferSize);
            plugin->d_activate();
        }
    }

    void setSampleRate(double sampleRate, bool callback = false)
    {
        assert(data && plugin && sampleRate > 0.0);

        if (data)
        {
            data->sampleRate = sampleRate;

            if (callback && data->sampleRate == sampleRate)
                callback = false;
        }

        if (plugin && callback)
        {
            plugin->d_deactivate();
            plugin->d_sampleRateChanged(sampleRate);
            plugin->d_activate();
        }
    }

    // ---------------------------------------------

protected:
    Plugin* const plugin;
    PluginPrivateData* const data;

private:
    static const d_string        fallbackString;
    static const ParameterRanges fallbackRanges;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_PLUGIN_INTERNAL_H__
