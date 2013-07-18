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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file
 */

#ifndef CARLA_STANDALONE_HPP_INCLUDED
#define CARLA_STANDALONE_HPP_INCLUDED

#include "CarlaBackend.hpp"

/*!
 * @defgroup CarlaStandaloneAPI Carla Standalone API
 *
 * The Carla Standalone API.
 *
 * This API makes it possible to use the Carla Backend in a Standalone application.\n
 * All functions are C-compatible, making it possible to use this API in non-C++ hosts.
 *
 * None of the returned values in this API calls need to be deleted or free'd.\n
 * When a function fails (returns false or NULL), use carla_get_last_error() to find out what went wrong.
 *
 * @{
 */

/*!
 * @defgroup HelperTypedefs Helper typedefs
 *
 * Basic typedefs to help make code cleaner.
 * @{
 */
typedef CarlaBackend::BinaryType CarlaBinaryType;
typedef CarlaBackend::PluginType CarlaPluginType;
typedef CarlaBackend::PluginCategory CarlaPluginCategory;
typedef CarlaBackend::OptionsType CarlaOptionsType;
typedef CarlaBackend::CallbackType CarlaCallbackType;
typedef CarlaBackend::CallbackFunc CarlaCallbackFunc;
typedef CarlaBackend::ParameterData CarlaParameterData;
typedef CarlaBackend::ParameterRanges CarlaParameterRanges;
typedef CarlaBackend::MidiProgramData CarlaMidiProgramData;
typedef CarlaBackend::CustomData CarlaCustomData;
/**@}*/

/*!
 * Plugin information.
 * \see carla_get_plugin_info()
 */
struct CarlaPluginInfo {
    CarlaPluginType type;
    CarlaPluginCategory category;
    unsigned int hints;
    unsigned int optionsAvailable;
    unsigned int optionsEnabled;
    const char* binary;
    const char* name;
    const char* label;
    const char* maker;
    const char* copyright;
    const char* iconName;
    long uniqueId;
    uint32_t latency;

#ifndef DOXYGEN
    CarlaPluginInfo()
        : type(CarlaBackend::PLUGIN_NONE),
          category(CarlaBackend::PLUGIN_CATEGORY_NONE),
          hints(0x0),
          optionsAvailable(0x0),
          optionsEnabled(0x0),
          binary(nullptr),
          name(nullptr),
          label(nullptr),
          maker(nullptr),
          copyright(nullptr),
          iconName(nullptr),
          uniqueId(0),
          latency(0) {}

    ~CarlaPluginInfo()
    {
        if (label != nullptr)
        {
            delete[] label;
            label = nullptr;
        }
        if (maker != nullptr)
        {
            delete[] maker;
            maker = nullptr;
        }
        if (copyright != nullptr)
        {
            delete[] copyright;
            copyright = nullptr;
        }
    }
#endif
};

/*!
 * Native plugin information.
 * \see carla_get_internal_plugin_info()
 */
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

#ifndef DOXYGEN
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
#endif
};

/*!
 * Port count information, used for Audio and MIDI ports and parameters.
 * \see carla_get_audio_port_count_info()
 * \see carla_get_midi_port_count_info()
 * \see carla_get_parameter_count_info()
 */
struct CarlaPortCountInfo {
    uint32_t ins;
    uint32_t outs;
    uint32_t total;

#ifndef DOXYGEN
    CarlaPortCountInfo()
        : ins(0),
          outs(0),
          total(0) {}
#endif
};

/*!
 * Parameter information.
 * \see carla_get_parameter_info()
 */
struct CarlaParameterInfo {
    const char* name;
    const char* symbol;
    const char* unit;
    uint32_t scalePointCount;

#ifndef DOXYGEN
    CarlaParameterInfo()
        : name(nullptr),
          symbol(nullptr),
          unit(nullptr),
          scalePointCount(0) {}

    ~CarlaParameterInfo()
    {
        if (name != nullptr)
        {
            delete[] name;
            name = nullptr;
        }
        if (symbol != nullptr)
        {
            delete[] symbol;
            symbol = nullptr;
        }
        if (unit != nullptr)
        {
            delete[] unit;
            unit = nullptr;
        }
    }
#endif
};

