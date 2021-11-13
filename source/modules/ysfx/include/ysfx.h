// Copyright 2021 Jean Pierre Cimalando
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once
#if !defined(YSFX_INCLUDED_YSFX_H)
#define YSFX_INCLUDED_YSFX_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#if defined(_WIN32)
#   include <wchar.h>
#endif

//------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(YSFX_API)
#   if defined(_WIN32) && defined(YSFX_DLL_BUILD)
#       define YSFX_API __declspec(dllexport)
#   elif defined(__GNUC__)
#       define YSFX_API __attribute__((visibility("default")))
#   else
#       define YSFX_API
#   endif
#endif

//------------------------------------------------------------------------------
// YSFX definitions

typedef double ysfx_real;

enum {
    ysfx_max_sliders = 64,
    ysfx_max_channels = 64,
    ysfx_max_midi_buses = 16,
    ysfx_max_triggers = 10,
};

typedef enum ysfx_log_level_e {
    ysfx_log_info,
    ysfx_log_warning,
    ysfx_log_error,
} ysfx_log_level;

typedef void (ysfx_log_reporter_t)(intptr_t userdata, ysfx_log_level level, const char *message);

//------------------------------------------------------------------------------
// YSFX configuration

typedef struct ysfx_config_s ysfx_config_t;
typedef struct ysfx_audio_format_s ysfx_audio_format_t;

// create a new configuration
YSFX_API ysfx_config_t *ysfx_config_new();
// delete a configuration
YSFX_API void ysfx_config_free(ysfx_config_t *config);
// increase the reference counter
YSFX_API void ysfx_config_add_ref(ysfx_config_t *config);
// set the path of the import root, a folder usually named "Effects"
YSFX_API void ysfx_set_import_root(ysfx_config_t *config, const char *root);
// set the path of the data root, a folder usually named "Data"
YSFX_API void ysfx_set_data_root(ysfx_config_t *config, const char *root);
// get the path of the import root, a folder usually named "Effects"
YSFX_API const char *ysfx_get_import_root(ysfx_config_t *config);
// get the path of the data root, a folder usually named "Data"
YSFX_API const char *ysfx_get_data_root(ysfx_config_t *config);
// guess the undefined root folders, based on the path to the JSFX file
YSFX_API void ysfx_guess_file_roots(ysfx_config_t *config, const char *sourcepath);
// register an audio format into the system
YSFX_API void ysfx_register_audio_format(ysfx_config_t *config, ysfx_audio_format_t *afmt);
// register the builtin audio formats (at least WAV file support)
YSFX_API void ysfx_register_builtin_audio_formats(ysfx_config_t *config);
// set the log reporting function
YSFX_API void ysfx_set_log_reporter(ysfx_config_t *config, ysfx_log_reporter_t *reporter);
// set the callback user data
YSFX_API void ysfx_set_user_data(ysfx_config_t *config, intptr_t userdata);

// get a string which textually represents the log level
YSFX_API const char *ysfx_log_level_string(ysfx_log_level level);

//------------------------------------------------------------------------------
// YSFX effect

typedef struct ysfx_s ysfx_t;

// create a new effect, taking a reference to config
YSFX_API ysfx_t *ysfx_new(ysfx_config_t *config);
// delete an effect
YSFX_API void ysfx_free(ysfx_t *fx);
// increase the reference counter
YSFX_API void ysfx_add_ref(ysfx_t *fx);

// get the configuration
YSFX_API ysfx_config_t *ysfx_get_config(ysfx_t *fx);

typedef enum ysfx_load_option_e {
    // skip imports; useful just for accessing header information and nothing else
    ysfx_load_ignoring_imports = 1,
} ysfx_load_option_t;

// load the source code from file without compiling
YSFX_API bool ysfx_load_file(ysfx_t *fx, const char *filepath, uint32_t loadopts);
// unload the source code and any compiled code
YSFX_API void ysfx_unload(ysfx_t *fx);
// check whether the effect is loaded
YSFX_API bool ysfx_is_loaded(ysfx_t *fx);

