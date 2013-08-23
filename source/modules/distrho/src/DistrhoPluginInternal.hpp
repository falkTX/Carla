/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_PLUGIN_INTERNAL_HPP_INCLUDED
#define DISTRHO_PLUGIN_INTERNAL_HPP_INCLUDED

#include "../DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

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
        if (parameters != nullptr)
        {
            delete[] parameters;
            parameters = nullptr;
        }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (programNames != nullptr)
        {
            delete[] programNames;
            programNames = nullptr;
        }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        if (stateKeys != nullptr)
        {
            delete[] stateKeys;
            stateKeys = nullptr;
        }
#endif
    }
};

// -----------------------------------------------------------------------

class PluginInternal
{
public:
    PluginInternal()
        : fPlugin(createPlugin()),
          fData((fPlugin != nullptr) ? fPlugin->pData : nullptr)
    {
        assert(fPlugin != nullptr);

        if (fPlugin == nullptr)
            return;

        for (uint32_t i=0, count=fData->parameterCount; i < count; ++i)
            fPlugin->d_initParameter(i, fData->parameters[i]);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        for (uint32_t i=0, count=fData->programCount; i < count; ++i)
            fPlugin->d_initProgramName(i, fData->programNames[i]);
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        for (uint32_t i=0, count=fData->stateCount; i < count; ++i)
            fPlugin->d_initStateKey(i, fData->stateKeys[i]);
#endif
    }

    ~PluginInternal()
    {
        if (fPlugin != nullptr)
            delete fPlugin;
    }

    // -------------------------------------------------------------------

    const char* getName() const
    {
        assert(fPlugin != nullptr);
        return (fPlugin != nullptr) ? fPlugin->d_getName() : "";
    }

    const char* getLabel() const
    {
        assert(fPlugin != nullptr);
        return (fPlugin != nullptr) ? fPlugin->d_getLabel() : "";
    }

    const char* getMaker() const
    {
        assert(fPlugin != nullptr);
        return (fPlugin != nullptr) ? fPlugin->d_getMaker() : "";
    }

    const char* getLicense() const
    {
        assert(fPlugin != nullptr);
        return (fPlugin != nullptr) ? fPlugin->d_getLicense() : "";
    }

    uint32_t getVersion() const
    {
        assert(fPlugin != nullptr);
        return (fPlugin != nullptr) ? fPlugin->d_getVersion() : 1000;
    }

    long getUniqueId() const
    {
        assert(fPlugin != nullptr);
        return (fPlugin != nullptr) ? fPlugin->d_getUniqueId() : 0;
    }

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t getLatency() const
    {
        assert(fData != nullptr);
        return (fData != nullptr) ? fData->getLatency : 0;
    }
#endif

    uint32_t getParameterCount() const
    {
        assert(fData != nullptr);
        return (fData != nullptr) ? fData->parameterCount : 0;
    }

    uint32_t getParameterHints(const uint32_t index) const
    {
        assert(fData != nullptr && index < fData->parameterCount);
        return (fData != nullptr && index < fData->parameterCount) ? fData->parameters[index].hints : 0x0;
    }

    bool isParameterIsOutput(const uint32_t index) const
    {
        return (getParameterHints(index) & PARAMETER_IS_OUTPUT);
    }

    const d_string& getParameterName(const uint32_t index) const
    {
        assert(fData != nullptr && index < fData->parameterCount);
        return (fData != nullptr && index < fData->parameterCount) ? fData->parameters[index].name : sFallbackString;
    }

    const d_string& getParameterSymbol(const uint32_t index) const
    {
        assert(fData != nullptr && index < fData->parameterCount);
        return (fData != nullptr && index < fData->parameterCount) ? fData->parameters[index].symbol : sFallbackString;
    }

    const d_string& getParameterUnit(const uint32_t index) const
    {
        assert(fData != nullptr && index < fData->parameterCount);
        return (fData != nullptr && index < fData->parameterCount) ? fData->parameters[index].unit : sFallbackString;
    }

