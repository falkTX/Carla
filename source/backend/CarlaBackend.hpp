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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_BACKEND_HPP__
#define __CARLA_BACKEND_HPP__

#include "CarlaDefines.hpp"

#include <cstdint>

#define CARLA_BACKEND_START_NAMESPACE namespace CarlaBackend {
#define CARLA_BACKEND_END_NAMESPACE }
#define CARLA_BACKEND_USE_NAMESPACE using namespace CarlaBackend;

#define STR_MAX 0xFF

CARLA_BACKEND_START_NAMESPACE

/*!
 * @defgroup CarlaBackendAPI Carla Backend API
 *
 * The Carla Backend API.\n
 * This is the base definitions for everything in the Carla code.
 *
 * @{
 */

const unsigned int MAX_DEFAULT_PLUGINS    = 99;  //!< Maximum default number of loadable plugins
const unsigned int MAX_RACK_PLUGINS       = 16;  //!< Maximum number of loadable plugins in rack mode
const unsigned int MAX_PATCHBAY_PLUGINS   = 255; //!< Maximum number of loadable plugins in patchbay mode
const unsigned int MAX_DEFAULT_PARAMETERS = 200; //!< Maximum default number of parameters allowed.\see OPTION_MAX_PARAMETERS

/*!
 * @defgroup PluginHints Plugin Hints
 *
 * Various plugin hints.
 * \see CarlaPlugin::hints()
 * @{
 */
const unsigned int PLUGIN_IS_BRIDGE         = 0x001; //!< Plugin is a bridge (ie, BridgePlugin). This hint is required because "bridge" itself is not a plugin type.
const unsigned int PLUGIN_IS_RTSAFE         = 0x002; //!< Plugin is hard real-time safe.
const unsigned int PLUGIN_IS_SYNTH          = 0x004; //!< Plugin is a synthesizer (produces sound).
const unsigned int PLUGIN_HAS_GUI           = 0x010; //!< Plugin has its own custom GUI.
const unsigned int PLUGIN_HAS_SINGLE_THREAD = 0x020; //!< Plugin needs a single thread for both DSP and UI events.
const unsigned int PLUGIN_CAN_DRYWET        = 0x100; //!< Plugin can make use of Dry/Wet controls.
const unsigned int PLUGIN_CAN_VOLUME        = 0x200; //!< Plugin can make use of Volume controls.
const unsigned int PLUGIN_CAN_BALANCE       = 0x400; //!< Plugin can make use of Left & Right Balance controls.
const unsigned int PLUGIN_CAN_PANNING       = 0x800; //!< Plugin can make use of Panning controls.
/**@}*/

/*!
 * @defgroup PluginOptions Plugin Options
 *
 * Various plugin options.
 * \see CarlaPlugin::availableOptions() and CarlaPlugin::options()
 * @{
 */
const unsigned int PLUGIN_OPTION_FIXED_BUFFER          = 0x001; //!< Use a constant, fixed-size audio buffer
const unsigned int PLUGIN_OPTION_FORCE_STEREO          = 0x002; //!< Force mono plugin as stereo
const unsigned int PLUGIN_OPTION_MAP_PROGRAM_CHANGES   = 0x004; //!< Map MIDI-Programs to plugin programs
const unsigned int PLUGIN_OPTION_USE_CHUNKS            = 0x008; //!< Use chunks to save data
const unsigned int PLUGIN_OPTION_SEND_CONTROL_CHANGES  = 0x010; //!< Send MIDI CC events
const unsigned int PLUGIN_OPTION_SEND_CHANNEL_PRESSURE = 0x020; //!< Send MIDI channel pressure events
const unsigned int PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH  = 0x040; //!< Send MIDI note aftertouch events
const unsigned int PLUGIN_OPTION_SEND_PITCHBEND        = 0x080; //!< Send MIDI pitchbend events
const unsigned int PLUGIN_OPTION_SEND_ALL_SOUND_OFF    = 0x100; //!< Send MIDI all sounds/notes off events, single-note offs otherwise
/**@}*/

/*!
 * @defgroup ParameterHints Parameter Hints
 *
 * Various parameter hints.
 * \see CarlaPlugin::parameterData()
 * @{
 */
