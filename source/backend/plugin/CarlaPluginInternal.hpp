/*
 * Carla Plugin
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_PLUGIN_INTERNAL_HPP_INCLUDED
#define CARLA_PLUGIN_INTERNAL_HPP_INCLUDED

#include "CarlaPlugin.hpp"
#include "CarlaPluginThread.hpp"

#include "CarlaOscUtils.hpp"
#include "CarlaStateUtils.hpp"

#include "CarlaMutex.hpp"
#include "RtLinkedList.hpp"

#include "CarlaMIDI.h"

// -----------------------------------------------------------------------

#define CARLA_PROCESS_CONTINUE_CHECK if (! pData->enabled) { pData->engine->callback(ENGINE_CALLBACK_DEBUG, pData->id, 0, 0, 0.0f, "Processing while plugin is disabled!!"); return; }

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------------------------------------
// Forward declarations of CarlaEngine classes

class CarlaEngineAudioPort;
class CarlaEngineCVPort;
class CarlaEngineEventPort;

// -----------------------------------------------------------------------
// Maximum pre-allocated events for some plugin types

const unsigned short kPluginMaxMidiEvents = 512;

// -----------------------------------------------------------------------
// Extra plugin hints, hidden from backend

const unsigned int PLUGIN_EXTRA_HINT_HAS_MIDI_IN      = 0x01;
const unsigned int PLUGIN_EXTRA_HINT_HAS_MIDI_OUT     = 0x02;
const unsigned int PLUGIN_EXTRA_HINT_CAN_RUN_RACK     = 0x04;
const unsigned int PLUGIN_EXTRA_HINT_USES_MULTI_PROGS = 0x08;

// -----------------------------------------------------------------------

/*!
 * Post-RT event type.\n
 * These are events postponned from within the process function,
 *
 * During process, we cannot lock, allocate memory or do UI stuff,\n
 * so events have to be postponned to be executed later, on a separate thread.
 */
enum PluginPostRtEventType {
    kPluginPostRtEventNull = 0,
    kPluginPostRtEventDebug,
    kPluginPostRtEventParameterChange,   // param, SP (*), value (SP: if 1, don't report change to Callback and OSC)
    kPluginPostRtEventProgramChange,     // index
    kPluginPostRtEventMidiProgramChange, // index
    kPluginPostRtEventNoteOn,            // channel, note, velo
    kPluginPostRtEventNoteOff            // channel, note
};

/*!
 * A Post-RT event.
 * \see PluginPostRtEventType
 */
struct PluginPostRtEvent {
    PluginPostRtEventType type;
    int32_t value1;
    int32_t value2;
    float   value3;
};

// -----------------------------------------------------------------------

struct ExternalMidiNote {
    int8_t  channel; // invalid if -1
    uint8_t note;    // 0 to 127
    uint8_t velo;    // note-off if 0
};

// -----------------------------------------------------------------------

struct PluginAudioPort {
    uint32_t rindex;
    CarlaEngineAudioPort* port;

    PluginAudioPort() noexcept;
    ~PluginAudioPort() noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(PluginAudioPort)
};

struct PluginAudioData {
    uint32_t count;
    PluginAudioPort* ports;

    PluginAudioData() noexcept;
    ~PluginAudioData() noexcept;
    void createNew(const uint32_t newCount);
    void clear() noexcept;
    void initBuffers() noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(PluginAudioData)
};

// -----------------------------------------------------------------------

struct PluginCVPort {
    uint32_t rindex;
    uint32_t param;
    CarlaEngineCVPort* port;

    PluginCVPort() noexcept;
    ~PluginCVPort() noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(PluginCVPort)
};

struct PluginCVData {
    uint32_t count;
    PluginCVPort* ports;

    PluginCVData() noexcept;
    ~PluginCVData() noexcept;
    void createNew(const uint32_t newCount);
    void clear() noexcept;
    void initBuffers() noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(PluginCVData)
};

// -----------------------------------------------------------------------

struct PluginEventData {
    CarlaEngineEventPort* portIn;
    CarlaEngineEventPort* portOut;

    PluginEventData() noexcept;
    ~PluginEventData() noexcept;
    void clear() noexcept;
    void initBuffers() noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(PluginEventData)
};

// -----------------------------------------------------------------------

enum SpecialParameterType {
    PARAMETER_SPECIAL_NULL          = 0,
    PARAMETER_SPECIAL_LATENCY       = 1,
    PARAMETER_SPECIAL_SAMPLE_RATE   = 2,
    PARAMETER_SPECIAL_LV2_FREEWHEEL = 3,
    PARAMETER_SPECIAL_LV2_TIME      = 4
};

struct PluginParameterData {
    uint32_t count;
    ParameterData* data;
    ParameterRanges* ranges;
    SpecialParameterType* special;

