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

#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Parameter Hints */

/**
   @defgroup ParameterHints Parameter Hints

   Various parameter hints.
   @see Parameter::hints
   @{
 */

/**
   Parameter is automable (real-time safe).
   @see Plugin::d_setParameterValue()
 */
static const uint32_t kParameterIsAutomable = 0x01;

/**
   Parameter value is boolean.
   It's always at either minimum or maximum value.
 */
static const uint32_t kParameterIsBoolean = 0x02;

/**
   Parameter value is integer.
 */
static const uint32_t kParameterIsInteger = 0x04;

/**
   Parameter value is logarithmic.
 */
static const uint32_t kParameterIsLogarithmic = 0x08;

/**
   Parameter is of output type.
   When unset, parameter is assumed to be of input type.

   Parameter inputs are changed by the host and must not be changed by the plugin.
   The only exception being when changing programs, see Plugin::d_setProgram().
   Outputs are changed by the plugin and never modified by the host.
 */
static const uint32_t kParameterIsOutput = 0x10;

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * DPF Base structs */

/**
   Parameter ranges.
   This is used to set the default, minimum and maximum values of a parameter.

   By default a parameter has 0.0 as minimum, 1.0 as maximum and 0.0 as default.
   When changing this struct values you must ensure maximum > minimum and default is within range.
 */
struct ParameterRanges {
   /**
      Default value.
    */
    float def;

   /**
      Minimum value.
    */
    float min;

   /**
      Maximum value.
    */
    float max;

   /**
      Default constructor.
    */
    ParameterRanges() noexcept
        : def(0.0f),
          min(0.0f),
          max(1.0f) {}

   /**
      Constructor using custom values.
    */
    ParameterRanges(const float df, const float mn, const float mx) noexcept
        : def(df),
          min(mn),
          max(mx) {}

   /**
      Fix the default value within range.
    */
    void fixDefault() noexcept
    {
        fixValue(def);
    }

   /**
      Fix a value within range.
    */
    void fixValue(float& value) const noexcept
    {
        if (value < min)
            value = min;
        else if (value > max)
            value = max;
    }

   /**
      Get a fixed value within range.
    */
    const float& getFixedValue(const float& value) const noexcept
    {
        if (value <= min)
            return min;
        if (value >= max)
            return max;
        return value;
    }

   /**
      Get a value normalized to 0.0<->1.0.
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

   /**
      Get a value normalized to 0.0<->1.0, fixed within range.
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

   /**
      Get a proper value previously normalized to 0.0<->1.0.
    */
    float getUnnormalizedValue(const float& value) const noexcept
    {
        if (value <= 0.0f)
            return min;
        if (value >= 1.0f)
            return max;

        return value * (max - min) + min;
    }
};

/**
   Parameter.
 */
struct Parameter {
   /**
      Hints describing this parameter.
      @see ParameterHints
    */
    uint32_t hints;

   /**
      The name of this parameter.
      A parameter name can contain any character, but hosts might have a hard time with non-ascii ones.
      The name doesn't have to be unique within a plugin instance, but it's recommended.
    */
    d_string name;

   /**
      The symbol of this parameter.
      A parameter symbol is a short restricted name used as a machine and human readable identifier.
      The first character must be one of _, a-z or A-Z and subsequent characters can be from _, a-z, A-Z and 0-9.
      @note: Parameter symbols MUST be unique within a plugin instance.
    */
    d_string symbol;

   /**
      The unit of this parameter.
      This means something like "dB", "kHz" and "ms".
      Can be left blank if units do not apply to this parameter.
    */
    d_string unit;

   /**
      Ranges of this parameter.
      The ranges describe the default, minimum and maximum values.
    */
    ParameterRanges ranges;

   /**
      Default constructor for a null parameter.
    */
    Parameter() noexcept
        : hints(0x0),
          name(),
          symbol(),
          unit(),
          ranges() {}
};

/**
   MIDI event.
 */
struct MidiEvent {
   /**
      Size of internal data.
    */
    static const uint32_t kDataSize = 4;

   /**
      Time offset in frames.
    */
    uint32_t frame;

   /**
      Number of bytes used.
    */
    uint32_t size;

   /**
      MIDI data.
      If size > kDataSize, dataExt is used (otherwise null).
    */
    uint8_t        data[kDataSize];
    const uint8_t* dataExt;
};

