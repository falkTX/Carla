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

// TODO - remove ifdef's when Carla stabilizes

#include "carla_defines.hpp"

#include <cstdint>
#include <cstdlib>

#define CARLA_BACKEND_START_NAMESPACE namespace CarlaBackend {
#define CARLA_BACKEND_END_NAMESPACE }
#define CARLA_BACKEND_USE_NAMESPACE using namespace CarlaBackend;

#define STR_MAX 0xFF

CARLA_BACKEND_START_NAMESPACE

/*!
 * @defgroup CarlaBackendAPI Carla Backend API
 *
 * The Carla Backend API
 *
 * @{
 */

const unsigned int MAX_DEFAULT_PLUGINS    = 99;  //!< Maximum default number of loadable plugins
const unsigned int MAX_RACK_PLUGINS       = 16;  //!< Maximum number of loadable plugins in rack mode
const unsigned int MAX_PATCHBAY_PLUGINS   = 999; //!< Maximum number of loadable plugins in patchbay mode
const unsigned int MAX_DEFAULT_PARAMETERS = 200; //!< Maximum default number of parameters allowed.\see OPTION_MAX_PARAMETERS

/*!
 * @defgroup PluginHints Plugin Hints
 *
 * Various plugin hints.
 * \see CarlaPlugin::hints()
 * @{
 */
#ifndef BUILD_BRIDGE
const unsigned int PLUGIN_IS_BRIDGE          = 0x001; //!< Plugin is a bridge (ie, BridgePlugin). This hint is required because "bridge" itself is not a plugin type.
#endif
const unsigned int PLUGIN_IS_RTSAFE          = 0x002; //!< Plugin is hard real-time safe.
const unsigned int PLUGIN_IS_SYNTH           = 0x004; //!< Plugin is a synthesizer (produces sound).
const unsigned int PLUGIN_HAS_GUI            = 0x010; //!< Plugin has its own custom GUI.
const unsigned int PLUGIN_USES_CHUNKS        = 0x020; //!< Plugin uses chunks to save internal data.\see CarlaPlugin::chunkData()
#ifndef BUILD_BRIDGE
const unsigned int PLUGIN_USES_SINGLE_THREAD = 0x040; //!< Plugin needs a single thread for both DSP and UI events.
#endif
const unsigned int PLUGIN_CAN_DRYWET         = 0x100; //!< Plugin can make use of Dry/Wet controls.
const unsigned int PLUGIN_CAN_VOLUME         = 0x200; //!< Plugin can make use of Volume controls.
const unsigned int PLUGIN_CAN_BALANCE        = 0x400; //!< Plugin can make use of Left & Right Balance controls.
const unsigned int PLUGIN_CAN_FORCE_STEREO   = 0x800; //!< Plugin can be used in forced-stereo mode.
/**@}*/

/*!
 * @defgroup PluginOptions Plugin Options
 *
 * Various plugin options.\n
 * ON or OFF defines the default plugin value.
 * \see CarlaPlugin::options()
 * \note PitchBend is disabled by default on VST plugins
 * @{
 */
const unsigned int PLUGIN_OPTION_FIXED_BUFFER         = 0x001; //!< OFF: Use a constant, fixed size audio buffer (128 or lower is used)
const unsigned int PLUGIN_OPTION_FORCE_STEREO         = 0x002; //!< OFF: Force mono plugin as stereo
const unsigned int PLUGIN_OPTION_SELF_AUTOMATION      = 0x004; //!< OFF: Let the plugin handle MIDI-CC automation, not the host
const unsigned int PLUGIN_OPTION_USE_CHUNKS           = 0x008; //!< ON:  Use chunks to save data
const unsigned int PLUGIN_OPTION_SEND_ALL_SOUND_OFF   = 0x010; //!< ON:  Send MIDI ALL_SOUND_OFF / ALL_NOTES_OFF events
const unsigned int PLUGIN_OPTION_SEND_NOTE_OFF_VELO   = 0x020; //!< OFF: Send MIDI Note-Off events with a velocity value
const unsigned int PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH = 0x040; //!< ON:  Send MIDI Note aftertouch events
const unsigned int PLUGIN_OPTION_SEND_PITCHBEND       = 0x080; //!< ON:  Send MIDI Pitchbend events
#ifdef WANT_VST
const unsigned int PLUGIN_OPTION_VST_SUPPLY_IDLE      = 0x100; //!< ON: Idle Plugin's custom GUI (VST only)
const unsigned int PLUGIN_OPTION_VST_UPDATE_DISPLAY   = 0x200; //!< ON: Recheck plugin properties on updateDisplay message (VST Only)
#endif

