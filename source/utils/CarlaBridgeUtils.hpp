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

// carla-plugin receives these via osc
enum PluginBridgeOscInfoType {
    kPluginBridgeOscNull = 0,
    kPluginBridgeOscPong,
    kPluginBridgeOscPluginInfo1,          // uuuuh => category, hints, optionsAvailable, optionsEnabled, uniqueId
    kPluginBridgeOscPluginInfo2,          // ssss  => realName, label, maker, copyright
    kPluginBridgeOscAudioCount,           // uu    => ins, outs
    kPluginBridgeOscMidiCount,            // uu    => ins, outs
    kPluginBridgeOscParameterCount,       // uu    => ins, outs
    kPluginBridgeOscProgramCount,         // u     => count
    kPluginBridgeOscMidiProgramCount,     // u     => count
    kPluginBridgeOscParameterData1,       // uiuui => index, rindex, type, hints, cc
    kPluginBridgeOscParameterData2,       // uss   => index, name, unit
    kPluginBridgeOscParameterRanges1,     // ufff  => index, def, min, max
    kPluginBridgeOscParameterRanges2,     // ufff  => index, step, stepSmall, stepLarge
    kPluginBridgeOscParameterValue,       // uf    => index, value
    kPluginBridgeOscDefaultValue,         // uf    => index, value
    kPluginBridgeOscCurrentProgram,       // i     => index
    kPluginBridgeOscCurrentMidiProgram,   // i     => index
    kPluginBridgeOscProgramName,          // us    => index, name
    kPluginBridgeOscMidiProgramData,      // uuus  => index, bank, program, name
    kPluginBridgeOscConfigure,            // ss    => key, value
    kPluginBridgeOscSetCustomData,        // sss   => type, key, value
    kPluginBridgeOscSetChunkDataFile,     // s     => chunkFile
    kPluginBridgeOscLatency,              // u     => value
    kPluginBridgeOscReady,
    kPluginBridgeOscError
};

// carla-plugin sends these during RT
enum PluginBridgeRtOpcode {
    kPluginBridgeRtNull = 0,
    kPluginBridgeRtSetAudioPool,   // ulong/ptr
    kPluginBridgeRtSetParameter,   // uint, float
    kPluginBridgeRtMidiData,       // uint/frame, uint/size, char[]
    kPluginBridgeRtMidiBank,       // uint/frame, byte/chan, ushort
    kPluginBridgeRtMidiProgram,    // uint/frame, byte/chan, ushort
    kPluginBridgeRtAllSoundOff,    // uint/frame
    kPluginBridgeRtAllNotesOff,    // uint/frame
    kPluginBridgeRtProcess
};

// carla-plugin sends these during non-RT
enum PluginBridgeNonRtOpcode {
    kPluginBridgeNonRtNull = 0,
    kPluginBridgeNonRtPing,
    kPluginBridgeNonRtActivate,
    kPluginBridgeNonRtDeactivate,
    kPluginBridgeNonRtSetBufferSize,           // uint
    kPluginBridgeNonRtSetSampleRate,           // double
    kPluginBridgeNonRtSetOffline,
    kPluginBridgeNonRtSetOnline,
    kPluginBridgeNonRtSetParameterValue,       // uint, float
    kPluginBridgeNonRtSetProgram,              // int
    kPluginBridgeNonRtSetMidiProgram,          // int
    kPluginBridgeNonRtSetCustomData,           // uint/size, str, uint/size, str, uint/size, str
    kPluginBridgeNonRtSetChunkDataFile,        // uint/size, str/file
    kPluginBridgeNonRtSetCtrlChannel,          // short
    kPluginBridgeNonRtSetOption,               // uint/option, bool
    kPluginBridgeNonRtPrepareForSave,
    kPluginBridgeNonRtShowUI,
    kPluginBridgeNonRtHideUI,
    kPluginBridgeNonRtUiParameterChange,       // uint, float
    kPluginBridgeNonRtUiProgramChange,         // uint
    kPluginBridgeNonRtUiMidiProgramChange,     // uint
    kPluginBridgeNonRtUiNoteOn,                // byte, byte, byte
    kPluginBridgeNonRtUiNoteOff,               // byte, byte
    kPluginBridgeNonRtQuit
};

// -----------------------------------------------------------------------

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

struct BridgeRtData {
    BridgeSemaphore sem;
    BridgeTimeInfo timeInfo;
    StackBuffer ringBuffer;
};

struct BridgeNonRtData {
    BigStackBuffer ringBuffer;
};

// -----------------------------------------------------------------------

