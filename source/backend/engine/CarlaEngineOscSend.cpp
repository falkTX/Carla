/*
 * Carla Plugin Host
 * Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngineOsc.hpp"

#ifdef HAVE_LIBLO

#include "CarlaBackendUtils.hpp"
#include "CarlaEngine.hpp"
#include "CarlaPlugin.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

void CarlaEngineOsc::sendCallback(const EngineCallbackOpcode action, const uint pluginId,
                                  const int value1, const int value2, const int value3,
                                  const float valuef, const char* const valueStr) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    carla_debug("CarlaEngineOsc::sendCallback(%i:%s, %i, %i, %i, %i, %f, \"%s\")",
                action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valuef, valueStr);

    static const char* const kFakeNullString = "(null)";

    char targetPath[std::strlen(fControlDataTCP.path)+10];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/cb");
    try_lo_send(fControlDataTCP.target, targetPath, "iiiiifs",
                action, pluginId, value1, value2, value3, static_cast<double>(valuef),
                valueStr != nullptr ? valueStr : kFakeNullString);
}

void CarlaEngineOsc::sendPluginInit(const uint pluginId, const char* const pluginName) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginName != nullptr && pluginName[0] != '\0',);
    carla_debug("CarlaEngineOsc::sendadd_plugin_start(%i, \"%s\")", pluginId, pluginName);

    char targetPath[std::strlen(fControlDataTCP.path)+18];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/init");
    try_lo_send(fControlDataTCP.target, targetPath, "is", static_cast<int32_t>(pluginId), pluginName);
}

void CarlaEngineOsc::sendPluginInfo(const CarlaPlugin* const plugin) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    carla_debug("CarlaEngineOsc::sendPluginInfo(%p)", plugin);

    char bufName[STR_MAX+1], bufLabel[STR_MAX+1], bufMaker[STR_MAX+1], bufCopyright[STR_MAX+1];
    carla_zeroChars(bufName, STR_MAX+1);
    carla_zeroChars(bufLabel, STR_MAX+1);
    carla_zeroChars(bufMaker, STR_MAX+1);
    carla_zeroChars(bufCopyright, STR_MAX+1);

    plugin->getRealName(bufName);
    plugin->getLabel(bufLabel);
    plugin->getMaker(bufMaker);
    plugin->getCopyright(bufCopyright);

    char targetPath[std::strlen(fControlDataTCP.path)+10];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/info");
    try_lo_send(fControlDataTCP.target, targetPath, "iiiihssss",
                static_cast<int32_t>(plugin->getId()),
                static_cast<int32_t>(plugin->getType()),
                static_cast<int32_t>(plugin->getCategory()),
                static_cast<int32_t>(plugin->getHints()),
                static_cast<int64_t>(plugin->getUniqueId()),
                bufName, bufLabel, bufMaker, bufCopyright);
}

void CarlaEngineOsc::sendPluginPortCount(const CarlaPlugin* const plugin) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    carla_debug("CarlaEngineOsc::sendPluginInfo(%p)", plugin);

    uint32_t paramIns, paramOuts;
    plugin->getParameterCountInfo(paramIns, paramOuts);

    if (paramIns > 49)
        paramIns = 49;
    if (paramOuts > 49)
        paramOuts = 49;

    char targetPath[std::strlen(fControlDataTCP.path)+7];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/ports");
    try_lo_send(fControlDataTCP.target, targetPath, "iiiiiiii",
                static_cast<int32_t>(plugin->getId()),
                static_cast<int32_t>(plugin->getAudioInCount()),
                static_cast<int32_t>(plugin->getAudioOutCount()),
                static_cast<int32_t>(plugin->getMidiInCount()),
                static_cast<int32_t>(plugin->getMidiOutCount()),
                static_cast<int32_t>(paramIns),
                static_cast<int32_t>(paramOuts),
                static_cast<int32_t>(plugin->getParameterCount()));
}

void CarlaEngineOsc::sendPluginParameterInfo(const CarlaPlugin* const plugin, const uint32_t parameterId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    carla_debug("CarlaEngineOsc::sendPluginInfo(%p, %u)", plugin, parameterId);

    char bufName[STR_MAX+1], bufUnit[STR_MAX+1];
    carla_zeroChars(bufName, STR_MAX+1);
    carla_zeroChars(bufUnit, STR_MAX+1);

    const ParameterData& paramData(plugin->getParameterData(parameterId));
    const ParameterRanges& paramRanges(plugin->getParameterRanges(parameterId));

    plugin->getParameterName(parameterId, bufName);
    plugin->getParameterUnit(parameterId, bufUnit);

    char targetPath[std::strlen(fControlDataTCP.path)+20];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/param");
    try_lo_send(fControlDataTCP.target, targetPath, "iiiiiissfffffff",
                static_cast<int32_t>(plugin->getId()), static_cast<int32_t>(parameterId),
                static_cast<int32_t>(paramData.type), static_cast<int32_t>(paramData.hints),
                static_cast<int32_t>(paramData.midiChannel), static_cast<int32_t>(paramData.midiCC),
                bufName, bufUnit,
                static_cast<double>(paramRanges.def),
                static_cast<double>(paramRanges.min),
                static_cast<double>(paramRanges.max),
                static_cast<double>(paramRanges.step),
                static_cast<double>(paramRanges.stepSmall),
                static_cast<double>(paramRanges.stepLarge),
                static_cast<double>(plugin->getParameterValue(parameterId)));
}

void CarlaEngineOsc::sendPluginInternalParameterValues(const CarlaPlugin* const plugin) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    carla_debug("CarlaEngineOsc::sendPluginInternalParameterValues(%p)", plugin);

    static_assert(PARAMETER_ACTIVE == -2 && PARAMETER_MAX == -9);

    double iparams[7];

    for (int32_t i = 0; i < 7; ++i)
        iparams[i] = plugin->getInternalParameterValue(PARAMETER_ACTIVE - i);

    char targetPath[std::strlen(fControlDataTCP.path)+18];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/iparams");
    try_lo_send(fControlDataTCP.target, targetPath, "ifffffff",
                static_cast<int32_t>(plugin->getId()),
                iparams[0], // PARAMETER_ACTIVE
                iparams[1], // PARAMETER_DRYWET
                iparams[2], // PARAMETER_VOLUME
                iparams[3], // PARAMETER_BALANCE_LEFT
                iparams[4], // PARAMETER_BALANCE_RIGHT
                iparams[5], // PARAMETER_PANNING
                iparams[6]  // PARAMETER_CTRL_CHANNEL
               );
}

#if 0
void CarlaEngineOsc::sendset_program_count(const uint pluginId, const uint32_t count) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    carla_debug("CarlaEngineOsc::sendset_program_count(%i, %i)", pluginId, count);

    char targetPath[std::strlen(fControlDataTCP.path)+19];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/set_program_count");
    try_lo_send(fControlDataTCP.target, targetPath, "ii", static_cast<int32_t>(pluginId), static_cast<int32_t>(count));
}

void CarlaEngineOsc::sendset_midi_program_count(const uint pluginId, const uint32_t count) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    carla_debug("CarlaEngineOsc::sendset_midi_program_count(%i, %i)", pluginId, count);

    char targetPath[std::strlen(fControlDataTCP.path)+24];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/set_midi_program_count");
    try_lo_send(fControlDataTCP.target, targetPath, "ii", static_cast<int32_t>(pluginId), static_cast<int32_t>(count));
}

void CarlaEngineOsc::sendset_program_name(const uint pluginId, const uint32_t index, const char* const name) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr,);
    carla_debug("CarlaEngineOsc::sendset_program_name(%i, %i, \"%s\")", pluginId, index, name);

    char targetPath[std::strlen(fControlDataTCP.path)+18];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/set_program_name");
    try_lo_send(fControlDataTCP.target, targetPath, "iis", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), name);
}

void CarlaEngineOsc::sendset_midi_program_data(const uint pluginId, const uint32_t index, const uint32_t bank, const uint32_t program, const char* const name) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr,);
    carla_debug("CarlaEngineOsc::sendset_midi_program_data(%i, %i, %i, %i, \"%s\")", pluginId, index, bank, program, name);

    char targetPath[std::strlen(fControlDataTCP.path)+23];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/set_midi_program_data");
    try_lo_send(fControlDataTCP.target, targetPath, "iiiis", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(bank), static_cast<int32_t>(program), name);
}
#endif

// -----------------------------------------------------------------------

// FIXME use UDP

void CarlaEngineOsc::sendRuntimeInfo() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    carla_debug("CarlaEngineOsc::sendRuntimeInfo()");

    const EngineTimeInfo timeInfo(fEngine->getTimeInfo());

    char targetPath[std::strlen(fControlDataTCP.path)+18];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/runtime");
    try_lo_send(fControlDataTCP.target, targetPath, "fiihiiif",
                static_cast<double>(fEngine->getDSPLoad()),
                static_cast<int32_t>(fEngine->getTotalXruns()),
                timeInfo.playing ? 1 : 0,
                static_cast<int64_t>(timeInfo.frame),
                static_cast<int32_t>(timeInfo.bbt.bar),
                static_cast<int32_t>(timeInfo.bbt.beat),
                static_cast<int32_t>(timeInfo.bbt.tick),
                timeInfo.bbt.beatsPerMinute);
}

void CarlaEngineOsc::sendParameterValue(const uint pluginId, const uint32_t index, const float value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);

    char targetPath[std::strlen(fControlDataTCP.path)+21];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/param_fixme");
    try_lo_send(fControlDataTCP.target, targetPath, "iif",
                static_cast<int32_t>(pluginId),
                index,
                static_cast<double>(value));
}

void CarlaEngineOsc::sendPeaks(const uint pluginId, const float peaks[4]) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);

    char targetPath[std::strlen(fControlDataTCP.path)+11];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/peaks");
    try_lo_send(fControlDataTCP.target, targetPath, "iffff", static_cast<int32_t>(pluginId),
                static_cast<double>(peaks[0]),
                static_cast<double>(peaks[1]),
                static_cast<double>(peaks[2]),
                static_cast<double>(peaks[3]));
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // HAVE_LIBLO
