/*
 * DAZ - Digital Audio with Zero dependencies
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2013 Harry van Haaren <harryhaaren@gmail.com>
 * Copyright (C) 2013 Jonathan Moore Liles <male@tuxfamily.org>
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

#ifndef DAZ_PLUGIN_H_INCLUDED
#define DAZ_PLUGIN_H_INCLUDED

#include "daz-common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @defgroup DAZPluginAPI
 * @{
 */

/* ------------------------------------------------------------------------------------------------------------
 * Plugin Categories */

/*!
 * @defgroup PluginCategories Plugin Categories
 *
 * A small list of pre-defined plugin categories.
 *
 * Plugins should provide at least one of these basic categories, terminating with ":".
 * They can use their own custom categories as well, as long as they are lowercase and contain ASCII characters only.
 * @{
 */

/*!
 * A synthesizer or generator.
 */
#define PLUGIN_CATEGORY_SYNTH ":synth"

/*!
 * A delay or reverberator.
 */
#define PLUGIN_CATEGORY_DELAY ":delay"

/*!
 * An equalizer.
 */
#define PLUGIN_CATEGORY_EQ ":eq"

/*!
 * A filter.
 */
#define PLUGIN_CATEGORY_FILTER ":filter"

/*!
 * A distortion plugin.
 */
#define PLUGIN_CATEGORY_DISTORTION ":distortion"

/*!
 * A 'dynamic' plugin (amplifier, compressor, gate, etc).
 */
#define PLUGIN_CATEGORY_DYNAMICS ":dynamics"

/*!
 * A 'modulator' plugin (chorus, flanger, phaser, etc).
 */
#define PLUGIN_CATEGORY_MODULATOR ":modulator"

/*!
 * An 'utility' plugin (analyzer, converter, mixer, etc).
 */
#define PLUGIN_CATEGORY_UTILITY ":utility"

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Plugin Features */

/*!
 * @defgroup PluginFeatures Plugin Features
 *
 * A list of plugin features or hints.
 *
 * Plugins should list all features it supports, terminating with ":"
 * Custom features are allowed, as long as they are lowercase and contain ASCII characters only.
 * The host will decide if it can load the plugin or not based on this information.
 * @{
 */

/*!
 * Supports buffer size changes on-the-fly.
 *
 * If unset, the host will re-initiate the plugin when the buffer size changes.
 */
#define PLUGIN_FEATURE_BUFFER_SIZE_CHANGES ":buffer_size_changes"

/*!
 * Supports sample rate changes on-the-fly.
 *
 * If unset, the host will re-initiate the plugin when the sample rate changes.
 */
#define PLUGIN_FEATURE_SAMPLE_RATE_CHANGES ":sample_rate_changes"

/*!
 * Needs non-realtime idle() function regularly.
 *
 * This can be used by plugins that need a non-realtime thread to do work.
 * The host will call PluginDescriptor::idle() at regular intervals.
 * The plugin may acquire a lock, but MUST NOT block indefinitely.
 *
 * Alternatively, the plugin can ask the host for a one-shot idle()
 * by using HOST_OPCODE_NEEDS_IDLE.
 */
#define PLUGIN_FEATURE_IDLE ":idle"

/*!
 * Supports get_state() and set_state() functions.
 */
#define PLUGIN_FEATURE_STATE ":state"

/*!
 * Uses get_time_info() host function.
 */
#define PLUGIN_FEATURE_TIME ":time"

/*!
 * Uses write_event() host function.
 */
#define PLUGIN_FEATURE_WRITE_EVENT ":write_event"

/*!
 * Uses send_ui_msg() host function.
 */
#define PLUGIN_FEATURE_SEND_MSG ":sendmsg"

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Plugin Supports */

/*!
 * TODO - this needs a better name...
 *
 * @defgroup PluginSupports Plugin Supports
 *
 * A list of plugin supported MIDI events.
 *
 * Plugins should list all the MIDI message types it supports, terminating with ":".
 * @{
 */