    const ParameterRanges& getParameterRanges(const uint32_t index) const
    {
        assert(fData != nullptr && index < fData->parameterCount);
        return (fData != nullptr && index < fData->parameterCount) ? fData->parameters[index].ranges : sFallbackRanges;
    }

    float getParameterValue(const uint32_t index) const
    {
        assert(fPlugin != nullptr && index < fData->parameterCount);
        return (fPlugin != nullptr && index < fData->parameterCount) ? fPlugin->d_getParameterValue(index) : 0.0f;
    }

    void setParameterValue(const uint32_t index, const float value)
    {
        assert(fPlugin != nullptr && index < fData->parameterCount);

        if (fPlugin != nullptr && index < fData->parameterCount)
            fPlugin->d_setParameterValue(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t getProgramCount() const
    {
        assert(fData != nullptr);
        return (fData != nullptr) ? fData->programCount : 0;
    }

    const d_string& getProgramName(const uint32_t index) const
    {
        assert(fData != nullptr && index < fData->programCount);
        return (fData != nullptr && index < fData->programCount) ? fData->programNames[index] : sFallbackString;
    }

    void setProgram(const uint32_t index)
    {
        assert(fPlugin != nullptr && index < fData->programCount);

        if (fPlugin != nullptr && index < fData->programCount)
            fPlugin->d_setProgram(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    uint32_t getStateCount() const
    {
        assert(fData != nullptr);
        return fData != nullptr ? fData->stateCount : 0;
    }

    const d_string& getStateKey(const uint32_t index) const
    {
        assert(fData != nullptr && index < fData->stateCount);
        return (fData != nullptr && index < fData->stateCount) ? fData->stateKeys[index] : sFallbackString;
    }

    void setState(const char* const key, const char* const value)
    {
        assert(fPlugin != nullptr && key != nullptr && value != nullptr);

        if (fPlugin != nullptr && key != nullptr && value != nullptr)
            fPlugin->d_setState(key, value);
    }
#endif

    // -------------------------------------------------------------------

    void activate()
    {
        assert(fPlugin != nullptr);

        if (fPlugin != nullptr)
            fPlugin->d_activate();
    }

    void deactivate()
    {
        assert(fPlugin != nullptr);

        if (fPlugin != nullptr)
            fPlugin->d_deactivate();
    }

    void run(float** const inputs, float** const outputs, const uint32_t frames, const MidiEvent* const midiEvents, const uint32_t midiEventCount)
    {
        assert(fPlugin != nullptr);

        if (fPlugin != nullptr)
            fPlugin->d_run(inputs, outputs, frames, midiEvents, midiEventCount);
    }

    // -------------------------------------------------------------------

    void setBufferSize(const uint32_t bufferSize, bool doCallback = false)
    {
        assert(fData != nullptr && fPlugin != nullptr && bufferSize >= 2);

        if (fData != nullptr)
        {
            if (doCallback && fData->bufferSize == bufferSize)
                doCallback = false;

            fData->bufferSize = bufferSize;
        }

        if (fPlugin != nullptr && doCallback)
        {
            fPlugin->d_deactivate();
            fPlugin->d_bufferSizeChanged(bufferSize);
            fPlugin->d_activate();
        }
    }

    void setSampleRate(const double sampleRate, bool doCallback = false)
    {
        assert(fData != nullptr && fPlugin != nullptr && sampleRate > 0.0);

        if (fData != nullptr)
        {
            if (doCallback && fData->sampleRate == sampleRate)
                doCallback = false;

            fData->sampleRate = sampleRate;
        }

        if (fPlugin != nullptr && doCallback)
        {
            fPlugin->d_deactivate();
            fPlugin->d_sampleRateChanged(sampleRate);
            fPlugin->d_activate();
        }
    }

    // -------------------------------------------------------------------

protected:
    Plugin* const fPlugin;
    Plugin::PrivateData* const fData;

private:
    static const d_string        sFallbackString;
    static const ParameterRanges sFallbackRanges;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_PLUGIN_INTERNAL_HPP_INCLUDED
