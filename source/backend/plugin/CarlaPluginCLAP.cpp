// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaClapUtils.hpp"
#include "CarlaMathUtils.hpp"

#include "CarlaPluginUI.hpp"

#ifdef CARLA_OS_MAC
# include "CarlaMacUtils.hpp"
# import <Foundation/Foundation.h>
#endif

#include "water/files/File.h"
#include "water/misc/Time.h"

#if defined(CLAP_WINDOW_API_NATIVE) && defined(_POSIX_VERSION)
# if defined(CARLA_OS_LINUX)
#  define CARLA_CLAP_POSIX_EPOLL
#  include <sys/epoll.h>
# else
#  include <sys/event.h>
#  include <sys/types.h>
# endif
#endif

// FIXME
// #ifndef CLAP_WINDOW_API_NATIVE
// #define CLAP_WINDOW_API_NATIVE ""
// #define HAVE_X11 1
// #endif

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

struct ClapEventData {
    uint16_t clapPortIndex;
    uint32_t supportedDialects;
    CarlaEngineEventPort* port;
};

struct CarlaPluginClapEventData {
    uint32_t portCount;
    ClapEventData* portData;
    ClapEventData* defaultPort; // either this->portData[x] or pData->portIn/Out

    CarlaPluginClapEventData() noexcept
        : portCount(0),
          portData(nullptr),
          defaultPort(nullptr) {}

    ~CarlaPluginClapEventData() noexcept
    {
        CARLA_SAFE_ASSERT_INT(portCount == 0, portCount);
        CARLA_SAFE_ASSERT(portData == nullptr);
        CARLA_SAFE_ASSERT(defaultPort == nullptr);
    }

