/*
 * Carla Plugin Host
 * Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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
using CARLA_BACKEND_NAMESPACE::BinaryType;
using CARLA_BACKEND_NAMESPACE::PluginType;
using CARLA_BACKEND_NAMESPACE::PluginCategory;
using CARLA_BACKEND_NAMESPACE::InternalParameterIndex;
using CARLA_BACKEND_NAMESPACE::EngineCallbackOpcode;
using CARLA_BACKEND_NAMESPACE::NsmCallbackOpcode;
using CARLA_BACKEND_NAMESPACE::EngineOption;
using CARLA_BACKEND_NAMESPACE::EngineProcessMode;
using CARLA_BACKEND_NAMESPACE::EngineTransportMode;
using CARLA_BACKEND_NAMESPACE::FileCallbackOpcode;
using CARLA_BACKEND_NAMESPACE::EngineCallbackFunc;
using CARLA_BACKEND_NAMESPACE::FileCallbackFunc;
using CARLA_BACKEND_NAMESPACE::ParameterData;
using CARLA_BACKEND_NAMESPACE::ParameterRanges;
using CARLA_BACKEND_NAMESPACE::MidiProgramData;
using CARLA_BACKEND_NAMESPACE::CustomData;
using CARLA_BACKEND_NAMESPACE::EngineDriverDeviceInfo;
using CARLA_BACKEND_NAMESPACE::CarlaEngine;
using CARLA_BACKEND_NAMESPACE::CarlaEngineClient;
using CARLA_BACKEND_NAMESPACE::CarlaPlugin;
#endif

/*!
 * @defgroup CarlaHostAPI Carla Host API
 *
 * The Carla Host API.
 *
 * This API makes it possible to use the Carla Backend in a standalone host application..
 *
 * None of the returned values in this API calls need to be deleted or free'd.
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
     * Plugin options currently enabled.
     * Some options are enabled but not available, which means they will always be on.
     * @see PluginOptions
     */
    uint optionsEnabled;

    /*!
     * Plugin filename.
     * This can be the plugin binary or resource file.
     */
    const char* filename;

    /*!
     * Plugin name.
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
     * Icon name for this plugin, in lowercase.
     * Default is "plugin".
     */
    const char* iconName;

    /*!
     * Plugin unique Id.
     * This Id is dependent on the plugin type and may sometimes be 0.
     */
    int64_t uniqueId;

#ifdef __cplusplus
    /*!
     * C++ constructor and destructor.
     */
    CARLA_API _CarlaPluginInfo() noexcept;
    CARLA_API ~_CarlaPluginInfo() noexcept;
    CARLA_DECLARE_NON_COPYABLE(_CarlaPluginInfo)
#endif

} CarlaPluginInfo;

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
     * Parameter comment / documentation.
     */
    const char* comment;

    /*!
     * Parameter group name, prefixed by a unique symbol and ":".
     */
    const char* groupName;

    /*!
     * Number of scale points.
     * @see CarlaScalePointInfo
     */
    uint32_t scalePointCount;

#ifdef __cplusplus
    /*!
     * C++ constructor and destructor.
     */
    CARLA_API _CarlaParameterInfo() noexcept;
    CARLA_API ~_CarlaParameterInfo() noexcept;
    CARLA_DECLARE_NON_COPYABLE(_CarlaParameterInfo)
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
    CARLA_API _CarlaScalePointInfo() noexcept;
    CARLA_API ~_CarlaScalePointInfo() noexcept;
    CARLA_DECLARE_NON_COPYABLE(_CarlaScalePointInfo)
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
    CARLA_API _CarlaTransportInfo() noexcept;

    /*!
     * Clear struct contents.
     */
    CARLA_API void clear() noexcept;
#endif

} CarlaTransportInfo;

/*!
 * Runtime engine information.
 */
typedef struct _CarlaRuntimeEngineInfo {
    /*!
     * DSP load.
     */
    float load;

    /*!
     * Number of xruns.
     */
    uint32_t xruns;

} CarlaRuntimeEngineInfo;

/*!
 * Runtime engine driver device information.
 */
