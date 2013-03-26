/*
 * Carla Bridge utils imported from dssi-vst code
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2004-2010 Chris Cannam <cannam@all-day-breakfast.com>
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

#ifndef __CARLA_BRIDGE_UTILS_HPP__
#define __CARLA_BRIDGE_UTILS_HPP__

#include "CarlaUtils.hpp"

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

enum PluginBridgeOpcode {
    kPluginBridgeOpcodeNull       = 0,
    kPluginBridgeOpcodeReadyWait  = 1,
    kPluginBridgeOpcodeBufferSize = 2,
    kPluginBridgeOpcodeSampleRate = 3,
    kPluginBridgeOpcodeProcess    = 4
};

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

// ---------------------------------------------------------------------------------------------

static inline
void rdwr_tryRead(BridgeRingBuffer* const ringbuf, void* const buf, const size_t size)
{
    char* const charbuf = static_cast<char*>(buf);

    size_t tail = ringbuf->tail;
    size_t head = ringbuf->head;
    size_t wrap = 0;

    if (head <= tail) {
        wrap = BRIDGE_SHM_RING_BUFFER_SIZE;
    }
    if (head - tail + wrap < size) {
        return;
    }

    size_t readto = tail + size;

    if (readto >= BRIDGE_SHM_RING_BUFFER_SIZE)
    {
        readto -= BRIDGE_SHM_RING_BUFFER_SIZE;
        size_t firstpart = BRIDGE_SHM_RING_BUFFER_SIZE - tail;
        std::memcpy(charbuf, ringbuf->buf + tail, firstpart);
        std::memcpy(charbuf + firstpart, ringbuf->buf, readto);
    }
    else
    {
        std::memcpy(charbuf, ringbuf->buf + tail, size);
    }

    ringbuf->tail = readto;
}

static inline
void rdwr_tryWrite(BridgeRingBuffer* const ringbuf, const void* const buf, const size_t size)
{
    const char* const charbuf = static_cast<const char*>(buf);

    size_t written = ringbuf->written;
    size_t tail = ringbuf->tail;
    size_t wrap = 0;

    if (tail <= written)
    {
        wrap = BRIDGE_SHM_RING_BUFFER_SIZE;
    }
    if (tail - written + wrap < size)
    {
        carla_stderr2("Operation ring buffer full! Dropping events.");
        ringbuf->invalidateCommit = true;
        return;
    }

    size_t writeto = written + size;

    if (writeto >= BRIDGE_SHM_RING_BUFFER_SIZE)
    {
        writeto -= BRIDGE_SHM_RING_BUFFER_SIZE;
        size_t firstpart = BRIDGE_SHM_RING_BUFFER_SIZE - written;
        std::memcpy(ringbuf->buf + written, charbuf, firstpart);
        std::memcpy(ringbuf->buf, charbuf + firstpart, writeto);
    }
    else
    {
        std::memcpy(ringbuf->buf + written, charbuf, size);
    }

    ringbuf->written = writeto;
}

static inline
void rdwr_commitWrite(BridgeRingBuffer* const ringbuf)
{
    if (ringbuf->invalidateCommit)
    {
        ringbuf->written = ringbuf->head;
        ringbuf->invalidateCommit = false;
    }
    else
    {
        ringbuf->head = ringbuf->written;
    }
}

// ---------------------------------------------------------------------------------------------

static inline
bool rdwr_dataAvailable(BridgeRingBuffer* const ringbuf)
{
    return (ringbuf->tail != ringbuf->head);
}

static inline
PluginBridgeOpcode rdwr_readOpcode(BridgeRingBuffer* const ringbuf)
{
    PluginBridgeOpcode code = kPluginBridgeOpcodeNull;
    rdwr_tryRead(ringbuf, &code, sizeof(PluginBridgeOpcode));
    return code;
}

static inline
int rdwr_readInt(BridgeRingBuffer* const ringbuf)
{
    int i = 0;
    rdwr_tryRead(ringbuf, &i, sizeof(int));
    return i;
}

static inline
float rdwr_readFloat(BridgeRingBuffer* const ringbuf)
{
    float f = 0.0f;
    rdwr_tryRead(ringbuf, &f, sizeof(float));
    return f;
}

// ---------------------------------------------------------------------------------------------

static inline
void rdwr_writeOpcode(BridgeRingBuffer* const ringbuf, const PluginBridgeOpcode opcode)
{
    rdwr_tryWrite(ringbuf, &opcode, sizeof(PluginBridgeOpcode));
}

static inline
void rdwr_writeInt(BridgeRingBuffer* const ringbuf, const int value)
{
    rdwr_tryWrite(ringbuf, &value, sizeof(int));
}

static inline
void rdwr_writeFloat(BridgeRingBuffer* const ringbuf, const float value)
{
    rdwr_tryWrite(ringbuf, &value, sizeof(float));
}

// ---------------------------------------------------------------------------------------------

#endif // __CARLA_BRIDGE_UTILS_HPP__
