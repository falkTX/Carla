/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_DETAILS_HPP_INCLUDED
#define DISTRHO_DETAILS_HPP_INCLUDED

#include "extra/String.hpp"

START_NAMESPACE_DISTRHO

/* --------------------------------------------------------------------------------------------------------------------
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
static constexpr const uint32_t kAudioPortIsCV = 0x1;

/**
   Audio port should be used as sidechan (LV2 and VST3 only).
   This hint should not be used with CV style ports.
   @note non-sidechain audio ports must exist in the plugin if this flag is set.
 */
static constexpr const uint32_t kAudioPortIsSidechain = 0x2;

/**
   CV port has bipolar range (-1 to +1, or -5 to +5 if scaled).
   This is merely a hint to tell the host what value range to expect.
 */
static constexpr const uint32_t kCVPortHasBipolarRange = 0x10;

/**
   CV port has negative unipolar range (-1 to 0, or -10 to 0 if scaled).
   This is merely a hint to tell the host what value range to expect.
 */
static constexpr const uint32_t kCVPortHasNegativeUnipolarRange = 0x20;

/**
   CV port has positive unipolar range (0 to +1, or 0 to +10 if scaled).
   This is merely a hint to tell the host what value range to expect.
 */
static constexpr const uint32_t kCVPortHasPositiveUnipolarRange = 0x40;

/**
   CV port has scaled range to match real values (-5 to +5v bipolar, +/-10 to 0v unipolar).
   One other range flag is required if this flag is set.

   When enabled, this makes the port a mod:CVPort, compatible with the MOD Devices platform.
 */
static constexpr const uint32_t kCVPortHasScaledRange = 0x80;

/**
   CV port is optional, allowing hosts that do no CV ports to load the plugin.
   When loaded in hosts that don't support CV, the float* buffer for this port will be null.
 */
static constexpr const uint32_t kCVPortIsOptional = 0x100;

/** @} */

/* --------------------------------------------------------------------------------------------------------------------
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
static constexpr const uint32_t kParameterIsAutomatable = 0x01;

/** It was a typo, sorry.. */
DISTRHO_DEPRECATED_BY("kParameterIsAutomatable")
static constexpr const uint32_t kParameterIsAutomable = kParameterIsAutomatable;

/**
   Parameter value is boolean.@n
   It's always at either minimum or maximum value.
 */
static constexpr const uint32_t kParameterIsBoolean = 0x02;

/**
   Parameter value is integer.
 */
static constexpr const uint32_t kParameterIsInteger = 0x04;

/**
   Parameter value is logarithmic.
 */
static constexpr const uint32_t kParameterIsLogarithmic = 0x08;

/**
   Parameter is of output type.@n
   When unset, parameter is assumed to be of input type.

   Parameter inputs are changed by the host and typically should not be changed by the plugin.@n
   One exception is when changing programs, see Plugin::loadProgram().@n
   The other exception is with parameter change requests, see Plugin::requestParameterValueChange().@n
   Outputs are changed by the plugin and never modified by the host.

   If you are targetting VST2, make sure to order your parameters so that all inputs are before any outputs.
 */
static constexpr const uint32_t kParameterIsOutput = 0x10;

/**
   Parameter value is a trigger.@n
   This means the value resets back to its default after each process/run call.@n
   Cannot be used for output parameters.

   @note Only officially supported under LV2. For other formats DPF simulates the behaviour.
*/
static constexpr const uint32_t kParameterIsTrigger = 0x20 | kParameterIsBoolean;

/**
   Parameter should be hidden from the host and user-visible GUIs.@n
   It is still saved and handled as any regular parameter, just not visible to the user
   (for example in a host generated GUI)
 */
static constexpr const uint32_t kParameterIsHidden = 0x40;

/** @} */

/* --------------------------------------------------------------------------------------------------------------------
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
static constexpr const uint32_t kStateIsHostReadable = 0x01;

/**
   State is writable by the host, allowing users to arbitrarily change the state.@n
   For obvious reasons a writable state is also readable by the host.
 */
static constexpr const uint32_t kStateIsHostWritable = 0x02 | kStateIsHostReadable;

/**
   State is a filename path instead of a regular string.@n
   The readable and writable hints are required for filenames to work, and thus are automatically set.
 */
static constexpr const uint32_t kStateIsFilenamePath = 0x04 | kStateIsHostWritable;

/**
   State is a base64 encoded string.
 */
static constexpr const uint32_t kStateIsBase64Blob = 0x08;

/**
   State is for Plugin/DSP side only, meaning there is never a need to notify the UI when it changes.
 */
static constexpr const uint32_t kStateIsOnlyForDSP = 0x10;

/**
   State is for UI side only.@n
   If the DSP and UI are separate and the UI is not available, this property won't be saved.
 */
static constexpr const uint32_t kStateIsOnlyForUI = 0x20;

/** @} */

/* --------------------------------------------------------------------------------------------------------------------
 * Base Plugin structs */

/**
   @defgroup BasePluginStructs Base Plugin Structs
   @{
 */