/*!
 * Parameter scale point information.
 * \see carla_get_parameter_scalepoint_info()
 */
struct CarlaScalePointInfo {
    float value;
    const char* label;

#ifndef DOXYGEN
    CarlaScalePointInfo()
        : value(0.0f),
          label(nullptr) {}

    ~CarlaScalePointInfo()
    {
        if (label != nullptr)
        {
            delete[] label;
            label = nullptr;
        }
    }
#endif
};

/*!
 * Transport information.
 * \see carla_get_transport_info()
 */
struct CarlaTransportInfo {
    bool playing;
    uint32_t frame;
    int32_t bar;
    int32_t beat;
    int32_t tick;
    double bpm;

#ifndef DOXYGEN
    CarlaTransportInfo()
        : playing(false),
          frame(0),
          bar(0),
          beat(0),
          bpm(0.0) {}
#endif
};

/*!
 * Get the complete license text of used third-party code and features.\n
 * Returned string is in basic html format.
 */
CARLA_EXPORT const char* carla_get_extended_license_text();

/*!
 * Get the supported file types in carla_load_filename().\n
 * Returned string uses this syntax:
 * \code
 * "*.ext1;*.ext2;*.ext3"
 * \endcode
 */
CARLA_EXPORT const char* carla_get_supported_file_types();

/*!
 * Get how many engine drivers are available to use.
 */
CARLA_EXPORT unsigned int carla_get_engine_driver_count();

/*!
 * Get the engine driver name \a index.
 */
CARLA_EXPORT const char* carla_get_engine_driver_name(unsigned int index);

/*!
 * Get the device names of the engine driver at \a index (for use in non-JACK drivers).\n
 * May return NULL.
 */
CARLA_EXPORT const char** carla_get_engine_driver_device_names(unsigned int index);

/*!
 * Get how many internal plugins are available to use.
 */
CARLA_EXPORT unsigned int carla_get_internal_plugin_count();

/*!
 * Get information about the internal plugin \a internalPluginId.
 */
CARLA_EXPORT const CarlaNativePluginInfo* carla_get_internal_plugin_info(unsigned int internalPluginId);

/*!
 * Initialize the engine with driver \a driverName, using \a clientName for its internal name.\n
 * Make sure to call carla_engine_idle() at regular intervals afterwards.
 */
CARLA_EXPORT bool carla_engine_init(const char* driverName, const char* clientName);

/*!
 * Close the running engine.\n
 * This function always closes the engine even if it returns false.\n
 * When false is returned, something went wrong when closing the engine, but it was still closed nonetheless.
 */
CARLA_EXPORT bool carla_engine_close();

/*!
 * Idle the running engine.\n
 * \note This should never be called if the engine is not running.
 */
CARLA_EXPORT void carla_engine_idle();

/*!
 * Check if the engine is running.
 */
CARLA_EXPORT bool carla_is_engine_running();

/*!
 * Tell the engine it's about to close.\n
 * This is used to prevent the engine thread(s) from reactivating.
 */
CARLA_EXPORT void carla_set_engine_about_to_close();

/*!
 * Set the engine callback function to \a func.
 * Use \a ptr to pass a custom pointer to the callback.
 */
CARLA_EXPORT void carla_set_engine_callback(CarlaCallbackFunc func, void* ptr);

/*!
 * Set the engine option \a option.\n
 * With the exception of OPTION_PROCESS_NAME, OPTION_TRANSPORT_MODE and OPTION_PATH_*,
 * this function should not be called when the engine is running.
 */
CARLA_EXPORT void carla_set_engine_option(CarlaOptionsType option, int value, const char* valueStr);

/*!
 * Load \a filename of any type.\n
 * This will try to load a generic file as a plugin,
 * either by direct handling (GIG, SF2 and SFZ) or by using an internal plugin (like Audio and MIDI).
 * \see carla_get_supported_file_types()
 */
CARLA_EXPORT bool carla_load_filename(const char* filename);

/*!
 * Load \a filename project file.\n
 * (project files have *.carxp extension)
 * \note Already loaded plugins are not removed; call carla_remove_all_plugins() first if needed.
 */
CARLA_EXPORT bool carla_load_project(const char* filename);

