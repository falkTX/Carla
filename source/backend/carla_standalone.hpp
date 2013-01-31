/*
 * Carla Standalone API
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_STANDALONE_HPP__
#define __CARLA_STANDALONE_HPP__

#include "carla_backend.hpp"

// TODO - create struct for internal plugin info
// TODO - dont strdup() on const-char* returns, use static char[STR_MAX]

/*!
 * @defgroup CarlaBackendStandalone Carla Backend Standalone
 *
 * The Carla Backend Standalone API
 *
 * @{
 */

typedef CarlaBackend::BinaryType CarlaBinaryType;
typedef CarlaBackend::PluginType CarlaPluginType;
typedef CarlaBackend::PluginCategory CarlaPluginCategory;

struct CarlaPluginInfo {
    CarlaPluginType type;
    CarlaPluginCategory category;
    unsigned int hints;
    const char* binary;
    const char* name;
    const char* label;
    const char* maker;
    const char* copyright;
    long uniqueId;

    CarlaPluginInfo()
        : type(CarlaBackend::PLUGIN_NONE),
          category(CarlaBackend::PLUGIN_CATEGORY_NONE),
          hints(0x0),
          binary(nullptr),
          name(nullptr),
          label(nullptr),
          maker(nullptr),
          copyright(nullptr),
          uniqueId(0) {}

    ~CarlaPluginInfo()
    {
        std::free((void*)label);
        std::free((void*)maker);
        std::free((void*)copyright);
    }
};

struct CarlaNativePluginInfo {
    CarlaPluginCategory category;
    unsigned int hints;
    uint32_t audioIns;
    uint32_t audioOuts;
    uint32_t midiIns;
    uint32_t midiOuts;
    uint32_t parameterIns;
    uint32_t parameterOuts;
    const char* name;
    const char* label;
    const char* maker;
    const char* copyright;

    CarlaNativePluginInfo()
        : category(CarlaBackend::PLUGIN_CATEGORY_NONE),
          hints(0x0),
          audioIns(0),
          audioOuts(0),
          midiIns(0),
          midiOuts(0),
          parameterIns(0),
          parameterOuts(0),
          name(nullptr),
          label(nullptr),
          maker(nullptr),
          copyright(nullptr) {}
};

struct CarlaPortCountInfo {
    uint32_t ins;
    uint32_t outs;
    uint32_t total;

    CarlaPortCountInfo()
        : ins(0),
          outs(0),
          total(0) {}
};

struct CarlaParameterInfo {
    const char* name;
    const char* symbol;
    const char* unit;
    uint32_t scalePointCount;

    CarlaParameterInfo()
        : name(nullptr),
          symbol(nullptr),
          unit(nullptr),
          scalePointCount(0) {}

    ~CarlaParameterInfo()
    {
        std::free((void*)name);
        std::free((void*)symbol);
        std::free((void*)unit);
    }
};

struct CarlaScalePointInfo {
    float value;
    const char* label;

    CarlaScalePointInfo()
        : value(0.0f),
          label(nullptr) {}

    ~CarlaScalePointInfo()
    {
        std::free((void*)label);
    }
};

CARLA_EXPORT const char* carla_get_extended_license_text();

CARLA_EXPORT unsigned int carla_get_engine_driver_count();
CARLA_EXPORT const char* carla_get_engine_driver_name(unsigned int index);

CARLA_EXPORT unsigned int carla_get_internal_plugin_count();
CARLA_EXPORT const CarlaNativePluginInfo* carla_get_internal_plugin_info(unsigned int internalPluginId);

CARLA_EXPORT bool carla_engine_init(const char* driverName, const char* clientName);
CARLA_EXPORT bool carla_engine_close();
CARLA_EXPORT bool carla_is_engine_running();

CARLA_EXPORT bool carla_add_plugin(CarlaBinaryType btype, CarlaPluginType ptype, const char* filename, const char* name, const char* label, void* extraPtr);
CARLA_EXPORT bool carla_remove_plugin(unsigned int pluginId);

CARLA_EXPORT const CarlaPluginInfo* carla_get_plugin_info(unsigned int pluginId);
CARLA_EXPORT const CarlaPortCountInfo* carla_get_audio_port_count_info(unsigned int pluginId);
CARLA_EXPORT const CarlaPortCountInfo* carla_get_midi_port_count_info(unsigned int pluginId);
CARLA_EXPORT const CarlaPortCountInfo* carla_get_parameter_count_info(unsigned int pluginId);
CARLA_EXPORT const CarlaParameterInfo* carla_get_parameter_info(unsigned int pluginId, uint32_t parameterId);
CARLA_EXPORT const CarlaScalePointInfo* carla_get_parameter_scalepoint_info(unsigned int pluginId, uint32_t parameterId, uint32_t scalePointId);

