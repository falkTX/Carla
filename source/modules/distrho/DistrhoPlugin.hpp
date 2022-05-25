/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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
   Audio port can be used as control voltage (LV2 and JACK standalone only).
 */
static const uint32_t kAudioPortIsCV = 0x1;

/**
   Audio port should be used as sidechan (LV2 and VST3 only).
   This hint should not be used with CV style ports.
   @note non-sidechain audio ports must exist in the plugin if this flag is set.
 */
static const uint32_t kAudioPortIsSidechain = 0x2;

/**
   CV port has bipolar range (-1 to +1, or -5 to +5 if scaled).
   This is merely a hint to tell the host what value range to expect.
 */
static const uint32_t kCVPortHasBipolarRange = 0x10;

/**
   CV port has negative unipolar range (-1 to 0, or -10 to 0 if scaled).
   This is merely a hint to tell the host what value range to expect.
 */
static const uint32_t kCVPortHasNegativeUnipolarRange = 0x20;

/**
   CV port has positive unipolar range (0 to +1, or 0 to +10 if scaled).
   This is merely a hint to tell the host what value range to expect.
 */
static const uint32_t kCVPortHasPositiveUnipolarRange = 0x40;

/**
   CV port has scaled range to match real values (-5 to +5v bipolar, +/-10 to 0v unipolar).
   One other range flag is required if this flag is set.

   When enabled, this makes the port a mod:CVPort, compatible with the MOD Devices platform.
 */
static const uint32_t kCVPortHasScaledRange = 0x80;

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
   Parameter is automatable (real-time safe).
   @see Plugin::setParameterValue(uint32_t, float)
 */
static const uint32_t kParameterIsAutomatable = 0x01;

/** It was a typo, sorry.. */
DISTRHO_DEPRECATED_BY("kParameterIsAutomatable")
static const uint32_t kParameterIsAutomable = kParameterIsAutomatable;

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

   Parameter inputs are changed by the host and typically should not be changed by the plugin.@n
   One exception is when changing programs, see Plugin::loadProgram().@n
   The other exception is with parameter change requests, see Plugin::requestParameterValueChange().@n
   Outputs are changed by the plugin and never modified by the host.

   If you are targetting VST2, make sure to order your parameters so that all inputs are before any outputs.
 */
static const uint32_t kParameterIsOutput = 0x10;

/**
   Parameter value is a trigger.@n
   This means the value resets back to its default after each process/run call.@n
   Cannot be used for output parameters.

   @note Only officially supported under LV2. For other formats DPF simulates the behaviour.
*/
static const uint32_t kParameterIsTrigger = 0x20 | kParameterIsBoolean;

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * State Hints */

/**
   @defgroup StateHints State Hints

   Various state hints.
   @see State::hints
   @{
 */

/**
   State is visible and readable by hosts that support string-type plugin parameters.
 */
static const uint32_t kStateIsHostReadable = 0x01;

/**
   State is writable by the host, allowing users to arbitrarily change the state.@n
   For obvious reasons a writable state is also readable by the host.
 */
static const uint32_t kStateIsHostWritable = 0x02 | kStateIsHostReadable;

/**
   State is a filename path instead of a regular string.@n
   The readable and writable hints are required for filenames to work, and thus are automatically set.
 */
static const uint32_t kStateIsFilenamePath = 0x04 | kStateIsHostWritable;

/**
   State is a base64 encoded string.
 */
static const uint32_t kStateIsBase64Blob = 0x08;

/**
   State is for Plugin/DSP side only, meaning there is never a need to notify the UI when it changes.
 */
static const uint32_t kStateIsOnlyForDSP = 0x10;

/**
   State is for UI side only.@n
   If the DSP and UI are separate and the UI is not available, this property won't be saved.
 */
static const uint32_t kStateIsOnlyForUI = 0x20;

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Base Plugin structs */

/**
   @defgroup BasePluginStructs Base Plugin Structs
   @{
 */

/**
   Parameter designation.@n
   Allows a parameter to be specially designated for a task, like bypass.

   Each designation is unique, there must be only one parameter that uses it.@n
   The use of designated parameters is completely optional.

   @note Designated parameters have strict ranges.
   @see ParameterRanges::adjustForDesignation()
 */
enum ParameterDesignation {
   /**
     Null or unset designation.
    */
    kParameterDesignationNull = 0,

   /**
     Bypass designation.@n
     When on (> 0.5f), it means the plugin must run in a bypassed state.
    */
    kParameterDesignationBypass = 1
};