/**
   Parameter designation.@n
   Allows a parameter to be specially designated for a task, like bypass and reset.

   Each designation is unique, there must be only one parameter that uses it.@n
   The use of designated parameters is completely optional.

   @note Designated parameters have strict ranges.
   @see ParameterRanges::adjustForDesignation()
 */
enum ParameterDesignation {
   /**
     Null or unset designation.
    */
    kParameterDesignationNull,

   /**
     Bypass designation.@n
     When on (> 0.5f), it means the plugin must run in a bypassed state.
    */
    kParameterDesignationBypass,

   /**
     Reset designation.@n
     When on (> 0.5f), it means the plugin should reset its internal processing state
     (like filters, oscillators, envelopes, lfos, etc) and kill all voices.
    */
    kParameterDesignationReset,
};

/**
   Parameter designation symbols.@n
   These are static, hard-coded definitions to ensure consistency across DPF and plugins.
*/
namespace ParameterDesignationSymbols {
   /**
     Bypass designation symbol.
    */
   static constexpr const char bypass[] = "dpf_bypass";

   /**
     Reset designation symbol.
    */
   static constexpr const char reset[] = "dpf_reset";

   /**
     LV2 bypass designation symbol, inverted for LV2 so it becomes "enabled".
    */
   static constexpr const char bypass_lv2[] = "lv2_enabled";
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
    constexpr ParameterRanges() noexcept
        : def(0.0f),
          min(0.0f),
          max(1.0f) {}

   /**
      Constructor using custom values.
    */
    constexpr ParameterRanges(const float df, const float mn, const float mx) noexcept
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
    float getFixedValue(const float value) const noexcept
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
    float getNormalizedValue(const float value) const noexcept
    {
        const float normValue = (value - min) / (max - min);

        if (normValue <= 0.0f)
            return 0.0f;
        if (normValue >= 1.0f)
            return 1.0f;
        return normValue;
    }

   /**
      Get a value normalized to 0.0<->1.0.
      Overloaded function using double precision values.
    */
    double getNormalizedValue(const double& value) const noexcept
    {
        const double normValue = (value - min) / (max - min);

        if (normValue <= 0.0)
            return 0.0;
        if (normValue >= 1.0)
            return 1.0;
        return normValue;
    }

   /**
      Get a value normalized to 0.0<->1.0, fixed within range.
    */
    float getFixedAndNormalizedValue(const float value) const noexcept
    {
        if (value <= min)
            return 0.0f;
        if (value >= max)
            return 1.0f;

        const float normValue = (value - min) / (max - min);

        if (normValue <= 0.0f)
            return 0.0f;
        if (normValue >= 1.0f)
            return 1.0f;

        return normValue;
    }

   /**
      Get a value normalized to 0.0<->1.0, fixed within range.
      Overloaded function using double precision values.
    */
    double getFixedAndNormalizedValue(const double value) const noexcept
    {
        if (value <= min)
            return 0.0;
        if (value >= max)
            return 1.0;

        const double normValue = (value - min) / (max - min);

        if (normValue <= 0.0)
            return 0.0;
        if (normValue >= 1.0)
            return 1.0;

        return normValue;
    }

   /**
      Get a proper value previously normalized to 0.0<->1.0.
    */
    float getUnnormalizedValue(const float value) const noexcept
    {
        if (value <= 0.0f)
            return min;
        if (value >= 1.0f)
            return max;

        return value * (max - min) + min;
    }

