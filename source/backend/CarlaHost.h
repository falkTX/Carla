/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_HOST_H_INCLUDED
#define CARLA_HOST_H_INCLUDED

#include "CarlaBackend.h"

#ifdef __cplusplus
using CarlaBackend::BinaryType;
using CarlaBackend::PluginType;
using CarlaBackend::PluginCategory;
using CarlaBackend::InternalParameterIndex;
using CarlaBackend::EngineCallbackOpcode;
using CarlaBackend::EngineOption;
using CarlaBackend::EngineProcessMode;
using CarlaBackend::EngineTransportMode;
using CarlaBackend::FileCallbackOpcode;
using CarlaBackend::EngineCallbackFunc;
using CarlaBackend::FileCallbackFunc;
using CarlaBackend::ParameterData;
using CarlaBackend::ParameterRanges;
using CarlaBackend::MidiProgramData;
using CarlaBackend::CustomData;
using CarlaBackend::EngineDriverDeviceInfo;
using CarlaBackend::CarlaEngine;
using CarlaBackend::CarlaEngineClient;
using CarlaBackend::CarlaPlugin;
#endif

/*!
 * @defgroup CarlaHostAPI Carla Host API
 *
 * The Carla Host API.
 *
 * This API makes it possible to use the Carla Backend in a standalone host application..
 *
 * None of the returned values in this API calls need to be deleted or free'd.\n
 * When a function fails (returns false or NULL), use carla_get_last_error() to find out what went wrong.
 * @{
 */

/* ------------------------------------------------------------------------------------------------------------
 * Carla Host API (C stuff) */

/*!
 * Information about a loaded plugin.
 * @see carla_get_plugin_info()
 */
typedef struct _CarlaPluginInfo {
    /*!
     * Plugin type.
     */
    PluginType type;

    /*!
     * Plugin category.
     */
    PluginCategory category;

    /*!
     * Plugin hints.
     * @see PluginHints
     */
    uint hints;

    /*!
     * Plugin options available for the user to change.
     * @see PluginOptions
     */
    uint optionsAvailable;

    /*!
     * Plugin options currently enabled.\n
     * Some options are enabled but not available, which means they will always be on.
     * @see PluginOptions
     */
    uint optionsEnabled;

    /*!
     * Plugin filename.\n
     * This can be the plugin binary or resource file.
     */
    const char* filename;

    /*!
     * Plugin name.\n
     * This name is unique within a Carla instance.
     * @see carla_get_real_plugin_name()
     */
    const char* name;

    /*!
     * Plugin label or URI.
     */
    const char* label;

    /*!
     * Plugin author/maker.
     */
    const char* maker;

    /*!
     * Plugin copyright/license.
     */
    const char* copyright;

    /*!
     * Icon name for this plugin, in lowercase.\n
     * Default is "plugin".
     */
    const char* iconName;

    /*!
     * Plugin unique Id.\n
     * This Id is dependant on the plugin type and may sometimes be 0.
     */
    int64_t uniqueId;

#ifdef __cplusplus
    /*!
     * C++ constructor and destructor.
     */
    _CarlaPluginInfo() noexcept;
    ~_CarlaPluginInfo() noexcept;
    CARLA_DECLARE_NON_COPY_STRUCT(_CarlaPluginInfo)
#endif

} CarlaPluginInfo;

/*!
 * Information about an internal Carla plugin.
 * @see carla_get_internal_plugin_info()
 */
typedef struct _CarlaNativePluginInfo {
    /*!
     * Plugin category.
     */
    PluginCategory category;

    /*!
     * Plugin hints.
     * @see PluginHints
     */
    uint hints;

    /*!
     * Number of audio inputs.
     */
    uint32_t audioIns;

    /*!
     * Number of audio outputs.
     */
    uint32_t audioOuts;

    /*!
     * Number of MIDI inputs.
     */
    uint32_t midiIns;

    /*!
     * Number of MIDI outputs.
     */
    uint32_t midiOuts;

    /*!
     * Number of input parameters.
     */
    uint32_t parameterIns;

    /*!
     * Number of output parameters.
     */
    uint32_t parameterOuts;

    /*!
     * Plugin name.
     */
    const char* name;

    /*!
     * Plugin label.
     */
    const char* label;

    /*!
     * Plugin author/maker.
     */
    const char* maker;

    /*!
     * Plugin copyright/license.
     */
    const char* copyright;

#ifdef __cplusplus
    /*!
     * C++ constructor.
     */
    _CarlaNativePluginInfo() noexcept;
    CARLA_DECLARE_NON_COPY_STRUCT(_CarlaNativePluginInfo)
#endif

} CarlaNativePluginInfo;

