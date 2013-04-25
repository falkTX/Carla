/*
 * Carla Backend utils
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

#ifndef __CARLA_BACKEND_UTILS_HPP__
#define __CARLA_BACKEND_UTILS_HPP__

#include "CarlaBackend.hpp"
#include "CarlaString.hpp"

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------

static inline
const char* PluginOption2Str(const unsigned int& option)
{
    switch (option)
    {
    case PLUGIN_OPTION_FIXED_BUFFER:
        return "PLUGIN_OPTION_FIXED_BUFFER";
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

    carla_stderr("CarlaBackend::PluginOption2Str(%i) - invalid type", option);
    return nullptr;
}

// -------------------------------------------------

static inline
const char* BinaryType2Str(const BinaryType& type)
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
    return nullptr;
}

static inline
const char* PluginType2Str(const PluginType& type)
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
    case PLUGIN_VST:
        return "PLUGIN_VST";
    case PLUGIN_VST3:
        return "PLUGIN_VST3";
    case PLUGIN_GIG:
        return "PLUGIN_GIG";
    case PLUGIN_SF2:
        return "PLUGIN_SF2";
    case PLUGIN_SFZ:
        return "PLUGIN_SFZ";
    }

    carla_stderr("CarlaBackend::PluginType2Str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* PluginCategory2Str(const PluginCategory& category)
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
    return nullptr;
}

static inline
const char* ParameterType2Str(const ParameterType& type)
{
    switch (type)
    {
    case PARAMETER_UNKNOWN:
        return "PARAMETER_UNKNOWN";
    case PARAMETER_INPUT:
        return "PARAMETER_INPUT";
    case PARAMETER_OUTPUT:
        return "PARAMETER_OUTPUT";
    case PARAMETER_LATENCY:
        return "PARAMETER_LATENCY";
    case PARAMETER_SAMPLE_RATE:
        return "PARAMETER_SAMPLE_RATE";
#ifdef WANT_LV2
    case PARAMETER_LV2_FREEWHEEL:
        return "PARAMETER_LV2_FREEWHEEL";
    case PARAMETER_LV2_TIME:
        return "PARAMETER_LV2_TIME";
#endif
    }

    carla_stderr("CarlaBackend::ParameterType2Str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* InternalParametersIndex2Str(const InternalParametersIndex& index)
{
    switch (index)
    {
    case PARAMETER_NULL:
        return "PARAMETER_NULL";
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
    case PARAMETER_MAX:
        return "PARAMETER_MAX";
    }

    carla_stderr("CarlaBackend::InternalParametersIndex2Str(%i) - invalid index", index);
    return nullptr;
}

static inline
const char* OptionsType2Str(const OptionsType& type)
{
    switch (type)
    {
    case OPTION_PROCESS_NAME:
        return "OPTION_PROCESS_NAME";
    case OPTION_PROCESS_MODE:
        return "OPTION_PROCESS_MODE";
    case OPTION_TRANSPORT_MODE:
        return "OPTION_TRANSPORT_MODE";
    case OPTION_FORCE_STEREO:
        return "OPTION_FORCE_STEREO";
    case OPTION_PREFER_PLUGIN_BRIDGES:
        return "OPTION_PREFER_PLUGIN_BRIDGES";
    case OPTION_PREFER_UI_BRIDGES:
        return "OPTION_PREFER_UI_BRIDGES";
#ifdef WANT_DSSI
    case OPTION_USE_DSSI_VST_CHUNKS:
        return "OPTION_USE_DSSI_VST_CHUNKS";
#endif
    case OPTION_MAX_PARAMETERS:
        return "OPTION_MAX_PARAMETERS";
    case OPTION_OSC_UI_TIMEOUT:
        return "OPTION_OSC_UI_TIMEOUT";
    case OPTION_JACK_AUTOCONNECT:
        return "OPTION_JACK_AUTOCONNECT";
    case OPTION_JACK_TIMEMASTER:
        return "OPTION_JACK_TIMEMASTER";
#ifdef WANT_RTAUDIO
    case OPTION_RTAUDIO_BUFFER_SIZE:
        return "OPTION_RTAUDIO_BUFFER_SIZE:";
    case OPTION_RTAUDIO_SAMPLE_RATE:
        return "OPTION_RTAUDIO_SAMPLE_RATE";
    case OPTION_RTAUDIO_DEVICE:
        return "OPTION_RTAUDIO_DEVICE";
#endif
#ifndef BUILD_BRIDGE
    case OPTION_PATH_BRIDGE_NATIVE:
        return "OPTION_PATH_BRIDGE_NATIVE";
    case OPTION_PATH_BRIDGE_POSIX32:
        return "OPTION_PATH_BRIDGE_POSIX32";
    case OPTION_PATH_BRIDGE_POSIX64:
        return "OPTION_PATH_BRIDGE_POSIX64";
    case OPTION_PATH_BRIDGE_WIN32:
        return "OPTION_PATH_BRIDGE_WIN32";
    case OPTION_PATH_BRIDGE_WIN64:
        return "OPTION_PATH_BRIDGE_WIN64";
#endif
#ifdef WANT_LV2
    case OPTION_PATH_BRIDGE_LV2_GTK2:
        return "OPTION_PATH_BRIDGE_LV2_GTK2";
    case OPTION_PATH_BRIDGE_LV2_GTK3:
        return "OPTION_PATH_BRIDGE_LV2_GTK3";
    case OPTION_PATH_BRIDGE_LV2_QT4:
        return "OPTION_PATH_BRIDGE_LV2_QT4";
    case OPTION_PATH_BRIDGE_LV2_QT5:
        return "OPTION_PATH_BRIDGE_LV2_QT5";
    case OPTION_PATH_BRIDGE_LV2_COCOA:
        return "OPTION_PATH_BRIDGE_LV2_COCOA";
    case OPTION_PATH_BRIDGE_LV2_WINDOWS:
        return "OPTION_PATH_BRIDGE_LV2_WINDOWS";
    case OPTION_PATH_BRIDGE_LV2_X11:
        return "OPTION_PATH_BRIDGE_LV2_X11";
#endif
#ifdef WANT_VST
    case OPTION_PATH_BRIDGE_VST_COCOA:
        return "OPTION_PATH_BRIDGE_VST_COCOA";
    case OPTION_PATH_BRIDGE_VST_HWND:
        return "OPTION_PATH_BRIDGE_VST_HWND";
    case OPTION_PATH_BRIDGE_VST_X11:
        return "OPTION_PATH_BRIDGE_VST_X11";
#endif
    }

    carla_stderr("CarlaBackend::OptionsType2Str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* CallbackType2Str(const CallbackType& type)
{
    switch (type)
    {
    case CALLBACK_DEBUG:
        return "CALLBACK_DEBUG";
    case CALLBACK_PLUGIN_ADDED:
        return "CALLBACK_PLUGIN_ADDED";
    case CALLBACK_PLUGIN_REMOVED:
        return "CALLBACK_PLUGIN_REMOVED";
    case CALLBACK_PLUGIN_RENAMED:
        return "CALLBACK_PLUGIN_RENAMED";
    case CALLBACK_PARAMETER_VALUE_CHANGED:
        return "CALLBACK_PARAMETER_VALUE_CHANGED";
    case CALLBACK_PARAMETER_DEFAULT_CHANGED:
        return "CALLBACK_PARAMETER_DEFAULT_CHANGED";
    case CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
        return "CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED";
    case CALLBACK_PARAMETER_MIDI_CC_CHANGED:
        return "CALLBACK_PARAMETER_MIDI_CC_CHANGED";
    case CALLBACK_PROGRAM_CHANGED:
        return "CALLBACK_PROGRAM_CHANGED";
    case CALLBACK_MIDI_PROGRAM_CHANGED:
        return "CALLBACK_MIDI_PROGRAM_CHANGED";
    case CALLBACK_NOTE_ON:
        return "CALLBACK_NOTE_ON";
    case CALLBACK_NOTE_OFF:
        return "CALLBACK_NOTE_OFF";
    case CALLBACK_SHOW_GUI:
        return "CALLBACK_SHOW_GUI";
    case CALLBACK_UPDATE:
        return "CALLBACK_UPDATE";
    case CALLBACK_PATCHBAY_CLIENT_ADDED:
        return "CALLBACK_PATCHBAY_CLIENT_ADDED";
    case CALLBACK_PATCHBAY_CLIENT_REMOVED:
        return "CALLBACK_PATCHBAY_CLIENT_REMOVED";
    case CALLBACK_PATCHBAY_CLIENT_RENAMED:
        return "CALLBACK_PATCHBAY_CLIENT_RENAMED";
    case CALLBACK_PATCHBAY_PORT_ADDED:
        return "CALLBACK_PATCHBAY_PORT_ADDED";
    case CALLBACK_PATCHBAY_PORT_REMOVED:
        return "CALLBACK_PATCHBAY_PORT_REMOVED";
    case CALLBACK_PATCHBAY_PORT_RENAMED:
        return "CALLBACK_PATCHBAY_PORT_RENAMED";
    case CALLBACK_PATCHBAY_CONNECTION_ADDED:
        return "CALLBACK_PATCHBAY_CONNECTION_ADDED";
    case CALLBACK_PATCHBAY_CONNECTION_REMOVED:
        return "CALLBACK_PATCHBAY_CONNECTION_REMOVED";
    case CALLBACK_RELOAD_INFO:
        return "CALLBACK_RELOAD_INFO";
    case CALLBACK_RELOAD_PARAMETERS:
        return "CALLBACK_RELOAD_PARAMETERS";
    case CALLBACK_RELOAD_PROGRAMS:
        return "CALLBACK_RELOAD_PROGRAMS";
    case CALLBACK_RELOAD_ALL:
        return "CALLBACK_RELOAD_ALL";
    case CALLBACK_BUFFER_SIZE_CHANGED:
        return "CALLBACK_BUFFER_SIZE_CHANGED";
    case CALLBACK_SAMPLE_RATE_CHANGED:
        return "CALLBACK_SAMPLE_RATE_CHANGED";
    case CALLBACK_NSM_ANNOUNCE:
        return "CALLBACK_NSM_ANNOUNCE";
    case CALLBACK_NSM_OPEN:
        return "CALLBACK_NSM_OPEN";
    case CALLBACK_NSM_SAVE:
        return "CALLBACK_NSM_SAVE";
    case CALLBACK_ERROR:
        return "CALLBACK_ERROR";
    case CALLBACK_QUIT:
        return "CALLBACK_QUIT";
    }

    carla_stderr("CarlaBackend::CallbackType2Str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* ProcessMode2Str(const ProcessMode& mode)
{
    switch (mode)
    {
    case PROCESS_MODE_SINGLE_CLIENT:
        return "PROCESS_MODE_SINGLE_CLIENT";
    case PROCESS_MODE_MULTIPLE_CLIENTS:
        return "PROCESS_MODE_MULTIPLE_CLIENTS";
    case PROCESS_MODE_CONTINUOUS_RACK:
        return "PROCESS_MODE_CONTINUOUS_RACK";
    case PROCESS_MODE_PATCHBAY:
        return "PROCESS_MODE_PATCHBAY";
    case PROCESS_MODE_BRIDGE:
        return "PROCESS_MODE_BRIDGE";
    }

    carla_stderr("CarlaBackend::ProcessMode2Str(%i) - invalid type", mode);
    return nullptr;
}

static inline
const char* TransportMode2Str(const TransportMode& mode)
{
    switch (mode)
    {
    case TRANSPORT_MODE_INTERNAL:
        return "TRANSPORT_MODE_INTERNAL";
    case TRANSPORT_MODE_JACK:
        return "TRANSPORT_MODE_JACK";
    case TRANSPORT_MODE_BRIDGE:
        return "TRANSPORT_MODE_BRIDGE";
    }

    carla_stderr("CarlaBackend::TransportMode2Str(%i) - invalid type", mode);
    return nullptr;
}

// -------------------------------------------------

static inline
uintptr_t getAddressFromPointer(void* ptr)
{
    CARLA_ASSERT(ptr != nullptr);

    uintptr_t* addr = (uintptr_t*)&ptr;
    return *addr;
}

static inline
void* getPointerFromAddress(uintptr_t& addr)
{
    CARLA_ASSERT(addr != 0);

    uintptr_t** const ptr = (uintptr_t**)&addr;
    return *ptr;
}

// -------------------------------------------------

static inline
const char* getPluginTypeAsString(const PluginType& type)
{
    carla_debug("CarlaBackend::getPluginTypeAsString(%s)", PluginType2Str(type));

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
    case PLUGIN_VST:
        return "VST";
    case PLUGIN_VST3:
        return "VST3";
    case PLUGIN_GIG:
        return "GIG";
    case PLUGIN_SF2:
        return "SF2";
    case PLUGIN_SFZ:
        return "SFZ";
    }

    carla_stderr("CarlaBackend::getPluginTypeAsString(%i) - invalid type", type);
    return "NONE";
}

static inline
PluginType getPluginTypeFromString(const char* const stype)
{
    CARLA_ASSERT(stype != nullptr);
    carla_debug("CarlaBackend::getPluginTypeFromString(%s)", stype);

    if (stype == nullptr)
    {
        carla_stderr("CarlaBackend::getPluginTypeFromString() - null string type");
        return PLUGIN_NONE;
    }

    if (std::strcmp(stype, "NONE") == 0)
        return PLUGIN_NONE;
    if (std::strcmp(stype, "INTERNAL") == 0 || std::strcmp(stype, "Internal") == 0)
        return PLUGIN_INTERNAL;
    if (std::strcmp(stype, "LADSPA") == 0)
        return PLUGIN_LADSPA;
    if (std::strcmp(stype, "DSSI") == 0)
        return PLUGIN_DSSI;
    if (std::strcmp(stype, "LV2") == 0)
        return PLUGIN_LV2;
    if (std::strcmp(stype, "VST") == 0)
        return PLUGIN_VST;
    if (std::strcmp(stype, "VST3") == 0)
        return PLUGIN_VST3;
    if (std::strcmp(stype, "GIG") == 0)
        return PLUGIN_GIG;
    if (std::strcmp(stype, "SF2") == 0)
        return PLUGIN_SF2;
    if (std::strcmp(stype, "SFZ") == 0)
        return PLUGIN_SFZ;

    carla_stderr("CarlaBackend::getPluginTypeFromString(%s) - invalid string type", stype);
    return PLUGIN_NONE;
}

// -------------------------------------------------

static inline
PluginCategory getPluginCategoryFromName(const char* const name)
{
    CARLA_ASSERT(name != nullptr);
    carla_debug("CarlaBackend::getPluginCategoryFromName(\"%s\")", name);

    if (name == nullptr)
    {
        carla_stderr("CarlaBackend::getPluginCategoryFromName() - null name");
        return PLUGIN_CATEGORY_NONE;
    }

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

    return PLUGIN_CATEGORY_NONE;
}

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_BACKEND_UTILS_HPP__
