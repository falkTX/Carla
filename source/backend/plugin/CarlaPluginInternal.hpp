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

#include "CarlaPluginThread.hpp"
#include "CarlaPlugin.hpp"
#include "CarlaEngine.hpp"
#include "CarlaOscUtils.hpp"
#include "CarlaStateUtils.hpp"
#include "CarlaMutex.hpp"
#include "CarlaMIDI.h"

#include "RtList.hpp"

#include <QtGui/QMainWindow>

#define CARLA_DECLARE_NON_COPY_STRUCT(structName) \
    structName(const structName&) = delete;

#define CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(structName) \
    CARLA_DECLARE_NON_COPY_STRUCT(structName) \
    CARLA_LEAK_DETECTOR(structName)

#define CARLA_PROCESS_CONTINUE_CHECK if (! fEnabled) { kData->engine->callback(CALLBACK_DEBUG, fId, 0, 0, 0.0, nullptr); return; }

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

const unsigned short MAX_RT_EVENTS   = 128;
const unsigned short MAX_MIDI_EVENTS = 512;

const unsigned int PLUGIN_HINT_HAS_MIDI_IN  = 0x1;
const unsigned int PLUGIN_HINT_HAS_MIDI_OUT = 0x2;

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
        CARLA_ASSERT(ports == nullptr);
    }

    void createNew(const uint32_t count)
    {
        CARLA_ASSERT(ports == nullptr);

        if (ports == nullptr)
            ports = new PluginAudioPort[count];

        this->count = count;
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
    ParameterData*   data;
    ParameterRanges* ranges;

    PluginParameterData()
        : count(0),
          data(nullptr),
          ranges(nullptr) {}

    ~PluginParameterData()
    {
        CARLA_ASSERT(data == nullptr);
        CARLA_ASSERT(ranges == nullptr);
    }

    void createNew(const uint32_t count)
    {
        CARLA_ASSERT(data == nullptr);
        CARLA_ASSERT(ranges == nullptr);

        if (data == nullptr)
            data = new ParameterData[count];

        if (ranges == nullptr)
            ranges = new ParameterRanges[count];

        this->count = count;
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
        CARLA_ASSERT(names == nullptr);
    }

    void createNew(const uint32_t count)
    {
        CARLA_ASSERT(names == nullptr);

        if (names == nullptr)
            names = new ProgramName[count];

        for (uint32_t i=0; i < count; i++)
            names[i] = nullptr;

        this->count = count;
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

        count   = 0;
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
        CARLA_ASSERT(data == nullptr);
    }

    void createNew(const uint32_t count)
    {
        CARLA_ASSERT(data == nullptr);

        if (data == nullptr)
            data = new MidiProgramData[count];

        this->count = count;
    }

    void clear()
    {
        if (data != nullptr)
        {
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
    double  value3;

    PluginPostRtEvent()
        : type(kPluginPostRtEventNull),
          value1(-1),
          value2(-1),
          value3(0.0) {}

    CARLA_DECLARE_NON_COPY_STRUCT(PluginPostRtEvent)
};

// -----------------------------------------------------------------------

struct ExternalMidiNote {
    int8_t  channel; // invalid = -1
    uint8_t note;
    uint8_t velo;

    ExternalMidiNote()
        : channel(-1),
          note(0),
          velo(0) {}

    CARLA_DECLARE_NON_COPY_STRUCT(ExternalMidiNote)
};

// -----------------------------------------------------------------------

class CarlaPluginGUI : public QMainWindow
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void guiClosedCallback() = 0;
    };

    CarlaPluginGUI(QWidget* const parent, Callback* const callback);

    ~CarlaPluginGUI();

private:
    Callback* const kCallback;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginGUI)
};

// -----------------------------------------------------------------------

struct CarlaPluginProtectedData {
    CarlaEngine* const engine;
    CarlaEngineClient* client;
    CarlaPluginGUI* gui;

    bool active;
    bool activeBefore;
    void* lib;

    // misc
    int8_t       ctrlInChannel;
    unsigned int extraHints;

    uint32_t latency;
    float**  latencyBuffers;

    // data
    PluginAudioData       audioIn;
    PluginAudioData       audioOut;
    PluginEventData       event;
    PluginParameterData   param;
    PluginProgramData     prog;
    PluginMidiProgramData midiprog;
    NonRtListNew<CustomData> custom;

