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

typedef int32_t AtomXYZ;
typedef void* HostHandle;
typedef void* PluginHandle;

// -----------------------------------------------------------------------
// enums

#define PLUGIN_API_VERSION 1

/*!
 * @defgroup PluginCategories Plugin Categories
 *
 * A small list of pre-defined plugin categories.
 *
 * Plugins can use their own custom categories as well, as long as they are lowercase and contain ASCII characters only.
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
/**@}*/

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
#define PLUGIN_FEATURE_RTSAFE         "rtsafe"         //!< Is hard-realtime safe.
#define PLUGIN_FEATURE_GUI            "gui"            //!< Provides custom UI.
#define PLUGIN_FEATURE_STATE          "state"          //!< Supports get_state() and set_state().
#define PLUGIN_FEATURE_TIME           "time"           //!< Uses get_time_info().
#define PLUGIN_FEATURE_WRITE_EVENT    "writeevent"     //!< Uses write_event().
#define PLUGIN_FEATURE_FIXED_BUFFERS  "fixedbuffers"   //!< Needs fixed-size audio buffers.
#define PLUGIN_FEATURE_MONO_PANNING   "monopanning"    //!< Prefers mono-style panning.
#define PLUGIN_FEATURE_STEREO_BALANCE "stereobalance"  //!< Prefers stereo balance.
#define PLUGIN_FEATURE_OPENSAVE       "uiopensave"     //!< UI uses ui_open_file() and/or ui_save_file() functions.
#define PLUGIN_FEATURE_SINGLE_THREAD  "uisinglethread" //!< UI needs paramter, midi-program and custom-data changes in the main thread.
/**@}*/

/*!
 * @defgroup PluginSupports Plugin Supports
 *
 * A list of plugin supported MIDI events.
 * @{
 */
#define PLUGIN_SUPPORTS_PROGRAM_CHANGES  "program"     //!< Handles MIDI programs internally instead of host-exposed/exported
#define PLUGIN_SUPPORTS_CONTROL_CHANGES  "control"     //!< Supports control changes (0xB0)
#define PLUGIN_SUPPORTS_CHANNEL_PRESSURE "pressure"    //!< Supports channel pressure (0xD0)
#define PLUGIN_SUPPORTS_NOTE_AFTERTOUCH  "aftertouch"  //!< Supports note aftertouch (0xA0)
#define PLUGIN_SUPPORTS_PITCHBEND        "pitchbend"   //!< Supports pitchbend (0xE0)
#define PLUGIN_SUPPORTS_ALL_SOUND_OFF    "allsoundoff" //!< Supporta all-sound-off and all-notes-off events
#define PLUGIN_SUPPORTS_EVERYTHING       "program:control:pressure:aftertouch:pitchbend:allsoundoff" //!< Supports everything
/**@}*/

/*!
 * @defgroup ParameterHints Parameter Hints
 *
 * List of parameter hints.
 * @{
 */
#define PARAMETER_IS_OUTPUT        "output"      //!< Is output; input if unset.
#define PARAMETER_IS_ENABLED       "enabled"     //!< Is enabled and shown by the host; can be changed if not output.
#define PARAMETER_IS_AUTOMABLE     "automable"   //!< Is automable; get_parameter_value() and set_parameter_value() MUST be realtime safe for this parameter.
#define PARAMETER_IS_BOOLEAN       "boolean"     //!< Values are boolean (always at minimum or maximum values).
#define PARAMETER_IS_INTEGER       "integer"     //!< Values are integer.
#define PARAMETER_IS_LOGARITHMIC   "logarithmic" //!< Values are logarithmic.
#define PARAMETER_USES_SAMPLE_RATE "samplerate"  //!< Needs sample-rate to work (value and ranges are multiplied by SR on usage, divided by SR on save).
#define PARAMETER_USES_SCALEPOINTS "scalepoints" //!< Uses scalepoints to define internal values in a meaningful way.
#define PARAMETER_USES_CUSTOM_TEXT "customtext"  //!< Uses custom text for displaying its value. @see get_parameter_text()
/**@}*/

typedef enum {
    PLUGIN_OPCODE_NULL                = 0, // nothing
    PLUGIN_OPCODE_BUFFER_SIZE_CHANGED = 1, // uses value
    PLUGIN_OPCODE_SAMPLE_RATE_CHANGED = 2, // uses opt
    PLUGIN_OPCODE_OFFLINE_CHANGED     = 3, // uses value (0=off, 1=on)
    PLUGIN_OPCODE_UI_NAME_CHANGED     = 4  // uses ptr
} PluginDispatcherOpcode;

typedef enum {
    HOST_OPCODE_NULL                  = 0,  // nothing
    HOST_OPCODE_SET_VOLUME            = 1,  // uses opt
    HOST_OPCODE_SET_DRYWET            = 2,  // uses opt
    HOST_OPCODE_SET_BALANCE_LEFT      = 3,  // uses opt
    HOST_OPCODE_SET_BALANCE_RIGHT     = 4,  // uses opt
    HOST_OPCODE_SET_PANNING           = 5,  // uses opt
    HOST_OPCODE_GET_PARAMETER_MIDI_CC = 6,  // uses index; return answer
    HOST_OPCODE_SET_PARAMETER_MIDI_CC = 7,  // uses index and value
    HOST_OPCODE_SET_PROCESS_PRECISION = 8,  // uses value
    HOST_OPCODE_UPDATE_PARAMETER      = 9,  // uses index, -1 for all
    HOST_OPCODE_UPDATE_MIDI_PROGRAM   = 10, // uses index, -1 for all; may use value for channel
    HOST_OPCODE_RELOAD_PARAMETERS     = 11, // nothing
    HOST_OPCODE_RELOAD_MIDI_PROGRAMS  = 12, // nothing
    HOST_OPCODE_RELOAD_ALL            = 13, // nothing
    HOST_OPCODE_UI_UNAVAILABLE        = 14  // nothing
} HostDispatcherOpcode;