/*!
 * Handles MIDI programs internally instead of host-exposed/exported.
 *
 * When this is set, the host will not try to map MIDI program changes into
 * plugin exported programs by sending MidiProgramEvent, but will send MidiEvent directly.
 *
 * @see MidiProgram, MidiProgramEvent
 */
#define PLUGIN_SUPPORTS_PROGRAM_CHANGES ":program"

/*!
 * Supports control changes (0xB0).
 *
 * @note:
 * The plugin MUST NEVER change exposed parameters on its own.
 * If the plugin wants to map a MIDI control change message to a parameter
 * it can do so by reporting it in the meta-data, which the host will read.
 */
#define PLUGIN_SUPPORTS_CONTROL_CHANGES ":control"

/*!
 * Supports channel pressure (0xD0).
 */
#define PLUGIN_SUPPORTS_CHANNEL_PRESSURE ":pressure"

/*!
 * Supports note aftertouch (0xA0).
 */
#define PLUGIN_SUPPORTS_NOTE_AFTERTOUCH ":aftertouch"

/*!
 * Supports pitchbend (0xE0).
 */
#define PLUGIN_SUPPORTS_PITCHBEND ":pitchbend"

/*!
 * Supports all-sound-off and all-notes-off events.
 *
 * When this is not set, the host might want to send various note-off events to silence the plugin.
 */
#define PLUGIN_SUPPORTS_ALL_SOUND_OFF ":allsoundoff"

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Parameter Hints */

/*!
 * @defgroup ParameterHints Parameter Hints
 *
 * List of parameter hints.
 *
 * < something something >, terminating with ":".
 * @{
 */

/*!
 * Is enabled.
 *
 * If set the host may show this parameter on its "built-in" dialog.
 */
#define PARAMETER_IS_ENABLED ":enabled"

/*!
 * Is output.
 *
 * If this is not set, the parameter should be considered input.
 *
 * Input parameters are managed by the host and changed by sending a ParameterEvent to the plugin.
 * The plugin MUST NEVER change input parameters on its own.
 *
 * Output parameters are managed by the plugin.
 * Most plugins that have output parameters should set PLUGIN_FEATURE_WRITE_EVENT,
 * see PARAMETER_IS_RTSAFE for details.
 */
#define PARAMETER_IS_OUTPUT ":output"

/*!
 * Is not real time safe.
 *
 * For input parameters:
 * When set, the host MUST ONLY use PluginDescriptor::non_rt_event().
 * When not set (default), the host MUST ONLY use in-process events to change this parameter.
 *
 * For output parameters:
 * When set, the host will call PluginDescriptor::get_parameter_value() where the plugin is allowed to lock.
 * When not set (default), the plugin must send a ParameterEvent to the host every time the value changes.
 *
 * @see PLUGIN_FEATURE_RTSAFE
 */
#define PARAMETER_IS_NON_RT ":non_rt"

/*!
 * Values are boolean (always at minimum or maximum values).
 */
#define PARAMETER_IS_BOOLEAN ":boolean"

/*!
 * Values are integer.
 */
#define PARAMETER_IS_INTEGER ":integer"

/*!
 * Values are logarithmic.
 */
#define PARAMETER_IS_LOGARITHMIC ":logarithmic"

/*!
 * Needs sample rate to work.
 *
 * The parameter value and ranges are multiplied by sample rate on usage
 * and divided by sample rate on save.
 */
#define PARAMETER_USES_SAMPLE_RATE ":sample_rate"

/*!
 * Uses scalepoints to define internal values in a meaningful way.
 */
#define PARAMETER_USES_SCALEPOINTS ":scalepoints"

/*!
 * Uses custom text for displaying its value.
 *
 * @see get_parameter_text()
 */
#define PARAMETER_USES_CUSTOM_TEXT ":custom_text"

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Default Parameter Ranges */

/*!
 * @defgroup DefaultParameterRanges Default Parameter Ranges
 *
 * Default values for parameter range steps.
 * @{
 */
#define PARAMETER_RANGE_DEFAULT_STEP       0.01f
#define PARAMETER_RANGE_DEFAULT_STEP_SMALL 0.0001f
#define PARAMETER_RANGE_DEFAULT_STEP_LARGE 0.1f
/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Event Types */

