/*
 * Carla JACK API for external applications
 * Copyright (C) 2016-2017 Filipe Coelho <falktx@falktx.com>
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

// need to include this first
#include "CarlaDefines.h"

// now define as bridge
#define BUILD_BRIDGE 1

// now include a bunch of stuff
#include "CarlaBackendUtils.hpp"
#include "CarlaBridgeUtils.hpp"
#include "CarlaMIDI.h"
#include "CarlaMutex.hpp"
#include "LinkedList.hpp"

#if 0
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/transport.h>
#include <jack/session.h>
#include <jack/metadata.h>
#endif

#include <cerrno>

#ifdef __SSE2_MATH__
# include <xmmintrin.h>
#endif

// must be last include
#include "jackbridge/JackBridge.hpp"

// small check to not hurt myself
#ifdef JACKBRIDGE_DIRECT
# error "Cannot create custom jack server while linking to libjack directly"
#endif

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

class CarlaJackAppClient;
struct JackClientState;
struct JackServerState;

struct JackMidiPortBuffer {
    static const uint8_t kMaxEventSize   = 128;
    static const size_t  kMaxEventCount  = 512;
    static const size_t  kBufferPoolSize = kMaxEventCount*8;

    uint16_t count;
    bool isInput;
    jack_midi_event_t* events;

    size_t bufferPoolPos;
    jack_midi_data_t* bufferPool;

    JackMidiPortBuffer()
        : count(0),
          isInput(true),
          events(new jack_midi_event_t[kMaxEventCount]),
          bufferPoolPos(0),
          bufferPool(new jack_midi_data_t[kBufferPoolSize]) {}

    // for unused ports
    JackMidiPortBuffer(const bool input, const char*)
        : count(0),
          isInput(input),
          events(nullptr),
          bufferPoolPos(kBufferPoolSize),
          bufferPool(nullptr) {}

    ~JackMidiPortBuffer()
    {
        delete[] events;
        delete[] bufferPool;
    }

    CARLA_DECLARE_NON_COPY_STRUCT(JackMidiPortBuffer)
};

struct JackPortState {
    char* name;
    char* fullname;
    void* buffer;
    uint index;
    uint flags;
    jack_uuid_t uuid;
    bool isMidi : 1;
    bool isSystem : 1;
    bool isConnected : 1;
    bool unused : 1;

    JackPortState()
        : name(nullptr),
          fullname(nullptr),
          buffer(nullptr),
          index(0),
          flags(0),
          uuid(0),
          isMidi(false),
          isSystem(false),
          isConnected(false),
          unused(false) {}

    JackPortState(const char* const clientName, const char* const portName, const uint i, const uint f,
                  const bool midi, const bool sys, const bool con)
        : name(portName != nullptr ? strdup(portName) : nullptr),
          fullname(nullptr),
          buffer(nullptr),
          index(i),
          flags(f),
          uuid(0),
          isMidi(midi),
          isSystem(sys),
          isConnected(con),
          unused(false)
    {
        if (clientName != nullptr && portName != nullptr)
        {
            char strBuf[STR_MAX+1];
            snprintf(strBuf, STR_MAX, "%s:%s", clientName, portName);
            strBuf[STR_MAX] = '\0';

            fullname = strdup(strBuf);
        }
    }

    ~JackPortState()
    {
        std::free(name);
        std::free(fullname);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(JackPortState)
};

struct JackClientState {
    const JackServerState& server;
    CarlaMutex mutex;

    bool activated;
    bool deactivated; // activated once, then deactivated

    char* name;

    LinkedList<JackPortState*> audioIns;
    LinkedList<JackPortState*> audioOuts;
    LinkedList<JackPortState*> midiIns;
    LinkedList<JackPortState*> midiOuts;

    JackShutdownCallback shutdownCb;
    void* shutdownCbPtr;

    JackInfoShutdownCallback infoShutdownCb;
    void* infoShutdownCbPtr;

    JackProcessCallback processCb;
    void* processCbPtr;

    JackFreewheelCallback freewheelCb;
    void* freewheelCbPtr;

    JackBufferSizeCallback bufferSizeCb;
    void* bufferSizeCbPtr;

    JackSampleRateCallback sampleRateCb;
    void* sampleRateCbPtr;

    JackSyncCallback syncCb;
    void* syncCbPtr;

    JackClientState(const JackServerState& s, const char* const n)
        : server(s),
          mutex(),
          activated(false),
          deactivated(false),
          name(strdup(n)),
          audioIns(),
          audioOuts(),
          midiIns(),
          midiOuts(),
          shutdownCb(nullptr),
          shutdownCbPtr(nullptr),
          infoShutdownCb(nullptr),
          infoShutdownCbPtr(nullptr),
          processCb(nullptr),
          processCbPtr(nullptr),
          freewheelCb(nullptr),
          freewheelCbPtr(nullptr),
          bufferSizeCb(nullptr),
          bufferSizeCbPtr(nullptr),
          sampleRateCb(nullptr),
          sampleRateCbPtr(nullptr),
          syncCb(nullptr),
          syncCbPtr(nullptr) {}

    ~JackClientState()
    {
        const CarlaMutexLocker cms(mutex);

        for (LinkedList<JackPortState*>::Itenerator it = audioIns.begin2(); it.valid(); it.next())
        {
            if (JackPortState* const jport = it.getValue(nullptr))
                delete jport;
        }

        for (LinkedList<JackPortState*>::Itenerator it = audioOuts.begin2(); it.valid(); it.next())
        {
            if (JackPortState* const jport = it.getValue(nullptr))
                delete jport;
        }

        for (LinkedList<JackPortState*>::Itenerator it = midiIns.begin2(); it.valid(); it.next())
        {
            if (JackPortState* const jport = it.getValue(nullptr))
                delete jport;
        }

        for (LinkedList<JackPortState*>::Itenerator it = midiOuts.begin2(); it.valid(); it.next())
        {
            if (JackPortState* const jport = it.getValue(nullptr))
                delete jport;
        }

        std::free(name);
        name = nullptr;

        audioIns.clear();
        audioOuts.clear();
        midiIns.clear();
        midiOuts.clear();
    }

    CARLA_DECLARE_NON_COPY_STRUCT(JackClientState)
};

struct JackServerState {
    CarlaJackAppClient* jackAppPtr;

    uint32_t bufferSize;
    double   sampleRate;

    uint8_t numAudioIns;
    uint8_t numAudioOuts;
    uint8_t numMidiIns;
    uint8_t numMidiOuts;

    bool playing;
    jack_position_t position;

    JackServerState(CarlaJackAppClient* const app)
        : jackAppPtr(app),
          bufferSize(0),
          sampleRate(0.0),
          numAudioIns(0),
          numAudioOuts(0),
          numMidiIns(0),
          numMidiOuts(0),
          playing(false),
          position()
    {
        carla_zeroStruct(position);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(JackServerState)
};

CARLA_BACKEND_END_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
