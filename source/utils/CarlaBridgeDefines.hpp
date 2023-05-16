/*
 * Carla Bridge definitions
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

#ifndef CARLA_BRIDGE_DEFINES_HPP_INCLUDED
#define CARLA_BRIDGE_DEFINES_HPP_INCLUDED

#include "CarlaDefines.h"

// how much backwards compatible we are
#define CARLA_PLUGIN_BRIDGE_API_VERSION_MINIMUM 6

// current API version, bumped when something is added
#define CARLA_PLUGIN_BRIDGE_API_VERSION_CURRENT 10

// -------------------------------------------------------------------------------------------------------------------

#ifdef CARLA_PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

// -------------------------------------------------------------------------------------------------------------------

// Server sends these to client during RT
enum PluginBridgeRtClientOpcode {
    kPluginBridgeRtClientNull = 0,
    kPluginBridgeRtClientSetAudioPool,            // ulong/ptr
    kPluginBridgeRtClientSetBufferSize,           // uint
    kPluginBridgeRtClientSetSampleRate,           // double
    kPluginBridgeRtClientSetOnline,               // bool
    kPluginBridgeRtClientControlEventParameter,   // uint/frame, byte/chan, ushort, float
    kPluginBridgeRtClientControlEventMidiBank,    // uint/frame, byte/chan, ushort
    kPluginBridgeRtClientControlEventMidiProgram, // uint/frame, byte/chan, ushort
    kPluginBridgeRtClientControlEventAllSoundOff, // uint/frame, byte/chan
    kPluginBridgeRtClientControlEventAllNotesOff, // uint/frame, byte/chan
    kPluginBridgeRtClientMidiEvent,               // uint/frame, byte/port, byte/size, byte[]/data
    kPluginBridgeRtClientProcess,                 // uint/frames
    kPluginBridgeRtClientQuit
};

// Server sends these to client during non-RT
enum PluginBridgeNonRtClientOpcode {
    kPluginBridgeNonRtClientNull = 0,
    kPluginBridgeNonRtClientVersion,                        // uint
    kPluginBridgeNonRtClientPing,
    kPluginBridgeNonRtClientPingOnOff,                      // bool
    kPluginBridgeNonRtClientActivate,
    kPluginBridgeNonRtClientDeactivate,
    kPluginBridgeNonRtClientInitialSetup,                   // uint, double
    kPluginBridgeNonRtClientSetParameterValue,              // uint, float
    kPluginBridgeNonRtClientSetParameterMidiChannel,        // uint, byte
    kPluginBridgeNonRtClientSetParameterMappedControlIndex, // uint, short
    kPluginBridgeNonRtClientSetProgram,                     // int
    kPluginBridgeNonRtClientSetMidiProgram,                 // int
    kPluginBridgeNonRtClientSetCustomData,                  // uint/size, str[], uint/size, str[], uint/size, str[]
    kPluginBridgeNonRtClientSetChunkDataFile,               // uint/size, str[] (filename, base64 content)
    kPluginBridgeNonRtClientSetCtrlChannel,                 // short
    kPluginBridgeNonRtClientSetOption,                      // uint/option, bool
    kPluginBridgeNonRtClientGetParameterText,               // uint
    kPluginBridgeNonRtClientPrepareForSave,
    kPluginBridgeNonRtClientRestoreLV2State,
    kPluginBridgeNonRtClientShowUI,
    kPluginBridgeNonRtClientHideUI,
    kPluginBridgeNonRtClientUiParameterChange,              // uint, float
    kPluginBridgeNonRtClientUiProgramChange,                // uint
    kPluginBridgeNonRtClientUiMidiProgramChange,            // uint
    kPluginBridgeNonRtClientUiNoteOn,                       // byte, byte, byte
    kPluginBridgeNonRtClientUiNoteOff,                      // byte, byte
    kPluginBridgeNonRtClientQuit,
    // stuff added in API 7
    kPluginBridgeNonRtClientSetParameterMappedRange,        // uint, float, float
    kPluginBridgeNonRtClientSetOptions,                     // uint
    // stuff added in API 8
    kPluginBridgeNonRtClientSetWindowTitle,                 // uint/size, str[]
    // stuff added in API 9
    kPluginBridgeNonRtClientEmbedUI,                        // ulong
    // stuff added in API 10
    kPluginBridgeNonRtClientReload,
};

// Client sends these to server during non-RT
enum PluginBridgeNonRtServerOpcode {
    kPluginBridgeNonRtServerNull = 0,
    kPluginBridgeNonRtServerPong,
    kPluginBridgeNonRtServerPluginInfo1,        // uint/category, uint/hints, uint/optionsAvailable, uint/optionsEnabled, long/uniqueId
    kPluginBridgeNonRtServerPluginInfo2,        // uint/size, str[] (realName), uint/size, str[] (label), uint/size, str[] (maker), uint/size, str[] (copyright)
    kPluginBridgeNonRtServerAudioCount,         // uint/ins, uint/outs
    kPluginBridgeNonRtServerMidiCount,          // uint/ins, uint/outs
    kPluginBridgeNonRtServerCvCount,            // uint/ins, uint/outs
    kPluginBridgeNonRtServerParameterCount,     // uint/count
    kPluginBridgeNonRtServerProgramCount,       // uint/count
    kPluginBridgeNonRtServerMidiProgramCount,   // uint/count
    kPluginBridgeNonRtServerPortName,           // byte/type, uint/index, uint/size, str[] (name)
    kPluginBridgeNonRtServerParameterData1,     // uint/index, int/rindex, uint/type, uint/hints, short/cc
    kPluginBridgeNonRtServerParameterData2,     // uint/index, uint/size, str[] (name), uint/size, str[] (symbol), uint/size, str[] (unit)
    kPluginBridgeNonRtServerParameterRanges,    // uint/index, float/def, float/min, float/max, float/step, float/stepSmall, float/stepLarge
    kPluginBridgeNonRtServerParameterValue,     // uint/index, float/value
    kPluginBridgeNonRtServerParameterValue2,    // uint/index, float/value (used for init/output parameters only, don't resend values)
    kPluginBridgeNonRtServerParameterTouch,     // uint/index, bool/touch
    kPluginBridgeNonRtServerDefaultValue,       // uint/index, float/value
    kPluginBridgeNonRtServerCurrentProgram,     // int/index
    kPluginBridgeNonRtServerCurrentMidiProgram, // int/index
    kPluginBridgeNonRtServerProgramName,        // uint/index, uint/size, str[] (name)
    kPluginBridgeNonRtServerMidiProgramData,    // uint/index, uint/bank, uint/program, uint/size, str[] (name)
    kPluginBridgeNonRtServerSetCustomData,      // uint/size, str[], uint/size, str[], uint/size, str[]
    kPluginBridgeNonRtServerSetChunkDataFile,   // uint/size, str[] (filename, base64 content)
    kPluginBridgeNonRtServerSetLatency,         // uint/latency
    kPluginBridgeNonRtServerSetParameterText,   // uint/index, uint/size, str[] (name)
    kPluginBridgeNonRtServerReady,
    kPluginBridgeNonRtServerSaved,
    kPluginBridgeNonRtServerUiClosed,
    kPluginBridgeNonRtServerError,              // uint/size, str[]
    // stuff added in API 7
    kPluginBridgeNonRtServerVersion,            // uint/version
    // stuff added in API 9
    kPluginBridgeNonRtServerRespEmbedUI,        // ulong/window-id
    kPluginBridgeNonRtServerResizeEmbedUI,      // uint/width, uint/height
};

// used for kPluginBridgeNonRtServerPortName
enum PluginBridgePortType {
    kPluginBridgePortNull = 0,
    kPluginBridgePortAudioInput,
    kPluginBridgePortAudioOutput,
    kPluginBridgePortCvInput,
    kPluginBridgePortCvOutput,
    kPluginBridgePortMidiInput,
    kPluginBridgePortMidiOutput,
    kPluginBridgePortTypeCount
};

enum PluginBridgeTimeInfoFlags {
    kPluginBridgeTimeInfoValidBBT = 0x1
};

// -------------------------------------------------------------------------------------------------------------------

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

// NOTE: needs to be 64bit aligned
struct BridgeTimeInfo {
    uint64_t playing;
    uint64_t frame;
    uint64_t usecs;
    uint32_t validFlags;
    uint32_t unused;
    // bbt
    int32_t bar, beat;
    float beatsPerBar, beatType;
    double tick, barStartTick, ticksPerBeat, beatsPerMinute;
};

// -------------------------------------------------------------------------------------------------------------------

#endif // CARLA_BRIDGE_DEFINES_HPP_INCLUDED