/**@}*/

/*!
 * @defgroup ParameterHints Parameter Hints
 *
 * Various parameter hints.
 * \see CarlaPlugin::parameterData()
 * @{
 */
const unsigned int PARAMETER_IS_BOOLEAN       = 0x01; //!< Parameter value is always a boolean (always at minimum or maximum range).
const unsigned int PARAMETER_IS_INTEGER       = 0x02; //!< Parameter value is always an integer.
const unsigned int PARAMETER_IS_LOGARITHMIC   = 0x04; //!< Parameter is logarithmic.
const unsigned int PARAMETER_IS_ENABLED       = 0x08; //!< Parameter is enabled and will be shown in the host built-in editor.
const unsigned int PARAMETER_IS_AUTOMABLE     = 0x10; //!< Parameter is automable (realtime safe)
const unsigned int PARAMETER_USES_SAMPLERATE  = 0x20; //!< Parameter needs sample rate to work (value and ranges are multiplied by SR, and must be divided by SR on save).
const unsigned int PARAMETER_USES_SCALEPOINTS = 0x40; //!< Parameter uses scalepoints to define internal values in a meaninful way.
const unsigned int PARAMETER_USES_CUSTOM_TEXT = 0x80; //!< Parameter uses custom text for displaying its value.\see CarlaPlugin::getParameterText()
/**@}*/

#if 0
/*!
 * @defgroup CustomDataTypes Custom Data types
 *
 * The type defines how the \param value in CustomData is stored.
 *
 * Types are valid URIs.\n
 * Any non-string, non-simple type (not integral) is saved in a base64 encoded format.
 */
const char* const CUSTOM_DATA_INVALID = nullptr;                                  //!< Null/Invalid data.
const char* const CUSTOM_DATA_CHUNK   = "http://kxstudio.sf.net/ns/carla/chunk";  //!< Carla Chunk
const char* const CUSTOM_DATA_STRING  = "http://kxstudio.sf.net/ns/carla/string"; //!< Carla String
/**@}*/
#endif

#if 0
/*!
 * @defgroup BridgeMessages Bridge Messages
 *
 * Various bridge related messages, used as configure(<message>, value).
 * \note This is for internal use only.
 *
 * TODO: Review these, may not be needed anymore
 * @{
 */
const char* const CARLA_BRIDGE_MSG_HIDE_GUI   = "CarlaBridgeHideGUI";   //!< Plugin -> Host call, tells host GUI is now hidden
const char* const CARLA_BRIDGE_MSG_SAVED      = "CarlaBridgeSaved";     //!< Plugin -> Host call, tells host state is saved
const char* const CARLA_BRIDGE_MSG_SAVE_NOW   = "CarlaBridgeSaveNow";   //!< Host -> Plugin call, tells plugin to save state now
const char* const CARLA_BRIDGE_MSG_SET_CHUNK  = "CarlaBridgeSetChunk";  //!< Host -> Plugin call, tells plugin to set chunk in file \a value
const char* const CARLA_BRIDGE_MSG_SET_CUSTOM = "CarlaBridgeSetCustom"; //!< Host -> Plugin call, tells plugin to set a custom data set using \a value ("type·key·rvalue").\n If \a type is 'chunk' or 'binary' \a rvalue refers to chunk file.
/**@}*/
#endif

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
#ifndef BUILD_BRIDGE
    PLUGIN_INTERNAL = 1, //!< Internal plugin.\see NativePlugin
#endif
    PLUGIN_LADSPA   = 2, //!< LADSPA plugin.\see LadspaPlugin
    PLUGIN_DSSI     = 3, //!< DSSI plugin.\see DssiPlugin
    PLUGIN_LV2      = 4, //!< LV2 plugin.\see Lv2Plugin
    PLUGIN_VST      = 5, //!< VST plugin.\see VstPlugin
#ifndef BUILD_BRIDGE
    PLUGIN_GIG      = 6, //!< GIG sound kit, implemented via LinuxSampler.\see LinuxSamplerPlugin
    PLUGIN_SF2      = 7, //!< SF2 sound kit (aka SoundFont), implemented via FluidSynth.\see FluidSynthPlugin
    PLUGIN_SFZ      = 8  //!< SFZ sound kit, implemented via LinuxSampler.\see LinuxSamplerPlugin