const unsigned int PARAMETER_IS_BOOLEAN       = 0x01; //!< Parameter value is a boolean (always at minimum or maximum values).
const unsigned int PARAMETER_IS_INTEGER       = 0x02; //!< Parameter value is an integer.
const unsigned int PARAMETER_IS_LOGARITHMIC   = 0x04; //!< Parameter is logarithmic.
const unsigned int PARAMETER_IS_ENABLED       = 0x08; //!< Parameter is enabled and will be shown in the host built-in editor.
const unsigned int PARAMETER_IS_AUTOMABLE     = 0x10; //!< Parameter is automable (realtime safe)
const unsigned int PARAMETER_USES_SAMPLERATE  = 0x20; //!< Parameter needs sample rate to work (value and ranges are multiplied by SR, and must be divided by SR on save).
const unsigned int PARAMETER_USES_SCALEPOINTS = 0x40; //!< Parameter uses scalepoints to define internal values in a meaningful way.
const unsigned int PARAMETER_USES_CUSTOM_TEXT = 0x80; //!< Parameter uses custom text for displaying its value.\see CarlaPlugin::getParameterText()
/**@}*/

/*!
 * @defgroup PatchbayPortHints Patchbay Port Hints
 *
 * Various patchbay port hints.
 * @{
 */
const unsigned int PATCHBAY_PORT_IS_INPUT  = 0x1; //!< Patchbay port is input.
const unsigned int PATCHBAY_PORT_IS_OUTPUT = 0x2; //!< Patchbay port is output.
const unsigned int PATCHBAY_PORT_IS_AUDIO  = 0x4; //!< Patchbay port is of Audio type.
const unsigned int PATCHBAY_PORT_IS_MIDI   = 0x8; //!< Patchbay port is of MIDI type.
/**@}*/

/*!
 * @defgroup CustomDataTypes Custom Data types
 *
 * The type defines how the \param value in CustomData is stored.
 *
 * Types are valid URIs.\n
 * Any non-string, non-simple type (not integral) is saved in a base64 encoded format.
 */
const char* const CUSTOM_DATA_INVALID = nullptr;                                  //!< Null or Invalid data.
const char* const CUSTOM_DATA_CHUNK   = "http://kxstudio.sf.net/ns/carla/chunk";  //!< Carla Chunk URI
const char* const CUSTOM_DATA_STRING  = "http://kxstudio.sf.net/ns/carla/string"; //!< Carla String URI
/**@}*/

/*!
 * The binary type of a plugin.
 */
enum BinaryType {
    BINARY_NONE    = 0, //!< Null binary type.
    BINARY_POSIX32 = 1, //!< POSIX 32bit.
    BINARY_POSIX64 = 2, //!< POSIX 64bit.
    BINARY_WIN32   = 3, //!< Windows 32bit.
    BINARY_WIN64   = 4, //!< Windows 64bit.
    BINARY_OTHER   = 5  //!< Other.
};

/*!
 * All the available plugin types, as provided by subclasses of CarlaPlugin.\n
 * Some plugin classes might provide more than 1 plugin type.
 */
enum PluginType {
    PLUGIN_NONE     = 0, //!< Null plugin type.
    PLUGIN_INTERNAL = 1, //!< Internal plugin.
    PLUGIN_LADSPA   = 2, //!< LADSPA plugin.
    PLUGIN_DSSI     = 3, //!< DSSI plugin.
    PLUGIN_LV2      = 4, //!< LV2 plugin.
    PLUGIN_VST      = 5, //!< VST1/2 plugin.
    PLUGIN_VST3     = 6, //!< VST3 plugin.
    PLUGIN_GIG      = 7, //!< GIG sound kit, implemented via LinuxSampler.
    PLUGIN_SF2      = 8, //!< SF2 sound kit (aka SoundFont), implemented via FluidSynth.
    PLUGIN_SFZ      = 9  //!< SFZ sound kit, implemented via LinuxSampler.
};

/*!
 * Plugin category, describing the funtionality of a plugin.\n
 * When a plugin fails to tell his own category, one is atributted to it based on its name.
 */
