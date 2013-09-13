/*
 * Carla Native Plugin API
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_NATIVE_H_INCLUDED
#define CARLA_NATIVE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*!
 * @defgroup CarlaNativeAPI Carla Native API
 *
 * The Carla Native API
 * @{
 */

// -----------------------------------------------------------------------
// Macros

/*!
 * Current API version.
 */
#define CARLA_NATIVE_API_VERSION 1

/*!
 * @defgroup PluginCategories Plugin Categories
 *
 * A small list of pre-defined plugin categories.
 *
 * Plugins should provide at least one of these basic categories.
 * THey can use their own custom categories as well, as long as they are lowercase and contain ASCII characters only.
 * Many categories can be set by using ":" in between them.
 * @{
 */
#define PLUGIN_CATEGORY_SYNTH     "synth"     //!< A synthesizer or generator.
#define PLUGIN_CATEGORY_DELAY     "delay"     //!< A delay or reverberator.
#define PLUGIN_CATEGORY_EQ        "eq"        //!< An equalizer.
#define PLUGIN_CATEGORY_FILTER    "filter"    //!< A filter.
#define PLUGIN_CATEGORY_DYNAMICS  "dynamics"  //!< A 'dynamic' plugin (amplifier, compressor, gate, etc).
#define PLUGIN_CATEGORY_MODULATOR "modulator" //!< A 'modulator' plugin (chorus, flanger, phaser, etc).
#define PLUGIN_CATEGORY_UTILITY   "utility"   //!< An 'utility' plugin (analyzer, converter, mixer, etc).
#define PLUGIN_CATEGORY_OTHER     "other"     //!< Misc plugin (used to check if the plugin has a category).
/** @} */

/*!
 * @defgroup PluginFeatures Plugin Features
 *
 * A list of plugin features or hints.
 *
 * Custom features are allowed, as long as they are lowercase and contain ASCII characters only.
 * The host can decide if it can load the plugin or not based on this information.
 *
 * Multiple features can be set by using ":" in between them.
 * @{
 */

/*!
 * Is hard-realtime safe.
 *
 * Plugins with non-rtsafe parameters can be considered rtsafe,
 * as long as they set parameters hints accordingly.
 *
 * If the plugin is hard-realtime safe and supports MIDI programs,
 * it MUST ensure MIDI Program events are hard-realtime safe as well.
 *
 * @see PARAMETER_IS_RTSAFE
 */
#define PLUGIN_FEATURE_RTSAFE "rtsafe"

/*!
 * Needs non-realtime idle() function regularly.
 *
 * This can be used by plugins that need a non-realtime thread to do work.
 * The host will call PluginDescriptor::idle() at regular intervals.
 * The plugin MUST NOT lock indefinitely.
 *
 * Alternatively, the plugin can ask the host for a one-shot idle(),
 * by using HOST_OPCODE_NEEDS_IDLE.
 */
#define PLUGIN_FEATURE_IDLE "idle"

/*!
 * Needs fixed-size audio buffers.
 *
 * When this feature is set, the host will always call the process() with frames equal to the current buffer size.
 * The plugin should probably listen for buffer size changes.
 *
 * @see PLUGIN_FEATURE_BUFFER_SIZE_CHANGES, PLUGIN_OPCODE_BUFFER_SIZE_CHANGED
 */
#define PLUGIN_FEATURE_FIXED_BUFFERS "fixedbuffers"

/*!
 * Supports buffer size changes on-the-fly.
 *
 * If unset, the host will re-initiate the plugin when the buffer size changes.
 */
#define PLUGIN_FEATURE_BUFFER_SIZE_CHANGES "buffersizechanges"

/*!
 * Supports sample rate changes on-the-fly.
 *
 * If unset, the host will re-initiate the plugin when the sample rate changes.
 */
#define PLUGIN_FEATURE_SAMPLE_RATE_CHANGES "sampleratechanges"

/*!
 * Prefers mono-style panning.
 */
#define PLUGIN_FEATURE_MONO_PANNING "monopanning"

/*!
 * Prefers stereo balance.
 */
#define PLUGIN_FEATURE_STEREO_BALANCE "stereobalance"

/*!
 * Supports get_state() and set_state() functions.
 */
#define PLUGIN_FEATURE_STATE "state"

/*!
 * Uses get_time_info() function.
 */
#define PLUGIN_FEATURE_TIME "time"

/*!
 * Uses write_event() function.
 */
#define PLUGIN_FEATURE_WRITE_EVENT "writeevent"

/*!
 * Uses send_ui_msg() function.
 */
