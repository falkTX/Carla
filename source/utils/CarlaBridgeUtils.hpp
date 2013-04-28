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

enum PluginBridgeOpcode {
    kPluginBridgeOpcodeNull           = 0,
    kPluginBridgeOpcodeReadyWait      = 1,
    kPluginBridgeOpcodeSetBufferSize  = 2, // int
    kPluginBridgeOpcodeSetSampleRate  = 3, // float
    kPluginBridgeOpcodeSetParameter   = 4, // int, float
    kPluginBridgeOpcodeSetProgram     = 5, // int
    kPluginBridgeOpcodeSetMidiProgram = 6, // int
    kPluginBridgeOpcodeMidiEvent      = 7, // int, char, ... (int = size, max 4)
    kPluginBridgeOpcodeProcess        = 8,
    kPluginBridgeOpcodeQuit           = 9
};

/*!
 * @defgroup BridgeMessages Bridge Messages
 *
 * Various bridge related messages, used as configure(<message>, value).
 * \note This is for internal use only.
 *
 * TODO: Review these, may not be needed anymore
 * @{
 */
const char* const CARLA_BRIDGE_MSG_HIDE_GUI   = "CarlaBridgeHideGUI";   //!< Plugin -> Host call, tells host GUI is now hidden
const char* const CARLA_BRIDGE_MSG_SAVED      = "CarlaBridgeSaved";     //!< Plugin -> Host call, tells host state is saved
#if 0
const char* const CARLA_BRIDGE_MSG_SAVE_NOW   = "CarlaBridgeSaveNow";   //!< Host -> Plugin call, tells plugin to save state now
const char* const CARLA_BRIDGE_MSG_SET_CHUNK  = "CarlaBridgeSetChunk";  //!< Host -> Plugin call, tells plugin to set chunk in file \a value
const char* const CARLA_BRIDGE_MSG_SET_CUSTOM = "CarlaBridgeSetCustom"; //!< Host -> Plugin call, tells plugin to set a custom data set using \a value ("type·key·rvalue").
//If \a type is 'chunk' or 'binary' \a rvalue refers to chunk file.
#endif
/**@}*/

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

static inline
const char* PluginBridgeInfoType2str(const PluginBridgeInfoType type)
{
    switch (type)
    {
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
    case kPluginBridgePluginInfo:
        return "kPluginBridgePluginInfo";
    case kPluginBridgeParameterInfo:
        return "kPluginBridgeParameterInfo";
    case kPluginBridgeParameterData:
        return "kPluginBridgeParameterData";
    case kPluginBridgeParameterRanges:
        return "kPluginBridgeParameterRanges";
    case kPluginBridgeProgramInfo:
        return "kPluginBridgeProgramInfo";
    case kPluginBridgeMidiProgramInfo:
        return "kPluginBridgeMidiProgramInfo";
    case kPluginBridgeConfigure:
        return "kPluginBridgeConfigure";
    case kPluginBridgeSetParameterValue:
        return "kPluginBridgeSetParameterValue";
    case kPluginBridgeSetDefaultValue:
        return "kPluginBridgeSetDefaultValue";
    case kPluginBridgeSetProgram:
        return "kPluginBridgeSetProgram";
    case kPluginBridgeSetMidiProgram:
        return "kPluginBridgeSetMidiProgram";
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

// ---------------------------------------------------------------------------------------------

static inline
void rdwr_tryRead(BridgeRingBuffer* const ringbuf, void* const buf, const size_t size)
{
    char* const charbuf(static_cast<char*>(buf));

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
    const char* const charbuf(static_cast<const char*>(buf));

    size_t written = ringbuf->written;
    size_t tail = ringbuf->tail;
    size_t wrap = 0;

    if (tail <= written)
    {
        wrap = BRIDGE_SHM_RING_BUFFER_SIZE;
    }
    if (tail - written + wrap < size)
    {
        carla_stderr2("Ring buffer full! Dropping events.");
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
    int i = kPluginBridgeOpcodeNull;
    rdwr_tryRead(ringbuf, &i, sizeof(int));
    return static_cast<PluginBridgeOpcode>(i);
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
    const int i = opcode;
    rdwr_tryWrite(ringbuf, &i, sizeof(int));
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