enum PluginCategory {
    PLUGIN_CATEGORY_NONE      = 0, //!< Null plugin category.
    PLUGIN_CATEGORY_SYNTH     = 1, //!< A synthesizer or generator.
    PLUGIN_CATEGORY_DELAY     = 2, //!< A delay or reverberator.
    PLUGIN_CATEGORY_EQ        = 3, //!< An equalizer.
    PLUGIN_CATEGORY_FILTER    = 4, //!< A filter.
    PLUGIN_CATEGORY_DYNAMICS  = 5, //!< A 'dynamic' plugin (amplifier, compressor, gate, etc).
    PLUGIN_CATEGORY_MODULATOR = 6, //!< A 'modulator' plugin (chorus, flanger, phaser, etc).
    PLUGIN_CATEGORY_UTILITY   = 7, //!< An 'utility' plugin (analyzer, converter, mixer, etc).
    PLUGIN_CATEGORY_OTHER     = 8  //!< Misc plugin (used to check if the plugin has a category).
};

/*!
 * Plugin parameter type.
 */
enum ParameterType {
    PARAMETER_UNKNOWN       = 0, //!< Null parameter type.
    PARAMETER_INPUT         = 1, //!< Input parameter.
    PARAMETER_OUTPUT        = 2, //!< Ouput parameter.
    PARAMETER_LATENCY       = 3, //!< Special latency parameter, used in LADSPA, DSSI and LV2 plugins.
    PARAMETER_SAMPLE_RATE   = 4, //!< Special sample-rate parameter, used in LADSPA, DSSI and LV2 plugins.
#ifdef WANT_LV2
    PARAMETER_LV2_FREEWHEEL = 5, //!< Special LV2 Plugin parameter used to report freewheel (offline) mode.
    PARAMETER_LV2_TIME      = 6  //!< Special LV2 Plugin parameter used to report time information.
#endif
};

/*!
 * Internal parameter indexes.\n
 * These are special parameters used internally, plugins do not know about their existence.
 */
enum InternalParametersIndex {
    PARAMETER_NULL          = -1, //!< Null parameter.
    PARAMETER_ACTIVE        = -2, //!< Active parameter, can only be 'true' or 'false'; default is 'false'.
    PARAMETER_DRYWET        = -3, //!< Dry/Wet parameter, range 0.0...1.0; default is 1.0.
    PARAMETER_VOLUME        = -4, //!< Volume parameter, range 0.0...1.27; default is 1.0.
    PARAMETER_BALANCE_LEFT  = -5, //!< Stereo Balance-Left parameter, range -1.0...1.0; default is -1.0.
    PARAMETER_BALANCE_RIGHT = -6, //!< Stereo Balance-Right parameter, range -1.0...1.0; default is 1.0.
    PARAMETER_PANNING       = -7, //!< Mono Panning parameter, range -1.0...1.0; default is 0.0.
    PARAMETER_CTRL_CHANNEL  = -8, //!< MIDI Control channel
    PARAMETER_MAX           = -9  //!< Max value, defined for convenience
};

/*!
 * Options used in the CarlaEngine::setOption() and set_option() calls.\n
 * All options except paths must be set before initiliazing or after closing the engine.
 */
enum OptionsType {
    /*!
     * Try to set the current process name.
     * \note Not available on all platforms.
     */
    OPTION_PROCESS_NAME = 0,

    /*!
     * Set the engine processing mode.\n
     * Default is PROCESS_MODE_CONTINUOUS_RACK.
     * \see ProcessMode
     */
    OPTION_PROCESS_MODE = 1,

    /*!
     * Set the engine transport mode.\n
     * Default is TRANSPORT_MODE_INTERNAL.
     * \see TransportMode
     */
    OPTION_TRANSPORT_MODE = 2,

    /*!
     * Force mono plugins as stereo, by running 2 instances at the same time.
     * \note Not supported by all plugins.
     */
    OPTION_FORCE_STEREO = 3,

    /*!
     * Use plugin bridges whenever possible.\n
     * Default is no, EXPERIMENTAL.
     */
    OPTION_PREFER_PLUGIN_BRIDGES = 4,