    void createNew(const uint32_t newCount)
    {
        CARLA_SAFE_ASSERT_INT(portCount == 0, portCount);
        CARLA_SAFE_ASSERT_RETURN(portData == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(defaultPort == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(newCount > 0,);

        portData  = new ClapEventData[newCount];
        portCount = newCount;

        defaultPort = nullptr;
    }

    void clear(CarlaEngineEventPort* const portToIgnore) noexcept
    {
        if (portData != nullptr)
        {
            for (uint32_t i=0; i < portCount; ++i)
            {
                if (portData[i].port != nullptr)
                {
                    if (portData[i].port != portToIgnore)
                        delete portData[i].port;
                    portData[i].port = nullptr;
                }
            }

            delete[] portData;
            portData = nullptr;
        }

        portCount = 0;

        defaultPort = nullptr;
    }

    void initBuffers() const noexcept
    {
        for (uint32_t i=0; i < portCount; ++i)
        {
            if (portData[i].port != nullptr && (defaultPort == nullptr || portData[i].port != defaultPort->port))
                portData[i].port->initBuffer();
        }
    }

    CARLA_DECLARE_NON_COPYABLE(CarlaPluginClapEventData)
};

#ifdef _POSIX_VERSION
// --------------------------------------------------------------------------------------------------------------------

struct HostPosixFileDescriptorDetails {
    int hostFd;
    int pluginFd;
    clap_posix_fd_flags_t flags;
};

static constexpr const HostPosixFileDescriptorDetails kPosixFileDescriptorFallback   = { -1, -1, 0x0 };
static /*           */ HostPosixFileDescriptorDetails kPosixFileDescriptorFallbackNC = { -1, -1, 0x0 };
#endif

// --------------------------------------------------------------------------------------------------------------------

struct HostTimerDetails {
    clap_id clapId;
    uint32_t periodInMs;
    uint32_t lastCallTimeInMs;
};

static constexpr const HostTimerDetails kTimerFallback   = { CLAP_INVALID_ID, 0, 0 };
static /*           */ HostTimerDetails kTimerFallbackNC = { CLAP_INVALID_ID, 0, 0 };

// --------------------------------------------------------------------------------------------------------------------

struct carla_clap_host : clap_host_t {
    class Callbacks {
    public:
        virtual ~Callbacks() {}
        virtual void clapRequestRestart() = 0;
        virtual void clapRequestProcess() = 0;
        virtual void clapRequestCallback() = 0;
        virtual void clapMarkDirty() = 0;
        virtual void clapLatencyChanged() = 0;
      #ifdef CLAP_WINDOW_API_NATIVE
        // gui
        virtual void clapGuiResizeHintsChanged() = 0;
        virtual bool clapGuiRequestResize(uint width, uint height) = 0;
        virtual bool clapGuiRequestShow() = 0;
        virtual bool clapGuiRequestHide() = 0;
        virtual void clapGuiClosed(bool wasDestroyed) = 0;
       #ifdef _POSIX_VERSION
        // posix fd
        virtual bool clapRegisterPosixFD(int fd, clap_posix_fd_flags_t flags) = 0;
        virtual bool clapModifyPosixFD(int fd, clap_posix_fd_flags_t flags) = 0;
        virtual bool clapUnregisterPosixFD(int fd) = 0;
       #endif
        // timer
        virtual bool clapRegisterTimer(uint32_t periodInMs, clap_id* timerId) = 0;
        virtual bool clapUnregisterTimer(clap_id timerId) = 0;
      #endif
    };

    Callbacks* const hostCallbacks;

    clap_host_latency_t latency;
    clap_host_state_t state;
  #ifdef CLAP_WINDOW_API_NATIVE
    clap_host_gui_t gui;
   #ifdef _POSIX_VERSION
    clap_host_posix_fd_support_t posixFD;
   #endif
    clap_host_timer_support_t timer;
  #endif

    carla_clap_host(Callbacks* const hostCb)
        : hostCallbacks(hostCb)
    {
        clap_version = CLAP_VERSION;
        host_data = this;
        name = "Carla";
        vendor = "falkTX";
        url = "https://kx.studio/carla";
        version = CARLA_VERSION_STRING;

        get_extension = carla_get_extension;
        request_restart = carla_request_restart;
        request_process = carla_request_process;
        request_callback = carla_request_callback;

        latency.changed = carla_latency_changed;

        state.mark_dirty = carla_mark_dirty;

      #ifdef CLAP_WINDOW_API_NATIVE
        gui.resize_hints_changed = carla_resize_hints_changed;
        gui.request_resize = carla_request_resize;
        gui.request_show = carla_request_show;
        gui.request_hide = carla_request_hide;
        gui.closed = carla_closed;

       #ifdef _POSIX_VERSION
        posixFD.register_fd = carla_register_fd;
        posixFD.modify_fd = carla_modify_fd;
        posixFD.unregister_fd = carla_unregister_fd;
       #endif

        timer.register_timer = carla_register_timer;
        timer.unregister_timer = carla_unregister_timer;
      #endif
    }

    static const void* CLAP_ABI carla_get_extension(const clap_host_t* const host, const char* const extension_id)
    {
        carla_clap_host* const self = static_cast<carla_clap_host*>(host->host_data);

        if (std::strcmp(extension_id, CLAP_EXT_LATENCY) == 0)
            return &self->latency;
        if (std::strcmp(extension_id, CLAP_EXT_STATE) == 0)
            return &self->state;
      #ifdef CLAP_WINDOW_API_NATIVE
        if (std::strcmp(extension_id, CLAP_EXT_GUI) == 0)
            return &self->gui;
       #ifdef _POSIX_VERSION
        if (std::strcmp(extension_id, CLAP_EXT_POSIX_FD_SUPPORT) == 0)
            return &self->posixFD;
       #endif
        if (std::strcmp(extension_id, CLAP_EXT_TIMER_SUPPORT) == 0)
            return &self->timer;
      #endif

        carla_stderr("Plugin requested unsupported CLAP extension '%s'", extension_id);
        return nullptr;
    }

    static void CLAP_ABI carla_request_restart(const clap_host_t* const host)
    {
        static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapRequestRestart();
    }

    static void CLAP_ABI carla_request_process(const clap_host_t* const host)
    {
        static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapRequestProcess();
    }

    static void CLAP_ABI carla_request_callback(const clap_host_t* const host)
    {
        static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapRequestCallback();
    }

    static void CLAP_ABI carla_latency_changed(const clap_host_t* const host)
    {
        static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapLatencyChanged();
    }

    static void CLAP_ABI carla_mark_dirty(const clap_host_t* const host)
    {
        static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapMarkDirty();
    }

  #ifdef CLAP_WINDOW_API_NATIVE
    static void CLAP_ABI carla_resize_hints_changed(const clap_host_t* const host)
    {
        static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapGuiResizeHintsChanged();
    }

    static bool CLAP_ABI carla_request_resize(const clap_host_t* const host, const uint32_t width, const uint32_t height)
    {
        return static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapGuiRequestResize(width, height);
    }

    static bool CLAP_ABI carla_request_show(const clap_host_t* const host)
    {
        return static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapGuiRequestShow();
    }

    static bool CLAP_ABI carla_request_hide(const clap_host_t* const host)
    {
        return static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapGuiRequestHide();
    }

    static void CLAP_ABI carla_closed(const clap_host_t* const host, bool was_destroyed)
    {
        static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapGuiClosed(was_destroyed);
    }

   #ifdef _POSIX_VERSION
    static bool CLAP_ABI carla_register_fd(const clap_host_t* const host, const int fd, const clap_posix_fd_flags_t flags)
    {
        return static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapRegisterPosixFD(fd, flags);
    }

    static bool CLAP_ABI carla_modify_fd(const clap_host_t* const host, const int fd, const clap_posix_fd_flags_t flags)
    {
        return static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapModifyPosixFD(fd, flags);
    }

    static bool CLAP_ABI carla_unregister_fd(const clap_host_t* const host, const int fd)
    {
        return static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapUnregisterPosixFD(fd);
    }
   #endif

    static bool CLAP_ABI carla_register_timer(const clap_host_t* const host, const uint32_t period_ms, clap_id* const timer_id)
    {
        return static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapRegisterTimer(period_ms, timer_id);
    }

    static bool CLAP_ABI carla_unregister_timer(const clap_host_t* const host, const clap_id timer_id)
    {
        return static_cast<const carla_clap_host*>(host->host_data)->hostCallbacks->clapUnregisterTimer(timer_id);
    }
  #endif
};

// --------------------------------------------------------------------------------------------------------------------

struct carla_clap_input_audio_buffers {
    clap_audio_buffer_const_t* buffers;
    clap_audio_buffer_extra_data_t* extra;
    uint32_t count;

    carla_clap_input_audio_buffers() noexcept
        : buffers(nullptr),
          extra(nullptr),
          count(0) {}

    ~carla_clap_input_audio_buffers()
    {
        delete[] buffers;
        delete[] extra;
    }

    void realloc(const uint32_t portCount)
    {
        delete[] buffers;
        delete[] extra;
        count = portCount;

        if (portCount != 0)
        {
            buffers = new clap_audio_buffer_const_t[portCount];
            extra = new clap_audio_buffer_extra_data_t[portCount];
            carla_zeroStructs(buffers, portCount);
            carla_zeroStructs(extra, portCount);
        }
        else
        {
            buffers = nullptr;
            extra = nullptr;
        }
    }

    const clap_audio_buffer_t* cast() const noexcept
    {
        return static_cast<const clap_audio_buffer_t*>(static_cast<const void*>(buffers));
    }
};

struct carla_clap_output_audio_buffers {
    clap_audio_buffer_t* buffers;
    clap_audio_buffer_extra_data_t* extra;
    uint32_t count;

    carla_clap_output_audio_buffers() noexcept
        : buffers(nullptr),
          extra(nullptr),
          count(0) {}

    ~carla_clap_output_audio_buffers()
    {
        delete[] buffers;
        delete[] extra;
    }

    void realloc(const uint32_t portCount)
    {
        delete[] buffers;
        delete[] extra;
        count = portCount;

        if (portCount != 0)
        {
            buffers = new clap_audio_buffer_t[portCount];
            extra = new clap_audio_buffer_extra_data_t[portCount];
            carla_zeroStructs(buffers, portCount);
            carla_zeroStructs(extra, portCount);
        }
        else
        {
            buffers = nullptr;
            extra = nullptr;
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

struct carla_clap_input_events : clap_input_events_t, CarlaPluginClapEventData {
    union Event {
        clap_event_header_t header;
        clap_event_param_value_t param;
        clap_event_param_gesture_t gesture;
        clap_event_midi_t midi;
        clap_event_note_t note;
        clap_event_midi_sysex_t sysex;
    };

    struct ScheduledParameterUpdate {
        bool updated;
        double value;
        clap_id clapId;
        void* cookie;

        ScheduledParameterUpdate()
          : updated(false),
            value(0.f),
            clapId(0),
            cookie(0) {}
    };

    Event* events;
    ScheduledParameterUpdate* updatedParams;

    uint32_t numEventsAllocated;
    uint32_t numEventsUsed;
    uint32_t numParams;

    carla_clap_input_events()
        : CarlaPluginClapEventData(),
          events(nullptr),
          updatedParams(nullptr),
          numEventsAllocated(0),
          numEventsUsed(0),
          numParams(0)
    {
        ctx = this;
        size = carla_size;
        get = carla_get;
    }

    ~carla_clap_input_events()
    {
        delete[] events;
        delete[] updatedParams;
    }

    // called on plugin reload
    // NOTE: clapId and cookie must be separately set outside this function
    void realloc(CarlaEngineEventPort* const defPortIn, const uint32_t portCount, const uint32_t paramCount)
    {
        numEventsUsed = 0;
        numParams = paramCount;
        delete[] events;
        delete[] updatedParams;

        if (portCount != 0 || paramCount != 0)
        {
            static_assert(kPluginMaxMidiEvents > MAX_MIDI_NOTE, "Enough space for input events");

            numEventsAllocated = paramCount * 2 + kPluginMaxMidiEvents * std::max(1u, portCount);
            events = new Event[numEventsAllocated];
            updatedParams = new ScheduledParameterUpdate[paramCount];
        }
        else
        {
            numEventsAllocated = 0;
            events = nullptr;
            updatedParams = nullptr;
        }

        CarlaPluginClapEventData::clear(defPortIn);

        if (portCount != 0)
            CarlaPluginClapEventData::createNew(portCount);
    }

    // used for temporary copies (this instance must be empty)
    void reallocEqualTo(const carla_clap_input_events& other)
    {
        numParams = other.numParams;
        numEventsAllocated = other.numEventsAllocated;

        if (numEventsAllocated != 0)
        {
            events = new Event[numEventsAllocated];
            updatedParams = new ScheduledParameterUpdate[numParams];

            for (uint32_t i=0; i<numParams; ++i)
            {
                updatedParams[i].clapId = other.updatedParams[i].clapId;
                updatedParams[i].cookie = other.updatedParams[i].cookie;
            }
        }
    }

    void swap(carla_clap_input_events& other)
    {
        CARLA_SAFE_ASSERT_RETURN(numParams == other.numParams,);
        CARLA_SAFE_ASSERT_RETURN(numEventsAllocated == other.numEventsAllocated,);

        std::swap(numEventsUsed, other.numEventsUsed);
        std::swap(updatedParams, other.updatedParams);
        std::swap(events, other.events);
    }

    const clap_input_events_t* cast() const noexcept
    {
        return static_cast<const clap_input_events_t*>(this);
    }

    // called just before plugin processing
    void handleScheduledParameterUpdates()
    {
        uint32_t count = 0;

        for (uint32_t i=0; i<numParams; ++i)
        {
            if (updatedParams[i].updated)
            {
                events[count++].param = {
                    { sizeof(clap_event_param_value_t), 0, 0, CLAP_EVENT_PARAM_VALUE, 0 },
                    updatedParams[i].clapId,
                    updatedParams[i].cookie,
                    -1, -1, -1, -1,
                    updatedParams[i].value
                };

                updatedParams[i].updated = false;
            }
        }

        numEventsUsed = count;
    }

    // called when a parameter is set from non-rt thread
    void setParamValue(const uint32_t index, const float value) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(index < numParams,);

        updatedParams[index].value = value;
        updatedParams[index].updated = true;
    }

    // called when a parameter is set from rt thread
    void setParamValueRT(const uint32_t index, const float value, const uint32_t frameOffset) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(index < numParams,);

        if (numEventsUsed == numEventsAllocated)
            return;

        events[numEventsUsed++].param = {
            { sizeof(clap_event_param_value_t), frameOffset, 0, CLAP_EVENT_PARAM_VALUE, CLAP_EVENT_IS_LIVE },
            updatedParams[index].clapId,
            updatedParams[index].cookie,
            -1, -1, -1, -1,
            value
        };
    }

    void addSimpleMidiEvent(const bool isLive, const uint16_t port, const uint32_t frameOffset, const uint8_t data[3])
    {
        if (numEventsUsed == numEventsAllocated)
            return;

        events[numEventsUsed++].midi = {
            { sizeof(clap_event_midi_t), frameOffset, 0, CLAP_EVENT_MIDI, isLive ? (uint32_t)CLAP_EVENT_IS_LIVE : 0u },
            port,
            { data[0], data[1], data[2] }
        };
    }

    void addSimpleNoteEvent(const bool isLive, const int16_t port, const uint32_t frameOffset,
                            const uint8_t channel, const uint8_t key, const uint8_t velocity)
    {
        if (numEventsUsed == numEventsAllocated)
            return;

        const uint16_t eventType = velocity > 0 ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF;

        events[numEventsUsed++].note = {
            { sizeof(clap_event_note_t), frameOffset, 0, eventType, isLive ? (uint32_t)CLAP_EVENT_IS_LIVE : 0u },
            -1,
            port,
            channel,
            key,
            static_cast<double>(velocity) / 127.0
        };
    }

    static uint32_t CLAP_ABI carla_size(const clap_input_events_t* const list) noexcept
    {
        return static_cast<const carla_clap_input_events*>(list->ctx)->numEventsUsed;
    }

    static const clap_event_header_t* CLAP_ABI carla_get(const clap_input_events_t* const list, const uint32_t index) noexcept
    {
        return &static_cast<const carla_clap_input_events*>(list->ctx)->events[index].header;
    }
};

// --------------------------------------------------------------------------------------------------------------------

struct carla_clap_output_events : clap_output_events_t, CarlaPluginClapEventData {
    union Event {
        clap_event_header_t header;
        clap_event_param_value_t param;
        clap_event_midi_t midi;
    };

    Event* events;
    uint32_t numEventsAllocated;
    uint32_t numEventsUsed;

    carla_clap_output_events()
        : events(nullptr),
          numEventsAllocated(0),
          numEventsUsed(0)
    {
        ctx = this;
        try_push = carla_try_push;
    }

    ~carla_clap_output_events()
    {
        delete[] events;
    }

    // called on plugin reload
    void realloc(CarlaEngineEventPort* const defPortOut, const uint32_t portCount, const uint32_t paramCount)
    {
        numEventsUsed = 0;
        delete[] events;

        if (portCount != 0 || paramCount != 0)
        {
            numEventsAllocated = paramCount + kPluginMaxMidiEvents * std::max(1u, portCount);
            events = new Event[numEventsAllocated];
        }
        else
        {
            numEventsAllocated = 0;
            events = nullptr;
        }

        CarlaPluginClapEventData::clear(defPortOut);

        if (portCount != 0)
            CarlaPluginClapEventData::createNew(portCount);
    }

    const clap_output_events_t* cast() const noexcept
    {
        return static_cast<const clap_output_events_t*>(this);
    }

    bool tryPush(const clap_event_header_t* const event)
    {
        if (numEventsUsed == numEventsAllocated)
            return false;

        Event e;
        switch (event->type)
        {
        case CLAP_EVENT_PARAM_VALUE:
            e.param = *static_cast<const clap_event_param_value_t*>(static_cast<const void*>(event));
            break;
        case CLAP_EVENT_MIDI:
            e.midi = *static_cast<const clap_event_midi_t*>(static_cast<const void*>(event));
            break;
        case CLAP_EVENT_PARAM_GESTURE_BEGIN:
        case CLAP_EVENT_PARAM_GESTURE_END:
            // TODO for now be nice to the plugins that require this
            return true;
        default:
            return false;
        }

        events[numEventsUsed++] = e;
        return true;
    }

    static bool CLAP_ABI carla_try_push(const clap_output_events_t* const list, const clap_event_header_t* const event)
    {
        return static_cast<carla_clap_output_events*>(list->ctx)->tryPush(event);
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CarlaPluginCLAP : public CarlaPlugin,
                       #ifdef CLAP_WINDOW_API_NATIVE
                        private CarlaPluginUI::Callback,
                       #endif
                        private carla_clap_host::Callbacks
{
public:
    CarlaPluginCLAP(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fPlugin(nullptr),
          fPluginDescriptor(nullptr),
          fPluginEntry(nullptr),
          fHost(this),
          fExtensions(),
          fInputAudioBuffers(),
          fOutputAudioBuffers(),
          fInputEvents(),
          fOutputEvents(),
         #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
          fAudioOutBuffers(nullptr),
         #endif
          fLastChunk(nullptr),
          fLastKnownLatency(0),
          kEngineHasIdleOnMainThread(engine->hasIdleOnMainThread()),
          fNeedsParamFlush(false),
          fNeedsRestart(false),
          fNeedsProcess(false),
          fNeedsIdleCallback(false)
    {
        carla_debug("CarlaPluginCLAP::CarlaPluginCLAP(%p, %i)", engine, id);
    }

    ~CarlaPluginCLAP() override
    {
        carla_debug("CarlaPluginCLAP::~CarlaPluginCLAP()");

        runIdleCallbacksAsNeeded(false);

       #ifdef CLAP_WINDOW_API_NATIVE
        // close UI
        if (fUI.isCreated)
            showCustomUI(false);
       #endif

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fPlugin != nullptr)
        {
            fPlugin->destroy(fPlugin);
            fPlugin = nullptr;
        }

        if (fLastChunk != nullptr)
        {
            std::free(fLastChunk);
            fLastChunk = nullptr;
        }

        clearBuffers();

        if (fPluginEntry != nullptr)
        {
            fPluginEntry->deinit();
            fPluginEntry = nullptr;
        }
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_CLAP;
    }

    PluginCategory getCategory() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPluginDescriptor != nullptr, PLUGIN_CATEGORY_NONE);

        if (fPluginDescriptor->features == nullptr)
            return PLUGIN_CATEGORY_NONE;

        return getPluginCategoryFromClapFeatures(fPluginDescriptor->features);
    }

    uint32_t getLatencyInFrames() const noexcept override
    {
        // under clap we can only request plugin latency in main thread,
        // which is unsuitable for this call
        return fLastKnownLatency;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getMidiInCount() const noexcept override
    {
        return fInputEvents.portCount;
    }

    uint32_t getMidiOutCount() const noexcept override
    {
        return fOutputEvents.portCount;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    uint getAudioPortHints(const bool isOutput, const uint32_t portIndex) const noexcept override
    {
        uint hints = 0x0;

        if (isOutput)
        {
            for (uint32_t i=0, j=0; i<fOutputAudioBuffers.count; ++i, j+=fOutputAudioBuffers.buffers[i].channel_count)
            {
                if (j != portIndex)
                    continue;

                if (!fOutputAudioBuffers.extra[i].isMain)
                    hints |= AUDIO_PORT_IS_SIDECHAIN;
            }
        }
        else
        {
            for (uint32_t i=0, j=0; i<fInputAudioBuffers.count; ++i, j+=fInputAudioBuffers.buffers[i].channel_count)
            {
                if (j != portIndex)
                    continue;

                if (!fInputAudioBuffers.extra[i].isMain)
                    hints |= AUDIO_PORT_IS_SIDECHAIN;
            }
        }

        return hints;
    }

    std::size_t getChunkData(void** const dataPtr) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS, 0);
        CARLA_SAFE_ASSERT_RETURN(fExtensions.state != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);

        std::free(fLastChunk);

        clap_ostream_impl stream;
        if (fExtensions.state->save(fPlugin, &stream))
        {
            *dataPtr = fLastChunk = stream.buffer;
            runIdleCallbacksAsNeeded(false);
            return stream.size;
        }
        else
        {
            *dataPtr = fLastChunk = nullptr;
            runIdleCallbacksAsNeeded(false);
            return 0;
        }
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        uint options = 0x0;

        if (fExtensions.state != nullptr)
            options |= PLUGIN_OPTION_USE_CHUNKS;

        for (uint32_t i=0; i<fInputEvents.portCount; ++i)
        {
            if (fInputEvents.portData[i].supportedDialects & CLAP_NOTE_DIALECT_MIDI)
            {
                options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
                options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                options |= PLUGIN_OPTION_SEND_PITCHBEND;
                options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
                options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;
                options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
                break;
            }
            if (fInputEvents.portData[i].supportedDialects & CLAP_NOTE_DIALECT_CLAP)
            {
                options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
                // nobreak, in case another port supports MIDI
            }
        }

        return options;
    }

    double getBestParameterValue(const uint32_t parameterId, const clap_id clapId) const noexcept
    {
        if (fInputEvents.updatedParams[parameterId].updated)
            return fInputEvents.updatedParams[parameterId].value;

        double value;
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params->get_value(fPlugin, clapId, &value), 0.0);

        return value;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr, 0.f);
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params != nullptr, 0.f);

        return getBestParameterValue(parameterId, pData->param.data[parameterId].rindex);
    }