// get the name of the effect
YSFX_API const char *ysfx_get_name(ysfx_t *fx);
// get the path of the file which is loaded
YSFX_API const char *ysfx_get_file_path(ysfx_t *fx);
// get the author of the effect
YSFX_API const char *ysfx_get_author(ysfx_t *fx);
// get the number of tags of the effect
#define ysfx_get_num_tags(fx) ysfx_get_tags((fx), NULL, 0)
// get the list of tags of the effect
YSFX_API uint32_t ysfx_get_tags(ysfx_t *fx, const char **dest, uint32_t destsize);
// get a single tag of the effect
YSFX_API const char *ysfx_get_tag(ysfx_t *fx, uint32_t index);
// get the number of inputs
YSFX_API uint32_t ysfx_get_num_inputs(ysfx_t *fx);
// get the number of outputs
YSFX_API uint32_t ysfx_get_num_outputs(ysfx_t *fx);
// get the name of the input
YSFX_API const char *ysfx_get_input_name(ysfx_t *fx, uint32_t index);
// get the name of the output
YSFX_API const char *ysfx_get_output_name(ysfx_t *fx, uint32_t index);
// get whether this effect wants metering
YSFX_API bool ysfx_wants_meters(ysfx_t *fx);
// get requested dimensions of the graphics area; 0 means host should decide
YSFX_API bool ysfx_get_gfx_dim(ysfx_t *fx, uint32_t dim[2]);

typedef enum ysfx_section_type_e {
    ysfx_section_init = 1,
    ysfx_section_slider = 2,
    ysfx_section_block = 3,
    ysfx_section_sample = 4,
    ysfx_section_gfx = 5,
    ysfx_section_serialize = 6,
} ysfx_section_type_t;

// get whether the source has the given section
YSFX_API bool ysfx_has_section(ysfx_t *fx, uint32_t type);

typedef struct ysfx_slider_range_s {
    ysfx_real def;
    ysfx_real min;
    ysfx_real max;
    ysfx_real inc;
} ysfx_slider_range_t;

// determine if slider exists; call from 0 to max-1 to scan available ones
YSFX_API bool ysfx_slider_exists(ysfx_t *fx, uint32_t index);
// get the name of a slider
YSFX_API const char *ysfx_slider_get_name(ysfx_t *fx, uint32_t index);
// get the range of a slider
YSFX_API bool ysfx_slider_get_range(ysfx_t *fx, uint32_t index, ysfx_slider_range_t *range);
// get whether the slider is an enumeration
YSFX_API bool ysfx_slider_is_enum(ysfx_t *fx, uint32_t index);
// get the number of labels for the enumeration slider
#define ysfx_slider_get_enum_size(fx, index) ysfx_slider_get_enum_names((fx), (index), NULL, 0)
// get the list of labels for the enumeration slider
YSFX_API uint32_t ysfx_slider_get_enum_names(ysfx_t *fx, uint32_t index, const char **dest, uint32_t destsize);
// get a single label for the enumeration slider
YSFX_API const char *ysfx_slider_get_enum_name(ysfx_t *fx, uint32_t slider_index, uint32_t enum_index);
// get whether the slider is a path (implies enumeration)
YSFX_API bool ysfx_slider_is_path(ysfx_t *fx, uint32_t index);
// get whether the slider is initially visible
YSFX_API bool ysfx_slider_is_initially_visible(ysfx_t *fx, uint32_t index);

// get the value of the slider
YSFX_API ysfx_real ysfx_slider_get_value(ysfx_t *fx, uint32_t index);
// set the value of the slider, and call @slider later if value has changed
YSFX_API void ysfx_slider_set_value(ysfx_t *fx, uint32_t index, ysfx_real value);

typedef enum ysfx_compile_option_e {
    // skip compiling the @serialize section
    ysfx_compile_no_serialize = 1 << 0,
    // skip compiling the @gfx section
    ysfx_compile_no_gfx = 1 << 1,
} ysfx_compile_option_t;

