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

//#include "CarlaBackend.h"
#include "CarlaEngine.hpp"
#include "CarlaRingBuffer.hpp"

// -----------------------------------------------------------------------

enum PluginBridgeInfoType {
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
    kPluginBridgeUpdateNow,            //
    kPluginBridgeError                 //
};

enum PluginBridgeOpcode {
    kPluginBridgeOpcodeNull           = 0,
    kPluginBridgeOpcodeSetAudioPool   = 1, // long
    kPluginBridgeOpcodeSetBufferSize  = 2, // int
    kPluginBridgeOpcodeSetSampleRate  = 3, // float
    kPluginBridgeOpcodeSetParameter   = 4, // int, float
    kPluginBridgeOpcodeSetProgram     = 5, // int
    kPluginBridgeOpcodeSetMidiProgram = 6, // int
    kPluginBridgeOpcodeMidiEvent      = 7, // long, int, char[] (long = timeFrame, int = size max 4)
    kPluginBridgeOpcodeProcess        = 8,
    kPluginBridgeOpcodeQuit           = 9
};

const char* const CARLA_BRIDGE_MSG_HIDE_GUI   = "CarlaBridgeHideGUI";   //!< Plugin -> Host call, tells host GUI is now hidden
const char* const CARLA_BRIDGE_MSG_SAVED      = "CarlaBridgeSaved";     //!< Plugin -> Host call, tells host state is saved
#if 0
const char* const CARLA_BRIDGE_MSG_SAVE_NOW   = "CarlaBridgeSaveNow";   //!< Host -> Plugin call, tells plugin to save state now
const char* const CARLA_BRIDGE_MSG_SET_CHUNK  = "CarlaBridgeSetChunk";  //!< Host -> Plugin call, tells plugin to set chunk in file \a value
const char* const CARLA_BRIDGE_MSG_SET_CUSTOM = "CarlaBridgeSetCustom"; //!< Host -> Plugin call, tells plugin to set a custom data set using \a value ("type·key·rvalue").
//If \a type is 'chunk' or 'binary' \a rvalue refers to chunk file.
#endif

// -----------------------------------------------------------------------

PRE_PACKED_STRUCTURE
struct BridgeTimeInfo {
    bool playing;
    uint64_t frame;
    uint64_t usecs;
    uint     valid;
    // bbt
    int32_t bar, beat, tick;
    float beatsPerBar, beatType;
    double barStartTick, ticksPerBeat, beatsPerMinute;
} POST_PACKED_STRUCTURE;

// -----------------------------------------------------------------------

PRE_PACKED_STRUCTURE
struct BridgeShmControl {
    union {
        void* runServer;
        char _padServer[32];
    };
    union {
        void* runClient;
        char _padClient[32];
    };
    StackRingBuffer ringBuffer;
} POST_PACKED_STRUCTURE;

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
    case kPluginBridgeOpcodeSetParameter:
        return "kPluginBridgeOpcodeSetParameter";
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