/*!
 * Port count information, used for Audio and MIDI ports and parameters.
 * @see carla_get_audio_port_count_info()
 * @see carla_get_midi_port_count_info()
 * @see carla_get_parameter_count_info()
 */
typedef struct _CarlaPortCountInfo {
    /*!
     * Number of inputs.
     */
    uint32_t ins;

    /*!
     * Number of outputs.
     */
    uint32_t outs;

} CarlaPortCountInfo;

/*!
 * Parameter information.
 * @see carla_get_parameter_info()
 */
typedef struct _CarlaParameterInfo {
    /*!
     * Parameter name.
     */
    const char* name;

    /*!
     * Parameter symbol.
     */
    const char* symbol;

    /*!
     * Parameter unit.
     */
    const char* unit;

    /*!
     * Number of scale points.
     * @see CarlaScalePointInfo
     */
    uint32_t scalePointCount;

#ifdef __cplusplus
    /*!
     * C++ constructor and destructor.
     */
    _CarlaParameterInfo() noexcept;
    ~_CarlaParameterInfo() noexcept;
    CARLA_DECLARE_NON_COPY_STRUCT(_CarlaParameterInfo)
#endif

} CarlaParameterInfo;

/*!
 * Parameter scale point information.
 * @see carla_get_parameter_scalepoint_info()
 */
typedef struct _CarlaScalePointInfo {
    /*!
     * Scale point value.
     */
    float value;

    /*!
     * Scale point label.
     */
    const char* label;

#ifdef __cplusplus
    /*!
     * C++ constructor and destructor.
     */
    _CarlaScalePointInfo() noexcept;
    ~_CarlaScalePointInfo() noexcept;
    CARLA_DECLARE_NON_COPY_STRUCT(_CarlaScalePointInfo)
#endif

} CarlaScalePointInfo;

/*!
 * Transport information.
 * @see carla_get_transport_info()
 */
typedef struct _CarlaTransportInfo {
    /*!
     * Wherever transport is playing.
     */
    bool playing;

    /*!
     * Current transport frame.
     */
    uint64_t frame;

    /*!
     * Bar.
     */
    int32_t bar;

    /*!
     * Beat.
     */
    int32_t beat;

    /*!
     * Tick.
     */
    int32_t tick;

    /*!
     * Beats per minute.
     */
    double bpm;

#ifdef __cplusplus
    /*!
     * C++ constructor.
     */
    _CarlaTransportInfo() noexcept;
#endif

} CarlaTransportInfo;

/* ------------------------------------------------------------------------------------------------------------
 * Carla Host API (C functions) */

/*!
 * Get the complete license text of used third-party code and features.\n
 * Returned string is in basic html format.
 */
CARLA_EXPORT const char* carla_get_complete_license_text();

/*!
 * Get all the supported file extensions in carla_load_file().\n
 * Returned string uses this syntax:
 * @code
 * "*.ext1;*.ext2;*.ext3"
 * @endcode
 */
CARLA_EXPORT const char* carla_get_supported_file_extensions();

/*!
 * Get how many engine drivers are available.
 */
CARLA_EXPORT uint carla_get_engine_driver_count();

/*!
 * Get an engine driver name.
 * @param index Driver index
 */
CARLA_EXPORT const char* carla_get_engine_driver_name(uint index);

/*!
 * Get the device names of an engine driver.
 * @param index Driver index
 */
CARLA_EXPORT const char* const* carla_get_engine_driver_device_names(uint index);

/*!
 * Get information about a device driver.
 * @param index Driver index
 * @param name  Device name
 */
CARLA_EXPORT const EngineDriverDeviceInfo* carla_get_engine_driver_device_info(uint index, const char* name);

#ifndef BUILD_BRIDGE
/*!
 * Get how many internal plugins are available.
 */
CARLA_EXPORT uint carla_get_internal_plugin_count();

/*!
 * Get information about an internal plugin.
 * @param index Internal plugin Id
 */
CARLA_EXPORT const CarlaNativePluginInfo* carla_get_internal_plugin_info(uint index);
#endif

