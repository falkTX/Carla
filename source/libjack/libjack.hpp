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

#include "AppConfig.h"
#include "juce_core/juce_core.h"

#if 0
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/transport.h>
#include <jack/session.h>
#include <jack/metadata.h>
#endif

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

struct JackPortState {
    char* name;
    char* fullname;
    void* buffer;
    uint index;
    uint flags;
    bool isSystem;

    JackPortState()
        : name(nullptr),
          fullname(nullptr),
          buffer(nullptr),
          index(0),
          flags(0),
          isSystem(false) {}

    JackPortState(const char* const cn, const char* const pn, const uint i, const uint f, const bool sys)
        : name(strdup(pn)),
          fullname(nullptr),
          buffer(nullptr),
          index(i),
          flags(f),
          isSystem(sys)
    {
        char strBuf[STR_MAX+1];
        snprintf(strBuf, STR_MAX, "%s:%s", cn, pn);
        strBuf[STR_MAX] = '\0';

        fullname = strdup(strBuf);
    }

    ~JackPortState()
    {
        free(name);
        free(fullname);
    }
};

struct JackClientState {
    const JackServerState& server;
    CarlaMutex mutex;

    bool activated;
    bool deactivated; // activated once, then deactivated

    char* name;

    LinkedList<JackPortState*> audioIns;
    LinkedList<JackPortState*> audioOuts;
    LinkedList<JackPortState> midiIns;
    LinkedList<JackPortState> midiOuts;

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

        free(name);
        name = nullptr;

        audioIns.clear();
        audioOuts.clear();
    }
};

struct JackServerState {
    CarlaJackAppClient* jackAppPtr;

    uint32_t bufferSize;
    double   sampleRate;

    bool playing;
    jack_position_t position;

    JackServerState()
        : jackAppPtr(nullptr),
          bufferSize(0),
          sampleRate(0.0),
          playing(false)
    {
        carla_zeroStruct(position);
    }
};

CARLA_BACKEND_END_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
