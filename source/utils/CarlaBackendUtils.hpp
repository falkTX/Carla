/*
 * Carla Backend utils
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_BACKEND_UTILS_HPP_INCLUDED
#define CARLA_BACKEND_UTILS_HPP_INCLUDED

#include "CarlaBackend.h"
#include "CarlaString.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

static inline
const char* PluginOption2Str(const uint option) noexcept
{
    switch (option)
    {
    case PLUGIN_OPTION_FIXED_BUFFERS:
        return "PLUGIN_OPTION_FIXED_BUFFERS";
    case PLUGIN_OPTION_FORCE_STEREO:
        return "PLUGIN_OPTION_FORCE_STEREO";
    case PLUGIN_OPTION_MAP_PROGRAM_CHANGES:
        return "PLUGIN_OPTION_MAP_PROGRAM_CHANGES";
    case PLUGIN_OPTION_USE_CHUNKS:
        return "PLUGIN_OPTION_USE_CHUNKS";
    case PLUGIN_OPTION_SEND_CONTROL_CHANGES:
        return "PLUGIN_OPTION_SEND_CONTROL_CHANGES";
    case PLUGIN_OPTION_SEND_CHANNEL_PRESSURE:
        return "PLUGIN_OPTION_SEND_CHANNEL_PRESSURE";
    case PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH:
        return "PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH";
    case PLUGIN_OPTION_SEND_PITCHBEND:
        return "PLUGIN_OPTION_SEND_PITCHBEND";
    case PLUGIN_OPTION_SEND_ALL_SOUND_OFF:
        return "PLUGIN_OPTION_SEND_ALL_SOUND_OFF";
    }

    carla_stderr("CarlaBackend::PluginOption2Str(%i) - invalid option", option);
    return "";
}

// -----------------------------------------------------------------------

static inline
const char* BinaryType2Str(const BinaryType type) noexcept
{
    switch (type)
    {
    case BINARY_NONE:
        return "BINARY_NONE";
    case BINARY_POSIX32:
        return "BINARY_POSIX32";
    case BINARY_POSIX64:
        return "BINARY_POSIX64";
    case BINARY_WIN32:
        return "BINARY_WIN32";
    case BINARY_WIN64:
        return "BINARY_WIN64";
    case BINARY_OTHER:
        return "BINARY_OTHER";
    }

    carla_stderr("CarlaBackend::BinaryType2Str(%i) - invalid type", type);
    return "";
}

static inline
const char* FileType2Str(const FileType type) noexcept
{
    switch (type)
    {
    case FILE_NONE:
        return "FILE_NONE";
    case FILE_AUDIO:
        return "FILE_AUDIO";
    case FILE_MIDI:
        return "FILE_MIDI";
    }

    carla_stderr("CarlaBackend::FileType2Str(%i) - invalid type", type);
    return "";
}

static inline
const char* PluginType2Str(const PluginType type) noexcept
{
    switch (type)
    {
    case PLUGIN_NONE:
        return "PLUGIN_NONE";
    case PLUGIN_INTERNAL:
        return "PLUGIN_INTERNAL";
    case PLUGIN_LADSPA:
        return "PLUGIN_LADSPA";
    case PLUGIN_DSSI:
        return "PLUGIN_DSSI";
    case PLUGIN_LV2:
        return "PLUGIN_LV2";
    case PLUGIN_VST2:
        return "PLUGIN_VST2";
    case PLUGIN_VST3:
        return "PLUGIN_VST3";
    case PLUGIN_AU:
        return "PLUGIN_AU";
    case PLUGIN_DLS:
        return "PLUGIN_DLS";
    case PLUGIN_GIG:
        return "PLUGIN_GIG";
    case PLUGIN_SF2:
        return "PLUGIN_SF2";
    case PLUGIN_SFZ:
        return "PLUGIN_SFZ";
    case PLUGIN_JACK:
        return "PLUGIN_JACK";
    case PLUGIN_JSFX:
        return "PLUGIN_JSFX";
    case PLUGIN_CLAP:
        return "PLUGIN_CLAP";
    case PLUGIN_TYPE_COUNT:
        break;
    }

    carla_stderr("CarlaBackend::PluginType2Str(%i) - invalid type", type);
    return "";
}

static inline
const char* PluginCategory2Str(const PluginCategory category) noexcept
{
    switch (category)
    {
    case PLUGIN_CATEGORY_NONE:
        return "PLUGIN_CATEGORY_NONE";
    case PLUGIN_CATEGORY_SYNTH:
        return "PLUGIN_CATEGORY_SYNTH";
    case PLUGIN_CATEGORY_DELAY:
        return "PLUGIN_CATEGORY_DELAY";
    case PLUGIN_CATEGORY_EQ:
        return "PLUGIN_CATEGORY_EQ";
    case PLUGIN_CATEGORY_FILTER:
        return "PLUGIN_CATEGORY_FILTER";
    case PLUGIN_CATEGORY_DISTORTION:
        return "PLUGIN_CATEGORY_DISTORTION";
    case PLUGIN_CATEGORY_DYNAMICS:
        return "PLUGIN_CATEGORY_DYNAMICS";
    case PLUGIN_CATEGORY_MODULATOR:
        return "PLUGIN_CATEGORY_MODULATOR";
    case PLUGIN_CATEGORY_UTILITY:
        return "PLUGIN_CATEGORY_UTILITY";
    case PLUGIN_CATEGORY_OTHER:
        return "PLUGIN_CATEGORY_OTHER";
    }

    carla_stderr("CarlaBackend::PluginCategory2Str(%i) - invalid category", category);
    return "";
}

static inline
const char* ParameterType2Str(const ParameterType type) noexcept
{
    switch (type)
    {
    case PARAMETER_UNKNOWN:
        return "PARAMETER_UNKNOWN";
    case PARAMETER_INPUT:
        return "PARAMETER_INPUT";
    case PARAMETER_OUTPUT:
        return "PARAMETER_OUTPUT";
    }

    carla_stderr("CarlaBackend::ParameterType2Str(%i) - invalid type", type);
    return "";
}

static inline
const char* InternalParameterIndex2Str(const InternalParameterIndex index) noexcept
{
    switch (index)
    {
    case PARAMETER_NULL:
        return "PARAMETER_NULL";
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    case PARAMETER_ACTIVE:
        return "PARAMETER_ACTIVE";
    case PARAMETER_DRYWET:
        return "PARAMETER_DRYWET";
    case PARAMETER_VOLUME:
        return "PARAMETER_VOLUME";
    case PARAMETER_BALANCE_LEFT:
        return "PARAMETER_BALANCE_LEFT";
    case PARAMETER_BALANCE_RIGHT:
        return "PARAMETER_BALANCE_RIGHT";
    case PARAMETER_PANNING:
        return "PARAMETER_PANNING";
    case PARAMETER_CTRL_CHANNEL:
        return "PARAMETER_CTRL_CHANNEL";
#endif
    case PARAMETER_MAX:
        return "PARAMETER_MAX";
    }

    carla_stderr("CarlaBackend::InternalParameterIndex2Str(%i) - invalid index", index);
    return "";
}

static inline
const char* EngineCallbackOpcode2Str(const EngineCallbackOpcode opcode) noexcept
{
    switch (opcode)
    {
    case ENGINE_CALLBACK_DEBUG:
        return "ENGINE_CALLBACK_DEBUG";
    case ENGINE_CALLBACK_PLUGIN_ADDED:
        return "ENGINE_CALLBACK_PLUGIN_ADDED";
    case ENGINE_CALLBACK_PLUGIN_REMOVED:
        return "ENGINE_CALLBACK_PLUGIN_REMOVED";
    case ENGINE_CALLBACK_PLUGIN_RENAMED:
        return "ENGINE_CALLBACK_PLUGIN_RENAMED";
    case ENGINE_CALLBACK_PLUGIN_UNAVAILABLE:
        return "ENGINE_CALLBACK_PLUGIN_UNAVAILABLE";
    case ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED:
        return "ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED";
    case ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED:
        return "ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED";
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    case ENGINE_CALLBACK_PARAMETER_MAPPED_CONTROL_INDEX_CHANGED:
        return "ENGINE_CALLBACK_PARAMETER_MAPPED_CONTROL_INDEX_CHANGED";
    case ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
        return "ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED";
    case ENGINE_CALLBACK_OPTION_CHANGED:
        return "ENGINE_CALLBACK_OPTION_CHANGED";
#endif
    case ENGINE_CALLBACK_PROGRAM_CHANGED:
        return "ENGINE_CALLBACK_PROGRAM_CHANGED";
    case ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED:
        return "ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED";
    case ENGINE_CALLBACK_UI_STATE_CHANGED:
        return "ENGINE_CALLBACK_UI_STATE_CHANGED";
    case ENGINE_CALLBACK_NOTE_ON:
        return "ENGINE_CALLBACK_NOTE_ON";
    case ENGINE_CALLBACK_NOTE_OFF:
        return "ENGINE_CALLBACK_NOTE_OFF";
    case ENGINE_CALLBACK_UPDATE:
        return "ENGINE_CALLBACK_UPDATE";
    case ENGINE_CALLBACK_RELOAD_INFO:
        return "ENGINE_CALLBACK_RELOAD_INFO";
    case ENGINE_CALLBACK_RELOAD_PARAMETERS:
        return "ENGINE_CALLBACK_RELOAD_PARAMETERS";
    case ENGINE_CALLBACK_RELOAD_PROGRAMS:
        return "ENGINE_CALLBACK_RELOAD_PROGRAMS";
    case ENGINE_CALLBACK_RELOAD_ALL:
        return "ENGINE_CALLBACK_RELOAD_ALL";
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    case ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED:
        return "ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED";
    case ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED:
        return "ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED";
    case ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED:
        return "ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED";
    case ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED:
        return "ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED";
    case ENGINE_CALLBACK_PATCHBAY_PORT_ADDED:
        return "ENGINE_CALLBACK_PATCHBAY_PORT_ADDED";
    case ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED:
        return "ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED";
    case ENGINE_CALLBACK_PATCHBAY_PORT_CHANGED:
        return "ENGINE_CALLBACK_PATCHBAY_PORT_CHANGED";
    case ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED:
        return "ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED";
    case ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED:
        return "ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED";
#endif
    case ENGINE_CALLBACK_ENGINE_STARTED:
        return "ENGINE_CALLBACK_ENGINE_STARTED";
    case ENGINE_CALLBACK_ENGINE_STOPPED:
        return "ENGINE_CALLBACK_ENGINE_STOPPED";
    case ENGINE_CALLBACK_PROCESS_MODE_CHANGED:
        return "ENGINE_CALLBACK_PROCESS_MODE_CHANGED";
    case ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED:
        return "ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED";
    case ENGINE_CALLBACK_BUFFER_SIZE_CHANGED:
        return "ENGINE_CALLBACK_BUFFER_SIZE_CHANGED";
    case ENGINE_CALLBACK_SAMPLE_RATE_CHANGED:
        return "ENGINE_CALLBACK_SAMPLE_RATE_CHANGED";
    case ENGINE_CALLBACK_CANCELABLE_ACTION:
        return "ENGINE_CALLBACK_CANCELABLE_ACTION";
    case ENGINE_CALLBACK_PROJECT_LOAD_FINISHED:
        return "ENGINE_CALLBACK_PROJECT_LOAD_FINISHED";
    case ENGINE_CALLBACK_NSM:
        return "ENGINE_CALLBACK_NSM";
    case ENGINE_CALLBACK_IDLE:
        return "ENGINE_CALLBACK_IDLE";
    case ENGINE_CALLBACK_INFO:
        return "ENGINE_CALLBACK_INFO";
    case ENGINE_CALLBACK_ERROR:
        return "ENGINE_CALLBACK_ERROR";
    case ENGINE_CALLBACK_QUIT:
        return "ENGINE_CALLBACK_QUIT";
    case ENGINE_CALLBACK_INLINE_DISPLAY_REDRAW:
        return "ENGINE_CALLBACK_INLINE_DISPLAY_REDRAW";
    case ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_ADDED:
        return "ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_ADDED";
    case ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_REMOVED:
        return "ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_REMOVED";
    case ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_CHANGED:
        return "ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_CHANGED";
    case ENGINE_CALLBACK_PARAMETER_MAPPED_RANGE_CHANGED:
        return "ENGINE_CALLBACK_PARAMETER_MAPPED_RANGE_CHANGED";
    case ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED:
        return "ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED";
    case ENGINE_CALLBACK_EMBED_UI_RESIZED:
        return "ENGINE_CALLBACK_EMBED_UI_RESIZED";
    }

    carla_stderr("CarlaBackend::EngineCallbackOpcode2Str(%i) - invalid opcode", opcode);
    return "";
}

static inline
const char* EngineOption2Str(const EngineOption option) noexcept
{
    switch (option)
    {
    case ENGINE_OPTION_DEBUG:
        return "ENGINE_OPTION_DEBUG";
    case ENGINE_OPTION_PROCESS_MODE:
        return "ENGINE_OPTION_PROCESS_MODE";
    case ENGINE_OPTION_TRANSPORT_MODE:
        return "ENGINE_OPTION_TRANSPORT_MODE";
    case ENGINE_OPTION_FORCE_STEREO:
        return "ENGINE_OPTION_FORCE_STEREO";
    case ENGINE_OPTION_PREFER_PLUGIN_BRIDGES:
        return "ENGINE_OPTION_PREFER_PLUGIN_BRIDGES";
    case ENGINE_OPTION_PREFER_UI_BRIDGES:
        return "ENGINE_OPTION_PREFER_UI_BRIDGES";
    case ENGINE_OPTION_UIS_ALWAYS_ON_TOP:
        return "ENGINE_OPTION_UIS_ALWAYS_ON_TOP";
    case ENGINE_OPTION_MAX_PARAMETERS:
        return "ENGINE_OPTION_MAX_PARAMETERS";
    case ENGINE_OPTION_RESET_XRUNS:
        return "ENGINE_OPTION_RESET_XRUNS";
    case ENGINE_OPTION_UI_BRIDGES_TIMEOUT:
        return "ENGINE_OPTION_UI_BRIDGES_TIMEOUT";
    case ENGINE_OPTION_AUDIO_BUFFER_SIZE:
        return "ENGINE_OPTION_AUDIO_BUFFER_SIZE";
    case ENGINE_OPTION_AUDIO_SAMPLE_RATE:
        return "ENGINE_OPTION_AUDIO_SAMPLE_RATE";
    case ENGINE_OPTION_AUDIO_TRIPLE_BUFFER:
        return "ENGINE_OPTION_AUDIO_TRIPLE_BUFFER";
    case ENGINE_OPTION_AUDIO_DRIVER:
        return "ENGINE_OPTION_AUDIO_DRIVER";
    case ENGINE_OPTION_AUDIO_DEVICE:
        return "ENGINE_OPTION_AUDIO_DEVICE";
#ifndef BUILD_BRIDGE
    case ENGINE_OPTION_OSC_ENABLED:
        return "ENGINE_OPTION_OSC_ENABLED";
    case ENGINE_OPTION_OSC_PORT_UDP:
        return "ENGINE_OPTION_OSC_PORT_UDP";
    case ENGINE_OPTION_OSC_PORT_TCP:
        return "ENGINE_OPTION_OSC_PORT_TCP";
#endif
    case ENGINE_OPTION_FILE_PATH:
        return "ENGINE_OPTION_FILE_PATH";
    case ENGINE_OPTION_PLUGIN_PATH:
        return "ENGINE_OPTION_PLUGIN_PATH";
    case ENGINE_OPTION_PATH_BINARIES:
        return "ENGINE_OPTION_PATH_BINARIES";
    case ENGINE_OPTION_PATH_RESOURCES:
        return "ENGINE_OPTION_PATH_RESOURCES";
    case ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR:
        return "ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR";
    case ENGINE_OPTION_FRONTEND_BACKGROUND_COLOR:
        return "ENGINE_OPTION_FRONTEND_BACKGROUND_COLOR";
    case ENGINE_OPTION_FRONTEND_FOREGROUND_COLOR:
        return "ENGINE_OPTION_FRONTEND_FOREGROUND_COLOR";
    case ENGINE_OPTION_FRONTEND_UI_SCALE:
        return "ENGINE_OPTION_FRONTEND_UI_SCALE";
    case ENGINE_OPTION_FRONTEND_WIN_ID:
        return "ENGINE_OPTION_FRONTEND_WIN_ID";
#if !defined(BUILD_BRIDGE_ALTERNATIVE_ARCH) && !defined(CARLA_OS_WIN)
    case ENGINE_OPTION_WINE_EXECUTABLE:
        return "ENGINE_OPTION_WINE_EXECUTABLE";
    case ENGINE_OPTION_WINE_AUTO_PREFIX:
        return "ENGINE_OPTION_WINE_AUTO_PREFIX";
    case ENGINE_OPTION_WINE_FALLBACK_PREFIX:
        return "ENGINE_OPTION_WINE_FALLBACK_PREFIX";
    case ENGINE_OPTION_WINE_RT_PRIO_ENABLED:
        return "ENGINE_OPTION_WINE_RT_PRIO_ENABLED";
    case ENGINE_OPTION_WINE_BASE_RT_PRIO:
        return "ENGINE_OPTION_WINE_BASE_RT_PRIO";
    case ENGINE_OPTION_WINE_SERVER_RT_PRIO:
        return "ENGINE_OPTION_WINE_SERVER_RT_PRIO";
#endif
#ifndef BUILD_BRIDGE
    case ENGINE_OPTION_DEBUG_CONSOLE_OUTPUT:
        return "ENGINE_OPTION_DEBUG_CONSOLE_OUTPUT";
#endif
    case ENGINE_OPTION_CLIENT_NAME_PREFIX:
        return "ENGINE_OPTION_CLIENT_NAME_PREFIX";
    case ENGINE_OPTION_PLUGINS_ARE_STANDALONE:
        return "ENGINE_OPTION_PLUGINS_ARE_STANDALONE";
    }

    carla_stderr("CarlaBackend::EngineOption2Str(%i) - invalid option", option);
    return "";
}

static inline
const char* EngineProcessMode2Str(const EngineProcessMode mode) noexcept
{
    switch (mode)
    {
    case ENGINE_PROCESS_MODE_SINGLE_CLIENT:
        return "ENGINE_PROCESS_MODE_SINGLE_CLIENT";
    case ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
        return "ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS";
    case ENGINE_PROCESS_MODE_CONTINUOUS_RACK:
        return "ENGINE_PROCESS_MODE_CONTINUOUS_RACK";
    case ENGINE_PROCESS_MODE_PATCHBAY:
        return "ENGINE_PROCESS_MODE_PATCHBAY";
    case ENGINE_PROCESS_MODE_BRIDGE:
        return "ENGINE_PROCESS_MODE_BRIDGE";
    }

    carla_stderr("CarlaBackend::EngineProcessMode2Str(%i) - invalid mode", mode);
    return "";
}

static inline
const char* EngineTransportMode2Str(const EngineTransportMode mode) noexcept
{
    switch (mode)
    {
    case ENGINE_TRANSPORT_MODE_DISABLED:
        return "ENGINE_TRANSPORT_MODE_DISABLED";
    case ENGINE_TRANSPORT_MODE_INTERNAL:
        return "ENGINE_TRANSPORT_MODE_INTERNAL";
    case ENGINE_TRANSPORT_MODE_JACK:
        return "ENGINE_TRANSPORT_MODE_JACK";
    case ENGINE_TRANSPORT_MODE_PLUGIN:
        return "ENGINE_TRANSPORT_MODE_PLUGIN";
    case ENGINE_TRANSPORT_MODE_BRIDGE:
        return "ENGINE_TRANSPORT_MODE_BRIDGE";
    }

    carla_stderr("CarlaBackend::EngineTransportMode2Str(%i) - invalid mode", mode);
    return "";
}

static inline
const char* FileCallbackOpcode2Str(const FileCallbackOpcode opcode) noexcept
{
    switch (opcode)
    {
    case FILE_CALLBACK_DEBUG:
        return "FILE_CALLBACK_DEBUG";
    case FILE_CALLBACK_OPEN:
        return "FILE_CALLBACK_OPEN";
    case FILE_CALLBACK_SAVE:
        return "FILE_CALLBACK_SAVE";
    }

    carla_stderr("CarlaBackend::FileCallbackOpcode2Str(%i) - invalid opcode", opcode);
    return "";
}

static inline
const char* PatchbayIcon2Str(const PatchbayIcon icon) noexcept
{
    switch (icon)
    {
    case PATCHBAY_ICON_APPLICATION:
        return "PATCHBAY_ICON_APPLICATION";
    case PATCHBAY_ICON_PLUGIN:
        return "PATCHBAY_ICON_PLUGIN";
    case PATCHBAY_ICON_HARDWARE:
        return "PATCHBAY_ICON_HARDWARE";
    case PATCHBAY_ICON_CARLA:
        return "PATCHBAY_ICON_CARLA";
    case PATCHBAY_ICON_DISTRHO:
        return "PATCHBAY_ICON_DISTRHO";
    case PATCHBAY_ICON_FILE:
        return "PATCHBAY_ICON_FILE";
    }

    carla_stderr("CarlaBackend::PatchbayIcon2Str(%i) - invalid icon", icon);
    return "";
}

// -----------------------------------------------------------------------

static inline
const char* getBinaryTypeAsString(const BinaryType type) noexcept
{
    carla_debug("CarlaBackend::getBinaryTypeAsString(%i:%s)", type, BinaryType2Str(type));

    if (type == BINARY_NATIVE)
        return "native";

    switch (type)
    {
    case BINARY_NONE:
        return "none";
    case BINARY_POSIX32:
        return "posix32";
    case BINARY_POSIX64:
        return "posix64";
    case BINARY_WIN32:
        return "win32";
    case BINARY_WIN64:
        return "win64";
    case BINARY_OTHER:
        return "other";
    }

    carla_stderr("CarlaBackend::getBinaryTypeAsString(%i) - invalid type", type);
    return "NONE";
}

static inline
BinaryType getBinaryTypeFromString(const char* const ctype) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(ctype != nullptr && ctype[0] != '\0', BINARY_NONE);
    carla_debug("CarlaBackend::getBinaryTypeFromString(\"%s\")", ctype);

    CarlaString stype(ctype);

    if (stype.isEmpty())
        return BINARY_NONE;

    stype.toLower();

    if (stype == "none")
        return BINARY_NONE;
    if (stype == "native")
        return BINARY_NATIVE;
    if (stype == "posix32" || stype == "linux32" || stype == "mac32")
        return BINARY_POSIX32;
    if (stype == "posix64" || stype == "linux64" || stype == "mac64")
        return BINARY_POSIX64;
    if (stype == "win32")
        return BINARY_WIN32;
    if (stype == "win64")
        return BINARY_WIN64;
    if (stype == "other")
        return BINARY_OTHER;

    carla_stderr("CarlaBackend::getBinaryTypeFromString(\"%s\") - invalid string type", ctype);
    return BINARY_NONE;
}

// -----------------------------------------------------------------------

static inline
const char* getPluginCategoryAsString(const PluginCategory category) noexcept
{
    carla_debug("CarlaBackend::getPluginCategoryAsString(%i:%s)", category, PluginCategory2Str(category));

    switch (category)
    {
    case PLUGIN_CATEGORY_NONE:
        return "none";
    case PLUGIN_CATEGORY_SYNTH:
        return "synth";
    case PLUGIN_CATEGORY_DELAY:
        return "delay";
    case PLUGIN_CATEGORY_EQ:
        return "eq";
    case PLUGIN_CATEGORY_FILTER:
        return "filter";
    case PLUGIN_CATEGORY_DISTORTION:
        return "distortion";
    case PLUGIN_CATEGORY_DYNAMICS:
        return "dynamics";
    case PLUGIN_CATEGORY_MODULATOR:
        return "modulator";
    case PLUGIN_CATEGORY_UTILITY:
        return "utility";
    case PLUGIN_CATEGORY_OTHER:
        return "other";
    }

    carla_stderr("CarlaBackend::getPluginCategoryAsString(%i) - invalid category", category);
    return "NONE";
}

static inline
PluginCategory getPluginCategoryFromString(const char* const category) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(category != nullptr && category[0] != '\0', PLUGIN_CATEGORY_NONE);
    carla_debug("CarlaBackend::getPluginCategoryFromString(\"%s\")", category);

    if (std::strcmp(category, "none") == 0)
        return PLUGIN_CATEGORY_NONE;
    if (std::strcmp(category, "synth") == 0)
        return PLUGIN_CATEGORY_SYNTH;
    if (std::strcmp(category, "delay") == 0)
        return PLUGIN_CATEGORY_DELAY;
    if (std::strcmp(category, "eq") == 0)
        return PLUGIN_CATEGORY_EQ;
    if (std::strcmp(category, "filter") == 0)
        return PLUGIN_CATEGORY_FILTER;
    if (std::strcmp(category, "distortion") == 0)
        return PLUGIN_CATEGORY_DISTORTION;
    if (std::strcmp(category, "dynamics") == 0)
        return PLUGIN_CATEGORY_DYNAMICS;
    if (std::strcmp(category, "modulator") == 0)
        return PLUGIN_CATEGORY_MODULATOR;
    if (std::strcmp(category, "utility") == 0)
        return PLUGIN_CATEGORY_UTILITY;
    if (std::strcmp(category, "other") == 0)
        return PLUGIN_CATEGORY_OTHER;

    carla_stderr("CarlaBackend::getPluginCategoryFromString(\"%s\") - invalid category", category);
    return PLUGIN_CATEGORY_NONE;
}

// -----------------------------------------------------------------------

static inline
const char* getPluginTypeAsString(const PluginType type) noexcept
{
    carla_debug("CarlaBackend::getPluginTypeAsString(%i:%s)", type, PluginType2Str(type));

    switch (type)
    {
    case PLUGIN_NONE:
        return "NONE";
    case PLUGIN_INTERNAL:
        return "INTERNAL";
    case PLUGIN_LADSPA:
        return "LADSPA";
    case PLUGIN_DSSI:
        return "DSSI";
    case PLUGIN_LV2:
        return "LV2";
    case PLUGIN_VST2:
        return "VST2";
    case PLUGIN_VST3:
        return "VST3";
    case PLUGIN_AU:
        return "AU";
    case PLUGIN_DLS:
        return "DLS";
    case PLUGIN_GIG:
        return "GIG";
    case PLUGIN_SF2:
        return "SF2";
    case PLUGIN_SFZ:
        return "SFZ";
    case PLUGIN_JACK:
        return "JACK";
    case PLUGIN_JSFX:
        return "JSFX";
    case PLUGIN_CLAP:
        return "CLAP";
    case PLUGIN_TYPE_COUNT:
        break;
    }

    carla_stderr("CarlaBackend::getPluginTypeAsString(%i) - invalid type", type);
    return "NONE";
}

static inline
PluginType getPluginTypeFromString(const char* const ctype) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(ctype != nullptr && ctype[0] != '\0', PLUGIN_NONE);
    carla_debug("CarlaBackend::getPluginTypeFromString(\"%s\")", ctype);

    CarlaString stype(ctype);

    if (stype.isEmpty())
        return PLUGIN_NONE;

    stype.toLower();

    if (stype == "none")
        return PLUGIN_NONE;
    if (stype == "internal" || stype == "native")
        return PLUGIN_INTERNAL;
    if (stype == "ladspa")
        return PLUGIN_LADSPA;
    if (stype == "dssi")
        return PLUGIN_DSSI;
    if (stype == "lv2")
        return PLUGIN_LV2;
    if (stype == "vst2" || stype == "vst")
        return PLUGIN_VST2;
    if (stype == "vst3")
        return PLUGIN_VST3;
    if (stype == "au" || stype == "audiounit")
        return PLUGIN_AU;
    if (stype == "dls")
        return PLUGIN_DLS;
    if (stype == "gig")
        return PLUGIN_GIG;
    if (stype == "sf2" || stype == "sf3")
        return PLUGIN_SF2;
    if (stype == "sfz")
        return PLUGIN_SFZ;
    if (stype == "jack")
        return PLUGIN_JACK;
    if (stype == "jsfx")
        return PLUGIN_JSFX;
    if (stype == "clap")
        return PLUGIN_CLAP;

    carla_stderr("CarlaBackend::getPluginTypeFromString(\"%s\") - invalid string type", ctype);
    return PLUGIN_NONE;
}

// -----------------------------------------------------------------------

static inline
PluginCategory getPluginCategoryFromName(const char* const name) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', PLUGIN_CATEGORY_NONE);
    carla_debug("CarlaBackend::getPluginCategoryFromName(\"%s\")", name);

    CarlaString sname(name);

    if (sname.isEmpty())
        return PLUGIN_CATEGORY_NONE;

    sname.toLower();

    // generic tags first
    if (sname.contains("delay"))
        return PLUGIN_CATEGORY_DELAY;
    if (sname.contains("reverb"))
        return PLUGIN_CATEGORY_DELAY;

    // filter
    if (sname.contains("filter"))
        return PLUGIN_CATEGORY_FILTER;

    // distortion
    if (sname.contains("distortion"))
        return PLUGIN_CATEGORY_DISTORTION;

    // dynamics
    if (sname.contains("dynamics"))
        return PLUGIN_CATEGORY_DYNAMICS;
    if (sname.contains("amplifier"))
        return PLUGIN_CATEGORY_DYNAMICS;
    if (sname.contains("compressor"))
        return PLUGIN_CATEGORY_DYNAMICS;
    if (sname.contains("enhancer"))
        return PLUGIN_CATEGORY_DYNAMICS;
    if (sname.contains("exciter"))
        return PLUGIN_CATEGORY_DYNAMICS;
    if (sname.contains("gate"))
        return PLUGIN_CATEGORY_DYNAMICS;
    if (sname.contains("limiter"))
        return PLUGIN_CATEGORY_DYNAMICS;

    // modulator
    if (sname.contains("modulator"))
        return PLUGIN_CATEGORY_MODULATOR;
    if (sname.contains("chorus"))
        return PLUGIN_CATEGORY_MODULATOR;
    if (sname.contains("flanger"))
        return PLUGIN_CATEGORY_MODULATOR;
    if (sname.contains("phaser"))
        return PLUGIN_CATEGORY_MODULATOR;
    if (sname.contains("saturator"))
        return PLUGIN_CATEGORY_MODULATOR;

    // utility
    if (sname.contains("utility"))
        return PLUGIN_CATEGORY_UTILITY;
    if (sname.contains("analyzer"))
        return PLUGIN_CATEGORY_UTILITY;
    if (sname.contains("converter"))
        return PLUGIN_CATEGORY_UTILITY;
    if (sname.contains("deesser"))
        return PLUGIN_CATEGORY_UTILITY;
    if (sname.contains("mixer"))
        return PLUGIN_CATEGORY_UTILITY;

    // common tags
    if (sname.contains("verb"))
        return PLUGIN_CATEGORY_DELAY;

    if (sname.contains("eq"))
        return PLUGIN_CATEGORY_EQ;

    if (sname.contains("tool"))
        return PLUGIN_CATEGORY_UTILITY;

    // synth
    if (sname.contains("synth"))
        return PLUGIN_CATEGORY_SYNTH;

    // other
    if (sname.contains("misc"))
        return PLUGIN_CATEGORY_OTHER;
    if (sname.contains("other"))
        return PLUGIN_CATEGORY_OTHER;

    return PLUGIN_CATEGORY_NONE;
}

// -----------------------------------------------------------------------

static inline
bool isPluginOptionEnabled(const uint options, const uint option)
{
    if (options == PLUGIN_OPTIONS_NULL)
        return true;
    if (options & option)
        return true;
    return false;
}

static inline
bool isPluginOptionInverseEnabled(const uint options, const uint option)
{
    if (options == PLUGIN_OPTIONS_NULL)
        return false;
    if (options & option)
        return true;
    return false;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_BACKEND_UTILS_HPP_INCLUDED
