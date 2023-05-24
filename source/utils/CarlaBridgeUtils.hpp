/*
 * Carla Bridge utils
 * Copyright (C) 2013-2023 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaMutex.hpp"
#include "CarlaRingBuffer.hpp"
#include "CarlaString.hpp"

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
    case kPluginBridgeRtClientSetBufferSize:
        return "kPluginBridgeRtClientSetBufferSize";
    case kPluginBridgeRtClientSetSampleRate:
        return "kPluginBridgeRtClientSetSampleRate";
    case kPluginBridgeRtClientSetOnline:
        return "kPluginBridgeRtClientSetOnline";
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
    return "";
}

static inline
const char* PluginBridgeNonRtClientOpcode2str(const PluginBridgeNonRtClientOpcode opcode) noexcept
{
    switch (opcode)
    {
    case kPluginBridgeNonRtClientNull:
        return "kPluginBridgeNonRtClientNull";
    case kPluginBridgeNonRtClientVersion:
        return "kPluginBridgeNonRtClientVersion";
    case kPluginBridgeNonRtClientPing:
        return "kPluginBridgeNonRtClientPing";
    case kPluginBridgeNonRtClientPingOnOff:
        return "kPluginBridgeNonRtClientPingOnOff";
    case kPluginBridgeNonRtClientActivate:
        return "kPluginBridgeNonRtClientActivate";
    case kPluginBridgeNonRtClientDeactivate:
        return "kPluginBridgeNonRtClientDeactivate";
    case kPluginBridgeNonRtClientInitialSetup:
        return "kPluginBridgeNonRtClientInitialSetup";
    case kPluginBridgeNonRtClientSetParameterValue:
        return "kPluginBridgeNonRtClientSetParameterValue";
    case kPluginBridgeNonRtClientSetParameterMidiChannel:
        return "kPluginBridgeNonRtClientSetParameterMidiChannel";
    case kPluginBridgeNonRtClientSetParameterMappedControlIndex:
        return "kPluginBridgeNonRtClientSetParameterMappedControlIndex";
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
    case kPluginBridgeNonRtClientGetParameterText:
        return "kPluginBridgeNonRtClientGetParameterText";
    case kPluginBridgeNonRtClientPrepareForSave:
        return "kPluginBridgeNonRtClientPrepareForSave";
    case kPluginBridgeNonRtClientRestoreLV2State:
        return "kPluginBridgeNonRtClientRestoreLV2State";
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
    case kPluginBridgeNonRtClientSetParameterMappedRange:
        return "kPluginBridgeNonRtClientSetParameterMappedRange";
    case kPluginBridgeNonRtClientSetOptions:
        return "kPluginBridgeNonRtClientSetOptions";
    case kPluginBridgeNonRtClientSetWindowTitle:
        return "kPluginBridgeNonRtClientSetWindowTitle";
    case kPluginBridgeNonRtClientEmbedUI:
        return "kPluginBridgeNonRtClientEmbedUI";
    case kPluginBridgeNonRtClientReload:
        return "kPluginBridgeNonRtClientReload";
    }

    carla_stderr("CarlaBackend::PluginBridgeNonRtClientOpcode2str(%i) - invalid opcode", opcode);
    return "";
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
    case kPluginBridgeNonRtServerParameterTouch:
        return "kPluginBridgeNonRtServerParameterTouch";
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
    case kPluginBridgeNonRtServerSetParameterText:
        return "kPluginBridgeNonRtServerSetParameterText";
    case kPluginBridgeNonRtServerReady:
        return "kPluginBridgeNonRtServerReady";
    case kPluginBridgeNonRtServerSaved:
        return "kPluginBridgeNonRtServerSaved";
    case kPluginBridgeNonRtServerUiClosed:
        return "kPluginBridgeNonRtServerUiClosed";
    case kPluginBridgeNonRtServerError:
        return "kPluginBridgeNonRtServerError";
    case kPluginBridgeNonRtServerVersion:
        return "kPluginBridgeNonRtServerVersion";
    case kPluginBridgeNonRtServerRespEmbedUI:
        return "kPluginBridgeNonRtServerRespEmbedUI";
    case kPluginBridgeNonRtServerResizeEmbedUI:
        return "kPluginBridgeNonRtServerResizeEmbedUI";
    }

    carla_stderr("CarlaBackend::PluginBridgeNonRtServerOpcode2str%i) - invalid opcode", opcode);
    return "";
}

// -------------------------------------------------------------------------------------------------------------------

static constexpr const std::size_t kBridgeRtClientDataMidiOutSize = 511*4;
static constexpr const std::size_t kBridgeBaseMidiOutHeaderSize   = 6U /* time, port and size */;

