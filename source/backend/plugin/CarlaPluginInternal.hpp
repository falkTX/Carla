/*
 * Carla Plugin
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaEngine.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaOscUtils.hpp"
#include "CarlaStateUtils.hpp"
#include "CarlaMutex.hpp"
#include "CarlaMIDI.h"
#include "RtList.hpp"

#define CARLA_PROCESS_CONTINUE_CHECK if (! fEnabled) { pData->engine->callback(CALLBACK_DEBUG, fId, 0, 0, 0.0f, "Processing while plugin is disabled!!"); return; }

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------------------------------------

const unsigned short kPluginMaxMidiEvents = 512;

const unsigned int PLUGIN_EXTRA_HINT_HAS_MIDI_IN  = 0x1;
const unsigned int PLUGIN_EXTRA_HINT_HAS_MIDI_OUT = 0x2;
const unsigned int PLUGIN_EXTRA_HINT_CAN_RUN_RACK = 0x4;

// -----------------------------------------------------------------------

/*!
 * Post-RT event type.\n
 * These are events postponned from within the process function,
 *
 * During process, we cannot lock, allocate memory or do UI stuff,\n
 * so events have to be postponned to be executed later, on a separate thread.
 */
enum PluginPostRtEventType {
    kPluginPostRtEventNull,
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

    PluginPostRtEvent() noexcept
        : type(kPluginPostRtEventNull),
          value1(-1),
          value2(-1),
          value3(0.0f) {}
};

// -----------------------------------------------------------------------

struct PluginAudioPort {
    uint32_t rindex;
    CarlaEngineAudioPort* port;

    PluginAudioPort() noexcept
        : rindex(0),
          port(nullptr) {}

    ~PluginAudioPort()
    {
        CARLA_ASSERT(port == nullptr);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(PluginAudioPort)
};

struct PluginAudioData {
    uint32_t count;
    PluginAudioPort* ports;

    PluginAudioData() noexcept
        : count(0),
          ports(nullptr) {}

    ~PluginAudioData()
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT(ports == nullptr);
    }

    void createNew(const uint32_t newCount)
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT(ports == nullptr);
        CARLA_ASSERT_INT(newCount > 0, newCount);

        if (ports != nullptr || newCount == 0)
            return;

        ports = new PluginAudioPort[newCount];
        count = newCount;
    }

    void clear()
    {
        if (ports != nullptr)
        {
            for (uint32_t i=0; i < count; ++i)
            {
                if (ports[i].port != nullptr)
                {
                    delete ports[i].port;
                    ports[i].port = nullptr;
                }
            }

            delete[] ports;
            ports = nullptr;
        }

        count = 0;
    }

    void initBuffers()
    {
        for (uint32_t i=0; i < count; ++i)
        {
            if (ports[i].port != nullptr)
                ports[i].port->initBuffer();
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(PluginAudioData)
};

// -----------------------------------------------------------------------

struct PluginCVPort {
    uint32_t rindex;
    uint32_t param;
    CarlaEngineCVPort* port;

    PluginCVPort() noexcept
        : rindex(0),
          param(0),
          port(nullptr) {}

    ~PluginCVPort()
    {
        CARLA_ASSERT(port == nullptr);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(PluginCVPort)
};

struct PluginCVData {
    uint32_t count;
    PluginCVPort* ports;

    PluginCVData() noexcept
        : count(0),
          ports(nullptr) {}

    ~PluginCVData()
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT(ports == nullptr);
    }

    void createNew(const uint32_t newCount)
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT(ports == nullptr);
        CARLA_ASSERT_INT(newCount > 0, newCount);

        if (ports != nullptr || newCount == 0)
            return;

        ports = new PluginCVPort[newCount];
        count = newCount;
    }

    void clear()
    {
        if (ports != nullptr)
        {
            for (uint32_t i=0; i < count; ++i)
            {
                if (ports[i].port != nullptr)
                {
                    delete ports[i].port;
                    ports[i].port = nullptr;
                }
            }

            delete[] ports;
            ports = nullptr;
        }

        count = 0;
    }

    void initBuffers()
    {
        for (uint32_t i=0; i < count; ++i)
        {
            if (ports[i].port != nullptr)
                ports[i].port->initBuffer();
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(PluginCVData)
};

// -----------------------------------------------------------------------

struct PluginEventData {
    CarlaEngineEventPort* portIn;
    CarlaEngineEventPort* portOut;

    PluginEventData() noexcept
        : portIn(nullptr),
          portOut(nullptr) {}

    ~PluginEventData()
    {
        CARLA_ASSERT(portIn == nullptr);
        CARLA_ASSERT(portOut == nullptr);
    }

    void clear()
    {
        if (portIn != nullptr)
        {
            delete portIn;
            portIn = nullptr;
        }

        if (portOut != nullptr)
        {
            delete portOut;
            portOut = nullptr;
        }
    }

    void initBuffers()
    {
        if (portIn != nullptr)
            portIn->initBuffer();

        if (portOut != nullptr)
            portOut->initBuffer();
    }

    CARLA_DECLARE_NON_COPY_STRUCT(PluginEventData)
};

// -----------------------------------------------------------------------

struct PluginParameterData {
    uint32_t count;
    ParameterData* data;
    ParameterRanges* ranges;

    PluginParameterData() noexcept
        : count(0),
          data(nullptr),
          ranges(nullptr) {}

    ~PluginParameterData()
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT(data == nullptr);
        CARLA_ASSERT(ranges == nullptr);
    }

    void createNew(const uint32_t newCount)
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT(data == nullptr);
        CARLA_ASSERT(ranges == nullptr);
        CARLA_ASSERT_INT(newCount > 0, newCount);

        if (data != nullptr || ranges != nullptr || newCount == 0)
            return;

        data   = new ParameterData[newCount];
        ranges = new ParameterRanges[newCount];
        count  = newCount;
    }

    void clear()
    {
        if (data != nullptr)
        {
            delete[] data;
            data = nullptr;
        }

        if (ranges != nullptr)
        {
            delete[] ranges;
            ranges = nullptr;
        }

        count = 0;
    }

    float getFixedValue(const uint32_t parameterId, const float& value) const
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < count, 0.0f);
        return ranges[parameterId].getFixedValue(value);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(PluginParameterData)
};

