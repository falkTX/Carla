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

static const char* const kNullString = "";

// -----------------------------------------------------------------------

void CarlaEngineOsc::sendCallback(const EngineCallbackOpcode action, const uint pluginId,
                                  const int value1, const int value2, const int value3,
                                  const float valuef, const char* const valueStr) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    carla_stdout("CarlaEngineOsc::sendCallback(%i:%s, %i, %i, %i, %i, %f, \"%s\")",
                action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3,
                 static_cast<double>(valuef), valueStr);

    char targetPath[std::strlen(fControlDataTCP.path)+4];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/cb");
    try_lo_send(fControlDataTCP.target, targetPath, "iiiiifs",
                action, pluginId, value1, value2, value3, static_cast<double>(valuef),
                valueStr != nullptr ? valueStr : kNullString);
}

void CarlaEngineOsc::sendPluginInfo(const CarlaPluginPtr& plugin) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    carla_stdout("CarlaEngineOsc::sendPluginInfo(%p)", plugin.get());

    char bufRealName[STR_MAX+1], bufLabel[STR_MAX+1], bufMaker[STR_MAX+1], bufCopyright[STR_MAX+1];
    carla_zeroChars(bufRealName, STR_MAX+1);
    carla_zeroChars(bufLabel, STR_MAX+1);
    carla_zeroChars(bufMaker, STR_MAX+1);
    carla_zeroChars(bufCopyright, STR_MAX+1);

    if (! plugin->getRealName(bufRealName))
        bufRealName[0] = '\0';
    if (! plugin->getLabel(bufLabel))
        bufLabel[0] = '\0';
    if (! plugin->getMaker(bufMaker))
        bufMaker[0] = '\0';
    if (! plugin->getCopyright(bufCopyright))
        bufCopyright[0] = '\0';

    const char* name = plugin->getName();
    const char* filename = plugin->getFilename();
    const char* iconName = plugin->getIconName();

    if (name == nullptr)
        name = kNullString;
    if (filename == nullptr)
        filename = kNullString;
    if (iconName == nullptr)
        iconName = kNullString;

    char targetPath[std::strlen(fControlDataTCP.path)+6];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/info");
    try_lo_send(fControlDataTCP.target, targetPath, "iiiihiisssssss",
                static_cast<int32_t>(plugin->getId()),
                static_cast<int32_t>(plugin->getType()),
                static_cast<int32_t>(plugin->getCategory()),
                static_cast<int32_t>(plugin->getHints()),
                static_cast<int64_t>(plugin->getUniqueId()),
                static_cast<int64_t>(plugin->getOptionsAvailable()),
                static_cast<int64_t>(plugin->getOptionsEnabled()),
                name, filename, iconName,
                bufRealName, bufLabel, bufMaker, bufCopyright);
}

void CarlaEngineOsc::sendPluginPortCount(const CarlaPluginPtr& plugin) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    carla_stdout("CarlaEngineOsc::sendPluginPortCount(%p)", plugin.get());

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

