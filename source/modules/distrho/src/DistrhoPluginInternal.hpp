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

#ifndef __DISTRHO_PLUGIN_INTERNAL_HPP__
#define __DISTRHO_PLUGIN_INTERNAL_HPP__

#include "../DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

#define MAX_MIDI_EVENTS 512

extern uint32_t d_lastBufferSize;
extern double   d_lastSampleRate;

struct Plugin::PrivateData {
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

#if DISTRHO_PLUGIN_WANT_TIMEPOS
    TimePos timePos;
#endif

    char _d; // dummy

    PrivateData()
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
          _d(0)
    {
        assert(bufferSize != 0);
        assert(sampleRate != 0.0);
    }

    ~PrivateData()
    {
        if (parameterCount > 0 && parameters != nullptr)
        {
            delete[] parameters;
            parameters = nullptr;
        }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (programCount > 0 && programNames != nullptr)
        {
            delete[] programNames;
            programNames = nullptr;
        }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        if (stateCount > 0 && stateKeys != nullptr)
        {
            delete[] stateKeys;
            stateKeys = nullptr;
        }
#endif
    }
};

// -------------------------------------------------

class PluginInternal
{
public:
    PluginInternal()
        : kPlugin(createPlugin()),
          kData((kPlugin != nullptr) ? kPlugin->pData : nullptr)
    {
        assert(kPlugin != nullptr);

        if (kPlugin == nullptr)
            return;

        for (uint32_t i=0, count=kData->parameterCount; i < count; ++i)
            kPlugin->d_initParameter(i, kData->parameters[i]);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        for (uint32_t i=0, count=kData->programCount; i < count; ++i)
            kPlugin->d_initProgramName(i, kData->programNames[i]);
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        for (uint32_t i=0, count=kData->stateCount; i < count; ++i)
            kPlugin->d_initStateKey(i, kData->stateKeys[i]);
#endif
    }

    ~PluginInternal()
    {
        if (kPlugin != nullptr)
            delete kPlugin;
    }

    // ---------------------------------------------

    const char* name() const
    {
        assert(kPlugin != nullptr);
        return (kPlugin != nullptr) ? kPlugin->d_name() : "";
    }

    const char* label() const
    {
        assert(kPlugin != nullptr);
        return (kPlugin != nullptr) ? kPlugin->d_label() : "";
    }

    const char* maker() const
    {
        assert(kPlugin != nullptr);
        return (kPlugin != nullptr) ? kPlugin->d_maker() : "";
    }

    const char* license() const
    {
        assert(kPlugin != nullptr);
        return (kPlugin != nullptr) ? kPlugin->d_license() : "";
    }

    uint32_t version() const
    {
        assert(kPlugin != nullptr);
        return (kPlugin != nullptr) ? kPlugin->d_version() : 1000;
    }

    long uniqueId() const
    {
        assert(kPlugin != nullptr);
        return (kPlugin != nullptr) ? kPlugin->d_uniqueId() : 0;
    }

    // ---------------------------------------------

#if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t latency() const
    {
        assert(kData != nullptr);
        return (kData != nullptr) ? kData->latency : 0;
    }
#endif

    uint32_t parameterCount() const
    {
        assert(kData != nullptr);
        return (kData != nullptr) ? kData->parameterCount : 0;
    }

    uint32_t parameterHints(const uint32_t index) const
    {
        assert(kData != nullptr && index < kData->parameterCount);
        return (kData != nullptr && index < kData->parameterCount) ? kData->parameters[index].hints : 0x0;
    }

    bool parameterIsOutput(const uint32_t index) const
    {
        return (parameterHints(index) & PARAMETER_IS_OUTPUT);
    }

    const d_string& parameterName(const uint32_t index) const
    {
        assert(kData != nullptr && index < kData->parameterCount);
        return (kData != nullptr && index < kData->parameterCount) ? kData->parameters[index].name : sFallbackString;
    }

    const d_string& parameterSymbol(const uint32_t index) const
    {
        assert(kData != nullptr && index < kData->parameterCount);
        return (kData != nullptr && index < kData->parameterCount) ? kData->parameters[index].symbol : sFallbackString;
    }

