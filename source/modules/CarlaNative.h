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

// ---------------------------------------------------------------------------------------
// Plugin Categories

#define PLUGIN_CATEGORY_SYNTH     ":synth"     //!< A synthesizer or generator.
#define PLUGIN_CATEGORY_DELAY     ":delay"     //!< A delay or reverberator.
#define PLUGIN_CATEGORY_EQ        ":eq"        //!< An equalizer.
#define PLUGIN_CATEGORY_FILTER    ":filter"    //!< A filter.
#define PLUGIN_CATEGORY_DYNAMICS  ":dynamics"  //!< A 'dynamic' plugin (amplifier, compressor, gate, etc).
#define PLUGIN_CATEGORY_MODULATOR ":modulator" //!< A 'modulator' plugin (chorus, flanger, phaser, etc).
#define PLUGIN_CATEGORY_UTILITY   ":utility"   //!< An 'utility' plugin (analyzer, converter, mixer, etc).
#define PLUGIN_CATEGORY_OTHER     ":other"     //!< Misc plugin (used to check if the plugin has a category).

// ---------------------------------------------------------------------------------------
// Plugin Features

#define PLUGIN_FEATURE_RTSAFE              ":rtsafe"
#define PLUGIN_FEATURE_FIXED_BUFFERS       ":fixedbuffers"
#define PLUGIN_FEATURE_BUFFER_SIZE_CHANGES ":buffersizechanges"
#define PLUGIN_FEATURE_SAMPLE_RATE_CHANGES ":sampleratechanges"
#define PLUGIN_FEATURE_MONO_PANNING        ":monopanning"
#define PLUGIN_FEATURE_STEREO_BALANCE      ":stereobalance"
#define PLUGIN_FEATURE_STATE               ":state"
#define PLUGIN_FEATURE_TIME                ":time"
#define PLUGIN_FEATURE_WRITE_EVENT         ":writeevent"
#define PLUGIN_FEATURE_OPEN_SAVE           ":uiopensave"

// ---------------------------------------------------------------------------------------
// Plugin Supports

#define PLUGIN_SUPPORTS_PROGRAM_CHANGES  ":program"
#define PLUGIN_SUPPORTS_CONTROL_CHANGES  ":control"
#define PLUGIN_SUPPORTS_CHANNEL_PRESSURE ":pressure"
#define PLUGIN_SUPPORTS_NOTE_AFTERTOUCH  ":aftertouch"
#define PLUGIN_SUPPORTS_PITCHBEND        ":pitchbend"
#define PLUGIN_SUPPORTS_ALL_SOUND_OFF    ":allsoundoff"
#define PLUGIN_SUPPORTS_EVERYTHING       ":control:pressure:aftertouch:pitchbend:allsoundoff:"

// ---------------------------------------------------------------------------------------
// Parameter Hints

#define PARAMETER_IS_OUTPUT        ":output"
#define PARAMETER_IS_ENABLED       ":enabled"
#define PARAMETER_IS_AUTOMABLE     ":rtsafe"
#define PARAMETER_IS_BOOLEAN       ":boolean"
#define PARAMETER_IS_INTEGER       ":integer"
#define PARAMETER_IS_LOGARITHMIC   ":logarithmic"
#define PARAMETER_USES_SAMPLE_RATE ":samplerate"
#define PARAMETER_USES_SCALEPOINTS ":scalepoints"
#define PARAMETER_USES_CUSTOM_TEXT ":customtext"

// ---------------------------------------------------------------------------------------
// Default Parameter Ranges

#define PARAMETER_RANGE_DEFAULT_STEP       0.01f
#define PARAMETER_RANGE_DEFAULT_STEP_SMALL 0.0001f
#define PARAMETER_RANGE_DEFAULT_STEP_LARGE 0.1f

// ---------------------------------------------------------------------------------------
// Event Types

#define EVENT_TYPE_MIDI         "midi"
#define EVENT_TYPE_MIDI_PROGRAM "midiprogram"
#define EVENT_TYPE_PARAMETER    "parameter"

// ---------------------------------------------------------------------------------------
// Host Dispatcher Opcodes

#define HOST_OPCODE_SET_VOLUME            "setVolume"          //!< Set host's volume, uses opt.
#define HOST_OPCODE_SET_DRYWET            "setDryWet"          //!< Set host's dry-wet, uses opt.
#define HOST_OPCODE_SET_BALANCE_LEFT      "setBalanceLeft"     //!< Set host's balance-left, uses opt.
#define HOST_OPCODE_SET_BALANCE_RIGHT     "setBalanceRight"    //!< Set host's balance-right, uses opt.
#define HOST_OPCODE_SET_PANNING           "setPanning"         //!< Set host's panning, uses opt.
#define HOST_OPCODE_GET_PARAMETER_MIDI_CC "getParameterMidiCC" //!< Get the parameter @a index currently mapped MIDI control, uses index, return answer.
#define HOST_OPCODE_SET_PARAMETER_MIDI_CC "setParameterMidiCC" //!< Set the parameter @a index mapped MIDI control, uses index and value.
#define HOST_OPCODE_UPDATE_PARAMETER      "updateParameter"    //!< Tell the host to update parameter @a index, uses index with -1 for all.
#define HOST_OPCODE_UPDATE_MIDI_PROGRAM   "updateMidiProgram"  //!< Tell the host to update midi-program @a index, uses index with -1 for all; may also use value for channel.
#define HOST_OPCODE_RELOAD_PARAMETERS     "reloadParameters"   //!< Tell the host to reload all parameters data, uses nothing.
#define HOST_OPCODE_RELOAD_MIDI_PROGRAMS  "reloadMidiPrograms" //!< Tell the host to reload all midi-programs data, uses nothing.
#define HOST_OPCODE_RELOAD_ALL            "reloadAll"          //!< Tell the host to reload everything all the plugin, uses nothing.
#define HOST_OPCODE_UI_UNAVAILABLE        "uiUnavailable"      //!< Tell the host the UI can't be shown, uses nothing.