/**
   Predefined Port Groups Ids.

   This enumeration provides a few commonly used groups for convenient use in plugins.
   For preventing conflicts with user code, negative values are used here.
   When rolling your own port groups, you MUST start their group ids from 0 and they MUST be sequential.

   @see PortGroup
 */
enum PredefinedPortGroupsIds {
   /**
     Null or unset port group.
    */
    kPortGroupNone = (uint32_t)-1,

   /**
     A single channel audio group.
    */
    kPortGroupMono = (uint32_t)-2,

   /**
     A 2-channel discrete stereo audio group,
     where the 1st audio port is the left channel and the 2nd port is the right channel.
    */
    kPortGroupStereo = (uint32_t)-3
};

/**
   Audio Port.

   Can be used as CV port by specifying kAudioPortIsCV in hints,@n
   but this is only supported in LV2 and JACK standalone formats.
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
      The group id that this audio/cv port belongs to.
      No group is assigned by default.

      You can use a group from PredefinedPortGroups or roll your own.@n
      When rolling your own port groups, you MUST start their group ids from 0 and they MUST be sequential.
      @see PortGroup, Plugin::initPortGroup
    */
    uint32_t groupId;

   /**
      Default constructor for a regular audio port.
    */
    AudioPort() noexcept
        : hints(0x0),
          name(),
          symbol(),
          groupId(kPortGroupNone) {}
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
      Default constructor, using 0.0 as default, 0.0 as minimum, 1.0 as maximum.
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
    float getFixedValue(const float& value) const noexcept
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
   Parameter enumeration value.@n
   A string representation of a plugin parameter value.@n
   Used together can be used to give meaning to parameter values, working as an enumeration.
 */
struct ParameterEnumerationValue {
   /**
      Parameter value.
    */
    float value;

   /**
      String representation of this value.
    */
    String label;

   /**
      Default constructor, using 0.0 as value and empty label.
    */
    ParameterEnumerationValue() noexcept
        : value(0.0f),
          label() {}

   /**
      Constructor using custom values.
    */
    ParameterEnumerationValue(float v, const char* l) noexcept
        : value(v),
          label(l) {}
};

/**
   Collection of parameter enumeration values.@n
   Handy class to handle the lifetime and count of all enumeration values.
 */
struct ParameterEnumerationValues {
   /**
      Number of elements allocated in @values.
    */
    uint8_t count;

   /**
      Wherever the host is to be restricted to only use enumeration values.

      @note This mode is only a hint! Not all hosts and plugin formats support this mode.
    */
    bool restrictedMode;

   /**
      Array of @ParameterEnumerationValue items.@n
      This pointer must be null or have been allocated on the heap with `new ParameterEnumerationValue[count]`.
    */
    ParameterEnumerationValue* values;

   /**
      Default constructor, for zero enumeration values.
    */
    ParameterEnumerationValues() noexcept
        : count(0),
          restrictedMode(false),
          values() {}

   /**
      Constructor using custom values.@n
      The pointer to @values must have been allocated on the heap with `new`.
    */
    ParameterEnumerationValues(uint32_t c, bool r, ParameterEnumerationValue* v) noexcept
        : count(c),
          restrictedMode(r),
          values(v) {}