    bool getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPluginDescriptor != nullptr, false);

        std::strncpy(strBuf, fPluginDescriptor->id, STR_MAX);
        return true;
    }

    bool getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPluginDescriptor != nullptr, false);

        std::strncpy(strBuf, fPluginDescriptor->vendor, STR_MAX);
        return true;
    }

    bool getCopyright(char* const strBuf) const noexcept override
    {
        return getMaker(strBuf);
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPluginDescriptor != nullptr, false);

        std::strncpy(strBuf, fPluginDescriptor->name, STR_MAX);
        return true;
    }

    bool getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        clap_param_info_t paramInfo = {};
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params->get_info(fPlugin, parameterId, &paramInfo), false);

        std::strncpy(strBuf, paramInfo.name, STR_MAX);
        strBuf[STR_MAX-1] = '\0';
        return true;
    }

    bool getParameterSymbol(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        const clap_id clapId = pData->param.data[parameterId].rindex;
        std::snprintf(strBuf, STR_MAX, "%u", clapId);
        return true;
    }

    bool getParameterText(const uint32_t parameterId, char* const strBuf) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        const clap_id clapId = pData->param.data[parameterId].rindex;
        const double value = getBestParameterValue(parameterId, clapId);

        return fExtensions.params->value_to_text(fPlugin, clapId, value, strBuf, STR_MAX);
    }

    bool getParameterGroupName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        clap_param_info_t paramInfo = {};
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params->get_info(fPlugin, parameterId, &paramInfo), false);

        if (paramInfo.module[0] == '\0')
            return false;

        if (char* const sep = std::strrchr(paramInfo.module, '/'))
        {
            paramInfo.module[STR_MAX/2-2] = sep[0] = '\0';

            char strBuf2[STR_MAX/2];
            std::strncpy(strBuf2, paramInfo.module, STR_MAX/2);
            strBuf2[STR_MAX/2-1] = '\0';

            std::snprintf(strBuf, STR_MAX, "%s:%s", strBuf2, strBuf2);
            strBuf[STR_MAX-1] = '\0';
            return true;
        }

        return false;
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

   #ifdef CLAP_WINDOW_API_NATIVE
    void setName(const char* const newName) override
    {
        CarlaPlugin::setName(newName);

        if (fUI.isCreated && pData->uiTitle.isEmpty())
            setWindowTitle(nullptr);
    }
   #endif

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue = pData->param.getFixedValue(parameterId, value);
        fInputEvents.setParamValue(parameterId, fixedValue);

        if (!pData->active && fExtensions.params->flush != nullptr)
            fNeedsParamFlush = true;

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setParameterValueRT(const uint32_t parameterId, const float value, const uint32_t frameOffset, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue = pData->param.getFixedValue(parameterId, value);
        fInputEvents.setParamValueRT(parameterId, fixedValue, frameOffset);

        CarlaPlugin::setParameterValueRT(parameterId, fixedValue, frameOffset, sendCallbackLater);
    }

    void setChunkData(const void* const data, const std::size_t dataSize) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(fExtensions.state != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(dataSize > 0,);

        const clap_istream_impl stream(data, dataSize);
        if (fExtensions.state->load(fPlugin, &stream))
            pData->updateParameterValues(this, true, true, false);

        runIdleCallbacksAsNeeded(false);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

   #ifdef CLAP_WINDOW_API_NATIVE
    void setWindowTitle(const char* const title) noexcept
    {
        if (!fUI.isCreated)
            return;

        String uiTitle;

        if (title != nullptr)
        {
            uiTitle = title;
        }
        else
        {
            uiTitle  = pData->name;
            uiTitle += " (GUI)";
        }

        if (fUI.isEmbed)
        {
            if (fUI.window != nullptr)
                fUI.window->setTitle(uiTitle.buffer());
        }
        else
        {
            fExtensions.gui->suggest_title(fPlugin, uiTitle.buffer());
        }
    }

    void setCustomUITitle(const char* const title) noexcept override
    {
        setWindowTitle(title);
        CarlaPlugin::setCustomUITitle(title);
    }

    void showCustomUI(const bool yesNo) override
    {
        CARLA_SAFE_ASSERT_RETURN(fExtensions.gui != nullptr,);

        if (fUI.isVisible == yesNo)
            return;

        if (yesNo)
        {
            if (fUI.isVisible)
            {
                fExtensions.gui->show(fPlugin);

                if (fUI.isEmbed)
                {
                    CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
                    fUI.window->show();
                    fUI.window->focus();
                }

                runIdleCallbacksAsNeeded(false);
                return;
            }

            if (!fUI.initalized)
            {
                fUI.isEmbed = fExtensions.gui->is_api_supported(fPlugin, CLAP_WINDOW_API_NATIVE, false);
                fUI.initalized = true;
            }

            if (!fUI.isCreated)
            {
                if (!fExtensions.gui->create(fPlugin, CLAP_WINDOW_API_NATIVE, !fUI.isEmbed))
                {
                    pData->engine->callback(true, true,
                                            ENGINE_CALLBACK_UI_STATE_CHANGED,
                                            pData->id,
                                            -1,
                                            0, 0, 0.0f,
                                            "Plugin refused to open its own UI");
                    return;
                }
                fUI.isCreated = true;
            }

            const bool resizable = fExtensions.gui->can_resize(fPlugin);

            const EngineOptions& opts(pData->engine->getOptions());

           #if defined(CARLA_OS_WIN)
            fUI.window = CarlaPluginUI::newWindows(this, opts.frontendWinId, opts.pluginsAreStandalone, resizable);
           #elif defined(CARLA_OS_MAC)
            fUI.window = CarlaPluginUI::newCocoa(this, opts.frontendWinId, opts.pluginsAreStandalone, resizable);
           #elif defined(HAVE_X11)
            fUI.window = CarlaPluginUI::newX11(this, opts.frontendWinId, opts.pluginsAreStandalone, resizable, false);
           #else
            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_UI_STATE_CHANGED,
                                    pData->id,
                                    -1,
                                    0, 0, 0.0f,
                                    "Unsupported UI type");
            return;
           #endif

           #ifndef CARLA_OS_MAC
            if (carla_isNotZero(opts.uiScale))
                fExtensions.gui->set_scale(fPlugin, opts.uiScale);
           #endif

            setWindowTitle(nullptr);

            if (fUI.isEmbed)
            {
                clap_window_t win = { CLAP_WINDOW_API_NATIVE, {} };
                win.ptr = fUI.window->getPtr();

                fExtensions.gui->set_parent(fPlugin, &win);

                uint32_t width, height;
                if (fExtensions.gui->get_size(fPlugin, &width, &height))
                {
                    fUI.isResizingFromInit = true;
                    fUI.width = width;
                    fUI.height = height;
                    fUI.window->setSize(width, height, true, true);
                }

                fExtensions.gui->show(fPlugin);
                fUI.window->show();
            }
            else
            {
                clap_window_t win = { CLAP_WINDOW_API_NATIVE, {} };
                win.uptr = opts.frontendWinId;
                fExtensions.gui->set_transient(fPlugin, &win);
                fExtensions.gui->show(fPlugin);
               #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                pData->tryTransient();
               #endif
            }

            fUI.isVisible = true;
        }
        else
        {
            fUI.isVisible = false;

           #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            pData->transientTryCounter = 0;
           #endif

            if (fUI.window != nullptr)
                fUI.window->hide();

            fExtensions.gui->hide(fPlugin);

            if (fUI.isCreated)
            {
                fExtensions.gui->destroy(fPlugin);
                fUI.isCreated = false;
            }

            if (fUI.window != nullptr)
            {
                delete fUI.window;
                fUI.window = nullptr;
            }
        }

        runIdleCallbacksAsNeeded(true);
    }

    void* embedCustomUI(void* const ptr) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.window == nullptr, nullptr);

        if (!fUI.initalized)
        {
            fUI.isEmbed = fExtensions.gui->is_api_supported(fPlugin, CLAP_WINDOW_API_NATIVE, false);
            fUI.initalized = true;
        }

        if (!fUI.isCreated)
        {
            if (!fExtensions.gui->create(fPlugin, CLAP_WINDOW_API_NATIVE, false))
            {
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_UI_STATE_CHANGED,
                                        pData->id,
                                        -1,
                                        0, 0, 0.0f,
                                        "Plugin refused to open its own UI");
                return nullptr;
            }
            fUI.isCreated = true;
        }

        fUI.isVisible = true;

       #ifndef CARLA_OS_MAC
        const EngineOptions& opts(pData->engine->getOptions());

        if (carla_isNotZero(opts.uiScale))
            fExtensions.gui->set_scale(fPlugin, opts.uiScale);
       #endif

        clap_window_t win = { CLAP_WINDOW_API_NATIVE, {} };
        win.ptr = ptr;

        fExtensions.gui->set_parent(fPlugin, &win);

        uint32_t width, height;
        if (fExtensions.gui->get_size(fPlugin, &width, &height))
        {
            fUI.isResizingFromInit = true;
            fUI.width = width;
            fUI.height = height;
            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_EMBED_UI_RESIZED,
                                    pData->id, width, height,
                                    0, 0.0f, nullptr);
        }

        fExtensions.gui->show(fPlugin);

        return nullptr;
    }
   #endif

    void idle() override
    {
        if (kEngineHasIdleOnMainThread)
            runIdleCallbacksAsNeeded(true);

        CarlaPlugin::idle();
    }

    void uiIdle() override
    {
       #ifdef CLAP_WINDOW_API_NATIVE
        if (fUI.shouldClose)
        {
            fUI.shouldClose = false;
            fUI.isResizingFromHost = fUI.isResizingFromInit = false;
            fUI.isResizingFromPlugin = 0;

            showCustomUI(false);
            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_UI_STATE_CHANGED,
                                    pData->id,
                                    0,
                                    0, 0, 0.0f, nullptr);
        }

        if (fUI.isResizingFromHost)
        {
            fUI.isResizingFromHost = false;

            if (fUI.isResizingFromPlugin == 0 && !fUI.isResizingFromInit == false)
            {
                carla_stdout("Host resize restarted");
                fExtensions.gui->set_size(fPlugin, fUI.width, fUI.height);
            }
        }

        if (fUI.window != nullptr)
            fUI.window->idle();

        if (fUI.isResizingFromPlugin == 2)
        {
            fUI.isResizingFromPlugin = 1;
        }
        else if (fUI.isResizingFromPlugin == 1)
        {
            fUI.isResizingFromPlugin = 0;
            carla_stdout("Plugin resize stopped");
        }
       #endif

        if (!kEngineHasIdleOnMainThread)
            runIdleCallbacksAsNeeded(true);

        CarlaPlugin::uiIdle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        carla_debug("CarlaPluginCLAP::reload() - start");

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        const clap_plugin_audio_ports_t* audioPortsExt = static_cast<const clap_plugin_audio_ports_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_AUDIO_PORTS));

        const clap_plugin_latency_t* latencyExt = static_cast<const clap_plugin_latency_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_LATENCY));

        const clap_plugin_note_ports_t* notePortsExt = static_cast<const clap_plugin_note_ports_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_NOTE_PORTS));

        const clap_plugin_params_t* paramsExt = static_cast<const clap_plugin_params_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_PARAMS));

        const clap_plugin_state_t* stateExt = static_cast<const clap_plugin_state_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_STATE));

        const clap_plugin_timer_support_t* timerExt = static_cast<const clap_plugin_timer_support_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_TIMER_SUPPORT));

        if (audioPortsExt != nullptr && (audioPortsExt->count == nullptr || audioPortsExt->get == nullptr))
            audioPortsExt = nullptr;

        if (latencyExt != nullptr && latencyExt->get == nullptr)
            latencyExt = nullptr;

        if (notePortsExt != nullptr && (notePortsExt->count == nullptr || notePortsExt->get == nullptr))
            notePortsExt = nullptr;

        if (paramsExt != nullptr && (paramsExt->count == nullptr || paramsExt->get_info == nullptr))
            paramsExt = nullptr;

        if (stateExt != nullptr && (stateExt->save == nullptr || stateExt->load == nullptr))
            stateExt = nullptr;

        if (timerExt != nullptr && timerExt->on_timer == nullptr)
            timerExt = nullptr;

        fExtensions.latency = latencyExt;
        fExtensions.params = paramsExt;
        fExtensions.state = stateExt;
        fExtensions.timer = timerExt;

       #ifdef CLAP_WINDOW_API_NATIVE
        const clap_plugin_gui_t* guiExt = static_cast<const clap_plugin_gui_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_GUI));

        if (guiExt != nullptr && (guiExt->is_api_supported == nullptr
                                  || guiExt->create == nullptr
                                  || guiExt->destroy == nullptr
                                  || guiExt->set_scale == nullptr
                                  || guiExt->get_size == nullptr
                                  || guiExt->can_resize == nullptr
                                  || guiExt->get_resize_hints == nullptr
                                  || guiExt->adjust_size == nullptr
                                  || guiExt->set_size == nullptr
                                  || guiExt->set_parent == nullptr
                                  || guiExt->set_transient == nullptr
                                  || guiExt->suggest_title == nullptr
                                  || guiExt->show == nullptr
                                  || guiExt->hide == nullptr))
            guiExt = nullptr;

        fExtensions.gui = guiExt;
       #endif

       #if defined(CLAP_WINDOW_API_NATIVE) && defined(_POSIX_VERSION)
        const clap_plugin_posix_fd_support_t* posixFdExt = static_cast<const clap_plugin_posix_fd_support_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_POSIX_FD_SUPPORT));

        if (posixFdExt != nullptr && posixFdExt->on_fd == nullptr)
            posixFdExt = nullptr;

        fExtensions.posixFD = posixFdExt;
       #endif

        const uint32_t numAudioInputPorts = audioPortsExt != nullptr ? audioPortsExt->count(fPlugin, true) : 0;
        const uint32_t numAudioOutputPorts = audioPortsExt != nullptr ? audioPortsExt->count(fPlugin, false) : 0;
        const uint32_t numNoteInputPorts = notePortsExt != nullptr ? notePortsExt->count(fPlugin, true) : 0;
        const uint32_t numNoteOutputPorts = notePortsExt != nullptr ? notePortsExt->count(fPlugin, false) : 0;
        const uint32_t numParameters = paramsExt != nullptr ? paramsExt->count(fPlugin) : 0;

        uint32_t aIns, aOuts, mIns, mOuts, params;
        aIns = aOuts = mIns = mOuts = params = 0;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        fInputAudioBuffers.realloc(numAudioInputPorts);
        fOutputAudioBuffers.realloc(numAudioOutputPorts);

        for (uint32_t i=0; i<numAudioInputPorts; ++i)
        {
            clap_audio_port_info_t portInfo = {};
            CARLA_SAFE_ASSERT_BREAK(audioPortsExt->get(fPlugin, i, true, &portInfo));
            CARLA_SAFE_ASSERT(portInfo.channel_count != 0);

            fInputAudioBuffers.buffers[i].channel_count = portInfo.channel_count;
            fInputAudioBuffers.extra[i].offset = aIns;
            fInputAudioBuffers.extra[i].isMain = portInfo.flags & CLAP_AUDIO_PORT_IS_MAIN;

            aIns += portInfo.channel_count;
        }

        for (uint32_t i=0; i<numAudioOutputPorts; ++i)
        {
            clap_audio_port_info_t portInfo = {};
            CARLA_SAFE_ASSERT_BREAK(audioPortsExt->get(fPlugin, i, false, &portInfo));
            CARLA_SAFE_ASSERT(portInfo.channel_count != 0);

            fOutputAudioBuffers.buffers[i].channel_count = portInfo.channel_count;
            fOutputAudioBuffers.extra[i].offset = aOuts;
            fOutputAudioBuffers.extra[i].isMain = portInfo.flags & CLAP_AUDIO_PORT_IS_MAIN;
            for (uint32_t j=0; j<portInfo.channel_count; ++j)
                fOutputAudioBuffers.buffers[i].constant_mask |= (1ULL << j);

            aOuts += portInfo.channel_count;
        }

        for (uint32_t i=0; i<numNoteInputPorts; ++i)
        {
            clap_note_port_info_t portInfo = {};
            CARLA_SAFE_ASSERT_BREAK(notePortsExt->get(fPlugin, i, true, &portInfo));

            if (portInfo.supported_dialects & (CLAP_NOTE_DIALECT_CLAP|CLAP_NOTE_DIALECT_MIDI))
                ++mIns;
        }

        for (uint32_t i=0; i<numNoteOutputPorts; ++i)
        {
            clap_note_port_info_t portInfo = {};
            CARLA_SAFE_ASSERT_BREAK(notePortsExt->get(fPlugin, i, false, &portInfo));

            if (portInfo.supported_dialects & CLAP_NOTE_DIALECT_MIDI)
                ++mOuts;
        }

        for (uint32_t i=0; i<numParameters; ++i, ++params)
        {
            clap_param_info_t paramInfo = {};
            CARLA_SAFE_ASSERT_BREAK(paramsExt->get_info(fPlugin, i, &paramInfo));
        }

        if (aIns > 0)
        {
            pData->audioIn.createNew(aIns);
        }

        if (aOuts > 0)
        {
            pData->audioOut.createNew(aOuts);
            needsCtrlIn = true;

           #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            fAudioOutBuffers = new float*[aOuts];
            for (uint32_t i=0; i < aOuts; ++i)
                fAudioOutBuffers[i] = nullptr;
           #endif
        }

        if (mIns == 1)
            needsCtrlIn = true;

        if (mOuts == 1)
            needsCtrlOut = true;

        if (params > 0)
        {
            pData->param.createNew(params, false);
            needsCtrlIn = true;
        }

        fInputEvents.realloc(pData->event.portIn, mIns, params);
        fOutputEvents.realloc(pData->event.portOut, mOuts, params);

        const EngineProcessMode processMode = pData->engine->getProccessMode();
        const uint portNameSize = pData->engine->getMaxPortNameSize();
        String portName;

        // Audio Ins
        for (uint32_t j=0; j < aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (aIns > 1)
            {
                portName += "input_";
                portName += String(j+1);
            }
            else
                portName += "input";

            portName.truncate(portNameSize);

            pData->audioIn.ports[j].port = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio,
                                                                                         portName, true, j);
            pData->audioIn.ports[j].rindex = j;
        }

        // Audio Outs
        for (uint32_t j=0; j < aOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (aOuts > 1)
            {
                portName += "output_";
                portName += String(j+1);
            }
            else
                portName += "output";

            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio,
                                                                                          portName, false, j);
            pData->audioOut.ports[j].rindex = j;
        }

        // MIDI Ins
        for (uint32_t i=0, j=0; i<numNoteInputPorts; ++i)
        {
            clap_note_port_info_t portInfo = {};
            CARLA_SAFE_ASSERT_BREAK(notePortsExt->get(fPlugin, i, true, &portInfo));

            if ((portInfo.supported_dialects & (CLAP_NOTE_DIALECT_CLAP|CLAP_NOTE_DIALECT_MIDI)) == 0x0)
                continue;

            CARLA_SAFE_ASSERT_UINT2_BREAK(j < mIns, j, mIns);

            fInputEvents.portData[j].clapPortIndex = i;
            fInputEvents.portData[j].supportedDialects = portInfo.supported_dialects;

            if (mIns > 1)
            {
                portName.clear();

                if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = pData->name;
                    portName += ":";
                }

                portName += portInfo.name;
                portName.truncate(portNameSize);
                fInputEvents.portData[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent,
                                                                                              portName, true, j);
            }
            else
            {
                fInputEvents.portData[j].port = nullptr;
                fInputEvents.defaultPort = &fInputEvents.portData[0];
            }

            ++j;
        }

        // MIDI Outs
        for (uint32_t i=0, j=0; i<numNoteOutputPorts; ++i)
        {
            clap_note_port_info_t portInfo = {};
            CARLA_SAFE_ASSERT_BREAK(notePortsExt->get(fPlugin, i, false, &portInfo));

            if ((portInfo.supported_dialects & CLAP_NOTE_DIALECT_MIDI) == 0x0)
                continue;

            CARLA_SAFE_ASSERT_UINT2_BREAK(j < mOuts, j, mOuts);

            fOutputEvents.portData[j].clapPortIndex = i;
            fOutputEvents.portData[j].supportedDialects = portInfo.supported_dialects;

            if (mOuts > 1)
            {
                portName.clear();

                if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = pData->name;
                    portName += ":";
                }

                portName += portInfo.name;
                portName.truncate(portNameSize);
                fOutputEvents.portData[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent,
                                                                                               portName, false, j);
            }
            else
            {
                fOutputEvents.portData[j].port = nullptr;
                fOutputEvents.defaultPort = &fOutputEvents.portData[0];
            }

            ++j;
        }

        // Parameters
        for (uint32_t i=0; i<numParameters; ++i)
        {
            clap_param_info_t paramInfo = {};
            CARLA_SAFE_ASSERT_BREAK(paramsExt->get_info(fPlugin, i, &paramInfo));
            CARLA_SAFE_ASSERT_BREAK(i < params);

            pData->param.data[i].index  = i;
            pData->param.data[i].rindex = static_cast<int32_t>(paramInfo.id);

            double min, max, def, step, stepSmall, stepLarge;

            min = paramInfo.min_value;
            max = paramInfo.max_value;
            def = paramInfo.default_value;

            if (min >= max)
                max = min + 0.1;

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            if (paramInfo.flags & (CLAP_PARAM_IS_HIDDEN|CLAP_PARAM_IS_BYPASS))
            {
                pData->param.data[i].type = PARAMETER_UNKNOWN;
            }
            else if (paramInfo.flags & CLAP_PARAM_IS_READONLY)
            {
                pData->param.data[i].type = PARAMETER_OUTPUT;
                needsCtrlOut = true;
            }
            else
            {
                pData->param.data[i].type = PARAMETER_INPUT;
            }

            if (paramInfo.flags & CLAP_PARAM_IS_STEPPED)
            {
                if (carla_isEqual(max - min, 1.0))
                {
                    step = stepSmall = stepLarge = 1.0;
                    pData->param.data[i].hints |= PARAMETER_IS_BOOLEAN;
                }
                else
                {
                    step = 1.0;
                    stepSmall = 1.0;
                    stepLarge = std::min(max - min, 10.0);
                }
                pData->param.data[i].hints |= PARAMETER_IS_INTEGER;
            }
            else
            {
                double range = max - min;
                step = range/100.0;
                stepSmall = range/1000.0;
                stepLarge = range/10.0;
            }

            if (pData->param.data[i].type != PARAMETER_UNKNOWN)
            {
                pData->param.data[i].hints |= PARAMETER_IS_ENABLED;
                pData->param.data[i].hints |= PARAMETER_USES_CUSTOM_TEXT;

                if (paramInfo.flags & CLAP_PARAM_IS_AUTOMATABLE)
                {
                    pData->param.data[i].hints |= PARAMETER_IS_AUTOMATABLE;

                    if ((paramInfo.flags & CLAP_PARAM_IS_STEPPED) == 0x0)
                        pData->param.data[i].hints |= PARAMETER_CAN_BE_CV_CONTROLLED;
                }
            }

            pData->param.ranges[i].min = min;
            pData->param.ranges[i].max = max;
            pData->param.ranges[i].def = def;
            pData->param.ranges[i].step = step;
            pData->param.ranges[i].stepSmall = stepSmall;
            pData->param.ranges[i].stepLarge = stepLarge;

            fInputEvents.updatedParams[i].clapId = paramInfo.id;
            fInputEvents.updatedParams[i].cookie = paramInfo.cookie;
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-in";
            portName.truncate(portNameSize);

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent,
                                                                                portName, true, 0);
           #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            pData->event.cvSourcePorts = pData->client->createCVSourcePorts();
           #endif

            if (mIns == 1)
                fInputEvents.portData[0].port = pData->event.portIn;
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-out";
            portName.truncate(portNameSize);

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent,
                                                                                 portName, false, 0);

            if (mOuts == 1)
                fOutputEvents.portData[0].port = pData->event.portOut;
        }

        // plugin hints
        pData->hints = PLUGIN_NEEDS_MAIN_THREAD_IDLE;

        if (clapFeaturesContainInstrument(fPluginDescriptor->features))
            pData->hints |= PLUGIN_IS_SYNTH;

       #ifdef CLAP_WINDOW_API_NATIVE
        if (guiExt != nullptr)
        {
            if (guiExt->is_api_supported(fPlugin, CLAP_WINDOW_API_NATIVE, false))
            {
                pData->hints |= PLUGIN_HAS_CUSTOM_UI;
                pData->hints |= PLUGIN_HAS_CUSTOM_EMBED_UI;
                pData->hints |= PLUGIN_NEEDS_UI_MAIN_THREAD;
                if (guiExt->can_resize(fPlugin))
                    pData->hints |= PLUGIN_HAS_CUSTOM_RESIZABLE_UI;
            }
            else if (guiExt->is_api_supported(fPlugin, CLAP_WINDOW_API_NATIVE, true))
            {
                pData->hints |= PLUGIN_HAS_CUSTOM_UI;
                pData->hints |= PLUGIN_NEEDS_UI_MAIN_THREAD;
            }
        }
       #endif

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints = 0x0;

        if (const uint32_t latency = fExtensions.latency != nullptr ? fExtensions.latency->get(fPlugin) : 0)
        {
            fLastKnownLatency = latency;
            pData->client->setLatency(latency);
           #ifndef BUILD_BRIDGE
            pData->latency.recreateBuffers(std::max(aIns, aOuts), latency);
           #endif
        }
        else
        {
            fLastKnownLatency = 0;
        }

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();
        else
            runIdleCallbacksAsNeeded(false);

        carla_debug("CarlaPluginCLAP::reload() - end");
    }

    void reloadPrograms(const bool doInit) override
    {
        carla_debug("CarlaPluginCLAP::reloadPrograms(%s)", bool2str(doInit));
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        // FIXME check return status
        fPlugin->activate(fPlugin, pData->engine->getSampleRate(), 1, pData->engine->getBufferSize());
        fPlugin->start_processing(fPlugin);

        fNeedsParamFlush = false;
        runIdleCallbacksAsNeeded(false);
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        // FIXME check return status
        fPlugin->stop_processing(fPlugin);
        fPlugin->deactivate(fPlugin);

        runIdleCallbacksAsNeeded(false);
    }

    void process(const float* const* const audioIn,
                 float** const audioOut,
                 const float* const* const cvIn,
                 float** const,
                 const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check buffers

        CARLA_SAFE_ASSERT_RETURN(frames > 0,);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioIn != nullptr,);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioOut != nullptr,);
           #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            CARLA_SAFE_ASSERT_RETURN(fAudioOutBuffers != nullptr,);
           #endif
        }

        // --------------------------------------------------------------------------------------------------------
        // Set audio buffers

       #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        for (uint32_t i=0; i < pData->audioOut.count; ++i)
            carla_zeroFloats(fAudioOutBuffers[i], frames);
       #endif

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            return;
        }

        // --------------------------------------------------------------------------------------------------------

        fInputEvents.handleScheduledParameterUpdates();

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            // TODO alternative if plugin does not support CLAP_NOTE_DIALECT_MIDI

            if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                for (uint32_t p=0; p<fInputEvents.portCount; ++p)
                {
                    const uint16_t port = fInputEvents.portData[p].clapPortIndex;

                    for (uint8_t i=0, k=fInputEvents.numEventsUsed; i < MAX_MIDI_CHANNELS; ++i)
                    {
                        fInputEvents.events[k + i].midi = {
                            { sizeof(clap_event_midi_t), 0, 0, CLAP_EVENT_MIDI, 0 },
                            port,
                            {
                                uint8_t(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT)),
                                MIDI_CONTROL_ALL_NOTES_OFF, 0
                            }
                        };
                        fInputEvents.events[k + MAX_MIDI_CHANNELS + i].midi = {
                            { sizeof(clap_event_midi_t), 0, 0, CLAP_EVENT_MIDI, 0 },
                            port,
                            {
                                uint8_t(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT)),
                                MIDI_CONTROL_ALL_SOUND_OFF, 0
                            }
                        };
                    }

                    fInputEvents.numEventsUsed += MAX_MIDI_CHANNELS * 2;
                }
            }
            else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                for (uint32_t p=0; p<fInputEvents.portCount; ++p)
                {
                    const uint16_t port = fInputEvents.portData[p].clapPortIndex;

                    for (uint8_t i=0, k=fInputEvents.numEventsUsed; i < MAX_MIDI_NOTE; ++i)
                    {
                        fInputEvents.events[k + i].midi = {
                            { sizeof(clap_event_midi_t), 0, 0, CLAP_EVENT_MIDI, 0 },
                            port,
                            {
                                uint8_t(MIDI_STATUS_NOTE_OFF | (pData->ctrlChannel & MIDI_CHANNEL_BIT)),
                                i, 0
                            }
                        };
                    }

                    fInputEvents.numEventsUsed += MAX_MIDI_NOTE;
                }
            }

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set TimeInfo

        const EngineTimeInfo timeInfo(pData->engine->getTimeInfo());

        const double sampleRate = pData->engine->getSampleRate();

        clap_event_transport_t clapTransport = {
            { sizeof(clap_event_transport_t), 0, 0, CLAP_EVENT_TRANSPORT, 0 },
            0x0, // flags
            0, // song_pos_beats, position in beats
            0, // song_pos_seconds, position in seconds
            0.0, // tempo, in bpm
            0.0, // tempo_inc, tempo increment for each samples and until the next time info event
            0, // loop_start_beats
            0, // loop_end_beats
            0, // loop_start_seconds
            0, // loop_end_seconds
            0, // bar_start, start pos of the current bar
            0, // bar_number, bar at song pos 0 has the number 0
            0, // tsig_num, time signature numerator
            0, // tsig_denom, time signature denominator
        };

        if (timeInfo.playing)
            clapTransport.flags |= CLAP_TRANSPORT_IS_PLAYING;

        const double positionSeconds = static_cast<double>(timeInfo.frame) / sampleRate;
        clapTransport.song_pos_seconds = std::round(CLAP_SECTIME_FACTOR * positionSeconds);
        clapTransport.flags |= CLAP_TRANSPORT_HAS_SECONDS_TIMELINE;

        if (timeInfo.bbt.valid)
        {
            CARLA_SAFE_ASSERT_INT(timeInfo.bbt.bar > 0, timeInfo.bbt.bar);
            CARLA_SAFE_ASSERT_INT(timeInfo.bbt.beat > 0, timeInfo.bbt.beat);

            const double barStart = static_cast<double>(timeInfo.bbt.beatsPerBar) * (timeInfo.bbt.bar - 1);
            const double positionBeats = static_cast<double>(timeInfo.frame)
                                       / (sampleRate * 60 / timeInfo.bbt.beatsPerMinute);

            // Bar/Beats
            clapTransport.bar_start = std::round(CLAP_BEATTIME_FACTOR * barStart);
            clapTransport.bar_number = timeInfo.bbt.bar - 1;
            clapTransport.song_pos_beats = std::round(CLAP_BEATTIME_FACTOR * positionBeats);
            clapTransport.flags |= CLAP_TRANSPORT_HAS_BEATS_TIMELINE;

            // Tempo
            clapTransport.tempo  = timeInfo.bbt.beatsPerMinute;
            clapTransport.flags |= CLAP_TRANSPORT_HAS_TEMPO;

            // Time Signature
            clapTransport.tsig_num = static_cast<uint16_t>(timeInfo.bbt.beatsPerBar + 0.5f);
            clapTransport.tsig_denom = static_cast<uint16_t>(timeInfo.bbt.beatType + 0.5f);
            clapTransport.flags |= CLAP_TRANSPORT_HAS_TIME_SIGNATURE;
        }
        else
        {
            // Tempo
            clapTransport.tempo = 120.0;
            clapTransport.flags |= CLAP_TRANSPORT_HAS_TEMPO;

            // Time Signature
            clapTransport.tsig_num = 4;
            clapTransport.tsig_denom = 4;
            clapTransport.flags |= CLAP_TRANSPORT_HAS_TIME_SIGNATURE;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input (main port)

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                if (fInputEvents.portCount == 0)
                {
                    // does not handle MIDI
                    pData->extNotes.data.clear();
                }
                else
                {
                    ExternalMidiNote note = { -1, 0, 0 };
                    const uint16_t p = fInputEvents.portData[0].clapPortIndex;

                    for (; fInputEvents.numEventsUsed < fInputEvents.numEventsAllocated && ! pData->extNotes.data.isEmpty();)
                    {
                        note = pData->extNotes.data.getFirst(note, true);

                        CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                        if (fInputEvents.portData[0].supportedDialects & CLAP_NOTE_DIALECT_MIDI)
                        {
                            const uint8_t data[3] = {
                                uint8_t((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT)),
                                note.note,
                                note.velo
                            };
                            fInputEvents.addSimpleMidiEvent(true, p, 0, data);
                        }
                        else
                        {
                            fInputEvents.addSimpleNoteEvent(true, -1, 0, note.channel, note.note, note.velo);
                        }
                    }
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            bool allNotesOffSent  = false;
#endif
            uint32_t previousEventTime = 0;
            uint32_t nextBankId;

            if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
            else
                nextBankId = 0;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            if (cvIn != nullptr && pData->event.cvSourcePorts != nullptr)
                pData->event.cvSourcePorts->initPortBuffers(cvIn + pData->cvIn.count, frames, true, pData->event.portIn);
#endif

            const uint32_t numSysEvents = pData->event.portIn->getEventCount();

            for (uint32_t i=0; i < numSysEvents; ++i)
            {
                EngineEvent& event(pData->event.portIn->getEvent(i));

                uint32_t eventTime = event.time;
                CARLA_SAFE_ASSERT_UINT2_CONTINUE(eventTime < frames, eventTime, frames);

                if (eventTime < previousEventTime)
                {
                    carla_stderr2("Timing error, eventTime:%u < previousEventTime:%u for '%s'",
                                  eventTime, previousEventTime, pData->name);
                    eventTime = previousEventTime;
                }

                previousEventTime = eventTime;

                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl: {
                    EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter: {
                        float value;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        // non-midi
                        if (event.channel == kEngineEventNonMidiChannel)
                        {
                            const uint32_t k = ctrlEvent.param;
                            CARLA_SAFE_ASSERT_CONTINUE(k < pData->param.count);

                            ctrlEvent.handled = true;
                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                            continue;
                        }

                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                ctrlEvent.handled = true;
                                value = ctrlEvent.normalizedValue;
                                setDryWetRT(value, true);
                            }
                            else if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                ctrlEvent.handled = true;
                                value = ctrlEvent.normalizedValue*127.0f/100.0f;
                                setVolumeRT(value, true);
                            }
                            else if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
                            {
                                float left, right;
                                value = ctrlEvent.normalizedValue/0.5f - 1.0f;

                                if (value < 0.0f)
                                {
                                    left  = -1.0f;
                                    right = (value*2.0f)+1.0f;
                                }
                                else if (value > 0.0f)
                                {
                                    left  = (value*2.0f)-1.0f;
                                    right = 1.0f;
                                }
                                else
                                {
                                    left  = -1.0f;
                                    right = 1.0f;
                                }

                                ctrlEvent.handled = true;
                                setBalanceLeftRT(left, true);
                                setBalanceRightRT(right, true);
                            }
                        }
#endif
                        // Control plugin parameters
                        uint32_t k;
                        for (k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].mappedControlIndex != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMATABLE) == 0)
                                continue;

                            ctrlEvent.handled = true;
                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                        }

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_VALUE)
                        {
                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = uint8_t(ctrlEvent.param);
                            midiData[2] = uint8_t(ctrlEvent.normalizedValue*127.0f + 0.5f);

                            fInputEvents.addSimpleMidiEvent(true, 0, eventTime, midiData);
                        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        if (! ctrlEvent.handled)
                            checkForMidiLearn(event);
#endif
                        break;
                    } // case kEngineControlEventTypeParameter

                    case kEngineControlEventTypeMidiBank:
                        if (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
                        {
                            if (event.channel == pData->ctrlChannel)
                                nextBankId = ctrlEvent.param;
                        }
                        else if (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)
                        {
                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = MIDI_CONTROL_BANK_SELECT;
                            midiData[2] = uint8_t(ctrlEvent.param);

                            fInputEvents.addSimpleMidiEvent(true, 0, eventTime, midiData);
                        }
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
                        {
                            if (event.channel == pData->ctrlChannel)
                            {
                                const uint32_t nextProgramId(ctrlEvent.param);

                                for (uint32_t k=0; k < pData->midiprog.count; ++k)
                                {
                                    if (pData->midiprog.data[k].bank == nextBankId && pData->midiprog.data[k].program == nextProgramId)
                                    {
                                        setMidiProgramRT(k, true);
                                        break;
                                    }
                                }
                            }
                        }
                        else if (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)
                        {
                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_PROGRAM_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = uint8_t(ctrlEvent.param);
                            midiData[2] = 0;

                            fInputEvents.addSimpleMidiEvent(true, 0, eventTime, midiData);
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                            midiData[2] = 0;

                            fInputEvents.addSimpleMidiEvent(true, 0, eventTime, midiData);
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                postponeRtAllNotesOff();
                            }
#endif

                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                            midiData[2] = 0;

                            fInputEvents.addSimpleMidiEvent(true, 0, eventTime, midiData);
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size > 3)
                        continue;

                    CARLA_SAFE_ASSERT_BREAK(midiEvent.port < fInputEvents.portCount);

                    const uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiEvent.data));

                    if (status == MIDI_STATUS_NOTE_OFF || status == MIDI_STATUS_NOTE_ON)
                    {
                        if (pData->options & PLUGIN_OPTION_SKIP_SENDING_NOTES)
                            continue;

                        // plugin does not support MIDI, send a simple note instead
                        if ((fInputEvents.portData[midiEvent.port].supportedDialects & CLAP_NOTE_DIALECT_MIDI) == 0x0)
                        {
                            fInputEvents.addSimpleNoteEvent(true, midiEvent.port, event.time,
                                                            event.channel, midiEvent.data[1], midiEvent.data[2]);
                        }
                    }

                    if (fInputEvents.portData[midiEvent.port].supportedDialects & CLAP_NOTE_DIALECT_MIDI)
                    {
                        if (status == MIDI_STATUS_CHANNEL_PRESSURE && (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                            continue;
                        if (status == MIDI_STATUS_CONTROL_CHANGE && (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                            continue;
                        if (status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH && (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                            continue;
                        if (status == MIDI_STATUS_PITCH_WHEEL_CONTROL && (pData->options & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                            continue;

                        // put back channel in data
                        uint8_t midiData[3] = { uint8_t(status | (event.channel & MIDI_CHANNEL_BIT)), 0, 0 };

                        switch (midiEvent.size)
                        {
                        case 3:
                            midiData[2] = midiEvent.data[2];
                            // fall through
                        case 2:
                            midiData[1] = midiEvent.data[1];
                            break;
                        }

                        fInputEvents.addSimpleMidiEvent(true, midiEvent.port, eventTime, midiData);
                    }

                    switch (status)
                    {
                    case MIDI_STATUS_NOTE_ON:
                        if (midiEvent.data[2] != 0)
                        {
                            pData->postponeNoteOnRtEvent(true, event.channel, midiEvent.data[1], midiEvent.data[2]);
                            break;
                        }
                        // fall through
                    case MIDI_STATUS_NOTE_OFF:
                        pData->postponeNoteOffRtEvent(true, event.channel, midiEvent.data[1]);
                        break;
                    }
                } break;
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

        } // End of Event Input (main port)

        // --------------------------------------------------------------------------------------------------------
        // Event input (multi MIDI port)

        if (fInputEvents.portCount > 1)
        {
            // TODO
        }

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        for (uint32_t i=0; i<fInputAudioBuffers.count; ++i)
            fInputAudioBuffers.buffers[i].data32 = audioIn + fInputAudioBuffers.extra[i].offset;

        for (uint32_t i=0; i<fOutputAudioBuffers.count; ++i)
        {
           #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            fOutputAudioBuffers.buffers[i].data32 = fAudioOutBuffers + fOutputAudioBuffers.extra[i].offset;
           #else
            fOutputAudioBuffers.buffers[i].data32 = audioOut + fOutputAudioBuffers.extra[i].offset;
           #endif
        }

        const clap_process_t process = {
            static_cast<int64_t>(timeInfo.frame),
            frames,
            &clapTransport,
            fInputAudioBuffers.cast(),
            fOutputAudioBuffers.buffers,
            fInputAudioBuffers.count,
            fOutputAudioBuffers.count,
            fInputEvents.cast(),
            fOutputEvents.cast()
        };

        fPlugin->process(fPlugin, &process);

       #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && carla_isNotEqual(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));
            const bool isMono    = (pData->audioIn.count == 1);

            bool isPair;
            float bufValue;
            float* const oldBufLeft = pData->postProc.extraBuffer;

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    const uint32_t c = isMono ? 0 : i;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        bufValue = audioIn[c][k];
                        fAudioOutBuffers[i][k] = (fAudioOutBuffers[i][k] * pData->postProc.dryWet) + (bufValue * (1.0f - pData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < pData->audioOut.count);
                        carla_copyFloats(oldBufLeft, fAudioOutBuffers[i], frames);
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (isPair)
                        {
                            // left
                            fAudioOutBuffers[i][k]  = oldBufLeft[k]            * (1.0f - balRangeL);
                            fAudioOutBuffers[i][k] += fAudioOutBuffers[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            fAudioOutBuffers[i][k]  = fAudioOutBuffers[i][k] * balRangeR;
                            fAudioOutBuffers[i][k] += oldBufLeft[k]          * balRangeL;
                        }
                    }
                }

                // Volume (and buffer copy)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        audioOut[i][k] = fAudioOutBuffers[i][k] * pData->postProc.volume;
                }
            }

        } // End of Post-processing
       #endif // BUILD_BRIDGE_ALTERNATIVE_ARCH

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();

       #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (pData->event.portOut != nullptr && fExtensions.params != nullptr)
        {
            uint8_t  channel;
            uint16_t param;
            double   value;

            for (uint32_t k=0; k < pData->param.count; ++k)
            {
                if (pData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;
                if (pData->param.data[k].mappedControlIndex <= 0)
                    continue;
                if (!fExtensions.params->get_value(fPlugin, pData->param.data[k].rindex, &value))
                    continue;

                channel = pData->param.data[k].midiChannel;
                param   = static_cast<uint16_t>(pData->param.data[k].mappedControlIndex);
                value   = pData->param.ranges[k].getNormalizedValue(value);

                pData->event.portOut->writeControlEvent(0, channel,
                                                        kEngineControlEventTypeParameter, param, -1,
                                                        value);
            }
        } // End of Control Output
       #endif

        // --------------------------------------------------------------------------------------------------------
        // Events/MIDI Output

        for (uint32_t i=0; i<fOutputEvents.numEventsUsed; ++i)
        {
            const carla_clap_output_events::Event& ev(fOutputEvents.events[i]);

            switch (ev.header.type)
            {
            case CLAP_EVENT_PARAM_VALUE:
                for (uint32_t j=0; j<pData->param.count; ++j)
                {
                    if (pData->param.data[j].rindex != static_cast<int32_t>(ev.param.param_id))
                        continue;
                    pData->postponeParameterChangeRtEvent(true, static_cast<int32_t>(j), ev.param.value);
                    break;
                }
                break;
            case CLAP_EVENT_MIDI:
                for (uint32_t j=0; j<fOutputEvents.portCount; ++j)
                {
                    if (fOutputEvents.portData[j].clapPortIndex != ev.midi.port_index)
                        continue;
                    fOutputEvents.portData[j].port->writeMidiEvent(ev.midi.header.time, 3, ev.midi.data);
                    break;
                }
                break;
            }
        }

        fOutputEvents.numEventsUsed = 0;

        // --------------------------------------------------------------------------------------------------------

#ifdef BUILD_BRIDGE_ALTERNATIVE_ARCH
        return;

        // unused
        (void)cvIn;
#endif
    }

   #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("CarlaPluginCLAP::bufferSizeChanged(%i)", newBufferSize);

        if (pData->active)
            deactivate();

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];
            fAudioOutBuffers[i] = new float[newBufferSize];
        }

        if (pData->active)
            activate();

        CarlaPlugin::bufferSizeChanged(newBufferSize);
    }
   #endif

    // -------------------------------------------------------------------
    // Plugin buffers

    void initBuffers() const noexcept override
    {
        fInputEvents.initBuffers();
        fOutputEvents.initBuffers();

        CarlaPlugin::initBuffers();
    }

    void clearBuffers() noexcept override
    {
        carla_debug("CarlaPluginCLAP::clearBuffers() - start");

       #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (fAudioOutBuffers != nullptr)
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                if (fAudioOutBuffers[i] != nullptr)
                {
                    delete[] fAudioOutBuffers[i];
                    fAudioOutBuffers[i] = nullptr;
                }
            }

            delete[] fAudioOutBuffers;
            fAudioOutBuffers = nullptr;
        }
       #endif

        fInputEvents.clear(pData->event.portIn);
        fOutputEvents.clear(pData->event.portOut);

        CarlaPlugin::clearBuffers();

        carla_debug("CarlaPluginCLAP::clearBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    // nothing

    // -------------------------------------------------------------------

protected:
   #ifdef CLAP_WINDOW_API_NATIVE
    void handlePluginUIClosed() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_stdout("CarlaPluginCLAP::handlePluginUIClosed()");

        fUI.shouldClose = true;
    }

    void handlePluginUIResized(const uint width, const uint height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_stdout("CarlaPluginCLAP::handlePluginUIResized(%u, %u | vs %u %u) %d %s %s",
                     width, height,
                     fUI.width, fUI.height,
                     fUI.isResizingFromPlugin, bool2str(fUI.isResizingFromInit), bool2str(fUI.isResizingFromHost));

        if (fExtensions.gui == nullptr)
            return;

        if (fUI.isResizingFromPlugin != 0)
        {
            CARLA_SAFE_ASSERT_UINT2_RETURN(fUI.width == width, fUI.width, width,);
            CARLA_SAFE_ASSERT_UINT2_RETURN(fUI.height == height, fUI.height, height,);
            fUI.isResizingFromPlugin = 2;
            return;
        }

        if (fUI.isResizingFromInit)
        {
            CARLA_SAFE_ASSERT_UINT2_RETURN(fUI.width == width, fUI.width, width,);
            CARLA_SAFE_ASSERT_UINT2_RETURN(fUI.height == height, fUI.height, height,);
            fUI.isResizingFromInit = false;
            return;
        }

        if (fUI.isResizingFromHost)
        {
            CARLA_SAFE_ASSERT_UINT2_RETURN(fUI.width == width, fUI.width, width,);
            CARLA_SAFE_ASSERT_UINT2_RETURN(fUI.height == height, fUI.height, height,);
            fUI.isResizingFromHost = false;
            return;
        }

        if (fUI.width != width || fUI.height != height)
        {
            uint width2 = width;
            uint height2 = height;

            if (fExtensions.gui->adjust_size(fPlugin, &width2, &height2))
            {
                if (width2 != width || height2 != height)
                {
                    fUI.isResizingFromHost = true;
                    fUI.width = width2;
                    fUI.height = height2;
                    fUI.window->setSize(width2, height2, false, false);
                }
                else
                {
                    fExtensions.gui->set_size(fPlugin, width2, height2);
                }
            }
        }
    }
   #endif

    // -------------------------------------------------------------------

    void clapRequestRestart() override
    {
        carla_stdout("CarlaPluginCLAP::clapRequestRestart()");

        fNeedsRestart = true;
    }

    void clapRequestProcess() override
    {
        carla_stdout("CarlaPluginCLAP::clapRequestProcess()");

        fNeedsProcess = true;
    }

    void clapRequestCallback() override
    {
        carla_stdout("CarlaPluginCLAP::clapRequestCallback()");

        if (fPlugin->on_main_thread != nullptr)
            fNeedsIdleCallback = true;
    }

    // -------------------------------------------------------------------

    void clapLatencyChanged() override
    {
        carla_stdout("CarlaPluginCLAP::clapLatencyChanged()");
        CARLA_SAFE_ASSERT_RETURN(fExtensions.latency != nullptr,);

        fLastKnownLatency = fExtensions.latency->get(fPlugin);
    }

    // -------------------------------------------------------------------

    void clapMarkDirty() override
    {
        carla_stdout("CarlaPluginCLAP::clapMarkDirty()");
    }

    // -------------------------------------------------------------------

  #ifdef CLAP_WINDOW_API_NATIVE
    void clapGuiResizeHintsChanged() override
    {
        carla_stdout("CarlaPluginCLAP::clapGuiResizeHintsChanged()");
    }

    bool clapGuiRequestResize(const uint width, const uint height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr, false);
        carla_stdout("CarlaPluginCLAP::hostRequestResize(%u, %u)", width, height);

        fUI.isResizingFromPlugin = 3;
        fUI.width = width;
        fUI.height = height;
        fUI.window->setSize(width, height, true, false);
        return true;
    }

    bool clapGuiRequestShow() override
    {
        carla_stdout("CarlaPluginCLAP::clapGuiRequestShow()");
        return false;
    }

    bool clapGuiRequestHide() override
    {
        carla_stdout("CarlaPluginCLAP::clapGuiRequestHide()");
        return false;
    }

    void clapGuiClosed(const bool wasDestroyed) override
    {
        carla_stdout("CarlaPluginCLAP::clapGuiClosed(%s)", bool2str(wasDestroyed));
        CARLA_SAFE_ASSERT_RETURN(!fUI.isEmbed,);
        CARLA_SAFE_ASSERT_RETURN(fUI.isVisible,);

        fUI.isVisible = false;

        if (wasDestroyed)
        {
            CARLA_SAFE_ASSERT_RETURN(fUI.isCreated,);
            fExtensions.gui->destroy(fPlugin);
            fUI.isCreated = false;
        }

        pData->engine->callback(true, true,
                                ENGINE_CALLBACK_UI_STATE_CHANGED,
                                pData->id,
                                0,
                                0, 0, 0.0f, nullptr);
    }

    // -------------------------------------------------------------------

   #ifdef _POSIX_VERSION
    bool clapRegisterPosixFD(const int fd, const clap_posix_fd_flags_t flags) override
    {
        carla_stdout("CarlaPluginCLAP::clapRegisterPosixFD(%i, %x)", fd, flags);

        // NOTE some plugins wont have their posix fd extension ready when first loaded, so try again here
        if (fExtensions.posixFD == nullptr)
        {
            const clap_plugin_posix_fd_support_t* const posixFdExt = static_cast<const clap_plugin_posix_fd_support_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_POSIX_FD_SUPPORT));

            if (posixFdExt != nullptr && posixFdExt->on_fd != nullptr)
                fExtensions.posixFD = posixFdExt;
        }

        CARLA_SAFE_ASSERT_RETURN(fExtensions.posixFD != nullptr, false);

        // FIXME events only driven as long as UI is open
        // CARLA_SAFE_ASSERT_RETURN(fUI.isCreated, false);

        if (flags & (CLAP_POSIX_FD_READ|CLAP_POSIX_FD_WRITE))
        {
           #ifdef CARLA_CLAP_POSIX_EPOLL
            const int hostFd = ::epoll_create1(0);
           #else
            const int hostFd = ::kqueue();
           #endif
            CARLA_SAFE_ASSERT_RETURN(hostFd >= 0, false);

           #ifdef CARLA_CLAP_POSIX_EPOLL
            struct ::epoll_event ev = {};
            if (flags & CLAP_POSIX_FD_READ)
                ev.events |= EPOLLIN;
            if (flags & CLAP_POSIX_FD_WRITE)
                ev.events |= EPOLLOUT;
            ev.data.fd = fd;

            if (::epoll_ctl(hostFd, EPOLL_CTL_ADD, fd, &ev) < 0)
            {
                ::close(hostFd);
                return false;
            }
           #endif

            const HostPosixFileDescriptorDetails posixFD = {
                hostFd,
                fd,
                flags,
            };
            fPosixFileDescriptors.append(posixFD);

            return true;
        }

        return false;
    }

    bool clapModifyPosixFD(const int fd, const clap_posix_fd_flags_t flags) override
    {
        carla_stdout("CarlaPluginCLAP::clapTimerUnregister(%i, %x)", fd, flags);
        for (LinkedList<HostPosixFileDescriptorDetails>::Itenerator it = fPosixFileDescriptors.begin2(); it.valid(); it.next())
        {
            HostPosixFileDescriptorDetails& posixFD(it.getValue(kPosixFileDescriptorFallbackNC));

            if (posixFD.pluginFd == fd)
            {
                if (posixFD.flags == flags)
                    return true;

               #ifdef CARLA_CLAP_POSIX_EPOLL
                struct ::epoll_event ev = {};
                if (flags & CLAP_POSIX_FD_READ)
                    ev.events |= EPOLLIN;
                if (flags & CLAP_POSIX_FD_WRITE)
                    ev.events |= EPOLLOUT;
                ev.data.fd = fd;

                if (::epoll_ctl(posixFD.hostFd, EPOLL_CTL_MOD, fd, &ev) < 0)
                    return false;
               #endif

                posixFD.flags = flags;
                return true;
            }
        }

        return false;
    }

    bool clapUnregisterPosixFD(const int fd) override
    {
        carla_stdout("CarlaPluginCLAP::clapTimerUnregister(%i)", fd);
        for (LinkedList<HostPosixFileDescriptorDetails>::Itenerator it = fPosixFileDescriptors.begin2(); it.valid(); it.next())
        {
            const HostPosixFileDescriptorDetails& posixFD(it.getValue(kPosixFileDescriptorFallback));

            if (posixFD.pluginFd == fd)
            {
               #ifdef CARLA_CLAP_POSIX_EPOLL
                ::epoll_ctl(posixFD.hostFd, EPOLL_CTL_DEL, fd, nullptr);
               #endif
                ::close(posixFD.hostFd);
                fPosixFileDescriptors.remove(it);
                return true;
            }
        }

        return false;
    }
   #endif // _POSIX_VERSION

    // -------------------------------------------------------------------

    bool clapRegisterTimer(const uint32_t periodInMs, clap_id* const timerId) override
    {
        carla_stdout("CarlaPluginCLAP::clapTimerRegister(%u, %p)", periodInMs, timerId);

        // NOTE some plugins wont have their timer extension ready when first loaded, so try again here
        if (fExtensions.timer == nullptr)
        {
            const clap_plugin_timer_support_t* const timerExt = static_cast<const clap_plugin_timer_support_t*>(
                fPlugin->get_extension(fPlugin, CLAP_EXT_TIMER_SUPPORT));

            if (timerExt != nullptr && timerExt->on_timer != nullptr)
                fExtensions.timer = timerExt;
        }

        CARLA_SAFE_ASSERT_RETURN(fExtensions.timer != nullptr, false);

        // FIXME events only driven as long as UI is open
        // CARLA_SAFE_ASSERT_RETURN(fUI.isCreated, false);

        const HostTimerDetails timer = {
            fTimers.isNotEmpty() ? fTimers.getLast(kTimerFallback).clapId + 1 : 1,
            periodInMs,
            0
        };
        fTimers.append(timer);

        *timerId = timer.clapId;
        return true;
    }

    bool clapUnregisterTimer(const clap_id timerId) override
    {
        carla_stdout("CarlaPluginCLAP::clapTimerUnregister(%u)", timerId);
        for (LinkedList<HostTimerDetails>::Itenerator it = fTimers.begin2(); it.valid(); it.next())
        {
            const HostTimerDetails& timer(it.getValue(kTimerFallback));

            if (timer.clapId == timerId)
            {
                fTimers.remove(it);
                return true;
            }
        }

        return false;
    }
  #endif // CLAP_WINDOW_API_NATIVE

    // -------------------------------------------------------------------