#define PLUGIN_FEATURE_SEND_MSG "sendmsg"

/** @} */

/*!
 * @defgroup UiFeatures UI Features
 *
 * A list of UI features or hints.
 *
 * Custom features are allowed, as long as they are lowercase and contain ASCII characters only.
 * The host can decide if it can load the UI or not based on this information.
 *
 * Multiple features can be set by using ":" in between them.
 * @{
 */

/*!
 * Supports sample rate changes on-the-fly.
 *
 * If unset, the host will re-initiate the UI when the sample rate changes.
 */
#define UI_FEATURE_SAMPLE_RATE_CHANGES "sampleratechanges"

/*!
 * Uses ui_open_file() and/or ui_save_file() functions.
 */
#define UI_FEATURE_OPEN_SAVE "opensave"

/*!
 * Uses send_plugin_msg() function.
 */
#define UI_FEATURE_SEND_MSG "sendmsg"

/** @} */

/*!
 * TODO - this needs a better name...
 *
 * @defgroup PluginSupports Plugin Supports
 *
 * A list of plugin supported MIDI events.
 *
 * Multiple (supports) can be set by using ":" in between them.
 * @{
 */

/*!
 * Handles MIDI programs internally instead of host-exposed/exported.
 *
 * When this is set, the host will no try to map MIDI program changes into
 * plugin exported programs by sending MidiProgramEvent, but will send MidiEvent directly.
 *
 * @see MidiProgram, MidiProgramEvent
 */
#define PLUGIN_SUPPORTS_PROGRAM_CHANGES "program"

/*!
 * Supports control changes (0xB0).
 *
 * @note:
 * The plugin MUST NEVER change exposed parameters on its own.
 * If the plugin wants to map a MIDI control change message to a parameter
 * it can do so by reporting it in the meta-data, which the host will read.
 */
#define PLUGIN_SUPPORTS_CONTROL_CHANGES "control"

/*!
 * Supports channel pressure (0xD0).
 */
#define PLUGIN_SUPPORTS_CHANNEL_PRESSURE "pressure"

/*!
 * Supports note aftertouch (0xA0).
 */
#define PLUGIN_SUPPORTS_NOTE_AFTERTOUCH "aftertouch"

/*!
 * Supports pitchbend (0xE0).
 */
#define PLUGIN_SUPPORTS_PITCHBEND "pitchbend"

/*!
 * Supports all-sound-off and all-notes-off events.
 *
 * When this is not set, the host might want to send various note-off events to silence the plugin.
 */
#define PLUGIN_SUPPORTS_ALL_SOUND_OFF "allsoundoff"

/**@}*/

/*!
 * @defgroup ParameterHints Parameter Hints
 *
 * List of parameter hints.
 *
 * Multiple hints can be set by using ":" in between them.
 * @{
 */

/*!
 * Is output.
 *
 * If this is not set, the parameter should be considered input.
 *
 * Input parameters are managed by the host and changed by sending a ParameterEvent to the plugin.
 * The plugin MUST NEVER change input parameters on its own.
 *
 * Output parameters are managed by the plugin.
 * Every time their values change the plugin should .
 * Because of this, any plugin that has output parameters should always set PLUGIN_FEATURE_WRITE_EVENT.
 */
#define PARAMETER_IS_OUTPUT "output"

/*!
 * Is enabled.
 *
 * If set the host may show this parameter on its "built-in" dialog.
 */
#define PARAMETER_IS_ENABLED "enabled"

/*!
 * Is hard-realtime safe.
 *
 * For input parameters:
 * When set, the host MUST ONLY use in-process events to change this parameter.
 * When not set the host MUST ONLY use PluginDescriptor::non_rt_event().
 *
 * For output parameters:
 * When set, the plugin must send a ParameterEvent to the host every time the value changes.
 * When not set the host will call PluginDescriptor::get_parameter_value(), where the plugin is allowed to lock.
 *
 * @see PLUGIN_FEATURE_RTSAFE
 */
#define PARAMETER_IS_RTSAFE "rtsafe"

/*!
 * Values are boolean (always at minimum or maximum values).
 */
#define PARAMETER_IS_BOOLEAN "boolean"

/*!
 * Values are integer.
 */
#define PARAMETER_IS_INTEGER "integer"

/*!
 * Values are logarithmic.
 */
#define PARAMETER_IS_LOGARITHMIC   "logarithmic"

/*!
 * Needs sample rate to work.
 *
 * The parameter value and ranges are multiplied by sample rate on usage
 * and divided by sample rate on save.
 */
