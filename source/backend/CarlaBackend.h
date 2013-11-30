/*
 * Carla Backend API
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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_BACKEND_HPP_INCLUDED
#define CARLA_BACKEND_HPP_INCLUDED

#include "CarlaDefines.h"

#ifdef CARLA_PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

#define STR_MAX 0xFF

#ifdef __cplusplus
# define CARLA_BACKEND_START_NAMESPACE namespace CarlaBackend {
# define CARLA_BACKEND_END_NAMESPACE }
# define CARLA_BACKEND_USE_NAMESPACE using namespace CarlaBackend;
/* Start namespace */
CARLA_BACKEND_START_NAMESPACE
#endif

/*!
 * @defgroup CarlaBackendAPI Carla Backend API
 *
 * The Carla Backend API.
 *
 * These are the base definitions for everything in the Carla backend code.
 * @{
 */

/*!
 * Maximum default number of loadable plugins.
 */
const unsigned int MAX_DEFAULT_PLUGINS = 99;

/*!
 * Maximum number of loadable plugins in rack mode.
 */
const unsigned int MAX_RACK_PLUGINS = 16;

/*!
 * Maximum number of loadable plugins in patchbay mode.
 */
const unsigned int MAX_PATCHBAY_PLUGINS = 255;

/*!
 * Maximum default number of parameters allowed.
 * @see ENGINE_OPTION_MAX_PARAMETERS
 */
const unsigned int MAX_DEFAULT_PARAMETERS = 200;

/*!
 * @defgroup EngineDriverHints Engine Driver Device Hints
 *
 * Various engine driver device hints.
 * @see CarlaEngine::getHints(), CarlaEngine::getDriverDeviceInfo() and carla_get_engine_driver_device_info()
 * @{
 */

/*!
 * Engine driver device has custom control-panel.
 */
const unsigned int ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL = 0x1;

/*!
 * Engine driver device can change buffer-size on the fly.
 * @see ENGINE_OPTION_AUDIO_BUFFER_SIZE
 */
const unsigned int ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE = 0x2;

/*!
 * Engine driver device can change sample-rate on the fly.
 * @see ENGINE_OPTION_AUDIO_SAMPLE_RATE
 */
const unsigned int ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE = 0x4;

/** @} */

/*!
 * @defgroup PluginHints Plugin Hints
 *
 * Various plugin hints.
 * @see CarlaPlugin::getHints() and carla_get_plugin_info()
 * @{
 */

/*!
 * Plugin is a bridge.\n
 * This hint is required because "bridge" itself is not a plugin type.
 */
const unsigned int PLUGIN_IS_BRIDGE = 0x01;

/*!
 * Plugin is hard real-time safe.
 */
const unsigned int PLUGIN_IS_RTSAFE = 0x02;

/*!
 * Plugin is a synth (produces sound).
 */
const unsigned int PLUGIN_IS_SYNTH = 0x04;

/*!
 * Plugin has its own custom UI.
 * @see CarlaPlugin::showCustomUI() and carla_show_custom_ui()
 */
const unsigned int PLUGIN_HAS_CUSTOM_UI = 0x08;

/*!
 * Plugin can use internal Dry/Wet control.
 */
const unsigned int PLUGIN_CAN_DRYWET = 0x10;

/*!
 * Plugin can use internal Volume control.
 */
const unsigned int PLUGIN_CAN_VOLUME = 0x20;

/*!
 * Plugin can use internal (Stereo) Balance controls.
 */
const unsigned int PLUGIN_CAN_BALANCE = 0x40;

/*!
 * Plugin can use internal (Mono) Panning control.
 */
const unsigned int PLUGIN_CAN_PANNING = 0x80;

/** @} */

/*!
 * @defgroup PluginOptions Plugin Options
 *
 * Various plugin options.
 * @see CarlaPlugin::getOptionsAvailable(), CarlaPlugin::getOptionsEnabled() and carla_get_plugin_info()
 * @{
 */