/*!
 * Save current project to \a filename.\n
 * (project files have *.carxp extension)
 */
CARLA_EXPORT bool carla_save_project(const char* filename);

/*!
 * Connect patchbay ports \a portA and \a portB.
 */
CARLA_EXPORT bool carla_patchbay_connect(int portA, int portB);

/*!
 * Disconnect patchbay connection \a connectionId.
 */
CARLA_EXPORT bool carla_patchbay_disconnect(int connectionId);

/*!
 * Force the engine to resend all patchbay clients, ports and connections again.
 */
CARLA_EXPORT void carla_patchbay_refresh();

/*!
 * Start playback of the engine transport.
 */
CARLA_EXPORT void carla_transport_play();

/*!
 * Pause the engine transport.
 */
CARLA_EXPORT void carla_transport_pause();

/*!
 * Relocate the engine transport to \a frames.
 */
CARLA_EXPORT void carla_transport_relocate(uint32_t frames);

/*!
 * Get the current transport frame.
 */
CARLA_EXPORT uint32_t carla_get_current_transport_frame();

/*!
 * Get the engine transport information.
 */
CARLA_EXPORT const CarlaTransportInfo* carla_get_transport_info();

/*!
 * Add new plugin.\n
 * If you don't know the binary type, use BINARY_NATIVE.
 */
CARLA_EXPORT bool carla_add_plugin(CarlaBinaryType btype, CarlaPluginType ptype, const char* filename, const char* name, const char* label, const void* extraPtr);

/*!
 * Remove plugin with id \a pluginId.
 */
CARLA_EXPORT bool carla_remove_plugin(unsigned int pluginId);

/*!
 * Remove all plugins.
 */
CARLA_EXPORT void carla_remove_all_plugins();

/*!
 * Rename plugin with id \a pluginId to \a newName. \n
 * Returns the new name, or NULL if the operation failed.
 */
CARLA_EXPORT const char* carla_rename_plugin(unsigned int pluginId, const char* newName);

/*!
 * Clone plugin with id \a pluginId.
 */
CARLA_EXPORT bool carla_clone_plugin(unsigned int pluginId);

/*!
 * Prepare replace of plugin with id \a pluginId. \n
 * The next call to carla_add_plugin() will use this id, replacing the current plugin.
 * \note This function requires carla_add_plugin() to be called afterwards as soon as possible.
 */
CARLA_EXPORT bool carla_replace_plugin(unsigned int pluginId);

/*!
 * Switch plugins with id \a pluginIdA and \a pluginIdB.
 */
CARLA_EXPORT bool carla_switch_plugins(unsigned int pluginIdA, unsigned int pluginIdB);

/*!
 * Load the plugin state at \a filename.\n
 * (Plugin states have *.carxs extension).
 * \see carla_save_plugin_state()
 */
CARLA_EXPORT bool carla_load_plugin_state(unsigned int pluginId, const char* filename);

/*!
 * Load the plugin state at \a filename.\n
 * (Plugin states have *.carxs extension).
 * \see carla_load_plugin_state()
 */
CARLA_EXPORT bool carla_save_plugin_state(unsigned int pluginId, const char* filename);

/*!
 * Get a plugin's information.
 */
CARLA_EXPORT const CarlaPluginInfo* carla_get_plugin_info(unsigned int pluginId);

/*!
 * Get a plugin's audio port count information.
 */
CARLA_EXPORT const CarlaPortCountInfo* carla_get_audio_port_count_info(unsigned int pluginId);

/*!
 * Get a plugin's midi port count information.
 */
CARLA_EXPORT const CarlaPortCountInfo* carla_get_midi_port_count_info(unsigned int pluginId);

/*!
 * Get a plugin's parameter count information.
 */
CARLA_EXPORT const CarlaPortCountInfo* carla_get_parameter_count_info(unsigned int pluginId);

/*!
 * * Get a plugin's parameter information.
 */
CARLA_EXPORT const CarlaParameterInfo* carla_get_parameter_info(unsigned int pluginId, uint32_t parameterId);

/*!
 * Get a plugin's parameter scale point information.
 */
CARLA_EXPORT const CarlaScalePointInfo* carla_get_parameter_scalepoint_info(unsigned int pluginId, uint32_t parameterId, uint32_t scalePointId);