    struct ExternalNotes {
        CarlaMutex mutex;
        RtList<ExternalMidiNote>::Pool dataPool;
        RtList<ExternalMidiNote> data;

        ExternalNotes()
            : dataPool(152, 512),
              data(&dataPool) {}

        ~ExternalNotes()
        {
            data.clear();
        }

        void append(const ExternalMidiNote& note)
        {
            data.append_sleepy(note);
        }

    } extNotes;

    struct PostRtEvents {
        CarlaMutex mutex;
        RtList<PluginPostRtEvent>::Pool dataPool;
        RtList<PluginPostRtEvent> data;
        RtList<PluginPostRtEvent> dataPendingRT;

        PostRtEvents()
            : dataPool(MAX_RT_EVENTS, MAX_RT_EVENTS),
              data(&dataPool),
              dataPendingRT(&dataPool) {}

        ~PostRtEvents()
        {
            clear();
        }

        void appendRT(const PluginPostRtEvent& event)
        {
            // FIXME!! need lock?
            dataPendingRT.append(event);
        }

        void trySplice()
        {
            if (mutex.tryLock())
            {
                dataPendingRT.splice(data, true);
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
    } postProc;

    struct OSC {
        CarlaOscData data;
        CarlaPluginThread* thread;

        OSC()
            : thread(nullptr) {}
    } osc;

    CarlaPluginProtectedData(CarlaEngine* const engine_)
        : engine(engine_),
          client(nullptr),
          gui(nullptr),
          active(false),
          activeBefore(false),
          lib(nullptr),
          ctrlInChannel(-1),
          extraHints(0x0),
          latency(0),
          latencyBuffers(nullptr) {}

    CarlaPluginProtectedData() = delete;
    CarlaPluginProtectedData(CarlaPluginProtectedData&) = delete;
    CarlaPluginProtectedData(const CarlaPluginProtectedData&) = delete;

    static CarlaEngine* getEngine(CarlaPlugin* const plugin)
    {
        return plugin->kData->engine;
    }

    static CarlaEngineAudioPort* getAudioInPort(CarlaPlugin* const plugin, uint32_t index)
    {
        return plugin->kData->audioIn.ports[index].port;
    }

    static CarlaEngineAudioPort* getAudioOutPort(CarlaPlugin* const plugin, uint32_t index)
    {
        return plugin->kData->audioOut.ports[index].port;
    }

    CARLA_LEAK_DETECTOR(CarlaPluginProtectedData)
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_PLUGIN_INTERNAL_HPP__

// common includes
//#include <cmath>
//#include <vector>
//#include <QtCore/QMutex>
//#include <QtGui/QMainWindow>

//#ifdef Q_WS_X11
//# include <QtGui/QX11EmbedContainer>
//typedef QX11EmbedContainer GuiContainer;
//#else
//# include <QtGui/QWidget>
//typedef QWidget GuiContainer;
//#endif

#if 0

/*!
 * \class CarlaPluginGUI
 *
 * \brief Carla Backend gui plugin class
 *
 * \see CarlaPlugin
 */
class CarlaPluginGUI : public QMainWindow
{
public:
    /*!
     * \class Callback
     *
     * \brief Carla plugin GUI callback
     */
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void guiClosedCallback() = 0;
    };

    // -------------------------------------------------------------------
    // Constructor and destructor

    /*!
     * TODO
     */
    CarlaPluginGUI(QWidget* const parent, Callback* const callback);

    /*!
     * TODO
     */
    ~CarlaPluginGUI();

    // -------------------------------------------------------------------
    // Get data

    /*!
     * TODO
     */
    GuiContainer* getContainer() const;

    /*!
     * TODO
     */
    WId getWinId() const;

    // -------------------------------------------------------------------
    // Set data

    /*!
     * TODO
     */
    void setNewSize(const int width, const int height);

    /*!
     * TODO
     */
    void setResizable(const bool resizable);

    /*!
     * TODO
     */
    void setTitle(const char* const title);

    /*!
     * TODO
     */
    void setVisible(const bool yesNo);

    // -------------------------------------------------------------------

private:
    Callback* const m_callback;
    GuiContainer*   m_container;

    QByteArray m_geometry;
    bool m_resizable;

    void hideEvent(QHideEvent* const event);
    void closeEvent(QCloseEvent* const event);
};
#endif
