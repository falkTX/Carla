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

// -----------------------------------------------------------------------

enum PluginBridgeInfoType {
    kPluginBridgeNull,
    kPluginBridgePong = 0,
    kPluginBridgePluginInfo1,          // uuh    => category, hints, uniqueId
    kPluginBridgePluginInfo2,          // ssss   => realName, label, maker, copyright
    kPluginBridgeAudioCount,           // uu     => ins, outs
    kPluginBridgeMidiCount,            // uu     => ins, outs
    kPluginBridgeParameterCount,       // uu     => ins, outs
    kPluginBridgeProgramCount,         // u      => count
    kPluginBridgeMidiProgramCount,     // u      => count
    kPluginBridgeParameterData,        // uiuuss => index, rindex, type, hints, name, unit
    kPluginBridgeParameterRanges1,     // ufff   => index, def, min, max
    kPluginBridgeParameterRanges2,     // ufff   => index, step, stepSmall, stepLarge
    kPluginBridgeParameterMidiCC,      // ui     => index, cc
    kPluginBridgeParameterMidiChannel, // uu     => index, channel
    kPluginBridgeParameterValue,       // uf     => index, value
    kPluginBridgeDefaultValue,         // uf     => index, value
    kPluginBridgeCurrentProgram,       // i      => index
    kPluginBridgeCurrentMidiProgram,   // i      => index
    kPluginBridgeProgramName,          // us     => index, name
    kPluginBridgeMidiProgramData,      // uuus   => index, bank, program, name
    kPluginBridgeConfigure,            // ss     => key, value
    kPluginBridgeSetCustomData,        // sss    => type, key, value
    kPluginBridgeSetChunkData,         // s      => chunkFile
    kPluginBridgeLatency,              // u      => value
    kPluginBridgeUpdateNow,            //
    kPluginBridgeError                 //
};

enum PluginBridgeRtOpcode {
    kPluginBridgeRtOpcodeNull = 0,
    kPluginBridgeRtOpcodeSetParameter,   // int, float
    kPluginBridgeRtOpcodeSetProgram,     // int
    kPluginBridgeRtOpcodeSetMidiProgram, // int
    kPluginBridgeRtOpcodeMidiEvent,      // uint/frame, uint/size, char[]
    kPluginBridgeRtOpcodeProcess,
    kPluginBridgeRtOpcodeQuit
};

enum PluginBridgeNonRtOpcode {
    kPluginBridgeNonRtOpcodeNull = 0,
    kPluginBridgeNonRtOpcodePing,
    kPluginBridgeNonRtOpcodeSetAudioPool,            // ulong/ptr
    kPluginBridgeNonRtOpcodeSetBufferSize,           // uint
    kPluginBridgeNonRtOpcodeSetSampleRate,           // double
    kPluginBridgeNonRtOpcodeSetParameterValue,       // int, float
    kPluginBridgeNonRtOpcodeSetParameterMidiChannel, // byte, float
    kPluginBridgeNonRtOpcodeSetParameterMidiCC,      // short, float
    kPluginBridgeNonRtOpcodeSetProgram,              // int
    kPluginBridgeNonRtOpcodeSetMidiProgram,          // int
    kPluginBridgeNonRtOpcodeSetCustomData,           // uint/size, str, uint/size, str, uint/size, str
    kPluginBridgeNonRtOpcodeSetChunkFile,            // uint/size, str/file
    kPluginBridgeNonRtOpcodePrepareForSave,
    kPluginBridgeNonRtOpcodeShowUI,                  // bool
    kPluginBridgeNonRtOpcodeQuit
};

const char* const CARLA_BRIDGE_MSG_HIDE_GUI = "CarlaBridgeHideGUI"; //!< Plugin -> Host configure, tells host GUI is now hidden
const char* const CARLA_BRIDGE_MSG_SAVED    = "CarlaBridgeSaved";   //!< Plugin -> Host configure, tells host state is saved

// -----------------------------------------------------------------------

struct BridgeSemaphore {
    union {
        void* server;
        char _padServer[32];
    };
    union {
        void* client;
        char _padClient[32];
    };
};

struct BridgeTimeInfo {
    bool playing;
    uint64_t frame;
    uint64_t usecs;
    uint     valid;
    // bbt
    int32_t bar, beat, tick;
    float beatsPerBar, beatType;
    double barStartTick, ticksPerBeat, beatsPerMinute;
};