#endif
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
    PARAMETER_MAX           = -8  //!< Max value, defined for convenience
};

/*!
 * Options used in the CarlaEngine::setOption() and set_option() calls.\n
 * These options must be set before initiliazing or after closing the engine.
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
     * Force mono plugins as stereo, by running 2 instances at the same time.
     * \note Not supported by all plugins.
     */
    OPTION_FORCE_STEREO = 2,

    /*!
     * Use plugin bridges whenever possible.\n
     * Default is no, and not recommended at this point!.
     * EXPERIMENTAL AND INCOMPLETE!
     */
    OPTION_PREFER_PLUGIN_BRIDGES = 3,

    /*!
     * Use OSC-UI bridges whenever possible, otherwise UIs will be handled in the main thread.\n
     * Default is yes.
     */
    OPTION_PREFER_UI_BRIDGES = 4,

#ifdef WANT_DSSI
    /*!
     * Use (unofficial) dssi-vst chunks feature.\n
     * Default is no.
     */
    OPTION_USE_DSSI_VST_CHUNKS = 5,
#endif

    /*!
     * Maximum number of parameters allowed.\n
     * Default is MAX_DEFAULT_PARAMETERS.
     */
    OPTION_MAX_PARAMETERS = 6,

    /*!
     * Timeout value in ms for how much to wait for OSC-Bridges to respond.\n
     * Default is 4000 (4 secs).
     */
    OPTION_OSC_UI_TIMEOUT = 7,

    /*!
     * Prefered buffer size.
     */
    OPTION_PREFERRED_BUFFER_SIZE = 8,

    /*!
     * Prefered sample rate.
     */
    OPTION_PREFERRED_SAMPLE_RATE = 9,

#ifndef BUILD_BRIDGE
    /*!
     * Set path to the native plugin bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_NATIVE = 10,

    /*!
     * Set path to the POSIX 32bit plugin bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_POSIX32 = 11,

    /*!
     * Set path to the POSIX 64bit plugin bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_POSIX64 = 12,

    /*!
     * Set path to the Windows 32bit plugin bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_WIN32 = 13,

    /*!
     * Set path to the Windows 64bit plugin bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_WIN64 = 14,
#endif

#ifdef WANT_LV2
    /*!
     * Set path to the LV2 Gtk2 UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_GTK2 = 15,

    /*!
     * Set path to the LV2 Gtk3 UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_GTK3 = 16,

    /*!
     * Set path to the LV2 Qt4 UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_QT4 = 17,

    /*!
     * Set path to the LV2 Qt5 UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_QT5 = 18,

    /*!
     * Set path to the LV2 Cocoa UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_COCOA = 19,

    /*!
     * Set path to the LV2 Windows UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_WINDOWS = 20,

    /*!
     * Set path to the LV2 X11 UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_LV2_X11 = 21,
#endif

#ifdef WANT_VST
    /*!
     * Set path to the VST Cocoa UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_VST_COCOA = 22,

    /*!
     * Set path to the VST HWND UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_VST_HWND = 23,

    /*!
     * Set path to the VST X11 UI bridge executable.\n
     * Default unset.
     */
    OPTION_PATH_BRIDGE_VST_X11 = 24
#endif
};

/*!
 * Opcodes sent from the engine callback to the GUI, as defined by CallbackFunc.
 *
 * \see CarlaEngine::setCallback()
 * \see set_callback_function()
 */
enum CallbackType {
    /*!
     * Debug.\n
     * This opcode is undefined and used only for testing purposes.
     */
    CALLBACK_DEBUG = 0,

    /*!
     * A parameter has been changed.
     *
     * \param value1 Parameter index
     * \param value3 Value
     */
    CALLBACK_PARAMETER_VALUE_CHANGED = 1,

    /*!
     * A parameter's MIDI channel has been changed.
     *
     * \param value1 Parameter index
     * \param value2 MIDI channel
     */
    CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED = 2,

    /*!
     * A parameter's MIDI CC has been changed.
     *
     * \param value1 Parameter index
     * \param value2 MIDI CC
     */
    CALLBACK_PARAMETER_MIDI_CC_CHANGED = 3,

    /*!
     * The current program has has been changed.
     *
     * \param value1 Program index
     */
    CALLBACK_PROGRAM_CHANGED = 4,

    /*!
     * The current MIDI program has been changed.
     *
     * \param value1 MIDI bank
     * \param value2 MIDI program
     */
    CALLBACK_MIDI_PROGRAM_CHANGED = 5,