/*!
 * Get a plugin's parameter data.
 */
CARLA_EXPORT const CarlaParameterData* carla_get_parameter_data(unsigned int pluginId, uint32_t parameterId);

/*!
 * Get a plugin's parameter ranges.
 */
CARLA_EXPORT const CarlaParameterRanges* carla_get_parameter_ranges(unsigned int pluginId, uint32_t parameterId);

/*!
 * Get a plugin's midi program data.
 */
CARLA_EXPORT const CarlaMidiProgramData* carla_get_midi_program_data(unsigned int pluginId, uint32_t midiProgramId);

/*!
 * Get a plugin's custom data.
 */
CARLA_EXPORT const CarlaCustomData* carla_get_custom_data(unsigned int pluginId, uint32_t customDataId);

/*!
 * Get a plugin's chunk data.
 */
CARLA_EXPORT const char* carla_get_chunk_data(unsigned int pluginId);

/*!
 * Get how many parameters a plugin has.
 */
CARLA_EXPORT uint32_t carla_get_parameter_count(unsigned int pluginId);

/*!
 * Get how many programs a plugin has.
 */
CARLA_EXPORT uint32_t carla_get_program_count(unsigned int pluginId);

/*!
 * Get how many midi programs a plugin has.
 */
CARLA_EXPORT uint32_t carla_get_midi_program_count(unsigned int pluginId);

/*!
 * Get how many custom data sets a plugin has.
 * \see carla_prepare_for_save()
 */
CARLA_EXPORT uint32_t carla_get_custom_data_count(unsigned int pluginId);

/*!
 * Get a plugin's custom parameter text display.
 * \see PARAMETER_USES_CUSTOM_TEXT
 */
CARLA_EXPORT const char* carla_get_parameter_text(unsigned int pluginId, uint32_t parameterId);

/*!
 * Get a plugin's program name.
 */
CARLA_EXPORT const char* carla_get_program_name(unsigned int pluginId, uint32_t programId);

/*!
 * Get a plugin's midi program name.
 */
CARLA_EXPORT const char* carla_get_midi_program_name(unsigned int pluginId, uint32_t midiProgramId);

/*!
 * Get the plugin's real name.\n
 * This is the name the plugin uses to identify itself; may not be unique.
 */
CARLA_EXPORT const char* carla_get_real_plugin_name(unsigned int pluginId);

/*!
 * Get the current plugin's program index.
 */
CARLA_EXPORT int32_t carla_get_current_program_index(unsigned int pluginId);

/*!
 * Get the current plugin's midi program index.
 */
CARLA_EXPORT int32_t carla_get_current_midi_program_index(unsigned int pluginId);

/*!
 * Get a plugin's default parameter value.
 */
CARLA_EXPORT float carla_get_default_parameter_value(unsigned int pluginId, uint32_t parameterId);

/*!
 * Get a plugin's current parameter value.
 */
CARLA_EXPORT float carla_get_current_parameter_value(unsigned int pluginId, uint32_t parameterId);

/*!
 * Get a plugin's input peak value.\n
 * \a portId must only be either 1 or 2
 */
CARLA_EXPORT float carla_get_input_peak_value(unsigned int pluginId, unsigned short portId);

/*!
 * Get a plugin's output peak value.\n
 * \a portId must only be either 1 or 2
 */
CARLA_EXPORT float carla_get_output_peak_value(unsigned int pluginId, unsigned short portId);

/*!
 * Enable a plugin's option.
 * \see PluginOptions
 */
CARLA_EXPORT void carla_set_option(unsigned int pluginId, unsigned int option, bool yesNo);

/*!
 * Enable or disable a plugin according to \a onOff.
 */
CARLA_EXPORT void carla_set_active(unsigned int pluginId, bool onOff);

#ifndef BUILD_BRIDGE
/*!
 * Change a plugin's internal drywet value to \a value.
 */
CARLA_EXPORT void carla_set_drywet(unsigned int pluginId, float value);

/*!
 * Change a plugin's internal volume value to \a value.
 */
CARLA_EXPORT void carla_set_volume(unsigned int pluginId, float value);

/*!
 * Change a plugin's internal balance-left value to \a value.
 */