#ifdef __cplusplus
/*!
 * Get the currently used engine, maybe be NULL.
 * @note C++ only
 */
CARLA_EXPORT const CarlaEngine* carla_get_engine();
#endif

/*!
 * Initialize the engine.\n
 * Make sure to call carla_engine_idle() at regular intervals afterwards.
 * @param driverName Driver to use
 * @param clientName Engine master client name
 */
CARLA_EXPORT bool carla_engine_init(const char* driverName, const char* clientName);

#ifdef BUILD_BRIDGE
/*!
 * Initialize the engine in bridged mode.
 * @param audioBaseName   Shared memory key for audio pool
 * @param controlBaseName Shared memory key for control messages
 * @param timeBaseName    Shared memory key for time info
 * @param clientName      Engine master client name
 */
CARLA_EXPORT bool carla_engine_init_bridge(const char audioBaseName[6+1], const char rtBaseName[6+1], const char nonRtBaseName[6+1], const char* clientName);
#endif

/*!
 * Close the engine.\n
 * This function always closes the engine even if it returns false.\n
 * In other words, even when something goes wrong when closing the engine it still be closed nonetheless.
 */
CARLA_EXPORT bool carla_engine_close();

/*!
 * Idle the engine.\n
 * Do not call this if the engine is not running.
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
 * Set the engine callback function.
 * @param func Callback function
 * @param ptr  Callback pointer
 */
CARLA_EXPORT void carla_set_engine_callback(EngineCallbackFunc func, void* ptr);

#ifndef BUILD_BRIDGE
/*!
 * Set an engine option.
 * @param option   Option
 * @param value    Value as number
 * @param valueStr Value as string
 */
CARLA_EXPORT void carla_set_engine_option(EngineOption option, int value, const char* valueStr);
#endif

/*!
 * Set the file callback function.
 * @param func Callback function
 * @param ptr  Callback pointer
 */
CARLA_EXPORT void carla_set_file_callback(FileCallbackFunc func, void* ptr);

/*!
 * Load a file of any type.\n
 * This will try to load a generic file as a plugin,
 * either by direct handling (GIG, SF2 and SFZ) or by using an internal plugin (like Audio and MIDI).
 * @param Filename Filename
 * @see carla_get_supported_file_extensions()
 */
CARLA_EXPORT bool carla_load_file(const char* filename);

/*!
 * Load a Carla project file.
 * @param Filename Filename
 * @note Currently loaded plugins are not removed; call carla_remove_all_plugins() first if needed.
 */
CARLA_EXPORT bool carla_load_project(const char* filename);

/*!
 * Save current project to a file.
 * @param Filename Filename
 */
CARLA_EXPORT bool carla_save_project(const char* filename);

#ifndef BUILD_BRIDGE
/*!
 * Connect two patchbay ports.
 * @param groupIdA Output group
 * @param portIdA  Output port
 * @param groupIdB Input group
 * @param portIdB  Input port
 * @see ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED
 */
CARLA_EXPORT bool carla_patchbay_connect(uint groupIdA, uint portIdA, uint groupIdB, uint portIdB);

/*!
 * Disconnect two patchbay ports.
 * @param connectionId Connection Id
 * @see ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED
 */
CARLA_EXPORT bool carla_patchbay_disconnect(uint connectionId);

/*!
 * Force the engine to resend all patchbay clients, ports and connections again.
 */
CARLA_EXPORT bool carla_patchbay_refresh();

/*!
 * Start playback of the engine transport.
 */
CARLA_EXPORT void carla_transport_play();

/*!
 * Pause the engine transport.
 */
CARLA_EXPORT void carla_transport_pause();

/*!
 * Relocate the engine transport to a specific frame.
 * @param frames Frame to relocate to.
 */
CARLA_EXPORT void carla_transport_relocate(uint64_t frame);

/*!
 * Get the current transport frame.
 */
CARLA_EXPORT uint64_t carla_get_current_transport_frame();

/*!
 * Get the engine transport information.
 */
CARLA_EXPORT const CarlaTransportInfo* carla_get_transport_info();
#endif

/*!
 * Add a new plugin.\n
 * If you don't know the binary type use the BINARY_NATIVE macro.
 * @param btype    Binary type
 * @param ptype    Plugin type
 * @param filename Filename, if applicable
 * @param name     Name of the plugin, can be NULL
 * @param label    Plugin label, if applicable
 * @param uniqueId Plugin unique Id, if applicable
 * @param extraPtr Extra pointer, defined per plugin type
 */