public:
    bool init(const CarlaPluginPtr plugin,
              const char* const filename, const char* const name, const char* const id, const uint options)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (filename == nullptr || filename[0] == '\0')
        {
            pData->engine->setLastError("null filename");
            return false;
        }

        // ---------------------------------------------------------------

        const clap_plugin_entry_t* entry;

       #ifdef CARLA_OS_MAC
        if (!water::File(filename).existsAsFile())
        {
            if (! fBundleLoader.load(filename))
            {
                pData->engine->setLastError("Failed to load CLAP bundle executable");
                return false;
            }

            entry = fBundleLoader.getSymbol<const clap_plugin_entry_t*>(CFSTR("clap_entry"));
        }
        else
       #endif
        {
            if (! pData->libOpen(filename))
            {
                pData->engine->setLastError(pData->libError(filename));
                return false;
            }

            entry = pData->libSymbol<const clap_plugin_entry_t*>("clap_entry");
        }

        if (entry == nullptr)
        {
            pData->engine->setLastError("Could not find the CLAP entry in the plugin library");
            return false;
        }

        if (entry->init == nullptr || entry->deinit == nullptr || entry->get_factory == nullptr)
        {
            pData->engine->setLastError("CLAP factory entries are null");
            return false;
        }

        if (!clap_version_is_compatible(entry->clap_version))
        {
            pData->engine->setLastError("Incompatible CLAP plugin");
            return false;
        }

        // ---------------------------------------------------------------

        const water::String pluginPath(water::File(filename).getParentDirectory().getFullPathName());

        if (!entry->init(pluginPath.toRawUTF8()))
        {
            pData->engine->setLastError("Plugin entry failed to initialize");
            return false;
        }

        fPluginEntry = entry;

        // ---------------------------------------------------------------

        const clap_plugin_factory_t* const factory = static_cast<const clap_plugin_factory_t*>(
            entry->get_factory(CLAP_PLUGIN_FACTORY_ID));

        if (factory == nullptr
            || factory->get_plugin_count == nullptr
            || factory->get_plugin_descriptor == nullptr
            || factory->create_plugin == nullptr)
        {
            pData->engine->setLastError("Plugin is missing factory methods");
            return false;
        }

        // ---------------------------------------------------------------

        if (const uint32_t count = factory->get_plugin_count(factory))
        {
            // null id requested, use first available plugin
            if (id == nullptr || id[0] == '\0')
            {
                fPluginDescriptor = factory->get_plugin_descriptor(factory, 0);

                if (fPluginDescriptor == nullptr)
                {
                    pData->engine->setLastError("Plugin library does not contain a valid first plugin");
                    return false;
                }
            }
            else
            {
                for (uint32_t i=0; i<count; ++i)
                {
                    const clap_plugin_descriptor_t* const desc = factory->get_plugin_descriptor(factory, i);
                    CARLA_SAFE_ASSERT_CONTINUE(desc != nullptr);
                    CARLA_SAFE_ASSERT_CONTINUE(desc->id != nullptr);

                    if (std::strcmp(desc->id, id) == 0)
                    {
                        fPluginDescriptor = desc;
                        break;
                    }
                }

                if (fPluginDescriptor == nullptr)
                {
                    pData->engine->setLastError("Plugin library does not contain the requested plugin");
                    return false;
                }
            }
        }
        else
        {
            pData->engine->setLastError("Plugin library contains no plugins");
            return false;
        }

        // ---------------------------------------------------------------

        fPlugin = factory->create_plugin(factory, &fHost, fPluginDescriptor->id);

        if (fPlugin == nullptr)
        {
            pData->engine->setLastError("Failed to create CLAP plugin instance");
            return false;
        }

        if (!fPlugin->init(fPlugin))
        {
            pData->engine->setLastError("Failed to initialize CLAP plugin instance");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        pData->name = pData->engine->getUniquePluginName(name != nullptr && name[0] != '\0' ? name
                                                                                            : fPluginDescriptor->name);
        pData->filename = carla_strdup(filename);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(plugin);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // set default options

        pData->options = PLUGIN_OPTION_FIXED_BUFFERS;

        if (const clap_plugin_state_t* const stateExt =
            static_cast<const clap_plugin_state_t*>(fPlugin->get_extension(fPlugin, CLAP_EXT_STATE)))
        {
            if (stateExt->save != nullptr && stateExt->load != nullptr)
                if (isPluginOptionEnabled(options, PLUGIN_OPTION_USE_CHUNKS))
                    pData->options |= PLUGIN_OPTION_USE_CHUNKS;
        }

        if (const clap_plugin_note_ports_t* const notePortsExt =
            static_cast<const clap_plugin_note_ports_t*>(fPlugin->get_extension(fPlugin, CLAP_EXT_NOTE_PORTS)))
        {
            const uint32_t numNoteInputPorts = notePortsExt->count != nullptr && notePortsExt->get != nullptr
                                             ? notePortsExt->count(fPlugin, true) : 0;

            for (uint32_t i=0; i<numNoteInputPorts; ++i)
            {
                clap_note_port_info_t portInfo = {};
                CARLA_SAFE_ASSERT_BREAK(notePortsExt->get(fPlugin, i, true, &portInfo));

                if (portInfo.supported_dialects & CLAP_NOTE_DIALECT_MIDI)
                {
                    if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CONTROL_CHANGES))
                        pData->options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
                    if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CHANNEL_PRESSURE))
                        pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                    if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH))
                        pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                    if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PITCHBEND))
                        pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
                    if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_ALL_SOUND_OFF))
                        pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
                    if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PROGRAM_CHANGES))
                        pData->options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;
                    if (isPluginOptionInverseEnabled(options, PLUGIN_OPTION_SKIP_SENDING_NOTES))
                        pData->options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
                    break;
                }
                if (portInfo.supported_dialects & CLAP_NOTE_DIALECT_CLAP)
                {
                    if (isPluginOptionInverseEnabled(options, PLUGIN_OPTION_SKIP_SENDING_NOTES))
                        pData->options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
                    // nobreak, in case another port supports MIDI
                }
            }
        }