CARLA_EXPORT void carla_set_balance_left(unsigned int pluginId, float value);

/*!
 * Change a plugin's internal balance-right value to \a value.
 */
CARLA_EXPORT void carla_set_balance_right(unsigned int pluginId, float value);

/*!
 * Change a plugin's internal panning value to \a value.
 */
CARLA_EXPORT void carla_set_panning(unsigned int pluginId, float value);
#endif

/*!
 * Change a plugin's internal control channel to \a channel.
 */
CARLA_EXPORT void carla_set_ctrl_channel(unsigned int pluginId, int8_t channel);

/*!
 * Set the plugin's parameter \a parameterId to \a value.
 */
CARLA_EXPORT void carla_set_parameter_value(unsigned int pluginId, uint32_t parameterId, float value);

#ifndef BUILD_BRIDGE
/*!
 * Set the plugin's parameter \a parameterId midi channel to \a channel.
 */
CARLA_EXPORT void carla_set_parameter_midi_channel(unsigned int pluginId, uint32_t parameterId, uint8_t channel);

/*!
 * Set the plugin's parameter \a parameterId midi cc to \a cc.
 */
CARLA_EXPORT void carla_set_parameter_midi_cc(unsigned int pluginId, uint32_t parameterId, int16_t cc);
#endif

/*!
 * Change a plugin's program to \a programId.
 */
CARLA_EXPORT void carla_set_program(unsigned int pluginId, uint32_t programId);

/*!
 * Change a plugin's midi program to \a midiProgramId.
 */
CARLA_EXPORT void carla_set_midi_program(unsigned int pluginId, uint32_t midiProgramId);

/*!
 * Set a plugin's custom data set.
 */
CARLA_EXPORT void carla_set_custom_data(unsigned int pluginId, const char* type, const char* key, const char* value);

/*!
 * Set a plugin's chunk data.
 */
CARLA_EXPORT void carla_set_chunk_data(unsigned int pluginId, const char* chunkData);

/*!
 * Tell a plugin to prepare for save.\n
 * This should be called before carla_get_custom_data_count().
 */
CARLA_EXPORT void carla_prepare_for_save(unsigned int pluginId);

#ifndef BUILD_BRIDGE
/*!
 * Send a single note of a plugin.\n
 * If \a note if 0, note-off is sent; note-on otherwise.
 */
CARLA_EXPORT void carla_send_midi_note(unsigned int pluginId, uint8_t channel, uint8_t note, uint8_t velocity);
#endif

/*!
 * Tell a plugin to show its own custom UI.
 * \see PLUGIN_HAS_GUI
 */
CARLA_EXPORT void carla_show_gui(unsigned int pluginId, bool yesNo);

/*!
 * Get the current engine buffer size.
 */
CARLA_EXPORT uint32_t carla_get_buffer_size();

/*!
 * Get the current engine sample rate.
 */
CARLA_EXPORT double carla_get_sample_rate();

/*!
 * Get the last error.
 */
CARLA_EXPORT const char* carla_get_last_error();

/*!
 * Get the current engine OSC URL (TCP).
 */
CARLA_EXPORT const char* carla_get_host_osc_url_tcp();

/*!
 * Get the current engine OSC URL (UDP).
 */
CARLA_EXPORT const char* carla_get_host_osc_url_udp();

/*!
 * Send NSM announce message.
 */
CARLA_EXPORT void carla_nsm_announce(const char* url, const char* appName, int pid);

/*!
 * Ready for handling NSM messages.
 */
CARLA_EXPORT void carla_nsm_ready();

/*!
 * Reply to NSM open message.
 * \see CALLBACK_NSM_OPEN
 */
CARLA_EXPORT void carla_nsm_reply_open();

/*!
 * Reply to NSM save message.
 * \see CALLBACK_NSM_SAVE
 */
CARLA_EXPORT void carla_nsm_reply_save();

#ifdef BUILD_BRIDGE
using CarlaBackend::CarlaEngine;
CARLA_EXPORT CarlaEngine* carla_get_standalone_engine();
CARLA_EXPORT bool carla_engine_init_bridge(const char* audioBaseName, const char* controlBaseName, const char* clientName);
#endif

/**@}*/

#endif // __CARLA_STANDALONE_HPP__