/**
   Time position.
   The @a playing and @a frame values are always valid.
   BBT values are only valid when @a bbt.valid is true.

   This struct is inspired by the JACK Transport API.
 */
struct TimePosition {
   /**
      Wherever the host transport is playing/rolling.
    */
    bool playing;

   /**
      Current host transport position in frames.
    */
    uint64_t frame;

   /**
      Bar-Beat-Tick time position.
    */
    struct BarBeatTick {
       /**
          Wherever the host transport is using BBT.
          If false you must not read from this struct.
        */
        bool valid;

       /**
          Current bar.
          Should always be > 0.
          The first bar is bar '1'.
        */
        int32_t bar;

       /**
          Current beat within bar.
          Should always be > 0 and <= @a beatsPerBar.
          The first beat is beat '1'.
        */
        int32_t beat;

       /**
          Current tick within beat.
          Should always be > 0 and <= @a ticksPerBeat.
          The first tick is tick '0'.
        */
        int32_t tick;

       /**
          Number of ticks that have elapsed between frame 0 and the first beat of the current measure.
        */
        double barStartTick;

       /**
          Time signature "numerator".
        */
        float beatsPerBar;

       /**
          Time signature "denominator".
        */
        float beatType;

       /**
          Number of ticks within a bar.
          Usually a moderately large integer with many denominators, such as 1920.0.
        */
        double ticksPerBeat;

       /**
          Number of beats per minute.
        */
        double beatsPerMinute;

       /**
          Default constructor for a null BBT time position.
        */
        BarBeatTick() noexcept
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

   /**
      Default constructor for a time position.
    */
    TimePosition() noexcept
        : playing(false),
          frame(0),
          bbt() {}
};

/* ------------------------------------------------------------------------------------------------------------
 * DPF Plugin */

/**
   DPF Plugin class from where plugin instances are created.

   The public methods (Host state) are called from the plugin to get or set host information.
   They can be called from a plugin instance at anytime unless stated otherwise.
   All other methods are to be implemented by the plugin and will be called by the host.

   Shortly after a plugin instance is created, the various d_init* functions will be called by the host.
   Host will call d_activate() before d_run(), and d_deactivate() before the plugin instance is destroyed.
   The host may call deactivate right after activate and vice-versa, but never activate/deactivate consecutively.
   There is no limit on how many times d_run() is called, only that activate/deactivate will be called in between.

   The buffer size and sample rate values will remain constant between activate and deactivate.
   Buffer size is only a hint though, the host might call d_run() with a higher or lower number of frames.

   Some of this class functions are only available according to some macros.

   DISTRHO_PLUGIN_WANT_PROGRAMS activates program related features.
   When enabled you need to implement d_initProgramName() and d_setProgram().

   DISTRHO_PLUGIN_WANT_STATE activates internal state features.
   When enabled you need to implement d_initStateKey() and d_setState().

   The process function d_run() changes wherever DISTRHO_PLUGIN_HAS_MIDI_INPUT is enabled or not.
   When enabled it provides midi input events.
 */
class Plugin
{
public:
   /**
      Plugin class constructor.
      You must set all parameter values to their defaults, matching ParameterRanges::def.
      If you're using states you must also set them to their defaults by calling d_setState().
    */
    Plugin(const uint32_t parameterCount, const uint32_t programCount, const uint32_t stateCount);

   /**
      Destructor.
    */
    virtual ~Plugin();

   /* --------------------------------------------------------------------------------------------------------
    * Host state */

   /**
      Get the current buffer size that will probably be used during processing, in frames.
      This value will remain constant between activate and deactivate.
      @note: This value is only a hint!
             Hosts might call d_run() with a higher or lower number of frames.
      @see d_bufferSizeChanged(uint32_t)
    */
    uint32_t d_getBufferSize() const noexcept;

