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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file
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
 *
 * @{
 */

typedef void* HostHandle;
typedef void* PluginHandle;

typedef enum _PluginCategory {
    PLUGIN_CATEGORY_NONE      = 0, //!< Null plugin category.
    PLUGIN_CATEGORY_SYNTH     = 1, //!< A synthesizer or generator.
    PLUGIN_CATEGORY_DELAY     = 2, //!< A delay or reverberator.
    PLUGIN_CATEGORY_EQ        = 3, //!< An equalizer.
    PLUGIN_CATEGORY_FILTER    = 4, //!< A filter.
    PLUGIN_CATEGORY_DYNAMICS  = 5, //!< A 'dynamic' plugin (amplifier, compressor, gate, etc).
    PLUGIN_CATEGORY_MODULATOR = 6, //!< A 'modulator' plugin (chorus, flanger, phaser, etc).
    PLUGIN_CATEGORY_UTILITY   = 7, //!< An 'utility' plugin (analyzer, converter, mixer, etc).
    PLUGIN_CATEGORY_OTHER     = 8  //!< Misc plugin (used to check if the plugin has a category).
} PluginCategory;

typedef enum _PluginHints {
    PLUGIN_IS_RTSAFE           = 1 << 0,
    PLUGIN_IS_SYNTH            = 1 << 1,
    PLUGIN_HAS_GUI             = 1 << 2,
    PLUGIN_USES_GUI_AS_FILE    = 1 << 3,
    PLUGIN_USES_PANNING        = 1 << 4, // uses stereo balance if unset (default)
    PLUGIN_USES_SINGLE_THREAD  = 1 << 5,
    PLUGIN_USES_STATE          = 1 << 6,
    PLUGIN_USES_STATIC_BUFFERS = 1 << 7
} PluginHints;

typedef enum _PluginSupports {
    PLUGIN_SUPPORTS_PROGRAM_CHANGES  = 1 << 0, // handles MIDI programs internally instead of host-exposed/exported
    PLUGIN_SUPPORTS_CONTROL_CHANGES  = 1 << 1,
    PLUGIN_SUPPORTS_CHANNEL_PRESSURE = 1 << 2,
    PLUGIN_SUPPORTS_NOTE_AFTERTOUCH  = 1 << 3,
    PLUGIN_SUPPORTS_PITCHBEND        = 1 << 4,
    PLUGIN_SUPPORTS_ALL_SOUND_OFF    = 1 << 5,
    PLUGIN_SUPPORTS_EVERYTHING       = PLUGIN_SUPPORTS_ALL_SOUND_OFF*2-1
} PluginSupports;

typedef enum _ParameterHints {
    PARAMETER_IS_OUTPUT        = 1 << 0,
    PARAMETER_IS_ENABLED       = 1 << 1,
    PARAMETER_IS_AUTOMABLE     = 1 << 2,
    PARAMETER_IS_BOOLEAN       = 1 << 3,
    PARAMETER_IS_INTEGER       = 1 << 4,
    PARAMETER_IS_LOGARITHMIC   = 1 << 5,
    PARAMETER_USES_SAMPLE_RATE = 1 << 6,
    PARAMETER_USES_SCALEPOINTS = 1 << 7,
    PARAMETER_USES_CUSTOM_TEXT = 1 << 8
} ParameterHints;

typedef enum _PluginDispatcherOpcode {
    PLUGIN_OPCODE_NULL                = 0, // nothing
    PLUGIN_OPCODE_BUFFER_SIZE_CHANGED = 1, // uses value
    PLUGIN_OPCODE_SAMPLE_RATE_CHANGED = 2, // uses opt
    PLUGIN_OPCODE_OFFLINE_CHANGED     = 3, // uses value
    PLUGIN_OPCODE_UI_NAME_CHANGED     = 4  // uses ptr
} PluginDispatcherOpcode;

typedef enum _HostDispatcherOpcode {
    HOST_OPCODE_NULL                  = 0,  // nothing
    HOST_OPCODE_SET_VOLUME            = 1,  // uses opt
    HOST_OPCODE_SET_DRYWET            = 2,  // uses opt
    HOST_OPCODE_SET_BALANCE_LEFT      = 3,  // uses opt
    HOST_OPCODE_SET_BALANCE_RIGHT     = 4,  // uses opt
    HOST_OPCODE_SET_PANNING           = 5,  // uses opt
    HOST_OPCODE_SET_PROCESS_PRECISION = 6,  // uses value
    HOST_OPCODE_UPDATE_PARAMETER      = 7,  // uses value, -1 for all
    HOST_OPCODE_UPDATE_MIDI_PROGRAM   = 8,  // uses value, -1 for all; may use index for channel
    HOST_OPCODE_RELOAD_PARAMETERS     = 9,  // nothing
    HOST_OPCODE_RELOAD_MIDI_PROGRAMS  = 10, // nothing
    HOST_OPCODE_RELOAD_ALL            = 11, // nothing
    HOST_OPCODE_UI_UNAVAILABLE        = 12  // nothing
} HostDispatcherOpcode;

typedef struct _ParameterScalePoint {
    const char* label;
    float value;
} ParameterScalePoint;

typedef struct _ParameterRanges {
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

typedef struct _Parameter {
    ParameterHints hints;
    const char* name;
    const char* unit;
    ParameterRanges ranges;

    uint32_t scalePointCount;
    ParameterScalePoint* scalePoints;
} Parameter;

typedef struct _MidiEvent {
    uint8_t  port;
    uint32_t time;
    uint8_t  data[4];
    uint8_t  size;
} MidiEvent;

typedef struct _MidiProgram {
    uint32_t bank;
    uint32_t program;
    const char* name;
} MidiProgram;

typedef struct _TimeInfoBBT {
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

typedef struct _TimeInfo {
    bool playing;
    uint64_t frame;
    uint64_t usecs;
    TimeInfoBBT bbt;
} TimeInfo;

typedef struct _HostDescriptor {
    HostHandle handle;
    const char* resource_dir;
    const char* ui_name;

    uint32_t (*get_buffer_size)(HostHandle handle);
    double   (*get_sample_rate)(HostHandle handle);
    bool     (*is_offline)(HostHandle handle);

    const TimeInfo* (*get_time_info)(HostHandle handle);
    bool            (*write_midi_event)(HostHandle handle, const MidiEvent* event);

    void (*ui_parameter_changed)(HostHandle handle, uint32_t index, float value);
    void (*ui_midi_program_changed)(HostHandle handle, uint8_t channel, uint32_t bank, uint32_t program);
    void (*ui_custom_data_changed)(HostHandle handle, const char* key, const char* value);
    void (*ui_closed)(HostHandle handle);

    const char* (*ui_open_file)(HostHandle handle, bool isDir, const char* title, const char* filter);
    const char* (*ui_save_file)(HostHandle handle, bool isDir, const char* title, const char* filter);

    intptr_t (*dispatcher)(HostHandle handle, HostDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt);

} HostDescriptor;

typedef struct _PluginDescriptor {
    const PluginCategory category;
    const PluginHints hints;
    const PluginSupports supports;
    const uint32_t audioIns;
    const uint32_t audioOuts;
    const uint32_t midiIns;
    const uint32_t midiOuts;
    const uint32_t parameterIns;
    const uint32_t parameterOuts;
    const char* const name;
    const char* const label;
    const char* const maker;
    const char* const copyright;

    PluginHandle (*instantiate)(const struct _PluginDescriptor* _this_, HostDescriptor* host);
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
    void (*process)(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents);

    char* (*get_state)(PluginHandle handle);
    void  (*set_state)(PluginHandle handle, const char* data);

    intptr_t (*dispatcher)(PluginHandle handle, PluginDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt);

} PluginDescriptor;

// -----------------------------------------------------------------------

// Register plugin
void carla_register_native_plugin(const PluginDescriptor* desc);

// Simple plugins
void carla_register_native_plugin_bypass();
void carla_register_native_plugin_lfo();
void carla_register_native_plugin_midiSequencer();
void carla_register_native_plugin_midiSplit();
void carla_register_native_plugin_midiThrough();
void carla_register_native_plugin_midiTranspose();
void carla_register_native_plugin_nekofilter();
void carla_register_native_plugin_sunvoxfile();

#ifndef BUILD_BRIDGE
// Carla
void carla_register_native_plugin_carla();
#endif

#ifdef WANT_AUDIOFILE
// AudioFile
void carla_register_native_plugin_audiofile();
#endif

#ifdef WANT_MIDIFILE
// MidiFile
void carla_register_native_plugin_midifile();
#endif

#ifdef WANT_OPENGL
// DISTRHO plugins (OpenGL)
void carla_register_native_plugin_3BandEQ();
void carla_register_native_plugin_3BandSplitter();
void carla_register_native_plugin_Nekobi();
void carla_register_native_plugin_PingPongPan();
// void carla_register_native_plugin_StereoEnhancer();
#endif

// DISTRHO plugins (Qt)
// void carla_register_native_plugin_Notes();

#ifdef WANT_ZYNADDSUBFX
// ZynAddSubFX
void carla_register_native_plugin_zynaddsubfx();
#endif

// -----------------------------------------------------------------------

/**@}*/

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CARLA_NATIVE_H_INCLUDED