typedef struct {
    /*!
     * Name of the driver device.
     */
    const char* name;

    /*!
     * This driver device hints.
     * @see EngineDriverHints
     */
    uint hints;

    /*!
     * Current buffer size.
     */
    uint bufferSize;

    /*!
     * Available buffer sizes.
     * Terminated with 0.
     */
    const uint32_t* bufferSizes;

    /*!
     * Current sample rate.
     */
    double sampleRate;

    /*!
     * Available sample rates.
     * Terminated with 0.0.
     */
    const double* sampleRates;

} CarlaRuntimeEngineDriverDeviceInfo;

/*!
 * Image data for LV2 inline display API.
 * raw image pixmap format is ARGB32,
 */
typedef struct {
    unsigned char* data;
    int width;
    int height;
    int stride;
} CarlaInlineDisplayImageSurface;

/*! Opaque data type for CarlaHost API calls */
typedef struct _CarlaHostHandle* CarlaHostHandle;

/* ------------------------------------------------------------------------------------------------------------
 * Carla Host API (C functions) */

/*!
 * Get how many engine drivers are available.
 */
CARLA_API_EXPORT uint carla_get_engine_driver_count(void);

/*!
 * Get an engine driver name.
 * @param index Driver index
 */
CARLA_API_EXPORT const char* carla_get_engine_driver_name(uint index);

/*!
 * Get the device names of an engine driver.
 * @param index Driver index
 */
CARLA_API_EXPORT const char* const* carla_get_engine_driver_device_names(uint index);

/*!
 * Get information about a device driver.
 * @param index Driver index
 * @param name  Device name
 */
CARLA_API_EXPORT const EngineDriverDeviceInfo* carla_get_engine_driver_device_info(uint index, const char* name);

/*!
 * Show a device custom control panel.
 * @see ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL
 * @param index Driver index
 * @param name  Device name
 */
CARLA_API_EXPORT bool carla_show_engine_driver_device_control_panel(uint index, const char* name);

/*!
 * Create a global host handle for standalone application usage.
 */
CARLA_API_EXPORT CarlaHostHandle carla_standalone_host_init(void);

#ifdef __cplusplus
/*!
 * Get the currently used engine, may be NULL.
 * @note C++ only
 */
CARLA_API_EXPORT CarlaEngine* carla_get_engine_from_handle(CarlaHostHandle handle);
#endif

/*!
 * Initialize the engine.
 * Make sure to call carla_engine_idle() at regular intervals afterwards.
 * @param driverName Driver to use
 * @param clientName Engine master client name
 */
CARLA_API_EXPORT bool carla_engine_init(CarlaHostHandle handle, const char* driverName, const char* clientName);

#ifdef BUILD_BRIDGE
/*!
 * Initialize the engine in bridged mode.
 */
CARLA_API_EXPORT bool carla_engine_init_bridge(CarlaHostHandle handle,
                                               const char audioBaseName[6+1],
                                               const char rtClientBaseName[6+1],
                                               const char nonRtClientBaseName[6+1],
                                               const char nonRtServerBaseName[6+1],
                                               const char* clientName);
#endif

/*!
 * Close the engine.
 * This function always closes the engine even if it returns false.
 * In other words, even when something goes wrong when closing the engine it still be closed nonetheless.
 */
CARLA_API_EXPORT bool carla_engine_close(CarlaHostHandle handle);

/*!
 * Idle the engine.
 * Do not call this if the engine is not running.
 */
CARLA_API_EXPORT void carla_engine_idle(CarlaHostHandle handle);

/*!
 * Check if the engine is running.
 */
CARLA_API_EXPORT bool carla_is_engine_running(CarlaHostHandle handle);

/*!
 * Get information about the currently running engine.
 */
CARLA_API_EXPORT const CarlaRuntimeEngineInfo* carla_get_runtime_engine_info(CarlaHostHandle handle);

#ifndef BUILD_BRIDGE
/*!
 * Get information about the currently running engine driver device.
 */
CARLA_API_EXPORT const CarlaRuntimeEngineDriverDeviceInfo* carla_get_runtime_engine_driver_device_info(CarlaHostHandle handle);

/*!
 * Dynamically change buffer size and/or sample rate while engine is running.
 * @see ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE
 * @see ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE
 */
CARLA_API_EXPORT bool carla_set_engine_buffer_size_and_sample_rate(CarlaHostHandle handle,
                                                                   uint bufferSize, double sampleRate);

/*!
 * Show the custom control panel for the current engine device.
 * @see ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL
 */