/*!
 * Use constant/fixed-size audio buffers.
 */
const unsigned int PLUGIN_OPTION_FIXED_BUFFERS = 0x001;

/*!
 * Force mono plugin as stereo.
 */
const unsigned int PLUGIN_OPTION_FORCE_STEREO = 0x002;

/*!
 * Map MIDI programs to plugin programs.
 */
const unsigned int PLUGIN_OPTION_MAP_PROGRAM_CHANGES = 0x004;

/*!
 * Use chunks to save and restore data.
 */
const unsigned int PLUGIN_OPTION_USE_CHUNKS = 0x008;

/*!
 * Send MIDI control change events.
 */
const unsigned int PLUGIN_OPTION_SEND_CONTROL_CHANGES = 0x010;

/*!
 * Send MIDI channel pressure events.
 */
const unsigned int PLUGIN_OPTION_SEND_CHANNEL_PRESSURE = 0x020;

/*!
 * Send MIDI note after-touch events.
 */
const unsigned int PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH = 0x040;

/*!
 * Send MIDI pitch-bend events.
 */
const unsigned int PLUGIN_OPTION_SEND_PITCHBEND = 0x080;

/*!
 * Send MIDI all-sounds/notes-off events, single note-offs otherwise.
 */
const unsigned int PLUGIN_OPTION_SEND_ALL_SOUND_OFF = 0x100;

/** @} */

/*!
 * @defgroup ParameterHints Parameter Hints
 *
 * Various parameter hints.
 * @see CarlaPlugin::getParameterData() and carla_get_parameter_data()
 * @{
 */

/*!
 * Parameter is input.\n
 * When this hint is not set, parameter is assumed to be output.
 */
const unsigned int PARAMETER_IS_INPUT = 0x001;

/*!
 * Parameter value is boolean.
 * It's always at either minimum or maximum value.
 */
const unsigned int PARAMETER_IS_BOOLEAN = 0x002;

/*!
 * Parameter value is integer.
 */
const unsigned int PARAMETER_IS_INTEGER = 0x004;

/*!
 * Parameter value is logarithmic.
 */
const unsigned int PARAMETER_IS_LOGARITHMIC = 0x008;

/*!
 * Parameter is enabled.
 * It can be viewed, changed and stored.
 */
const unsigned int PARAMETER_IS_ENABLED = 0x010;

/*!
 * Parameter is automable (real-time safe).
 */
const unsigned int PARAMETER_IS_AUTOMABLE = 0x020;

/*!
 * Parameter is read-only.
 * It cannot be changed.
 */
const unsigned int PARAMETER_IS_READ_ONLY = 0x040;

/*!
 * Parameter needs sample rate to work.
 * Value and ranges are multiplied by sample rate on usage and divided by sample rate on save.
 */
const unsigned int PARAMETER_USES_SAMPLERATE = 0x100;

/*!
 * Parameter uses scale points to define internal values in a meaningful way.
 */
const unsigned int PARAMETER_USES_SCALEPOINTS = 0x200;

/*!
 * Parameter uses custom text for displaying its value.
 * @see CarlaPlugin::getParameterText() and carla_get_parameter_text()
 */
const unsigned int PARAMETER_USES_CUSTOM_TEXT = 0x400;

/** @} */

/*!
 * @defgroup PatchbayPortHints Patchbay Port Hints
 *
 * Various patchbay port hints.
 * @{
 */

/*!
 * Patchbay port is input.\n
 * When this hint is not set, port is assumed to be output.
 */
const unsigned int PATCHBAY_PORT_IS_INPUT = 0x1;

/*!
 * Patchbay port is of Audio type.
 */
const unsigned int PATCHBAY_PORT_TYPE_AUDIO = 0x2;

/*!
 * Patchbay port is of CV type (Control Voltage).
 */
const unsigned int PATCHBAY_PORT_TYPE_CV = 0x4;