/*!
 * @defgroup EventTypes Event Types
 *
 * List of supported event types.
 *
 * The types are mapped into mapped_value_t by the host.
 * @see PluginHostDescriptor::map_value()
 * @{
 */

/*!
 * Generic MIDI event.
 *
 * Realtime MIDI events are always used in-process,
 * while non realtime ones should be used in PluginDescriptor::non_rt_event().
 *
 * @see MidiEvent
 */
#define EVENT_TYPE_MIDI "midi"

/*!
 * Midi program event.
 *
 * Used in-process only.
 *
 * @see MidiProgramEvent
 */
#define EVENT_TYPE_MIDI_PROGRAM "midiprogram"

/*!
 * Parameter event.
 *
 * There are some rules for parameter events,
 * please see PARAMETER_IS_RTSAFE.
 *
 * @see ParameterEvent
 */
#define EVENT_TYPE_PARAMETER "parameter"

/*!
 * Time information event.
 *
 * Used in-process only.
 *
 * @see TimeInfoEvent
 */
#define EVENT_TYPE_TIME "time"

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Host Dispatcher Opcodes */

/*!
 * @defgroup HostDispatcherOpcodes Host Dispatcher Opcodes
 *
 * Opcodes dispatched by the plugin to report and request information from the host.
 *
 * The opcodes are mapped into MappedValue by the host.
 * @see HostDescriptor::dispatcher()
 * @{
 */

/*!
 * Tell the host to call idle() as soon as possible (once), uses nothing.
 */
#define HOST_OPCODE_NEEDS_IDLE "needsIdle"

/*!
 * Tell the host to update parameter @a index.
 * Uses index with -1 for all.
 */
#define HOST_OPCODE_UPDATE_PARAMETER "updateParameter"

/*!
 * Tell the host to update midi-program @a index.
 * Uses index with -1 for all.
 * May also use value for channel. <= FIXME: maybe remove this bit, but for synths it's nice to have
 */
#define HOST_OPCODE_UPDATE_MIDI_PROGRAM "updateMidiProgram"

/*!
 * Tell the host to reload all parameters data, uses nothing.
 */
#define HOST_OPCODE_RELOAD_PARAMETERS "reloadParameters"

/*!
 * Tell the host to reload all midi-programs data, uses nothing.
 */
#define HOST_OPCODE_RELOAD_MIDI_PROGRAMS "reloadMidiPrograms"

/*!
 * Tell the host to reload everything all the plugin, uses nothing.
 */
#define HOST_OPCODE_RELOAD_ALL "reloadAll"

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Plugin Dispatcher Opcodes */

/*!
 * @defgroup PluginDispatcherOpcodes Plugin Dispatcher Opcodes
 *
 * Opcodes dispatched by the host to report changes to the plugin.
 *
 * The opcodes are mapped into MappedValue by the host.
 * @see PluginDescriptor::dispatcher()
 * @{
 */

/*!
 * Message received, uses ptr as char*.
 */
#define PLUGIN_OPCODE_MSG_RECEIVED "msgReceived"

/*!
 * Audio buffer size changed, uses value, returns 1 if supported.
 * @see PluginHostDescriptor::buffer_size
 */
#define PLUGIN_OPCODE_BUFFER_SIZE_CHANGED "bufferSizeChanged"

/*!
 * Audio sample rate changed, uses opt, returns 1 if supported.
 * @see PluginHostDescriptor::sample_rate
 */
#define PLUGIN_OPCODE_SAMPLE_RATE_CHANGED "sampleRateChanged"

/*!
 * Offline mode changed, uses value (0=off, 1=on).
 * @see PluginHostDescriptor::is_offline
 */
#define PLUGIN_OPCODE_OFFLINE_CHANGED "offlineChanged"

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Base types */

/*!
 * Audio sample type.
 */
typedef float audio_sample_t;

/*!
 * Opaque plugin handle.
 */
typedef void* PluginHandle;

/*!
 * Opaque host handle.
 */
typedef void* PluginHostHandle;