CARLA_API_EXPORT bool carla_show_engine_device_control_panel(CarlaHostHandle handle);
#endif

/*!
 * Clear the xrun count on the engine, so that the next time carla_get_runtime_engine_info() is called, it returns 0.
 */
CARLA_API_EXPORT void carla_clear_engine_xruns(CarlaHostHandle handle);

/*!
 * Tell the engine to stop the current cancelable action.
 * @see ENGINE_CALLBACK_CANCELABLE_ACTION
 */
CARLA_API_EXPORT void carla_cancel_engine_action(CarlaHostHandle handle);

/*!
 * Tell the engine it's about to close.
 * This is used to prevent the engine thread(s) from reactivating.
 * Returns true if there's no pending engine events.
 */
CARLA_API_EXPORT bool carla_set_engine_about_to_close(CarlaHostHandle handle);

/*!
 * Set the engine callback function.
 * @param func Callback function
 * @param ptr  Callback pointer
 */
CARLA_API_EXPORT void carla_set_engine_callback(CarlaHostHandle handle, EngineCallbackFunc func, void* ptr);

/*!
 * Set an engine option.
 * @param option   Option
 * @param value    Value as number
 * @param valueStr Value as string
 */
CARLA_API_EXPORT void carla_set_engine_option(CarlaHostHandle handle,
                                              EngineOption option, int value, const char* valueStr);

/*!
 * Set the file callback function.
 * @param func Callback function
 * @param ptr  Callback pointer
 */
CARLA_API_EXPORT void carla_set_file_callback(CarlaHostHandle handle, FileCallbackFunc func, void* ptr);

/*!
 * Load a file of any type.
 * This will try to load a generic file as a plugin,
 * either by direct handling (SF2 and SFZ) or by using an internal plugin (like Audio and MIDI).
 * @see carla_get_supported_file_extensions()
 */
CARLA_API_EXPORT bool carla_load_file(CarlaHostHandle handle, const char* filename);

/*!
 * Load a Carla project file.
 * @note Currently loaded plugins are not removed; call carla_remove_all_plugins() first if needed.
 */
CARLA_API_EXPORT bool carla_load_project(CarlaHostHandle handle, const char* filename);

/*!
 * Save current project to a file.
 */
CARLA_API_EXPORT bool carla_save_project(CarlaHostHandle handle, const char* filename);

#ifndef BUILD_BRIDGE
/*!
  * Get the currently set project folder.
  * @note Valid for both standalone and plugin versions.
 */
CARLA_API_EXPORT const char* carla_get_current_project_folder(CarlaHostHandle handle);

/*!
 * Get the currently set project filename.
 * @note Valid only for standalone version.
 */
CARLA_API_EXPORT const char* carla_get_current_project_filename(CarlaHostHandle handle);

/*!
 * Clear the currently set project filename.
 */
CARLA_API_EXPORT void carla_clear_project_filename(CarlaHostHandle handle);

/*!
 * Connect two patchbay ports.
 * @param groupIdA Output (source) group
 * @param portIdA  Output (source) port
 * @param groupIdB Input (target) group
 * @param portIdB  Input (target) port
 * @note The group corresponds to the client Id received in the engine callback
 * for ENGINE_CALLBACK_PATCHBAY_PORT_ADDED.
 * @see ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED
 */
CARLA_API_EXPORT bool carla_patchbay_connect(CarlaHostHandle handle,
                                             bool external, uint groupIdA, uint portIdA, uint groupIdB, uint portIdB);

/*!
 * Disconnect two patchbay ports.
 * @param connectionId Connection Id
 * @see ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED
 */
CARLA_API_EXPORT bool carla_patchbay_disconnect(CarlaHostHandle handle, bool external, uint connectionId);

/*!
 * Set the position of a group.
 * This is purely cached and saved in the project file, Carla backend does nothing with the value.
 * When loading a project, callbacks are used to inform of the previously saved positions.
 * @see ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED
 */
CARLA_API_EXPORT bool carla_patchbay_set_group_pos(CarlaHostHandle handle,
                                                   bool external, uint groupId, int x1, int y1, int x2, int y2);

/*!
 * Force the engine to resend all patchbay clients, ports and connections again.
 * @param external Wherever to show external/hardware ports instead of internal ones.
 *                 Only valid in patchbay engine mode, other modes will ignore this.
 */
