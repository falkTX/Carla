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
    bool isProcessing;

    uint32_t   parameterCount;
    Parameter* parameters;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t  programCount;
    d_string* programNames;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    uint32_t  stateCount;
    d_string* stateKeys;
    d_string* stateDefValues;
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t latency;
#endif

#if DISTRHO_PLUGIN_WANT_TIMEPOS
    TimePosition timePosition;
#endif

    uint32_t bufferSize;
    double   sampleRate;

    PrivateData() noexcept
        : isProcessing(false),
          parameterCount(0),
          parameters(nullptr),
#if DISTRHO_PLUGIN_WANT_PROGRAMS
          programCount(0),
          programNames(nullptr),
#endif
#if DISTRHO_PLUGIN_WANT_STATE
          stateCount(0),
          stateKeys(nullptr),
          stateDefValues(nullptr),
#endif
#if DISTRHO_PLUGIN_WANT_LATENCY
          latency(0),
#endif
          bufferSize(d_lastBufferSize),
          sampleRate(d_lastSampleRate)
    {
        DISTRHO_SAFE_ASSERT(bufferSize != 0);
        DISTRHO_SAFE_ASSERT(d_isNotZero(sampleRate));
    }

    ~PrivateData() noexcept
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

        if (stateDefValues != nullptr)
        {
            delete[] stateDefValues;
            stateDefValues = nullptr;
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
          fData((fPlugin != nullptr) ? fPlugin->pData : nullptr),
          fIsActive(false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);

        for (uint32_t i=0, count=fData->parameterCount; i < count; ++i)
            fPlugin->d_initParameter(i, fData->parameters[i]);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        for (uint32_t i=0, count=fData->programCount; i < count; ++i)
            fPlugin->d_initProgramName(i, fData->programNames[i]);
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        for (uint32_t i=0, count=fData->stateCount; i < count; ++i)
            fPlugin->d_initState(i, fData->stateKeys[i], fData->stateDefValues[i]);
#endif
    }

    ~PluginExporter()
    {
        delete fPlugin;
    }

    // -------------------------------------------------------------------

    const char* getName() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, "");

        return fPlugin->d_getName();
    }

    const char* getLabel() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, "");

        return fPlugin->d_getLabel();
    }

    const char* getMaker() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, "");

        return fPlugin->d_getMaker();
    }

    const char* getLicense() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, "");

        return fPlugin->d_getLicense();
    }

    uint32_t getVersion() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, 0);

        return fPlugin->d_getVersion();
    }

    long getUniqueId() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, 0);

        return fPlugin->d_getUniqueId();
    }

    void* getInstancePointer() const noexcept
    {
        return fPlugin;
    }

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t getLatency() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->latency;
    }
#endif

    uint32_t getParameterCount() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->parameterCount;
    }

    uint32_t getParameterHints(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, 0x0);

        return fData->parameters[index].hints;
    }

    bool isParameterOutput(const uint32_t index) const noexcept
    {
        return (getParameterHints(index) & kParameterIsOutput);
    }

    const d_string& getParameterName(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, sFallbackString);

        return fData->parameters[index].name;
    }

    const d_string& getParameterSymbol(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, sFallbackString);

        return fData->parameters[index].symbol;
    }

    const d_string& getParameterUnit(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, sFallbackString);

        return fData->parameters[index].unit;
    }

    const ParameterRanges& getParameterRanges(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, sFallbackRanges);

        return fData->parameters[index].ranges;
    }

    float getParameterValue(const uint32_t index) const
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, 0.0f);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, 0.0f);

        return fPlugin->d_getParameterValue(index);
    }

    void setParameterValue(const uint32_t index, const float value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount,);

        fPlugin->d_setParameterValue(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t getProgramCount() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->programCount;
    }

    const d_string& getProgramName(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->programCount, sFallbackString);

        return fData->programNames[index];
    }

    void setProgram(const uint32_t index)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->programCount,);

        fPlugin->d_setProgram(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    uint32_t getStateCount() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->stateCount;
    }

    const d_string& getStateKey(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->stateCount, sFallbackString);

        return fData->stateKeys[index];
    }

    const d_string& getStateDefaultValue(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->stateCount, sFallbackString);

        return fData->stateDefValues[index];
    }

    void setState(const char* const key, const char* const value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        DISTRHO_SAFE_ASSERT_RETURN(value != nullptr,);

        fPlugin->d_setState(key, value);
    }

    bool wantStateKey(const char* const key) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0', false);

        for (uint32_t i=0; i < fData->stateCount; ++i)
        {
            if (fData->stateKeys[i] == key)
                return true;
        }

        return false;
    }
#endif

#if DISTRHO_PLUGIN_WANT_TIMEPOS
    void setTimePosition(const TimePosition& timePosition) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);

        std::memcpy(&fData->timePosition, &timePosition, sizeof(TimePosition));
    }
#endif

    // -------------------------------------------------------------------

    void activate()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        fIsActive = true;
        fPlugin->d_activate();
    }

    void deactivate()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        fIsActive = false;
        fPlugin->d_deactivate();
    }

#if DISTRHO_PLUGIN_IS_SYNTH
    void run(const float** const inputs, float** const outputs, const uint32_t frames,
             const MidiEvent* const midiEvents, const uint32_t midiEventCount)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        fData->isProcessing = true;
        fPlugin->d_run(inputs, outputs, frames, midiEvents, midiEventCount);
        fData->isProcessing = false;
    }
#else
    void run(const float** const inputs, float** const outputs, const uint32_t frames)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        fData->isProcessing = true;
        fPlugin->d_run(inputs, outputs, frames);
        fData->isProcessing = false;
    }
#endif

    // -------------------------------------------------------------------

    uint32_t getBufferSize() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);
        return fData->bufferSize;
    }

    double getSampleRate() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0.0);
        return fData->sampleRate;
    }

    void setBufferSize(const uint32_t bufferSize, const bool doCallback = false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        DISTRHO_SAFE_ASSERT(bufferSize >= 2);

        if (fData->bufferSize == bufferSize)
            return;

        fData->bufferSize = bufferSize;

        if (doCallback)
        {
            if (fIsActive) fPlugin->d_deactivate();
            fPlugin->d_bufferSizeChanged(bufferSize);
            if (fIsActive) fPlugin->d_activate();
        }
    }

    void setSampleRate(const double sampleRate, const bool doCallback = false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        DISTRHO_SAFE_ASSERT(sampleRate > 0.0);

        if (d_isEqual(fData->sampleRate, sampleRate))
            return;

        fData->sampleRate = sampleRate;

        if (doCallback)
        {
            if (fIsActive) fPlugin->d_deactivate();
            fPlugin->d_sampleRateChanged(sampleRate);
            if (fIsActive) fPlugin->d_activate();
        }
    }

private:
    // -------------------------------------------------------------------
    // Plugin and DistrhoPlugin data

    Plugin* const fPlugin;
    Plugin::PrivateData* const fData;
    bool fIsActive;

    // -------------------------------------------------------------------
    // Static fallback data, see DistrhoPlugin.cpp

    static const d_string        sFallbackString;
    static const ParameterRanges sFallbackRanges;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginExporter)
    DISTRHO_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_PLUGIN_INTERNAL_HPP_INCLUDED
