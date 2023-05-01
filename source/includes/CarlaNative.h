/*
 * Carla Native Plugin API
 * Copyright (C) 2012-2023 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaDefines.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @defgroup CarlaNativeAPI Carla Native API
 *
 * The Carla Native API
 * @{
 */

typedef void* NativeHostHandle;
typedef void* NativePluginHandle;

/* ------------------------------------------------------------------------------------------------------------
 * enums */

typedef enum {
    NATIVE_PLUGIN_CATEGORY_NONE       = 0, /** Null plugin category.                                     */
    NATIVE_PLUGIN_CATEGORY_SYNTH      = 1, /** A synthesizer or generator.                               */
    NATIVE_PLUGIN_CATEGORY_DELAY      = 2, /** A delay or reverberator.                                  */
    NATIVE_PLUGIN_CATEGORY_EQ         = 3, /** An equalizer.                                             */
    NATIVE_PLUGIN_CATEGORY_FILTER     = 4, /** A filter.                                                 */
    NATIVE_PLUGIN_CATEGORY_DISTORTION = 5, /** A distortion plugin.                                      */
    NATIVE_PLUGIN_CATEGORY_DYNAMICS   = 6, /** A 'dynamic' plugin (amplifier, compressor, gate, etc).    */
    NATIVE_PLUGIN_CATEGORY_MODULATOR  = 7, /** A 'modulator' plugin (chorus, flanger, phaser, etc).      */
    NATIVE_PLUGIN_CATEGORY_UTILITY    = 8, /** An 'utility' plugin (analyzer, converter, mixer, etc).    */
    NATIVE_PLUGIN_CATEGORY_OTHER      = 9  /** Misc plugin (used to check if the plugin has a category). */
} NativePluginCategory;

typedef enum {
    NATIVE_PLUGIN_IS_RTSAFE            = 1 <<  0,
    NATIVE_PLUGIN_IS_SYNTH             = 1 <<  1,
    NATIVE_PLUGIN_HAS_UI               = 1 <<  2,
    NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS  = 1 <<  3,
    NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD = 1 <<  4,
    NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE   = 1 <<  6,
    NATIVE_PLUGIN_USES_MULTI_PROGS     = 1 <<  7, /** has 1 program per midi channel         */
    NATIVE_PLUGIN_USES_PANNING         = 1 <<  8, /** uses stereo balance if unset (default) */
    NATIVE_PLUGIN_USES_STATE           = 1 <<  9,
    NATIVE_PLUGIN_USES_TIME            = 1 << 10,
    NATIVE_PLUGIN_USES_PARENT_ID       = 1 << 11, /** can set transient hint to parent       */
    NATIVE_PLUGIN_HAS_INLINE_DISPLAY   = 1 << 12,
    NATIVE_PLUGIN_USES_CONTROL_VOLTAGE = 1 << 13,
    NATIVE_PLUGIN_REQUESTS_IDLE        = 1 << 15,
    NATIVE_PLUGIN_USES_UI_SIZE         = 1 << 16
} NativePluginHints;

typedef enum {
    NATIVE_PLUGIN_SUPPORTS_NOTHING          = 0,
    NATIVE_PLUGIN_SUPPORTS_PROGRAM_CHANGES  = 1 << 0, /** handles MIDI programs internally instead of host-exposed/exported */
    NATIVE_PLUGIN_SUPPORTS_CONTROL_CHANGES  = 1 << 1,
    NATIVE_PLUGIN_SUPPORTS_CHANNEL_PRESSURE = 1 << 2,
    NATIVE_PLUGIN_SUPPORTS_NOTE_AFTERTOUCH  = 1 << 3,
    NATIVE_PLUGIN_SUPPORTS_PITCHBEND        = 1 << 4,
    NATIVE_PLUGIN_SUPPORTS_ALL_SOUND_OFF    = 1 << 5,
    NATIVE_PLUGIN_SUPPORTS_EVERYTHING       = (1 << 6)-1
} NativePluginSupports;

typedef enum {
    NATIVE_PARAMETER_DESIGNATION_NONE = 0,
    NATIVE_PARAMETER_DESIGNATION_ENABLED
} NativeParameterDesignations;

typedef enum {
    NATIVE_PARAMETER_IS_OUTPUT        = 1 << 0,
    NATIVE_PARAMETER_IS_ENABLED       = 1 << 1,
    NATIVE_PARAMETER_IS_AUTOMATABLE   = 1 << 2,
    NATIVE_PARAMETER_IS_AUTOMABLE     = NATIVE_PARAMETER_IS_AUTOMATABLE, // there was a typo..
    NATIVE_PARAMETER_IS_BOOLEAN       = 1 << 3,
    NATIVE_PARAMETER_IS_INTEGER       = 1 << 4,
    NATIVE_PARAMETER_IS_LOGARITHMIC   = 1 << 5,
    NATIVE_PARAMETER_USES_SAMPLE_RATE = 1 << 6,
    NATIVE_PARAMETER_USES_SCALEPOINTS = 1 << 7,
    NATIVE_PARAMETER_USES_DESIGNATION = 1 << 8
} NativeParameterHints;