/*!
 * Parameter scale point.
 */
typedef struct {
    const char* label; /*!< not null */
    float value;
} ParameterScalePoint;

/*!
 * Parameter ranges.
 */
typedef struct {
    float def;
    float min;
    float max;
#if 1
    float step;
    float stepSmall;
    float stepLarge;
#endif
} ParameterRanges;

/*!
 * Parameter.
 */
typedef struct {
    const char* hints;  /*!< not null, @see ParameterHints */
    const char* name;   /*!< not null */
    const char* symbol; /*!< not null */
    const char* unit;   /*!< can be null */
    ParameterRanges ranges;

    uint32_t scalePointCount;
    ParameterScalePoint* scalePoints;
} Parameter;

/*!
 * MIDI Program.
 */
typedef struct {
    uint32_t bank;
    uint32_t program;
    const char* name;
} MidiProgram;

/* ------------------------------------------------------------------------------------------------------------
 * TimeInfo related types */

/*!
 * Bar-Beat-Tick information.
 *
 * @note this is the same data provided by JACK
 */
typedef struct {
    bool valid;

    int32_t bar;  /*!< current bar */
    int32_t beat; /*!< current beat-within-bar */
    int32_t tick; /*!< current tick-within-beat */
    double barStartTick;

    float beatsPerBar; /*!< time signature "numerator" */
    float beatType;    /*!< time signature "denominator" */

    double ticksPerBeat;
    double beatsPerMinute;
} TimeInfoBBT;

/*!
 * Time information.
 */
typedef struct {
    bool playing;
    uint64_t frame;
    uint64_t usecs;
    TimeInfoBBT bbt;
} TimeInfo;

/* ------------------------------------------------------------------------------------------------------------
 * Event related types */

/*!
 * Generic event.
 */
typedef struct {
    mapped_value_t type;  /*!< Type of event. @see EventTypes */
    uint32_t       frame; /*!< Frame offset since the beginning of process() */
} Event;

/*!
 * MIDI event.
 */
typedef struct {
    Event e;
    uint8_t port;
    uint8_t size;
    uint8_t data[4];
} MidiEvent;

/*!
 * MIDI Program event.
 *
 * This is a special type of event that tells to plugin to switch MIDI program.
 * The plugin is allowed to change its parameter values, the host should request them afterwards if needed.
 *
 * If the plugin has PLUGIN_SUPPORTS_PROGRAM_CHANGES set, the host must never use event type.
 *
 * @see MidiProgram
 */
typedef struct {
    Event e;
    uint8_t  channel; /* used only in synths */
    uint32_t bank;
    uint32_t program;
} MidiProgramEvent;

/*!
 * Parameter event.
 */
typedef struct {
    Event e;
    uint32_t index;
    float    value;
} ParameterEvent;

/*!
 * Time information event.
 */
typedef struct {
    Event e;
    bool playing;
    uint64_t frame;
    TimeInfoBBT bbt;
} TimeInfoEvent;

/* ------------------------------------------------------------------------------------------------------------
 * Plugin Host Descriptor */

/*!
 * PluginHostDescriptor
 */
typedef struct {
    PluginHostHandle handle;

    /*!
     * Previously used plugin version, may be NULL.
     */
    const char* pluginVersion;

    /*!
     * Current audio buffer size.
     */
    uint32_t buffer_size;

    /*!
     * Current audio sample rate.
     */
    double sample_rate;

    /*!
     * Wherever the host is currently processing offline.
     FIXME: what is this for?
     */
    bool is_offline;

    /* NOTE: NOT allowed during process()
     * probably better if only allowed during instantiate() */
    mapped_value_t (*map_value)(PluginHostHandle handle, const char* valueStr);
    const char*    (*unmap_value)(PluginHostHandle handle, mapped_value_t value);

    /* plugin must set "time" feature to use this
     * NOTE: only allowed during process() */
    const TimeInfo* (*get_time_info)(PluginHostHandle handle);

    /* plugin must set "writeevent" feature to use this
     * NOTE: only allowed during process() */
    bool (*write_event)(PluginHostHandle handle, const Event* event);

    /* plugin must set "sendmsg" feature to use this
     * NOTE: only allowed during idle() or non_rt_event() */
    bool (*send_ui_msg)(PluginHostHandle handle, const void* data, size_t size);

    /* uses HostDispatcherOpcodes : FIXME - use "const void* value" only? */
    int * (*dispatcher)(PluginHostHandle handle, mapped_value_t opcode, int32_t index, int32_t value, void* ptr, float opt);

} PluginHostDescriptor;