/*!
 * Patchbay port is of MIDI type.
 */
const unsigned int PATCHBAY_PORT_TYPE_MIDI = 0x8;

/** @} */

/*!
 * @defgroup CustomDataTypes Custom Data Types
 *
 * These types define how the value in the CustomData struct is stored.
 * @see CustomData::type
 * @{
 */

/*!
 * Boolean string type URI.\n
 * Only "true" and "false" are valid values.
 */
const char* const CUSTOM_DATA_TYPE_BOOLEAN = "http://kxstudio.sf.net/ns/carla/boolean";

/*!
 * Chunk type URI.
 */
const char* const CUSTOM_DATA_TYPE_CHUNK = "http://kxstudio.sf.net/ns/carla/chunk";

/*!
 * String type URI.
 */
const char* const CUSTOM_DATA_TYPE_STRING = "http://kxstudio.sf.net/ns/carla/string";

/** @} */

/*!
 * @defgroup CustomDataKeys Custom Data Keys
 *
 * Pre-defined keys used internally in Carla.
 * @see CustomData::key
 * @{
 */

/*!
 * Plugin options key.
 */
const char* const CUSTOM_DATA_KEY_PLUGIN_OPTIONS = "CarlaPluginOptions";

/*!
 * UI position key.
 */
const char* const CUSTOM_DATA_KEY_UI_POSITION = "CarlaUiPosition";

/*!
 * UI size key.
 */
const char* const CUSTOM_DATA_KEY_UI_SIZE = "CarlaUiSize";

/*!
 * UI visible key.
 */
const char* const CUSTOM_DATA_KEY_UI_VISIBLE = "CarlaUiVisible";

/** @} */

/*!
 * The binary type of a plugin.
 */
typedef enum {
    /*!
     * Null binary type.
     */
    BINARY_NONE = 0,

    /*!
     * POSIX 32bit binary.
     */
    BINARY_POSIX32 = 1,

    /*!
     * POSIX 64bit binary.
     */
    BINARY_POSIX64 = 2,

    /*!
     * Windows 32bit binary.
     */
    BINARY_WIN32 = 3,

    /*!
     * Windows 64bit binary.
     */
    BINARY_WIN64 = 4,

    /*!
     * Other binary type.
     */
    BINARY_OTHER = 5

} BinaryType;

/*!
 * Plugin type.
 * Some files are handled as if they were plugins.
 */
typedef enum {
    /*!
     * Null plugin type.
     */
    PLUGIN_NONE = 0,

    /*!
     * Internal plugin.
     */
    PLUGIN_INTERNAL = 1,

    /*!
     * LADSPA plugin.
     */
    PLUGIN_LADSPA = 2,

    /*!
     * DSSI plugin.
     */
    PLUGIN_DSSI = 3,

    /*!
     * LV2 plugin.
     */
    PLUGIN_LV2 = 4,

    /*!
     * VST plugin.
     */
    PLUGIN_VST = 5,

    /*!
     * AU plugin.
     * @note MacOS only
     */
    PLUGIN_AU = 6,

    /*!
     * Csound file.
     */
    PLUGIN_CSOUND = 7,

    /*!
     * GIG file.
     */
    PLUGIN_GIG = 8,

    /*!
     * SF2 file (also known as SoundFont).
     */
    PLUGIN_SF2 = 9,

    /*!
     * SFZ file.
     */
    PLUGIN_SFZ = 10

} PluginType;

/*!
 * Plugin category, which describes the functionality of a plugin.
 */