typedef enum {
    NATIVE_PLUGIN_OPCODE_NULL                = 0, /** nothing                   */
    NATIVE_PLUGIN_OPCODE_BUFFER_SIZE_CHANGED = 1, /** uses value                */
    NATIVE_PLUGIN_OPCODE_SAMPLE_RATE_CHANGED = 2, /** uses opt                  */
    NATIVE_PLUGIN_OPCODE_OFFLINE_CHANGED     = 3, /** uses value (0=off, 1=on)  */
    NATIVE_PLUGIN_OPCODE_UI_NAME_CHANGED     = 4, /** uses ptr                  */
    NATIVE_PLUGIN_OPCODE_GET_INTERNAL_HANDLE = 5, /** nothing                   */
    NATIVE_PLUGIN_OPCODE_IDLE                = 6, /** nothing                   */
    NATIVE_PLUGIN_OPCODE_UI_MIDI_EVENT       = 7, /** uses ptr                  */
    NATIVE_PLUGIN_OPCODE_HOST_USES_EMBED     = 8, /** nothing                   */
    NATIVE_PLUGIN_OPCODE_HOST_OPTION         = 9  /** uses index, value and ptr */
} NativePluginDispatcherOpcode;

typedef enum {
    NATIVE_HOST_OPCODE_NULL                  = 0,  /** nothing                                           */
    NATIVE_HOST_OPCODE_UPDATE_PARAMETER      = 1,  /** uses index, -1 for all                            */
    NATIVE_HOST_OPCODE_UPDATE_MIDI_PROGRAM   = 2,  /** uses index, -1 for all; may use value for channel */
    NATIVE_HOST_OPCODE_RELOAD_PARAMETERS     = 3,  /** nothing                                           */
    NATIVE_HOST_OPCODE_RELOAD_MIDI_PROGRAMS  = 4,  /** nothing                                           */
    NATIVE_HOST_OPCODE_RELOAD_ALL            = 5,  /** nothing                                           */
    NATIVE_HOST_OPCODE_UI_UNAVAILABLE        = 6,  /** nothing                                           */
    NATIVE_HOST_OPCODE_HOST_IDLE             = 7,  /** nothing                                           */
    NATIVE_HOST_OPCODE_INTERNAL_PLUGIN       = 8,  /** nothing                                           */
    NATIVE_HOST_OPCODE_QUEUE_INLINE_DISPLAY  = 9,  /** nothing                                           */
    NATIVE_HOST_OPCODE_UI_TOUCH_PARAMETER    = 10, /** uses index, value as bool                         */
    NATIVE_HOST_OPCODE_REQUEST_IDLE          = 11, /** nothing                                           */
    NATIVE_HOST_OPCODE_GET_FILE_PATH         = 12, /** uses ptr as string for file type                  */
    NATIVE_HOST_OPCODE_UI_RESIZE             = 13, /** uses index and value                              */
    NATIVE_HOST_OPCODE_PREVIEW_BUFFER_DATA   = 14  /** uses index as type, value as size, and ptr        */
} NativeHostDispatcherOpcode;

/* ------------------------------------------------------------------------------------------------------------
 * base structs */

typedef struct {
    const char* label;
    float value;
} NativeParameterScalePoint;

typedef struct {
    float def;
    float min;
    float max;
    float step;
    float stepSmall;
    float stepLarge;
} NativeParameterRanges;

#define PARAMETER_RANGES_DEFAULT_STEP       0.01f
#define PARAMETER_RANGES_DEFAULT_STEP_SMALL 0.0001f
#define PARAMETER_RANGES_DEFAULT_STEP_LARGE 0.1f

typedef struct {
    NativeParameterHints hints;
    const char* name;
    const char* unit;
    NativeParameterRanges ranges;

    uint32_t scalePointCount;
    const NativeParameterScalePoint* scalePoints;

    const char* comment;
    const char* groupName;

    uint designation;
} NativeParameter;

typedef struct {
    uint32_t time;
    uint8_t  port;
    uint8_t  size;
    uint8_t  data[4];
} NativeMidiEvent;

typedef struct {
    uint32_t bank;
    uint32_t program;
    const char* name;
} NativeMidiProgram;

typedef struct {
    bool valid;

    int32_t bar;  /** current bar              */
    int32_t beat; /** current beat-within-bar  */
    double  tick; /** current tick-within-beat */
    double  barStartTick;

    float beatsPerBar; /** time signature "numerator"  */
    float beatType;    /** time signature "denominator" */

    double ticksPerBeat;
    double beatsPerMinute;
} NativeTimeInfoBBT;

typedef struct {
    bool playing;
    uint64_t frame;
    uint64_t usecs;
    NativeTimeInfoBBT bbt;
} NativeTimeInfo;

typedef struct {
    unsigned char* data;
    int width;
    int height;
    int stride;
} NativeInlineDisplayImageSurface;

typedef struct {
    float minimum, maximum;
} NativePortRange;

/* ------------------------------------------------------------------------------------------------------------
 * HostDescriptor */

