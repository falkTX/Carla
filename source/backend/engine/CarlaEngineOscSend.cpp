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

#include "CarlaDefines.h"

#ifdef HAVE_LIBLO

#include "CarlaBackendUtils.hpp"
#include "CarlaEngineInternal.hpp"
#include "CarlaMIDI.h"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

#ifndef BUILD_BRIDGE
void CarlaEngine::oscSend_control_add_plugin_start(const uint pluginId, const char* const pluginName) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(pluginName != nullptr && pluginName[0] != '\0',);
    carla_debug("CarlaEngine::oscSend_control_add_plugin_start(%i, \"%s\")", pluginId, pluginName);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/add_plugin_start");
    try_lo_send(pData->oscData->target, targetPath, "is", static_cast<int32_t>(pluginId), pluginName);
}

void CarlaEngine::oscSend_control_add_plugin_end(const uint pluginId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_add_plugin_end(%i)", pluginId);

    char targetPath[std::strlen(pData->oscData->path)+16];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/add_plugin_end");
    try_lo_send(pData->oscData->target, targetPath, "i", static_cast<int32_t>(pluginId));
}

void CarlaEngine::oscSend_control_remove_plugin(const uint pluginId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_remove_plugin(%i)", pluginId);

    char targetPath[std::strlen(pData->oscData->path)+15];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/remove_plugin");
    try_lo_send(pData->oscData->target, targetPath, "i", static_cast<int32_t>(pluginId));
}

void CarlaEngine::oscSend_control_set_plugin_info1(const uint pluginId, const PluginType type, const PluginCategory category, const uint hints, const int64_t uniqueId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(type != PLUGIN_NONE,);
    carla_debug("CarlaEngine::oscSend_control_set_plugin_data(%i, %i:%s, %i:%s, %X, " P_INT64 ")", pluginId, type, PluginType2Str(type), category, PluginCategory2Str(category), hints, uniqueId);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_plugin_info1");
    try_lo_send(pData->oscData->target, targetPath, "iiiih", static_cast<int32_t>(pluginId), static_cast<int32_t>(type), static_cast<int32_t>(category), static_cast<int32_t>(hints), static_cast<int64_t>(uniqueId));
}

void CarlaEngine::oscSend_control_set_plugin_info2(const uint pluginId, const char* const realName, const char* const label, const char* const maker, const char* const copyright) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(realName != nullptr && realName[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(label != nullptr && label[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(maker != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(copyright != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_set_plugin_data(%i, \"%s\", \"%s\", \"%s\", \"%s\")", pluginId, realName, label, maker, copyright);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_plugin_info2");
    try_lo_send(pData->oscData->target, targetPath, "issss", static_cast<int32_t>(pluginId), realName, label, maker, copyright);
}

void CarlaEngine::oscSend_control_set_audio_count(const uint pluginId, const uint32_t ins, const uint32_t outs) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_audio_count(%i, %i, %i)", pluginId, ins, outs);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_audio_count");
    try_lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(ins), static_cast<int32_t>(outs));
}

void CarlaEngine::oscSend_control_set_midi_count(const uint pluginId, const uint32_t ins, const uint32_t outs) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_midi_count(%i, %i, %i)", pluginId, ins, outs);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_midi_count");
    try_lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(ins), static_cast<int32_t>(outs));
}

void CarlaEngine::oscSend_control_set_parameter_count(const uint pluginId, const uint32_t ins, const uint32_t outs) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_count(%i, %i, %i)", pluginId, ins, outs);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_count");
    try_lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(ins), static_cast<int32_t>(outs));
}

void CarlaEngine::oscSend_control_set_program_count(const uint pluginId, const uint32_t count) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_program_count(%i, %i)", pluginId, count);

    char targetPath[std::strlen(pData->oscData->path)+19];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_program_count");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(pluginId), static_cast<int32_t>(count));
}

void CarlaEngine::oscSend_control_set_midi_program_count(const uint pluginId, const uint32_t count) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_midi_program_count(%i, %i)", pluginId, count);

    char targetPath[std::strlen(pData->oscData->path)+24];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_midi_program_count");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(pluginId), static_cast<int32_t>(count));
}

void CarlaEngine::oscSend_control_set_parameter_data(const uint pluginId, const uint32_t index, const ParameterType type, const uint hints, const char* const name, const char* const unit) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(unit != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_data(%i, %i, %i:%s, %X, \"%s\", \"%s\")", pluginId, index, type, ParameterType2Str(type), hints, name, unit);

    char targetPath[std::strlen(pData->oscData->path)+20];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_data");
    try_lo_send(pData->oscData->target, targetPath, "iiiiss", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(type), static_cast<int32_t>(hints), name, unit);
}

void CarlaEngine::oscSend_control_set_parameter_ranges1(const uint pluginId, const uint32_t index, const float def, const float min, const float max) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    CARLA_SAFE_ASSERT(def >= min && def <= max);
    CARLA_SAFE_ASSERT(min < max);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_ranges1(%i, %i, %f, %f, %f)", pluginId, index, def, min, max, def);

    char targetPath[std::strlen(pData->oscData->path)+24];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_ranges1");
    try_lo_send(pData->oscData->target, targetPath, "iifff", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), def, min, max);
}

