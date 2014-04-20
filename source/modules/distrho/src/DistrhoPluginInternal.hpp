/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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
// Maxmimum values

static const uint32_t kMaxMidiEvents = 512;

// -----------------------------------------------------------------------
// Static data, see DistrhoPlugin.cpp

extern uint32_t d_lastBufferSize;
extern double   d_lastSampleRate;

// -----------------------------------------------------------------------
// Plugin private data

struct Plugin::PrivateData {
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

    uint32_t bufferSize;
    double   sampleRate;

    PrivateData() noexcept
        : parameterCount(0),
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
          bufferSize(d_lastBufferSize),
          sampleRate(d_lastSampleRate)
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
// Plugin exporter class

class PluginExporter
{
public:
    PluginExporter()
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

    ~PluginExporter()
    {
        delete fPlugin;
    }

    // -------------------------------------------------------------------

    const char* getName() const noexcept
    {
        return (fPlugin != nullptr) ? fPlugin->d_getName() : "";
    }

    const char* getLabel() const noexcept
    {
        return (fPlugin != nullptr) ? fPlugin->d_getLabel() : "";
    }

    const char* getMaker() const noexcept
    {
        return (fPlugin != nullptr) ? fPlugin->d_getMaker() : "";
    }

    const char* getLicense() const noexcept
    {
        return (fPlugin != nullptr) ? fPlugin->d_getLicense() : "";
    }

    uint32_t getVersion() const noexcept
    {
        return (fPlugin != nullptr) ? fPlugin->d_getVersion() : 1000;
    }

    long getUniqueId() const noexcept
    {
        return (fPlugin != nullptr) ? fPlugin->d_getUniqueId() : 0;
    }

    void* getInstancePointer() const noexcept
    {
        return fPlugin;
    }

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t getLatency() const noexcept
    {
        return (fData != nullptr) ? fData->latency : 0;
    }
#endif

    uint32_t getParameterCount() const noexcept
    {
        return (fData != nullptr) ? fData->parameterCount : 0;
    }

    uint32_t getParameterHints(const uint32_t index) const noexcept
    {
        assert(index < fData->parameterCount);
        return (fData != nullptr && index < fData->parameterCount) ? fData->parameters[index].hints : 0x0;
    }

    bool isParameterOutput(const uint32_t index) const noexcept
    {
        return (getParameterHints(index) & PARAMETER_IS_OUTPUT);
    }

    const d_string& getParameterName(const uint32_t index) const noexcept
    {
        assert(index < fData->parameterCount);
        return (fData != nullptr && index < fData->parameterCount) ? fData->parameters[index].name : sFallbackString;
    }

    const d_string& getParameterSymbol(const uint32_t index) const noexcept
    {
        assert(index < fData->parameterCount);
        return (fData != nullptr && index < fData->parameterCount) ? fData->parameters[index].symbol : sFallbackString;
    }

    const d_string& getParameterUnit(const uint32_t index) const noexcept
    {
        assert(index < fData->parameterCount);
        return (fData != nullptr && index < fData->parameterCount) ? fData->parameters[index].unit : sFallbackString;
    }

    const ParameterRanges& getParameterRanges(const uint32_t index) const noexcept
    {
        assert(index < fData->parameterCount);
        return (fData != nullptr && index < fData->parameterCount) ? fData->parameters[index].ranges : sFallbackRanges;
    }

    float getParameterValue(const uint32_t index) const noexcept
    {
        assert(index < fData->parameterCount);
        return (fPlugin != nullptr && index < fData->parameterCount) ? fPlugin->d_getParameterValue(index) : 0.0f;
    }

    void setParameterValue(const uint32_t index, const float value)
    {
        assert(index < fData->parameterCount);

        if (fPlugin != nullptr && index < fData->parameterCount)
            fPlugin->d_setParameterValue(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t getProgramCount() const noexcept
    {
        return (fData != nullptr) ? fData->programCount : 0;
    }

    const d_string& getProgramName(const uint32_t index) const noexcept
    {
        assert(index < fData->programCount);
        return (fData != nullptr && index < fData->programCount) ? fData->programNames[index] : sFallbackString;
    }

    void setProgram(const uint32_t index)
    {
        assert(index < fData->programCount);

        if (fPlugin != nullptr && index < fData->programCount)
            fPlugin->d_setProgram(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    bool wantsStateKey(const char* const key) const noexcept
    {
        for (uint32_t i=0; i < fData->stateCount; ++i)
        {
            if (fData->stateKeys[i] == key)
                return true;
        }

        return false;
    }

    uint32_t getStateCount() const noexcept
    {
        return fData != nullptr ? fData->stateCount : 0;
    }

    const d_string& getStateKey(const uint32_t index) const noexcept
    {
        assert(index < fData->stateCount);
        return (fData != nullptr && index < fData->stateCount) ? fData->stateKeys[index] : sFallbackString;
    }

    void setState(const char* const key, const char* const value)
    {
        assert(key != nullptr && value != nullptr);

        if (fPlugin != nullptr && key != nullptr && value != nullptr)
            fPlugin->d_setState(key, value);
    }
#endif

#if DISTRHO_PLUGIN_WANT_TIMEPOS
    void setTimePos(const TimePos& timePos)
    {
        if (fData != nullptr)
            std::memcpy(&fData->timePos, &timePos, sizeof(TimePos));
    }
#endif

    // -------------------------------------------------------------------

    void activate()
    {
        if (fPlugin != nullptr)
            fPlugin->d_activate();
    }

    void deactivate()
    {
        if (fPlugin != nullptr)
            fPlugin->d_deactivate();
    }

#if DISTRHO_PLUGIN_IS_SYNTH
    void run(float** const inputs, float** const outputs, const uint32_t frames, const MidiEvent* const midiEvents, const uint32_t midiEventCount)
    {
        if (fPlugin != nullptr)
            fPlugin->d_run(inputs, outputs, frames, midiEvents, midiEventCount);
    }
#else
    void run(float** const inputs, float** const outputs, const uint32_t frames)
    {
        if (fPlugin != nullptr)
            fPlugin->d_run(inputs, outputs, frames);
    }
#endif
    // -------------------------------------------------------------------

    void setBufferSize(const uint32_t bufferSize, bool doCallback = false)
    {
        assert(bufferSize >= 2);

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
        assert(sampleRate > 0.0);

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

private:
    // -------------------------------------------------------------------
    // private members accessed by DistrhoPlugin class

    Plugin* const fPlugin;
    Plugin::PrivateData* const fData;

    // -------------------------------------------------------------------
    // Static fallback data, see DistrhoPlugin.cpp

    static const d_string        sFallbackString;
    static const ParameterRanges sFallbackRanges;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_PLUGIN_INTERNAL_HPP_INCLUDED