typedef enum {
    /*!
     * Null plugin category.
     */
    PLUGIN_CATEGORY_NONE = 0,

    /*!
     * A synthesizer or generator.
     */
    PLUGIN_CATEGORY_SYNTH = 1,

    /*!
     * A delay or reverb.
     */
    PLUGIN_CATEGORY_DELAY = 2,

    /*!
     * An equalizer.
     */
    PLUGIN_CATEGORY_EQ = 3,

    /*!
     * A filter.
     */
    PLUGIN_CATEGORY_FILTER = 4,

    /*!
     * A distortion plugin.
     */
    PLUGIN_CATEGORY_DISTORTION = 5,

    /*!
     * A 'dynamic' plugin (amplifier, compressor, gate, etc).
     */
    PLUGIN_CATEGORY_DYNAMICS = 6,

    /*!
     * A 'modulator' plugin (chorus, flanger, phaser, etc).
     */
    PLUGIN_CATEGORY_MODULATOR = 7,

    /*!
     * An 'utility' plugin (analyzer, converter, mixer, etc).
     */
    PLUGIN_CATEGORY_UTILITY = 8,

    /*!
     * Miscellaneous plugin (used to check if the plugin has a category).
     */
    PLUGIN_CATEGORY_OTHER = 9

} PluginCategory;

/*!
 * Special parameters used internally in Carla.\n
 * Plugins do not know about their existence.
 */
typedef enum {
    /*!
     * Null parameter.
     */
    PARAMETER_NULL = -1,

    /*!
     * Active parameter, boolean type.\n
     * Default is 'false'.
     */
    PARAMETER_ACTIVE = -2,

    /*!
     * Dry/Wet parameter.\n
     * Range 0.0...1.0; default is 1.0.
     */
    PARAMETER_DRYWET = -3,

    /*!
     * Volume parameter.\n
     * Range 0.0...1.27; default is 1.0.
     */
    PARAMETER_VOLUME = -4,

    /*!
     * Stereo Balance-Left parameter.\n
     * Range -1.0...1.0; default is -1.0.
     */
    PARAMETER_BALANCE_LEFT = -5,

    /*!
     * Stereo Balance-Right parameter.\n
     * Range -1.0...1.0; default is 1.0.
     */
    PARAMETER_BALANCE_RIGHT = -6,

    /*!
     * Mono Panning parameter.\n
     * Range -1.0...1.0; default is 0.0.
     */
    PARAMETER_PANNING = -7,

    /*!
     * MIDI Control channel, integer type.\n
     * Range -1...15 (-1 = off).
     */
    PARAMETER_CTRL_CHANNEL = -8,

    /*!
     * Max value, defined only for convenience.
     */
    PARAMETER_MAX = -9

} InternalParameterIndex;

/*!
 * Engine callback opcodes.\n
 * Front-ends must never block indefinitely during a callback.
 * @see EngineCallbackFunc, CarlaEngine::setCallback() and carla_set_engine_callback()
 */
