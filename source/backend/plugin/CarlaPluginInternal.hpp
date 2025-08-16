// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CARLA_PLUGIN_INTERNAL_HPP_INCLUDED
#define CARLA_PLUGIN_INTERNAL_HPP_INCLUDED

#include "CarlaPlugin.hpp"

#include "CarlaJuceUtils.hpp"
#include "CarlaLibUtils.hpp"
#include "CarlaStateUtils.hpp"

#include "CarlaMIDI.h"
#include "CarlaMutex.hpp"
#include "RtLinkedList.hpp"

#include "extra/String.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Engine helper macro, sets lastError and returns false/NULL

#define CARLA_SAFE_ASSERT_RETURN_ERR(cond, err)  if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); pData->engine->setLastError(err); return false;   }
#define CARLA_SAFE_ASSERT_RETURN_ERRN(cond, err) if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); pData->engine->setLastError(err); return nullptr; }

#define CARLA_SAFE_EXCEPTION_RETURN_ERR(excptMsg, errMsg)  catch(...) { carla_safe_exception(excptMsg, __FILE__, __LINE__); pData->engine->setLastError(errMsg); return false;   }
#define CARLA_SAFE_EXCEPTION_RETURN_ERRN(excptMsg, errMsg) catch(...) { carla_safe_exception(excptMsg, __FILE__, __LINE__); pData->engine->setLastError(errMsg); return nullptr; }

// -----------------------------------------------------------------------
// Maximum pre-allocated events for some plugin types

constexpr const uint16_t kPluginMaxMidiEvents = 512;

// -----------------------------------------------------------------------
// Extra parameter hints, hidden from backend

constexpr const uint PARAMETER_MAPPED_RANGES_SET = 0x10000;
constexpr const uint PARAMETER_IS_STRICT_BOUNDS  = 0x20000;
constexpr const uint PARAMETER_IS_TRIGGER        = 0x40000;

// -----------------------------------------------------------------------
// Extra plugin hints, hidden from backend

constexpr const uint PLUGIN_EXTRA_HINT_HAS_MIDI_IN  = 0x01;
constexpr const uint PLUGIN_EXTRA_HINT_HAS_MIDI_OUT = 0x02;

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
 * During process, we cannot lock, allocate memory or do UI stuff.
 * Events have to be postponned to be executed later, on a separate thread.
 * @see PluginPostRtEvent
 */
enum PluginPostRtEventType {
    kPluginPostRtEventNull = 0,
    kPluginPostRtEventParameterChange,
    kPluginPostRtEventProgramChange,
    kPluginPostRtEventMidiProgramChange,
    kPluginPostRtEventNoteOn,
    kPluginPostRtEventNoteOff,
    kPluginPostRtEventMidiLearn
};

/*!
 * A Post-RT event.
 * @see PluginPostRtEventType
 */
struct PluginPostRtEvent {
    PluginPostRtEventType type;
    bool sendCallback;
    union {
        struct {
            int32_t index;
            float value;
        } parameter;
        struct {
            uint32_t index;
        } program;
        struct {
            uint8_t channel;
            uint8_t note;
            uint8_t velocity;
        } note;
        struct {
            uint32_t parameter;
            uint8_t cc;
            uint8_t channel;
        } midiLearn;
    };
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
    void createNew(uint32_t newCount);
    void clear() noexcept;
    void initBuffers() const noexcept;

    CARLA_DECLARE_NON_COPYABLE(PluginAudioData)
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
    void createNew(uint32_t newCount);
    void clear() noexcept;
    void initBuffers() const noexcept;

    CARLA_DECLARE_NON_COPYABLE(PluginCVData)
};

// -----------------------------------------------------------------------

struct PluginEventData {
    CarlaEngineEventPort* portIn;
    CarlaEngineEventPort* portOut;
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CarlaEngineCVSourcePorts* cvSourcePorts;
#endif

    PluginEventData() noexcept;
    ~PluginEventData() noexcept;
    void clear() noexcept;
    void initBuffers() const noexcept;

    CARLA_DECLARE_NON_COPYABLE(PluginEventData)
};

// -----------------------------------------------------------------------

struct PluginParameterData {
    uint32_t count;
    ParameterData* data;
    ParameterRanges* ranges;
    SpecialParameterType* special;

    PluginParameterData() noexcept;
    ~PluginParameterData() noexcept;
    void createNew(uint32_t newCount, bool withSpecial);
    void clear() noexcept;
    float getFixedValue(uint32_t parameterId, float value) const noexcept;
    float getFinalUnnormalizedValue(uint32_t parameterId, float normalizedValue) const noexcept;
    float getFinalValueWithMidiDelta(uint32_t parameterId, float value, int8_t delta) const noexcept;

    CARLA_DECLARE_NON_COPYABLE(PluginParameterData)
};

// -----------------------------------------------------------------------

typedef const char* ProgramName;

struct PluginProgramData {
    uint32_t count;
    int32_t current;
    ProgramName* names;

    PluginProgramData() noexcept;
    ~PluginProgramData() noexcept;
    void createNew(uint32_t newCount);
    void clear() noexcept;

    CARLA_DECLARE_NON_COPYABLE(PluginProgramData)
};

// -----------------------------------------------------------------------

struct PluginMidiProgramData {
    uint32_t count;
    int32_t current;
    MidiProgramData* data;

    PluginMidiProgramData() noexcept;
    ~PluginMidiProgramData() noexcept;
    void createNew(uint32_t newCount);
    void clear() noexcept;
    const MidiProgramData& getCurrent() const noexcept;

    CARLA_DECLARE_NON_COPYABLE(PluginMidiProgramData)
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

    bool engineBridged;
    bool enginePlugin;