CARLA_API_EXPORT bool carla_patchbay_refresh(CarlaHostHandle handle, bool external);

/*!
 * Start playback of the engine transport.
 */
CARLA_API_EXPORT void carla_transport_play(CarlaHostHandle handle);

/*!
 * Pause the engine transport.
 */
CARLA_API_EXPORT void carla_transport_pause(CarlaHostHandle handle);

/*!
 * Set the engine transport bpm.
 */
CARLA_API_EXPORT void carla_transport_bpm(CarlaHostHandle handle, double bpm);

/*!
 * Relocate the engine transport to a specific frame.
 */
CARLA_API_EXPORT void carla_transport_relocate(CarlaHostHandle handle, uint64_t frame);

/*!
 * Get the current transport frame.
 */
CARLA_API_EXPORT uint64_t carla_get_current_transport_frame(CarlaHostHandle handle);

/*!
 * Get the engine transport information.
 */
CARLA_API_EXPORT const CarlaTransportInfo* carla_get_transport_info(CarlaHostHandle handle);
#endif

/*!
 * Current number of plugins loaded.
 */
CARLA_API_EXPORT uint32_t carla_get_current_plugin_count(CarlaHostHandle handle);

/*!
 * Maximum number of loadable plugins allowed.
 * Returns 0 if engine is not started.
 */
CARLA_API_EXPORT uint32_t carla_get_max_plugin_number(CarlaHostHandle handle);

/*!
 * Add a new plugin.
 * If you don't know the binary type use the BINARY_NATIVE macro.
 * @param btype    Binary type
 * @param ptype    Plugin type
 * @param filename Filename, if applicable
 * @param name     Name of the plugin, can be NULL
 * @param label    Plugin label, if applicable
 * @param uniqueId Plugin unique Id, if applicable
 * @param extraPtr Extra pointer, defined per plugin type
 * @param options  Initial plugin options
 */
CARLA_API_EXPORT bool carla_add_plugin(CarlaHostHandle handle,
                                       BinaryType btype, PluginType ptype,
                                       const char* filename, const char* name, const char* label, int64_t uniqueId,
                                       const void* extraPtr, uint options);

/*!
 * Remove one plugin.
 * @param pluginId Plugin to remove.
 */
CARLA_API_EXPORT bool carla_remove_plugin(CarlaHostHandle handle, uint pluginId);

/*!
 * Remove all plugins.
 */
CARLA_API_EXPORT bool carla_remove_all_plugins(CarlaHostHandle handle);

#ifndef BUILD_BRIDGE
/*!
 * Rename a plugin.
 * Returns the new name, or NULL if the operation failed.
 * @param pluginId Plugin to rename
 * @param newName  New plugin name
 */
CARLA_API_EXPORT bool carla_rename_plugin(CarlaHostHandle handle, uint pluginId, const char* newName);

/*!
 * Clone a plugin.
 * @param pluginId Plugin to clone
 */
CARLA_API_EXPORT bool carla_clone_plugin(CarlaHostHandle handle, uint pluginId);

/*!
 * Prepare replace of a plugin.
 * The next call to carla_add_plugin() will use this id, replacing the current plugin.
 * @param pluginId Plugin to replace
 * @note This function requires carla_add_plugin() to be called afterwards *as soon as possible*.
 */
CARLA_API_EXPORT bool carla_replace_plugin(CarlaHostHandle handle, uint pluginId);

/*!
 * Switch two plugins positions.
 * @param pluginIdA Plugin A
 * @param pluginIdB Plugin B
 */
CARLA_API_EXPORT bool carla_switch_plugins(CarlaHostHandle handle, uint pluginIdA, uint pluginIdB);
#endif

/*!
 * Load a plugin state.
 * @param pluginId Plugin
 * @param filename Path to plugin state
 * @see carla_save_plugin_state()
 */
CARLA_API_EXPORT bool carla_load_plugin_state(CarlaHostHandle handle, uint pluginId, const char* filename);

/*!
 * Save a plugin state.
 * @param pluginId Plugin
 * @param filename Path to plugin state
 * @see carla_load_plugin_state()
 */
CARLA_API_EXPORT bool carla_save_plugin_state(CarlaHostHandle handle, uint pluginId, const char* filename);