//         if (fEffect->numPrograms > 1 && (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES) == 0)
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_MAP_PROGRAM_CHANGES))
//                 pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        return true;
    }

private:
    const clap_plugin_t* fPlugin;
    const clap_plugin_descriptor_t* fPluginDescriptor;
    const clap_plugin_entry_t* fPluginEntry;
    const carla_clap_host fHost;

    struct Extensions {
        const clap_plugin_latency_t* latency;
        const clap_plugin_params_t* params;
        const clap_plugin_state_t* state;
        const clap_plugin_timer_support_t* timer;
      #ifdef CLAP_WINDOW_API_NATIVE
        const clap_plugin_gui_t* gui;
       #ifdef _POSIX_VERSION
        const clap_plugin_posix_fd_support_t* posixFD;
       #endif
      #endif

        Extensions()
            : latency(nullptr),
              params(nullptr),
              state(nullptr),
              timer(nullptr)
          #ifdef CLAP_WINDOW_API_NATIVE
            , gui(nullptr)
           #ifdef _POSIX_VERSION
            , posixFD(nullptr)
           #endif
          #endif
        {
        }

        CARLA_DECLARE_NON_COPYABLE(Extensions)
    } fExtensions;

  #ifdef CLAP_WINDOW_API_NATIVE
    struct UI {
        bool initalized;
        bool isCreated;
        bool isEmbed;
        bool isVisible;
        bool isResizingFromHost;
        bool isResizingFromInit;
        int isResizingFromPlugin;
        bool shouldClose;
        uint32_t width, height;
        CarlaPluginUI* window;

        UI()
            : initalized(false),
              isCreated(false),
              isEmbed(false),
              isVisible(false),
              isResizingFromHost(false),
              isResizingFromInit(false),
              isResizingFromPlugin(0),
              shouldClose(false),
              width(0),
              height(0),
              window(nullptr) {}

        ~UI()
        {
            CARLA_SAFE_ASSERT(window == nullptr);
        }

        CARLA_DECLARE_NON_COPYABLE(UI)
    } fUI;

   #ifdef _POSIX_VERSION
    LinkedList<HostPosixFileDescriptorDetails> fPosixFileDescriptors;
   #endif
    LinkedList<HostTimerDetails> fTimers;
  #endif

    carla_clap_input_audio_buffers fInputAudioBuffers;
    carla_clap_output_audio_buffers fOutputAudioBuffers;
    carla_clap_input_events fInputEvents;
    carla_clap_output_events fOutputEvents;
   #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    float** fAudioOutBuffers;
   #endif
    void* fLastChunk;
    uint32_t fLastKnownLatency;
    const bool kEngineHasIdleOnMainThread;
    bool fNeedsParamFlush;
    bool fNeedsRestart;
    bool fNeedsProcess;
    bool fNeedsIdleCallback;

   #ifdef CARLA_OS_MAC
    BundleLoader fBundleLoader;
   #endif

    void runIdleCallbacksAsNeeded(const bool isIdleCallback)
    {
        if (isIdleCallback && (fNeedsRestart || fNeedsProcess))
        {
            carla_stdout("runIdleCallbacksAsNeeded %d %d", fNeedsRestart, fNeedsProcess);
            const bool needsRestart = fNeedsRestart;

            if (needsRestart)
            {
                fNeedsRestart = false;
                setActive(false, true, true);
            }

            if (fNeedsProcess)
            {
                fNeedsProcess = false;
                setEnabled(true);
                setActive(true, true, true);
            }
            else if (needsRestart)
            {
                setActive(true, true, true);
            }
        }

        if (fNeedsParamFlush)
        {
            fNeedsParamFlush = false;

            carla_clap_input_events copy;
            copy.reallocEqualTo(fInputEvents);

            {
                const ScopedSingleProcessLocker sspl(this, true);
                fInputEvents.handleScheduledParameterUpdates();
                fInputEvents.swap(copy);
            }

            fExtensions.params->flush(fPlugin, copy.cast(), nullptr);
        }

        if (fNeedsIdleCallback)
        {
            fNeedsIdleCallback = false;
            fPlugin->on_main_thread(fPlugin);
        }

      #ifdef CLAP_WINDOW_API_NATIVE
       #ifdef _POSIX_VERSION
        for (LinkedList<HostPosixFileDescriptorDetails>::Itenerator it = fPosixFileDescriptors.begin2(); it.valid(); it.next())
        {
            const HostPosixFileDescriptorDetails& posixFD(it.getValue(kPosixFileDescriptorFallback));

           #ifdef CARLA_CLAP_POSIX_EPOLL
            struct ::epoll_event event;
           #else
            const int16_t filter = posixFD.flags & CLAP_POSIX_FD_WRITE ? EVFILT_WRITE : EVFILT_READ;
            struct ::kevent kev = {}, event;
            struct ::timespec timeout = {};
            EV_SET(&kev, posixFD.pluginFd, filter, EV_ADD|EV_ENABLE, 0, 0, nullptr);
           #endif

            for (int i=0; i<50; ++i)
            {
               #ifdef CARLA_CLAP_POSIX_EPOLL
                switch (::epoll_wait(posixFD.hostFd, &event, 1, 0))
               #else
                switch (::kevent(posixFD.hostFd, &kev, 1, &event, 1, &timeout))
               #endif
                {
                case 1:
                    fExtensions.posixFD->on_fd(fPlugin, posixFD.pluginFd, posixFD.flags);
                    break;
                case -1:
                    fExtensions.posixFD->on_fd(fPlugin, posixFD.pluginFd, posixFD.flags | CLAP_POSIX_FD_ERROR);
                    // fall through
                case 0:
                    i = 50;
                    break;
                default:
                    carla_safe_exception("posix fd received abnormal value", __FILE__, __LINE__);
                    i = 50;
                    break;
                }
            }
        }
       #endif // _POSIX_VERSION

        for (LinkedList<HostTimerDetails>::Itenerator it = fTimers.begin2(); it.valid(); it.next())
        {
            const uint32_t currentTimeInMs = water::Time::getMillisecondCounter();
            HostTimerDetails& timer(it.getValue(kTimerFallbackNC));

            if (currentTimeInMs > timer.lastCallTimeInMs + timer.periodInMs)
            {
                timer.lastCallTimeInMs = currentTimeInMs;
                fExtensions.timer->on_timer(fPlugin, timer.clapId);
            }
        }
      #endif // CLAP_WINDOW_API_NATIVE
    }
};

// --------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newCLAP(const Initializer& init)
{
    carla_debug("CarlaPlugin::newCLAP({%p, \"%s\", \"%s\", \"%s\"})",
                init.engine, init.filename, init.name, init.label);

    std::shared_ptr<CarlaPluginCLAP> plugin(new CarlaPluginCLAP(init.engine, init.id));

    if (! plugin->init(plugin, init.filename, init.name, init.label, init.options))
        return nullptr;

    return plugin;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