void CarlaEngineOsc::sendPluginParameterInfo(const CarlaPluginPtr& plugin, const uint32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    carla_debug("CarlaEngineOsc::sendPluginParameterInfo(%p, %u)", plugin.get(), index);

    char bufName[STR_MAX+1], bufUnit[STR_MAX+1], bufComment[STR_MAX+1], bufGroupName[STR_MAX+1];
    carla_zeroChars(bufName, STR_MAX+1);
    carla_zeroChars(bufUnit, STR_MAX+1);
    carla_zeroChars(bufComment, STR_MAX+1);
    carla_zeroChars(bufGroupName, STR_MAX+1);

    if (! plugin->getParameterName(index, bufName))
        bufName[0] = '\0';
    if (! plugin->getParameterUnit(index, bufUnit))
        bufUnit[0] = '\0';
    if (! plugin->getParameterComment(index, bufComment))
        bufComment[0] = '\0';
    if (! plugin->getParameterGroupName(index, bufGroupName))
        bufGroupName[0] = '\0';

    const ParameterData& paramData(plugin->getParameterData(index));
    const ParameterRanges& paramRanges(plugin->getParameterRanges(index));

    const int32_t pluginId = static_cast<int32_t>(plugin->getId());
    const int32_t paramId  = static_cast<int32_t>(index);

    char targetPath[std::strlen(fControlDataTCP.path)+13];

    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/paramInfo");
    try_lo_send(fControlDataTCP.target, targetPath, "iissss",
                pluginId, 
                paramId,
                bufName, 
                bufUnit, 
                bufComment, 
                bufGroupName);

    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/paramData");
    try_lo_send(fControlDataTCP.target, targetPath, "iiiiiifff",
                pluginId, 
                paramId,
                static_cast<int32_t>(paramData.type), 
                static_cast<int32_t>(paramData.hints),
                static_cast<int32_t>(paramData.midiChannel),
                static_cast<int32_t>(paramData.mappedControlIndex),
                static_cast<double>(paramData.mappedMinimum), 
                static_cast<double>(paramData.mappedMaximum),
                static_cast<double>(plugin->getParameterValue(index)));

    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/paramRanges");
    try_lo_send(fControlDataTCP.target, targetPath, "iiffffff",
                pluginId, 
                paramId,
                static_cast<double>(paramRanges.def),
                static_cast<double>(paramRanges.min),
                static_cast<double>(paramRanges.max),
                static_cast<double>(paramRanges.step),
                static_cast<double>(paramRanges.stepSmall),
                static_cast<double>(paramRanges.stepLarge));
}

void CarlaEngineOsc::sendPluginDataCount(const CarlaPluginPtr& plugin) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    carla_stdout("CarlaEngineOsc::sendPluginDataCount(%p)", plugin.get());

    char targetPath[std::strlen(fControlDataTCP.path)+7];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/count");
    try_lo_send(fControlDataTCP.target, targetPath, "iiiiii",
                static_cast<int32_t>(plugin->getId()),
                static_cast<int32_t>(plugin->getProgramCount()),
                static_cast<int32_t>(plugin->getMidiProgramCount()),
                static_cast<int32_t>(plugin->getCustomDataCount()),
                static_cast<int32_t>(plugin->getCurrentProgram()),
                static_cast<int32_t>(plugin->getCurrentMidiProgram()));
}

void CarlaEngineOsc::sendPluginProgramCount(const CarlaPluginPtr& plugin) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    carla_stdout("CarlaEngineOsc::sendPluginDataCount(%p)", plugin.get());

    char targetPath[std::strlen(fControlDataTCP.path)+8];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/pcount");
    try_lo_send(fControlDataTCP.target, targetPath, "iii",
                static_cast<int32_t>(plugin->getId()),
                static_cast<int32_t>(plugin->getProgramCount()),
                static_cast<int32_t>(plugin->getMidiProgramCount()));
}

void CarlaEngineOsc::sendPluginProgram(const CarlaPluginPtr& plugin, const uint32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    carla_stdout("CarlaEngineOsc::sendPluginProgram(%p, %u)", plugin.get(), index);

    char strBuf[STR_MAX+1];
    carla_zeroChars(strBuf, STR_MAX+1);
    if (! plugin->getProgramName(index, strBuf))
        strBuf[0] = '\0';

    char targetPath[std::strlen(fControlDataTCP.path)+6];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/prog");
    try_lo_send(fControlDataTCP.target, targetPath, "iis",
                static_cast<int32_t>(plugin->getId()), static_cast<int32_t>(index), strBuf);
}

void CarlaEngineOsc::sendPluginMidiProgram(const CarlaPluginPtr& plugin, const uint32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    carla_stdout("CarlaEngineOsc::sendPluginProgram(%p, %u)", plugin.get(), index);

    const MidiProgramData& mpdata(plugin->getMidiProgramData(index));
    CARLA_SAFE_ASSERT_RETURN(mpdata.name != nullptr,);

    char targetPath[std::strlen(fControlDataTCP.path)+7];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/mprog");
    try_lo_send(fControlDataTCP.target, targetPath, "iiiis",
                static_cast<int32_t>(plugin->getId()),
                static_cast<int32_t>(index),
                static_cast<int32_t>(mpdata.bank), static_cast<int32_t>(mpdata.program), mpdata.name);
}