typedef enum {
    /*!
     * Debug.\n
     * This opcode is undefined and used only for testing purposes.
     */
    ENGINE_CALLBACK_DEBUG = 0,

    /*!
     * A plugin has been added.
     * @param pluginId Plugin Id
     * @param valueStr Plugin name
     */
    ENGINE_CALLBACK_PLUGIN_ADDED = 1,

    /*!
     * A plugin has been removed.
     * @param pluginId Plugin Id
     */
    ENGINE_CALLBACK_PLUGIN_REMOVED = 2,

    /*!
     * A plugin has been renamed.
     * @param pluginId Plugin Id
     * @param valueStr New plugin name
     */
    ENGINE_CALLBACK_PLUGIN_RENAMED = 3,

    /*!
     * A plugin has become unavailable.
     * @param pluginId Plugin Id
     * @param valueStr Related error string
     */
    ENGINE_CALLBACK_PLUGIN_UNAVAILABLE = 4,

    /*!
     * A parameter value has changed.
     * @param pluginId Plugin Id
     * @param value1   Parameter index
     * @param value3   New parameter value
     */
    ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED = 5,

    /*!
     * A parameter default has changed.
     * @param pluginId Plugin Id
     * @param value1   Parameter index
     * @param value3   New default value
     */
    ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED = 6,

    /*!
     * A parameter's MIDI CC has changed.
     * @param pluginId Plugin Id
     * @param value1   Parameter index
     * @param value2   New MIDI CC
     */
    ENGINE_CALLBACK_PARAMETER_MIDI_CC_CHANGED = 7,

    /*!
     * A parameter's MIDI channel has changed.
     * @param pluginId Plugin Id
     * @param value1   Parameter index
     * @param value2   New MIDI channel
     */
    ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED = 8,

    /*!
     * The current program of a plugin has changed.
     * @param pluginId Plugin Id
     * @param value1   New program index
     */
    ENGINE_CALLBACK_PROGRAM_CHANGED = 9,

    /*!
     * The current MIDI program of a plugin has changed.
     * @param pluginId Plugin Id
     * @param value1   New MIDI bank
     * @param value2   New MIDI program
     */
    ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED = 10,

    /*!
     * A plugin's custom UI state has changed.
     * @param pluginId Plugin Id
     * @param value1   New state, as follows:\n
     *                  0: UI is now hidden\n
     *                  1: UI is now visible\n
     *                 -1: UI has crashed and should not be shown again
     */
    ENGINE_CALLBACK_UI_STATE_CHANGED = 11,

    /*!
     * A note has been pressed.
     * @param pluginId Plugin Id
     * @param value1   Channel
     * @param value2   Note
     * @param value3   Velocity
     */
    ENGINE_CALLBACK_NOTE_ON = 12,

    /*!
     * A note has been released.
     * @param pluginId Plugin Id
     * @param value1   Channel
     * @param value2   Note
     */
    ENGINE_CALLBACK_NOTE_OFF = 13,

    /*!
     * A plugin needs update.
     * @param pluginId Plugin Id
     */
    ENGINE_CALLBACK_UPDATE = 14,

    /*!
     * A plugin's data/information has changed.
     * @param pluginId Plugin Id
     */
    ENGINE_CALLBACK_RELOAD_INFO = 15,

    /*!
     * A plugin's parameters have changed.
     * @param pluginId Plugin Id
     */
    ENGINE_CALLBACK_RELOAD_PARAMETERS = 16,

    /*!
     * A plugin's programs have changed.
     * @param pluginId Plugin Id
     */
    ENGINE_CALLBACK_RELOAD_PROGRAMS = 17,

    /*!
     * A plugin state has changed.
     * @param pluginId Plugin Id
     */
    ENGINE_CALLBACK_RELOAD_ALL = 18,

    /*!
     * A patchbay client has been added.
     * @param pluginId Client Id
     * @param valueStr Client name and icon, as "name:icon"
     */
    ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED = 19,

    /*!
     * A patchbay client has been removed.
     * @param pluginId Client Id
     */
    ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED = 20,

    /*!
     * A patchbay client has been renamed.
     * @param pluginId Client Id
     * @param valueStr New client name
     */
    ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED = 21,

    /*!
     * A patchbay client icon has changed.
     * @param pluginId Client Id
     * @param valueStr New icon name
     */
    ENGINE_CALLBACK_PATCHBAY_CLIENT_ICON_CHANGED = 22,

    /*!
     * A patchbay port has been added.
     * @param pluginId Client Id
     * @param value1   Port Id
     * @param value2   Port hints
     * @param valueStr Port name
     * @see PatchbayPortHints
     */
    ENGINE_CALLBACK_PATCHBAY_PORT_ADDED = 23,

    /*!
     * A patchbay port has been removed.
     * @param pluginId Client Id
     * @param value1   Port Id
     */
    ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED = 24,

    /*!
     * A patchbay port has been renamed.
     * @param pluginId Client Id
     * @param value1   Port Id
     * @param valueStr New port name
     */
    ENGINE_CALLBACK_PATCHBAY_PORT_RENAMED = 25,

    /*!
     * A patchbay connection has been added.
     * @param value1 Output port Id
     * @param value2 Input port Id
     */
    ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED = 26,

    /*!
     * A patchbay connection has been removed.
     * @param value1 Output port Id
     * @param value2 Input port Id
     */
    ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED = 27,

    /*!
     * Engine started.
     * @param value1   Process mode
     * @param value2   Transport mode
     * @param valuestr Engine driver
     * @see EngineProcessMode
     * @see EngineTransportMode
     */
    ENGINE_CALLBACK_ENGINE_STARTED = 28,

    /*!
     * Engine stopped.
     */
    ENGINE_CALLBACK_ENGINE_STOPPED = 29,

    /*!
     * Engine process mode has changed.
     * @param value1 New process mode
     * @see EngineProcessMode
     */
    ENGINE_CALLBACK_PROCESS_MODE_CHANGED = 30,

    /*!
     * Engine transport mode has changed.
     * @param value1 New transport mode
     * @see EngineTransportMode
     */
    ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED = 31,

    /*!
     * Engine buffer-size changed.
     * @param value1 New buffer size
     */
    ENGINE_CALLBACK_BUFFER_SIZE_CHANGED = 32,

    /*!
     * Engine sample-rate changed.
     * @param value3 New sample rate
     */
    ENGINE_CALLBACK_SAMPLE_RATE_CHANGED = 33,

    /*!
     * Show a message as information.
     * @param valueStr The message
     */
    ENGINE_CALLBACK_INFO = 34,

    /*!
     * Show a message as an error.
     * @param valueStr The message
     */
    ENGINE_CALLBACK_ERROR = 35,

    /*!
     * The engine has crashed or malfunctioned and will no longer work.
     */
    ENGINE_CALLBACK_QUIT = 36

} EngineCallbackOpcode;