    const d_string& parameterUnit(const uint32_t index) const
    {
        assert(kData != nullptr && index < kData->parameterCount);
        return (kData != nullptr && index < kData->parameterCount) ? kData->parameters[index].unit : sFallbackString;
    }

    const ParameterRanges& parameterRanges(const uint32_t index) const
    {
        assert(kData != nullptr && index < kData->parameterCount);
        return (kData != nullptr && index < kData->parameterCount) ? kData->parameters[index].ranges : sFallbackRanges;
    }

    float parameterValue(const uint32_t index)
    {
        assert(kPlugin != nullptr && index < kData->parameterCount);
        return (kPlugin != nullptr && index < kData->parameterCount) ? kPlugin->d_parameterValue(index) : 0.0f;
    }

    void setParameterValue(const uint32_t index, const float value)
    {
        assert(kPlugin != nullptr && index < kData->parameterCount);

        if (kPlugin != nullptr && index < kData->parameterCount)
            kPlugin->d_setParameterValue(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t programCount() const
    {
        assert(kData != nullptr);
        return (kData != nullptr) ? kData->programCount : 0;
    }

    const d_string& programName(const uint32_t index) const
    {
        assert(kData != nullptr && index < kData->programCount);
        return (kData != nullptr && index < kData->programCount) ? kData->programNames[index] : sFallbackString;
    }

    void setProgram(const uint32_t index)
    {
        assert(kPlugin != nullptr && index < kData->programCount);

        if (kPlugin != nullptr && index < kData->programCount)
            kPlugin->d_setProgram(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    uint32_t stateCount() const
    {
        assert(kData != nullptr);
        return kData != nullptr ? kData->stateCount : 0;
    }

    const d_string& stateKey(const uint32_t index) const
    {
        assert(kData != nullptr && index < kData->stateCount);
        return (kData != nullptr && index < kData->stateCount) ? kData->stateKeys[index] : sFallbackString;
    }

    void setState(const char* const key, const char* const value)
    {
        assert(kPlugin != nullptr && key != nullptr && value != nullptr);

        if (kPlugin != nullptr && key != nullptr && value != nullptr)
            kPlugin->d_setState(key, value);
    }
#endif

    // ---------------------------------------------

    void activate()
    {
        assert(kPlugin != nullptr);

        if (kPlugin != nullptr)
            kPlugin->d_activate();
    }

    void deactivate()
    {
        assert(kPlugin != nullptr);

        if (kPlugin != nullptr)
            kPlugin->d_deactivate();
    }

    void run(float** const inputs, float** const outputs, const uint32_t frames, const uint32_t midiEventCount, const MidiEvent* const midiEvents)
    {
        assert(kPlugin != nullptr);

        if (kPlugin != nullptr)
            kPlugin->d_run(inputs, outputs, frames, midiEventCount, midiEvents);
    }

    // ---------------------------------------------

    void setBufferSize(const uint32_t bufferSize, bool doCallback = false)
    {
        assert(kData != nullptr && kPlugin != nullptr && bufferSize >= 2);

        if (kData != nullptr)
        {
            if (doCallback && kData->bufferSize == bufferSize)
                doCallback = false;

            kData->bufferSize = bufferSize;
        }

        if (kPlugin != nullptr && doCallback)
        {
            kPlugin->d_deactivate();
            kPlugin->d_bufferSizeChanged(bufferSize);
            kPlugin->d_activate();
        }
    }

    void setSampleRate(const double sampleRate, bool doCallback = false)
    {
        assert(kData != nullptr && kPlugin != nullptr && sampleRate > 0.0);

        if (kData != nullptr)
        {
            if (doCallback && kData->sampleRate == sampleRate)
                doCallback = false;

            kData->sampleRate = sampleRate;
        }

        if (kPlugin != nullptr && doCallback)
        {
            kPlugin->d_deactivate();
            kPlugin->d_sampleRateChanged(sampleRate);
            kPlugin->d_activate();
        }
    }

    // ---------------------------------------------

protected:
    Plugin* const kPlugin;
    Plugin::PrivateData* const kData;

private:
    static const d_string        sFallbackString;
    static const ParameterRanges sFallbackRanges;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_PLUGIN_INTERNAL_HPP__
