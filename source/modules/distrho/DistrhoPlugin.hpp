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

#ifndef DISTRHO_PLUGIN_HPP_INCLUDED
#define DISTRHO_PLUGIN_HPP_INCLUDED

#include "extra/d_string.hpp"
#include "src/DistrhoPluginChecks.h"

#include <cmath>

#ifdef PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

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

    ParameterRanges() noexcept
        : def(0.0f),
          min(0.0f),
          max(1.0f) {}

    ParameterRanges(float def, float min, float max) noexcept
    {
        this->def = def;
        this->min = min;
        this->max = max;
    }

    void clear() noexcept
    {
        def = 0.0f;
        min = 0.0f;
        max = 1.0f;
    }

    /*!
     * Fix default value within range.
     */
    void fixDefault() noexcept
    {
        fixValue(def);
    }

    /*!
     * Fix a value within range.
     */
    void fixValue(float& value) const noexcept
    {
        if (value <= min)
            value = min;
        else if (value > max)
            value = max;
    }

    /*!
     * Get a fixed value within range.
     */
    float getFixedValue(const float& value) const noexcept
    {
        if (value <= min)
            return min;
        if (value >= max)
            return max;
        return value;
    }

    /*!
     * Get a value normalized to 0.0<->1.0.
     */
    float getNormalizedValue(const float& value) const noexcept
    {
        const float normValue((value - min) / (max - min));

        if (normValue <= 0.0f)
            return 0.0f;
        if (normValue >= 1.0f)
            return 1.0f;
        return normValue;
    }

    /*!
     * Get a value normalized to 0.0<->1.0, fixed within range.
     */
    float getFixedAndNormalizedValue(const float& value) const noexcept
    {
        if (value <= min)
            return 0.0f;
        if (value >= max)
            return 1.0f;

        const float normValue((value - min) / (max - min));

        if (normValue <= 0.0f)
            return 0.0f;
        if (normValue >= 1.0f)
            return 1.0f;

        return normValue;
    }

    /*!
     * Get a proper value previously normalized to 0.0<->1.0.
     */
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

    Parameter() noexcept
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
    uint8_t  size;
    uint8_t  buf[4];

    void clear() noexcept
    {
        frame  = 0;
        size   = 0;
        buf[0] = 0;
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
    }
};

// -----------------------------------------------------------------------
// TimePos

struct TimePos {
    bool playing;
    uint64_t frame;

    struct BeatBarTick {
        bool valid;

        int32_t bar;  /*!< current bar */
        int32_t beat; /*!< current beat-within-bar */
        int32_t tick; /*!< current tick-within-beat */
        double barStartTick;

        float beatsPerBar; /*!< time signature "numerator" */
        float beatType;    /*!< time signature "denominator" */

        double ticksPerBeat;
        double beatsPerMinute;

        BeatBarTick() noexcept
            : valid(false),
              bar(0),
              beat(0),
              tick(0),
              barStartTick(0.0),
              beatsPerBar(0.0f),
              beatType(0.0f),
              ticksPerBeat(0.0),
              beatsPerMinute(0.0) {}
    } bbt;

    TimePos() noexcept
        : playing(false),
          frame(0) {}
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
#if DISTRHO_PLUGIN_IS_SYNTH
    virtual void d_run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) = 0;
#else
    virtual void d_run(const float** inputs, float** outputs, uint32_t frames) = 0;
#endif

    // -------------------------------------------------------------------
    // Callbacks (optional)

    virtual void d_bufferSizeChanged(uint32_t newBufferSize);
    virtual void d_sampleRateChanged(double newSampleRate);

    // -------------------------------------------------------------------

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class PluginExporter;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plugin)
};

// -----------------------------------------------------------------------
// Create plugin, entry point

extern Plugin* createPlugin();

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_PLUGIN_HPP_INCLUDED
