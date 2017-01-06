/*
 * Carla Bridge utils
 * Copyright (C) 2013-2017 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_BRIDGE_UTILS_HPP_INCLUDED
#define CARLA_BRIDGE_UTILS_HPP_INCLUDED

#include "CarlaBridgeDefines.hpp"
#include "CarlaString.hpp"

#if defined(CARLA_OS_WIN) && defined(BUILDING_CARLA_FOR_WINDOWS)
# define PLUGIN_BRIDGE_NAMEPREFIX_RT_CLIENT     "Global\\carla-bridge_shm_rtC_"
# define PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_CLIENT "Global\\carla-bridge_shm_nonrtC_"
# define PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_SERVER "Global\\carla-bridge_shm_nonrtS_"
#else
# define PLUGIN_BRIDGE_NAMEPREFIX_RT_CLIENT     "/crlbrdg_shm_rtC_"
# define PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_CLIENT "/crlbrdg_shm_nonrtC_"
# define PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_SERVER "/crlbrdg_shm_nonrtS_"
#endif

// -------------------------------------------------------------------------------------------------------------------

static inline
const char* PluginBridgeRtClientOpcode2str(const PluginBridgeRtClientOpcode opcode) noexcept
{
    switch (opcode)
    {
    case kPluginBridgeRtClientNull:
        return "kPluginBridgeRtClientNull";
    case kPluginBridgeRtClientSetAudioPool:
        return "kPluginBridgeRtClientSetAudioPool";
    case kPluginBridgeRtClientControlEventParameter:
        return "kPluginBridgeRtClientControlEventParameter";
    case kPluginBridgeRtClientControlEventMidiBank:
        return "kPluginBridgeRtClientControlEventMidiBank";
    case kPluginBridgeRtClientControlEventMidiProgram:
        return "kPluginBridgeRtClientControlEventMidiProgram";
    case kPluginBridgeRtClientControlEventAllSoundOff:
        return "kPluginBridgeRtClientControlEventAllSoundOff";
    case kPluginBridgeRtClientControlEventAllNotesOff:
        return "kPluginBridgeRtClientControlEventAllNotesOff";
    case kPluginBridgeRtClientMidiEvent:
        return "kPluginBridgeRtClientMidiEvent";
    case kPluginBridgeRtClientProcess:
        return "kPluginBridgeRtClientProcess";
    case kPluginBridgeRtClientQuit:
        return "kPluginBridgeRtClientQuit";
    }

    carla_stderr("CarlaBackend::PluginBridgeRtClientOpcode2str(%i) - invalid opcode", opcode);
    return nullptr;
}

static inline
const char* PluginBridgeNonRtClientOpcode2str(const PluginBridgeNonRtClientOpcode opcode) noexcept
{
    switch (opcode)
    {
    case kPluginBridgeNonRtClientNull:
        return "kPluginBridgeNonRtClientNull";
    case kPluginBridgeNonRtClientPing:
        return "kPluginBridgeNonRtClientPing";
    case kPluginBridgeNonRtClientPingOnOff:
        return "kPluginBridgeNonRtClientPingOnOff";
    case kPluginBridgeNonRtClientActivate:
        return "kPluginBridgeNonRtClientActivate";
    case kPluginBridgeNonRtClientDeactivate:
        return "kPluginBridgeNonRtClientDeactivate";
    case kPluginBridgeNonRtClientSetBufferSize:
        return "kPluginBridgeNonRtClientSetBufferSize";
    case kPluginBridgeNonRtClientSetSampleRate:
        return "kPluginBridgeNonRtClientSetSampleRate";
    case kPluginBridgeNonRtClientSetOffline:
        return "kPluginBridgeNonRtClientSetOffline";
    case kPluginBridgeNonRtClientSetOnline:
        return "kPluginBridgeNonRtClientSetOnline";
    case kPluginBridgeNonRtClientSetParameterValue:
        return "kPluginBridgeNonRtClientSetParameterValue";
    case kPluginBridgeNonRtClientSetParameterMidiChannel:
        return "kPluginBridgeNonRtClientSetParameterMidiChannel";
    case kPluginBridgeNonRtClientSetParameterMidiCC:
        return "kPluginBridgeNonRtClientSetParameterMidiCC";
    case kPluginBridgeNonRtClientSetProgram:
        return "kPluginBridgeNonRtClientSetProgram";
    case kPluginBridgeNonRtClientSetMidiProgram:
        return "kPluginBridgeNonRtClientSetMidiProgram";
    case kPluginBridgeNonRtClientSetCustomData:
        return "kPluginBridgeNonRtClientSetCustomData";
    case kPluginBridgeNonRtClientSetChunkDataFile:
        return "kPluginBridgeNonRtClientSetChunkDataFile";
    case kPluginBridgeNonRtClientSetCtrlChannel:
        return "kPluginBridgeNonRtClientSetCtrlChannel";
    case kPluginBridgeNonRtClientSetOption:
        return "kPluginBridgeNonRtClientSetOption";
    case kPluginBridgeNonRtClientPrepareForSave:
        return "kPluginBridgeNonRtClientPrepareForSave";
    case kPluginBridgeNonRtClientShowUI:
        return "kPluginBridgeNonRtClientShowUI";
    case kPluginBridgeNonRtClientHideUI:
        return "kPluginBridgeNonRtClientHideUI";
    case kPluginBridgeNonRtClientUiParameterChange:
        return "kPluginBridgeNonRtClientUiParameterChange";
    case kPluginBridgeNonRtClientUiProgramChange:
        return "kPluginBridgeNonRtClientUiProgramChange";
    case kPluginBridgeNonRtClientUiMidiProgramChange:
        return "kPluginBridgeNonRtClientUiMidiProgramChange";
    case kPluginBridgeNonRtClientUiNoteOn:
        return "kPluginBridgeNonRtClientUiNoteOn";
    case kPluginBridgeNonRtClientUiNoteOff:
        return "kPluginBridgeNonRtClientUiNoteOff";
    case kPluginBridgeNonRtClientQuit:
        return "kPluginBridgeNonRtClientQuit";
    }

    carla_stderr("CarlaBackend::PluginBridgeNonRtClientOpcode2str(%i) - invalid opcode", opcode);
    return nullptr;
}

static inline
const char* PluginBridgeNonRtServerOpcode2str(const PluginBridgeNonRtServerOpcode opcode) noexcept
{
    switch (opcode)
    {
    case kPluginBridgeNonRtServerNull:
        return "kPluginBridgeNonRtServerNull";
    case kPluginBridgeNonRtServerPong:
        return "kPluginBridgeNonRtServerPong";
    case kPluginBridgeNonRtServerPluginInfo1:
        return "kPluginBridgeNonRtServerPluginInfo1";
    case kPluginBridgeNonRtServerPluginInfo2:
        return "kPluginBridgeNonRtServerPluginInfo2";
    case kPluginBridgeNonRtServerAudioCount:
        return "kPluginBridgeNonRtServerAudioCount";
    case kPluginBridgeNonRtServerMidiCount:
        return "kPluginBridgeNonRtServerMidiCount";
    case kPluginBridgeNonRtServerCvCount:
        return "kPluginBridgeNonRtServerCvCount";
    case kPluginBridgeNonRtServerParameterCount:
        return "kPluginBridgeNonRtServerParameterCount";
    case kPluginBridgeNonRtServerProgramCount:
        return "kPluginBridgeNonRtServerProgramCount";
    case kPluginBridgeNonRtServerMidiProgramCount:
        return "kPluginBridgeNonRtServerMidiProgramCount";
    case kPluginBridgeNonRtServerPortName:
        return "kPluginBridgeNonRtServerPortName";
    case kPluginBridgeNonRtServerParameterData1:
        return "kPluginBridgeNonRtServerParameterData1";
    case kPluginBridgeNonRtServerParameterData2:
        return "kPluginBridgeNonRtServerParameterData2";
    case kPluginBridgeNonRtServerParameterRanges:
        return "kPluginBridgeNonRtServerParameterRanges";
    case kPluginBridgeNonRtServerParameterValue:
        return "kPluginBridgeNonRtServerParameterValue";
    case kPluginBridgeNonRtServerParameterValue2:
        return "kPluginBridgeNonRtServerParameterValue2";
    case kPluginBridgeNonRtServerDefaultValue:
        return "kPluginBridgeNonRtServerDefaultValue";
    case kPluginBridgeNonRtServerCurrentProgram:
        return "kPluginBridgeNonRtServerCurrentProgram";
    case kPluginBridgeNonRtServerCurrentMidiProgram:
        return "kPluginBridgeNonRtServerCurrentMidiProgram";
    case kPluginBridgeNonRtServerProgramName:
        return "kPluginBridgeNonRtServerProgramName";
    case kPluginBridgeNonRtServerMidiProgramData:
        return "kPluginBridgeNonRtServerMidiProgramData";
    case kPluginBridgeNonRtServerSetCustomData:
        return "kPluginBridgeNonRtServerSetCustomData";
    case kPluginBridgeNonRtServerSetChunkDataFile:
        return "kPluginBridgeNonRtServerSetChunkDataFile";
    case kPluginBridgeNonRtServerSetLatency:
        return "kPluginBridgeNonRtServerSetLatency";
    case kPluginBridgeNonRtServerReady:
        return "kPluginBridgeNonRtServerReady";
    case kPluginBridgeNonRtServerSaved:
        return "kPluginBridgeNonRtServerSaved";
    case kPluginBridgeNonRtServerUiClosed:
        return "kPluginBridgeNonRtServerUiClosed";
    case kPluginBridgeNonRtServerError:
        return "kPluginBridgeNonRtServerError";
    }

    carla_stderr("CarlaBackend::PluginBridgeNonRtServerOpcode2str%i) - invalid opcode", opcode);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

struct BridgeAudioPool {
    float* data;
    std::size_t dataSize;
    CarlaString filename;
    char shm[64];

    BridgeAudioPool() noexcept;
    ~BridgeAudioPool() noexcept;

    bool initializeServer() noexcept;
    bool attachClient(const char* const fname) noexcept;
    void clear() noexcept;

    void resize(const uint32_t bufferSize, const uint32_t audioPortCount, const uint32_t cvPortCount) noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeAudioPool)
};

// -------------------------------------------------------------------------------------------------------------------

#endif // CARLA_BRIDGE_UTILS_HPP_INCLUDED