// compile the previously loaded source
YSFX_API bool ysfx_compile(ysfx_t *fx, uint32_t compileopts);
// check whether the effect is compiled
YSFX_API bool ysfx_is_compiled(ysfx_t *fx);

// get the block size
YSFX_API uint32_t ysfx_get_block_size(ysfx_t *fx);
// get the sample rate
YSFX_API ysfx_real ysfx_get_sample_rate(ysfx_t *fx);
// update the block size; don't forget to call @init
YSFX_API void ysfx_set_block_size(ysfx_t *fx, uint32_t blocksize);
// update the sample rate; don't forget to call @init
YSFX_API void ysfx_set_sample_rate(ysfx_t *fx, ysfx_real samplerate);

// set the capacity of the MIDI buffer
YSFX_API void ysfx_set_midi_capacity(ysfx_t *fx, uint32_t capacity, bool extensible);

// activate and invoke @init
YSFX_API void ysfx_init(ysfx_t *fx);

// get the output latency
YSFX_API ysfx_real ysfx_get_pdc_delay(ysfx_t *fx);
// get the range of channels where output latency applies (end not included)
YSFX_API void ysfx_get_pdc_channels(ysfx_t *fx, uint32_t channels[2]);
// get whether the output latency applies to MIDI as well
YSFX_API bool ysfx_get_pdc_midi(ysfx_t *fx);

typedef enum ysfx_playback_state_e {
    ysfx_playback_error = 0,
    ysfx_playback_playing = 1,
    ysfx_playback_paused = 2,
    ysfx_playback_recording = 5,
    ysfx_playback_recording_paused = 6,
} ysfx_playback_state_t;

typedef struct ysfx_time_info_s {
    // tempo in beats/minute
    ysfx_real tempo;
    // state of the playback (ysfx_playback_state_t)
    uint32_t playback_state;
    // time position in seconds
    ysfx_real time_position;
    // time position in beats
    ysfx_real beat_position;
    // time signature in fraction form
    uint32_t time_signature[2];
} ysfx_time_info_t;

// update time information; do this before processing the cycle
YSFX_API void ysfx_set_time_info(ysfx_t *fx, const ysfx_time_info_t *info);

typedef struct ysfx_midi_event_s {
    // the bus number
    uint32_t bus;
    // the frame when it happens within the cycle
    uint32_t offset;
    // the size of the message
    uint32_t size;
    // the contents of the message
    const uint8_t *data;
} ysfx_midi_event_t;

// send MIDI, it will be processed during the cycle
YSFX_API bool ysfx_send_midi(ysfx_t *fx, const ysfx_midi_event_t *event);
// receive MIDI, after having processed the cycle
YSFX_API bool ysfx_receive_midi(ysfx_t *fx, ysfx_midi_event_t *event);
// receive MIDI from a single bus (do not mix with API above, use either)
YSFX_API bool ysfx_receive_midi_from_bus(ysfx_t *fx, uint32_t bus, ysfx_midi_event_t *event);

// send a trigger, it will be processed during the cycle
YSFX_API bool ysfx_send_trigger(ysfx_t *fx, uint32_t index);

// get a bit mask of sliders whose values must be redisplayed, and clear it to zero
YSFX_API uint64_t ysfx_fetch_slider_changes(ysfx_t *fx);
// get a bit mask of sliders whose values must be automated, and clear it to zero
YSFX_API uint64_t ysfx_fetch_slider_automations(ysfx_t *fx);
// get a bit mask of sliders currently visible
YSFX_API uint64_t ysfx_get_slider_visibility(ysfx_t *fx);

// process a cycle in 32-bit float
YSFX_API void ysfx_process_float(ysfx_t *fx, const float *const *ins, float *const *outs, uint32_t num_ins, uint32_t num_outs, uint32_t num_frames);
// process a cycle in 64-bit float
YSFX_API void ysfx_process_double(ysfx_t *fx, const double *const *ins, double *const *outs, uint32_t num_ins, uint32_t num_outs, uint32_t num_frames);

typedef struct ysfx_state_slider_s {
    // index of the slider
    uint32_t index;
    // value of the slider
    ysfx_real value;
} ysfx_state_slider_t;

