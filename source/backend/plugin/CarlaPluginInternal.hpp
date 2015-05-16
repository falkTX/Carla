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

#include "CarlaLibUtils.hpp"
#include "CarlaStateUtils.hpp"

#include "CarlaMIDI.h"
#include "CarlaMutex.hpp"
#include "CarlaString.hpp"
#include "RtLinkedList.hpp"

#include "juce_audio_basics.h"

using juce::FloatVectorOperations;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Engine helper macro, sets lastError and returns false/NULL

#define CARLA_SAFE_ASSERT_RETURN_ERR(cond, err)  if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); pData->engine->setLastError(err); return false;   }
#define CARLA_SAFE_ASSERT_RETURN_ERRN(cond, err) if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); pData->engine->setLastError(err); return nullptr; }

#define CARLA_SAFE_EXCEPTION_RETURN_ERR(excptMsg, errMsg)  catch(...) { carla_safe_exception(excptMsg, __FILE__, __LINE__); pData->engine->setLastError(errMsg); return false;   }
#define CARLA_SAFE_EXCEPTION_RETURN_ERRN(excptMsg, errMsg) catch(...) { carla_safe_exception(excptMsg, __FILE__, __LINE__); pData->engine->setLastError(errMsg); return nullptr; }

// -----------------------------------------------------------------------
// Maximum pre-allocated events for some plugin types

const ushort kPluginMaxMidiEvents = 512;

// -----------------------------------------------------------------------
// Extra plugin hints, hidden from backend

const uint PLUGIN_EXTRA_HINT_HAS_MIDI_IN      = 0x01;
const uint PLUGIN_EXTRA_HINT_HAS_MIDI_OUT     = 0x02;
const uint PLUGIN_EXTRA_HINT_CAN_RUN_RACK     = 0x04;

// -----------------------------------------------------------------------
// Special parameters

enum SpecialParameterType {
    PARAMETER_SPECIAL_NULL        = 0,
    PARAMETER_SPECIAL_FREEWHEEL   = 1,
    PARAMETER_SPECIAL_LATENCY     = 2,
    PARAMETER_SPECIAL_SAMPLE_RATE = 3,
    PARAMETER_SPECIAL_TIME        = 4
};

// -----------------------------------------------------------------------

/*!
 * Post-RT event type.
 * These are events postponned from within the process function,
 *
 * During process, we cannot lock, allocate memory or do UI stuff,
 * so events have to be postponned to be executed later, on a separate thread.
 * @see PluginPostRtEvent
 */
enum PluginPostRtEventType {
    kPluginPostRtEventNull = 0,
    kPluginPostRtEventDebug,
    kPluginPostRtEventParameterChange,   // param, SP (*), value (SP: if 1 only report change to UI, don't report to Callback and OSC)
    kPluginPostRtEventProgramChange,     // index
    kPluginPostRtEventMidiProgramChange, // index
    kPluginPostRtEventNoteOn,            // channel, note, velo
    kPluginPostRtEventNoteOff            // channel, note
};