// -----------------------------------------------------------------------
// base structs

typedef struct {
    const char* label;
    float value;
} ParameterScalePoint;

typedef struct {
    float def;
    float min;
    float max;
    float step;
    float stepSmall;
    float stepLarge;
} ParameterRanges;

#define PARAMETER_RANGES_DEFAULT_STEP       0.01f
#define PARAMETER_RANGES_DEFAULT_STEP_SMALL 0.0001f
#define PARAMETER_RANGES_DEFAULT_STEP_LARGE 0.1f

typedef struct {
    const char* hints;
    const char* name;
    const char* unit;
    ParameterRanges ranges;

    uint32_t scalePointCount;
    ParameterScalePoint* scalePoints;
} Parameter;

typedef struct {
    AtomXYZ type;
    uint32_t frame;
} Event;

typedef struct {
    Event e;
    uint8_t port;
    uint8_t size;
    uint8_t data[4];
} MidiEvent;

typedef struct {
    Event e;
    uint32_t index;
    float    value;
} ParameterEvent;

typedef struct {
    uint32_t bank;
    uint32_t program;
    const char* name;
} MidiProgram;

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

typedef struct {
    bool playing;
    uint64_t frame;
    uint64_t usecs;
    TimeInfoBBT bbt;
} TimeInfo;

// -----------------------------------------------------------------------
// HostDescriptor

typedef struct {
    HostHandle handle;
    const char* resourceDir;
    const char* uiName;

    uint32_t (*get_buffer_size)(HostHandle handle);
    double   (*get_sample_rate)(HostHandle handle);
    bool     (*is_offline)(HostHandle handle);

    const TimeInfo* (*get_time_info)(HostHandle handle);
    bool            (*write_event)(HostHandle handle, const Event* event);

    void (*ui_parameter_changed)(HostHandle handle, uint32_t index, float value);
    void (*ui_midi_program_changed)(HostHandle handle, uint8_t channel, uint32_t bank, uint32_t program);
    void (*ui_custom_data_changed)(HostHandle handle, const char* key, const char* value);
    void (*ui_closed)(HostHandle handle);

    // TODO: add some msgbox call
    const char* (*ui_open_file)(HostHandle handle, bool isDir, const char* title, const char* filter);
    const char* (*ui_save_file)(HostHandle handle, bool isDir, const char* title, const char* filter);

    intptr_t (*dispatcher)(HostHandle handle, HostDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt);

} HostDescriptor;

// -----------------------------------------------------------------------
// PluginDescriptor

typedef struct _PluginDescriptor {
    const int api;                //!< Must be set to PLUGIN_API_VERSION
    const char* const categories; //!< Categories. @see PluginCategories
    const char* const hints;      //!< Hints. @see PluginHints
    const char* const supports;   //!< MIDI supported events. @see PluginSupports
    const uint32_t audioIns;      //!< Default number of audio inputs.
    const uint32_t audioOuts;     //!< Default number of audio outputs.
    const uint32_t midiIns;       //!< Default number of MIDI inputs.
    const uint32_t midiOuts;      //!< Default number of MIDI inputs.
    const uint32_t parameterIns;  //!< Default number of input parameters, may be 0.
    const uint32_t parameterOuts; //!< Default number of output parameters, may be 0.
    const char* const name;       //!< Name.
    const char* const label;      //!< Label, can only contain letters, numbers and "_".
    const char* const maker;      //!< Maker.
    const char* const copyright;  //!< Copyright.
    const int version;            //!< Version.

    PluginHandle (*instantiate)(const HostDescriptor* host);
    void         (*cleanup)(PluginHandle handle);

    uint32_t         (*get_parameter_count)(PluginHandle handle);
    const Parameter* (*get_parameter_info)(PluginHandle handle, uint32_t index);
    float            (*get_parameter_value)(PluginHandle handle, uint32_t index);
    const char*      (*get_parameter_text)(PluginHandle handle, uint32_t index, float value);

    uint32_t           (*get_midi_program_count)(PluginHandle handle);
    const MidiProgram* (*get_midi_program_info)(PluginHandle handle, uint32_t index);

    void (*set_parameter_value)(PluginHandle handle, uint32_t index, float value);
    void (*set_midi_program)(PluginHandle handle, uint8_t channel, uint32_t bank, uint32_t program);
    void (*set_custom_data)(PluginHandle handle, const char* key, const char* value);

    void (*ui_show)(PluginHandle handle, bool show);
    void (*ui_idle)(PluginHandle handle);

    void (*ui_set_parameter_value)(PluginHandle handle, uint32_t index, float value);
    void (*ui_set_midi_program)(PluginHandle handle, uint8_t channel, uint32_t bank, uint32_t program);
    void (*ui_set_custom_data)(PluginHandle handle, const char* key, const char* value);

    void (*activate)(PluginHandle handle);
    void (*deactivate)(PluginHandle handle);
    void (*process)(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const Event* events, uint32_t eventCount);

    char* (*get_state)(PluginHandle handle);
    void  (*set_state)(PluginHandle handle, const char* data);

    intptr_t (*dispatcher)(PluginHandle handle, PluginDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt);

} PluginDescriptor;

// -----------------------------------------------------------------------
// Register plugin

extern void carla_register_native_plugin(const PluginDescriptor* desc);

// -----------------------------------------------------------------------

/**@}*/

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CARLA_NATIVE_H_INCLUDED