    PluginParameterData() noexcept;
    ~PluginParameterData() noexcept;
    void createNew(const uint32_t newCount, const bool withSpecial, const bool doReset);
    void clear() noexcept;
    float getFixedValue(const uint32_t parameterId, const float& value) const noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(PluginParameterData)
};

// -----------------------------------------------------------------------

typedef const char* ProgramName;

struct PluginProgramData {
    uint32_t count;
    int32_t  current;
    ProgramName* names;

    PluginProgramData() noexcept;
    ~PluginProgramData() noexcept;
    void createNew(const uint32_t newCount);
    void clear() noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(PluginProgramData)
};

// -----------------------------------------------------------------------

struct PluginMidiProgramData {
    uint32_t count;
    int32_t  current;
    MidiProgramData* data;

    PluginMidiProgramData() noexcept;
    ~PluginMidiProgramData() noexcept;
    void createNew(const uint32_t newCount);
    void clear() noexcept;
    const MidiProgramData& getCurrent() const noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(PluginMidiProgramData)
};

// -----------------------------------------------------------------------

struct CarlaPlugin::ProtectedData {
    CarlaEngine* const engine;
    CarlaEngineClient* client;

    unsigned int id;
    unsigned int hints;
    unsigned int options;

    bool active;
    bool enabled;
    bool needsReset;

    void* lib;
    void* uiLib;

    // misc
    int8_t ctrlChannel;
    uint   extraHints;
    uint   transientTryCounter;

    // latency
    uint32_t latency;
    float**  latencyBuffers;

    // data 1
    const char* name;
    const char* filename;
    const char* iconName;
    const char* identifier; // used for save/restore settings per plugin

    // data 2
    PluginAudioData audioIn;
    PluginAudioData audioOut;
    PluginEventData event;
    PluginParameterData param;
    PluginProgramData prog;
    PluginMidiProgramData midiprog;
    LinkedList<CustomData> custom;

    SaveState saveState;

    CarlaMutex masterMutex; // global master lock
    CarlaMutex singleMutex; // small lock used only in processSingle()

    struct ExternalNotes {
        CarlaMutex mutex;
        RtLinkedList<ExternalMidiNote>::Pool dataPool;
        RtLinkedList<ExternalMidiNote> data;

        ExternalNotes();
        ~ExternalNotes();
        void append(const ExternalMidiNote& note);

        CARLA_DECLARE_NON_COPY_STRUCT(ExternalNotes)

    } extNotes;

    struct PostRtEvents {
        CarlaMutex mutex;
        RtLinkedList<PluginPostRtEvent>::Pool dataPool;
        RtLinkedList<PluginPostRtEvent> data;
        RtLinkedList<PluginPostRtEvent> dataPendingRT;

        PostRtEvents();
        ~PostRtEvents();
        void appendRT(const PluginPostRtEvent& event);
        void trySplice();
        void clear();

        CARLA_DECLARE_NON_COPY_STRUCT(PostRtEvents)

    } postRtEvents;

#ifndef BUILD_BRIDGE
    struct PostProc {
        float dryWet;
        float volume;
        float balanceLeft;
        float balanceRight;
        float panning;

        PostProc() noexcept;

        CARLA_DECLARE_NON_COPY_STRUCT(PostProc)

    } postProc;
#endif

    struct OSC {
        CarlaOscData data;
        CarlaPluginThread thread;

        OSC(CarlaEngine* const engine, CarlaPlugin* const plugin);

#ifdef CARLA_PROPER_CPP11_SUPPORT
        OSC() = delete;
        CARLA_DECLARE_NON_COPY_STRUCT(OSC)
#endif
    } osc;

    ProtectedData(CarlaEngine* const eng, const unsigned int idx, CarlaPlugin* const self);
    ~ProtectedData();

    // -------------------------------------------------------------------
    // Buffer functions

    void clearBuffers();
    void recreateLatencyBuffers();

    // -------------------------------------------------------------------
    // Post-poned events

    void postponeRtEvent(const PluginPostRtEventType type, const int32_t value1, const int32_t value2, const float value3);

    // -------------------------------------------------------------------
    // Library functions

    const char* libError(const char* const filename);

    bool  libOpen(const char* const filename);
    bool  libClose();
    void* libSymbol(const char* const symbol);

    bool  uiLibOpen(const char* const filename, const bool canDelete);
    bool  uiLibClose();
    void* uiLibSymbol(const char* const symbol);

    // -------------------------------------------------------------------
    // Settings functions

    void saveSetting(const uint option, const bool yesNo);
    uint loadSettings(const uint options, const uint availOptions);

    // -------------------------------------------------------------------
    // Misc

    void tryTransient();

    // -------------------------------------------------------------------

#ifdef CARLA_PROPER_CPP11_SUPPORT
    ProtectedData() = delete;
    CARLA_DECLARE_NON_COPY_STRUCT(ProtectedData)
#endif
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_PLUGIN_INTERNAL_HPP_INCLUDED