/*!
 * Engine options.
 * @see CarlaEngine::getOptions(), CarlaEngine::setOption() and carla_set_engine_option()
 */
typedef enum {
    /*!
     * Debug.\n
     * This option is undefined and used only for testing purposes.
     */
    ENGINE_OPTION_DEBUG = 0,

    /*!
     * Set the engine processing mode.\n
     * Default is ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS on Linux and ENGINE_PROCESS_MODE_CONTINUOUS_RACK for all other OSes.
     * @see EngineProcessMode
     */
    ENGINE_OPTION_PROCESS_MODE = 1,

    /*!
     * Set the engine transport mode.\n
     * Default is ENGINE_TRANSPORT_MODE_JACK on Linux and ENGINE_TRANSPORT_MODE_INTERNAL for all other OSes.
     * @see EngineTransportMode
     */
    ENGINE_OPTION_TRANSPORT_MODE = 2,

    /*!
     * Force mono plugins as stereo, by running 2 instances at the same time.\n
     * Default is false, but always true when process mode is ENGINE_PROCESS_MODE_CONTINUOUS_RACK.
     * @note Not supported by all plugins
     * @see PLUGIN_OPTION_FORCE_STEREO
     */
    ENGINE_OPTION_FORCE_STEREO = 3,

    /*!
     * Use plugin bridges whenever possible.\n
     * Default is no, EXPERIMENTAL.
     */
    ENGINE_OPTION_PREFER_PLUGIN_BRIDGES = 4,

    /*!
     * Use UI bridges whenever possible, otherwise UIs will be directly handled in the main backend thread.\n
     * Default is yes.
     */
    ENGINE_OPTION_PREFER_UI_BRIDGES = 5,

    /*!
     * Make custom plugin UIs always-on-top.\n
     * Default is yes.
     */
    ENGINE_OPTION_UIS_ALWAYS_ON_TOP = 6,

    /*!
     * Maximum number of parameters allowed.\n
     * Default is MAX_DEFAULT_PARAMETERS.
     */
    ENGINE_OPTION_MAX_PARAMETERS = 7,

    /*!
     * Timeout value for how much to wait for UI bridges to respond, in milliseconds.\n
     * Default is 4000 (4 seconds).
     */
    ENGINE_OPTION_UI_BRIDGES_TIMEOUT = 8,

    /*!
     * Audio number of periods.\n
     * Default is 2.
     */
    ENGINE_OPTION_AUDIO_NUM_PERIODS = 9,

    /*!
     * Audio buffer size.\n
     * Default is 512.
     */
    ENGINE_OPTION_AUDIO_BUFFER_SIZE = 10,

    /*!
     * Audio sample rate.\n
     * Default is 44100.
     */
    ENGINE_OPTION_AUDIO_SAMPLE_RATE = 11,

    /*!
     * Audio device (within a driver).\n
     * Default unset.
     */
    ENGINE_OPTION_AUDIO_DEVICE = 12,

    /*!
     * Set path to the binary files.\n
     * Default unset.
     * @note Must be set for plugin and UI bridges to work
     */
    ENGINE_OPTION_PATH_BINARIES = 13,

    /*!
     * Set path to the resource files.\n
     * Default unset.
     * @note Must be set for some internal plugins to work
     */
    ENGINE_OPTION_PATH_RESOURCES = 14

} EngineOption;