typedef struct ysfx_state_s {
    // values of the sliders
    ysfx_state_slider_t *sliders;
    // number of sliders
    uint32_t slider_count;
    // serialized data
    uint8_t *data;
    // size of serialized data
    size_t data_size;
} ysfx_state_t;

// load state
YSFX_API bool ysfx_load_state(ysfx_t *fx, ysfx_state_t *state);
// save current state; release this object when done
YSFX_API ysfx_state_t *ysfx_save_state(ysfx_t *fx);
// release a saved state object
YSFX_API void ysfx_state_free(ysfx_state_t *state);
// duplicate a state object
YSFX_API ysfx_state_t *ysfx_state_dup(ysfx_state_t *state);

typedef struct ysfx_preset_s {
    // name of the preset
    char *name;
    // state of the preset
    ysfx_state_t *state;
} ysfx_preset_t;

typedef struct ysfx_bank_s {
    // name of the bank
    char *name;
    // list of presets
    ysfx_preset_t *presets;
    // number of programs
    uint32_t preset_count;
} ysfx_bank_t;

// get the path of the RPL preset bank of the loaded JSFX, if present
YSFX_API const char *ysfx_get_bank_path(ysfx_t *fx);
// read a preset bank from RPL file
YSFX_API ysfx_bank_t *ysfx_load_bank(const char *path);
// free a preset bank
YSFX_API void ysfx_bank_free(ysfx_bank_t *bank);

// type of a function which can enumerate VM variables; returning 0 ends the search
typedef int (ysfx_enum_vars_callback_t)(const char *name, ysfx_real *var, void *userdata);
// enumerate all variables currently in the VM
YSFX_API void ysfx_enum_vars(ysfx_t *fx, ysfx_enum_vars_callback_t *callback, void *userdata);
// find a single variable in the VM
YSFX_API ysfx_real *ysfx_find_var(ysfx_t *fx, const char *name);
// read a chunk of virtual memory from the VM
YSFX_API void ysfx_read_vmem(ysfx_t *fx, uint32_t addr, ysfx_real *dest, uint32_t count);

//------------------------------------------------------------------------------
// YSFX graphics

// NOTE: all `ysfx_gfx_*` functions must be invoked from a dedicated UI thread

typedef struct ysfx_gfx_config_s {
    // opaque user data passed to callbacks
    void *user_data;
    // the width of the frame buffer (having the scale factor applied)
    uint32_t pixel_width;
    // the height of the frame buffer (having the scale factor applied)
    uint32_t pixel_height;
    // the distance in bytes between lines; if 0, it defaults to (4*width)
    // currently it is required to be a multiple of 4
    uint32_t pixel_stride;
    // the pixel data of the frame buffer, of size (stride*height) bytes
    // the byte order in little-endian is 'BGRA', big-endian is 'ARGB'
    uint8_t *pixels;
    // the scale factor of the display; 1.0 or greater, 2.0 for Retina display
    ysfx_real scale_factor;
    // show a menu and run it synchronously; returns an item ID >= 1, or 0 if none
    int32_t (*show_menu)(void *user_data, const char *menu_spec, int32_t xpos, int32_t ypos);
    // change the cursor
    void (*set_cursor)(void *user_data, int32_t cursor);
    // if index is not -1, get the dropped file at this index (otherwise null)
    // if index is -1, clear the list of dropped files, and return null
    const char *(*get_drop_file)(void *user_data, int32_t index);
} ysfx_gfx_config_t;

// set up the graphics rendering
YSFX_API void ysfx_gfx_setup(ysfx_t *fx, ysfx_gfx_config_t *gc);
// get whether the current effect is requesting Retina support
YSFX_API bool ysfx_gfx_wants_retina(ysfx_t *fx);
// push a key to the input queue
YSFX_API void ysfx_gfx_add_key(ysfx_t *fx, uint32_t mods, uint32_t key, bool press);
// update mouse information; position is relative to canvas; wheel should be in steps normalized to ±1.0
YSFX_API void ysfx_gfx_update_mouse(ysfx_t *fx, uint32_t mods, int32_t xpos, int32_t ypos, uint32_t buttons, ysfx_real wheel, ysfx_real hwheel);
// invoke @gfx to paint the graphics; returns whether the framer buffer is modified
YSFX_API bool ysfx_gfx_run(ysfx_t *fx);