typedef struct {
    NativeHostHandle handle;
    const char* resourceDir;
    const char* uiName;
    uintptr_t   uiParentId;

    uint32_t (*get_buffer_size)(NativeHostHandle handle);
    double   (*get_sample_rate)(NativeHostHandle handle);
    bool     (*is_offline)(NativeHostHandle handle);

    const NativeTimeInfo* (*get_time_info)(NativeHostHandle handle);
    bool                  (*write_midi_event)(NativeHostHandle handle, const NativeMidiEvent* event);

    void (*ui_parameter_changed)(NativeHostHandle handle, uint32_t index, float value);
    void (*ui_midi_program_changed)(NativeHostHandle handle, uint8_t channel, uint32_t bank, uint32_t program);
    void (*ui_custom_data_changed)(NativeHostHandle handle, const char* key, const char* value);
    void (*ui_closed)(NativeHostHandle handle);

    const char* (*ui_open_file)(NativeHostHandle handle, bool isDir, const char* title, const char* filter);
    const char* (*ui_save_file)(NativeHostHandle handle, bool isDir, const char* title, const char* filter);

    intptr_t (*dispatcher)(NativeHostHandle handle,
                           NativeHostDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt);

} NativeHostDescriptor;

/* ------------------------------------------------------------------------------------------------------------
 * PluginDescriptor */

typedef struct _NativePluginDescriptor {
    const NativePluginCategory category;
    const NativePluginHints hints;
    const NativePluginSupports supports;
    const uint32_t audioIns;
    const uint32_t audioOuts;
    const uint32_t midiIns;
    const uint32_t midiOuts;
    const uint32_t paramIns;
    const uint32_t paramOuts;
    const char* const name;
    const char* const label;
    const char* const maker;
    const char* const copyright;

    NativePluginHandle (*instantiate)(const NativeHostDescriptor* host);
    void               (*cleanup)(NativePluginHandle handle);

    uint32_t               (*get_parameter_count)(NativePluginHandle handle);
    const NativeParameter* (*get_parameter_info)(NativePluginHandle handle, uint32_t index);
    float                  (*get_parameter_value)(NativePluginHandle handle, uint32_t index);

    uint32_t                 (*get_midi_program_count)(NativePluginHandle handle);
    const NativeMidiProgram* (*get_midi_program_info)(NativePluginHandle handle, uint32_t index);

    void (*set_parameter_value)(NativePluginHandle handle, uint32_t index, float value);
    void (*set_midi_program)(NativePluginHandle handle, uint8_t channel, uint32_t bank, uint32_t program);
    void (*set_custom_data)(NativePluginHandle handle, const char* key, const char* value);

    void (*ui_show)(NativePluginHandle handle, bool show);
    void (*ui_idle)(NativePluginHandle handle);

    void (*ui_set_parameter_value)(NativePluginHandle handle, uint32_t index, float value);
    void (*ui_set_midi_program)(NativePluginHandle handle, uint8_t channel, uint32_t bank, uint32_t program);
    void (*ui_set_custom_data)(NativePluginHandle handle, const char* key, const char* value);

    void (*activate)(NativePluginHandle handle);
    void (*deactivate)(NativePluginHandle handle);

    /* FIXME for v3.0, use const for the input buffer */
    void (*process)(NativePluginHandle handle,
                    float** inBuffer, float** outBuffer, uint32_t frames,
                    const NativeMidiEvent* midiEvents, uint32_t midiEventCount);

    char* (*get_state)(NativePluginHandle handle);
    void  (*set_state)(NativePluginHandle handle, const char* data);

    intptr_t (*dispatcher)(NativePluginHandle handle,
                           NativePluginDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt);

    /* placed at the end for backwards compatibility. only valid if NATIVE_PLUGIN_HAS_INLINE_DISPLAY is set */
    const NativeInlineDisplayImageSurface* (*render_inline_display)(NativePluginHandle handle,
                                                                    uint32_t width, uint32_t height);

    /* placed at the end for backwards compatibility. only valid if NATIVE_PLUGIN_USES_CONTROL_VOLTAGE is set */
    const uint32_t cvIns;
    const uint32_t cvOuts;
    const char* (*get_buffer_port_name)(NativePluginHandle handle, uint32_t index, bool isOutput);
    const NativePortRange* (*get_buffer_port_range)(NativePluginHandle handle, uint32_t index, bool isOutput);

    /* placed at the end for backwards compatibility. only valid if NATIVE_PLUGIN_USES_UI_SIZE is set */
    uint16_t ui_width, ui_height;

} NativePluginDescriptor;

/* ------------------------------------------------------------------------------------------------------------
 * Register plugin */

/** Implemented by host */
extern void carla_register_native_plugin(const NativePluginDescriptor* desc);

/** Called once on host init */
void carla_register_all_native_plugins(void);

/** Get meta-data only */
CARLA_API_EXPORT
const NativePluginDescriptor* carla_get_native_plugins_data(uint32_t* count);

/* ------------------------------------------------------------------------------------------------------------ */

/**@}*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CARLA_NATIVE_H_INCLUDED */