    ~ParameterEnumerationValues() noexcept
    {
        count = 0;
        restrictedMode = false;

        if (values != nullptr)
        {
            delete[] values;
            values = nullptr;
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE(ParameterEnumerationValues)
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
      The short name of this parameter.@n
      Used when displaying the parameter name in a very limited space.
      @note This value is optional, the full name is used when the short one is missing.
    */
    String shortName;

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
      An extensive description/comment about the parameter.
      @note This value is optional and only used for LV2.
    */
    String description;

   /**
      Ranges of this parameter.@n
      The ranges describe the default, minimum and maximum values.
    */
    ParameterRanges ranges;

   /**
      Enumeration values.@n
      Can be used to give meaning to parameter values, working as an enumeration.
    */
    ParameterEnumerationValues enumValues;

   /**
      Designation for this parameter.
    */
    ParameterDesignation designation;

   /**
      MIDI CC to use by default on this parameter.@n
      A value of 0 or 32 (bank change) is considered invalid.@n
      Must also be less or equal to 120.
      @note This value is only a hint! Hosts might map it automatically or completely ignore it.
    */
    uint8_t midiCC;

   /**
      The group id that this parameter belongs to.
      No group is assigned by default.

      You can use a group from PredefinedPortGroups or roll your own.@n
      When rolling your own port groups, you MUST start their group ids from 0 and they MUST be sequential.
      @see PortGroup, Plugin::initPortGroup
    */
    uint32_t groupId;

   /**
      Default constructor for a null parameter.
    */
    Parameter() noexcept
        : hints(0x0),
          name(),
          shortName(),
          symbol(),
          unit(),
          ranges(),
          enumValues(),
          designation(kParameterDesignationNull),
          midiCC(0),
          groupId(kPortGroupNone) {}

   /**
      Constructor using custom values.
    */
    Parameter(uint32_t h, const char* n, const char* s, const char* u, float def, float min, float max) noexcept
        : hints(h),
          name(n),
          shortName(),
          symbol(s),
          unit(u),
          ranges(def, min, max),
          enumValues(),
          designation(kParameterDesignationNull),
          midiCC(0),
          groupId(kPortGroupNone) {}

   /**
      Initialize a parameter for a specific designation.
    */
    void initDesignation(ParameterDesignation d) noexcept
    {
        designation = d;

        switch (d)
        {
        case kParameterDesignationNull:
            break;
        case kParameterDesignationBypass:
            hints      = kParameterIsAutomatable|kParameterIsBoolean|kParameterIsInteger;
            name       = "Bypass";
            shortName  = "Bypass";
            symbol     = "dpf_bypass";
            unit       = "";
            midiCC     = 0;
            groupId    = kPortGroupNone;
            ranges.def = 0.0f;
            ranges.min = 0.0f;
            ranges.max = 1.0f;
            break;
        }
    }
};

/**
   Port Group.@n
   Allows to group together audio/cv ports or parameters.

   Each unique group MUST have an unique symbol and a name.
   A group can be applied to both inputs and outputs (at the same time).
   The same group cannot be used in audio ports and parameters.

   An audio port group logically combines ports which should be considered part of the same stream.@n
   For example, two audio ports in a group may form a stereo stream.

   A parameter group provides meta-data to the host to indicate that some parameters belong together.

   The use of port groups is completely optional.

   @see Plugin::initPortGroup, AudioPort::group, Parameter::group
 */
struct PortGroup {
   /**
      The name of this port group.@n
      A port group name can contain any character, but hosts might have a hard time with non-ascii ones.@n
      The name doesn't have to be unique within a plugin instance, but it's recommended.
    */
    String name;

   /**
      The symbol of this port group.@n
      A port group symbol is a short restricted name used as a machine and human readable identifier.@n
      The first character must be one of _, a-z or A-Z and subsequent characters can be from _, a-z, A-Z and 0-9.
      @note Port group symbols MUST be unique within a plugin instance.
    */
    String symbol;
};

/**
   State.

   In DPF states refer to key:value string pairs, used to store arbitrary non-parameter data.@n
   By default states are completely internal to the plugin and not visible by the host.@n
   Flags can be set to allow hosts to see and/or change them.

   TODO API under construction
 */
struct State {
   /**
      Hints describing this state.
      @note Changing these hints can break compatibility with previously saved data.
      @see StateHints
    */
    uint32_t hints;

   /**
      The key or "symbol" of this state.@n
      A state key is a short restricted name used as a machine and human readable identifier.
      @note State keys MUST be unique within a plugin instance.
      TODO define rules for allowed characters, must be usable as URI non-encoded parameters
    */
    String key;

   /**
      The default value of this state.@n
      Can be left empty if considered a valid initial state.
    */
    String defaultValue;

   /**
      String representation of this state.
    */
    String label;

   /**
      An extensive description/comment about this state.
      @note This value is optional and only used for LV2.
    */
    String description;
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

      When dataExt is used, the event holder is responsible for
      keeping the pointer valid during the entirety of the run function.
    */
    uint8_t        data[kDataSize];
    const uint8_t* dataExt;
};

/**
   Time position.@n
   The @a playing and @a frame values are always valid.@n
   BBT values are only valid when @a bbt.valid is true.

   This struct is inspired by the [JACK Transport API](https://jackaudio.org/api/structjack__position__t.html).
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
          Should always be >= 0 and < @a ticksPerBeat.@n
          The first tick is tick '0'.
          @note Fraction part of tick is only available on some plugin formats.
        */
        double tick;

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
          Number of ticks within a beat.@n
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

       /**
          Reinitialize this position using the default null initialization.
        */
        void clear() noexcept
        {
            valid = false;
            bar = 0;
            beat = 0;
            tick = 0;
            barStartTick = 0.0;
            beatsPerBar = 0.0f;
            beatType = 0.0f;
            ticksPerBeat = 0.0;
            beatsPerMinute = 0.0;
        }
    } bbt;

   /**
      Default constructor for a time position.
    */
    TimePosition() noexcept
        : playing(false),
          frame(0),
          bbt() {}