struct BridgeShmRtData {
    BridgeSemaphore sem;
    BridgeTimeInfo timeInfo;
    StackBuffer ringBuffer;
};

struct BridgeShmNonRtData {
    BridgeSemaphore sem;
    BigStackBuffer ringBuffer;
};

// -----------------------------------------------------------------------

static inline
const char* PluginBridgeInfoType2str(const PluginBridgeInfoType type) noexcept
{
    switch (type)
    {
    case kPluginBridgePong:
        return "kPluginBridgePong";
    case kPluginBridgePluginInfo1:
        return "kPluginBridgePluginInfo1";
    case kPluginBridgePluginInfo2:
        return "kPluginBridgePluginInfo2";
    case kPluginBridgeAudioCount:
        return "kPluginBridgeAudioCount";
    case kPluginBridgeMidiCount:
        return "kPluginBridgeMidiCount";
    case kPluginBridgeParameterCount:
        return "kPluginBridgeParameterCount";
    case kPluginBridgeProgramCount:
        return "kPluginBridgeProgramCount";
    case kPluginBridgeMidiProgramCount:
        return "kPluginBridgeMidiProgramCount";
    case kPluginBridgeParameterData:
        return "kPluginBridgeParameterData";
    case kPluginBridgeParameterRanges1:
        return "kPluginBridgeParameterRanges1";
    case kPluginBridgeParameterRanges2:
        return "kPluginBridgeParameterRanges2";
    case kPluginBridgeParameterMidiCC:
        return "kPluginBridgeParameterMidiCC";
    case kPluginBridgeParameterMidiChannel:
        return "kPluginBridgeParameterMidiChannel";
    case kPluginBridgeParameterValue:
        return "kPluginBridgeParameterValue";
    case kPluginBridgeDefaultValue:
        return "kPluginBridgeDefaultValue";
    case kPluginBridgeCurrentProgram:
        return "kPluginBridgeCurrentProgram";
    case kPluginBridgeCurrentMidiProgram:
        return "kPluginBridgeCurrentMidiProgram";
    case kPluginBridgeProgramName:
        return "kPluginBridgeProgramName";
    case kPluginBridgeMidiProgramData:
        return "kPluginBridgeMidiProgramData";
    case kPluginBridgeConfigure:
        return "kPluginBridgeConfigure";
    case kPluginBridgeSetCustomData:
        return "kPluginBridgeSetCustomData";
    case kPluginBridgeSetChunkData:
        return "kPluginBridgeSetChunkData";
    case kPluginBridgeUpdateNow:
        return "kPluginBridgeUpdateNow";
    case kPluginBridgeError:
        return "kPluginBridgeError";
    }

    carla_stderr("CarlaBackend::PluginBridgeInfoType2str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* PluginBridgeOpcode2str(const PluginBridgeOpcode opcode) noexcept
{
    switch (opcode)
    {
    case kPluginBridgeOpcodeNull:
        return "kPluginBridgeOpcodeNull";
    case kPluginBridgeOpcodeSetAudioPool:
        return "kPluginBridgeOpcodeSetAudioPool";
    case kPluginBridgeOpcodeSetBufferSize:
        return "kPluginBridgeOpcodeSetBufferSize";
    case kPluginBridgeOpcodeSetSampleRate:
        return "kPluginBridgeOpcodeSetSampleRate";
    case kPluginBridgeOpcodeSetParameterRt:
        return "kPluginBridgeOpcodeSetParameterRt";
    case kPluginBridgeOpcodeSetParameterNonRt:
        return "kPluginBridgeOpcodeSetParameterNonRt";
    case kPluginBridgeOpcodeSetProgram:
        return "kPluginBridgeOpcodeSetProgram";
    case kPluginBridgeOpcodeSetMidiProgram:
        return "kPluginBridgeOpcodeSetMidiProgram";
    case kPluginBridgeOpcodeMidiEvent:
        return "kPluginBridgeOpcodeMidiEvent";
    case kPluginBridgeOpcodeProcess:
        return "kPluginBridgeOpcodeProcess";
    case kPluginBridgeOpcodeQuit:
        return "kPluginBridgeOpcodeQuit";
    }

    carla_stderr("CarlaBackend::PluginBridgeOpcode2str(%i) - invalid opcode", opcode);
    return nullptr;
}

// -----------------------------------------------------------------------

#endif // CARLA_BRIDGE_UTILS_HPP_INCLUDED