/*!
 * Engine process mode.
 * @see ENGINE_OPTION_PROCESS_MODE
 */
typedef enum {
    /*!
     * Single client mode.\n
     * Inputs and outputs are added dynamically as needed by plugins.
     */
    ENGINE_PROCESS_MODE_SINGLE_CLIENT = 0,

    /*!
     * Multiple client mode.\n
     * It has 1 master client + 1 client per plugin.
     */
    ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS = 1,

    /*!
     * Single client, 'rack' mode.\n
     * Processes plugins in order of Id, with forced stereo always on.
     */
    ENGINE_PROCESS_MODE_CONTINUOUS_RACK = 2,

    /*!
     * Single client, 'patchbay' mode.
     */
    ENGINE_PROCESS_MODE_PATCHBAY = 3,

    /*!
     * Special mode, used in plugin-bridges only.
     */
    ENGINE_PROCESS_MODE_BRIDGE = 4

} EngineProcessMode;

/*!
 * Engine transport mode
 * @see ENGINE_OPTION_TRANSPORT_MODE
 */
typedef enum {
    /*!
     * Internal transport mode.
     */
    ENGINE_TRANSPORT_MODE_INTERNAL = 0,

    /*!
     * Transport from JACK.\n
     * Only available if driver name is "JACK".
     */
    ENGINE_TRANSPORT_MODE_JACK = 1,

    /*!
     * Transport from host, used when Carla is a plugin.
     */
    ENGINE_TRANSPORT_MODE_PLUGIN = 2,

    /*!
     * Special mode, used in plugin-bridges only.
     */
    ENGINE_TRANSPORT_MODE_BRIDGE = 3

} EngineTransportMode;

/*!
 * Engine callback function.\n
 * Front-ends must never block indefinitely during a callback.
 * @see EngineCallbackOpcode, CarlaEngine::setCallback() and carla_set_engine_callback()
 */
typedef void (*EngineCallbackFunc)(void* ptr, EngineCallbackOpcode action, unsigned int pluginId, int value1, int value2, float value3, const char* valueStr);

/*!
 * Parameter data.
 */
typedef struct _ParameterData {
    /*!
     *
     */
    int32_t index;

    /*!
     *
     */
    int32_t rindex;

    /*!
     *
     */
    unsigned int hints;

    /*!
     *
     */
    int16_t midiCC;

    /*!
     *
     */
    uint8_t midiChannel;

#ifdef __cplusplus
    /*!
     *
     */
    _ParameterData() noexcept
        : index(PARAMETER_NULL),
          rindex(-1),
          hints(0x0),
          midiCC(-1),
          midiChannel(0) {}
#endif
} ParameterData;

/*!
 * Parameter ranges.
 */
