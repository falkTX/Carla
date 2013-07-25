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

#ifndef DISTRHO_PLUGIN_HPP_INCLUDED
#define DISTRHO_PLUGIN_HPP_INCLUDED

#include "DistrhoUtils.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Parameter Hints

const uint32_t PARAMETER_IS_AUTOMABLE   = 1 << 0;
const uint32_t PARAMETER_IS_BOOLEAN     = 1 << 1;
const uint32_t PARAMETER_IS_INTEGER     = 1 << 2;
const uint32_t PARAMETER_IS_LOGARITHMIC = 1 << 3;
const uint32_t PARAMETER_IS_OUTPUT      = 1 << 4;

// -----------------------------------------------------------------------
// Parameter Ranges

struct ParameterRanges {
    float def;
    float min;
    float max;
    float step;
    float stepSmall;
    float stepLarge;

    ParameterRanges() noexcept
        : def(0.0f),
          min(0.0f),
          max(1.0f),
          step(0.001f),
          stepSmall(0.00001f),
          stepLarge(0.01f) {}

    ParameterRanges(float def, float min, float max) noexcept
        : step(0.001f),
          stepSmall(0.00001f),
          stepLarge(0.01f)
    {
        this->def = def;
        this->min = min;
        this->max = max;
    }

    ParameterRanges(float def, float min, float max, float step, float stepSmall, float stepLarge) noexcept
    {
        this->def = def;
        this->min = min;
        this->max = max;
        this->step = step;
        this->stepSmall = stepSmall;
        this->stepLarge = stepLarge;
    }

    void clear() noexcept
    {
        def = 0.0f;
        min = 0.0f;
        max = 1.0f;
        step = 0.001f;
        stepSmall = 0.00001f;
        stepLarge = 0.01f;
    }

    void fixValue(float& value) const noexcept
    {
        if (value < min)
            value = min;
        else if (value > max)
            value = max;
    }

    float getFixedValue(const float& value) const noexcept
    {
        if (value < min)
            return min;
        else if (value > max)
            return max;
        return value;
    }

    float getNormalizedValue(const float& value) const noexcept
    {
        const float newValue((value - min) / (max - min));

        if (newValue <= 0.0f)
            return 0.0f;
        if (newValue >= 1.0f)
            return 1.0f;
        return newValue;
    }

    float getUnnormalizedValue(const float& value) const noexcept
    {
        return value * (max - min) + min;
    }
};

// -----------------------------------------------------------------------
// Parameter

struct Parameter {
    uint32_t hints;
    d_string name;
    d_string symbol;
    d_string unit;
    ParameterRanges ranges;

    Parameter()
        : hints(0x0) {}

    void clear() noexcept
    {
        hints  = 0x0;
        name   = "";
        symbol = "";
        unit   = "";
        ranges.clear();
    }
};

// -----------------------------------------------------------------------
// MidiEvent

struct MidiEvent {
    uint32_t frame;
    uint8_t  buf[4];
    uint8_t  size;

    MidiEvent() noexcept
    {
        clear();
    }

    void clear() noexcept
    {
        frame  = 0;
        buf[0] = 0;
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
        size   = 0;
    }
};

// -----------------------------------------------------------------------
// TimePos

struct TimePos {
    bool playing;
    uint64_t frame;
    double bpm;

    TimePos() noexcept
        : playing(false),
          frame(0),
          bpm(120.0) {}
};

// -----------------------------------------------------------------------
// Plugin

class Plugin
{
public:
    Plugin(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount);
    virtual ~Plugin();

    // -------------------------------------------------------------------
    // Host state

    uint32_t       d_getBufferSize() const noexcept;
    double         d_getSampleRate() const noexcept;
#if DISTRHO_PLUGIN_WANT_TIMEPOS
    const TimePos& d_getTimePos()    const noexcept;
#endif
#if DISTRHO_PLUGIN_WANT_LATENCY
    void           d_setLatency(uint32_t frames) noexcept;
#endif

protected:
    // -------------------------------------------------------------------
    // Information

    virtual const char* d_getName()     const noexcept { return DISTRHO_PLUGIN_NAME; }
    virtual const char* d_getLabel()    const noexcept = 0;
    virtual const char* d_getMaker()    const noexcept = 0;
    virtual const char* d_getLicense()  const noexcept = 0;
    virtual uint32_t    d_getVersion()  const noexcept = 0;
    virtual long        d_getUniqueId() const noexcept = 0;

    // -------------------------------------------------------------------
    // Init

    virtual void d_initParameter(uint32_t index, Parameter& parameter) = 0;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    virtual void d_initProgramName(uint32_t index, d_string& programName) = 0;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    virtual void d_initStateKey(uint32_t index, d_string& stateKey) = 0;
#endif

    // -------------------------------------------------------------------
    // Internal data

    virtual float d_getParameterValue(uint32_t index) const = 0;
    virtual void  d_setParameterValue(uint32_t index, float value) = 0;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    virtual void  d_setProgram(uint32_t index) = 0;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    virtual void  d_setState(const char* key, const char* value) = 0;
#endif

    // -------------------------------------------------------------------
    // Process

    virtual void d_activate() {}
    virtual void d_deactivate() {}
    virtual void d_run(float** inputs, float** outputs, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents) = 0;

    // -------------------------------------------------------------------
    // Callbacks (optional)

    virtual void d_bufferSizeChanged(uint32_t newBufferSize);
    virtual void d_sampleRateChanged(double newSampleRate);

    // -------------------------------------------------------------------

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class PluginInternal;
};

// -----------------------------------------------------------------------
// Create plugin, entry point

extern Plugin* createPlugin();

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_PLUGIN_HPP_INCLUDED
