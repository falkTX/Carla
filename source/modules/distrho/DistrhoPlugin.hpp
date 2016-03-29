/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#include "extra/String.hpp"
#include "extra/LeakDetector.hpp"
#include "src/DistrhoPluginChecks.h"

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Audio Port Hints */

/**
   @defgroup AudioPortHints Audio Port Hints

   Various audio port hints.
   @see AudioPort::hints
   @{
 */

/**
   Audio port can be used as control voltage (LV2 only).
 */
static const uint32_t kAudioPortIsCV = 0x1;

/**
   Audio port should be used as sidechan (LV2 only).
 */
static const uint32_t kAudioPortIsSidechain = 0x2;

/** @} */

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
   @see Plugin::setParameterValue(uint32_t, float)
 */
static const uint32_t kParameterIsAutomable = 0x01;

/**
   Parameter value is boolean.@n
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
   Parameter is of output type.@n
   When unset, parameter is assumed to be of input type.

   Parameter inputs are changed by the host and must not be changed by the plugin.@n
   The only exception being when changing programs, see Plugin::loadProgram().@n
   Outputs are changed by the plugin and never modified by the host.
 */
static const uint32_t kParameterIsOutput = 0x10;

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Base Plugin structs */

/**
   @defgroup BasePluginStructs Base Plugin Structs
   @{
 */

/**
   Audio Port.
 */
struct AudioPort {
   /**
      Hints describing this audio port.
      @see AudioPortHints
    */
    uint32_t hints;

   /**
      The name of this audio port.@n
      An audio port name can contain any character, but hosts might have a hard time with non-ascii ones.@n
      The name doesn't have to be unique within a plugin instance, but it's recommended.
    */
    String name;

   /**
      The symbol of this audio port.@n
      An audio port symbol is a short restricted name used as a machine and human readable identifier.@n
      The first character must be one of _, a-z or A-Z and subsequent characters can be from _, a-z, A-Z and 0-9.
      @note Audio port and parameter symbols MUST be unique within a plugin instance.
    */
    String symbol;

   /**
      Default constructor for a regular audio port.
    */
    AudioPort() noexcept
        : hints(0x0),
          name(),
          symbol() {}
};

/**
   Parameter ranges.@n
   This is used to set the default, minimum and maximum values of a parameter.

   By default a parameter has 0.0 as minimum, 1.0 as maximum and 0.0 as default.@n
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
      Default constructor, using 0.0 as minimum, 1.0 as maximum and 0.0 as default.
    */
    ParameterRanges() noexcept
        : def(0.0f),
          min(0.0f),
          max(1.0f) {}

   /**
      Constructor using custom values.
    */
    ParameterRanges(float df, float mn, float mx) noexcept
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
      The name of this parameter.@n
      A parameter name can contain any character, but hosts might have a hard time with non-ascii ones.@n
      The name doesn't have to be unique within a plugin instance, but it's recommended.
    */
    String name;

   /**
      The symbol of this parameter.@n
      A parameter symbol is a short restricted name used as a machine and human readable identifier.@n
      The first character must be one of _, a-z or A-Z and subsequent characters can be from _, a-z, A-Z and 0-9.
      @note Parameter symbols MUST be unique within a plugin instance.
    */
    String symbol;

   /**
      The unit of this parameter.@n
      This means something like "dB", "kHz" and "ms".@n
      Can be left blank if a unit does not apply to this parameter.
    */
    String unit;

   /**
      Ranges of this parameter.@n
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

   /**
      Constructor using custom values.
    */
    Parameter(uint32_t h, const char* n, const char* s, const char* u, float def, float min, float max) noexcept
        : hints(h),
          name(n),
          symbol(s),
          unit(u),
          ranges(def, min, max) {}
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
      MIDI data.@n
      If size > kDataSize, dataExt is used (otherwise null).
    */
    uint8_t        data[kDataSize];
    const uint8_t* dataExt;
};