void CarlaEngine::oscSend_control_set_parameter_ranges2(const uint pluginId, const uint32_t index, const float step, const float stepSmall, const float stepLarge) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    CARLA_SAFE_ASSERT(step >= stepSmall && step <= stepLarge);
    CARLA_SAFE_ASSERT(stepSmall <= stepLarge);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_ranges2(%i, %i, %f, %f, %f)", pluginId, index, step, stepSmall, stepLarge);

    char targetPath[std::strlen(pData->oscData->path)+24];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_ranges2");
    try_lo_send(pData->oscData->target, targetPath, "iifff", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), step, stepSmall, stepLarge);
}

void CarlaEngine::oscSend_control_set_parameter_midi_cc(const uint pluginId, const uint32_t index, const int16_t cc) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(cc >= -1 && cc < MAX_MIDI_CONTROL,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_midi_cc(%i, %i, %i)", pluginId, index, cc);

    char targetPath[std::strlen(pData->oscData->path)+23];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_midi_cc");
    try_lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(cc));
}

void CarlaEngine::oscSend_control_set_parameter_midi_channel(const uint pluginId, const uint32_t index, const uint8_t channel) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_midi_channel(%i, %i, %i)", pluginId, index, channel);

    char targetPath[std::strlen(pData->oscData->path)+28];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_midi_channel");
    try_lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(channel));
}

void CarlaEngine::oscSend_control_set_parameter_value(const uint pluginId, const int32_t index, const float value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(index != PARAMETER_NULL,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_value(%i, %i:%s, %f)", pluginId, index, (index < 0) ? InternalParameterIndex2Str(static_cast<InternalParameterIndex>(index)) : "(none)", value);

    char targetPath[std::strlen(pData->oscData->path)+21];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_value");
    try_lo_send(pData->oscData->target, targetPath, "iif", static_cast<int32_t>(pluginId), index, value);
}

void CarlaEngine::oscSend_control_set_default_value(const uint pluginId, const uint32_t index, const float value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_default_value(%i, %i, %f)", pluginId, index, value);

    char targetPath[std::strlen(pData->oscData->path)+19];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_default_value");
    try_lo_send(pData->oscData->target, targetPath, "iif", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), value);
}

void CarlaEngine::oscSend_control_set_current_program(const uint pluginId, const int32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_current_program(%i, %i)", pluginId, index);

    char targetPath[std::strlen(pData->oscData->path)+21];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_current_program");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(pluginId), index);
}

void CarlaEngine::oscSend_control_set_current_midi_program(const uint pluginId, const int32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_current_midi_program(%i, %i)", pluginId, index);

    char targetPath[std::strlen(pData->oscData->path)+26];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_current_midi_program");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(pluginId), index);
}

void CarlaEngine::oscSend_control_set_program_name(const uint pluginId, const uint32_t index, const char* const name) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_set_program_name(%i, %i, \"%s\")", pluginId, index, name);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_program_name");
    try_lo_send(pData->oscData->target, targetPath, "iis", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), name);
}

void CarlaEngine::oscSend_control_set_midi_program_data(const uint pluginId, const uint32_t index, const uint32_t bank, const uint32_t program, const char* const name) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId <= pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_set_midi_program_data(%i, %i, %i, %i, \"%s\")", pluginId, index, bank, program, name);

    char targetPath[std::strlen(pData->oscData->path)+23];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_midi_program_data");
    try_lo_send(pData->oscData->target, targetPath, "iiiis", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(bank), static_cast<int32_t>(program), name);
}

void CarlaEngine::oscSend_control_note_on(const uint pluginId, const uint8_t channel, const uint8_t note, const uint8_t velo) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
    CARLA_SAFE_ASSERT_RETURN(velo < MAX_MIDI_VALUE,);
    carla_debug("CarlaEngine::oscSend_control_note_on(%i, %i, %i, %i)", pluginId, channel, note, velo);

    char targetPath[std::strlen(pData->oscData->path)+9];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/note_on");
    try_lo_send(pData->oscData->target, targetPath, "iiii", static_cast<int32_t>(pluginId), static_cast<int32_t>(channel), static_cast<int32_t>(note), static_cast<int32_t>(velo));
}

void CarlaEngine::oscSend_control_note_off(const uint pluginId, const uint8_t channel, const uint8_t note) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
    carla_debug("CarlaEngine::oscSend_control_note_off(%i, %i, %i)", pluginId, channel, note);

    char targetPath[std::strlen(pData->oscData->path)+10];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/note_off");
    try_lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(channel), static_cast<int32_t>(note));
}

void CarlaEngine::oscSend_control_set_peaks(const uint pluginId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);

    // TODO - try and see if we can get peaks[4] ref
    const EnginePluginData& epData(pData->plugins[pluginId]);

    char targetPath[std::strlen(pData->oscData->path)+11];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_peaks");
    try_lo_send(pData->oscData->target, targetPath, "iffff", static_cast<int32_t>(pluginId), epData.insPeak[0], epData.insPeak[1], epData.outsPeak[0], epData.outsPeak[1]);
}

void CarlaEngine::oscSend_control_exit() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_exit()");

    char targetPath[std::strlen(pData->oscData->path)+6];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/exit");
    try_lo_send(pData->oscData->target, targetPath, "");
}
#endif // BUILD_BRIDGE

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // HAVE_LIBLO