static inline
const char* PluginBridgeOscInfoType2str(const PluginBridgeOscInfoType type) noexcept
{
    switch (type)
    {
    case kPluginBridgeOscNull:
        return "kPluginBridgeOscNull";
    case kPluginBridgeOscPong:
        return "kPluginBridgeOscPong";
    case kPluginBridgeOscPluginInfo1:
        return "kPluginBridgeOscPluginInfo1";
    case kPluginBridgeOscPluginInfo2:
        return "kPluginBridgeOscPluginInfo2";
    case kPluginBridgeOscAudioCount:
        return "kPluginBridgeOscAudioCount";
    case kPluginBridgeOscMidiCount:
        return "kPluginBridgeOscMidiCount";
    case kPluginBridgeOscParameterCount:
        return "kPluginBridgeOscParameterCount";
    case kPluginBridgeOscProgramCount:
        return "kPluginBridgeOscProgramCount";
    case kPluginBridgeOscMidiProgramCount:
        return "kPluginBridgeOscMidiProgramCount";
    case kPluginBridgeOscParameterData1:
        return "kPluginBridgeOscParameterData1";
    case kPluginBridgeOscParameterData2:
        return "kPluginBridgeOscParameterData2";
    case kPluginBridgeOscParameterRanges1:
        return "kPluginBridgeOscParameterRanges1";
    case kPluginBridgeOscParameterRanges2:
        return "kPluginBridgeOscParameterRanges2";
    case kPluginBridgeOscParameterValue:
        return "kPluginBridgeOscParameterValue";
    case kPluginBridgeOscDefaultValue:
        return "kPluginBridgeOscDefaultValue";
    case kPluginBridgeOscCurrentProgram:
        return "kPluginBridgeOscCurrentProgram";
    case kPluginBridgeOscCurrentMidiProgram:
        return "kPluginBridgeOscCurrentMidiProgram";
    case kPluginBridgeOscProgramName:
        return "kPluginBridgeOscProgramName";
    case kPluginBridgeOscMidiProgramData:
        return "kPluginBridgeOscMidiProgramData";
    case kPluginBridgeOscConfigure:
        return "kPluginBridgeOscConfigure";
    case kPluginBridgeOscSetCustomData:
        return "kPluginBridgeOscSetCustomData";
    case kPluginBridgeOscSetChunkDataFile:
        return "kPluginBridgeOscSetChunkDataFile";
    case kPluginBridgeOscLatency:
        return "kPluginBridgeOscLatency";
    case kPluginBridgeOscReady:
        return "kPluginBridgeOscReady";
    case kPluginBridgeOscError:
        return "kPluginBridgeOscError";
    }

    carla_stderr("CarlaBackend::PluginBridgeOscInfoType2str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* PluginBridgeRtOpcode2str(const PluginBridgeRtOpcode opcode) noexcept
{
    switch (opcode)
    {
    case kPluginBridgeRtNull:
        return "kPluginBridgeRtNull";
    case kPluginBridgeRtSetAudioPool:
        return "kPluginBridgeRtSetAudioPool";
    case kPluginBridgeRtSetParameter:
        return "kPluginBridgeRtSetParameter";
    case kPluginBridgeRtMidiData:
        return "kPluginBridgeRtMidiData";
    case kPluginBridgeRtMidiBank:
        return "kPluginBridgeRtMidiBank";
    case kPluginBridgeRtMidiProgram:
        return "kPluginBridgeRtMidiProgram";
    case kPluginBridgeRtAllSoundOff:
        return "kPluginBridgeRtAllSoundOff";
    case kPluginBridgeRtAllNotesOff:
        return "kPluginBridgeRtAllNotesOff";
    case kPluginBridgeRtProcess:
        return "kPluginBridgeRtProcess";
    }

    carla_stderr("CarlaBackend::PluginBridgeRtOpcode2str(%i) - invalid opcode", opcode);
    return nullptr;
}

static inline
const char* PluginBridgeNonRtOpcode2str(const PluginBridgeNonRtOpcode opcode) noexcept
{
    switch (opcode)
    {
    case kPluginBridgeNonRtNull:
        return "kPluginBridgeNonRtNull";
    case kPluginBridgeNonRtPing:
        return "kPluginBridgeNonRtPing";
    case kPluginBridgeNonRtActivate:
        return "kPluginBridgeNonRtActivate";
    case kPluginBridgeNonRtDeactivate:
        return "kPluginBridgeNonRtDeactivate";
    case kPluginBridgeNonRtSetBufferSize:
        return "kPluginBridgeNonRtSetBufferSize";
    case kPluginBridgeNonRtSetSampleRate:
        return "kPluginBridgeNonRtSetSampleRate";
    case kPluginBridgeNonRtSetOffline:
        return "kPluginBridgeNonRtSetOffline";
    case kPluginBridgeNonRtSetOnline:
        return "kPluginBridgeNonRtSetOnline";
    case kPluginBridgeNonRtSetParameterValue:
        return "kPluginBridgeNonRtSetParameterValue";
    case kPluginBridgeNonRtSetProgram:
        return "kPluginBridgeNonRtSetProgram";
    case kPluginBridgeNonRtSetMidiProgram:
        return "kPluginBridgeNonRtSetMidiProgram";
    case kPluginBridgeNonRtSetCustomData:
        return "kPluginBridgeNonRtSetCustomData";
    case kPluginBridgeNonRtSetChunkDataFile:
        return "kPluginBridgeNonRtSetChunkDataFile";
    case kPluginBridgeNonRtSetCtrlChannel:
        return "kPluginBridgeNonRtSetCtrlChannel";
    case kPluginBridgeNonRtSetOption:
        return "kPluginBridgeNonRtSetOption";
    case kPluginBridgeNonRtPrepareForSave:
        return "kPluginBridgeNonRtPrepareForSave";
    case kPluginBridgeNonRtShowUI:
        return "kPluginBridgeNonRtShowUI";
    case kPluginBridgeNonRtHideUI:
        return "kPluginBridgeNonRtHideUI";
    case kPluginBridgeNonRtUiParameterChange:
        return "kPluginBridgeNonRtUiParameterChange";
    case kPluginBridgeNonRtUiProgramChange:
        return "kPluginBridgeNonRtUiProgramChange";
    case kPluginBridgeNonRtUiMidiProgramChange:
        return "kPluginBridgeNonRtUiMidiProgramChange";
    case kPluginBridgeNonRtUiNoteOn:
        return "kPluginBridgeNonRtUiNoteOn";
    case kPluginBridgeNonRtUiNoteOff:
        return "kPluginBridgeNonRtUiNoteOff";
    case kPluginBridgeNonRtQuit:
        return "kPluginBridgeNonRtQuit";
    }

    carla_stderr("CarlaBackend::PluginBridgeNonRtOpcode2str(%i) - invalid opcode", opcode);
    return nullptr;
}

// -----------------------------------------------------------------------

#endif // CARLA_BRIDGE_UTILS_HPP_INCLUDED