    /*!
     * A note has been pressed.
     *
     * \param value1 Channel
     * \param value2 Note
     * \param value3 Velocity
     */
    CALLBACK_NOTE_ON = 6,

    /*!
     * A note has been released.
     *
     * \param value1 Channel
     * \param value2 Note
     */
    CALLBACK_NOTE_OFF = 7,

    /*!
     * The plugin's custom GUI state has changed.
     *
     * \param value1 State, as follows:.\n
     *                0: GUI has been closed or hidden\n
     *                1: GUI has been shown\n
     *               -1: GUI has crashed and should not be shown again
     */
    CALLBACK_SHOW_GUI = 8,

    /*!
     * The plugin needs update.
     */
    CALLBACK_UPDATE = 9,

    /*!
     * The plugin's data/information has changed.
     */
    CALLBACK_RELOAD_INFO = 10,

    /*!
     * The plugin's parameters have changed.
     */
    CALLBACK_RELOAD_PARAMETERS = 11,

    /*!
     * The plugin's programs have changed.
     */
    CALLBACK_RELOAD_PROGRAMS = 12,

    /*!
     * The plugin's state has changed.
     */
    CALLBACK_RELOAD_ALL = 13,

    /*!
     * Non-Session-Manager Announce message.
     */
    CALLBACK_NSM_ANNOUNCE = 14,

    /*!
     * Non-Session-Manager Open message #1.
     */
    CALLBACK_NSM_OPEN1 = 15,

    /*!
     * Non-Session-Manager Open message #2.
     */
    CALLBACK_NSM_OPEN2 = 16,

    /*!
     * Non-Session-Manager Save message.
     */
    CALLBACK_NSM_SAVE = 17,

    /*!
     * An error occurred, show last error to user.
     */
    CALLBACK_ERROR = 18,

    /*!
     * The engine has crashed or malfunctioned and will no longer work.
     */
    CALLBACK_QUIT = 19
};

/*!
 * Engine process mode.
 *
 * \see OPTION_PROCESS_MODE
 */
enum ProcessMode {
    PROCESS_MODE_SINGLE_CLIENT    = 0, //!< Single client mode (dynamic input/outputs as needed by plugins)
    PROCESS_MODE_MULTIPLE_CLIENTS = 1, //!< Multiple client mode (1 master client + 1 client per plugin)
    PROCESS_MODE_CONTINUOUS_RACK  = 2, //!< Single client, 'rack' mode. Processes plugins in order of id, with forced stereo.
    PROCESS_MODE_PATCHBAY         = 3, //!< Single client, 'patchbay' mode.
    PROCESS_MODE_BRIDGE           = 4  //!< Special mode, used in plugin-bridges only. RT buffers come from shared memory in a separate host app.
};

/*!
 * Callback function the backend will call when something interesting happens.
 *
 * \see set_callback_function() and CallbackType
 */
typedef void (*CallbackFunc)(void* ptr, CallbackType action, int pluginId, int value1, int value2, double value3, const char* valueStr);

/*!
 * Parameter data
 */
struct ParameterData {
    ParameterType type;
    int32_t index;
    int32_t rindex;
    int32_t hints;
    uint8_t midiChannel;
    int16_t midiCC;

    ParameterData()
        : type(PARAMETER_UNKNOWN),
          index(-1),
          rindex(-1),
          hints(0),
          midiChannel(0),
          midiCC(-1) {}
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

    ParameterRanges()
        : def(0.0f),
          min(0.0f),
          max(1.0f),
          step(0.01f),
          stepSmall(0.0001f),
          stepLarge(0.1f) {}

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
};

/*!
 * MIDI Program data
 */
struct MidiProgramData {
    uint32_t bank;
    uint32_t program;
    const char* name;

    MidiProgramData()
        : bank(0),
          program(0),
          name(nullptr) {}

    ~MidiProgramData()
    {
        std::free((void*)name);
    }
};

/*!
 * Custom data, saving key:value 'dictionaries'.
 * \a type is an URI which defines the \a value type.
 *
 * \see CustomDataTypes
 */
struct CustomData {
    const char* type;
    const char* key;
    const char* value;

    CustomData()
        : type(nullptr),
          key(nullptr),
          value(nullptr) {}

    ~CustomData()
    {
        std::free((void*)type);
        std::free((void*)key);
        std::free((void*)value);
    }
};

/**@}*/

// forward declarations of commonly used Carla classes
class CarlaEngine;
class CarlaPlugin;

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_BACKEND_HPP__