    /*!
     * Use OSC-UI bridges whenever possible, otherwise UIs will be handled in the main thread.\n
     * Default is yes.
     */
    OPTION_PREFER_UI_BRIDGES = 5,

#ifdef WANT_DSSI
    /*!
     * Use (unofficial) dssi-vst chunks feature.\n
     * Default is no.
     */
    OPTION_USE_DSSI_VST_CHUNKS = 6,
#endif

    /*!
     * Maximum number of parameters allowed.\n
     * Default is MAX_DEFAULT_PARAMETERS.
     */
    OPTION_MAX_PARAMETERS = 7,

    /*!
     * Timeout value in ms for how much to wait for OSC-Bridges to respond.\n
     * Default is 4000 (4 secs).
     */
    OPTION_OSC_UI_TIMEOUT = 8,

    /*!
     * JACK auto-connect to hardware ports.
     */
    OPTION_JACK_AUTOCONNECT = 9,

    /*!
     * JACK Transport master.
     */
    OPTION_JACK_TIMEMASTER = 10,

#ifdef WANT_RTAUDIO
    /*!
     * RtAudio buffer size.
     */
    OPTION_RTAUDIO_BUFFER_SIZE = 11,

    /*!
     * RtAudio sample rate.
     */
    OPTION_RTAUDIO_SAMPLE_RATE = 12,

    /*!
     * RtAudio device.
     */
    OPTION_RTAUDIO_DEVICE = 13,
#endif

    /*!
     * Set path to the backend resource files.\n
     * Default unset.
     *
     * \note Must be set for some internal plugins to work!
     */
    OPTION_PATH_RESOURCES = 14,

#ifndef BUILD_BRIDGE
    /*!
     * Set path to the native plugin bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_NATIVE = 15,

    /*!
     * Set path to the POSIX 32bit plugin bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_POSIX32 = 16,

    /*!
     * Set path to the POSIX 64bit plugin bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_POSIX64 = 17,

    /*!
     * Set path to the Windows 32bit plugin bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_WIN32 = 18,

    /*!
     * Set path to the Windows 64bit plugin bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_WIN64 = 19,
#endif

#ifdef WANT_LV2
    /*!
     * Set path to the LV2 Gtk2 UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_GTK2 = 20,

    /*!
     * Set path to the LV2 Gtk3 UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_GTK3 = 21,

    /*!
     * Set path to the LV2 Qt4 UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_QT4 = 22,

    /*!
     * Set path to the LV2 Qt5 UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_QT5 = 23,

    /*!
     * Set path to the LV2 Cocoa UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_COCOA = 24,

    /*!
     * Set path to the LV2 Windows UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_WINDOWS = 25,

    /*!
     * Set path to the LV2 X11 UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_X11 = 26,
#endif

#ifdef WANT_VST
    /*!
     * Set path to the VST Cocoa UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_VST_COCOA = 27,

    /*!
     * Set path to the VST HWND UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_VST_HWND = 28,

    /*!
     * Set path to the VST X11 UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_VST_X11 = 29
#endif
};

/*!
 * Opcodes sent from the engine callback to the GUI, as defined by CallbackFunc.
 *
 * \see CarlaEngine::setCallback() and set_callback_function()
 */
enum CallbackType {
    /*!
     * Debug.\n
     * This opcode is undefined and used only for testing purposes.
     */
    CALLBACK_DEBUG = 0,

    /*!
     * A plugin has been added.
     * \param valueStr Plugin name
     */
    CALLBACK_PLUGIN_ADDED = 1,

    /*!
     * A plugin has been removed.
     */
    CALLBACK_PLUGIN_REMOVED = 2,

    /*!
     * A plugin has been renamed.
     * \param valueStr New name
     */
    CALLBACK_PLUGIN_RENAMED = 3,

    /*!
     * A parameter value has been changed.
     *
     * \param value1 Parameter index
     * \param value3 Value
     */
    CALLBACK_PARAMETER_VALUE_CHANGED = 4,

    /*!
     * A parameter default has changed.
     *
     * \param value1 Parameter index
     * \param value3 New default value
     */
    CALLBACK_PARAMETER_DEFAULT_CHANGED = 5,