CARLA_EXPORT const CarlaBackend::ParameterData* carla_get_parameter_data(unsigned int pluginId, uint32_t parameterId);
CARLA_EXPORT const CarlaBackend::ParameterRanges* carla_get_parameter_ranges(unsigned int pluginId, uint32_t parameterId);
CARLA_EXPORT const CarlaBackend::MidiProgramData* carla_get_midi_program_data(unsigned int pluginId, uint32_t midiProgramId);
#if 0
CARLA_EXPORT const CustomData* get_custom_data(unsigned int pluginId, uint32_t customDataId);
CARLA_EXPORT const char* get_chunk_data(unsigned int pluginId);

CARLA_EXPORT uint32_t get_parameter_count(unsigned int pluginId);
CARLA_EXPORT uint32_t get_program_count(unsigned int pluginId);
CARLA_EXPORT uint32_t get_midi_program_count(unsigned int pluginId);
CARLA_EXPORT uint32_t get_custom_data_count(unsigned int pluginId);

CARLA_EXPORT const char* get_parameter_text(unsigned int pluginId, uint32_t parameterId);
CARLA_EXPORT const char* get_program_name(unsigned int pluginId, uint32_t programId);
CARLA_EXPORT const char* get_midi_program_name(unsigned int pluginId, uint32_t midiProgramId);
CARLA_EXPORT const char* get_real_plugin_name(unsigned int pluginId);

CARLA_EXPORT int32_t get_current_program_index(unsigned int pluginId);
CARLA_EXPORT int32_t get_current_midi_program_index(unsigned int pluginId);

CARLA_EXPORT double get_default_parameter_value(unsigned int pluginId, uint32_t parameterId);
CARLA_EXPORT double get_current_parameter_value(unsigned int pluginId, uint32_t parameterId);

CARLA_EXPORT double get_input_peak_value(unsigned int pluginId, unsigned short portId);
CARLA_EXPORT double get_output_peak_value(unsigned int pluginId, unsigned short portId);

CARLA_EXPORT void set_active(unsigned int pluginId, bool onOff);
CARLA_EXPORT void set_drywet(unsigned int pluginId, double value);
CARLA_EXPORT void set_volume(unsigned int pluginId, double value);
CARLA_EXPORT void set_balance_left(unsigned int pluginId, double value);
CARLA_EXPORT void set_balance_right(unsigned int pluginId, double value);

CARLA_EXPORT void set_parameter_value(unsigned int pluginId, uint32_t parameterId, double value);
CARLA_EXPORT void set_parameter_midi_channel(unsigned int pluginId, uint32_t parameterId, uint8_t channel);
CARLA_EXPORT void set_parameter_midi_cc(unsigned int pluginId, uint32_t parameterId, int16_t cc);
CARLA_EXPORT void set_program(unsigned int pluginId, uint32_t programId);
CARLA_EXPORT void set_midi_program(unsigned int pluginId, uint32_t midiProgramId);

CARLA_EXPORT void set_custom_data(unsigned int pluginId, const char* type, const char* key, const char* value);
CARLA_EXPORT void set_chunk_data(unsigned int pluginId, const char* chunkData);

CARLA_EXPORT void show_gui(unsigned int pluginId, bool yesNo);
CARLA_EXPORT void idle_guis();

CARLA_EXPORT void send_midi_note(unsigned int pluginId, uint8_t channel, uint8_t note, uint8_t velocity);
CARLA_EXPORT void prepare_for_save(unsigned int pluginId);

CARLA_EXPORT uint32_t get_buffer_size();
CARLA_EXPORT double   get_sample_rate();

CARLA_EXPORT const char* get_last_error();
CARLA_EXPORT const char* get_host_osc_url();

CARLA_EXPORT void set_callback_function(CallbackFunc func);
CARLA_EXPORT void set_option(OptionsType option, int value, const char* valueStr);

CARLA_EXPORT void nsm_announce(const char* url, int pid);
CARLA_EXPORT void nsm_reply_open();
CARLA_EXPORT void nsm_reply_save();
#endif

/**@}*/

#endif // __CARLA_STANDALONE_HPP__
