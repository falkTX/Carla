/*
 * Carla Bridge API
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_BRIDGE_HPP__
#define __CARLA_BRIDGE_HPP__

#include <semaphore.h>

#define BRIDGE_SHM_RING_BUFFER_SIZE 2048

#ifndef BUILD_BRIDGE
/*!
 * TODO.
 */
enum PluginBridgeInfoType {
    kPluginBridgeAudioCount,
    kPluginBridgeMidiCount,
    kPluginBridgeParameterCount,
    kPluginBridgeProgramCount,
    kPluginBridgeMidiProgramCount,
    kPluginBridgePluginInfo,
    kPluginBridgeParameterInfo,
    kPluginBridgeParameterData,
    kPluginBridgeParameterRanges,
    kPluginBridgeProgramInfo,
    kPluginBridgeMidiProgramInfo,
    kPluginBridgeConfigure,
    kPluginBridgeSetParameterValue,
    kPluginBridgeSetDefaultValue,
    kPluginBridgeSetProgram,
    kPluginBridgeSetMidiProgram,
    kPluginBridgeSetCustomData,
    kPluginBridgeSetChunkData,
    kPluginBridgeUpdateNow,
    kPluginBridgeError
};
#endif

/*!
 * TODO.
 */
struct BridgeRingBuffer {
    int head;
    int tail;
    int written;
    bool invalidateCommit;
    char buf[BRIDGE_SHM_RING_BUFFER_SIZE];
};

/*!
 * TODO.
 */
struct BridgeShmControl {
    // 32 and 64-bit binaries align semaphores differently.
    // Let's make sure there's plenty of room for either one.
    union {
        sem_t runServer;
        char _alignServer[32];
    };
    union {
        sem_t runClient;
        char _alignClient[32];
    };

    BridgeRingBuffer ringBuffer;
};

#endif // __CARLA_BRIDGE_HPP__