    /*!
     * A parameter's MIDI channel has been changed.
     *
     * \param value1 Parameter index
     * \param value2 MIDI channel
     */
    CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED = 6,

    /*!
     * A parameter's MIDI CC has been changed.
     *
     * \param value1 Parameter index
     * \param value2 MIDI CC
     */
    CALLBACK_PARAMETER_MIDI_CC_CHANGED = 7,

    /*!
     * The current program has has been changed.
     *
     * \param value1 Program index
     */
    CALLBACK_PROGRAM_CHANGED = 8,

    /*!
     * The current MIDI program has been changed.
     *
     * \param value1 MIDI bank
     * \param value2 MIDI program
     */
    CALLBACK_MIDI_PROGRAM_CHANGED = 9,

    /*!
     * A note has been pressed.
     *
     * \param value1 Channel
     * \param value2 Note
     * \param value3 Velocity
     */
    CALLBACK_NOTE_ON = 10,

    /*!
     * A note has been released.
     *
     * \param value1 Channel
     * \param value2 Note
     */
    CALLBACK_NOTE_OFF = 11,

    /*!
     * The plugin's custom GUI state has changed.
     *
     * \param value1 State, as follows:.\n
     *                0: GUI has been closed or hidden\n
     *                1: GUI has been shown\n
     *               -1: GUI has crashed and should not be shown again
     */
    CALLBACK_SHOW_GUI = 12,

    /*!
     * The plugin needs update.
     */
    CALLBACK_UPDATE = 13,

    /*!
     * The plugin's data/information has changed.
     */
    CALLBACK_RELOAD_INFO = 14,

    /*!
     * The plugin's parameters have changed.
     */
    CALLBACK_RELOAD_PARAMETERS = 15,

    /*!
     * The plugin's programs have changed.
     */
    CALLBACK_RELOAD_PROGRAMS = 16,

    /*!
     * The plugin's state has changed.
     */
    CALLBACK_RELOAD_ALL = 17,

    /*!
     * Canvas client added
     *
     * \param value1   Client Id
     * \param valueStr Client name
     */
    CALLBACK_PATCHBAY_CLIENT_ADDED = 18,

    /*!
     * Canvas client removed
     *
     * \param value1 Client Id
     */
    CALLBACK_PATCHBAY_CLIENT_REMOVED = 19,

    /*!
     * Canvas client renamed
     *
     * \param value1   Client Id
     * \param valueStr New client name
     */
    CALLBACK_PATCHBAY_CLIENT_RENAMED = 20,

    /*!
     * Canvas port added
     *
     * \param value1   Client Id
     * \param value2   Port Id
     * \param value3   Port flags
     * \param valueStr Port name
     */
    CALLBACK_PATCHBAY_PORT_ADDED = 21,

    /*!
     * Canvas port remvoed
     *
     * \param value1 Port Id
     */
    CALLBACK_PATCHBAY_PORT_REMOVED = 22,

    /*!
     * Canvas port renamed
     *
     * \param value1   Port Id
     * \param valueStr New port name
     */
    CALLBACK_PATCHBAY_PORT_RENAMED = 23,

    /*!
     * Canvas port connection added
     *
     * \param value1 Output port Id
     * \param value2 Input port Id
     */
    CALLBACK_PATCHBAY_CONNECTION_ADDED = 24,

    /*!
     * Canvas port connection removed
     *
     * \param value1 Output port Id
     * \param value2 Input port Id
     */
    CALLBACK_PATCHBAY_CONNECTION_REMOVED = 25,

    /*!
     * Non-Session-Manager Announce message.
     */
    CALLBACK_BUFFER_SIZE_CHANGED = 26,

    /*!
     * Non-Session-Manager Announce message.
     */
    CALLBACK_SAMPLE_RATE_CHANGED = 27,

    /*!
     * Non-Session-Manager Announce message.
     */
    CALLBACK_NSM_ANNOUNCE = 28,

    /*!
     * Non-Session-Manager Open message.
     */
    CALLBACK_NSM_OPEN = 29,

    /*!
     * Non-Session-Manager Save message.
     */
    CALLBACK_NSM_SAVE = 30,