typedef struct _ParameterRanges {
    /*!
     * Default value.
     */
    float def;

    /*!
     * Minimum value.
     */
    float min;

    /*!
     * Maximum value.
     */
    float max;

    /*!
     * Regular step value.
     */
    float step;

    /*!
     * Small step value.
     */
    float stepSmall;

    /*!
     * Large step value.
     */
    float stepLarge;

#ifdef __cplusplus
    /*!
     *
     */
    _ParameterRanges() noexcept
        : def(0.0f),
          min(0.0f),
          max(1.0f),
          step(0.01f),
          stepSmall(0.0001f),
          stepLarge(0.1f) {}

    /*!
     * Fix default value range.
     */
    void fixDefault() noexcept
    {
        fixValue(def);
    }

    /*!
     * Fix a value within range.
     */
    void fixValue(float& value) const noexcept
    {
        if (value <= min)
            value = min;
        else if (value > max)
            value = max;
    }

    /*!
     *
     */
    float getFixedValue(const float& value) const noexcept
    {
        if (value <= min)
            return min;
        if (value >= max)
            return max;
        return value;
    }

    /*!
     * Get a value normalized to 0.0<->1.0.
     */
    float getNormalizedValue(const float& value) const noexcept
    {
        const float normValue((value - min) / (max - min));

        if (normValue <= 0.0f)
            return 0.0f;
        if (normValue >= 1.0f)
            return 1.0f;
        return normValue;
    }

    /*!
     * Get a value normalized to 0.0<->1.0, but fix range first.
     */
    float getFixedAndNormalizedValue(const float& value) const noexcept
    {
        if (value <= min)
            return 0.0f;
        if (value >= max)
            return 1.0f;

        const float normValue((value - min) / (max - min));

        if (normValue <= 0.0f)
            return 0.0f;
        if (normValue >= 1.0f)
            return 1.0f;

        return normValue;
    }

    /*!
     * Get a proper value previously normalized to 0.0<->1.0.
     */
    float getUnnormalizedValue(const float& value) const noexcept
    {
        return value * (max - min) + min;
    }
#endif
} ParameterRanges;

/*!
 * MIDI Program data.
 */
typedef struct _MidiProgramData {
    /*!
     *
     */
    uint32_t bank;

    /*!
     *
     */
    uint32_t program;

    /*!
     *
     */
    const char* name;

#ifdef __cplusplus
    /*!
     *
     */
    _MidiProgramData() noexcept
        : bank(0),
          program(0),
          name(nullptr) {}
#endif
} MidiProgramData;

/*!
 * Custom data, for saving key:value 'dictionaries'.
 */
typedef struct _CustomData {
    /*!
     * Value type, in URI form.
     * @see CustomDataTypes
     */
    const char* type;

    /*!
     * Key.
     * @see CustomDataKeys
     */
    const char* key;

    /*!
     * Value.
     */
    const char* value;

#ifdef __cplusplus
    _CustomData() noexcept
        : type(nullptr),
          key(nullptr),
          value(nullptr) {}
#endif
} CustomData;

/*!
 * Engine driver device information.
 */
typedef struct _EngineDriverDeviceInfo {
    /*!
     *
     */
    unsigned int hints;

    /*!
     *
     */
    const uint32_t* bufferSizes; // terminated with 0

    /*!
     *
     */
    const double* sampleRates;   // terminated with 0.0

#ifdef __cplusplus
    /*!
     *
     */
    _EngineDriverDeviceInfo()
        : hints(0x0),
          bufferSizes(nullptr),
          sampleRates(nullptr) {}
#endif
} EngineDriverDeviceInfo;

/** @} */

#ifdef __cplusplus
/* Forward declarations of commonly used Carla classes */
class CarlaEngine;
class CarlaPlugin;
/* End namespace */
CARLA_BACKEND_END_NAMESPACE
#endif

#endif /* CARLA_BACKEND_HPP_INCLUDED */