/*!
 * Export plugin as LV2.
 * @param pluginId Plugin
 * @param lv2path Path to lv2 plugin folder
 */
CARLA_API_EXPORT bool carla_export_plugin_lv2(CarlaHostHandle handle, uint pluginId, const char* lv2path);

/*!
 * Get information from a plugin.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT const CarlaPluginInfo* carla_get_plugin_info(CarlaHostHandle handle, uint pluginId);

/*!
 * Get audio port count information from a plugin.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT const CarlaPortCountInfo* carla_get_audio_port_count_info(CarlaHostHandle handle, uint pluginId);

/*!
 * Get MIDI port count information from a plugin.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT const CarlaPortCountInfo* carla_get_midi_port_count_info(CarlaHostHandle handle, uint pluginId);

/*!
 * Get parameter count information from a plugin.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT const CarlaPortCountInfo* carla_get_parameter_count_info(CarlaHostHandle handle, uint pluginId);

/*!
 * Get hints about an audio port.
 * @param pluginId  Plugin
 * @param isOutput  Whether port is output, input otherwise
 * @param portIndex Port index, related to input or output
 */
CARLA_API_EXPORT uint carla_get_audio_port_hints(CarlaHostHandle handle, uint pluginId, bool isOutput, uint32_t portIndex);

/*!
 * Get parameter information from a plugin.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @see carla_get_parameter_count()
 */
CARLA_API_EXPORT const CarlaParameterInfo* carla_get_parameter_info(CarlaHostHandle handle,
                                                                    uint pluginId,
                                                                    uint32_t parameterId);

/*!
 * Get parameter scale point information from a plugin.
 * @param pluginId     Plugin
 * @param parameterId  Parameter index
 * @param scalePointId Parameter scale-point index
 * @see CarlaParameterInfo::scalePointCount
 */
CARLA_API_EXPORT const CarlaScalePointInfo* carla_get_parameter_scalepoint_info(CarlaHostHandle handle,
                                                                                uint pluginId,
                                                                                uint32_t parameterId,
                                                                                uint32_t scalePointId);

/*!
 * Get a plugin's parameter data.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @see carla_get_parameter_count()
 */
CARLA_API_EXPORT const ParameterData* carla_get_parameter_data(CarlaHostHandle handle,
                                                               uint pluginId, uint32_t parameterId);

/*!
 * Get a plugin's parameter ranges.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @see carla_get_parameter_count()
 */
CARLA_API_EXPORT const ParameterRanges* carla_get_parameter_ranges(CarlaHostHandle handle,
                                                                   uint pluginId, uint32_t parameterId);

/*!
 * Get a plugin's MIDI program data.
 * @param pluginId      Plugin
 * @param midiProgramId MIDI Program index
 * @see carla_get_midi_program_count()
 */
CARLA_API_EXPORT const MidiProgramData* carla_get_midi_program_data(CarlaHostHandle handle,
                                                                    uint pluginId, uint32_t midiProgramId);

/*!
 * Get a plugin's custom data, using index.
 * @param pluginId     Plugin
 * @param customDataId Custom data index
 * @see carla_get_custom_data_count()
 */
CARLA_API_EXPORT const CustomData* carla_get_custom_data(CarlaHostHandle handle, uint pluginId, uint32_t customDataId);

/*!
 * Get a plugin's custom data value, using type and key.
 * @param pluginId Plugin
 * @param type     Custom data type
 * @param key      Custom data key
 * @see carla_get_custom_data_count()
 */
CARLA_API_EXPORT const char* carla_get_custom_data_value(CarlaHostHandle handle,
                                                         uint pluginId, const char* type, const char* key);

/*!
 * Get a plugin's chunk data.
 * @param pluginId Plugin
 * @see PLUGIN_OPTION_USE_CHUNKS and carla_set_chunk_data()
 */
CARLA_API_EXPORT const char* carla_get_chunk_data(CarlaHostHandle handle, uint pluginId);

/*!
 * Get how many parameters a plugin has.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT uint32_t carla_get_parameter_count(CarlaHostHandle handle, uint pluginId);

/*!
 * Get how many programs a plugin has.
 * @param pluginId Plugin
 * @see carla_get_program_name()
 */