   /**
      Get a proper value previously normalized to 0.0<->1.0.
      Overloaded function using double precision values.
    */
    double getUnnormalizedValue(const double value) const noexcept
    {
        if (value <= 0.0)
            return min;
        if (value >= 1.0)
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
   Details around parameter enumeration values.@n
   Wraps ParameterEnumerationValues and provides a few extra details to the host about these values.
 */
struct ParameterEnumerationValues {
   /**
      Number of elements allocated in @values.
    */
    uint8_t count;

   /**
      Whether the host is to be restricted to only use enumeration values.

      @note This mode is only a hint! Not all hosts and plugin formats support this mode.
    */
    bool restrictedMode;

   /**
      Array of @ParameterEnumerationValue items.@n
      When assining this pointer manually, it must be allocated on the heap with `new ParameterEnumerationValue[count]`.@n
      The array pointer will be automatically deleted later unless @p deleteLater is set to false.
    */
    ParameterEnumerationValue* values;

   /**
      Whether to take ownership of the @p values pointer.@n
      Defaults to true unless stated otherwise.
    */
    bool deleteLater;

   /**
      Default constructor, for zero enumeration values.
    */
    constexpr ParameterEnumerationValues() noexcept
        : count(0),
          restrictedMode(false),
          values(nullptr),
          deleteLater(true) {}

   /**
      Constructor using custom values.@n
      When using this constructor the pointer to @values MUST have been statically declared.@n
      It will not be automatically deleted later.
    */
    constexpr ParameterEnumerationValues(uint8_t c, bool r, ParameterEnumerationValue* v) noexcept
        : count(c),
          restrictedMode(r),
          values(v),
          deleteLater(false) {}

    // constexpr
    ~ParameterEnumerationValues() noexcept
    {
        if (deleteLater)
            delete[] values;
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
      Enumeration details.@n
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
          description(),
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
          description(),
          ranges(def, min, max),
          enumValues(),
          designation(kParameterDesignationNull),
          midiCC(0),
          groupId(kPortGroupNone) {}

#ifdef DISTRHO_PROPER_CPP11_SUPPORT
   /**
      Constructor using custom values and enumeration.
      Assumes enumeration details should have `restrictedMode` on.
    */
    Parameter(uint32_t h, const char* n, const char* s, const char* u, float def, float min, float max,
              uint8_t evcount, ParameterEnumerationValue ev[]) noexcept
        : hints(h),
          name(n),
          shortName(),
          symbol(s),
          unit(u),
          description(),
          ranges(def, min, max),
          enumValues(evcount, true, ev),
          designation(kParameterDesignationNull),
          midiCC(0),
          groupId(kPortGroupNone) {}
#endif

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
            symbol     = ParameterDesignationSymbols::bypass;
            unit       = "";
            midiCC     = 0;
            groupId    = kPortGroupNone;
            ranges.def = 0.0f;
            ranges.min = 0.0f;
            ranges.max = 1.0f;
            break;
        case kParameterDesignationReset:
            hints      = kParameterIsAutomatable|kParameterIsBoolean|kParameterIsInteger|kParameterIsTrigger;
            name       = "Reset";
            shortName  = "Reset";
            symbol     = ParameterDesignationSymbols::reset;
            unit       = "";
            midiCC     = 0;
            groupId    = kPortGroupNone;
            ranges.def = 0.0f;
            ranges.min = 0.0f;
            ranges.max = 1.0f;
            break;
        }
    }

    Parameter& operator=(Parameter& other) noexcept
    {
         hints = other.hints;
         name = other.name;
         shortName = other.shortName;
         symbol = other.symbol;
         unit = other.unit;
         description = other.description;
         ranges = other.ranges;
         designation = other.designation;
         midiCC = other.midiCC;
         groupId = other.groupId;

         // enumValues needs special handling
         enumValues.count = other.enumValues.count;
         enumValues.restrictedMode = other.enumValues.restrictedMode;
         enumValues.values = other.enumValues.values;
         enumValues.deleteLater = other.enumValues.deleteLater;

         // make sure to not delete data twice
         other.enumValues.deleteLater = false;

         return *this;
    }

    Parameter& operator=(const Parameter& other) noexcept
    {
         hints = other.hints;
         name = other.name;
         shortName = other.shortName;
         symbol = other.symbol;
         unit = other.unit;
         description = other.description;
         ranges = other.ranges;
         designation = other.designation;
         midiCC = other.midiCC;
         groupId = other.groupId;

         // make sure to not delete data twice
         DISTRHO_SAFE_ASSERT_RETURN(other.enumValues.values == nullptr || !other.enumValues.deleteLater, *this);

         // enumValues needs special handling
         enumValues.count = other.enumValues.count;
         enumValues.restrictedMode = other.enumValues.restrictedMode;
         enumValues.values = other.enumValues.values;
         enumValues.deleteLater = other.enumValues.deleteLater;

         return *this;
    }
};

/**
   Port Group.@n
   Allows to group together audio/cv ports or parameters.

   Each unique group MUST have an unique symbol and a name.
   A group can be applied to both inputs and outputs (at the same time).
   The same group cannot be used in audio ports and parameters.

   When both audio and parameter groups are used, audio groups MUST be defined first.
   That is, group indexes start with audio ports, then parameters.

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

   #ifdef __MOD_DEVICES__
   /**
      The file types that a filename path state supports, written as a comma-separated string without whitespace.
      Currently supported file types are:
         - audioloop: Audio Loops, meant to be used for looper-style plugins
         - audiorecording: Audio Recordings, triggered by plugins and stored in the unit
         - audiosample: One-shot Audio Samples, meant to be used for sampler-style plugins
         - audiotrack: Audio Tracks, meant to be used as full-performance/song or backtrack
         - cabsim: Speaker Cabinets, meant as small IR audio files
         - h2drumkit: Hydrogen Drumkits, must use h2drumkit file extension
         - ir: Impulse Responses
         - midiclip: MIDI Clips, to be used in sync with host tempo, must have mid or midi file extension
         - midisong: MIDI Songs, meant to be used as full-performance/song or backtrack
         - sf2: SF2 Instruments, must have sf2 or sf3 file extension
         - sfz: SFZ Instruments, must have sfz file extension

      @note This is a custom extension only valid in builds MOD Audio.
    */
    String fileTypes;
   #endif

   /**
      Default constructor for a null state.
    */
    State() noexcept
        : hints(0x0),
          key(),
          defaultValue(),
          label(),
          description() {}
};

/**
   MIDI event.
 */
struct MidiEvent {
   /**
      Size of internal data.
    */
    static constexpr const uint32_t kDataSize = 4;

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
      @note This value is not always monotonic,
            with some plugin hosts assigning it based on a source that can accumulate rounding errors.
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

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_DETAILS_HPP_INCLUDED