void CarlaEngineOsc::sendPluginCustomData(const CarlaPluginPtr& plugin, const uint32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    carla_stdout("CarlaEngineOsc::sendPluginCustomData(%p, %u)", plugin.get(), index);

    const CustomData& cdata(plugin->getCustomData(index));
    CARLA_SAFE_ASSERT_RETURN(cdata.isValid(),);

    char targetPath[std::strlen(fControlDataTCP.path)+7];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/cdata");
    try_lo_send(fControlDataTCP.target, targetPath, "iisss",
                static_cast<int32_t>(plugin->getId()),
                static_cast<int32_t>(index),
                cdata.type, cdata.key, cdata.value);
}

void CarlaEngineOsc::sendPluginInternalParameterValues(const CarlaPluginPtr& plugin) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    carla_debug("CarlaEngineOsc::sendPluginInternalParameterValues(%p)", plugin.get());

#ifdef CARLA_PROPER_CPP11_SUPPORT
    static_assert(PARAMETER_ACTIVE == -2 && PARAMETER_MAX == -9, "Incorrect data");
#endif

    double iparams[7];

    for (int32_t i = 0; i < 7; ++i)
        iparams[i] = plugin->getInternalParameterValue(PARAMETER_ACTIVE - i);

    char targetPath[std::strlen(fControlDataTCP.path)+9];
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

// -----------------------------------------------------------------------

void CarlaEngineOsc::sendPing() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);

    char targetPath[std::strlen(fControlDataTCP.path)+6];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/ping");
    try_lo_send(fControlDataTCP.target, targetPath, "");
}

void CarlaEngineOsc::sendResponse(const int messageId, const char* const error) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    carla_debug("CarlaEngineOsc::sendResponse()");

    char targetPath[std::strlen(fControlDataTCP.path)+6];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/resp");
    try_lo_send(fControlDataTCP.target, targetPath, "is", messageId, error);
}

void CarlaEngineOsc::sendExit() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.path != nullptr && fControlDataTCP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataTCP.target != nullptr,);
    carla_debug("CarlaEngineOsc::sendExit()");

    char targetPath[std::strlen(fControlDataTCP.path)+6];
    std::strcpy(targetPath, fControlDataTCP.path);
    std::strcat(targetPath, "/exit");
    try_lo_send(fControlDataTCP.target, targetPath, "");
}

// -----------------------------------------------------------------------

void CarlaEngineOsc::sendRuntimeInfo() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataUDP.path != nullptr && fControlDataUDP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataUDP.target != nullptr,);

    const EngineTimeInfo timeInfo(fEngine->getTimeInfo());

    char targetPath[std::strlen(fControlDataUDP.path)+9];
    std::strcpy(targetPath, fControlDataUDP.path);
    std::strcat(targetPath, "/runtime");
    try_lo_send(fControlDataUDP.target, targetPath, "fiihiiif",
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
    CARLA_SAFE_ASSERT_RETURN(fControlDataUDP.path != nullptr && fControlDataUDP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataUDP.target != nullptr,);

    char targetPath[std::strlen(fControlDataUDP.path)+7];
    std::strcpy(targetPath, fControlDataUDP.path);
    std::strcat(targetPath, "/param");
    try_lo_send(fControlDataUDP.target, targetPath, "iif",
                static_cast<int32_t>(pluginId),
                index,
                static_cast<double>(value));
}

void CarlaEngineOsc::sendPeaks(const uint pluginId, const float peaks[4]) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fControlDataUDP.path != nullptr && fControlDataUDP.path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(fControlDataUDP.target != nullptr,);

    char targetPath[std::strlen(fControlDataUDP.path)+7];
    std::strcpy(targetPath, fControlDataUDP.path);
    std::strcat(targetPath, "/peaks");
    try_lo_send(fControlDataUDP.target, targetPath, "iffff", static_cast<int32_t>(pluginId),
                static_cast<double>(peaks[0]),
                static_cast<double>(peaks[1]),
                static_cast<double>(peaks[2]),
                static_cast<double>(peaks[3]));
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // HAVE_LIBLO