CARLA_EXPORT bool carla_add_plugin(BinaryType btype, PluginType ptype, const char* filename, const char* name, const char* label, int64_t uniqueId, const void* extraPtr);

/*!
 * Remove one plugin.
 * @param pluginId Plugin to remove.
 */
CARLA_EXPORT bool carla_remove_plugin(uint pluginId);

/*!
 * Remove all plugins.
 */
CARLA_EXPORT bool carla_remove_all_plugins();

#ifndef BUILD_BRIDGE
/*!
 * Rename a plugin.\n
 * Returns the new name, or NULL if the operation failed.
 * @param pluginId Plugin to rename
 * @param newName  New plugin name
 */
CARLA_EXPORT const char* carla_rename_plugin(uint pluginId, const char* newName);

/*!
 * Clone a plugin.
 * @param pluginId Plugin to clone
 */
CARLA_EXPORT bool carla_clone_plugin(uint pluginId);

/*!
 * Prepare replace of a plugin.\n
 * The next call to carla_add_plugin() will use this id, replacing the current plugin.
 * @param pluginId Plugin to replace
 * @note This function requires carla_add_plugin() to be called afterwards *as soon as possible*.
 */
CARLA_EXPORT bool carla_replace_plugin(uint pluginId);

/*!
 * Switch two plugins positions.
 * @param pluginIdA Plugin A
 * @param pluginIdB Plugin B
 */
CARLA_EXPORT bool carla_switch_plugins(uint pluginIdA, uint pluginIdB);
#endif

/*!
 * Load a plugin state.
 * @param pluginId Plugin
 * @param filename Path to plugin state
 * @see carla_save_plugin_state()
 */
CARLA_EXPORT bool carla_load_plugin_state(uint pluginId, const char* filename);

/*!
 * Save a plugin state.
 * @param pluginId Plugin
 * @param filename Path to plugin state
 * @see carla_load_plugin_state()
 */
CARLA_EXPORT bool carla_save_plugin_state(uint pluginId, const char* filename);

/*!
 * Get information from a plugin.
 * @param pluginId Plugin
 */
CARLA_EXPORT const CarlaPluginInfo* carla_get_plugin_info(uint pluginId);

/*!
 * Get audio port count information from a plugin.
 * @param pluginId Plugin
 */
CARLA_EXPORT const CarlaPortCountInfo* carla_get_audio_port_count_info(uint pluginId);

/*!
 * Get MIDI port count information from a plugin.
 * @param pluginId Plugin
 */
CARLA_EXPORT const CarlaPortCountInfo* carla_get_midi_port_count_info(uint pluginId);

/*!
 * Get parameter count information from a plugin.
 * @param pluginId Plugin
 */
CARLA_EXPORT const CarlaPortCountInfo* carla_get_parameter_count_info(uint pluginId);

/*!
 * Get parameter information from a plugin.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @see carla_get_parameter_count()
 */
CARLA_EXPORT const CarlaParameterInfo* carla_get_parameter_info(uint pluginId, uint32_t parameterId);

/*!
 * Get parameter scale point information from a plugin.
 * @param pluginId     Plugin
 * @param parameterId  Parameter index
 * @param scalePointId Parameter scale-point index
 * @see CarlaParameterInfo::scalePointCount
 */
CARLA_EXPORT const CarlaScalePointInfo* carla_get_parameter_scalepoint_info(uint pluginId, uint32_t parameterId, uint32_t scalePointId);

/*!
 * Get a plugin's parameter data.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @see carla_get_parameter_count()
 */
CARLA_EXPORT const ParameterData* carla_get_parameter_data(uint pluginId, uint32_t parameterId);

/*!
 * Get a plugin's parameter ranges.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @see carla_get_parameter_count()
 */
CARLA_EXPORT const ParameterRanges* carla_get_parameter_ranges(uint pluginId, uint32_t parameterId);

/*!
 * Get a plugin's MIDI program data.
 * @param pluginId      Plugin
 * @param midiProgramId MIDI Program index
 * @see carla_get_midi_program_count()
 */
CARLA_EXPORT const MidiProgramData* carla_get_midi_program_data(uint pluginId, uint32_t midiProgramId);