CARLA_API_EXPORT uint32_t carla_get_program_count(CarlaHostHandle handle, uint pluginId);

/*!
 * Get how many MIDI programs a plugin has.
 * @param pluginId Plugin
 * @see carla_get_midi_program_name() and carla_get_midi_program_data()
 */
CARLA_API_EXPORT uint32_t carla_get_midi_program_count(CarlaHostHandle handle, uint pluginId);

/*!
 * Get how many custom data sets a plugin has.
 * @param pluginId Plugin
 * @see carla_get_custom_data()
 */
CARLA_API_EXPORT uint32_t carla_get_custom_data_count(CarlaHostHandle handle, uint pluginId);

/*!
 * Get a plugin's parameter text (custom display of internal values).
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @see PARAMETER_USES_CUSTOM_TEXT
 */
CARLA_API_EXPORT const char* carla_get_parameter_text(CarlaHostHandle handle, uint pluginId, uint32_t parameterId);

/*!
 * Get a plugin's program name.
 * @param pluginId  Plugin
 * @param programId Program index
 * @see carla_get_program_count()
 */
CARLA_API_EXPORT const char* carla_get_program_name(CarlaHostHandle handle, uint pluginId, uint32_t programId);

/*!
 * Get a plugin's MIDI program name.
 * @param pluginId      Plugin
 * @param midiProgramId MIDI Program index
 * @see carla_get_midi_program_count()
 */
CARLA_API_EXPORT const char* carla_get_midi_program_name(CarlaHostHandle handle, uint pluginId, uint32_t midiProgramId);

/*!
 * Get a plugin's real name.
 * This is the name the plugin uses to identify itself; may not be unique.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT const char* carla_get_real_plugin_name(CarlaHostHandle handle, uint pluginId);

/*!
 * Get a plugin's program index.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT int32_t carla_get_current_program_index(CarlaHostHandle handle, uint pluginId);

/*!
 * Get a plugin's midi program index.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT int32_t carla_get_current_midi_program_index(CarlaHostHandle handle, uint pluginId);

/*!
 * Get a plugin's default parameter value.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 */
CARLA_API_EXPORT float carla_get_default_parameter_value(CarlaHostHandle handle, uint pluginId, uint32_t parameterId);

/*!
 * Get a plugin's current parameter value.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 */
CARLA_API_EXPORT float carla_get_current_parameter_value(CarlaHostHandle handle, uint pluginId, uint32_t parameterId);

/*!
 * Get a plugin's internal parameter value.
 * @param pluginId    Plugin
 * @param parameterId Parameter index, maybe be negative
 * @see InternalParameterIndex
 */
CARLA_API_EXPORT float carla_get_internal_parameter_value(CarlaHostHandle handle, uint pluginId, int32_t parameterId);

/*!
 * Get a plugin's internal latency, in samples.
 * @param pluginId    Plugin
 * @see InternalParameterIndex
 */
CARLA_API_EXPORT uint32_t carla_get_plugin_latency(CarlaHostHandle handle, uint pluginId);

/*!
 * Get a plugin's peak values.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT const float* carla_get_peak_values(CarlaHostHandle handle, uint pluginId);

/*!
 * Get a plugin's input peak value.
 * @param pluginId Plugin
 * @param isLeft   Wherever to get the left/mono value, otherwise right.
 */
CARLA_API_EXPORT float carla_get_input_peak_value(CarlaHostHandle handle, uint pluginId, bool isLeft);

/*!
 * Get a plugin's output peak value.
 * @param pluginId Plugin
 * @param isLeft   Wherever to get the left/mono value, otherwise right.
 */
CARLA_API_EXPORT float carla_get_output_peak_value(CarlaHostHandle handle, uint pluginId, bool isLeft);

/*!
 * Render a plugin's inline display.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT const CarlaInlineDisplayImageSurface* carla_render_inline_display(CarlaHostHandle handle,
                                                                                   uint pluginId,
                                                                                   uint32_t width,
                                                                                   uint32_t height);

/*!
 * Enable or disable a plugin.
 * @param pluginId Plugin
 * @param onOff    New active state
 */
CARLA_API_EXPORT void carla_set_active(CarlaHostHandle handle, uint pluginId, bool onOff);

#ifndef BUILD_BRIDGE
/*!
 * Change a plugin's internal dry/wet.
 * @param pluginId Plugin
 * @param value    New dry/wet value
 */
