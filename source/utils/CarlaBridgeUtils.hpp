/*
 * Carla Bridge utils
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaRingBuffer.hpp"

#define PLUGIN_BRIDGE_NAMEPREFIX_AUDIO_POOL    "/carla-bridge_shm_ap_"
#define PLUGIN_BRIDGE_NAMEPREFIX_RT_CLIENT     "/carla-bridge_shm_rtC_"
#define PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_CLIENT "/carla-bridge_shm_nonrtC_"
#define PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_SERVER "/carla-bridge_shm_nonrtS_"

// -----------------------------------------------------------------------

// Server sends these to client during RT
enum PluginBridgeRtClientOpcode {
    kPluginBridgeRtClientNull = 0,
    kPluginBridgeRtClientSetAudioPool,            // ulong/ptr
    kPluginBridgeRtClientControlEventParameter,   // uint/frame, byte/chan, ushort, float
    kPluginBridgeRtClientControlEventMidiBank,    // uint/frame, byte/chan, ushort
    kPluginBridgeRtClientControlEventMidiProgram, // uint/frame, byte/chan, ushort
    kPluginBridgeRtClientControlEventAllSoundOff, // uint/frame, byte/chan
    kPluginBridgeRtClientControlEventAllNotesOff, // uint/frame, byte/chan
    kPluginBridgeRtClientMidiEvent,               // uint/frame, byte/port, byte/size, byte[]/data
    kPluginBridgeRtClientProcess,
    kPluginBridgeRtClientQuit
};

// Server sends these to client during non-RT
enum PluginBridgeNonRtClientOpcode {
    kPluginBridgeNonRtClientNull = 0,
    kPluginBridgeNonRtClientPing,
    kPluginBridgeNonRtClientActivate,
    kPluginBridgeNonRtClientDeactivate,
    kPluginBridgeNonRtClientSetBufferSize,           // uint
    kPluginBridgeNonRtClientSetSampleRate,           // double
    kPluginBridgeNonRtClientSetOffline,
    kPluginBridgeNonRtClientSetOnline,
    kPluginBridgeNonRtClientSetParameterValue,       // uint, float
    kPluginBridgeNonRtClientSetParameterMidiChannel, // uint, byte
    kPluginBridgeNonRtClientSetParameterMidiCC,      // uint, short
    kPluginBridgeNonRtClientSetProgram,              // int
    kPluginBridgeNonRtClientSetMidiProgram,          // int
    kPluginBridgeNonRtClientSetCustomData,           // uint/size, str[], uint/size, str[], uint/size, str[] (base64, compressed)
    kPluginBridgeNonRtClientSetChunkDataFile,        // uint/size, str[] (filename, base64 content)
    kPluginBridgeNonRtClientSetCtrlChannel,          // short
    kPluginBridgeNonRtClientSetOption,               // uint/option, bool
    kPluginBridgeNonRtClientPrepareForSave,
    kPluginBridgeNonRtClientShowUI,
    kPluginBridgeNonRtClientHideUI,
    kPluginBridgeNonRtClientUiParameterChange,       // uint, float
    kPluginBridgeNonRtClientUiProgramChange,         // uint
    kPluginBridgeNonRtClientUiMidiProgramChange,     // uint
    kPluginBridgeNonRtClientUiNoteOn,                // byte, byte, byte
    kPluginBridgeNonRtClientUiNoteOff,               // byte, byte
    kPluginBridgeNonRtClientQuit
};

// Client sends these to server during non-RT
enum PluginBridgeNonRtServerOpcode {
    kPluginBridgeNonRtServerNull = 0,
    kPluginBridgeNonRtServerPong,
    kPluginBridgeNonRtServerPluginInfo1,        // uint/category, uint/hints, uint/optionsAvailable, uint/optionsEnabled, long/uniqueId
    kPluginBridgeNonRtServerPluginInfo2,        // uint/size, str[] (realName), uint/size, str[] (label), uint/size, str[] (maker), uint/size, str[] (copyright)
    kPluginBridgeNonRtServerAudioCount,         // uint/ins, uint/outs
    kPluginBridgeNonRtServerMidiCount,          // uint/ins, uint/outs
    kPluginBridgeNonRtServerParameterCount,     // uint/ins, uint/outs
    kPluginBridgeNonRtServerProgramCount,       // uint/count
    kPluginBridgeNonRtServerMidiProgramCount,   // uint/count
    kPluginBridgeNonRtServerParameterData1,     // uint/index, int/rindex, uint/type, uint/hints, short/cc
    kPluginBridgeNonRtServerParameterData2,     // uint/index, uint/size, str[] (name), uint/size, str[] (unit)
    kPluginBridgeNonRtServerParameterRanges,    // uint/index, float/def, float/min, float/max, float/step, float/stepSmall, float/stepLarge
    kPluginBridgeNonRtServerParameterValue,     // uint/index float/value
    kPluginBridgeNonRtServerParameterValue2,    // uint/index float/value (used for init/output parameters only, don't resend values)
    kPluginBridgeNonRtServerDefaultValue,       // uint/index float/value
    kPluginBridgeNonRtServerCurrentProgram,     // int/index
    kPluginBridgeNonRtServerCurrentMidiProgram, // int/index
    kPluginBridgeNonRtServerProgramName,        // uint/index, uint/size, str[] (name)
    kPluginBridgeNonRtServerMidiProgramData,    // uint/index, uint/bank, uint/program, uint/size, str[] (name)
    kPluginBridgeNonRtServerSetCustomData,      // uint/size, str[], uint/size, str[], uint/size, str[] (base64, compressed)
    kPluginBridgeNonRtServerSetChunkDataFile,   // uint/size, str[] (filename, base64 content)
    kPluginBridgeNonRtServerSetLatency,         // uint
    kPluginBridgeNonRtServerReady,
    kPluginBridgeNonRtServerSaved,
    kPluginBridgeNonRtServerUiClosed,
    kPluginBridgeNonRtServerError
};

// -----------------------------------------------------------------------

struct BridgeSemaphore {
    union {
        void* server;
        char _padServer[64];
    };
    union {
        void* client;
        char _padClient[64];
    };
};

// needs to be 64bit aligned
struct BridgeTimeInfo {
    uint64_t playing;
    uint64_t frame;
    uint64_t usecs;
    uint32_t valid;
    // bbt
    int32_t bar, beat, tick;
    float beatsPerBar, beatType;
    double barStartTick, ticksPerBeat, beatsPerMinute;
};

// -----------------------------------------------------------------------

static const std::size_t kBridgeRtClientDataMidiOutSize = 512*4;

// Server => Client RT
struct BridgeRtClientData {
    BridgeSemaphore sem;
    BridgeTimeInfo timeInfo;
    SmallStackBuffer ringBuffer;
    uint8_t midiOut[kBridgeRtClientDataMidiOutSize];
};

// Server => Client Non-RT
struct BridgeNonRtClientData {
    BigStackBuffer ringBuffer;
};

// Client => Server Non-RT
struct BridgeNonRtServerData {
    HugeStackBuffer ringBuffer;
};

// -----------------------------------------------------------------------

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
    case kPluginBridgeNonRtServerParameterCount:
        return "kPluginBridgeNonRtServerParameterCount";
    case kPluginBridgeNonRtServerProgramCount:
        return "kPluginBridgeNonRtServerProgramCount";
    case kPluginBridgeNonRtServerMidiProgramCount:
        return "kPluginBridgeNonRtServerMidiProgramCount";
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

// -----------------------------------------------------------------------

#endif // CARLA_BRIDGE_UTILS_HPP_INCLUDED
