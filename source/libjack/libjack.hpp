/*
 * Carla JACK API for external applications
 * Copyright (C) 2016-2022 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_LIBJACK_HPP_INCLUDED
#define CARLA_LIBJACK_HPP_INCLUDED

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
#include <map>
#include <string>

#ifdef __SSE2_MATH__
# include <xmmintrin.h>
#endif

// must be last include
#include "api.hpp"

// small check to not hurt myself
#ifdef JACKBRIDGE_DIRECT
# error "Cannot create custom jack server while linking to libjack directly"
#endif

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

class CarlaJackAppClient;
struct JackClientState;
struct JackServerState;

struct JackMidiPortBufferBase {
    static const uint8_t kMaxEventSize   = 128;
    static const size_t  kMaxEventCount  = 512;
    static const size_t  kBufferPoolSize = kMaxEventCount*8;

    bool isInput;
    bool isDummy;
};

struct JackMidiPortBufferOnStack : JackMidiPortBufferBase {
    size_t bufferPoolPos;
    uint16_t count;

    jack_midi_event_t events[kMaxEventCount];
    jack_midi_data_t bufferPool[kBufferPoolSize];

    JackMidiPortBufferOnStack()
        : bufferPoolPos(0),
          count(0),
          events(),
          bufferPool()
    {
        isInput = true;
        isDummy = false;
    }

    CARLA_DECLARE_NON_COPYABLE(JackMidiPortBufferOnStack)
};

struct JackMidiPortBufferDummy : JackMidiPortBufferBase {
    JackMidiPortBufferDummy(const bool input)
    {
        isInput = input;
        isDummy = true;
    }

    CARLA_DECLARE_NON_COPYABLE(JackMidiPortBufferDummy)
};

struct JackPortState {
    enum Offsets {
        kPortIdOffsetAudioIn  = 100,
        kPortIdOffsetMidiIn   = 300,
        kPortIdOffsetAudioOut = 500,
        kPortIdOffsetMidiOut  = 700,
        kPortIdOffsetUser     = 1000,
    };

    char* name;
    char* fullname;
    void* buffer;
    uint index;
    int flags;
    uint gid;
    jack_uuid_t uuid;
    bool isMidi : 1;
    bool isSystem : 1;
    bool isConnected : 1;
    bool unused : 1;

    JackPortState(const char* const fullPortName,
                  const char* const portName,
                  const uint i, const int f, const uint id,
                  const bool midi, const bool con)
        : name(portName != nullptr ? strdup(portName) : nullptr),
          fullname(fullPortName != nullptr ? strdup(fullPortName) : nullptr),
          buffer(nullptr),
          index(i),
          flags(f),
          gid(id),
          uuid(jack_port_uuid_generate(id)),
          isMidi(midi),
          isSystem(true),
          isConnected(con),
          unused(false) {}

    JackPortState(const char* const clientName,
                  const char* const portName,
                  const uint i, const int f, const uint id,
                  const bool midi, const bool sys, const bool con)
        : name(portName != nullptr ? strdup(portName) : nullptr),
          fullname(nullptr),
          buffer(nullptr),
          index(i),
          flags(f),
          gid(id),
          uuid(jack_port_uuid_generate(id)),
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
        name = nullptr;

        std::free(fullname);
        fullname = nullptr;
    }

    CARLA_DECLARE_NON_COPYABLE(JackPortState)
};

struct JackClientState {
    const JackServerState& server;
    CarlaMutex mutex;

    bool activated;
    bool deactivated; // activated once, then deactivated

    char* name;
    jack_uuid_t uuid;

    LinkedList<JackPortState*> audioIns;
    LinkedList<JackPortState*> audioOuts;
    LinkedList<JackPortState*> midiIns;
    LinkedList<JackPortState*> midiOuts;

    std::map<uint, JackPortState*> portIdMapping;
    std::map<std::string, JackPortState*> portNameMapping;

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

    JackThreadInitCallback threadInitCb;
    void* threadInitCbPtr;

    JackClientState(const JackServerState& s, const char* const n)
        : server(s),
          mutex(),
          activated(false),
          deactivated(false),
          name(strdup(n)),
          uuid(jack_client_uuid_generate()),
          audioIns(),
          audioOuts(),
          midiIns(),
          midiOuts(),
          portIdMapping(),
          portNameMapping(),
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
          syncCbPtr(nullptr),
          threadInitCb(nullptr),
          threadInitCbPtr(nullptr) {}

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

        portIdMapping.clear();
        portNameMapping.clear();
    }

    CARLA_DECLARE_NON_COPYABLE(JackClientState)
};

struct JackServerState {
    CarlaJackAppClient* jackAppPtr;

    uint32_t bufferSize;
    double   sampleRate;

    jack_uuid_t uuid;

    uint8_t numAudioIns;
    uint8_t numAudioOuts;
    uint8_t numMidiIns;
    uint8_t numMidiOuts;

    bool playing;
    jack_position_t position;
    jack_nframes_t monotonic_frame;

    JackServerState(CarlaJackAppClient* const app)
        : jackAppPtr(app),
          bufferSize(0),
          sampleRate(0.0),
          uuid(jack_client_uuid_generate()),
          numAudioIns(0),
          numAudioOuts(0),
          numMidiIns(0),
          numMidiOuts(0),
          playing(false),
          position(),
          monotonic_frame(0)
    {
        carla_zeroStruct(position);
    }

    CARLA_DECLARE_NON_COPYABLE(JackServerState)
};

CARLA_BACKEND_END_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

#endif // CARLA_LIBJACK_HPP_INCLUDED