// ---------------------------------------------------------------------------------------
// Plugin Dispatcher Opcodes

#define PLUGIN_OPCODE_BUFFER_SIZE_CHANGED "bufferSizeChanged" //!< Audio buffer size changed, uses value.
#define PLUGIN_OPCODE_SAMPLE_RATE_CHANGED "sampleRateChanged" //!< Audio sample rate changed, uses opt.
#define PLUGIN_OPCODE_OFFLINE_CHANGED     "offlineChanged"    //!< Offline mode changed, uses value (0=off, 1=on).
#define PLUGIN_OPCODE_UI_TITLE_CHANGED    "uiTitleChanged"    //!< UI title changed, uses ptr.

// ---------------------------------------------------------------------------------------
// Base types

typedef float    audio_sample_t;
typedef uint32_t mapped_value_t;

typedef void* PluginHandle;
typedef void* HostHandle;

// ---------------------------------------------------------------------------------------
// Base structs

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

typedef struct {
    const char* hints;
    const char* name;
    const char* unit;
    ParameterRanges ranges;

    uint32_t scalePointCount;
    ParameterScalePoint* scalePoints;
} Parameter;

typedef struct {
    uint32_t bank;
    uint32_t program;
    const char* name;
} MidiProgram;

// ---------------------------------------------------------------------------------------
// Events

typedef struct {
    mapped_value_t type;
    uint32_t frame;
} Event;

typedef struct {
    Event e;
    uint32_t port;
    uint32_t size;
    uint8_t  data[4];
} MidiEvent;

typedef struct {
    Event e;
    uint32_t channel; // used only in synths
    uint32_t bank;
    uint32_t program;
} MidiProgramEvent;

typedef struct {
    Event e;
    uint32_t index;
    float    value;
} ParameterEvent;

// ---------------------------------------------------------------------------------------
// Time

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

// ---------------------------------------------------------------------------------------
// PluginHostDescriptor

typedef struct {
    HostHandle handle;

    const char* resourceDir;
    const char* uiTitle;

    uint32_t bufferSize;
    uint32_t sampleRate;
    bool     isOffline;

    mapped_value_t (*map_value)(HostHandle handle, const char* valueStr);
    const char*    (*unmap_value)(HostHandle handle, mapped_value_t value);

    const TimeInfo* (*get_time_info)(HostHandle handle);
    bool            (*write_event)(HostHandle handle, const Event* event);

    void (*ui_parameter_changed)(HostHandle handle, uint32_t index, float value);
    void (*ui_midi_program_changed)(HostHandle handle, uint8_t channel, uint32_t bank, uint32_t program);
    void (*ui_closed)(HostHandle handle);

    // TODO: add some msgbox call
    const char* (*ui_open_file)(HostHandle handle, bool isDir, const char* title, const char* filter);
    const char* (*ui_save_file)(HostHandle handle, bool isDir, const char* title, const char* filter);

    intptr_t (*dispatcher)(HostHandle handle, mapped_value_t opcode, int32_t index, intptr_t value, void* ptr, float opt);

} PluginHostDescriptor;

// ---------------------------------------------------------------------------------------
// PluginDescriptor

typedef struct {
    const char* const categories;
    const char* const features;
    const char* const supports;
    const uint32_t audioIns;
    const uint32_t audioOuts;
    const uint32_t midiIns;
    const uint32_t midiOuts;
    const uint32_t paramIns;
    const uint32_t paramOuts;
    const char* const author;
    const char* const name;
    const char* const label;
    const char* const copyright;
    const char* const version;

    PluginHandle (*instantiate)(const PluginHostDescriptor* host);
    void         (*cleanup)(PluginHandle handle);

    uint32_t         (*get_parameter_count)(PluginHandle handle);
    const Parameter* (*get_parameter_info)(PluginHandle handle, uint32_t index);
    float            (*get_parameter_value)(PluginHandle handle, uint32_t index);
    const char*      (*get_parameter_text)(PluginHandle handle, uint32_t index, float value);

    uint32_t           (*get_midi_program_count)(PluginHandle handle);
    const MidiProgram* (*get_midi_program_info)(PluginHandle handle, uint32_t index);

    char* (*get_state)(PluginHandle handle);
    void  (*set_state)(PluginHandle handle, const char* data);

    void (*activate)(PluginHandle handle);
    void (*deactivate)(PluginHandle handle);
    void (*process)(PluginHandle handle, audio_sample_t** inBuffer, audio_sample_t** outBuffer, uint32_t frames, const Event* events, uint32_t eventCount);

    void (*ui_show)(PluginHandle handle, bool show);
    void (*ui_idle)(PluginHandle handle);

    void (*ui_set_parameter)(PluginHandle handle, uint32_t index, float value);
    void (*ui_set_midi_program)(PluginHandle handle, uint8_t channel, uint32_t bank, uint32_t program);

    intptr_t (*dispatcher)(PluginHandle handle, mapped_value_t opcode, int32_t index, intptr_t value, void* ptr, float opt);

} PluginDescriptor;

// -----------------------------------------------------------------------
// Register plugin

extern void carla_register_native_plugin(const PluginDescriptor* desc);

// -----------------------------------------------------------------------

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CARLA_NATIVE_H_INCLUDED