/**
   Time position.@n
   The @a playing and @a frame values are always valid.@n
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
          Wherever the host transport is using BBT.@n
          If false you must not read from this struct.
        */
        bool valid;

       /**
          Current bar.@n
          Should always be > 0.@n
          The first bar is bar '1'.
        */
        int32_t bar;

       /**
          Current beat within bar.@n
          Should always be > 0 and <= @a beatsPerBar.@n
          The first beat is beat '1'.
        */
        int32_t beat;

       /**
          Current tick within beat.@n
          Should always be > 0 and <= @a ticksPerBeat.@n
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
          Number of ticks within a bar.@n
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

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * DPF Plugin */

/**
   @defgroup MainClasses Main Classes
   @{
 */

/**
   DPF Plugin class from where plugin instances are created.

   The public methods (Host state) are called from the plugin to get or set host information.@n
   They can be called from a plugin instance at anytime unless stated otherwise.@n
   All other methods are to be implemented by the plugin and will be called by the host.

   Shortly after a plugin instance is created, the various init* functions will be called by the host.@n
   Host will call activate() before run(), and deactivate() before the plugin instance is destroyed.@n
   The host may call deactivate right after activate and vice-versa, but never activate/deactivate consecutively.@n
   There is no limit on how many times run() is called, only that activate/deactivate will be called in between.

   The buffer size and sample rate values will remain constant between activate and deactivate.@n
   Buffer size is only a hint though, the host might call run() with a higher or lower number of frames.

   Some of this class functions are only available according to some macros.

   DISTRHO_PLUGIN_WANT_PROGRAMS activates program related features.@n
   When enabled you need to implement initProgramName() and loadProgram().

   DISTRHO_PLUGIN_WANT_STATE activates internal state features.@n
   When enabled you need to implement initStateKey() and setState().

   The process function run() changes wherever DISTRHO_PLUGIN_WANT_MIDI_INPUT is enabled or not.@n
   When enabled it provides midi input events.
 */
class Plugin
{
public:
   /**
      Plugin class constructor.@n
      You must set all parameter values to their defaults, matching ParameterRanges::def.
    */
    Plugin(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount);

   /**
      Destructor.
    */
    virtual ~Plugin();

   /* --------------------------------------------------------------------------------------------------------
    * Host state */

   /**
      Get the current buffer size that will probably be used during processing, in frames.@n
      This value will remain constant between activate and deactivate.
      @note This value is only a hint!@n
            Hosts might call run() with a higher or lower number of frames.
      @see bufferSizeChanged(uint32_t)
    */
    uint32_t getBufferSize() const noexcept;

   /**
      Get the current sample rate that will be used during processing.@n
      This value will remain constant between activate and deactivate.
      @see sampleRateChanged(double)
    */
    double getSampleRate() const noexcept;

#if DISTRHO_PLUGIN_WANT_TIMEPOS
   /**
      Get the current host transport time position.@n
      This function should only be called during run().@n
      You can call this during other times, but the returned position is not guaranteed to be in sync.
      @note TimePosition is not supported in LADSPA and DSSI plugin formats.
    */
    const TimePosition& getTimePosition() const noexcept;
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
   /**
      Change the plugin audio output latency to @a frames.@n
      This function should only be called in the constructor, activate() and run().
      @note This function is only available if DISTRHO_PLUGIN_WANT_LATENCY is enabled.
    */
    void setLatency(uint32_t frames) noexcept;
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
   /**
      Write a MIDI output event.@n
      This function must only be called during run().@n
      Returns false when the host buffer is full, in which case do not call this again until the next run().
      @note This function is not implemented yet!@n
            It's here so that developers can prepare MIDI plugins in advance.@n
            If you plan to use this, please report to DPF authos so it can be implemented.
    */
    bool writeMidiEvent(const MidiEvent& midiEvent) noexcept;
#endif

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin name.@n
      Returns DISTRHO_PLUGIN_NAME by default.
    */
    virtual const char* getName() const { return DISTRHO_PLUGIN_NAME; }

   /**
      Get the plugin label.@n
      This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
    */
    virtual const char* getLabel() const = 0;

   /**
      Get an extensive comment/description about the plugin.@n
      Optional, returns nothing by default.
    */
    virtual const char* getDescription() const { return ""; }

   /**
      Get the plugin author/maker.
    */
    virtual const char* getMaker() const = 0;

   /**
      Get the plugin homepage.@n
      Optional, returns nothing by default.
    */
    virtual const char* getHomePage() const { return ""; }