/*!
 * Get a plugin's custom data.
 * @param pluginId     Plugin
 * @param customDataId Custom data index
 * @see carla_get_custom_data_count()
 */
CARLA_EXPORT const CustomData* carla_get_custom_data(uint pluginId, uint32_t customDataId);

/*!
 * Get a plugin's chunk data.
 * @param pluginId Plugin
 * @see PLUGIN_OPTION_USE_CHUNKS and carla_set_chunk_data()
 */
CARLA_EXPORT const char* carla_get_chunk_data(uint pluginId);

/*!
 * Get how many parameters a plugin has.
 * @param pluginId Plugin
 */
CARLA_EXPORT uint32_t carla_get_parameter_count(uint pluginId);

/*!
 * Get how many programs a plugin has.
 * @param pluginId Plugin
 * @see carla_get_program_name()
 */
CARLA_EXPORT uint32_t carla_get_program_count(uint pluginId);

/*!
 * Get how many MIDI programs a plugin has.
 * @param pluginId Plugin
 * @see carla_get_midi_program_name() and carla_get_midi_program_data()
 */
CARLA_EXPORT uint32_t carla_get_midi_program_count(uint pluginId);

/*!
 * Get how many custom data sets a plugin has.
 * @param pluginId Plugin
 * @see carla_get_custom_data()
 */
CARLA_EXPORT uint32_t carla_get_custom_data_count(uint pluginId);

/*!
 * Get a plugin's parameter text (custom display of internal values).
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @see PARAMETER_USES_CUSTOM_TEXT
 */
CARLA_EXPORT const char* carla_get_parameter_text(uint pluginId, uint32_t parameterId);

/*!
 * Get a plugin's program name.
 * @param pluginId  Plugin
 * @param programId Program index
 * @see carla_get_program_count()
 */
CARLA_EXPORT const char* carla_get_program_name(uint pluginId, uint32_t programId);

/*!
 * Get a plugin's MIDI program name.
 * @param pluginId      Plugin
 * @param midiProgramId MIDI Program index
 * @see carla_get_midi_program_count()
 */
CARLA_EXPORT const char* carla_get_midi_program_name(uint pluginId, uint32_t midiProgramId);

/*!
 * Get a plugin's real name.\n
 * This is the name the plugin uses to identify itself; may not be unique.
 * @param pluginId Plugin
 */
CARLA_EXPORT const char* carla_get_real_plugin_name(uint pluginId);

/*!
 * Get a plugin's program index.
 * @param pluginId Plugin
 */
CARLA_EXPORT int32_t carla_get_current_program_index(uint pluginId);

/*!
 * Get a plugin's midi program index.
 * @param pluginId Plugin
 */
CARLA_EXPORT int32_t carla_get_current_midi_program_index(uint pluginId);

/*!
 * Get a plugin's default parameter value.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 */
CARLA_EXPORT float carla_get_default_parameter_value(uint pluginId, uint32_t parameterId);

/*!
 * Get a plugin's current parameter value.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 */
CARLA_EXPORT float carla_get_current_parameter_value(uint pluginId, uint32_t parameterId);

/*!
 * Get a plugin's internal parameter value.
 * @param pluginId    Plugin
 * @param parameterId Parameter index, maybe be negative
 * @see InternalParameterIndex
 */
CARLA_EXPORT float carla_get_internal_parameter_value(uint pluginId, int32_t parameterId);

/*!
 * Get a plugin's input peak value.
 * @param pluginId Plugin
 * @param isLeft   Wherever to get the left/mono value, otherwise right.
 */
CARLA_EXPORT float carla_get_input_peak_value(uint pluginId, bool isLeft);

/*!
 * Get a plugin's output peak value.
 * @param pluginId Plugin
 * @param isLeft   Wherever to get the left/mono value, otherwise right.
 */
CARLA_EXPORT float carla_get_output_peak_value(uint pluginId, bool isLeft);

/*!
 * Enable or disable a plugin.
 * @param pluginId Plugin
 * @param onOff    New active state
 */
CARLA_EXPORT void carla_set_active(uint pluginId, bool onOff);

#ifndef BUILD_BRIDGE
/*!
 * Change a plugin's internal dry/wet.
 * @param pluginId Plugin
 * @param value    New dry/wet value
 */
CARLA_EXPORT void carla_set_drywet(uint pluginId, float value);