    lib_t lib;
    lib_t uiLib;

    // misc
    int8_t ctrlChannel;
    uint   extraHints;
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    int32_t midiLearnParameterIndex;
    uint    transientTryCounter;
    bool    transientFirstTry;
#endif

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

    String uiTitle;

    struct ExternalNotes {
        CarlaMutex mutex;
        RtLinkedList<ExternalMidiNote>::Pool dataPool;
        RtLinkedList<ExternalMidiNote> data;

        ExternalNotes() noexcept;
        ~ExternalNotes() noexcept;
        void appendNonRT(const ExternalMidiNote& note) noexcept;
        void clear() noexcept;

        CARLA_DECLARE_NON_COPYABLE(ExternalNotes)

    } extNotes;

    struct Latency {
        uint32_t frames;
#ifndef BUILD_BRIDGE
        uint32_t channels;
        float**  buffers;
#endif

        Latency() noexcept;
#ifndef BUILD_BRIDGE
        ~Latency() noexcept;
        void clearBuffers() noexcept;
        void recreateBuffers(uint32_t newChannels, uint32_t newFrames);
#endif

        CARLA_DECLARE_NON_COPYABLE(Latency)

    } latency;

    class PostRtEvents {
    public:
        PostRtEvents() noexcept;
        ~PostRtEvents() noexcept;
        void appendRT(const PluginPostRtEvent& event) noexcept;
        void trySplice() noexcept;

        struct Access {
            Access(PostRtEvents& e)
              : data2(e.dataPool),
                poolMutex(e.poolMutex)
            {
                const CarlaMutexLocker cml1(e.dataMutex);
                const CarlaMutexLocker cml2(e.poolMutex);

                if (e.data.isNotEmpty())
                    e.data.moveTo(data2, true);
            }

            ~Access()
            {
                const CarlaMutexLocker cml(poolMutex);

                data2.clear();
            }

            inline RtLinkedList<PluginPostRtEvent>::Itenerator getDataIterator() const noexcept
            {
                return data2.begin2();
            }

            inline std::size_t isEmpty() const noexcept
            {
                return data2.isEmpty();
            }

        private:
            RtLinkedList<PluginPostRtEvent> data2;
            CarlaMutex& poolMutex;
        };

    private:
        RtLinkedList<PluginPostRtEvent>::Pool dataPool;
        RtLinkedList<PluginPostRtEvent> data, dataPendingRT;
        CarlaMutex dataMutex;
        CarlaMutex dataPendingMutex;
        CarlaMutex poolMutex;

        CARLA_DECLARE_NON_COPYABLE(PostRtEvents)

    } postRtEvents;

    struct PostUiEvents {
        CarlaMutex mutex;
        LinkedList<PluginPostRtEvent> data;

        PostUiEvents() noexcept;
        ~PostUiEvents() noexcept;
        void append(const PluginPostRtEvent& event) noexcept;
        void clear() noexcept;

        CARLA_DECLARE_NON_COPYABLE(PostUiEvents)

    } postUiEvents;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    struct PostProc {
        float dryWet;
        float volume;
        float balanceLeft;
        float balanceRight;
        float panning;
        float* extraBuffer;

        PostProc() noexcept;

        CARLA_DECLARE_NON_COPYABLE(PostProc)

    } postProc;
#endif

    ProtectedData(CarlaEngine* engine, uint idx) noexcept;
    ~ProtectedData() noexcept;

    // -------------------------------------------------------------------
    // Buffer functions

    void clearBuffers() noexcept;

    // -------------------------------------------------------------------
    // Post-poned events

    void postponeRtEvent(const PluginPostRtEvent& rtEvent) noexcept;
    void postponeParameterChangeRtEvent(bool sendCallbackLater, int32_t index, float value) noexcept;
    void postponeProgramChangeRtEvent(bool sendCallbackLater, uint32_t index) noexcept;
    void postponeMidiProgramChangeRtEvent(bool sendCallbackLater, uint32_t index) noexcept;
    void postponeNoteOnRtEvent(bool sendCallbackLater, uint8_t channel, uint8_t note, uint8_t velocity) noexcept;
    void postponeNoteOffRtEvent(bool sendCallbackLater, uint8_t channel, uint8_t note) noexcept;
    void postponeMidiLearnRtEvent(bool sendCallbackLater, uint32_t parameter, uint8_t cc, uint8_t channel) noexcept;

    // -------------------------------------------------------------------
    // Library functions

    static const char* libError(const char* filename) noexcept;

    bool libOpen(const char* filename) noexcept;
    bool libClose() noexcept;
    void setCanDeleteLib(bool canDelete) noexcept;

    bool uiLibOpen(const char* filename, bool canDelete) noexcept;
    bool uiLibClose() noexcept;

    template<typename Func>
    Func libSymbol(const char* symbol) const noexcept
    {
        return lib_symbol<Func>(lib, symbol);
    }

    template<typename Func>
    Func uiLibSymbol(const char* symbol) const noexcept
    {
        return lib_symbol<Func>(uiLib, symbol);
    }

    // -------------------------------------------------------------------
    // Misc

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    void tryTransient() noexcept;
#endif
    void updateParameterValues(CarlaPlugin* plugin,
                               bool sendCallback, bool sendOsc, bool useDefault) noexcept;
    void updateDefaultParameterValues(CarlaPlugin* plugin) noexcept;

    // -------------------------------------------------------------------

#ifdef CARLA_PROPER_CPP11_SUPPORT
    ProtectedData() = delete;
    CARLA_DECLARE_NON_COPYABLE(ProtectedData);
#endif
    CARLA_LEAK_DETECTOR(ProtectedData);
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_PLUGIN_INTERNAL_HPP_INCLUDED
