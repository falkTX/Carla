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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_PLUGIN_INTERNAL_HPP__
#define __CARLA_PLUGIN_INTERNAL_HPP__

#include "CarlaPlugin.hpp"
#include "CarlaPluginThread.hpp"
#include "CarlaEngine.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaOscUtils.hpp"
#include "CarlaMutex.hpp"
#include "CarlaMIDI.h"

#include "RtList.hpp"

#define CARLA_PROCESS_CONTINUE_CHECK if (! fEnabled) { kData->engine->callback(CALLBACK_DEBUG, fId, 0, 0, 0.0f, nullptr); return; }

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

const unsigned short MAX_MIDI_EVENTS = 512;

const unsigned int PLUGIN_HINT_HAS_MIDI_IN  = 0x1;
const unsigned int PLUGIN_HINT_HAS_MIDI_OUT = 0x2;
const unsigned int PLUGIN_HINT_CAN_RUN_RACK = 0x4;

// -----------------------------------------------------------------------

struct PluginAudioPort {
    uint32_t rindex;
    CarlaEngineAudioPort* port;

    PluginAudioPort()
        : rindex(0),
          port(nullptr) {}

    ~PluginAudioPort()
    {
        CARLA_ASSERT(port == nullptr);
    }

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(PluginAudioPort)
};

struct PluginAudioData {
    uint32_t count;
    PluginAudioPort* ports;

    PluginAudioData()
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
            for (uint32_t i=0; i < count; i++)
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

    void initBuffers(CarlaEngine* const engine)
    {
        for (uint32_t i=0; i < count; i++)
        {
            if (ports[i].port != nullptr)
                ports[i].port->initBuffer(engine);
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(PluginAudioData)
};

// -----------------------------------------------------------------------

struct PluginEventData {
    CarlaEngineEventPort* portIn;
    CarlaEngineEventPort* portOut;

    PluginEventData()
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

    void initBuffers(CarlaEngine* const engine)
    {
        if (portIn != nullptr)
            portIn->initBuffer(engine);

        if (portOut != nullptr)
            portOut->initBuffer(engine);
    }

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(PluginEventData)
};

// -----------------------------------------------------------------------

struct PluginParameterData {
    uint32_t count;
    ParameterData* data;
    ParameterRanges* ranges;

    PluginParameterData()
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

    float fixValue(const uint32_t parameterId, const float& value)
    {
        CARLA_ASSERT(parameterId < count);
        return ranges[parameterId].fixValue(value);
    }

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(PluginParameterData)
};

// -----------------------------------------------------------------------

typedef const char* ProgramName;

struct PluginProgramData {
    uint32_t count;
    int32_t  current;
    ProgramName* names;

    PluginProgramData()
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

        for (uint32_t i=0; i < newCount; i++)
            names[i] = nullptr;
    }

    void clear()
    {
        if (names != nullptr)
        {
            for (uint32_t i=0; i < count; i++)
            {
                if (names[i] != nullptr)
                    delete[] names[i];
            }

            delete[] names;
            names = nullptr;
        }

        count = 0;
        current = -1;
    }

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(PluginProgramData)
};

// -----------------------------------------------------------------------

struct PluginMidiProgramData {
    uint32_t count;
    int32_t  current;
    MidiProgramData* data;

    PluginMidiProgramData()
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
            for (uint32_t i=0; i < count; i++)
            {
                if (data[i].name != nullptr)
                    delete[] data[i].name;
            }

            delete[] data;
            data = nullptr;
        }

        count = 0;
        current = -1;
    }

    const MidiProgramData& getCurrent() const
    {
        CARLA_ASSERT(current >= 0 && current < static_cast<int32_t>(count));
        return data[current];
    }

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(PluginMidiProgramData)
};

// -----------------------------------------------------------------------

struct PluginPostRtEvent {
    PluginPostRtEventType type;
    int32_t value1;
    int32_t value2;
    float   value3;

    PluginPostRtEvent()
        : type(kPluginPostRtEventNull),
          value1(-1),
          value2(-1),
          value3(0.0f) {}

#if 1//def DEBUG
    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(PluginPostRtEvent)
#else
    CARLA_DECLARE_NON_COPY_STRUCT(PluginPostRtEvent)
#endif
};

// -----------------------------------------------------------------------

struct ExternalMidiNote {
    int8_t  channel; // invalid == -1
    uint8_t note;
    uint8_t velo; // note-off if 0

    ExternalMidiNote()
        : channel(-1),
          note(0),
          velo(0) {}

#if 1//def DEBUG
    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(ExternalMidiNote)
#else
    CARLA_DECLARE_NON_COPY_STRUCT(ExternalMidiNote)
#endif
};

// -----------------------------------------------------------------------

class CarlaPluginGui;

struct CarlaPluginProtectedData {
    CarlaEngine* const engine;
    CarlaEngineClient* client;
    CarlaPluginGui* gui;

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

    CarlaMutex masterMutex; // global master lock
    CarlaMutex singleMutex; // small lock used only in processSingle()

    struct ExternalNotes {
        CarlaMutex mutex;
        RtList<ExternalMidiNote>::Pool dataPool;
        RtList<ExternalMidiNote> data;

        ExternalNotes()
            : dataPool(32, 152),
              data(&dataPool) {}

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

        CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(ExternalNotes)

    } extNotes;

    struct PostRtEvents {
        CarlaMutex mutex;
        RtList<PluginPostRtEvent>::Pool dataPool;
        RtList<PluginPostRtEvent> data;
        RtList<PluginPostRtEvent> dataPendingRT;