   /**
      Get the plugin license (a single line of text or a URL).@n
      For commercial plugins this should return some short copyright information.
    */
    virtual const char* getLicense() const = 0;

   /**
      Get the plugin version, in hexadecimal.
      @see d_version()
    */
    virtual uint32_t getVersion() const = 0;

   /**
      Get the plugin unique Id.@n
      This value is used by LADSPA, DSSI and VST plugin formats.
      @see d_cconst()
    */
    virtual int64_t getUniqueId() const = 0;

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the audio port @a index.@n
      This function will be called once, shortly after the plugin is created.
    */
    virtual void initAudioPort(bool input, uint32_t index, AudioPort& port);

   /**
      Initialize the parameter @a index.@n
      This function will be called once, shortly after the plugin is created.
    */
    virtual void initParameter(uint32_t index, Parameter& parameter) = 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
   /**
      Set the name of the program @a index.@n
      This function will be called once, shortly after the plugin is created.@n
      Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_PROGRAMS is enabled.
    */
    virtual void initProgramName(uint32_t index, String& programName) = 0;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      Set the state key and default value of @a index.@n
      This function will be called once, shortly after the plugin is created.@n
      Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_STATE is enabled.
    */
    virtual void initState(uint32_t index, String& stateKey, String& defaultStateValue) = 0;
#endif

   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.@n
      The host may call this function from any context, including realtime processing.
    */
    virtual float getParameterValue(uint32_t index) const = 0;

   /**
      Change a parameter value.@n
      The host may call this function from any context, including realtime processing.@n
      When a parameter is marked as automable, you must ensure no non-realtime operations are performed.
      @note This function will only be called for parameter inputs.
    */
    virtual void setParameterValue(uint32_t index, float value) = 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
   /**
      Load a program.@n
      The host may call this function from any context, including realtime processing.@n
      Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_PROGRAMS is enabled.
    */
    virtual void loadProgram(uint32_t index) = 0;
#endif

#if DISTRHO_PLUGIN_WANT_FULL_STATE
   /**
      Get the value of an internal state.@n
      The host may call this function from any non-realtime context.@n
      Must be implemented by your plugin class if DISTRHO_PLUGIN_WANT_PROGRAMS or DISTRHO_PLUGIN_WANT_FULL_STATE is enabled.
      @note The use of this function breaks compatibility with the DSSI format.
    */
    virtual String getState(const char* key) const = 0;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      Change an internal state @a key to @a value.@n
      Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_STATE is enabled.
    */
    virtual void setState(const char* key, const char* value) = 0;
#endif

   /* --------------------------------------------------------------------------------------------------------
    * Audio/MIDI Processing */

   /**
      Activate this plugin.
    */
    virtual void activate() {}

   /**
      Deactivate this plugin.
    */
    virtual void deactivate() {}

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
   /**
      Run/process function for plugins with MIDI input.
      @note Some parameters might be null if there are no audio inputs/outputs or MIDI events.
    */
    virtual void run(const float** inputs, float** outputs, uint32_t frames,
                     const MidiEvent* midiEvents, uint32_t midiEventCount) = 0;
#else
   /**
      Run/process function for plugins without MIDI input.
      @note Some parameters might be null if there are no audio inputs or outputs.
    */
    virtual void run(const float** inputs, float** outputs, uint32_t frames) = 0;
#endif

   /* --------------------------------------------------------------------------------------------------------
    * Callbacks (optional) */

   /**
      Optional callback to inform the plugin about a buffer size change.@n
      This function will only be called when the plugin is deactivated.
      @note This value is only a hint!@n
            Hosts might call run() with a higher or lower number of frames.
      @see getBufferSize()
    */
    virtual void bufferSizeChanged(uint32_t newBufferSize);

   /**
      Optional callback to inform the plugin about a sample rate change.@n
      This function will only be called when the plugin is deactivated.
      @see getSampleRate()
    */
    virtual void sampleRateChanged(double newSampleRate);

    // -------------------------------------------------------------------------------------------------------

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class PluginExporter;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plugin)
};

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

/**
   @defgroup EntryPoints Entry Points
   @{
 */

/**
   TODO.
 */
extern Plugin* createPlugin();

/** @} */

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_PLUGIN_HPP_INCLUDED