/*!
 * A Post-RT event.
 * @see PluginPostRtEventType
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
    uint8_t velo;    // 1 to 127, 0 for note-off
};

// -----------------------------------------------------------------------

struct PluginAudioPort {
    uint32_t rindex;
    CarlaEngineAudioPort* port;
};

struct PluginAudioData {
    uint32_t count;
    PluginAudioPort* ports;

    PluginAudioData() noexcept;
    ~PluginAudioData() noexcept;
    void createNew(const uint32_t newCount);
    void clear() noexcept;
    void initBuffers() const noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(PluginAudioData)
};

// -----------------------------------------------------------------------

struct PluginCVPort {
    uint32_t rindex;
    //uint32_t param; // FIXME is this needed?
    CarlaEngineCVPort* port;
};

struct PluginCVData {
    uint32_t count;
    PluginCVPort* ports;

    PluginCVData() noexcept;
    ~PluginCVData() noexcept;
    void createNew(const uint32_t newCount);
    void clear() noexcept;
    void initBuffers() const noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(PluginCVData)
};

// -----------------------------------------------------------------------

struct PluginEventData {
    CarlaEngineEventPort* portIn;
    CarlaEngineEventPort* portOut;

    PluginEventData() noexcept;
    ~PluginEventData() noexcept;
    void clear() noexcept;
    void initBuffers() const noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(PluginEventData)
};

// -----------------------------------------------------------------------

struct PluginParameterData {
    uint32_t count;
    ParameterData* data;
    ParameterRanges* ranges;
    SpecialParameterType* special;

    PluginParameterData() noexcept;
    ~PluginParameterData() noexcept;
    void createNew(const uint32_t newCount, const bool withSpecial);
    void clear() noexcept;
    float getFixedValue(const uint32_t parameterId, const float& value) const noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(PluginParameterData)
};

// -----------------------------------------------------------------------

typedef const char* ProgramName;

struct PluginProgramData {
    uint32_t count;
    int32_t current;
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
    int32_t current;
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

    uint id;
    uint hints;
    uint options;
    uint32_t nodeId;

    bool active;
    bool enabled;
    bool needsReset;

    lib_t lib;
    lib_t uiLib;

    // misc
    int8_t ctrlChannel;
    uint   extraHints;
    uint   transientTryCounter;

    // data 1
    const char* name;
    const char* filename;
    const char* iconName;

    // data 2
    PluginAudioData audioIn;
    PluginAudioData audioOut;
    PluginCVData cvIn;
    PluginCVData cvOut;
    PluginEventData event;
    PluginParameterData param;
    PluginProgramData prog;
    PluginMidiProgramData midiprog;
    LinkedList<CustomData> custom;

    CarlaMutex masterMutex; // global master lock
    CarlaMutex singleMutex; // small lock used only in processSingle()

    CarlaStateSave stateSave;

    struct ExternalNotes {
        CarlaMutex mutex;
        RtLinkedList<ExternalMidiNote>::Pool dataPool;
        RtLinkedList<ExternalMidiNote> data;

        ExternalNotes() noexcept;
        ~ExternalNotes() noexcept;
        void appendNonRT(const ExternalMidiNote& note) noexcept;
        void clear() noexcept;

        CARLA_DECLARE_NON_COPY_STRUCT(ExternalNotes)

    } extNotes;

    struct Latency {
        uint32_t channels;
        uint32_t frames;
        float**  buffers;

        Latency() noexcept;
        ~Latency() noexcept;
        void clearBuffers() noexcept;
        void recreateBuffers(const uint32_t newChannels, const uint32_t newFrames);

        CARLA_DECLARE_NON_COPY_STRUCT(Latency)

    } latency;

    struct PostRtEvents {
        CarlaMutex mutex;
        RtLinkedList<PluginPostRtEvent>::Pool dataPool;
        RtLinkedList<PluginPostRtEvent> data;
        RtLinkedList<PluginPostRtEvent> dataPendingRT;

        PostRtEvents() noexcept;
        ~PostRtEvents() noexcept;
        void appendRT(const PluginPostRtEvent& event) noexcept;
        void trySplice() noexcept;
        void clear() noexcept;

        CARLA_DECLARE_NON_COPY_STRUCT(PostRtEvents)

    } postRtEvents;

    struct PostUiEvents {
        CarlaMutex mutex;
        LinkedList<PluginPostRtEvent> data;

        PostUiEvents() noexcept;
        ~PostUiEvents() noexcept;
        void append(const PluginPostRtEvent& event) noexcept;
        void clear() noexcept;

        CARLA_DECLARE_NON_COPY_STRUCT(PostUiEvents)

    } postUiEvents;

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

    ProtectedData(CarlaEngine* const engine, const uint idx) noexcept;
    ~ProtectedData() noexcept;

    // -------------------------------------------------------------------
    // Buffer functions

    void clearBuffers() noexcept;

    // -------------------------------------------------------------------
    // Post-poned events

    void postponeRtEvent(const PluginPostRtEvent& rtEvent) noexcept;
    void postponeRtEvent(const PluginPostRtEventType type, const int32_t value1, const int32_t value2, const float value3) noexcept;

    // -------------------------------------------------------------------
    // Library functions

    static const char* libError(const char* const filename) noexcept;

    bool libOpen(const char* const filename) noexcept;
    bool libClose() noexcept;

    bool uiLibOpen(const char* const filename, const bool canDelete) noexcept;
    bool uiLibClose() noexcept;

    template<typename Func>
    Func libSymbol(const char* const symbol) const noexcept
    {
        return lib_symbol<Func>(lib, symbol);
    }

    template<typename Func>
    Func uiLibSymbol(const char* const symbol) const noexcept
    {
        return lib_symbol<Func>(uiLib, symbol);
    }

    // -------------------------------------------------------------------
    // Misc

#ifndef BUILD_BRIDGE
    void tryTransient() noexcept;
#endif
    void updateParameterValues(CarlaPlugin* const plugin, const bool sendOsc, const bool sendCallback, const bool useDefault) noexcept;

    // -------------------------------------------------------------------

#ifdef CARLA_PROPER_CPP11_SUPPORT
    ProtectedData() = delete;
    CARLA_DECLARE_NON_COPY_STRUCT(ProtectedData);
#endif
    CARLA_LEAK_DETECTOR(ProtectedData);
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_PLUGIN_INTERNAL_HPP_INCLUDED