CARLA_API_EXPORT void carla_set_drywet(CarlaHostHandle handle, uint pluginId, float value);

/*!
 * Change a plugin's internal volume.
 * @param pluginId Plugin
 * @param value    New volume
 */
CARLA_API_EXPORT void carla_set_volume(CarlaHostHandle handle, uint pluginId, float value);

/*!
 * Change a plugin's internal stereo balance, left channel.
 * @param pluginId Plugin
 * @param value    New value
 */
CARLA_API_EXPORT void carla_set_balance_left(CarlaHostHandle handle, uint pluginId, float value);

/*!
 * Change a plugin's internal stereo balance, right channel.
 * @param pluginId Plugin
 * @param value    New value
 */
CARLA_API_EXPORT void carla_set_balance_right(CarlaHostHandle handle, uint pluginId, float value);

/*!
 * Change a plugin's internal mono panning value.
 * @param pluginId Plugin
 * @param value    New value
 */
CARLA_API_EXPORT void carla_set_panning(CarlaHostHandle handle, uint pluginId, float value);

/*!
 * Change a plugin's internal control channel.
 * @param pluginId Plugin
 * @param channel  New channel
 */
CARLA_API_EXPORT void carla_set_ctrl_channel(CarlaHostHandle handle, uint pluginId, int8_t channel);
#endif

/*!
 * Enable a plugin's option.
 * @param pluginId Plugin
 * @param option   An option from PluginOptions
 * @param yesNo    New enabled state
 */
CARLA_API_EXPORT void carla_set_option(CarlaHostHandle handle, uint pluginId, uint option, bool yesNo);

/*!
 * Change a plugin's parameter value.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @param value       New value
 */
CARLA_API_EXPORT void carla_set_parameter_value(CarlaHostHandle handle, uint pluginId, uint32_t parameterId, float value);

#ifndef BUILD_BRIDGE
/*!
 * Change a plugin's parameter MIDI channel.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @param channel     New MIDI channel
 */
CARLA_API_EXPORT void carla_set_parameter_midi_channel(CarlaHostHandle handle,
                                                       uint pluginId, uint32_t parameterId, uint8_t channel);

/*!
 * Change a plugin's parameter mapped control index.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @param cc          New control index
 */
CARLA_API_EXPORT void carla_set_parameter_mapped_control_index(CarlaHostHandle handle,
                                                               uint pluginId, uint32_t parameterId, int16_t index);

/*!
 * Change a plugin's parameter mapped range.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @param minimum     New mapped minimum
 * @param maximum     New mapped maximum
 */
CARLA_API_EXPORT void carla_set_parameter_mapped_range(CarlaHostHandle handle,
                                                       uint pluginId, uint32_t parameterId, float minimum, float maximum);

/*!
 * Change a plugin's parameter in drag/touch mode state.
 * Usually happens from a UI when the user is moving a parameter with a mouse or similar input.
 * @param pluginId    Plugin
 * @param parameterId Parameter index
 * @param touch       New state
 */
CARLA_API_EXPORT void carla_set_parameter_touch(CarlaHostHandle handle,
                                                uint pluginId, uint32_t parameterId, bool touch);
#endif

/*!
 * Change a plugin's current program.
 * @param pluginId  Plugin
 * @param programId New program
 */
CARLA_API_EXPORT void carla_set_program(CarlaHostHandle handle, uint pluginId, uint32_t programId);

/*!
 * Change a plugin's current MIDI program.
 * @param pluginId      Plugin
 * @param midiProgramId New value
 */
CARLA_API_EXPORT void carla_set_midi_program(CarlaHostHandle handle, uint pluginId, uint32_t midiProgramId);

/*!
 * Set a plugin's custom data set.
 * @param pluginId Plugin
 * @param type     Type
 * @param key      Key
 * @param value    New value
 * @see CustomDataTypes and CustomDataKeys
 */
CARLA_API_EXPORT void carla_set_custom_data(CarlaHostHandle handle,
                                            uint pluginId, const char* type, const char* key, const char* value);

/*!
 * Set a plugin's chunk data.
 * @param pluginId  Plugin
 * @param chunkData New chunk data
 * @see PLUGIN_OPTION_USE_CHUNKS and carla_get_chunk_data()
 */