   /**
      Get the current sample rate that will be used during processing.
      This value will remain constant between activate and deactivate.
      @see d_sampleRateChanged(double)
    */
    double d_getSampleRate() const noexcept;

#if DISTRHO_PLUGIN_WANT_TIMEPOS
   /**
      Get the current host transport time position.
      This function should only be called during d_run().
      You can call this during other times, but the returned position is not guaranteed to be in sync.
      @note: TimePos is not supported in LADSPA and DSSI plugin formats.
    */
    const TimePosition& d_getTimePosition() const noexcept;
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
   /**
      Change the plugin audio output latency to @a frames.
      This function should only be called in the constructor, d_activate() and d_run().
    */
    void d_setLatency(const uint32_t frames) noexcept;
#endif

#if DISTRHO_PLUGIN_HAS_MIDI_OUTPUT
   /**
      Write a MIDI output event.
      This function must only be called during d_run().
      Returns false when the host buffer is full, in which case do not call this again until the next d_run().
    */
    bool d_writeMidiEvent(const MidiEvent& midiEvent) noexcept;
#endif

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin name.
      Returns DISTRHO_PLUGIN_NAME by default.
    */
    virtual const char* d_getName() const { return DISTRHO_PLUGIN_NAME; }

   /**
      Get the plugin label.
      A plugin label follows the same rules as Parameter::symbol, with the exception that it can start with numbers.
    */
    virtual const char* d_getLabel() const = 0;

   /**
      Get the plugin author/maker.
    */
    virtual const char* d_getMaker() const = 0;

   /**
      Get the plugin license name (a single line of text).
    */
    virtual const char* d_getLicense() const = 0;

   /**
      Get the plugin version, in hexadecimal.
      TODO format to be defined
    */
    virtual uint32_t d_getVersion() const = 0;

   /**
      Get the plugin unique Id.
      This value is used by LADSPA, DSSI and VST plugin formats.
    */
    virtual int64_t d_getUniqueId() const = 0;

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    virtual void d_initParameter(uint32_t index, Parameter& parameter) = 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
   /**
      Set the name of the program @a index.
      This function will be called once, shortly after the plugin is created.
      Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_PROGRAMS is enabled.
    */
    virtual void d_initProgramName(uint32_t index, d_string& programName) = 0;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      Set the state key and default value of @a index.
      This function will be called once, shortly after the plugin is created.
      Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_STATE is enabled.
    */
    virtual void d_initState(uint32_t index, d_string& stateKey, d_string& defaultStateValue) = 0;
#endif

   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.
      The host may call this function from any context, including realtime processing.
    */
    virtual float d_getParameterValue(uint32_t index) const = 0;

   /**
      Change a parameter value.
      The host may call this function from any context, including realtime processing.
      When a parameter is marked as automable, you must ensure no non-realtime operations are called.
      @note This function will only be called for parameter inputs.
    */
    virtual void d_setParameterValue(uint32_t index, float value) = 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
   /**
      Change the currently used program to @a index.
      The host may call this function from any context, including realtime processing.
      Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_PROGRAMS is enabled.
    */
    virtual void d_setProgram(uint32_t index) = 0;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      Change an internal state @a key to @a value.
      Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_STATE is enabled.
    */
    virtual void d_setState(const char* key, const char* value) = 0;
#endif

   /* --------------------------------------------------------------------------------------------------------
    * Process */

   /**
      Activate this plugin.
    */
    virtual void d_activate() {}

   /**
      Deactivate this plugin.
    */
    virtual void d_deactivate() {}

#if DISTRHO_PLUGIN_HAS_MIDI_INPUT
   /**
      Run/process function for plugins with MIDI input.
      @note: Some parameters might be null if there are no audio inputs/outputs or MIDI events.
    */
    virtual void d_run(const float** inputs, float** outputs, uint32_t frames,
                       const MidiEvent* midiEvents, uint32_t midiEventCount) = 0;
#else
   /**
      Run/process function for plugins without MIDI input.
      @note: Some parameters might be null if there are no audio inputs or outputs.
    */
    virtual void d_run(const float** inputs, float** outputs, uint32_t frames) = 0;
#endif

   /* --------------------------------------------------------------------------------------------------------
    * Callbacks (optional) */

   /**
      Optional callback to inform the plugin about a buffer size change.
      This function will only be called when the plugin is deactivated.
      @note: This value is only a hint!
             Hosts might call d_run() with a higher or lower number of frames.
      @see d_getBufferSize()
    */
    virtual void d_bufferSizeChanged(uint32_t newBufferSize);

   /**
      Optional callback to inform the plugin about a sample rate change.
      This function will only be called when the plugin is deactivated.
      @see d_getSampleRate()
    */
    virtual void d_sampleRateChanged(double newSampleRate);

    // -------------------------------------------------------------------------------------------------------

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class PluginExporter;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

/**
   TODO.
 */
extern Plugin* createPlugin();

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_PLUGIN_HPP_INCLUDED
