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
    bool activated;
    bool prematurelyActivated;

    char* name;

    uint32_t bufferSize;
    double   sampleRate;

    bool playing;
    jack_position_t position;

    LinkedList<JackPortState*> audioIns;
    LinkedList<JackPortState*> audioOuts;
    uint32_t fakeIns, fakeOuts;

    LinkedList<JackPortState> midiIns;
    LinkedList<JackPortState> midiOuts;

    JackProcessCallback process;
    void* processPtr;

    JackShutdownCallback shutdown;
    void* shutdownPtr;

    JackClientState()
        : activated(false),
          prematurelyActivated(false),
          name(nullptr),
          bufferSize(0),
          sampleRate(0.0),
          playing(false),
          audioIns(),
          audioOuts(),
          fakeIns(0),
          fakeOuts(0),
          midiIns(),
          midiOuts(),
          process(nullptr),
          processPtr(nullptr),
          shutdown(nullptr),
          shutdownPtr(nullptr)
    {
        carla_zeroStruct(position);
    }

    ~JackClientState()
    {
        free(name);
    }
};

class CarlaJackClient : public juce::Thread
{
public:
    JackClientState fState;

    CarlaJackClient();
    ~CarlaJackClient() noexcept override;

    bool initIfNeeded(const char* const clientName);
    void clear() noexcept;
    bool isValid() const noexcept;

    void activate();
    void deactivate();
    void handleNonRtData();

    // -------------------------------------------------------------------

protected:
    void run() override;

private:
    BridgeAudioPool          fShmAudioPool;
    BridgeRtClientControl    fShmRtClientControl;
    BridgeNonRtClientControl fShmNonRtClientControl;
    BridgeNonRtServerControl fShmNonRtServerControl;

    char fBaseNameAudioPool[6+1];
    char fBaseNameRtClientControl[6+1];
    char fBaseNameNonRtClientControl[6+1];
    char fBaseNameNonRtServerControl[6+1];

    bool fIsValid;
    bool fIsOffline;
    bool fFirstIdle;
    int64_t fLastPingTime;

    uint32_t fAudioIns;
    uint32_t fAudioOuts;

    CarlaMutex fRealtimeThreadMutex;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaJackClient)
};

CARLA_BACKEND_END_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