        PostRtEvents()
            : dataPool(128, 128),
              data(&dataPool),
              dataPendingRT(&dataPool) {}

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
                dataPendingRT.spliceAppend(data, true);
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

        CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(PostRtEvents)

    } postRtEvents;

    struct PostProc {
        float dryWet;
        float volume;
        float balanceLeft;
        float balanceRight;
        float panning;

        PostProc()
            : dryWet(1.0f),
              volume(1.0f),
              balanceLeft(-1.0f),
              balanceRight(1.0f),
              panning(0.0f) {}

        CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(PostProc)

    } postProc;

    struct OSC {
        CarlaOscData data;
        CarlaPluginThread thread;

        OSC(CarlaEngine* const engine, CarlaPlugin* const plugin)
            : thread(engine, plugin) {}

        OSC() = delete;
        OSC(OSC&) = delete;
        OSC(const OSC&) = delete;
        CARLA_LEAK_DETECTOR(OSC)

    } osc;

    CarlaPluginProtectedData(CarlaEngine* const engine_, CarlaPlugin* const plugin)
        : engine(engine_),
          client(nullptr),
          gui(nullptr),
          active(false),
          needsReset(false),
          lib(nullptr),
          uiLib(nullptr),
          ctrlChannel(0),
          extraHints(0x0),
          latency(0),
          latencyBuffers(nullptr),
          osc(engine, plugin) {}

    CarlaPluginProtectedData() = delete;
    CarlaPluginProtectedData(CarlaPluginProtectedData&) = delete;
    CarlaPluginProtectedData(const CarlaPluginProtectedData&) = delete;
    CARLA_LEAK_DETECTOR(CarlaPluginProtectedData)

    ~CarlaPluginProtectedData()
    {
        CARLA_ASSERT(gui == nullptr);
        CARLA_ASSERT(! active);
        CARLA_ASSERT(! needsReset);
        CARLA_ASSERT(lib == nullptr);
        CARLA_ASSERT(uiLib == nullptr);
        CARLA_ASSERT(latency == 0);
        CARLA_ASSERT(latencyBuffers == nullptr);
    }

    // -------------------------------------------------------------------
    // Cleanup

    void cleanup()
    {
        {
            const bool lockMaster(masterMutex.tryLock());
            const bool lockSingle(singleMutex.tryLock());
            CARLA_ASSERT(! lockMaster);
            CARLA_ASSERT(! lockSingle);
        }

        if (client != nullptr)
        {
            if (client->isActive())
                client->deactivate();

            clearBuffers();

            delete client;
            client = nullptr;
        }

        for (auto it = custom.begin(); it.valid(); it.next())
        {
            CustomData& cData(*it);

            CARLA_ASSERT(cData.type != nullptr);
            CARLA_ASSERT(cData.key != nullptr);
            CARLA_ASSERT(cData.value != nullptr);

            if (cData.type != nullptr)
                delete[] cData.type;
            if (cData.key != nullptr)
                delete[] cData.key;
            if (cData.value != nullptr)
                delete[] cData.value;
        }

        prog.clear();
        midiprog.clear();
        custom.clear();

        // MUST have been unlocked before
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
            for (uint32_t i=0; i < audioIn.count; i++)
            {
                CARLA_ASSERT(latencyBuffers[i] != nullptr);

                if (latencyBuffers[i] != nullptr)
                    delete[] latencyBuffers[i];
            }

            delete[] latencyBuffers;
            latencyBuffers = nullptr;
            latency = 0;
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
            for (uint32_t i=0; i < audioIn.count; i++)
            {
                CARLA_ASSERT(latencyBuffers[i] != nullptr);

                if (latencyBuffers[i] != nullptr)
                    delete[] latencyBuffers[i];
            }

            delete[] latencyBuffers;
            latencyBuffers = nullptr;
        }

        if (audioIn.count > 0 && latency > 0)
        {
            latencyBuffers = new float*[audioIn.count];

            for (uint32_t i=0; i < audioIn.count; i++)
            {
                latencyBuffers[i] = new float[latency];
                carla_zeroFloat(latencyBuffers[i], latency);
            }
        }
    }

    // -------------------------------------------------------------------
    // Library functions, see CarlaPlugin.cpp

    bool libOpen(const char* const filename);
    bool uiLibOpen(const char* const filename);
    bool libClose();
    bool uiLibClose();
    void* libSymbol(const char* const symbol);
    void* uiLibSymbol(const char* const symbol);
    const char* libError(const char* const filename);

    // -------------------------------------------------------------------
    // Settings functions, see CarlaPlugin.cpp

    void saveSetting(const unsigned int option, const bool yesNo);
    unsigned int loadSettings(const unsigned int options, const unsigned int availOptions);

    // -------------------------------------------------------------------
    // Static helper functions

    static CarlaEngine* getEngine(CarlaPlugin* const plugin)
    {
        return plugin->kData->engine;
    }

    static CarlaEngineClient* getEngineClient(CarlaPlugin* const plugin)
    {
        return plugin->kData->client;
    }

    static CarlaEngineAudioPort* getAudioInPort(CarlaPlugin* const plugin, const uint32_t index)
    {
        return plugin->kData->audioIn.ports[index].port;
    }

    static CarlaEngineAudioPort* getAudioOutPort(CarlaPlugin* const plugin, const uint32_t index)
    {
        return plugin->kData->audioOut.ports[index].port;
    }

    static bool canRunInRack(CarlaPlugin* const plugin)
    {
        return (plugin->kData->extraHints & PLUGIN_HINT_CAN_RUN_RACK);
    }
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_PLUGIN_INTERNAL_HPP__