   /**
      Reinitialize this position using the default null initialization.
    */
    void clear() noexcept
    {
        playing  = false;
        frame = 0;
        bbt.clear();
    }
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

   /**
      Get the bundle path where the plugin resides.
      Can return null if the plugin is not available in a bundle (if it is a single binary).
      @see getBinaryFilename
      @see getResourcePath
    */
    const char* getBundlePath() const noexcept;

   /**
      Check if this plugin instance is a "dummy" one used for plugin meta-data/information export.@n
      When true no processing will be done, the plugin is created only to extract information.@n
      In DPF, LADSPA/DSSI, VST2 and VST3 formats create one global instance per plugin binary
      while LV2 creates one when generating turtle meta-data.
    */
    bool isDummyInstance() const noexcept;

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
    */
    bool writeMidiEvent(const MidiEvent& midiEvent) noexcept;
#endif

#if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
   /**
      Check if parameter value change requests will work with the current plugin host.
      @note This function is only available if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST is enabled.
      @see requestParameterValueChange(uint32_t, float)
    */
    bool canRequestParameterValueChanges() const noexcept;

   /**
      Request a parameter value change from the host.
      If successful, this function will automatically trigger a parameter update on the UI side as well.
      This function can fail, for example if the host is busy with the parameter for read-only automation.
      Some hosts simply do not have this functionality, which can be verified with canRequestParameterValueChanges().
      @note This function is only available if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST is enabled.
    */
    bool requestParameterValueChange(uint32_t index, float value) noexcept;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      Set state value and notify the host about the change.@n
      This function will call `setState()` and also trigger an update on the UI side as necessary.@n
      It must not be called during run.@n
      The state must be host readable.
      @note this function does nothing on DSSI plugin format, as DSSI only supports UI->DSP messages.

      TODO API under construction
    */
    bool updateStateValue(const char* key, const char* value) noexcept;
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
    virtual void initParameter(uint32_t index, Parameter& parameter);

   /**
      Initialize the port group @a groupId.@n
      This function will be called once,
      shortly after the plugin is created and all audio ports and parameters have been enumerated.
    */
    virtual void initPortGroup(uint32_t groupId, PortGroup& portGroup);

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
      Initialize the state @a index.@n
      This function will be called once, shortly after the plugin is created.@n
      Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_STATE is enabled.
    */
    virtual void initState(uint32_t index, State& state);

    DISTRHO_DEPRECATED_BY("initState(uint32_t,State&)")
    virtual void initState(uint32_t, String&, String&) {}

    DISTRHO_DEPRECATED_BY("initState(uint32_t,State&)")
    virtual bool isStateFile(uint32_t) { return false; }
#endif

   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.@n
      The host may call this function from any context, including realtime processing.
    */
    virtual float getParameterValue(uint32_t index) const;

   /**
      Change a parameter value.@n
      The host may call this function from any context, including realtime processing.@n
      When a parameter is marked as automatable, you must ensure no non-realtime operations are performed.
      @note This function will only be called for parameter inputs.
    */
    virtual void setParameterValue(uint32_t index, float value);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
   /**
      Load a program.@n
      The host may call this function from any context, including realtime processing.@n
      Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_PROGRAMS is enabled.
    */
    virtual void loadProgram(uint32_t index);
#endif

#if DISTRHO_PLUGIN_WANT_FULL_STATE
   /**
      Get the value of an internal state.@n
      The host may call this function from any non-realtime context.@n
      Must be implemented by your plugin class if DISTRHO_PLUGIN_WANT_FULL_STATE is enabled.
      @note The use of this function breaks compatibility with the DSSI format.
    */
    virtual String getState(const char* key) const;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      Change an internal state @a key to @a value.@n
      Must be implemented by your plugin class only if DISTRHO_PLUGIN_WANT_STATE is enabled.
    */
    virtual void setState(const char* key, const char* value);
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
    virtual void run(const float* const* inputs, float** outputs, uint32_t frames,
                     const MidiEvent* midiEvents, uint32_t midiEventCount) = 0;
#else
   /**
      Run/process function for plugins without MIDI input.
      @note Some parameters might be null if there are no audio inputs or outputs.
    */
    virtual void run(const float* const* inputs, float** outputs, uint32_t frames) = 0;
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
   Create an instance of the Plugin class.@n
   This is the entry point for DPF plugins.@n
   DPF will call this to either create an instance of your plugin for the host
   or to fetch some initial information for internal caching.
 */
extern Plugin* createPlugin();

/** @} */

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_PLUGIN_HPP_INCLUDED