// Server => Client RT
struct BridgeRtClientData {
    BridgeSemaphore sem;
    BridgeTimeInfo timeInfo;
    SmallStackBuffer ringBuffer;
    uint8_t midiOut[kBridgeRtClientDataMidiOutSize];
    uint32_t procFlags;
};

// Server => Client Non-RT
struct BridgeNonRtClientData {
    BigStackBuffer ringBuffer;
};

// Client => Server Non-RT
struct BridgeNonRtServerData {
    HugeStackBuffer ringBuffer;
};

// -------------------------------------------------------------------------------------------------------------------

struct CARLA_API BridgeAudioPool {
    float* data;
    std::size_t dataSize;
    CarlaString filename;
    char shm[64];
    bool isServer;

    BridgeAudioPool() noexcept;
    ~BridgeAudioPool() noexcept;

    bool initializeServer() noexcept;
    bool attachClient(const char* const fname) noexcept;
    void clear() noexcept;

    void resize(const uint32_t bufferSize, const uint32_t audioPortCount, const uint32_t cvPortCount) noexcept;

    const char* getFilenameSuffix() const noexcept;

    CARLA_DECLARE_NON_COPYABLE(BridgeAudioPool)
};

// -------------------------------------------------------------------------------------------------------------------

struct CARLA_API BridgeRtClientControl : public CarlaRingBufferControl<SmallStackBuffer> {
    BridgeRtClientData* data;
    CarlaString filename;
    bool needsSemDestroy; // client only
    char shm[64];
    bool isServer;

    BridgeRtClientControl() noexcept;
    ~BridgeRtClientControl() noexcept override;

    bool initializeServer() noexcept;
    bool attachClient(const char* const basename) noexcept;
    void clear() noexcept;

    bool mapData() noexcept;
    void unmapData() noexcept;

    // non-bridge, server
    bool waitForClient(const uint msecs) noexcept;
    bool writeOpcode(const PluginBridgeRtClientOpcode opcode) noexcept;

    // bridge, client
    PluginBridgeRtClientOpcode readOpcode() noexcept;

    // helper class that automatically posts semaphore on destructor
    struct WaitHelper {
        BridgeRtClientData* const data;
        const bool ok;

        WaitHelper(BridgeRtClientControl& c) noexcept;
        ~WaitHelper() noexcept;

        CARLA_DECLARE_NON_COPYABLE(WaitHelper)
    };

    CARLA_DECLARE_NON_COPYABLE(BridgeRtClientControl)
};

// -------------------------------------------------------------------------------------------------------------------

struct CARLA_API BridgeNonRtClientControl : public CarlaRingBufferControl<BigStackBuffer> {
    BridgeNonRtClientData* data;
    CarlaString filename;
    CarlaMutex mutex;
    char shm[64];
    bool isServer;

    BridgeNonRtClientControl() noexcept;
    ~BridgeNonRtClientControl() noexcept override;

    bool initializeServer() noexcept;
    bool attachClient(const char* const basename) noexcept;
    void clear() noexcept;

    bool mapData() noexcept;
    void unmapData() noexcept;

    // non-bridge, server
    void waitIfDataIsReachingLimit() noexcept;
    bool writeOpcode(const PluginBridgeNonRtClientOpcode opcode) noexcept;

    // bridge, client
    PluginBridgeNonRtClientOpcode readOpcode() noexcept;

    CARLA_DECLARE_NON_COPYABLE(BridgeNonRtClientControl)
};

// -------------------------------------------------------------------------------------------------------------------

struct CARLA_API BridgeNonRtServerControl : public CarlaRingBufferControl<HugeStackBuffer> {
    BridgeNonRtServerData* data;
    CarlaString filename;
    CarlaMutex mutex;
    char shm[64];
    bool isServer;

    BridgeNonRtServerControl() noexcept;
    ~BridgeNonRtServerControl() noexcept override;

    bool initializeServer() noexcept;
    bool attachClient(const char* const basename) noexcept;
    void clear() noexcept;

    bool mapData() noexcept;
    void unmapData() noexcept;

    // non-bridge, server
    PluginBridgeNonRtServerOpcode readOpcode() noexcept;

    // bridge, client
    void waitIfDataIsReachingLimit() noexcept;
    bool writeOpcode(const PluginBridgeNonRtServerOpcode opcode) noexcept;

    CARLA_DECLARE_NON_COPYABLE(BridgeNonRtServerControl)
};

// -------------------------------------------------------------------------------------------------------------------

#endif // CARLA_BRIDGE_UTILS_HPP_INCLUDED