#define PARAMETER_USES_SAMPLE_RATE "samplerate"

/*!
 * Uses scalepoints to define internal values in a meaningful way.
 */
#define PARAMETER_USES_SCALEPOINTS "scalepoints"

/*!
 * Uses custom text for displaying its value.
 *
 * @see get_parameter_text()
 */
#define PARAMETER_USES_CUSTOM_TEXT "customtext"

/**@}*/

/*!
 * @defgroup DefaultParameterRanges Default Parameter Ranges
 *
 * Default values for parameter range steps.
 * @{
 */
#define PARAMETER_RANGE_DEFAULT_STEP       0.01f
#define PARAMETER_RANGE_DEFAULT_STEP_SMALL 0.0001f
#define PARAMETER_RANGE_DEFAULT_STEP_LARGE 0.1f
/**@}*/

/*!
 * @defgroup EventTypes EventTypes
 *
 * List of supported event types.
 *
 * The types are mapped into mapped_value_t by the host.
 * @see Plugin/UiHostDescriptor::map_value()
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
#define EVENT_TYPE_PARAMETER "time"

/**@}*/

/*!
 * @defgroup PluginDispatcherOpcodes Plugin Dispatcher Opcodes
 *
 * Opcodes dispatched by the host to report changes to the plugin or UI.
 *
 * The opcodes are mapped into MappedValue by the host.
 * @see PluginDescriptor::dispatcher()
 * @{
 */
#define PLUGIN_OPCODE_MSG_RECEIVED        "msgReceived"       //!< Message received, uses ptr as char*.
#define PLUGIN_OPCODE_BUFFER_SIZE_CHANGED "bufferSizeChanged" //!< Audio buffer size changed, uses value, returns 1 if supported. @see PluginHostDescriptor::get_buffer_size()
#define PLUGIN_OPCODE_SAMPLE_RATE_CHANGED "sampleRateChanged" //!< Audio sample rate changed, uses opt, returns 1 if supported. @see Plugin/UiHostDescriptor::get_sample_rate()
#define PLUGIN_OPCODE_OFFLINE_CHANGED     "offlineChanged"    //!< Offline mode changed, uses value (0=off, 1=on). @see Plugin/UiHostDescriptor::is_offline()
#define PLUGIN_OPCODE_UI_TITLE_CHANGED    "uiTitleChanged"    //!< UI title changed, uses ptr. @see UiHostDescriptor::is_offline()
/**@}*/

/*!
 * @defgroup HostDispatcherOpcodes Host Dispatcher Opcodes
 *
 * Opcodes dispatched by the plugin or UI to report and request information from the host.
 *
 * The opcodes are mapped into MappedValue by the host.
 * @see HostDescriptor::dispatcher()
 * @{
 */
#define HOST_OPCODE_NEEDS_IDLE            "needsIdle"          //!< Tell the host to call idle() as soon as possible (once), uses nothing.
#define HOST_OPCODE_SET_VOLUME            "setVolume"          //!< Set host's volume, uses opt. MUST ONLY be called within PluginDescriptor::set_midi_program().
#define HOST_OPCODE_SET_DRYWET            "setDryWet"          //!< Set host's dry-wet, uses opt. MUST ONLY be called within PluginDescriptor::set_midi_program().
#define HOST_OPCODE_SET_BALANCE_LEFT      "setBalanceLeft"     //!< Set host's balance-left, uses opt. MUST ONLY be called within PluginDescriptor::set_midi_program().
#define HOST_OPCODE_SET_BALANCE_RIGHT     "setBalanceRight"    //!< Set host's balance-right, uses opt. MUST ONLY be called within PluginDescriptor::set_midi_program().
#define HOST_OPCODE_SET_PANNING           "setPanning"         //!< Set host's panning, uses opt. MUST ONLY be called within PluginDescriptor::set_midi_program().
#define HOST_OPCODE_GET_PARAMETER_MIDI_CC "getParameterMidiCC" //!< Get the parameter @a index currently mapped MIDI control, uses index, return answer.
#define HOST_OPCODE_SET_PARAMETER_MIDI_CC "setParameterMidiCC" //!< Set the parameter @a index mapped MIDI control, uses index and value.
#define HOST_OPCODE_UPDATE_PARAMETER      "updateParameter"    //!< Tell the host to update parameter @a index, uses index with -1 for all.
#define HOST_OPCODE_UPDATE_MIDI_PROGRAM   "updateMidiProgram"  //!< Tell the host to update midi-program @index,  uses index with -1 for all; may also use value for channel.
#define HOST_OPCODE_RELOAD_PARAMETERS     "reloadParameters"   //!< Tell the host to reload all parameters data, uses nothing.
#define HOST_OPCODE_RELOAD_MIDI_PROGRAMS  "reloadMidiPrograms" //!< Tell the host to reload all midi-programs data, uses nothing.
#define HOST_OPCODE_RELOAD_ALL            "reloadAll"          //!< Tell the host to reload everything all the plugin, uses nothing.
#define HOST_OPCODE_UI_UNAVAILABLE        "uiUnavailable"      //!< Tell the host the UI can't be shown, uses nothing.
/**@}*/