CARLA_API_EXPORT void carla_set_chunk_data(CarlaHostHandle handle, uint pluginId, const char* chunkData);

/*!
 * Tell a plugin to prepare for save.
 * This should be called before saving custom data sets.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT void carla_prepare_for_save(CarlaHostHandle handle, uint pluginId);

/*!
 * Reset all plugin's parameters.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT void carla_reset_parameters(CarlaHostHandle handle, uint pluginId);

/*!
 * Randomize all plugin's parameters.
 * @param pluginId Plugin
 */
CARLA_API_EXPORT void carla_randomize_parameters(CarlaHostHandle handle, uint pluginId);

#ifndef BUILD_BRIDGE
/*!
 * Send a single note of a plugin.
 * If velocity is 0, note-off is sent; note-on otherwise.
 * @param pluginId Plugin
 * @param channel  Note channel
 * @param note     Note pitch
 * @param velocity Note velocity
 */
CARLA_API_EXPORT void carla_send_midi_note(CarlaHostHandle handle,
                                           uint pluginId, uint8_t channel, uint8_t note, uint8_t velocity);
#endif

/*!
 * Set a custom title for the plugin UI window created by Carla.
 */
CARLA_API_EXPORT void carla_set_custom_ui_title(CarlaHostHandle handle, uint pluginId, const char* title);

/*!
 * Tell a plugin to show its own custom UI.
 * @param pluginId Plugin
 * @param yesNo    New UI state, visible or not
 * @see PLUGIN_HAS_CUSTOM_UI
 */
CARLA_API_EXPORT void carla_show_custom_ui(CarlaHostHandle handle, uint pluginId, bool yesNo);

/*!
  * Embed the plugin's custom UI to the system pointer @a ptr.
  * This function is always called from the main thread.
  * @note This is very experimental and subject to change at this point
  */
CARLA_API_EXPORT void* carla_embed_custom_ui(CarlaHostHandle handle, uint pluginId, void* ptr);

/*!
 * Get the current engine buffer size.
 */
CARLA_API_EXPORT uint32_t carla_get_buffer_size(CarlaHostHandle handle);

/*!
 * Get the current engine sample rate.
 */
CARLA_API_EXPORT double carla_get_sample_rate(CarlaHostHandle handle);

/*!
 * Get the last error.
 */
CARLA_API_EXPORT const char* carla_get_last_error(CarlaHostHandle handle);

/*!
 * Get the current engine OSC URL (TCP).
 */
CARLA_API_EXPORT const char* carla_get_host_osc_url_tcp(CarlaHostHandle handle);

/*!
 * Get the current engine OSC URL (UDP).
 */
CARLA_API_EXPORT const char* carla_get_host_osc_url_udp(CarlaHostHandle handle);

/*!
 * Initialize NSM (that is, announce ourselves to it).
 * Must be called as early as possible in the program's lifecycle.
 * Returns true if NSM is available and initialized correctly.
 */
CARLA_API_EXPORT bool carla_nsm_init(CarlaHostHandle handle, uint64_t pid, const char* executableName);

/*!
 * Respond to an NSM callback.
 */
CARLA_API_EXPORT void carla_nsm_ready(CarlaHostHandle handle, NsmCallbackOpcode opcode);

#ifndef CARLA_UTILS_H_INCLUDED
/*!
 * Get the complete license text of used third-party code and features.
 * Returned string is in basic html format.
 */
CARLA_API_EXPORT const char* carla_get_complete_license_text(void);

/*!
 * Get the juce version used in the current Carla build.
 */
CARLA_API_EXPORT const char* carla_get_juce_version(void);

/*!
 * Get the list of supported file extensions in carla_load_file().
 */
CARLA_API_EXPORT const char* const* carla_get_supported_file_extensions(void);

/*!
 * Get the list of supported features in the current Carla build.
 */
CARLA_API_EXPORT const char* const* carla_get_supported_features(void);

/*!
 * Get the absolute filename of this carla library.
 */
CARLA_API_EXPORT const char* carla_get_library_filename(void);

/*!
 * Get the folder where this carla library resides.
 */
CARLA_API_EXPORT const char* carla_get_library_folder(void);
#endif

/** @} */

#endif /* CARLA_HOST_H_INCLUDED */