/* ------------------------------------------------------------------------------------------------------------
 * Plugin Descriptor */

/*!
 * PluginDescriptor
 */
typedef struct {
    /* descriptor metadata. the required set of metadata is: (TODO review)

       const char* metadata[] = {
       "api version", "1",                      // <-- MUST be version of API used when plugin was built.
       "author", "Bob Cat, MD",
       "display name", "Best EQ Evar",
       "organization", "bobcat",
       "uuid", "org.bobcat.best_eq_evar_v1",    // <-- MUST be universally unique for this plugin major version. Only allowed punctuation is "." and "_".
       "copyright", "copyright 2013 foobar inc",
       "interface version", "1",                // <-- MUST be incremented if the available parameters have changed in a not-backwards compatible way!
       "dsp version", "0",                      // <-- MUST be incremented if the DSP algorithm has been changed in any audible way from previous version.
       "description", "15 Band FFT EQ",
       "documentation", "This eq blah blah blah",
       "website", "http://bobcat.com/plugins/best_eq_evar/buyitnow",
       "provides", "foo:bar",                   // <-- features this plugin provides
       "requires", "bar",                       // <-- features this plugin requires
       NULL                                     // <-- MUST be NULL terminated!
      };
    */
    const char* const* metadata;

    PluginHandle (*instantiate)(const PluginHostDescriptor* host);
    void         (*cleanup)(PluginHandle handle);

    uint32_t         (*get_parameter_count)(PluginHandle handle);
    const Parameter* (*get_parameter_info)(PluginHandle handle, uint32_t index);
    float            (*get_parameter_value)(PluginHandle handle, uint32_t index);
    const char*      (*get_parameter_text)(PluginHandle handle, uint32_t index, float value); /* only used if parameter hint "customtext" is set */

    uint32_t           (*get_midi_program_count)(PluginHandle handle);
    const MidiProgram* (*get_midi_program_info)(PluginHandle handle, uint32_t index);

    /* only used if "idle" feature is set, or HOST_OPCODE_NEEDS_IDLE was triggered (for one-shot).
     * NOTE: although it's a non-realtime function, it will probably still not be called from the host main thread */
    void (*idle)(PluginHandle handle);

    /* NOTE: host will never call this while process() is running
     * FIXME: the above statement requires a lot of unnecessary house keeping in the host  */
    void (*non_rt_event)(PluginHandle handle, const Event* event);

    /* only used if "state" feature is set */
    char* (*get_state)(PluginHandle handle);
    void  (*set_state)(PluginHandle handle, const char* data);

    void (*activate)(PluginHandle handle);
    void (*deactivate)(PluginHandle handle);
    void (*process)(PluginHandle handle, audio_sample_t** inBuffer, audio_sample_t** outBuffer, uint32_t frames, const Event* events, uint32_t eventCount);

    /* uses PluginDispatcherOpcodes : FIXME - use "const void* value" only?  */
    int * (*dispatcher)(PluginHandle handle, mapped_value_t opcode, int32_t index, int32_t value, void* ptr, float opt);

} PluginDescriptor;

/* ------------------------------------------------------------------------------------------------------------
 * Plugin entry point */

/*!
 * Plugin entry point function used by the plugin.
 */
DAZ_SYMBOL_EXPORT
const PluginDescriptor* daz_get_plugin_descriptor(uint32_t index);

/*!
 * Plugin entry point function used by the host.
 */
typedef const PluginDescriptor* (*daz_get_plugin_descriptor_fn)(uint32_t index);

/* ------------------------------------------------------------------------------------------------------------ */

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DAZ_PLUGIN_H_INCLUDED */
