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

#include "carla_plugin.hpp"
#include "carla_plugin_thread.hpp"

#include "carla_engine.hpp"
#include "carla_osc_utils.hpp"

//#include "carla_bridge_osc.hpp"

#include "rt_list.hpp"

#include <QtGui/QMainWindow>

#define CARLA_DECLARE_NON_COPY_STRUCT(structName) \
    structName(const structName&) = delete;

#define CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(structName) \
    CARLA_DECLARE_NON_COPY_STRUCT(structName) \
    CARLA_LEAK_DETECTOR(structName)

#define CARLA_PROCESS_CONTINUE_CHECK if (! fData->enabled) { fData->engine->callback(CALLBACK_DEBUG, fData->id, 0, 0, 0.0, nullptr); return; }

CARLA_BACKEND_START_NAMESPACE

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

    void createNew(const size_t count)
    {
        CARLA_ASSERT(ports == nullptr);

        if (ports == nullptr)
            ports = new PluginAudioPort[count];
    }

    void freePorts()
    {
        for (uint32_t i=0; i < count; i++)
        {
            if (ports[i].port != nullptr)
            {
                delete ports[i].port;
                ports[i].port = nullptr;
            }
        }
    }

    void clear()
    {
        freePorts();

        if (ports != nullptr)
        {
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

    void freePorts()
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

    void clear()
    {
        freePorts();
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

    void createNew(const size_t count)
    {
        CARLA_ASSERT(data == nullptr);
        CARLA_ASSERT(ranges == nullptr);

        if (data == nullptr)
            data = new ParameterData[count];

        if (ranges == nullptr)
            ranges = new ParameterRanges[count];
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
                std::free((void*)names[i]);

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
    CarlaPluginGUI(QWidget* const parent = nullptr);

    ~CarlaPluginGUI();

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginGUI)
};

// -----------------------------------------------------------------------

const unsigned short MIN_RT_EVENTS = 152;
const unsigned short MAX_RT_EVENTS = 512;

const unsigned int PLUGIN_OPTION2_HAS_MIDI_IN  = 0x1;
const unsigned int PLUGIN_OPTION2_HAS_MIDI_OUT = 0x2;

// -----------------------------------------------------------------------

struct CarlaPluginProtectedData {
    unsigned int id;

    CarlaEngine* const engine;
    CarlaEngineClient* client;
    CarlaPluginGUI* gui;

    unsigned int hints;
    unsigned int options;
    unsigned int options2;

    bool active;
    bool activeBefore;
    bool enabled;

    void* lib;
    CarlaString name;
    CarlaString filename;

    // misc
    int8_t ctrlInChannel;

#if 0
    uint32_t latency;
    float**  latencyBuffers;
#endif

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
        RtList<ExternalMidiNote> data;

        ExternalNotes()
            : data(32, 512) {}

        void append(const ExternalMidiNote& note)
        {
            data.append_sleepy(note);
        }

    } extNotes;

    struct PostRtEvents {
        CarlaMutex mutex;
        RtList<PluginPostRtEvent> data;
        RtList<PluginPostRtEvent> dataPendingRT;

        PostRtEvents()
            : data(MIN_RT_EVENTS, MAX_RT_EVENTS),
              dataPendingRT(MIN_RT_EVENTS, MAX_RT_EVENTS) {}

        void appendRT(const PluginPostRtEvent& event)
        {
            dataPendingRT.append(event);

            if (mutex.tryLock())
            {
                dataPendingRT.splice(data, true);
                mutex.unlock();
            }
        }

        //void appendNonRT(const PluginPostRtEvent& event)
        //{
        //    data.append_sleepy(event);
        //}

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

    CarlaPluginProtectedData(CarlaEngine* const engine_, const unsigned short id_)
        : id(id_),
          engine(engine_),
          client(nullptr),
          gui(nullptr),
          hints(0x0),
          options(0x0),
          options2(0x0),
          active(false),
          activeBefore(false),
          enabled(false),
          lib(nullptr),
          ctrlInChannel(-1) {}
#if 0
          latency(0),
          latencyBuffers(nullptr) {}
#endif

    CarlaPluginProtectedData() = delete;

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
// -------------------------------------------------------------------
// Extra

ExternalMidiNote extMidiNotes[MAX_MIDI_EVENTS];

// -------------------------------------------------------------------
// Utilities

static double fixParameterValue(double& value, const ParameterRanges& ranges)
{
    if (value < ranges.min)
        value = ranges.min;
    else if (value > ranges.max)
        value = ranges.max;
    return value;
}

static float fixParameterValue(float& value, const ParameterRanges& ranges)
{
    if (value < ranges.min)
        value = ranges.min;
    else if (value > ranges.max)
        value = ranges.max;
    return value;
}

friend class CarlaEngine; // FIXME
friend class CarlaEngineJack;
#endif

#if 0

// -------------------------------------------------------------------

/*!
 * \class ScopedDisabler
 *
 * \brief Carla plugin scoped disabler
 *
 * This is a handy class that temporarily disables a plugin during a function scope.\n
 * It should be used when the plugin needs reload or state change, something like this:
 * \code
 * {
 *      const CarlaPlugin::ScopedDisabler m(plugin);
 *      plugin->setChunkData(data);
 * }
 * \endcode
 */
class ScopedDisabler
{
public:
    /*!
     * Disable plugin \a plugin if \a disable is true.
     * The plugin is re-enabled in the deconstructor of this class if \a disable is true.
     *
     * \param plugin The plugin to disable
     * \param disable Wherever to disable the plugin or not, true by default
     */
    ScopedDisabler(CarlaPlugin* const plugin, const bool disable = true)
        : m_plugin(plugin),
          m_disable(disable)
    {
        if (m_disable)
        {
            m_plugin->engineProcessLock();
            m_plugin->setEnabled(false);
            m_plugin->engineProcessUnlock();
        }
    }

    ~ScopedDisabler()
    {
        if (m_disable)
        {
            m_plugin->engineProcessLock();
            m_plugin->setEnabled(true);
            m_plugin->engineProcessUnlock();
        }
    }

private:
    CarlaPlugin* const m_plugin;
    const bool m_disable;
};

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