// -----------------------------------------------------------------------

typedef const char* ProgramName;

struct PluginProgramData {
    uint32_t count;
    int32_t  current;
    ProgramName* names;

    PluginProgramData() noexcept
        : count(0),
          current(-1),
          names(nullptr) {}

    ~PluginProgramData()
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT_INT(current == -1, current);
        CARLA_ASSERT(names == nullptr);
    }

    void createNew(const uint32_t newCount)
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT_INT(current == -1, current);
        CARLA_ASSERT(names == nullptr);
        CARLA_ASSERT_INT(newCount > 0, newCount);

        if (names != nullptr || newCount == 0)
            return;

        names = new ProgramName[newCount];
        count = newCount;

        for (uint32_t i=0; i < newCount; ++i)
            names[i] = nullptr;
    }

    void clear()
    {
        if (names != nullptr)
        {
            for (uint32_t i=0; i < count; ++i)
            {
                if (names[i] != nullptr)
                {
                    delete[] names[i];
                    names[i] = nullptr;
                }
            }

            delete[] names;
            names = nullptr;
        }

        count = 0;
        current = -1;
    }

    CARLA_DECLARE_NON_COPY_STRUCT(PluginProgramData)
};

// -----------------------------------------------------------------------

struct PluginMidiProgramData {
    uint32_t count;
    int32_t  current;
    MidiProgramData* data;

    PluginMidiProgramData() noexcept
        : count(0),
          current(-1),
          data(nullptr) {}

    ~PluginMidiProgramData()
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT_INT(current == -1, current);
        CARLA_ASSERT(data == nullptr);
    }

    void createNew(const uint32_t newCount)
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT_INT(current == -1, current);
        CARLA_ASSERT(data == nullptr);
        CARLA_ASSERT_INT(newCount > 0, newCount);

        if (data != nullptr || newCount == 0)
            return;

        data  = new MidiProgramData[newCount];
        count = newCount;
    }

    void clear()
    {
        if (data != nullptr)
        {
            for (uint32_t i=0; i < count; ++i)
            {
                if (data[i].name != nullptr)
                {
                    delete[] data[i].name;
                    data[i].name = nullptr;
                }
            }

            delete[] data;
            data = nullptr;
        }

        count = 0;
        current = -1;
    }

    const MidiProgramData& getCurrent() const
    {
        CARLA_ASSERT_INT2(current >= 0 && current < static_cast<int32_t>(count), current, count);
        return data[current];
    }

    CARLA_DECLARE_NON_COPY_STRUCT(PluginMidiProgramData)
};