    /*!
     * An error occurred, show \a valueStr as an error to user.
     */
    CALLBACK_ERROR = 31,

    /*!
     * The engine has crashed or malfunctioned and will no longer work.
     */
    CALLBACK_QUIT = 32
};

/*!
 * Engine process mode.
 *
 * \see OPTION_PROCESS_MODE
 */
enum ProcessMode {
    PROCESS_MODE_SINGLE_CLIENT    = 0, //!< Single client mode (dynamic input/outputs as needed by plugins)
    PROCESS_MODE_MULTIPLE_CLIENTS = 1, //!< Multiple client mode (1 master client + 1 client per plugin)
    PROCESS_MODE_CONTINUOUS_RACK  = 2, //!< Single client, 'rack' mode. Processes plugins in order of Id, with forced stereo.
    PROCESS_MODE_PATCHBAY         = 3, //!< Single client, 'patchbay' mode.
    PROCESS_MODE_BRIDGE           = 4  //!< Special mode, used in plugin-bridges only.
};

/*!
 * All the available transport modes
 */
enum TransportMode {
    TRANSPORT_MODE_INTERNAL = 0, //!< Internal transport mode.
    TRANSPORT_MODE_JACK     = 1, //!< JACK transport, only available if driver name is "JACK"
    TRANSPORT_MODE_PLUGIN   = 2, //!< Plugin transport from host, used when Carla is a plugin
    TRANSPORT_MODE_BRIDGE   = 3  //!< Special mode, used in plugin-bridges only.
};

/*!
 * Callback function the engine will call when something interesting happens.
 *
 * \see CallbackType and set_callback_function()
 */
typedef void (*CallbackFunc)(void* ptr, CallbackType action, unsigned int pluginId, int value1, int value2, float value3, const char* valueStr);

/*!
 * Parameter data
 */
struct ParameterData {
    ParameterType type;
    int32_t  index;
    int32_t  rindex;
    uint32_t hints;
    uint8_t  midiChannel;
    int16_t  midiCC;

#ifndef DOXYGEN
    ParameterData()
        : type(PARAMETER_UNKNOWN),
          index(PARAMETER_NULL),
          rindex(-1),
          hints(0x0),
          midiChannel(0),
          midiCC(-1) {}
#endif
};

/*!
 * Parameter ranges
 */
struct ParameterRanges {
    float def;
    float min;
    float max;
    float step;
    float stepSmall;
    float stepLarge;

#ifndef DOXYGEN
    ParameterRanges()
        : def(0.0f),
          min(0.0f),
          max(1.0f),
          step(0.01f),
          stepSmall(0.0001f),
          stepLarge(0.1f) {}
#endif

    void fixDefault()
    {
        fixValue(def);
    }

    void fixValue(float& value) const
    {
        if (value < min)
            value = min;
        else if (value > max)
            value = max;
    }

    float fixValue(const float& value) const
    {
        if (value < min)
            return min;
        else if (value > max)
            return max;
        return value;
    }

    float normalizeValue(const float& value) const
    {
        float newValue = (value - min) / (max - min);

        if (newValue < 0.0f)
            newValue = 0.0f;
        else if (newValue > 1.0f)
            newValue = 1.0f;

        return newValue;
    }

    float unnormalizeValue(const float& value) const
    {
        return value * (max - min) + min;
    }
};

/*!
 * MIDI Program data
 */
struct MidiProgramData {
    uint32_t bank;
    uint32_t program;
    const char* name;

#ifndef DOXYGEN
    MidiProgramData()
        : bank(0),
          program(0),
          name(nullptr) {}
#endif
};

/*!
 * Custom data, saving key:value 'dictionaries'.\n
 * \a type is an URI which defines the \a value type.
 *
 * \see CustomDataTypes
 */
struct CustomData {
    const char* type;
    const char* key;
    const char* value;

#ifndef DOXYGEN
    CustomData()
        : type(nullptr),
          key(nullptr),
          value(nullptr) {}
#endif
};

/**@}*/

// forward declarations of commonly used Carla classes
class CarlaEngine;
class CarlaPlugin;

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_BACKEND_HPP__