// -----------------------------------------------------------------------
// Base types

/*!
 * Audio sample type.
 */
typedef float audio_sample_t;

/*!
 * Host mapped value of a string.
 * The value 0 is reserved as undefined.
 * @see Plugin/UiHostDescriptor::map_value()
 */
typedef uint32_t mapped_value_t;

/*!
 * Opaque plugin handle.
 */
typedef void* PluginHandle;

/*!
 * Opaque UI handle.
 */
typedef void* UiHandle;

/*!
 * Opaque plugin host handle.
 */
typedef void* PluginHostHandle;

/*!
 * Opaque UI host handle.
 */
typedef void* UiHostHandle;

// -----------------------------------------------------------------------
// Base structs

/*!
 * Parameter scale point.
 */
typedef struct {
    const char* label;
    float value;
} ParameterScalePoint;

/*!
 * Parameter ranges.
 */
typedef struct {
    float def;
    float min;
    float max;
    float step;
    float stepSmall;
    float stepLarge;
} ParameterRanges;

/*!
 * Parameter.
 */
typedef struct {
    const char* hints; //!< @see ParameterHints
    const char* name;
    const char* symbol;
    const char* unit;
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

/*!
 * Bar-Beat-Tick information.
 *
 * @note this is the same data provided by JACK
 */
typedef struct {
    bool valid;

    int32_t bar;  //!< current bar
    int32_t beat; //!< current beat-within-bar
    int32_t tick; //!< current tick-within-beat
    double barStartTick;

    float beatsPerBar; //!< time signature "numerator"
    float beatType;    //!< time signature "denominator"

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

/*!
 * Generic event.
 */
typedef struct {
    mapped_value_t type;  //!< Type of event. @see EventTypes
    uint32_t       frame; //!< Frame offset since the beginning of process()
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
    uint8_t  channel; // used only in synths
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
    TimeInfoBBT bbt;
} TimeInfoEvent;

// -----------------------------------------------------------------------
// PluginHostDescriptor

typedef struct {
    PluginHostHandle handle;

    /*!
     * Previously used plugin version, may be NULL.
     */
    const char* pluginVersion;

    // NOTE: NOT allowed during process()
    mapped_value_t (*map_value)(PluginHostHandle handle, const char* valueStr);
    const char*    (*unmap_value)(PluginHostHandle handle, mapped_value_t value);

    // NOTE: always allowed
    uint32_t (*get_buffer_size)(PluginHostHandle handle);
    double   (*get_sample_rate)(PluginHostHandle handle);
    bool     (*is_offline)(PluginHostHandle handle);

    // plugin must set "time" feature to use this
    // NOTE: only allowed during process()
    const TimeInfo* (*get_time_info)(PluginHostHandle handle);

    // plugin must set "sendmsg" feature to use this
    // NOTE: only allowed during idle()
    bool (*send_ui_msg)(PluginHostHandle handle, const char* msg);

    // plugin must set "writeevent" feature to use this
    // NOTE: only allowed during process()
    bool (*write_event)(PluginHostHandle handle, const Event* event);

    // uses HostDispatcherOpcodes
    intptr_t (*dispatcher)(PluginHostHandle handle, MappedValue opcode, int32_t index, intptr_t value, void* ptr, float opt);

} PluginHostDescriptor;

// -----------------------------------------------------------------------
// UiHostDescriptor

typedef struct {
    UiHostHandle handle;
    const char* resourceDir;
    const char* uiTitle;

    mapped_value_t (*map_value)(UiHostHandle handle, const char* valueStr);
    const char*    (*unmap_value)(UiHostHandle handle, mapped_value_t value);

    double (*get_sample_rate)(UiHostHandle handle);
    bool   (*is_offline)(UiHostHandle handle);

    void (*parameter_changed)(UiHostHandle handle, uint32_t index, float value);
    void (*midi_program_changed)(UiHostHandle handle, uint8_t channel, uint32_t bank, uint32_t program);
    void (*closed)(UiHostHandle handle);

    // TODO: add some msgbox call

    // ui must set "opensave" feature to use these
    const char* (*open_file)(UiHostHandle handle, bool isDir, const char* title, const char* filter);
    const char* (*save_file)(UiHostHandle handle, bool isDir, const char* title, const char* filter);

    // ui must set "sendmsg" feature to use this
    bool (*send_plugin_msg)(const char* msg);

    // uses HostDispatcherOpcodes
    intptr_t (*dispatcher)(PluginHostHandle handle, MappedValue opcode, int32_t index, intptr_t value, void* ptr, float opt);

} UiHostDescriptor;

// -----------------------------------------------------------------------
// PluginDescriptor

typedef struct _PluginDescriptor {
    const int api;                //!< Must be set to CARLA_NATIVE_API_VERSION.
    const char* const categories; //!< Categories. @see PluginCategories
    const char* const features;   //!< Features. @see PluginFeatures
    const char* const supports;   //!< MIDI supported events. @see PluginSupports
    const char* const metadata;   //!< Meta-data in JSON? format.
    const uint32_t audioIns;      //!< Default number of audio inputs.
    const uint32_t audioOuts;     //!< Default number of audio outputs.
    const uint32_t midiIns;       //!< Default number of MIDI inputs.
    const uint32_t midiOuts;      //!< Default number of MIDI inputs.
    const uint32_t paramIns;      //!< Default number of input parameters, may be 0.
    const uint32_t paramOuts;     //!< Default number of output parameters, may be 0.
    const char* const author;     //!< Author.
    const char* const name;       //!< Name.
    const char* const label;      //!< Label, can only contain letters, numbers and "_".
    const char* const copyright;  //!< Copyright.
    const int version;            //!< Version in hexadecimal (0x1023 = 1.0.23).

    PluginHandle (*instantiate)(const PluginHostDescriptor* host);
    void         (*cleanup)(PluginHandle handle);

    uint32_t         (*get_parameter_count)(PluginHandle handle);
    const Parameter* (*get_parameter_info)(PluginHandle handle, uint32_t index);
    float            (*get_parameter_value)(PluginHandle handle, uint32_t index);
    const char*      (*get_parameter_text)(PluginHandle handle, uint32_t index, float value); // only used if parameter hint "customtext" is set

    uint32_t           (*get_midi_program_count)(PluginHandle handle);
    const MidiProgram* (*get_midi_program_info)(PluginHandle handle, uint32_t index);

    // NOTE: host will never call this while process() is running
    void (*non_rt_event)(PluginHandle handle, const Event* event);

    // only used if "idle" feature is set, or HOST_OPCODE_NEEDS_IDLE was triggered (for one-shot).
    // NOTE: although it's a non-realtime function, it will probably still not be called from the host main thread
    void (*idle)(PluginHandle handle);

    // only used if "state" feature is set
    char* (*get_state)(PluginHandle handle);
    void  (*set_state)(PluginHandle handle, const char* data);

    void (*activate)(PluginHandle handle);
    void (*deactivate)(PluginHandle handle);
    void (*process)(PluginHandle handle, audio_sample_t** inBuffer, audio_sample_t** outBuffer, uint32_t frames, const Event* events, uint32_t eventCount);

    // uses PluginDispatcherOpcodes
    intptr_t (*dispatcher)(PluginHandle handle, MappedValue opcode, int32_t index, intptr_t value, void* ptr, float opt);

} PluginDescriptor;

// -----------------------------------------------------------------------
// UiDescriptor

typedef struct {
    const int api;              //!< Must be set to CARLA_NATIVE_API_VERSION.
    const char* const features; //!< Features. @see UiFeatures
    const char* const author;   //!< Author this UI matches to.
    const char* const label;    //!< Label this UI matches to, can only contain letters, numbers and "_". May be null, in which case represents all UIs for @a maker.

    UiHandle (*instantiate)(const UiHostDescriptor* host);
    void     (*cleanup)(UiHandle handle);

    void (*show)(UiHandle handle, bool show);
    void (*idle)(UiHandle handle);

    void (*event)(UiHandle handle, const Event* event);

    intptr_t (*dispatcher)(UiHandle handle, MappedValue opcode, int32_t index, intptr_t value, void* ptr, float opt);

} UiDescriptor;

// -----------------------------------------------------------------------
// Register plugin

extern void carla_register_native_plugin(const PluginDescriptor* desc);

// -----------------------------------------------------------------------

/**@}*/

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CARLA_NATIVE_H_INCLUDED