// -----------------------------------------------------------------------

struct ExternalMidiNote {
    int8_t  channel; // invalid if -1
    uint8_t note;
    uint8_t velo; // note-off if 0

    ExternalMidiNote() noexcept
        : channel(-1),
          note(0),
          velo(0) {}
};

// -----------------------------------------------------------------------

struct CarlaPluginProtectedData {
    CarlaEngine* const engine;
    CarlaEngineClient* client;

    bool active;
    bool needsReset;
    void* lib;
    void* uiLib;

    // misc
    int8_t       ctrlChannel;
    unsigned int extraHints;
    CarlaString  idStr;

    // latency
    uint32_t latency;
    float**  latencyBuffers;

    // data
    PluginAudioData audioIn;
    PluginAudioData audioOut;
    PluginEventData event;
    PluginParameterData param;
    PluginProgramData prog;
    PluginMidiProgramData midiprog;
    NonRtList<CustomData> custom;

    SaveState saveState;

    CarlaMutex masterMutex; // global master lock
    CarlaMutex singleMutex; // small lock used only in processSingle()

    struct ExternalNotes {
        CarlaMutex mutex;
        RtList<ExternalMidiNote>::Pool dataPool;
        RtList<ExternalMidiNote> data;

        ExternalNotes()
            : dataPool(32, 152),
              data(dataPool) {}

        ~ExternalNotes()
        {
            mutex.lock();
            data.clear();
            mutex.unlock();
        }

        void append(const ExternalMidiNote& note)
        {
            mutex.lock();
            data.append_sleepy(note);
            mutex.unlock();
        }

        CARLA_DECLARE_NON_COPY_STRUCT(ExternalNotes)

    } extNotes;

    struct PostRtEvents {
        CarlaMutex mutex;
        RtList<PluginPostRtEvent>::Pool dataPool;
        RtList<PluginPostRtEvent> data;
        RtList<PluginPostRtEvent> dataPendingRT;

        PostRtEvents()
            : dataPool(128, 128),
              data(dataPool),
              dataPendingRT(dataPool) {}

        ~PostRtEvents()
        {
            clear();
        }

        void appendRT(const PluginPostRtEvent& event)
        {
            dataPendingRT.append(event);
        }

        void trySplice()
        {
            if (mutex.tryLock())
            {
                dataPendingRT.spliceAppend(data);
                mutex.unlock();
            }
        }

        void clear()
        {
            mutex.lock();
            data.clear();
            dataPendingRT.clear();
            mutex.unlock();
        }

        CARLA_DECLARE_NON_COPY_STRUCT(PostRtEvents)

    } postRtEvents;

#ifndef BUILD_BRIDGE
    struct PostProc {
        float dryWet;
        float volume;
        float balanceLeft;
        float balanceRight;
        float panning;

        PostProc() noexcept
            : dryWet(1.0f),
              volume(1.0f),
              balanceLeft(-1.0f),
              balanceRight(1.0f),
              panning(0.0f) {}

        CARLA_DECLARE_NON_COPY_STRUCT(PostProc)

    } postProc;
#endif

    struct OSC {
        CarlaOscData data;
        CarlaPluginThread thread;

        OSC(CarlaEngine* const engine, CarlaPlugin* const plugin)
            : thread(engine, plugin) {}

#ifdef CARLA_PROPER_CPP11_SUPPORT
        OSC() = delete;
        CARLA_DECLARE_NON_COPY_STRUCT(OSC)
#endif
    } osc;

    CarlaPluginProtectedData(CarlaEngine* const eng, CarlaPlugin* const plug)
        : engine(eng),
          client(nullptr),
          active(false),
          needsReset(false),
          lib(nullptr),
          uiLib(nullptr),
          ctrlChannel(0),
          extraHints(0x0),
          latency(0),
          latencyBuffers(nullptr),
          osc(eng, plug) {}

#ifdef CARLA_PROPER_CPP11_SUPPORT
    CarlaPluginProtectedData() = delete;
    CARLA_DECLARE_NON_COPY_STRUCT(CarlaPluginProtectedData)
#endif

    ~CarlaPluginProtectedData()
    {
        CARLA_ASSERT(client == nullptr);
        CARLA_ASSERT(! active);
        CARLA_ASSERT(lib == nullptr);
        CARLA_ASSERT(uiLib == nullptr);
        CARLA_ASSERT(latency == 0);
        CARLA_ASSERT(latencyBuffers == nullptr);
        CARLA_SAFE_ASSERT(! needsReset);
    }