//------------------------------------------------------------------------------
// YSFX key map

// these key definitions match those of pugl

enum {
    ysfx_mod_shift = 1 << 0,
    ysfx_mod_ctrl = 1 << 1,
    ysfx_mod_alt = 1 << 2,
    ysfx_mod_super = 1 << 3,
};

enum {
    ysfx_key_backspace = 0x08,
    ysfx_key_escape = 0x1b,
    ysfx_key_delete = 0x7f,

    ysfx_key_f1 = 0xe000,
    ysfx_key_f2,
    ysfx_key_f3,
    ysfx_key_f4,
    ysfx_key_f5,
    ysfx_key_f6,
    ysfx_key_f7,
    ysfx_key_f8,
    ysfx_key_f9,
    ysfx_key_f10,
    ysfx_key_f11,
    ysfx_key_f12,
    ysfx_key_left,
    ysfx_key_up,
    ysfx_key_right,
    ysfx_key_down,
    ysfx_key_page_up,
    ysfx_key_page_down,
    ysfx_key_home,
    ysfx_key_end,
    ysfx_key_insert,
};

enum {
    ysfx_button_left = 1 << 0,
    ysfx_button_middle = 1 << 1,
    ysfx_button_right = 1 << 2,
};

//------------------------------------------------------------------------------
// YSFX audio formats

typedef struct ysfx_audio_reader_s ysfx_audio_reader_t;

typedef struct ysfx_audio_file_info_s {
    uint32_t channels;
    ysfx_real sample_rate;
} ysfx_audio_file_info_t;

typedef struct ysfx_audio_format_s {
    // quickly checks if this format would be able to handle the given file
    bool (*can_handle)(const char *path);
    // open an audio file of this format for reading
    ysfx_audio_reader_t *(*open)(const char *path);
    // close the audio file
    void (*close)(ysfx_audio_reader_t *reader);
    // get the sample rate and the channel count
    ysfx_audio_file_info_t (*info)(ysfx_audio_reader_t *reader);
    // get the number of samples left to read
    uint64_t (*avail)(ysfx_audio_reader_t *reader);
    // move the read pointer back to the beginning
    void (*rewind)(ysfx_audio_reader_t *reader);
    // read the next block of samples
    uint64_t (*read)(ysfx_audio_reader_t *reader, ysfx_real *samples, uint64_t count);
} ysfx_audio_format_t;

//------------------------------------------------------------------------------

#ifdef __cplusplus
} // extern "C"
#endif

//------------------------------------------------------------------------------
// YSFX RAII helpers

#if defined(__cplusplus) && (__cplusplus >= 201103L || (defined(_MSC_VER) && _MSVC_LANG >= 201103L))
#include <memory>

#define YSFX_DEFINE_AUTO_PTR(aptr, styp, freefn)                \
    struct aptr##_deleter {                                     \
        void operator()(styp *x) const noexcept { freefn(x); }  \
    };                                                          \
    using aptr = std::unique_ptr<styp, aptr##_deleter>

YSFX_DEFINE_AUTO_PTR(ysfx_config_u, ysfx_config_t, ysfx_config_free);
YSFX_DEFINE_AUTO_PTR(ysfx_u, ysfx_t, ysfx_free);
YSFX_DEFINE_AUTO_PTR(ysfx_state_u, ysfx_state_t, ysfx_state_free);
YSFX_DEFINE_AUTO_PTR(ysfx_bank_u, ysfx_bank_t, ysfx_bank_free);
#endif // defined(__cplusplus) && (__cplusplus >= 201103L || (defined(_MSC_VER) && _MSVC_LANG >= 201103L))

//------------------------------------------------------------------------------

#endif // !defined(YSFX_INCLUDED_YSFX_H)