/*!
 * Change a plugin's internal volume.
 * @param pluginId Plugin
 * @param value    New volume
 */
CARLA_EXPORT void carla_set_volume(uint pluginId, float value);

/*!
 * Change a plugin's internal stereo balance, left channel.
 * @param pluginId Plugin
 * @param value    New value
 */
CARLA_EXPORT void carla_set_balance_left(uint pluginId, float value);

/*!
 * Change a plugin's internal stereo balance, right channel.
 * @param pluginId Plugin
 * @param value    New value
 */
CARLA_EXPORT void carla_set_balance_right(uint pluginId, float value);

/*!
 * Change a plugin's internal mono panning value.
 * @param pluginId Plugin
 * @param value    New value
 */
CARLA_EXPORT void carla_set_panning(uint pluginId, float value);

/*!
 * Change a plugin's internal control channel.
 * @param pluginId Plugin
 * @param channel  New channel
 */
CARLA_EXPORT void carla_set_ctrl_channel(uint pluginId, int8_t channel);

/*!
 * Enable a plugin's option.
 * @param pluginId Plugin
 * @param option   An option from PluginOptions
 * @param yesNo    New enabled state
 */
CARLA_EXPORT void carla_set_option(uint pluginId, uint option, bool yesNo);
#endif

/*!
 * Change a plugin's parameter value.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @param value       New value
 */
CARLA_EXPORT void carla_set_parameter_value(uint pluginId, uint32_t parameterId, float value);

#ifndef BUILD_BRIDGE
/*!
 * Change a plugin's parameter MIDI channel.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @param channel     New MIDI channel
 */
CARLA_EXPORT void carla_set_parameter_midi_channel(uint pluginId, uint32_t parameterId, uint8_t channel);

/*!
 * Change a plugin's parameter MIDI cc.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @param cc          New MIDI cc
 */
CARLA_EXPORT void carla_set_parameter_midi_cc(uint pluginId, uint32_t parameterId, int16_t cc);
#endif

/*!
 * Change a plugin's current program.
 * @param pluginId  Plugin
 * @param programId New program
 */
CARLA_EXPORT void carla_set_program(uint pluginId, uint32_t programId);

/*!
 * Change a plugin's current MIDI program.
 * @param pluginId      Plugin
 * @param midiProgramId New value
 */
CARLA_EXPORT void carla_set_midi_program(uint pluginId, uint32_t midiProgramId);

/*!
 * Set a plugin's custom data set.
 * @param pluginId Plugin
 * @param type     Type
 * @param key      Key
 * @param value    New value
 * @see CustomDataTypes and CustomDataKeys
 */
CARLA_EXPORT void carla_set_custom_data(uint pluginId, const char* type, const char* key, const char* value);

/*!
 * Set a plugin's chunk data.
 * @param pluginId Plugin
 * @param value    New value
 * @see PLUGIN_OPTION_USE_CHUNKS and carla_get_chunk_data()
 */
CARLA_EXPORT void carla_set_chunk_data(uint pluginId, const char* chunkData);

/*!
 * Tell a plugin to prepare for save.\n
 * This should be called before saving custom data sets.
 * @param pluginId Plugin
 */
CARLA_EXPORT void carla_prepare_for_save(uint pluginId);

/*!
 * Reset all plugin's parameters.
 * @param pluginId Plugin
 */
CARLA_EXPORT void carla_reset_parameters(uint pluginId);

/*!
 * Randomize all plugin's parameters.
 * @param pluginId Plugin
 */
CARLA_EXPORT void carla_randomize_parameters(uint pluginId);

#ifndef BUILD_BRIDGE
/*!
 * Send a single note of a plugin.\n
 * If velocity is 0, note-off is sent; note-on otherwise.
 * @param pluginId Plugin
 * @param channel  Note channel
 * @param note     Note pitch
 * @param velocity Note velocity
 */
CARLA_EXPORT void carla_send_midi_note(uint pluginId, uint8_t channel, uint8_t note, uint8_t velocity);
#endif

/*!
 * Tell a plugin to show its own custom UI.
 * @param pluginId Plugin
 * @param yesNo    New UI state, visible or not
 * @see PLUGIN_HAS_CUSTOM_UI
 */
CARLA_EXPORT void carla_show_custom_ui(uint pluginId, bool yesNo);

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

/** @} */

#endif /* CARLA_HOST_H_INCLUDED */