    // -------------------------------------------------------------------
    // Cleanup

    void cleanup()
    {
        {
            // mutex MUST have been locked before
            const bool lockMaster(masterMutex.tryLock());
            const bool lockSingle(singleMutex.tryLock());
            CARLA_ASSERT(! lockMaster);
            CARLA_ASSERT(! lockSingle);
        }

        if (client != nullptr)
        {
            if (client->isActive())
                client->deactivate();
            else
                carla_assert("client->isActive()", __FILE__, __LINE__);

            clearBuffers();

            delete client;
            client = nullptr;
        }

        for (NonRtList<CustomData>::Itenerator it = custom.begin(); it.valid(); it.next())
        {
            CustomData& cData(*it);

            if (cData.type != nullptr)
            {
                delete[] cData.type;
                cData.type = nullptr;
            }
            else
                carla_assert("cData.type != nullptr", __FILE__, __LINE__);

            if (cData.key != nullptr)
            {
                delete[] cData.key;
                cData.key = nullptr;
            }
            else
                carla_assert("cData.key != nullptr", __FILE__, __LINE__);

            if (cData.value != nullptr)
            {
                delete[] cData.value;
                cData.value = nullptr;
            }
            else
                carla_assert("cData.value != nullptr", __FILE__, __LINE__);
        }

        prog.clear();
        midiprog.clear();
        custom.clear();

        // MUST have been locked before
        masterMutex.unlock();
        singleMutex.unlock();

        if (lib != nullptr)
            libClose();
    }

    // -------------------------------------------------------------------
    // Buffer functions

    void clearBuffers()
    {
        if (latencyBuffers != nullptr)
        {
            CARLA_ASSERT(audioIn.count > 0);

            for (uint32_t i=0; i < audioIn.count; ++i)
            {
                CARLA_SAFE_ASSERT_CONTINUE(latencyBuffers[i] != nullptr);

                delete[] latencyBuffers[i];
                latencyBuffers[i] = nullptr;
            }

            delete[] latencyBuffers;
            latencyBuffers = nullptr;
            latency = 0;
        }
        else
        {
            CARLA_ASSERT(latency == 0);
        }

        audioIn.clear();
        audioOut.clear();
        param.clear();
        event.clear();
    }

    void recreateLatencyBuffers()
    {
        if (latencyBuffers != nullptr)
        {
            CARLA_ASSERT(audioIn.count > 0);

            for (uint32_t i=0; i < audioIn.count; ++i)
            {
                CARLA_SAFE_ASSERT_CONTINUE(latencyBuffers[i] != nullptr);

                delete[] latencyBuffers[i];
                latencyBuffers[i] = nullptr;
            }

            delete[] latencyBuffers;
            latencyBuffers = nullptr;
        }

        if (audioIn.count > 0 && latency > 0)
        {
            latencyBuffers = new float*[audioIn.count];

            for (uint32_t i=0; i < audioIn.count; ++i)
            {
                latencyBuffers[i] = new float[latency];
                carla_zeroFloat(latencyBuffers[i], latency);
            }
        }
    }

    // -------------------------------------------------------------------
    // Post-poned events

    void postponeRtEvent(const PluginPostRtEventType type, const int32_t value1, const int32_t value2, const float value3)
    {
        CARLA_SAFE_ASSERT_RETURN(type != kPluginPostRtEventNull,);

        PluginPostRtEvent event;
        event.type   = type;
        event.value1 = value1;
        event.value2 = value2;
        event.value3 = value3;

        postRtEvents.appendRT(event);
    }

    // -------------------------------------------------------------------
    // Library functions, see CarlaPlugin.cpp

    bool  libOpen(const char* const filename);
    bool  libClose();
    void* libSymbol(const char* const symbol);

    bool  uiLibOpen(const char* const filename);
    bool  uiLibClose();
    void* uiLibSymbol(const char* const symbol);

    // -------------------------------------------------------------------
    // Settings functions, see CarlaPlugin.cpp

    void         saveSetting(const unsigned int option, const bool yesNo);
    unsigned int loadSettings(const unsigned int options, const unsigned int availOptions);
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_PLUGIN_INTERNAL_HPP_INCLUDED
