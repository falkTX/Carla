// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// testing macros
// #define LV2_UIS_ONLY_BRIDGES
// #define LV2_UIS_ONLY_INPROCESS

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#include "CarlaLv2Utils.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaPipeUtils.hpp"
#include "CarlaPluginUI.hpp"
#include "CarlaScopeUtils.hpp"
#include "Lv2AtomRingBuffer.hpp"

#include "extra/Base64.hpp"

#include "../modules/lilv/config/lilv_config.h"

extern "C" {
#include "rtmempool/rtmempool-lv2.h"
}

#include "water/files/File.h"
#include "water/misc/Time.h"

#ifdef CARLA_OS_MAC
# include "CarlaMacUtils.hpp"
# if defined(CARLA_OS_64BIT) && defined(HAVE_LIBMAGIC) && ! defined(BUILD_BRIDGE_ALTERNATIVE_ARCH)
#  define ADAPT_FOR_APPLE_SILLICON
#  include "CarlaBinaryUtils.hpp"
# endif
#endif

#ifdef CARLA_OS_WASM
# define LV2_UIS_ONLY_INPROCESS
#endif

#include <string>
#include <vector>

using water::File;

#define URI_CARLA_ATOM_WORKER_IN   "http://kxstudio.sf.net/ns/carla/atomWorkerIn"
#define URI_CARLA_ATOM_WORKER_RESP "http://kxstudio.sf.net/ns/carla/atomWorkerResp"
#define URI_CARLA_PARAMETER_CHANGE "http://kxstudio.sf.net/ns/carla/parameterChange"

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Fallback data

static const CustomData       kCustomDataFallback       = { nullptr, nullptr, nullptr };
static /* */ CustomData       kCustomDataFallbackNC     = { nullptr, nullptr, nullptr };
static const ExternalMidiNote kExternalMidiNoteFallback = { -1, 0, 0 };
static const char* const      kUnmapFallback            = "urn:null";

// -------------------------------------------------------------------------------------------------------------------

// Maximum default buffer size
const uint MAX_DEFAULT_BUFFER_SIZE = 8192; // 0x2000

// Extra Plugin Hints
const uint PLUGIN_HAS_EXTENSION_OPTIONS        = 0x01000;
const uint PLUGIN_HAS_EXTENSION_PROGRAMS       = 0x02000;
const uint PLUGIN_HAS_EXTENSION_STATE          = 0x04000;
const uint PLUGIN_HAS_EXTENSION_WORKER         = 0x08000;
const uint PLUGIN_HAS_EXTENSION_INLINE_DISPLAY = 0x10000;
const uint PLUGIN_HAS_EXTENSION_MIDNAM         = 0x20000;

// LV2 Event Data/Types
const uint CARLA_EVENT_DATA_ATOM    = 0x01;
const uint CARLA_EVENT_DATA_EVENT   = 0x02;
const uint CARLA_EVENT_DATA_MIDI_LL = 0x04;
const uint CARLA_EVENT_TYPE_MESSAGE = 0x10; // unused
const uint CARLA_EVENT_TYPE_MIDI    = 0x20;
const uint CARLA_EVENT_TYPE_TIME    = 0x40;

// LV2 URI Map Ids
enum CarlaLv2URIDs {
    kUridNull = 0,
    kUridAtomBlank,
    kUridAtomBool,
    kUridAtomChunk,
    kUridAtomDouble,
    kUridAtomEvent,
    kUridAtomFloat,
    kUridAtomInt,
    kUridAtomLiteral,
    kUridAtomLong,
    kUridAtomNumber,
    kUridAtomObject,
    kUridAtomPath,
    kUridAtomProperty,
    kUridAtomResource,
    kUridAtomSequence,
    kUridAtomSound,
    kUridAtomString,
    kUridAtomTuple,
    kUridAtomURI,
    kUridAtomURID,
    kUridAtomVector,
    kUridAtomTransferAtom,
    kUridAtomTransferEvent,
    kUridBufMaxLength,
    kUridBufMinLength,
    kUridBufNominalLength,
    kUridBufSequenceSize,
    kUridLogError,
    kUridLogNote,
    kUridLogTrace,
    kUridLogWarning,
    kUridPatchSet,
    kUridPatchProperty,
    kUridPatchSubject,
    kUridPatchValue,
    // time base type
    kUridTimePosition,
     // time values
    kUridTimeBar,
    kUridTimeBarBeat,
    kUridTimeBeat,
    kUridTimeBeatUnit,
    kUridTimeBeatsPerBar,
    kUridTimeBeatsPerMinute,
    kUridTimeFrame,
    kUridTimeFramesPerSecond,
    kUridTimeSpeed,
    kUridTimeTicksPerBeat,
    kUridMidiEvent,
    kUridParamSampleRate,
    // ui stuff
    kUridBackgroundColor,
    kUridForegroundColor,
#ifndef CARLA_OS_MAC
    kUridScaleFactor,
#endif
    kUridWindowTitle,
    // custom carla props
    kUridCarlaAtomWorkerIn,
    kUridCarlaAtomWorkerResp,
    kUridCarlaParameterChange,
    kUridCarlaTransientWindowId,
    // count
    kUridCount
};

// LV2 Feature Ids
enum CarlaLv2Features {
    // DSP features
    kFeatureIdBufSizeBounded = 0,
    kFeatureIdBufSizeFixed,
    kFeatureIdBufSizePowerOf2,
    kFeatureIdEvent,
    kFeatureIdHardRtCapable,
    kFeatureIdInPlaceBroken,
    kFeatureIdIsLive,
    kFeatureIdLogs,
    kFeatureIdOptions,
    kFeatureIdPrograms,
    kFeatureIdResizePort,
    kFeatureIdRtMemPool,
    kFeatureIdRtMemPoolOld,
    kFeatureIdStateFreePath,
    kFeatureIdStateMakePath,
    kFeatureIdStateMapPath,
    kFeatureIdStrictBounds,
    kFeatureIdUriMap,
    kFeatureIdUridMap,
    kFeatureIdUridUnmap,
    kFeatureIdWorker,
    kFeatureIdInlineDisplay,
    kFeatureIdMidnam,
    kFeatureIdCtrlInPortChangeReq,
    kFeatureCountPlugin,
    // UI features
    kFeatureIdUiDataAccess = kFeatureCountPlugin,
    kFeatureIdUiInstanceAccess,
    kFeatureIdUiIdleInterface,
    kFeatureIdUiFixedSize,
    kFeatureIdUiMakeResident,
    kFeatureIdUiMakeResident2,
    kFeatureIdUiNoUserResize,
    kFeatureIdUiParent,
    kFeatureIdUiPortMap,
    kFeatureIdUiPortSubscribe,
    kFeatureIdUiResize,
    kFeatureIdUiRequestValue,
    kFeatureIdUiTouch,
    kFeatureIdExternalUi,
    kFeatureIdExternalUiOld,
    kFeatureCountAll
};

// LV2 Feature Ids (special state handlers)
enum CarlaLv2StateFeatures {
    kStateFeatureIdFreePath,
    kStateFeatureIdMakePath,
    kStateFeatureIdMapPath,
    kStateFeatureIdWorker,
    kStateFeatureCountAll
};

// -------------------------------------------------------------------------------------------------------------------

struct Lv2EventData {
    uint32_t type;
    uint32_t rindex;
    CarlaEngineEventPort* port;

    union {
        LV2_Atom_Buffer* atom;
        LV2_Event_Buffer* event;
        LV2_MIDI midi;
    };

    Lv2EventData() noexcept
        : type(0x0),
          rindex(0),
          port(nullptr) {}

    ~Lv2EventData() noexcept
    {
        if (port != nullptr)
        {
            delete port;
            port = nullptr;
        }

        const uint32_t rtype = type;
        type = 0x0;

        if (rtype & CARLA_EVENT_DATA_ATOM)
        {
            CARLA_SAFE_ASSERT_RETURN(atom != nullptr,);

            std::free(atom);
            atom = nullptr;
        }
        else if (rtype & CARLA_EVENT_DATA_EVENT)
        {
            CARLA_SAFE_ASSERT_RETURN(event != nullptr,);

            std::free(event);
            event = nullptr;
        }
        else if (rtype & CARLA_EVENT_DATA_MIDI_LL)
        {
            CARLA_SAFE_ASSERT_RETURN(midi.data != nullptr,);

            delete[] midi.data;
            midi.data = nullptr;
        }
    }

    CARLA_DECLARE_NON_COPYABLE(Lv2EventData)
};

union LV2EventIters {
    LV2_Atom_Buffer_Iterator atom;
    LV2_Event_Iterator event;
    LV2_MIDIState midiState;
};

struct CarlaPluginLV2EventData {
    uint32_t count;
    Lv2EventData* data;
    LV2EventIters* iters;
    Lv2EventData* ctrl; // default port, either this->data[x] or pData->portIn/Out
    uint32_t ctrlIndex;

    CarlaPluginLV2EventData() noexcept
        : count(0),
          data(nullptr),
          iters(nullptr),
          ctrl(nullptr),
          ctrlIndex(0) {}

    ~CarlaPluginLV2EventData() noexcept
    {
        CARLA_SAFE_ASSERT_INT(count == 0, count);
        CARLA_SAFE_ASSERT(data == nullptr);
        CARLA_SAFE_ASSERT(iters == nullptr);
        CARLA_SAFE_ASSERT(ctrl == nullptr);
        CARLA_SAFE_ASSERT_INT(ctrlIndex == 0, ctrlIndex);
    }

    void createNew(const uint32_t newCount)
    {
        CARLA_SAFE_ASSERT_INT(count == 0, count);
        CARLA_SAFE_ASSERT_INT(ctrlIndex == 0, ctrlIndex);
        CARLA_SAFE_ASSERT_RETURN(data == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(iters == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(ctrl == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(newCount > 0,);

        data  = new Lv2EventData[newCount];
        iters = new LV2EventIters[newCount];
        count = newCount;

        ctrl      = nullptr;
        ctrlIndex = 0;
    }

    void clear(CarlaEngineEventPort* const portToIgnore) noexcept
    {
        if (data != nullptr)
        {
            for (uint32_t i=0; i < count; ++i)
            {
                if (data[i].port != nullptr)
                {
                    if (data[i].port != portToIgnore)
                        delete data[i].port;
                    data[i].port = nullptr;
                }
            }

            delete[] data;
            data = nullptr;
        }

        if (iters != nullptr)
        {
            delete[] iters;
            iters = nullptr;
        }

        count = 0;

        ctrl      = nullptr;
        ctrlIndex = 0;
    }

    void initBuffers() const noexcept
    {
        for (uint32_t i=0; i < count; ++i)
        {
            if (data[i].port != nullptr && (ctrl == nullptr || data[i].port != ctrl->port))
                data[i].port->initBuffer();
        }
    }

    CARLA_DECLARE_NON_COPYABLE(CarlaPluginLV2EventData)
};

// -------------------------------------------------------------------------------------------------------------------

struct CarlaPluginLV2Options {
    enum OptIndex {
        MaxBlockLenth = 0,
        MinBlockLenth,
        NominalBlockLenth,
        SequenceSize,
        SampleRate,
        TransientWinId,
        BackgroundColor,
        ForegroundColor,
#ifndef CARLA_OS_MAC
        ScaleFactor,
#endif
        WindowTitle,
        Null,
        Count
    };

    int maxBufferSize;
    int minBufferSize;
    int nominalBufferSize;
    int sequenceSize;
    float sampleRate;
    int64_t transientWinId;
    uint32_t bgColor;
    uint32_t fgColor;
    float uiScale;
    const char* windowTitle;
    LV2_Options_Option opts[Count];

    CarlaPluginLV2Options() noexcept
        : maxBufferSize(0),
          minBufferSize(0),
          nominalBufferSize(0),
          sequenceSize(MAX_DEFAULT_BUFFER_SIZE),
          sampleRate(0.0),
          transientWinId(0),
          bgColor(0x000000ff),
          fgColor(0xffffffff),
          uiScale(1.0f),
          windowTitle(nullptr)
    {
        LV2_Options_Option& optMaxBlockLenth(opts[MaxBlockLenth]);
        optMaxBlockLenth.context = LV2_OPTIONS_INSTANCE;
        optMaxBlockLenth.subject = 0;
        optMaxBlockLenth.key     = kUridBufMaxLength;
        optMaxBlockLenth.size    = sizeof(int);
        optMaxBlockLenth.type    = kUridAtomInt;
        optMaxBlockLenth.value   = &maxBufferSize;

        LV2_Options_Option& optMinBlockLenth(opts[MinBlockLenth]);
        optMinBlockLenth.context = LV2_OPTIONS_INSTANCE;
        optMinBlockLenth.subject = 0;
        optMinBlockLenth.key     = kUridBufMinLength;
        optMinBlockLenth.size    = sizeof(int);
        optMinBlockLenth.type    = kUridAtomInt;
        optMinBlockLenth.value   = &minBufferSize;

        LV2_Options_Option& optNominalBlockLenth(opts[NominalBlockLenth]);
        optNominalBlockLenth.context = LV2_OPTIONS_INSTANCE;
        optNominalBlockLenth.subject = 0;
        optNominalBlockLenth.key     = kUridBufNominalLength;
        optNominalBlockLenth.size    = sizeof(int);
        optNominalBlockLenth.type    = kUridAtomInt;
        optNominalBlockLenth.value   = &nominalBufferSize;

        LV2_Options_Option& optSequenceSize(opts[SequenceSize]);
        optSequenceSize.context = LV2_OPTIONS_INSTANCE;
        optSequenceSize.subject = 0;
        optSequenceSize.key     = kUridBufSequenceSize;
        optSequenceSize.size    = sizeof(int);
        optSequenceSize.type    = kUridAtomInt;
        optSequenceSize.value   = &sequenceSize;

        LV2_Options_Option& optBackgroundColor(opts[BackgroundColor]);
        optBackgroundColor.context = LV2_OPTIONS_INSTANCE;
        optBackgroundColor.subject = 0;
        optBackgroundColor.key     = kUridBackgroundColor;
        optBackgroundColor.size    = sizeof(int32_t);
        optBackgroundColor.type    = kUridAtomInt;
        optBackgroundColor.value   = &bgColor;

        LV2_Options_Option& optForegroundColor(opts[ForegroundColor]);
        optForegroundColor.context = LV2_OPTIONS_INSTANCE;
        optForegroundColor.subject = 0;
        optForegroundColor.key     = kUridForegroundColor;
        optForegroundColor.size    = sizeof(int32_t);
        optForegroundColor.type    = kUridAtomInt;
        optForegroundColor.value   = &fgColor;

#ifndef CARLA_OS_MAC
        LV2_Options_Option& optScaleFactor(opts[ScaleFactor]);
        optScaleFactor.context = LV2_OPTIONS_INSTANCE;
        optScaleFactor.subject = 0;
        optScaleFactor.key     = kUridScaleFactor;
        optScaleFactor.size    = sizeof(float);
        optScaleFactor.type    = kUridAtomFloat;
        optScaleFactor.value   = &uiScale;
#endif

        LV2_Options_Option& optSampleRate(opts[SampleRate]);
        optSampleRate.context = LV2_OPTIONS_INSTANCE;
        optSampleRate.subject = 0;
        optSampleRate.key     = kUridParamSampleRate;
        optSampleRate.size    = sizeof(float);
        optSampleRate.type    = kUridAtomFloat;
        optSampleRate.value   = &sampleRate;

        LV2_Options_Option& optTransientWinId(opts[TransientWinId]);
        optTransientWinId.context = LV2_OPTIONS_INSTANCE;
        optTransientWinId.subject = 0;
        optTransientWinId.key     = kUridCarlaTransientWindowId;
        optTransientWinId.size    = sizeof(int64_t);
        optTransientWinId.type    = kUridAtomLong;
        optTransientWinId.value   = &transientWinId;

        LV2_Options_Option& optWindowTitle(opts[WindowTitle]);
        optWindowTitle.context = LV2_OPTIONS_INSTANCE;
        optWindowTitle.subject = 0;
        optWindowTitle.key     = kUridWindowTitle;
        optWindowTitle.size    = 0;
        optWindowTitle.type    = kUridAtomString;
        optWindowTitle.value   = nullptr;

        LV2_Options_Option& optNull(opts[Null]);
        optNull.context = LV2_OPTIONS_INSTANCE;
        optNull.subject = 0;
        optNull.key     = kUridNull;
        optNull.size    = 0;
        optNull.type    = kUridNull;
        optNull.value   = nullptr;
    }

    ~CarlaPluginLV2Options() noexcept
    {
        LV2_Options_Option& optWindowTitle(opts[WindowTitle]);

        optWindowTitle.size  = 0;
        optWindowTitle.value = nullptr;

        if (windowTitle != nullptr)
        {
            std::free(const_cast<char*>(windowTitle));
            windowTitle = nullptr;
        }
    }

    CARLA_DECLARE_NON_COPYABLE(CarlaPluginLV2Options);
};

#ifndef LV2_UIS_ONLY_INPROCESS
// -------------------------------------------------------------------------------------------------------------------

class CarlaPluginLV2;

class CarlaPipeServerLV2 : public CarlaPipeServer
{
public:
    enum UiState {
        UiNone = 0,
        UiHide,
        UiShow,
        UiCrashed
    };

    CarlaPipeServerLV2(CarlaEngine* const engine, CarlaPluginLV2* const plugin)
        : kEngine(engine),
          kPlugin(plugin),
          fFilename(),
          fPluginURI(),
          fUiURI(),
          fUiState(UiNone) {}

    ~CarlaPipeServerLV2() noexcept override
    {
        CARLA_SAFE_ASSERT_INT(fUiState == UiNone, fUiState);
    }

    UiState getAndResetUiState() noexcept
    {
        const UiState uiState(fUiState);
        fUiState = UiNone;
        return uiState;
    }

    void setData(const char* const filename, const char* const pluginURI, const char* const uiURI) noexcept
    {
        fFilename  = filename;
        fPluginURI = pluginURI;
        fUiURI     = uiURI;
    }

    bool startPipeServer(const int size) noexcept
    {
        char sampleRateStr[32];
        {
            const ScopedSafeLocale ssl;
            std::snprintf(sampleRateStr, 31, "%.12g", kEngine->getSampleRate());
        }
        sampleRateStr[31] = '\0';

        const ScopedEngineEnvironmentLocker _seel(kEngine);
        const CarlaScopedEnvVar _sev1("LV2_PATH", kEngine->getOptions().pathLV2);
#ifdef CARLA_OS_LINUX
        const CarlaScopedEnvVar _sev2("LD_PRELOAD", nullptr);
#endif
        carla_setenv("CARLA_SAMPLE_RATE", sampleRateStr);

        return CarlaPipeServer::startPipeServer(fFilename, fPluginURI, fUiURI, size);
    }

    void writeUiTitleMessage(const char* const title) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(title != nullptr && title[0] != '\0',);

        const CarlaMutexLocker cml(getPipeLock());

        if (! _writeMsgBuffer("uiTitle\n", 8))
            return;
        if (! writeAndFixMessage(title))
            return;

        syncMessages();
    }

protected:
    // returns true if msg was handled
    bool msgReceived(const char* const msg) noexcept override;

private:
    CarlaEngine*    const kEngine;
    CarlaPluginLV2* const kPlugin;

    String fFilename;
    String fPluginURI;
    String fUiURI;
    UiState fUiState;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPipeServerLV2)
};

// -------------------------------------------------------------------------------------------------------------------
#endif

static void initAtomForge(LV2_Atom_Forge& atomForge) noexcept
{
    carla_zeroStruct(atomForge);

    atomForge.Bool     = kUridAtomBool;
    atomForge.Chunk    = kUridAtomChunk;
    atomForge.Double   = kUridAtomDouble;
    atomForge.Float    = kUridAtomFloat;
    atomForge.Int      = kUridAtomInt;
    atomForge.Literal  = kUridAtomLiteral;
    atomForge.Long     = kUridAtomLong;
    atomForge.Object   = kUridAtomObject;
    atomForge.Path     = kUridAtomPath;
    atomForge.Property = kUridAtomProperty;
    atomForge.Sequence = kUridAtomSequence;
    atomForge.String   = kUridAtomString;
    atomForge.Tuple    = kUridAtomTuple;
    atomForge.URI      = kUridAtomURI;
    atomForge.URID     = kUridAtomURID;
    atomForge.Vector   = kUridAtomVector;

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    atomForge.Blank    = kUridAtomBlank;
    atomForge.Resource = kUridAtomResource;
#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif
}

// -------------------------------------------------------------------------------------------------------------------

class CarlaPluginLV2 : public CarlaPlugin,
                       private CarlaPluginUI::Callback
{
public:
    CarlaPluginLV2(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fHandle(nullptr),
          fHandle2(nullptr),
          fDescriptor(nullptr),
          fRdfDescriptor(nullptr),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fCvInBuffers(nullptr),
          fCvOutBuffers(nullptr),
          fParamBuffers(nullptr),
          fHasLoadDefaultState(false),
          fHasThreadSafeRestore(false),
          fNeedsFixedBuffers(false),
          fNeedsUiClose(false),
          fInlineDisplayNeedsRedraw(false),
          fInlineDisplayLastRedrawTime(0),
          fLatencyIndex(-1),
          fStrictBounds(-1),
          fAtomBufferEvIn(),
          fAtomBufferUiOut(),
          fAtomBufferWorkerIn(),
          fAtomBufferWorkerResp(),
          fAtomBufferUiOutTmpData(nullptr),
          fAtomBufferWorkerInTmpData(nullptr),
          fAtomBufferRealtime(nullptr),
          fAtomBufferRealtimeSize(0),
          fEventsIn(),
          fEventsOut(),
          fLv2Options(),
#ifndef LV2_UIS_ONLY_INPROCESS
          fPipeServer(engine, this),
#endif
          fCustomURIDs(kUridCount, std::string("urn:null")),
          fFirstActive(true),
          fLastStateChunk(nullptr),
          fLastTimeInfo(),
          fFilePathURI(),
          fExt(),
          fUI()
    {
        carla_debug("CarlaPluginLV2::CarlaPluginLV2(%p, %i)", engine, id);
        CARLA_SAFE_ASSERT(fCustomURIDs.size() == kUridCount);

        carla_zeroPointers(fFeatures, kFeatureCountAll+1);
        carla_zeroPointers(fStateFeatures, kStateFeatureCountAll+1);
    }

    ~CarlaPluginLV2() override
    {
        carla_debug("CarlaPluginLV2::~CarlaPluginLV2()");

        fInlineDisplayNeedsRedraw = false;

        // close UI
        if (fUI.type != UI::TYPE_NULL)
        {
            showCustomUI(false);

#ifndef LV2_UIS_ONLY_INPROCESS
            if (fUI.type == UI::TYPE_BRIDGE)
            {
                fPipeServer.stopPipeServer(pData->engine->getOptions().uiBridgesTimeout);
            }
            else
#endif
            {
                if (fFeatures[kFeatureIdUiDataAccess] != nullptr && fFeatures[kFeatureIdUiDataAccess]->data != nullptr)
                    delete (LV2_Extension_Data_Feature*)fFeatures[kFeatureIdUiDataAccess]->data;

                if (fFeatures[kFeatureIdUiPortMap] != nullptr && fFeatures[kFeatureIdUiPortMap]->data != nullptr)
                    delete (LV2UI_Port_Map*)fFeatures[kFeatureIdUiPortMap]->data;

                if (fFeatures[kFeatureIdUiRequestValue] != nullptr && fFeatures[kFeatureIdUiRequestValue]->data != nullptr)
                    delete (LV2UI_Request_Value*)fFeatures[kFeatureIdUiRequestValue]->data;

                if (fFeatures[kFeatureIdUiResize] != nullptr && fFeatures[kFeatureIdUiResize]->data != nullptr)
                    delete (LV2UI_Resize*)fFeatures[kFeatureIdUiResize]->data;

                if (fFeatures[kFeatureIdUiTouch] != nullptr && fFeatures[kFeatureIdUiTouch]->data != nullptr)
                    delete (LV2UI_Touch*)fFeatures[kFeatureIdUiTouch]->data;

                if (fFeatures[kFeatureIdExternalUi] != nullptr && fFeatures[kFeatureIdExternalUi]->data != nullptr)
                    delete (LV2_External_UI_Host*)fFeatures[kFeatureIdExternalUi]->data;

                fUI.descriptor = nullptr;
                pData->uiLibClose();
            }

#ifndef LV2_UIS_ONLY_BRIDGES
            if (fUI.window != nullptr)
            {
                delete fUI.window;
                fUI.window = nullptr;
            }
#endif

            fUI.rdfDescriptor = nullptr;
        }

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fExt.state != nullptr)
        {
            const File tmpDir(handleStateMapToAbsolutePath(false, false, true, "."));

            if (tmpDir.exists())
                tmpDir.deleteRecursively();
        }

        if (fDescriptor != nullptr)
        {
            if (fDescriptor->cleanup != nullptr)
            {
                if (fHandle != nullptr)
                    fDescriptor->cleanup(fHandle);
                if (fHandle2 != nullptr)
                    fDescriptor->cleanup(fHandle2);
            }

            fHandle  = nullptr;
            fHandle2 = nullptr;
            fDescriptor = nullptr;
        }

        if (fRdfDescriptor != nullptr)
        {
            delete fRdfDescriptor;
            fRdfDescriptor = nullptr;
        }

        if (fFeatures[kFeatureIdEvent] != nullptr && fFeatures[kFeatureIdEvent]->data != nullptr)
            delete (LV2_Event_Feature*)fFeatures[kFeatureIdEvent]->data;

        if (fFeatures[kFeatureIdLogs] != nullptr && fFeatures[kFeatureIdLogs]->data != nullptr)
            delete (LV2_Log_Log*)fFeatures[kFeatureIdLogs]->data;

        if (fFeatures[kFeatureIdStateFreePath] != nullptr && fFeatures[kFeatureIdStateFreePath]->data != nullptr)
            delete (LV2_State_Free_Path*)fFeatures[kFeatureIdStateFreePath]->data;

        if (fFeatures[kFeatureIdStateMakePath] != nullptr && fFeatures[kFeatureIdStateMakePath]->data != nullptr)
            delete (LV2_State_Make_Path*)fFeatures[kFeatureIdStateMakePath]->data;

        if (fFeatures[kFeatureIdStateMapPath] != nullptr && fFeatures[kFeatureIdStateMapPath]->data != nullptr)
            delete (LV2_State_Map_Path*)fFeatures[kFeatureIdStateMapPath]->data;

        if (fFeatures[kFeatureIdPrograms] != nullptr && fFeatures[kFeatureIdPrograms]->data != nullptr)
            delete (LV2_Programs_Host*)fFeatures[kFeatureIdPrograms]->data;

        if (fFeatures[kFeatureIdResizePort] != nullptr && fFeatures[kFeatureIdResizePort]->data != nullptr)
            delete (LV2_Resize_Port_Resize*)fFeatures[kFeatureIdResizePort]->data;

        if (fFeatures[kFeatureIdRtMemPool] != nullptr && fFeatures[kFeatureIdRtMemPool]->data != nullptr)
            delete (LV2_RtMemPool_Pool*)fFeatures[kFeatureIdRtMemPool]->data;

        if (fFeatures[kFeatureIdRtMemPoolOld] != nullptr && fFeatures[kFeatureIdRtMemPoolOld]->data != nullptr)
            delete (LV2_RtMemPool_Pool_Deprecated*)fFeatures[kFeatureIdRtMemPoolOld]->data;

        if (fFeatures[kFeatureIdUriMap] != nullptr && fFeatures[kFeatureIdUriMap]->data != nullptr)
            delete (LV2_URI_Map_Feature*)fFeatures[kFeatureIdUriMap]->data;

        if (fFeatures[kFeatureIdUridMap] != nullptr && fFeatures[kFeatureIdUridMap]->data != nullptr)
            delete (LV2_URID_Map*)fFeatures[kFeatureIdUridMap]->data;

        if (fFeatures[kFeatureIdUridUnmap] != nullptr && fFeatures[kFeatureIdUridUnmap]->data != nullptr)
            delete (LV2_URID_Unmap*)fFeatures[kFeatureIdUridUnmap]->data;

        if (fFeatures[kFeatureIdWorker] != nullptr && fFeatures[kFeatureIdWorker]->data != nullptr)
            delete (LV2_Worker_Schedule*)fFeatures[kFeatureIdWorker]->data;

        if (fFeatures[kFeatureIdInlineDisplay] != nullptr && fFeatures[kFeatureIdInlineDisplay]->data != nullptr)
            delete (LV2_Inline_Display*)fFeatures[kFeatureIdInlineDisplay]->data;

        if (fFeatures[kFeatureIdMidnam] != nullptr && fFeatures[kFeatureIdMidnam]->data != nullptr)
            delete (LV2_Midnam*)fFeatures[kFeatureIdMidnam]->data;

        if (fFeatures[kFeatureIdCtrlInPortChangeReq] != nullptr && fFeatures[kFeatureIdCtrlInPortChangeReq]->data != nullptr)
            delete (LV2_ControlInputPort_Change_Request*)fFeatures[kFeatureIdCtrlInPortChangeReq]->data;

        for (uint32_t i=0; i < kFeatureCountAll; ++i)
        {
            if (fFeatures[i] != nullptr)
            {
                delete fFeatures[i];
                fFeatures[i] = nullptr;
            }
        }

        if (fStateFeatures[kStateFeatureIdMakePath] != nullptr && fStateFeatures[kStateFeatureIdMakePath]->data != nullptr)
            delete (LV2_State_Make_Path*)fStateFeatures[kStateFeatureIdMakePath]->data;

        if (fStateFeatures[kStateFeatureIdMapPath] != nullptr && fStateFeatures[kStateFeatureIdMapPath]->data != nullptr)
            delete (LV2_State_Map_Path*)fStateFeatures[kStateFeatureIdMapPath]->data;

        for (uint32_t i=0; i < kStateFeatureCountAll; ++i)
        {
            if (fStateFeatures[i] != nullptr)
            {
                delete fStateFeatures[i];
                fStateFeatures[i] = nullptr;
            }
        }

        if (fLastStateChunk != nullptr)
        {
            std::free(fLastStateChunk);
            fLastStateChunk = nullptr;
        }

        if (fAtomBufferUiOutTmpData != nullptr)
        {
            delete[] fAtomBufferUiOutTmpData;
            fAtomBufferUiOutTmpData = nullptr;
        }

        if (fAtomBufferWorkerInTmpData != nullptr)
        {
            delete[] fAtomBufferWorkerInTmpData;
            fAtomBufferWorkerInTmpData = nullptr;
        }

        if (fAtomBufferRealtime != nullptr)
        {
            std::free(fAtomBufferRealtime);
            fAtomBufferRealtime = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_LV2;
    }

    PluginCategory getCategory() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, CarlaPlugin::getCategory());

        const LV2_Property cat1(fRdfDescriptor->Type[0]);
        const LV2_Property cat2(fRdfDescriptor->Type[1]);

        if (LV2_IS_DELAY(cat1, cat2))
            return PLUGIN_CATEGORY_DELAY;
        if (LV2_IS_DISTORTION(cat1, cat2))
            return PLUGIN_CATEGORY_OTHER;
        if (LV2_IS_DYNAMICS(cat1, cat2))
            return PLUGIN_CATEGORY_DYNAMICS;
        if (LV2_IS_EQ(cat1, cat2))
            return PLUGIN_CATEGORY_EQ;
        if (LV2_IS_FILTER(cat1, cat2))
            return PLUGIN_CATEGORY_FILTER;
        if (LV2_IS_GENERATOR(cat1, cat2))
            return PLUGIN_CATEGORY_SYNTH;
        if (LV2_IS_MODULATOR(cat1, cat2))
            return PLUGIN_CATEGORY_MODULATOR;
        if (LV2_IS_REVERB(cat1, cat2))
            return PLUGIN_CATEGORY_DELAY;
        if (LV2_IS_SIMULATOR(cat1, cat2))
            return PLUGIN_CATEGORY_OTHER;
        if (LV2_IS_SPATIAL(cat1, cat2))
            return PLUGIN_CATEGORY_OTHER;
        if (LV2_IS_SPECTRAL(cat1, cat2))
            return PLUGIN_CATEGORY_UTILITY;
        if (LV2_IS_UTILITY(cat1, cat2))
            return PLUGIN_CATEGORY_UTILITY;

        return CarlaPlugin::getCategory();
    }

    uint32_t getLatencyInFrames() const noexcept override
    {
        if (fLatencyIndex < 0 || fParamBuffers == nullptr)
            return 0;

        const float latency(fParamBuffers[fLatencyIndex]);
        CARLA_SAFE_ASSERT_RETURN(latency >= 0.0f, 0);

        return static_cast<uint32_t>(latency);
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getMidiInCount() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, 0);

        uint32_t count = 0;

        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (LV2_IS_PORT_INPUT(portTypes) && LV2_PORT_SUPPORTS_MIDI_EVENT(portTypes))
                ++count;
        }

        return count;
    }

    uint32_t getMidiOutCount() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, 0);

        uint32_t count = 0;

        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (LV2_IS_PORT_OUTPUT(portTypes) && LV2_PORT_SUPPORTS_MIDI_EVENT(portTypes))
                ++count;
        }

        return count;
    }

    uint32_t getParameterScalePointCount(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);
            return port->ScalePointCount;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    uint getAudioPortHints(bool isOutput, uint32_t portIndex) const noexcept override
    {
        uint hints = 0x0;

        for (uint32_t i=0, j=0; i<fRdfDescriptor->PortCount; ++i)
        {
            LV2_RDF_Port& port(fRdfDescriptor->Ports[i]);

            if (! LV2_IS_PORT_AUDIO(port.Types))
                continue;

            if (isOutput)
            {
                if (! LV2_IS_PORT_OUTPUT(port.Types))
                    continue;
            }
            else
            {
                if (! LV2_IS_PORT_INPUT(port.Types))
                    continue;
            }

            if (j++ != portIndex)
                continue;

            if (LV2_IS_PORT_SIDECHAIN(port.Properties))
                hints |= AUDIO_PORT_IS_SIDECHAIN;

            break;
        }

        return hints;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        uint options = 0x0;

        // can't disable fixed buffers if using latency or MIDI output
        if (fLatencyIndex == -1 && getMidiOutCount() == 0 && ! fNeedsFixedBuffers)
            options |= PLUGIN_OPTION_FIXED_BUFFERS;

        // can't disable forced stereo if enabled in the engine
        if (pData->engine->getOptions().forceStereo)
            pass();
        // if there are event outputs, we can't force stereo
        else if (fEventsOut.count != 0)
            pass();
        // if inputs or outputs are just 1, then yes we can force stereo
        else if ((pData->audioIn.count == 1 || pData->audioOut.count == 1) || fHandle2 != nullptr)
            options |= PLUGIN_OPTION_FORCE_STEREO;

        if (fExt.programs != nullptr)
            options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (getMidiInCount() != 0)
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;
            options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        if (pData->param.data[parameterId].type == PARAMETER_INPUT)
        {
            if (pData->param.data[parameterId].hints & PARAMETER_IS_STRICT_BOUNDS)
                pData->param.ranges[parameterId].fixValue(fParamBuffers[parameterId]);
        }
        else
        {
            if (fStrictBounds >= 0 && (pData->param.data[parameterId].hints & PARAMETER_IS_STRICT_BOUNDS) == 0)
                pData->param.ranges[parameterId].fixValue(fParamBuffers[parameterId]);
        }

        return fParamBuffers[parameterId];
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);
            CARLA_SAFE_ASSERT_RETURN(scalePointId < port->ScalePointCount, 0.0f);

            const LV2_RDF_PortScalePoint* const portScalePoint(&port->ScalePoints[scalePointId]);
            return portScalePoint->Value;
        }

        return 0.0f;
    }

    bool getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor->URI != nullptr, false);

        std::strncpy(strBuf, fRdfDescriptor->URI, STR_MAX);
        return true;
    }

    bool getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);

        if (fRdfDescriptor->Author != nullptr)
        {
            std::strncpy(strBuf, fRdfDescriptor->Author, STR_MAX);
            return true;
        }

        return false;
    }

    bool getCopyright(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);

        if (fRdfDescriptor->License != nullptr)
        {
            std::strncpy(strBuf, fRdfDescriptor->License, STR_MAX);
            return true;
        }

        return false;
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);

        if (fRdfDescriptor->Name != nullptr)
        {
            std::strncpy(strBuf, fRdfDescriptor->Name, STR_MAX);
            return true;
        }

        return false;
    }

    bool getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        int32_t rindex = pData->param.data[parameterId].rindex;
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0, false);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            std::strncpy(strBuf, fRdfDescriptor->Ports[rindex].Name, STR_MAX);
            return true;
        }

        rindex -= static_cast<int32_t>(fRdfDescriptor->PortCount);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->ParameterCount))
        {
            std::strncpy(strBuf, fRdfDescriptor->Parameters[rindex].Label, STR_MAX);
            return true;
        }

        return CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    bool getParameterSymbol(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        int32_t rindex = pData->param.data[parameterId].rindex;
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0, false);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            std::strncpy(strBuf, fRdfDescriptor->Ports[rindex].Symbol, STR_MAX);
            return true;
        }

        rindex -= static_cast<int32_t>(fRdfDescriptor->PortCount);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->ParameterCount))
        {
            std::strncpy(strBuf, fRdfDescriptor->Parameters[rindex].URI, STR_MAX);
            return true;
        }

        return CarlaPlugin::getParameterSymbol(parameterId, strBuf);
    }

    bool getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        LV2_RDF_PortUnit* portUnit = nullptr;

        int32_t rindex = pData->param.data[parameterId].rindex;
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0, false);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            portUnit = &fRdfDescriptor->Ports[rindex].Unit;
        }
        else
        {
            rindex -= static_cast<int32_t>(fRdfDescriptor->PortCount);

            if (rindex < static_cast<int32_t>(fRdfDescriptor->ParameterCount))
            {
                portUnit = &fRdfDescriptor->Parameters[rindex].Unit;
            }
        }

        if (portUnit != nullptr)
        {
            if (LV2_HAVE_PORT_UNIT_SYMBOL(portUnit->Hints) && portUnit->Symbol != nullptr)
            {
                std::strncpy(strBuf, portUnit->Symbol, STR_MAX);
                return true;
            }

            if (LV2_HAVE_PORT_UNIT_UNIT(portUnit->Hints))
            {
                switch (portUnit->Unit)
                {
                case LV2_PORT_UNIT_BAR:
                    std::strncpy(strBuf, "bars", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_BEAT:
                    std::strncpy(strBuf, "beats", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_BPM:
                    std::strncpy(strBuf, "BPM", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_CENT:
                    std::strncpy(strBuf, "ct", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_CM:
                    std::strncpy(strBuf, "cm", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_COEF:
                    std::strncpy(strBuf, "(coef)", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_DB:
                    std::strncpy(strBuf, "dB", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_DEGREE:
                    std::strncpy(strBuf, "deg", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_FRAME:
                    std::strncpy(strBuf, "frames", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_HZ:
                    std::strncpy(strBuf, "Hz", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_INCH:
                    std::strncpy(strBuf, "in", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_KHZ:
                    std::strncpy(strBuf, "kHz", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_KM:
                    std::strncpy(strBuf, "km", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_M:
                    std::strncpy(strBuf, "m", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_MHZ:
                    std::strncpy(strBuf, "MHz", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_MIDINOTE:
                    std::strncpy(strBuf, "note", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_MILE:
                    std::strncpy(strBuf, "mi", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_MIN:
                    std::strncpy(strBuf, "min", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_MM:
                    std::strncpy(strBuf, "mm", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_MS:
                    std::strncpy(strBuf, "ms", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_OCT:
                    std::strncpy(strBuf, "oct", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_PC:
                    std::strncpy(strBuf, "%", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_S:
                    std::strncpy(strBuf, "s", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_SEMITONE:
                    std::strncpy(strBuf, "semi", STR_MAX);
                    return true;
                case LV2_PORT_UNIT_VOLTS:
                    std::strncpy(strBuf, "v", STR_MAX);
                    return true;
                }
            }
        }

        return CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    bool getParameterComment(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        int32_t rindex = pData->param.data[parameterId].rindex;
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0, false);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            if (const char* const comment = fRdfDescriptor->Ports[rindex].Comment)
            {
                std::strncpy(strBuf, comment, STR_MAX);
                return true;
            }
            return false;
        }

        rindex -= static_cast<int32_t>(fRdfDescriptor->PortCount);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->ParameterCount))
        {
            if (const char* const comment = fRdfDescriptor->Parameters[rindex].Comment)
            {
                std::strncpy(strBuf, comment, STR_MAX);
                return true;
            }
            return false;
        }

        return CarlaPlugin::getParameterComment(parameterId, strBuf);
    }

    bool getParameterGroupName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        int32_t rindex = pData->param.data[parameterId].rindex;
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0, false);

        const char* uri = nullptr;

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            uri = fRdfDescriptor->Ports[rindex].GroupURI;
        }
        else
        {
            rindex -= static_cast<int32_t>(fRdfDescriptor->PortCount);

            if (rindex < static_cast<int32_t>(fRdfDescriptor->ParameterCount))
                uri = fRdfDescriptor->Parameters[rindex].GroupURI;
        }

        if (uri == nullptr)
            return false;

        for (uint32_t i=0; i<fRdfDescriptor->PortGroupCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->PortGroups[i].URI, uri) == 0)
            {
                const char* const name   = fRdfDescriptor->PortGroups[i].Name;
                const char* const symbol = fRdfDescriptor->PortGroups[i].Symbol;

                if (name != nullptr && symbol != nullptr)
                {
                    std::snprintf(strBuf, STR_MAX, "%s:%s", symbol, name);
                    return true;
                }
                return false;
            }
        }

        return false;
    }

    bool getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0, false);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);
            CARLA_SAFE_ASSERT_RETURN(scalePointId < port->ScalePointCount, false);

            const LV2_RDF_PortScalePoint* const portScalePoint(&port->ScalePoints[scalePointId]);

            if (portScalePoint->Label != nullptr)
            {
                std::strncpy(strBuf, portScalePoint->Label, STR_MAX);
                return true;
            }
        }

        return CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    void prepareForSave(const bool temporary) override
    {
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fExt.state != nullptr && fExt.state->save != nullptr)
        {
            // move temporary stuff to main state dir on full save
            if (! temporary)
            {
                const File tmpDir(handleStateMapToAbsolutePath(false, false, true, "."));

                if (tmpDir.exists())
                {
                    const File stateDir(handleStateMapToAbsolutePath(true, false, false, "."));

                    if (stateDir.isNotNull())
                        tmpDir.moveFileTo(stateDir);
                }
            }

            fExt.state->save(fHandle, carla_lv2_state_store, this, LV2_STATE_IS_POD, fStateFeatures);

            if (fHandle2 != nullptr)
                fExt.state->save(fHandle2, carla_lv2_state_store, this, LV2_STATE_IS_POD, fStateFeatures);
        }
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setName(const char* const newName) override
    {
        const File tmpDir1(handleStateMapToAbsolutePath(false, false, true, "."));

        CarlaPlugin::setName(newName);

        if (tmpDir1.exists())
        {
            const File tmpDir2(handleStateMapToAbsolutePath(false, false, true, "."));

            carla_stdout("dir1 %s, dir2 %s",
                         tmpDir1.getFullPathName().toRawUTF8(),
                         tmpDir2.getFullPathName().toRawUTF8());

            if (tmpDir2.isNotNull())
            {
                if (tmpDir2.exists())
                    tmpDir2.deleteRecursively();

                tmpDir1.moveFileTo(tmpDir2);
            }
        }

        if (fLv2Options.windowTitle != nullptr && pData->uiTitle.isEmpty())
            setWindowTitle(nullptr);
    }

    void setWindowTitle(const char* const title) noexcept
    {
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

        std::free(const_cast<char*>(fLv2Options.windowTitle));
        fLv2Options.windowTitle = uiTitle.getAndReleaseBuffer();

        fLv2Options.opts[CarlaPluginLV2Options::WindowTitle].size  = (uint32_t)std::strlen(fLv2Options.windowTitle);
        fLv2Options.opts[CarlaPluginLV2Options::WindowTitle].value = fLv2Options.windowTitle;

        if (fFeatures[kFeatureIdExternalUi] != nullptr && fFeatures[kFeatureIdExternalUi]->data != nullptr)
            ((LV2_External_UI_Host*)fFeatures[kFeatureIdExternalUi]->data)->plugin_human_id = fLv2Options.windowTitle;

#ifndef LV2_UIS_ONLY_INPROCESS
        if (fPipeServer.isPipeRunning())
            fPipeServer.writeUiTitleMessage(fLv2Options.windowTitle);
#endif

#ifndef LV2_UIS_ONLY_BRIDGES
        if (fUI.window != nullptr)
        {
            try {
                fUI.window->setTitle(fLv2Options.windowTitle);
            } CARLA_SAFE_EXCEPTION("set custom title");
        }
#endif
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    float setParamterValueCommon(const uint32_t parameterId, const float value) noexcept
    {
        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fParamBuffers[parameterId] = fixedValue;

        if (pData->param.data[parameterId].rindex >= static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const uint32_t rparamId = static_cast<uint32_t>(pData->param.data[parameterId].rindex) - fRdfDescriptor->PortCount;
            CARLA_SAFE_ASSERT_UINT2_RETURN(rparamId < fRdfDescriptor->ParameterCount,
                                           rparamId, fRdfDescriptor->PortCount, fixedValue);

            uint8_t atomBuf[256];
            LV2_Atom_Forge atomForge;
            initAtomForge(atomForge);
            lv2_atom_forge_set_buffer(&atomForge, atomBuf, sizeof(atomBuf));

            LV2_Atom_Forge_Frame forgeFrame;
            lv2_atom_forge_object(&atomForge, &forgeFrame, kUridNull, kUridPatchSet);

            lv2_atom_forge_key(&atomForge, kUridCarlaParameterChange);
            lv2_atom_forge_bool(&atomForge, true);

            lv2_atom_forge_key(&atomForge, kUridPatchProperty);
            lv2_atom_forge_urid(&atomForge, getCustomURID(fRdfDescriptor->Parameters[rparamId].URI));

            lv2_atom_forge_key(&atomForge, kUridPatchValue);

            switch (fRdfDescriptor->Parameters[rparamId].Type)
            {
            case LV2_PARAMETER_TYPE_BOOL:
                lv2_atom_forge_bool(&atomForge, fixedValue > 0.5f);
                break;
            case LV2_PARAMETER_TYPE_INT:
                lv2_atom_forge_int(&atomForge, static_cast<int32_t>(fixedValue + 0.5f));
                break;
            case LV2_PARAMETER_TYPE_LONG:
                lv2_atom_forge_long(&atomForge, static_cast<int64_t>(fixedValue + 0.5f));
                break;
            case LV2_PARAMETER_TYPE_FLOAT:
                lv2_atom_forge_float(&atomForge, fixedValue);
                break;
            case LV2_PARAMETER_TYPE_DOUBLE:
                lv2_atom_forge_double(&atomForge, fixedValue);
                break;
            default:
                carla_stderr2("setParameterValue called for invalid parameter, expect issues!");
                break;
            }

            lv2_atom_forge_pop(&atomForge, &forgeFrame);

            LV2_Atom* const atom((LV2_Atom*)atomBuf);
            CARLA_SAFE_ASSERT(atom->size < sizeof(atomBuf));

            fAtomBufferEvIn.put(atom, fEventsIn.ctrlIndex);
        }

        return fixedValue;
    }

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue = setParamterValueCommon(parameterId, value);

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setParameterValueRT(const uint32_t parameterId, const float value, const uint32_t frameOffset, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue = setParamterValueCommon(parameterId, value);

        CarlaPlugin::setParameterValueRT(parameterId, fixedValue, frameOffset, sendCallbackLater);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
        carla_debug("CarlaPluginLV2::setCustomData(\"%s\", \"%s\", \"%s\", %s)", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_TYPE_PATH) == 0)
        {
            if (std::strcmp(key, "file") != 0)
                return;

            CARLA_SAFE_ASSERT_RETURN(fFilePathURI.isNotEmpty(),);
            CARLA_SAFE_ASSERT_RETURN(value[0] != '\0',);

            carla_stdout("LV2 file path to send: '%s'", value);
            writeAtomPath(value, getCustomURID(fFilePathURI));
            return;
        }

        if (std::strcmp(type, CUSTOM_DATA_TYPE_PROPERTY) == 0)
            return CarlaPlugin::setCustomData(type, key, value, sendGui);

        // See if this key is from a parameter exposed by carla, apply value if yes
        for (uint32_t i=0; i < fRdfDescriptor->ParameterCount; ++i)
        {
            const LV2_RDF_Parameter& rdfParam(fRdfDescriptor->Parameters[i]);

            if (std::strcmp(rdfParam.URI, key) == 0)
            {
                uint32_t parameterId = UINT32_MAX;
                const int32_t rindex = static_cast<int32_t>(fRdfDescriptor->PortCount + i);

                switch (rdfParam.Type)
                {
                case LV2_PARAMETER_TYPE_BOOL:
                case LV2_PARAMETER_TYPE_INT:
                // case LV2_PARAMETER_TYPE_LONG:
                case LV2_PARAMETER_TYPE_FLOAT:
                case LV2_PARAMETER_TYPE_DOUBLE:
                    for (uint32_t j=0; j < pData->param.count; ++j)
                    {
                        if (pData->param.data[j].rindex == rindex)
                        {
                            parameterId = j;
                            break;
                        }
                    }
                    break;
                }

                if (parameterId == UINT32_MAX)
                    break;

                std::vector<uint8_t> chunk;
                d_getChunkFromBase64String_impl(chunk, value);
                CARLA_SAFE_ASSERT_RETURN(chunk.size() > 0,);

#ifdef CARLA_PROPER_CPP11_SUPPORT
                const uint8_t* const valueptr = chunk.data();
#else
                const uint8_t* const valueptr = &chunk.front();
#endif
                float rvalue;

                switch (rdfParam.Type)
                {
                case LV2_PARAMETER_TYPE_BOOL:
                    rvalue = *(const int32_t*)valueptr != 0 ? 1.0f : 0.0f;
                    break;
                case LV2_PARAMETER_TYPE_INT:
                    rvalue = static_cast<float>(*(const int32_t*)valueptr);
                    break;
                case LV2_PARAMETER_TYPE_FLOAT:
                    rvalue = *(const float*)valueptr;
                    break;
                case LV2_PARAMETER_TYPE_DOUBLE:
                    rvalue = static_cast<float>(*(const double*)valueptr);
                    break;
                default:
                    // making compilers happy
                    rvalue = pData->param.ranges[parameterId].def;
                    break;
                }

                fParamBuffers[parameterId] = pData->param.getFixedValue(parameterId, rvalue);
                break;
            }
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool doingInit) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->prog.count),);
        CARLA_SAFE_ASSERT_RETURN(sendGui || sendOsc || sendCallback,);

        if (index >= 0 && index < static_cast<int32_t>(fRdfDescriptor->PresetCount))
        {
            const LV2_URID_Map* const uridMap = (const LV2_URID_Map*)fFeatures[kFeatureIdUridMap]->data;

            LilvState* const state = Lv2WorldClass::getInstance().getStateFromURI(fRdfDescriptor->Presets[index].URI,
                                                                                  uridMap);
            CARLA_SAFE_ASSERT_RETURN(state != nullptr,);

            // invalidate midi-program selection
            CarlaPlugin::setMidiProgram(-1, false, false, sendCallback, false);

            if (fExt.state != nullptr)
            {
                const bool block = (sendGui || sendOsc || sendCallback) && !fHasThreadSafeRestore;
                const ScopedSingleProcessLocker spl(this, block);

                lilv_state_restore(state, fExt.state, fHandle, carla_lilv_set_port_value, this, 0, fFeatures);

                if (fHandle2 != nullptr)
                    lilv_state_restore(state, fExt.state, fHandle2, carla_lilv_set_port_value, this, 0, fFeatures);
            }
            else
            {
                lilv_state_emit_port_values(state, carla_lilv_set_port_value, this);
            }

            lilv_state_free(state);
        }

        CarlaPlugin::setProgram(index, sendGui, sendOsc, sendCallback, doingInit);
    }

    void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool doingInit) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);
        CARLA_SAFE_ASSERT_RETURN(sendGui || sendOsc || sendCallback || doingInit,);

        if (index >= 0 && fExt.programs != nullptr && fExt.programs->select_program != nullptr)
        {
            const uint32_t bank(pData->midiprog.data[index].bank);
            const uint32_t program(pData->midiprog.data[index].program);

            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            try {
                fExt.programs->select_program(fHandle, bank, program);
            } CARLA_SAFE_EXCEPTION("select program");

            if (fHandle2 != nullptr)
            {
                try {
                    fExt.programs->select_program(fHandle2, bank, program);
                } CARLA_SAFE_EXCEPTION("select program 2");
            }
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, doingInit);
    }

    void setMidiProgramRT(const uint32_t uindex, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(uindex < pData->midiprog.count,);

        if (fExt.programs != nullptr && fExt.programs->select_program != nullptr)
        {
            const uint32_t bank(pData->midiprog.data[uindex].bank);
            const uint32_t program(pData->midiprog.data[uindex].program);

            try {
                fExt.programs->select_program(fHandle, bank, program);
            } CARLA_SAFE_EXCEPTION("select program RT");

            if (fHandle2 != nullptr)
            {
                try {
                    fExt.programs->select_program(fHandle2, bank, program);
                } CARLA_SAFE_EXCEPTION("select program RT 2");
            }
        }

        CarlaPlugin::setMidiProgramRT(uindex, sendCallbackLater);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void setCustomUITitle(const char* const title) noexcept override
    {
        setWindowTitle(title);
        CarlaPlugin::setCustomUITitle(title);
    }

    void showCustomUI(const bool yesNo) override
    {
        if (fUI.type == UI::TYPE_NULL)
        {
            if (yesNo && fFilePathURI.isNotEmpty())
            {
                const char* const path = pData->engine->runFileCallback(FILE_CALLBACK_OPEN, false, "Open File", "");

                if (path != nullptr && path[0] != '\0')
                {
                    carla_stdout("LV2 file path to send: '%s'", path);
                    writeAtomPath(path, getCustomURID(fFilePathURI));
                }
            }
            else
            {
                CARLA_SAFE_ASSERT(!yesNo);
            }
            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0, 0.0f, nullptr);
            return;
        }

        const uintptr_t frontendWinId = pData->engine->getOptions().frontendWinId;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (! yesNo)
            pData->transientTryCounter = 0;
#endif

#ifndef LV2_UIS_ONLY_INPROCESS
        if (fUI.type == UI::TYPE_BRIDGE)
        {
            if (yesNo)
            {
                if (fPipeServer.isPipeRunning())
                {
                    fPipeServer.writeFocusMessage();
                    return;
                }

                if (! fPipeServer.startPipeServer(std::min(fLv2Options.sequenceSize, 819200)))
                {
                    pData->engine->callback(true, true,
                                            ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0, 0.0f, nullptr);
                    return;
                }

                // manually write messages so we can take the lock for ourselves
                {
                    char tmpBuf[0xff];
                    tmpBuf[0xfe] = '\0';

                    const CarlaMutexLocker cml(fPipeServer.getPipeLock());
                    const ScopedSafeLocale ssl;

                    // write URI mappings
                    uint32_t u = 0;
                    for (std::vector<std::string>::iterator it=fCustomURIDs.begin(), end=fCustomURIDs.end(); it != end; ++it, ++u)
                    {
                        if (u < kUridCount)
                            continue;
                        const std::string& uri(*it);

                        if (! fPipeServer.writeMessage("urid\n", 5))
                            return;

                        std::snprintf(tmpBuf, 0xfe, "%u\n", u);
                        if (! fPipeServer.writeMessage(tmpBuf))
                            return;

                        std::snprintf(tmpBuf, 0xfe, "%lu\n", static_cast<long unsigned>(uri.length()));
                        if (! fPipeServer.writeMessage(tmpBuf))
                            return;

                        if (! fPipeServer.writeAndFixMessage(uri.c_str()))
                            return;
                    }

                    // write UI options
                    if (! fPipeServer.writeMessage("uiOptions\n", 10))
                        return;

                    const EngineOptions& opts(pData->engine->getOptions());

                    std::snprintf(tmpBuf, 0xff, "%g\n", pData->engine->getSampleRate());
                    if (! fPipeServer.writeMessage(tmpBuf))
                        return;

                    std::snprintf(tmpBuf, 0xff, "%u\n", opts.bgColor);
                    if (! fPipeServer.writeMessage(tmpBuf))
                        return;

                    std::snprintf(tmpBuf, 0xff, "%u\n", opts.fgColor);
                    if (! fPipeServer.writeMessage(tmpBuf))
                        return;

                    std::snprintf(tmpBuf, 0xff, "%.12g\n", static_cast<double>(opts.uiScale));
                    if (! fPipeServer.writeMessage(tmpBuf))
                        return;

                    std::snprintf(tmpBuf, 0xff, "%s\n", bool2str(true)); // useTheme
                    if (! fPipeServer.writeMessage(tmpBuf))
                        return;

                    std::snprintf(tmpBuf, 0xff, "%s\n", bool2str(true)); // useThemeColors
                    if (! fPipeServer.writeMessage(tmpBuf))
                        return;

                    if (! fPipeServer.writeAndFixMessage(fLv2Options.windowTitle != nullptr
                                                         ? fLv2Options.windowTitle
                                                         : ""))
                        return;

                    std::snprintf(tmpBuf, 0xff, P_INTPTR "\n", frontendWinId);
                    if (! fPipeServer.writeMessage(tmpBuf))
                        return;

                    // write parameter values
                    for (uint32_t i=0; i < pData->param.count; ++i)
                    {
                        ParameterData& pdata(pData->param.data[i]);

                        if (pdata.hints & PARAMETER_IS_NOT_SAVED)
                        {
                            int32_t rindex = pdata.rindex;
                            CARLA_SAFE_ASSERT_CONTINUE(rindex - static_cast<int32_t>(fRdfDescriptor->PortCount) >= 0);

                            rindex -= static_cast<int32_t>(fRdfDescriptor->PortCount);
                            CARLA_SAFE_ASSERT_CONTINUE(rindex < static_cast<int32_t>(fRdfDescriptor->ParameterCount));

                            if (! fPipeServer.writeLv2ParameterMessage(fRdfDescriptor->Parameters[rindex].URI,
                                                                       getParameterValue(i), false))
                                return;
                        }
                        else
                        {
                            if (! fPipeServer.writeControlMessage(static_cast<uint32_t>(pData->param.data[i].rindex),
                                                                  getParameterValue(i), false))
                                return;
                        }
                    }

                    // ready to show
                    if (! fPipeServer.writeMessage("show\n", 5))
                        return;

                    fPipeServer.syncMessages();
                }
            }
            else
            {
                fPipeServer.stopPipeServer(pData->engine->getOptions().uiBridgesTimeout);
            }
            return;
        }
#endif

        // take some precautions
        CARLA_SAFE_ASSERT_RETURN(fUI.descriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fUI.rdfDescriptor != nullptr,);

        if (yesNo)
        {
            CARLA_SAFE_ASSERT_RETURN(fUI.descriptor->instantiate != nullptr,);
            CARLA_SAFE_ASSERT_RETURN(fUI.descriptor->cleanup != nullptr,);
        }
        else
        {
            if (fUI.handle == nullptr)
                return;
        }

        if (yesNo)
        {
            if (fUI.handle == nullptr)
            {
#ifndef LV2_UIS_ONLY_BRIDGES
                if (fUI.type == UI::TYPE_EMBED && fUI.rdfDescriptor->Type != LV2_UI_NONE && fUI.window == nullptr)
                {
                    const char* msg = nullptr;
                    const bool isStandalone = pData->engine->getOptions().pluginsAreStandalone;

                    switch (fUI.rdfDescriptor->Type)
                    {
                    case LV2_UI_GTK2:
                    case LV2_UI_GTK3:
                    case LV2_UI_QT4:
                    case LV2_UI_QT5:
                    case LV2_UI_EXTERNAL:
                    case LV2_UI_OLD_EXTERNAL:
                        msg = "Invalid UI type";
                        break;

                    case LV2_UI_COCOA:
# ifdef CARLA_OS_MAC
                        fUI.window = CarlaPluginUI::newCocoa(this, frontendWinId, isStandalone, isUiResizable());
# else
                        msg = "UI is for MacOS only";
# endif
                        break;

                    case LV2_UI_WINDOWS:
# ifdef CARLA_OS_WIN
                        fUI.window = CarlaPluginUI::newWindows(this, frontendWinId, isStandalone, isUiResizable());
# else
                        msg = "UI is for Windows only";
# endif
                        break;

                    case LV2_UI_X11:
# ifdef HAVE_X11
                        fUI.window = CarlaPluginUI::newX11(this, frontendWinId, isStandalone, isUiResizable(), true);
# else
                        msg = "UI is only for systems with X11";
# endif
                        break;

                    default:
                        msg = "Unknown UI type";
                        break;
                    }

                    if (fUI.window == nullptr && fExt.uishow == nullptr)
                        return pData->engine->callback(true, true,
                                                       ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, -1, 0, 0, 0.0f, msg);

                    if (fUI.window != nullptr)
                        fFeatures[kFeatureIdUiParent]->data = fUI.window->getPtr();
                }
#endif
                fUI.widget = nullptr;
                fUI.handle = fUI.descriptor->instantiate(fUI.descriptor, fRdfDescriptor->URI, fUI.rdfDescriptor->Bundle,
                                                         carla_lv2_ui_write_function, this, &fUI.widget, fFeatures);

                if (fUI.window != nullptr)
                {
                    if (fUI.widget != nullptr)
                        fUI.window->setChildWindow(fUI.widget);
                    fUI.window->setTitle(fLv2Options.windowTitle);
                }
            }

            CARLA_SAFE_ASSERT(fUI.handle != nullptr);
            CARLA_SAFE_ASSERT(fUI.type != UI::TYPE_EXTERNAL || fUI.widget != nullptr);

            if (fUI.handle == nullptr || (fUI.type == UI::TYPE_EXTERNAL && fUI.widget == nullptr))
            {
                fUI.widget = nullptr;

                if (fUI.handle != nullptr)
                {
                    fUI.descriptor->cleanup(fUI.handle);
                    fUI.handle = nullptr;
                }

                return pData->engine->callback(true, true,
                                               ENGINE_CALLBACK_UI_STATE_CHANGED,
                                               pData->id,
                                               -1,
                                               0, 0, 0.0f,
                                               "Plugin refused to open its own UI");
            }

            updateUi();

#ifndef LV2_UIS_ONLY_BRIDGES
            if (fUI.type == UI::TYPE_EMBED)
            {
                if (fUI.window != nullptr)
                {
                    fUI.window->show();
                }
                else if (fExt.uishow != nullptr)
                {
                    fExt.uishow->show(fUI.handle);
# ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                    pData->tryTransient();
# endif
                }
            }
            else
#endif
            {
                LV2_EXTERNAL_UI_SHOW((LV2_External_UI_Widget*)fUI.widget);
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                pData->tryTransient();
#endif
            }
        }
        else
        {
#ifndef LV2_UIS_ONLY_BRIDGES
            if (fUI.type == UI::TYPE_EMBED)
            {
                if (fUI.window != nullptr)
                    fUI.window->hide();
                else if (fExt.uishow != nullptr)
                    fExt.uishow->hide(fUI.handle);
            }
            else
#endif
            {
                CARLA_SAFE_ASSERT(fUI.widget != nullptr);

                if (fUI.widget != nullptr)
                    LV2_EXTERNAL_UI_HIDE((LV2_External_UI_Widget*)fUI.widget);
            }

            fUI.descriptor->cleanup(fUI.handle);
            fUI.handle = nullptr;
            fUI.widget = nullptr;

            if (fUI.type == UI::TYPE_EMBED && fUI.window != nullptr)
            {
                delete fUI.window;
                fUI.window = nullptr;
            }
        }
    }

#ifndef LV2_UIS_ONLY_BRIDGES
    void* embedCustomUI(void* const ptr) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type == UI::TYPE_EMBED, nullptr);
        CARLA_SAFE_ASSERT_RETURN(fUI.descriptor != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(fUI.descriptor->instantiate != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(fUI.descriptor->cleanup != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(fUI.rdfDescriptor->Type != LV2_UI_NONE, nullptr);
        CARLA_SAFE_ASSERT_RETURN(fUI.window == nullptr, nullptr);

        fFeatures[kFeatureIdUiParent]->data = ptr;

        fUI.embedded = true;
        fUI.widget = nullptr;
        fUI.handle = fUI.descriptor->instantiate(fUI.descriptor, fRdfDescriptor->URI, fUI.rdfDescriptor->Bundle,
                                                 carla_lv2_ui_write_function, this, &fUI.widget, fFeatures);

        updateUi();

        return fUI.widget;
    }
#endif

    void idle() override
    {
        if (fAtomBufferWorkerIn.isDataAvailableForReading())
        {
            Lv2AtomRingBuffer tmpRingBuffer(fAtomBufferWorkerIn, fAtomBufferWorkerInTmpData);
            CARLA_SAFE_ASSERT_RETURN(tmpRingBuffer.isDataAvailableForReading(),);
            CARLA_SAFE_ASSERT_RETURN(fExt.worker != nullptr && fExt.worker->work != nullptr,);

            const size_t localSize = fAtomBufferWorkerIn.getSize();
            uint8_t* const localData = new uint8_t[localSize];
            LV2_Atom* const localAtom = static_cast<LV2_Atom*>(static_cast<void*>(localData));
            localAtom->size = localSize;
            uint32_t portIndex;

            for (; tmpRingBuffer.get(portIndex, localAtom); localAtom->size = localSize)
            {
                CARLA_SAFE_ASSERT_CONTINUE(localAtom->type == kUridCarlaAtomWorkerIn);
                fExt.worker->work(fHandle, carla_lv2_worker_respond, this, localAtom->size, LV2_ATOM_BODY_CONST(localAtom));
            }

            delete[] localData;
        }

        if (fInlineDisplayNeedsRedraw)
        {
            // TESTING
            CARLA_SAFE_ASSERT(pData->enabled)
            CARLA_SAFE_ASSERT(!pData->engine->isAboutToClose());
            CARLA_SAFE_ASSERT(pData->client->isActive());

            if (pData->enabled && !pData->engine->isAboutToClose() && pData->client->isActive())
            {
                const int64_t timeNow = water::Time::currentTimeMillis();

                if (timeNow - fInlineDisplayLastRedrawTime > (1000 / 30))
                {
                    fInlineDisplayNeedsRedraw = false;
                    fInlineDisplayLastRedrawTime = timeNow;
                    pData->engine->callback(true, true,
                                            ENGINE_CALLBACK_INLINE_DISPLAY_REDRAW,
                                            pData->id,
                                            0, 0, 0, 0.0f, nullptr);
                }
            }
            else
            {
                fInlineDisplayNeedsRedraw = false;
            }
        }

        CarlaPlugin::idle();
    }

    void uiIdle() override
    {
        if (const char* const fileNeededForURI = fUI.fileNeededForURI)
        {
            fUI.fileBrowserOpen = true;
            fUI.fileNeededForURI = nullptr;

            const char* const path = pData->engine->runFileCallback(FILE_CALLBACK_OPEN,
                                                                    /* isDir   */ false,
                                                                    /* title   */ "File open",
                                                                    /* filters */ "");

            fUI.fileBrowserOpen = false;

            if (path != nullptr)
            {
                carla_stdout("LV2 requested path to send: '%s'", path);
                writeAtomPath(path, getCustomURID(fileNeededForURI));
            }

            // this function will be called recursively, stop here
            return;
        }

        if (fAtomBufferUiOut.isDataAvailableForReading())
        {
            Lv2AtomRingBuffer tmpRingBuffer(fAtomBufferUiOut, fAtomBufferUiOutTmpData);
            CARLA_SAFE_ASSERT(tmpRingBuffer.isDataAvailableForReading());

            const size_t localSize = fAtomBufferUiOut.getSize();
            uint8_t* const localData = new uint8_t[localSize];
            LV2_Atom* const localAtom = static_cast<LV2_Atom*>(static_cast<void*>(localData));
            localAtom->size = localSize;

            uint32_t portIndex;
            const bool hasPortEvent(fUI.handle != nullptr &&
                                    fUI.descriptor != nullptr &&
                                    fUI.descriptor->port_event != nullptr);

            for (; tmpRingBuffer.get(portIndex, localAtom); localAtom->size = localSize)
            {
#ifndef LV2_UIS_ONLY_INPROCESS
                if (fUI.type == UI::TYPE_BRIDGE)
                {
                    if (fPipeServer.isPipeRunning())
                        fPipeServer.writeLv2AtomMessage(portIndex, localAtom);
                }
                else
#endif
                {
                    if (hasPortEvent && ! fNeedsUiClose)
                        fUI.descriptor->port_event(fUI.handle, portIndex, lv2_atom_total_size(localAtom), kUridAtomTransferEvent, localAtom);
                }

                inspectAtomForParameterChange(localAtom);
            }

            delete[] localData;
        }

#ifndef LV2_UIS_ONLY_INPROCESS
        if (fPipeServer.isPipeRunning())
        {
            fPipeServer.idlePipe();

            switch (fPipeServer.getAndResetUiState())
            {
            case CarlaPipeServerLV2::UiNone:
            case CarlaPipeServerLV2::UiShow:
                break;
            case CarlaPipeServerLV2::UiHide:
                fPipeServer.stopPipeServer(2000);
                // fall through
            case CarlaPipeServerLV2::UiCrashed:
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                pData->transientTryCounter = 0;
#endif
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_UI_STATE_CHANGED,
                                        pData->id,
                                        0,
                                        0, 0, 0.0f, nullptr);
                break;
            }
        }
        else
        {
            // TODO - detect if ui-bridge crashed
        }
#endif

        if (fNeedsUiClose)
        {
            fNeedsUiClose = false;
            showCustomUI(false);
            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_UI_STATE_CHANGED,
                                    pData->id,
                                    0,
                                    0, 0, 0.0f, nullptr);
        }
        else if (fUI.handle != nullptr && fUI.descriptor != nullptr)
        {
            if (fUI.type == UI::TYPE_EXTERNAL && fUI.widget != nullptr)
                LV2_EXTERNAL_UI_RUN((LV2_External_UI_Widget*)fUI.widget);
#ifndef LV2_UIS_ONLY_BRIDGES
            else if (fUI.type == UI::TYPE_EMBED && fUI.window != nullptr)
                fUI.window->idle();

            // note: UI might have been closed by window idle
            if (fNeedsUiClose)
            {
                pass();
            }
            else if (fUI.handle != nullptr && fExt.uiidle != nullptr && fExt.uiidle->idle(fUI.handle) != 0)
            {
                showCustomUI(false);
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_UI_STATE_CHANGED,
                                        pData->id,
                                        0,
                                        0, 0, 0.0f, nullptr);
                CARLA_SAFE_ASSERT(fUI.handle == nullptr);
            }
#endif
        }

        CarlaPlugin::uiIdle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        carla_debug("CarlaPluginLV2::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        const float sampleRate(static_cast<float>(pData->engine->getSampleRate()));
        const uint32_t portCount(fRdfDescriptor->PortCount);

        uint32_t aIns, aOuts, cvIns, cvOuts, params;
        aIns = aOuts = cvIns = cvOuts = params = 0;
        LinkedList<uint> evIns, evOuts;

        const uint32_t eventBufferSize = static_cast<uint32_t>(fLv2Options.sequenceSize) + 0xff;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        bool needsCtrlIn, needsCtrlOut, hasPatchParameterOutputs;
        needsCtrlIn = needsCtrlOut = hasPatchParameterOutputs = false;

        for (uint32_t i=0; i < portCount; ++i)
        {
            const LV2_Property portTypes = fRdfDescriptor->Ports[i].Types;

            if (LV2_IS_PORT_AUDIO(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                    aIns += 1;
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                    aOuts += 1;
            }
            else if (LV2_IS_PORT_CV(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                    cvIns += 1;
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                    cvOuts += 1;
            }
            else if (LV2_IS_PORT_ATOM_SEQUENCE(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                    evIns.append(CARLA_EVENT_DATA_ATOM);
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                    evOuts.append(CARLA_EVENT_DATA_ATOM);
            }
            else if (LV2_IS_PORT_EVENT(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                    evIns.append(CARLA_EVENT_DATA_EVENT);
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                    evOuts.append(CARLA_EVENT_DATA_EVENT);
            }
            else if (LV2_IS_PORT_MIDI_LL(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                    evIns.append(CARLA_EVENT_DATA_MIDI_LL);
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                    evOuts.append(CARLA_EVENT_DATA_MIDI_LL);
            }
            else if (LV2_IS_PORT_CONTROL(portTypes))
                params += 1;
        }

        for (uint32_t i=0; i < fRdfDescriptor->ParameterCount; ++i)
        {
            switch (fRdfDescriptor->Parameters[i].Type)
            {
            case LV2_PARAMETER_TYPE_BOOL:
            case LV2_PARAMETER_TYPE_INT:
            // case LV2_PARAMETER_TYPE_LONG:
            case LV2_PARAMETER_TYPE_FLOAT:
            case LV2_PARAMETER_TYPE_DOUBLE:
                params += 1;
                break;
            case LV2_PARAMETER_TYPE_PATH:
                if (fFilePathURI.isEmpty())
                    fFilePathURI = fRdfDescriptor->Parameters[i].URI;
                break;
            }
        }

        if ((pData->options & PLUGIN_OPTION_FORCE_STEREO) != 0 && aIns <= 1 && aOuts <= 1 && evOuts.count() == 0 && fExt.state == nullptr && fExt.worker == nullptr)
        {
            if (fHandle2 == nullptr)
            {
                try {
                    fHandle2 = fDescriptor->instantiate(fDescriptor, sampleRate, fRdfDescriptor->Bundle, fFeatures);
                } catch(...) {}
            }

            if (fHandle2 != nullptr)
            {
                if (aIns == 1)
                {
                    aIns = 2;
                    forcedStereoIn = true;
                }

                if (aOuts == 1)
                {
                    aOuts = 2;
                    forcedStereoOut = true;
                }
            }
        }

        if (aIns > 0)
        {
            pData->audioIn.createNew(aIns);
            fAudioInBuffers = new float*[aIns];

            for (uint32_t i=0; i < aIns; ++i)
                fAudioInBuffers[i] = nullptr;
        }

        if (aOuts > 0)
        {
            pData->audioOut.createNew(aOuts);
            fAudioOutBuffers = new float*[aOuts];
            needsCtrlIn = true;

            for (uint32_t i=0; i < aOuts; ++i)
                fAudioOutBuffers[i] = nullptr;
        }

        if (cvIns > 0)
        {
            pData->cvIn.createNew(cvIns);
            fCvInBuffers = new float*[cvIns];

            for (uint32_t i=0; i < cvIns; ++i)
                fCvInBuffers[i] = nullptr;
        }

        if (cvOuts > 0)
        {
            pData->cvOut.createNew(cvOuts);
            fCvOutBuffers = new float*[cvOuts];

            for (uint32_t i=0; i < cvOuts; ++i)
                fCvOutBuffers[i] = nullptr;
        }

        if (params > 0)
        {
            pData->param.createNew(params, true);
            fParamBuffers = new float[params];
            carla_zeroFloats(fParamBuffers, params);
        }

        if (const uint32_t count = static_cast<uint32_t>(evIns.count()))
        {
            fEventsIn.createNew(count);

            for (uint32_t i=0; i < count; ++i)
            {
                const uint32_t type = evIns.getAt(i, 0x0);

                if (type == CARLA_EVENT_DATA_ATOM)
                {
                    fEventsIn.data[i].type = CARLA_EVENT_DATA_ATOM;
                    fEventsIn.data[i].atom = lv2_atom_buffer_new(eventBufferSize, kUridNull, kUridAtomSequence, true);
                }
                else if (type == CARLA_EVENT_DATA_EVENT)
                {
                    fEventsIn.data[i].type  = CARLA_EVENT_DATA_EVENT;
                    fEventsIn.data[i].event = lv2_event_buffer_new(eventBufferSize, LV2_EVENT_AUDIO_STAMP);
                }
                else if (type == CARLA_EVENT_DATA_MIDI_LL)
                {
                    fEventsIn.data[i].type = CARLA_EVENT_DATA_MIDI_LL;
                    fEventsIn.data[i].midi.capacity = eventBufferSize;
                    fEventsIn.data[i].midi.data     = new uchar[eventBufferSize];
                }
            }
        }
        else
        {
            fEventsIn.createNew(1);
            fEventsIn.ctrl = &fEventsIn.data[0];
        }

        if (const uint32_t count = static_cast<uint32_t>(evOuts.count()))
        {
            fEventsOut.createNew(count);

            for (uint32_t i=0; i < count; ++i)
            {
                const uint32_t type = evOuts.getAt(i, 0x0);

                if (type == CARLA_EVENT_DATA_ATOM)
                {
                    fEventsOut.data[i].type = CARLA_EVENT_DATA_ATOM;
                    fEventsOut.data[i].atom = lv2_atom_buffer_new(eventBufferSize, kUridNull, kUridAtomSequence, false);
                }
                else if (type == CARLA_EVENT_DATA_EVENT)
                {
                    fEventsOut.data[i].type  = CARLA_EVENT_DATA_EVENT;
                    fEventsOut.data[i].event = lv2_event_buffer_new(eventBufferSize, LV2_EVENT_AUDIO_STAMP);
                }
                else if (type == CARLA_EVENT_DATA_MIDI_LL)
                {
                    fEventsOut.data[i].type = CARLA_EVENT_DATA_MIDI_LL;
                    fEventsOut.data[i].midi.capacity = eventBufferSize;
                    fEventsOut.data[i].midi.data     = new uchar[eventBufferSize];
                }
            }
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        String portName;
        uint32_t iCtrl = 0;

        for (uint32_t i=0, iAudioIn=0, iAudioOut=0, iCvIn=0, iCvOut=0, iEvIn=0, iEvOut=0; i < portCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            portName.clear();

            if (LV2_IS_PORT_AUDIO(portTypes) || LV2_IS_PORT_CV(portTypes) || LV2_IS_PORT_ATOM_SEQUENCE(portTypes) || LV2_IS_PORT_EVENT(portTypes) || LV2_IS_PORT_MIDI_LL(portTypes))
            {
                if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = pData->name;
                    portName += ":";
                }

                portName += fRdfDescriptor->Ports[i].Name;
                portName.truncate(portNameSize);
            }

            if (LV2_IS_PORT_AUDIO(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    const uint32_t j = iAudioIn++;
                    pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, j);
                    pData->audioIn.ports[j].rindex = i;

                    if (forcedStereoIn)
                    {
                        portName += "_2";
                        pData->audioIn.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, 1);
                        pData->audioIn.ports[1].rindex = i;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    const uint32_t j = iAudioOut++;
                    pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, j);
                    pData->audioOut.ports[j].rindex = i;

                    if (forcedStereoOut)
                    {
                        portName += "_2";
                        pData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, 1);
                        pData->audioOut.ports[1].rindex = i;
                    }
                }
                else
                    carla_stderr2("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LV2_IS_PORT_CV(portTypes))
            {
                const LV2_RDF_PortPoints portPoints(fRdfDescriptor->Ports[i].Points);
                float min, max;

                // min value
                if (LV2_HAVE_MINIMUM_PORT_POINT(portPoints.Hints))
                    min = portPoints.Minimum;
                else
                    min = -1.0f;

                // max value
                if (LV2_HAVE_MAXIMUM_PORT_POINT(portPoints.Hints))
                    max = portPoints.Maximum;
                else
                    max = 1.0f;

                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    const uint32_t j = iCvIn++;
                    pData->cvIn.ports[j].port   = (CarlaEngineCVPort*)pData->client->addPort(kEnginePortTypeCV, portName, true, j);
                    pData->cvIn.ports[j].rindex = i;
                    pData->cvIn.ports[j].port->setRange(min, max);
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    const uint32_t j = iCvOut++;
                    pData->cvOut.ports[j].port   = (CarlaEngineCVPort*)pData->client->addPort(kEnginePortTypeCV, portName, false, j);
                    pData->cvOut.ports[j].rindex = i;
                    pData->cvOut.ports[j].port->setRange(min, max);
                }
                else
                    carla_stderr("WARNING - Got a broken Port (CV, but not input or output)");
            }
            else if (LV2_IS_PORT_ATOM_SEQUENCE(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    const uint32_t j = iEvIn++;

                    fDescriptor->connect_port(fHandle, i, &fEventsIn.data[j].atom->atoms);

                    if (fHandle2 != nullptr)
                        fDescriptor->connect_port(fHandle2, i, &fEventsIn.data[j].atom->atoms);

                    fEventsIn.data[j].rindex = i;

                    if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                        fEventsIn.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                    if (portTypes & LV2_PORT_DATA_PATCH_MESSAGE)
                        fEventsIn.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    if (portTypes & LV2_PORT_DATA_TIME_POSITION)
                        fEventsIn.data[j].type |= CARLA_EVENT_TYPE_TIME;

                    if (evIns.count() == 1)
                    {
                        fEventsIn.ctrl = &fEventsIn.data[j];
                        fEventsIn.ctrlIndex = j;

                        if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                            needsCtrlIn = true;
                    }
                    else
                    {
                        if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                            fEventsIn.data[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, j);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsIn.ctrl = &fEventsIn.data[j];
                            fEventsIn.ctrlIndex = j;
                        }
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    const uint32_t j = iEvOut++;

                    fDescriptor->connect_port(fHandle, i, &fEventsOut.data[j].atom->atoms);

                    if (fHandle2 != nullptr)
                        fDescriptor->connect_port(fHandle2, i, &fEventsOut.data[j].atom->atoms);

                    fEventsOut.data[j].rindex = i;

                    if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                        fEventsOut.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                    if (portTypes & LV2_PORT_DATA_PATCH_MESSAGE)
                        fEventsOut.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    if (portTypes & LV2_PORT_DATA_TIME_POSITION)
                        fEventsOut.data[j].type |= CARLA_EVENT_TYPE_TIME;

                    if (evOuts.count() == 1)
                    {
                        fEventsOut.ctrl = &fEventsOut.data[j];
                        fEventsOut.ctrlIndex = j;

                        if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                            needsCtrlOut = true;
                    }
                    else
                    {
                        if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                            fEventsOut.data[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, j);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsOut.ctrl = &fEventsOut.data[j];
                            fEventsOut.ctrlIndex = j;
                        }
                    }
                }
                else
                    carla_stderr2("WARNING - Got a broken Port (Atom-Sequence, but not input or output)");
            }
            else if (LV2_IS_PORT_EVENT(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    const uint32_t j = iEvIn++;

                    fDescriptor->connect_port(fHandle, i, fEventsIn.data[j].event);

                    if (fHandle2 != nullptr)
                        fDescriptor->connect_port(fHandle2, i, fEventsIn.data[j].event);

                    fEventsIn.data[j].rindex = i;

                    if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                        fEventsIn.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                    if (portTypes & LV2_PORT_DATA_PATCH_MESSAGE)
                        fEventsIn.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    if (portTypes & LV2_PORT_DATA_TIME_POSITION)
                        fEventsIn.data[j].type |= CARLA_EVENT_TYPE_TIME;

                    if (evIns.count() == 1)
                    {
                        fEventsIn.ctrl = &fEventsIn.data[j];
                        fEventsIn.ctrlIndex = j;

                        if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                            needsCtrlIn = true;
                    }
                    else
                    {
                        if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                            fEventsIn.data[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, j);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsIn.ctrl = &fEventsIn.data[j];
                            fEventsIn.ctrlIndex = j;
                        }
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    const uint32_t j = iEvOut++;

                    fDescriptor->connect_port(fHandle, i, fEventsOut.data[j].event);

                    if (fHandle2 != nullptr)
                        fDescriptor->connect_port(fHandle2, i, fEventsOut.data[j].event);

                    fEventsOut.data[j].rindex = i;

                    if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                        fEventsOut.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                    if (portTypes & LV2_PORT_DATA_PATCH_MESSAGE)
                        fEventsOut.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    if (portTypes & LV2_PORT_DATA_TIME_POSITION)
                        fEventsOut.data[j].type |= CARLA_EVENT_TYPE_TIME;

                    if (evOuts.count() == 1)
                    {
                        fEventsOut.ctrl = &fEventsOut.data[j];
                        fEventsOut.ctrlIndex = j;

                        if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                            needsCtrlOut = true;
                    }
                    else
                    {
                        if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                            fEventsOut.data[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, j);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsOut.ctrl = &fEventsOut.data[j];
                            fEventsOut.ctrlIndex = j;
                        }
                    }
                }
                else
                    carla_stderr2("WARNING - Got a broken Port (Event, but not input or output)");
            }
            else if (LV2_IS_PORT_MIDI_LL(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    const uint32_t j = iEvIn++;

                    fDescriptor->connect_port(fHandle, i, &fEventsIn.data[j].midi);

                    if (fHandle2 != nullptr)
                        fDescriptor->connect_port(fHandle2, i, &fEventsIn.data[j].midi);

                    fEventsIn.data[j].type  |= CARLA_EVENT_TYPE_MIDI;
                    fEventsIn.data[j].rindex = i;

                    if (evIns.count() == 1)
                    {
                        needsCtrlIn = true;
                        fEventsIn.ctrl = &fEventsIn.data[j];
                        fEventsIn.ctrlIndex = j;
                    }
                    else
                    {
                        fEventsIn.data[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, j);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsIn.ctrl = &fEventsIn.data[j];
                            fEventsIn.ctrlIndex = j;
                        }
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    const uint32_t j = iEvOut++;

                    fDescriptor->connect_port(fHandle, i, &fEventsOut.data[j].midi);

                    if (fHandle2 != nullptr)
                        fDescriptor->connect_port(fHandle2, i, &fEventsOut.data[j].midi);

                    fEventsOut.data[j].type  |= CARLA_EVENT_TYPE_MIDI;
                    fEventsOut.data[j].rindex = i;

                    if (evOuts.count() == 1)
                    {
                        needsCtrlOut = true;
                        fEventsOut.ctrl = &fEventsOut.data[j];
                        fEventsOut.ctrlIndex = j;
                    }
                    else
                    {
                        fEventsOut.data[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, j);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsOut.ctrl = &fEventsOut.data[j];
                            fEventsOut.ctrlIndex = j;
                        }
                    }
                }
                else
                    carla_stderr2("WARNING - Got a broken Port (MIDI, but not input or output)");
            }
            else if (LV2_IS_PORT_CONTROL(portTypes))
            {
                const LV2_Property portProps(fRdfDescriptor->Ports[i].Properties);
                const LV2_Property portDesignation(fRdfDescriptor->Ports[i].Designation);
                const LV2_RDF_PortPoints portPoints(fRdfDescriptor->Ports[i].Points);

                const uint32_t j = iCtrl++;
                pData->param.data[j].index  = static_cast<int32_t>(j);
                pData->param.data[j].rindex = static_cast<int32_t>(i);

                float min, max, def, step, stepSmall, stepLarge;

                // min value
                if (LV2_HAVE_MINIMUM_PORT_POINT(portPoints.Hints))
                    min = portPoints.Minimum;
                else
                    min = 0.0f;

                // max value
                if (LV2_HAVE_MAXIMUM_PORT_POINT(portPoints.Hints))
                    max = portPoints.Maximum;
                else
                    max = 1.0f;

                if (LV2_IS_PORT_SAMPLE_RATE(portProps))
                {
                    min *= sampleRate;
                    max *= sampleRate;
                    pData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                // stupid hack for ir.lv2 (broken plugin)
                if (std::strcmp(fRdfDescriptor->URI, "http://factorial.hu/plugins/lv2/ir") == 0 && std::strncmp(fRdfDescriptor->Ports[i].Name, "FileHash", 8) == 0)
                {
                    min = 0.0f;
                    max = (float)0xffffff;
                }

                if (min >= max)
                {
                    carla_stderr2("WARNING - Broken plugin parameter '%s': min >= max", fRdfDescriptor->Ports[i].Name);
                    max = min + 0.1f;
                }

                // default value
                if (LV2_HAVE_DEFAULT_PORT_POINT(portPoints.Hints))
                {
                    def = portPoints.Default;
                }
                else
                {
                    // no default value
                    if (min < 0.0f && max > 0.0f)
                        def = 0.0f;
                    else
                        def = min;
                }

                if (def < min)
                    def = min;
                else if (def > max)
                    def = max;

                if (LV2_IS_PORT_TOGGLED(portProps))
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    pData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LV2_IS_PORT_INTEGER(portProps))
                {
                    step = 1.0f;
                    stepSmall = 1.0f;
                    stepLarge = 10.0f;
                    pData->param.data[j].hints |= PARAMETER_IS_INTEGER;
                }
                else
                {
                    float range = max - min;
                    step = range/100.0f;
                    stepSmall = range/1000.0f;
                    stepLarge = range/10.0f;
                }

                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    pData->param.data[j].type = PARAMETER_INPUT;

                    if (LV2_IS_PORT_DESIGNATION_LATENCY(portDesignation))
                    {
                        carla_stderr("Plugin has latency input port, this should not happen!");
                    }
                    else if (LV2_IS_PORT_DESIGNATION_SAMPLE_RATE(portDesignation))
                    {
                        def = sampleRate;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;
                        pData->param.special[j] = PARAMETER_SPECIAL_SAMPLE_RATE;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_FREEWHEELING(portDesignation))
                    {
                        pData->param.special[j] = PARAMETER_SPECIAL_FREEWHEEL;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_TIME(portDesignation))
                    {
                        pData->param.special[j] = PARAMETER_SPECIAL_TIME;
                    }
                    else
                    {
                        pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        pData->param.data[j].hints |= PARAMETER_IS_AUTOMATABLE;
                        needsCtrlIn = true;

                        if (! LV2_IS_PORT_CAUSES_ARTIFACTS(portProps) &&
                            ! LV2_IS_PORT_ENUMERATION(portProps) &&
                            ! LV2_IS_PORT_EXPENSIVE(portProps) &&
                            ! LV2_IS_PORT_NOT_AUTOMATIC(portProps) &&
                            ! LV2_IS_PORT_NOT_ON_GUI(portProps) &&
                            ! LV2_IS_PORT_TRIGGER(portProps))
                        {
                            pData->param.data[j].hints |= PARAMETER_CAN_BE_CV_CONTROLLED;
                        }
                    }

                    // MIDI CC value
                    const LV2_RDF_PortMidiMap& portMidiMap(fRdfDescriptor->Ports[i].MidiMap);

                    if (LV2_IS_PORT_MIDI_MAP_CC(portMidiMap.Type))
                    {
                        if (portMidiMap.Number < MAX_MIDI_CONTROL && ! MIDI_IS_CONTROL_BANK_SELECT(portMidiMap.Number))
                            pData->param.data[j].mappedControlIndex = static_cast<int16_t>(portMidiMap.Number);
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    pData->param.data[j].type = PARAMETER_OUTPUT;

                    if (LV2_IS_PORT_DESIGNATION_LATENCY(portDesignation))
                    {
                        min = 0.0f;
                        max = sampleRate;
                        def = 0.0f;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;
                        pData->param.special[j] = PARAMETER_SPECIAL_LATENCY;
                        CARLA_SAFE_ASSERT_INT2(fLatencyIndex == static_cast<int32_t>(j), fLatencyIndex, j);
                    }
                    else if (LV2_IS_PORT_DESIGNATION_SAMPLE_RATE(portDesignation))
                    {
                        def = sampleRate;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;
                        pData->param.special[j] = PARAMETER_SPECIAL_SAMPLE_RATE;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_FREEWHEELING(portDesignation))
                    {
                        carla_stderr("Plugin has freewheeling output port, this should not happen!");
                    }
                    else if (LV2_IS_PORT_DESIGNATION_TIME(portDesignation))
                    {
                        pData->param.special[j] = PARAMETER_SPECIAL_TIME;
                    }
                    else
                    {
                        pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        pData->param.data[j].hints |= PARAMETER_IS_AUTOMATABLE;
                        needsCtrlOut = true;
                    }
                }
                else
                {
                    pData->param.data[j].type = PARAMETER_UNKNOWN;
                    carla_stderr2("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LV2_IS_PORT_LOGARITHMIC(portProps))
                    pData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                if (LV2_IS_PORT_TRIGGER(portProps))
                    pData->param.data[j].hints |= PARAMETER_IS_TRIGGER;

                if (LV2_IS_PORT_STRICT_BOUNDS(portProps))
                    pData->param.data[j].hints |= PARAMETER_IS_STRICT_BOUNDS;

                if (LV2_IS_PORT_ENUMERATION(portProps))
                    pData->param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                // check if parameter is not enabled or automatable
                if (LV2_IS_PORT_NOT_ON_GUI(portProps))
                    pData->param.data[j].hints &= ~(PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMATABLE);
                else if (LV2_IS_PORT_CAUSES_ARTIFACTS(portProps) || LV2_IS_PORT_EXPENSIVE(portProps))
                    pData->param.data[j].hints &= ~PARAMETER_IS_AUTOMATABLE;
                else if (LV2_IS_PORT_NOT_AUTOMATIC(portProps) || LV2_IS_PORT_NON_AUTOMATABLE(portProps))
                    pData->param.data[j].hints &= ~PARAMETER_IS_AUTOMATABLE;

                pData->param.ranges[j].min = min;
                pData->param.ranges[j].max = max;
                pData->param.ranges[j].def = def;
                pData->param.ranges[j].step = step;
                pData->param.ranges[j].stepSmall = stepSmall;
                pData->param.ranges[j].stepLarge = stepLarge;

                // Start parameters in their default values (except freewheel, which is off by default)
                if (pData->param.data[j].type == PARAMETER_INPUT && pData->param.special[j] == PARAMETER_SPECIAL_FREEWHEEL)
                    fParamBuffers[j] = min;
                else
                    fParamBuffers[j] = def;

                fDescriptor->connect_port(fHandle, i, &fParamBuffers[j]);

                if (fHandle2 != nullptr)
                    fDescriptor->connect_port(fHandle2, i, &fParamBuffers[j]);
            }
            else
            {
                // Port Type not supported, but it's optional anyway
                fDescriptor->connect_port(fHandle, i, nullptr);

                if (fHandle2 != nullptr)
                    fDescriptor->connect_port(fHandle2, i, nullptr);
            }
        }

        for (uint32_t i=0; i < fRdfDescriptor->ParameterCount; ++i)
        {
            const LV2_RDF_Parameter& rdfParam(fRdfDescriptor->Parameters[i]);

            switch (rdfParam.Type)
            {
            case LV2_PARAMETER_TYPE_BOOL:
            case LV2_PARAMETER_TYPE_INT:
            // case LV2_PARAMETER_TYPE_LONG:
            case LV2_PARAMETER_TYPE_FLOAT:
            case LV2_PARAMETER_TYPE_DOUBLE:
                break;
            default:
                continue;
            }

            const LV2_RDF_PortPoints& portPoints(rdfParam.Points);

            const uint32_t j = iCtrl++;
            pData->param.data[j].index  = static_cast<int32_t>(j);
            pData->param.data[j].rindex = static_cast<int32_t>(fRdfDescriptor->PortCount + i);

            float min, max, def, step, stepSmall, stepLarge;

            // min value
            if (LV2_HAVE_MINIMUM_PORT_POINT(portPoints.Hints))
                min = portPoints.Minimum;
            else
                min = 0.0f;

            // max value
            if (LV2_HAVE_MAXIMUM_PORT_POINT(portPoints.Hints))
                max = portPoints.Maximum;
            else
                max = 1.0f;

            if (min >= max)
            {
                carla_stderr2("WARNING - Broken plugin parameter '%s': min >= max", rdfParam.Label);
                max = min + 0.1f;
            }

            // default value
            if (LV2_HAVE_DEFAULT_PORT_POINT(portPoints.Hints))
            {
                def = portPoints.Default;
            }
            else
            {
                // no default value
                if (min < 0.0f && max > 0.0f)
                    def = 0.0f;
                else
                    def = min;
            }

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            switch (rdfParam.Type)
            {
            case LV2_PARAMETER_TYPE_BOOL:
                step = max - min;
                stepSmall = step;
                stepLarge = step;
                pData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                break;

            case LV2_PARAMETER_TYPE_INT:
            case LV2_PARAMETER_TYPE_LONG:
                step = 1.0f;
                stepSmall = 1.0f;
                stepLarge = 10.0f;
                pData->param.data[j].hints |= PARAMETER_IS_INTEGER;
                break;

            default:
                const float range = max - min;
                step = range/100.0f;
                stepSmall = range/1000.0f;
                stepLarge = range/10.0f;
                break;
            }

            if (rdfParam.Flags & LV2_PARAMETER_FLAG_INPUT)
            {
                pData->param.data[j].type   = PARAMETER_INPUT;
                pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                pData->param.data[j].hints |= PARAMETER_IS_AUTOMATABLE;
                pData->param.data[j].hints |= PARAMETER_IS_NOT_SAVED;
                needsCtrlIn = true;

                if (rdfParam.Flags & LV2_PARAMETER_FLAG_OUTPUT)
                    hasPatchParameterOutputs = true;

                if (LV2_IS_PORT_MIDI_MAP_CC(rdfParam.MidiMap.Type))
                {
                    if (rdfParam.MidiMap.Number < MAX_MIDI_CONTROL && ! MIDI_IS_CONTROL_BANK_SELECT(rdfParam.MidiMap.Number))
                        pData->param.data[j].mappedControlIndex = static_cast<int16_t>(rdfParam.MidiMap.Number);
                }
            }
            else if (rdfParam.Flags & LV2_PARAMETER_FLAG_OUTPUT)
            {
                pData->param.data[j].type   = PARAMETER_OUTPUT;
                pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                pData->param.data[j].hints |= PARAMETER_IS_AUTOMATABLE;
                needsCtrlOut = true;
                hasPatchParameterOutputs = true;
            }

            pData->param.ranges[j].min = min;
            pData->param.ranges[j].max = max;
            pData->param.ranges[j].def = def;
            pData->param.ranges[j].step = step;
            pData->param.ranges[j].stepSmall = stepSmall;
            pData->param.ranges[j].stepLarge = stepLarge;

            fParamBuffers[j] = def;
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

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, 0);
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            pData->event.cvSourcePorts = pData->client->createCVSourcePorts();
#endif
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

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, 0);
        }

        if (fExt.worker != nullptr && fEventsIn.ctrl != nullptr)
        {
            fAtomBufferWorkerIn.createBuffer(eventBufferSize, true);
            fAtomBufferWorkerResp.createBuffer(eventBufferSize, true);
            fAtomBufferRealtimeSize = fAtomBufferWorkerIn.getSize(); // actual buffer size will be next power of 2
            fAtomBufferRealtime = static_cast<LV2_Atom*>(std::malloc(fAtomBufferRealtimeSize));
            fAtomBufferWorkerInTmpData = new uint8_t[fAtomBufferRealtimeSize];
            carla_mlock(fAtomBufferRealtime, fAtomBufferRealtimeSize);
        }

        if (fRdfDescriptor->ParameterCount > 0 ||
            (fUI.type != UI::TYPE_NULL && fEventsIn.count > 0 && (fEventsIn.data[0].type & CARLA_EVENT_DATA_ATOM) != 0))
        {
            fAtomBufferEvIn.createBuffer(eventBufferSize, true);

            if (fAtomBufferRealtimeSize == 0)
            {
                fAtomBufferRealtimeSize = fAtomBufferEvIn.getSize(); // actual buffer size will be next power of 2
                fAtomBufferRealtime = static_cast<LV2_Atom*>(std::malloc(fAtomBufferRealtimeSize));
                carla_mlock(fAtomBufferRealtime, fAtomBufferRealtimeSize);
            }
        }

        if (hasPatchParameterOutputs ||
            (fUI.type != UI::TYPE_NULL && fEventsOut.count > 0 && (fEventsOut.data[0].type & CARLA_EVENT_DATA_ATOM) != 0))
        {
            fAtomBufferUiOut.createBuffer(std::min(eventBufferSize*32, 1638400U), true);
            fAtomBufferUiOutTmpData = new uint8_t[fAtomBufferUiOut.getSize()];
        }

        if (fEventsIn.ctrl != nullptr && fEventsIn.ctrl->port == nullptr)
            fEventsIn.ctrl->port = pData->event.portIn;

        if (fEventsOut.ctrl != nullptr && fEventsOut.ctrl->port == nullptr)
            fEventsOut.ctrl->port = pData->event.portOut;

        if (fEventsIn.ctrl != nullptr && fExt.midnam != nullptr)
        {
            if (char* const midnam = fExt.midnam->midnam(fHandle))
            {
                fEventsIn.ctrl->port->setMetaData("http://www.midi.org/dtds/MIDINameDocument10.dtd",
                                                  midnam, "text/xml");
                if (fExt.midnam->free != nullptr)
                    fExt.midnam->free(midnam);
            }
        }

        if (forcedStereoIn || forcedStereoOut)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;
        else
            pData->options &= ~PLUGIN_OPTION_FORCE_STEREO;

        // plugin hints
        pData->hints = (pData->hints & PLUGIN_HAS_INLINE_DISPLAY) ? PLUGIN_HAS_INLINE_DISPLAY : 0
                     | (pData->hints & PLUGIN_NEEDS_UI_MAIN_THREAD) ? PLUGIN_NEEDS_UI_MAIN_THREAD : 0;

        if (isRealtimeSafe())
            pData->hints |= PLUGIN_IS_RTSAFE;

        if (fUI.type != UI::TYPE_NULL || fFilePathURI.isNotEmpty())
        {
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;

            if (fUI.type == UI::TYPE_EMBED)
            {
                switch (fUI.rdfDescriptor->Type)
                {
                case LV2_UI_GTK2:
                case LV2_UI_GTK3:
                case LV2_UI_QT4:
                case LV2_UI_QT5:
                case LV2_UI_EXTERNAL:
                case LV2_UI_OLD_EXTERNAL:
                    break;
                default:
                    pData->hints |= PLUGIN_HAS_CUSTOM_EMBED_UI|PLUGIN_HAS_CUSTOM_RESIZABLE_UI;
                    for (uint32_t i = 0; i < fUI.rdfDescriptor->ExtensionCount; ++i)
                    {
                        const char* const extension = fUI.rdfDescriptor->Extensions[i];
                        CARLA_SAFE_ASSERT_CONTINUE(extension != nullptr);

                        if (std::strcmp(extension, LV2_UI__noUserResize) == 0)
                        {
                            pData->hints &= ~PLUGIN_HAS_CUSTOM_RESIZABLE_UI;
                            break;
                        }
                    }
                    break;
                }
            }

            if (fUI.type == UI::TYPE_EMBED || fUI.type == UI::TYPE_EXTERNAL)
                pData->hints |= PLUGIN_NEEDS_UI_MAIN_THREAD;
            else if (fFilePathURI.isNotEmpty())
                pData->hints |= PLUGIN_HAS_CUSTOM_UI_USING_FILE_OPEN;
        }

        if (LV2_IS_GENERATOR(fRdfDescriptor->Type[0], fRdfDescriptor->Type[1]))
            pData->hints |= PLUGIN_IS_SYNTH;

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints = 0x0;

        // check initial latency
        findInitialLatencyValue(aIns, cvIns, aOuts, cvOuts);

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        evIns.clear();
        evOuts.clear();

        if (pData->active)
            activate();

        carla_debug("CarlaPluginLV2::reload() - end");
    }

    void findInitialLatencyValue(const uint32_t aIns,
                                 const uint32_t cvIns,
                                 const uint32_t aOuts,
                                 const uint32_t cvOuts) const
    {
        if (fLatencyIndex < 0)
            return;

        // we need to pre-run the plugin so it can update its latency control-port
        const uint32_t bufferSize = static_cast<uint32_t>(fLv2Options.nominalBufferSize);

        float* tmpIn[96];
        float* tmpOut[96];

        {
            uint32_t i=0;
            for (; i < aIns; ++i)
            {
                tmpIn[i] = new float[bufferSize];
                carla_zeroFloats(tmpIn[i], bufferSize);

                try {
                    fDescriptor->connect_port(fHandle, pData->audioIn.ports[i].rindex, tmpIn[i]);
                } CARLA_SAFE_EXCEPTION("LV2 connect_port latency audio input");
            }

            for (uint32_t j=0; j < cvIns; ++i, ++j)
            {
                tmpIn[i] = new float[bufferSize];
                carla_zeroFloats(tmpIn[i], bufferSize);

                try {
                    fDescriptor->connect_port(fHandle, pData->cvIn.ports[j].rindex, tmpIn[i]);
                } CARLA_SAFE_EXCEPTION("LV2 connect_port latency cv input");
            }
        }

        {
            uint32_t i=0;
            for (; i < aOuts; ++i)
            {
                tmpOut[i] = new float[bufferSize];
                carla_zeroFloats(tmpOut[i], bufferSize);

                try {
                    fDescriptor->connect_port(fHandle, pData->audioOut.ports[i].rindex, tmpOut[i]);
                } CARLA_SAFE_EXCEPTION("LV2 connect_port latency audio output");
            }

            for (uint32_t j=0; j < cvOuts; ++i, ++j)
            {
                tmpOut[i] = new float[bufferSize];
                carla_zeroFloats(tmpOut[i], bufferSize);

                try {
                    fDescriptor->connect_port(fHandle, pData->cvOut.ports[j].rindex, tmpOut[i]);
                } CARLA_SAFE_EXCEPTION("LV2 connect_port latency cv output");
            }
        }

        if (fDescriptor->activate != nullptr)
        {
            try {
                fDescriptor->activate(fHandle);
            } CARLA_SAFE_EXCEPTION("LV2 latency activate");
        }

        try {
            fDescriptor->run(fHandle, bufferSize);
        } CARLA_SAFE_EXCEPTION("LV2 latency run");

        if (fDescriptor->deactivate != nullptr)
        {
            try {
                fDescriptor->deactivate(fHandle);
            } CARLA_SAFE_EXCEPTION("LV2 latency deactivate");
        }

        // done, let's get the value
        if (const uint32_t latency = getLatencyInFrames())
        {
            pData->client->setLatency(latency);
#ifndef BUILD_BRIDGE
            pData->latency.recreateBuffers(std::max(aIns, aOuts), latency);
#endif
        }

        for (uint32_t i=0; i < aIns + cvIns; ++i)
            delete[] tmpIn[i];

        for (uint32_t i=0; i < aOuts + cvOuts; ++i)
            delete[] tmpOut[i];
    }

    void reloadPrograms(const bool doInit) override
    {
        carla_debug("CarlaPluginLV2::reloadPrograms(%s)", bool2str(doInit));
        const uint32_t oldCount = pData->midiprog.count;
        const int32_t  current  = pData->midiprog.current;

        // special LV2 programs handling
        if (doInit)
        {
            pData->prog.clear();

            const uint32_t presetCount(fRdfDescriptor->PresetCount);

            if (presetCount > 0)
            {
                pData->prog.createNew(presetCount);

                for (uint32_t i=0; i < presetCount; ++i)
                    pData->prog.names[i] = carla_strdup(fRdfDescriptor->Presets[i].Label);
            }
        }

        // Delete old programs
        pData->midiprog.clear();

        // Query new programs
        uint32_t newCount = 0;
        if (fExt.programs != nullptr && fExt.programs->get_program != nullptr && fExt.programs->select_program != nullptr)
        {
            for (; fExt.programs->get_program(fHandle, newCount);)
                ++newCount;
        }

        if (newCount > 0)
        {
            pData->midiprog.createNew(newCount);

            // Update data
            for (uint32_t i=0; i < newCount; ++i)
            {
                const LV2_Program_Descriptor* const pdesc(fExt.programs->get_program(fHandle, i));
                CARLA_SAFE_ASSERT_CONTINUE(pdesc != nullptr);
                CARLA_SAFE_ASSERT(pdesc->name != nullptr);

                pData->midiprog.data[i].bank    = pdesc->bank;
                pData->midiprog.data[i].program = pdesc->program;
                pData->midiprog.data[i].name    = carla_strdup(pdesc->name);
            }
        }

        if (doInit)
        {
            if (newCount > 0)
            {
                setMidiProgram(0, false, false, false, true);
            }
            else if (fHasLoadDefaultState)
            {
                // load default state
                if (LilvState* const state = Lv2WorldClass::getInstance().getStateFromURI(fDescriptor->URI,
                                                                                          (const LV2_URID_Map*)fFeatures[kFeatureIdUridMap]->data))
                {
                    lilv_state_restore(state, fExt.state, fHandle, carla_lilv_set_port_value, this, 0, fFeatures);

                    if (fHandle2 != nullptr)
                        lilv_state_restore(state, fExt.state, fHandle2, carla_lilv_set_port_value, this, 0, fFeatures);

                    lilv_state_free(state);
                }
            }
        }
        else
        {
            // Check if current program is invalid
            bool programChanged = false;

            if (newCount == oldCount+1)
            {
                // one midi program added, probably created by user
                pData->midiprog.current = static_cast<int32_t>(oldCount);
                programChanged = true;
            }
            else if (current < 0 && newCount > 0)
            {
                // programs exist now, but not before
                pData->midiprog.current = 0;
                programChanged = true;
            }
            else if (current >= 0 && newCount == 0)
            {
                // programs existed before, but not anymore
                pData->midiprog.current = -1;
                programChanged = true;
            }
            else if (current >= static_cast<int32_t>(newCount))
            {
                // current midi program > count
                pData->midiprog.current = 0;
                programChanged = true;
            }
            else
            {
                // no change
                pData->midiprog.current = current;
            }

            if (programChanged)
                setMidiProgram(pData->midiprog.current, true, true, true, false);

            pData->engine->callback(true, true, ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fDescriptor->activate != nullptr)
        {
            try {
                fDescriptor->activate(fHandle);
            } CARLA_SAFE_EXCEPTION("LV2 activate");

            if (fHandle2 != nullptr)
            {
                try {
                    fDescriptor->activate(fHandle2);
                } CARLA_SAFE_EXCEPTION("LV2 activate #2");
            }
        }

        fFirstActive = true;
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fDescriptor->deactivate != nullptr)
        {
            try {
                fDescriptor->deactivate(fHandle);
            } CARLA_SAFE_EXCEPTION("LV2 deactivate");

            if (fHandle2 != nullptr)
            {
                try {
                    fDescriptor->deactivate(fHandle2);
                } CARLA_SAFE_EXCEPTION("LV2 deactivate #2");
            }
        }
    }

    void process(const float* const* const audioIn, float** const audioOut,
                 const float* const* const cvIn, float** const cvOut, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
                carla_zeroFloats(cvOut[i], frames);
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event itenerators from different APIs (input)

        for (uint32_t i=0; i < fEventsIn.count; ++i)
        {
            if (fEventsIn.data[i].type & CARLA_EVENT_DATA_ATOM)
            {
                lv2_atom_buffer_reset(fEventsIn.data[i].atom, true);
                lv2_atom_buffer_begin(&fEventsIn.iters[i].atom, fEventsIn.data[i].atom);
            }
            else if (fEventsIn.data[i].type & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(fEventsIn.data[i].event, LV2_EVENT_AUDIO_STAMP, fEventsIn.data[i].event->data);
                lv2_event_begin(&fEventsIn.iters[i].event, fEventsIn.data[i].event);
            }
            else if (fEventsIn.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
            {
                fEventsIn.data[i].midi.event_count = 0;
                fEventsIn.data[i].midi.size        = 0;
                LV2_MIDIState& midiState(fEventsIn.iters[i].midiState);
                midiState.midi        = &fEventsIn.data[i].midi;
                midiState.frame_count = frames;
                midiState.position    = 0;
            }
        }

        for (uint32_t i=0; i < fEventsOut.count; ++i)
        {
            if (fEventsOut.data[i].type & CARLA_EVENT_DATA_ATOM)
            {
                lv2_atom_buffer_reset(fEventsOut.data[i].atom, false);
            }
            else if (fEventsOut.data[i].type & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(fEventsOut.data[i].event, LV2_EVENT_AUDIO_STAMP, fEventsOut.data[i].event->data);
            }
            else if (fEventsOut.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
            {
                // not needed
            }
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (fEventsIn.ctrl != nullptr && (fEventsIn.ctrl->type & CARLA_EVENT_TYPE_MIDI) != 0)
            {
                const uint32_t j = fEventsIn.ctrlIndex;
                CARLA_ASSERT(j < fEventsIn.count);

                uint8_t midiData[3] = { 0, 0, 0 };

                if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                {
                    for (uint8_t i=0; i < MAX_MIDI_CHANNELS; ++i)
                    {
                        midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT));
                        midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                            lv2_atom_buffer_write(&fEventsIn.iters[j].atom, 0, 0, kUridMidiEvent, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&fEventsIn.iters[j].event, 0, 0, kUridMidiEvent, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&fEventsIn.iters[j].midiState, 0.0, 3, midiData);

                        midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT));
                        midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                            lv2_atom_buffer_write(&fEventsIn.iters[j].atom, 0, 0, kUridMidiEvent, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&fEventsIn.iters[j].event, 0, 0, kUridMidiEvent, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&fEventsIn.iters[j].midiState, 0.0, 3, midiData);
                    }
                }
                else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
                {
                    for (uint8_t k=0; k < MAX_MIDI_NOTE; ++k)
                    {
                        midiData[0] = uint8_t(MIDI_STATUS_NOTE_OFF | (pData->ctrlChannel & MIDI_CHANNEL_BIT));
                        midiData[1] = k;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                            lv2_atom_buffer_write(&fEventsIn.iters[j].atom, 0, 0, kUridMidiEvent, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&fEventsIn.iters[j].event, 0, 0, kUridMidiEvent, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&fEventsIn.iters[j].midiState, 0.0, 3, midiData);
                    }
                }
            }

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // TimeInfo

        const EngineTimeInfo timeInfo(pData->engine->getTimeInfo());

        if (fFirstActive || fLastTimeInfo != timeInfo)
        {
            bool doPostRt;
            int32_t rindex;

            const double barBeat = static_cast<double>(timeInfo.bbt.beat - 1)
                                 + (timeInfo.bbt.tick / timeInfo.bbt.ticksPerBeat);

            // update input ports
            for (uint32_t k=0; k < pData->param.count; ++k)
            {
                if (pData->param.data[k].type != PARAMETER_INPUT)
                    continue;
                if (pData->param.special[k] != PARAMETER_SPECIAL_TIME)
                    continue;

                doPostRt = false;
                rindex = pData->param.data[k].rindex;

                CARLA_SAFE_ASSERT_CONTINUE(rindex >= 0 && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount));

                switch (fRdfDescriptor->Ports[rindex].Designation)
                {
                // Non-BBT
                case LV2_PORT_DESIGNATION_TIME_SPEED:
                    if (fLastTimeInfo.playing != timeInfo.playing)
                    {
                        fParamBuffers[k] = timeInfo.playing ? 1.0f : 0.0f;
                        doPostRt = true;
                    }
                    break;

                case LV2_PORT_DESIGNATION_TIME_FRAME:
                    if (fLastTimeInfo.frame != timeInfo.frame)
                    {
                        fParamBuffers[k] = static_cast<float>(timeInfo.frame);
                        doPostRt = true;
                    }
                    break;

                case LV2_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND:
                    break;

                // BBT
                case LV2_PORT_DESIGNATION_TIME_BAR:
                    if (timeInfo.bbt.valid && fLastTimeInfo.bbt.bar != timeInfo.bbt.bar)
                    {
                        fParamBuffers[k] = static_cast<float>(timeInfo.bbt.bar - 1);
                        doPostRt = true;
                    }
                    break;

                case LV2_PORT_DESIGNATION_TIME_BAR_BEAT:
                    if (timeInfo.bbt.valid && (carla_isNotEqual(fLastTimeInfo.bbt.tick, timeInfo.bbt.tick) ||
                                               fLastTimeInfo.bbt.beat != timeInfo.bbt.beat))
                    {
                        fParamBuffers[k] = static_cast<float>(barBeat);
                        doPostRt = true;
                    }
                    break;

                case LV2_PORT_DESIGNATION_TIME_BEAT:
                    if (timeInfo.bbt.valid && fLastTimeInfo.bbt.beat != timeInfo.bbt.beat)
                    {
                        fParamBuffers[k] = static_cast<float>(timeInfo.bbt.beat - 1);
                        doPostRt = true;
                    }
                    break;

                case LV2_PORT_DESIGNATION_TIME_BEAT_UNIT:
                    if (timeInfo.bbt.valid && carla_isNotEqual(fLastTimeInfo.bbt.beatType, timeInfo.bbt.beatType))
                    {
                        fParamBuffers[k] = timeInfo.bbt.beatType;
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR:
                    if (timeInfo.bbt.valid && carla_isNotEqual(fLastTimeInfo.bbt.beatsPerBar, timeInfo.bbt.beatsPerBar))
                    {
                        fParamBuffers[k] = timeInfo.bbt.beatsPerBar;
                        doPostRt = true;
                    }
                    break;

                case LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE:
                    if (timeInfo.bbt.valid && carla_isNotEqual(fLastTimeInfo.bbt.beatsPerMinute, timeInfo.bbt.beatsPerMinute))
                    {
                        fParamBuffers[k] = static_cast<float>(timeInfo.bbt.beatsPerMinute);
                        doPostRt = true;
                    }
                    break;

                case LV2_PORT_DESIGNATION_TIME_TICKS_PER_BEAT:
                    if (timeInfo.bbt.valid && carla_isNotEqual(fLastTimeInfo.bbt.ticksPerBeat, timeInfo.bbt.ticksPerBeat))
                    {
                        fParamBuffers[k] = static_cast<float>(timeInfo.bbt.ticksPerBeat);
                        doPostRt = true;
                    }
                    break;
                }

                if (doPostRt)
                    pData->postponeParameterChangeRtEvent(true, static_cast<int32_t>(k), fParamBuffers[k]);
            }

            for (uint32_t i=0; i < fEventsIn.count; ++i)
            {
                if ((fEventsIn.data[i].type & CARLA_EVENT_DATA_ATOM) == 0 || (fEventsIn.data[i].type & CARLA_EVENT_TYPE_TIME) == 0)
                    continue;

                uint8_t timeInfoBuf[256];
                LV2_Atom_Forge atomForge;
                initAtomForge(atomForge);
                lv2_atom_forge_set_buffer(&atomForge, timeInfoBuf, sizeof(timeInfoBuf));

                LV2_Atom_Forge_Frame forgeFrame;
                lv2_atom_forge_object(&atomForge, &forgeFrame, kUridNull, kUridTimePosition);

                lv2_atom_forge_key(&atomForge, kUridTimeSpeed);
                lv2_atom_forge_float(&atomForge, timeInfo.playing ? 1.0f : 0.0f);

                lv2_atom_forge_key(&atomForge, kUridTimeFrame);
                lv2_atom_forge_long(&atomForge, static_cast<int64_t>(timeInfo.frame));

                if (timeInfo.bbt.valid)
                {
                    lv2_atom_forge_key(&atomForge, kUridTimeBar);
                    lv2_atom_forge_long(&atomForge, timeInfo.bbt.bar - 1);

                    lv2_atom_forge_key(&atomForge, kUridTimeBarBeat);
                    lv2_atom_forge_float(&atomForge, static_cast<float>(barBeat));

                    lv2_atom_forge_key(&atomForge, kUridTimeBeat);
                    lv2_atom_forge_double(&atomForge, timeInfo.bbt.beat - 1);

                    lv2_atom_forge_key(&atomForge, kUridTimeBeatUnit);
                    lv2_atom_forge_int(&atomForge, static_cast<int32_t>(timeInfo.bbt.beatType));

                    lv2_atom_forge_key(&atomForge, kUridTimeBeatsPerBar);
                    lv2_atom_forge_float(&atomForge, timeInfo.bbt.beatsPerBar);

                    lv2_atom_forge_key(&atomForge, kUridTimeBeatsPerMinute);
                    lv2_atom_forge_float(&atomForge, static_cast<float>(timeInfo.bbt.beatsPerMinute));

                    lv2_atom_forge_key(&atomForge, kUridTimeTicksPerBeat);
                    lv2_atom_forge_double(&atomForge, timeInfo.bbt.ticksPerBeat);
                }

                lv2_atom_forge_pop(&atomForge, &forgeFrame);

                LV2_Atom* const atom((LV2_Atom*)timeInfoBuf);
                CARLA_SAFE_ASSERT_BREAK(atom->size < 256);

                // send only deprecated blank object for now
                lv2_atom_buffer_write(&fEventsIn.iters[i].atom, 0, 0, kUridAtomBlank, atom->size, LV2_ATOM_BODY_CONST(atom));

                // for atom:object
                //lv2_atom_buffer_write(&fEventsIn.iters[i].atom, 0, 0, atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));
            }

            pData->postRtEvents.trySplice();

            fLastTimeInfo = timeInfo;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (fEventsIn.ctrl != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // Message Input

            if (fAtomBufferEvIn.tryLock())
            {
                if (fAtomBufferEvIn.isDataAvailableForReading())
                {
                    uint32_t j, portIndex;
                    LV2_Atom* const atom = fAtomBufferRealtime;
                    atom->size = fAtomBufferRealtimeSize;

                    for (; fAtomBufferEvIn.get(portIndex, atom); atom->size = fAtomBufferRealtimeSize)
                    {
                        j = (portIndex < fEventsIn.count) ? portIndex : fEventsIn.ctrlIndex;

                        if (! lv2_atom_buffer_write(&fEventsIn.iters[j].atom, 0, 0, atom->type, atom->size, LV2_ATOM_BODY_CONST(atom)))
                        {
                            carla_stderr2("Event input buffer full, at least 1 message lost");
                            continue;
                        }

                        inspectAtomForParameterChange(atom);
                    }
                }

                fAtomBufferEvIn.unlock();
            }

            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                if ((fEventsIn.ctrl->type & CARLA_EVENT_TYPE_MIDI) == 0)
                {
                    // does not handle MIDI
                    pData->extNotes.data.clear();
                }
                else
                {
                    const uint32_t j = fEventsIn.ctrlIndex;

                    for (RtLinkedList<ExternalMidiNote>::Itenerator it = pData->extNotes.data.begin2(); it.valid(); it.next())
                    {
                        const ExternalMidiNote& note(it.getValue(kExternalMidiNoteFallback));
                        CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                        uint8_t midiEvent[3];
                        midiEvent[0] = uint8_t((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT));
                        midiEvent[1] = note.note;
                        midiEvent[2] = note.velo;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                            lv2_atom_buffer_write(&fEventsIn.iters[j].atom, 0, 0, kUridMidiEvent, 3, midiEvent);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&fEventsIn.iters[j].event, 0, 0, kUridMidiEvent, 3, midiEvent);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&fEventsIn.iters[j].midiState, 0.0, 3, midiEvent);
                    }

                    pData->extNotes.data.clear();
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            bool allNotesOffSent  = false;
#endif
            bool isSampleAccurate = (pData->options & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;
            uint32_t nextBankId;

            if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
            else
                nextBankId = 0;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            if (cvIn != nullptr && pData->event.cvSourcePorts != nullptr)
                pData->event.cvSourcePorts->initPortBuffers(cvIn + pData->cvIn.count, frames, isSampleAccurate, pData->event.portIn);
#endif

            const uint32_t numEvents = (fEventsIn.ctrl->port != nullptr) ? fEventsIn.ctrl->port->getEventCount() : 0;

            for (uint32_t i=0; i < numEvents; ++i)
            {
                EngineEvent& event(fEventsIn.ctrl->port->getEvent(i));

                uint32_t eventTime = event.time;
                CARLA_SAFE_ASSERT_UINT2_CONTINUE(eventTime < frames, eventTime, frames);

                if (eventTime < timeOffset)
                {
                    carla_stderr2("Timing error, eventTime:%u < timeOffset:%u for '%s'",
                                  eventTime, timeOffset, pData->name);
                    eventTime = timeOffset;
                }

                if (isSampleAccurate && eventTime > timeOffset)
                {
                    if (processSingle(audioIn, audioOut, cvIn, cvOut, eventTime - timeOffset, timeOffset))
                    {
                        startTime  = 0;
                        timeOffset = eventTime;

                        if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                            nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
                        else
                            nextBankId = 0;

                        for (uint32_t j=0; j < fEventsIn.count; ++j)
                        {
                            if (fEventsIn.data[j].type & CARLA_EVENT_DATA_ATOM)
                            {
                                lv2_atom_buffer_reset(fEventsIn.data[j].atom, true);
                                lv2_atom_buffer_begin(&fEventsIn.iters[j].atom, fEventsIn.data[j].atom);
                            }
                            else if (fEventsIn.data[j].type & CARLA_EVENT_DATA_EVENT)
                            {
                                lv2_event_buffer_reset(fEventsIn.data[j].event, LV2_EVENT_AUDIO_STAMP, fEventsIn.data[j].event->data);
                                lv2_event_begin(&fEventsIn.iters[j].event, fEventsIn.data[j].event);
                            }
                            else if (fEventsIn.data[j].type & CARLA_EVENT_DATA_MIDI_LL)
                            {
                                fEventsIn.data[j].midi.event_count = 0;
                                fEventsIn.data[j].midi.size        = 0;
                                fEventsIn.iters[j].midiState.position = eventTime;
                            }
                        }

                        for (uint32_t j=0; j < fEventsOut.count; ++j)
                        {
                            if (fEventsOut.data[j].type & CARLA_EVENT_DATA_ATOM)
                            {
                                lv2_atom_buffer_reset(fEventsOut.data[j].atom, false);
                            }
                            else if (fEventsOut.data[j].type & CARLA_EVENT_DATA_EVENT)
                            {
                                lv2_event_buffer_reset(fEventsOut.data[j].event, LV2_EVENT_AUDIO_STAMP, fEventsOut.data[j].event->data);
                            }
                            else if (fEventsOut.data[j].type & CARLA_EVENT_DATA_MIDI_LL)
                            {
                                // not needed
                            }
                        }
                    }
                    else
                    {
                        startTime += timeOffset;
                    }
                }

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

                            if (pData->param.data[k].mappedFlags & PARAMETER_MAPPING_MIDI_DELTA)
                                value = pData->param.getFinalValueWithMidiDelta(k, fParamBuffers[k], ctrlEvent.midiValue);
                            else
                                value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);

                            setParameterValueRT(k, value, event.time, true);
                        }

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_VALUE)
                        {
                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = uint8_t(ctrlEvent.param);
                            midiData[2] = uint8_t(ctrlEvent.normalizedValue*127.0f + 0.5f);

                            const uint32_t mtime(isSampleAccurate ? startTime : eventTime);

                            if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                                lv2_atom_buffer_write(&fEventsIn.iters[fEventsIn.ctrlIndex].atom, mtime, 0, kUridMidiEvent, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&fEventsIn.iters[fEventsIn.ctrlIndex].event, mtime, 0, kUridMidiEvent, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&fEventsIn.iters[fEventsIn.ctrlIndex].midiState, mtime, 3, midiData);
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

                            const uint32_t mtime(isSampleAccurate ? startTime : eventTime);

                            if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                                lv2_atom_buffer_write(&fEventsIn.iters[fEventsIn.ctrlIndex].atom, mtime, 0, kUridMidiEvent, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&fEventsIn.iters[fEventsIn.ctrlIndex].event, mtime, 0, kUridMidiEvent, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&fEventsIn.iters[fEventsIn.ctrlIndex].midiState, mtime, 3, midiData);
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
                            uint8_t midiData[2];
                            midiData[0] = uint8_t(MIDI_STATUS_PROGRAM_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = uint8_t(ctrlEvent.param);

                            const uint32_t mtime(isSampleAccurate ? startTime : eventTime);

                            if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                                lv2_atom_buffer_write(&fEventsIn.iters[fEventsIn.ctrlIndex].atom, mtime, 0, kUridMidiEvent, 2, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&fEventsIn.iters[fEventsIn.ctrlIndex].event, mtime, 0, kUridMidiEvent, 2, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&fEventsIn.iters[fEventsIn.ctrlIndex].midiState, mtime, 2, midiData);
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            const uint32_t mtime(isSampleAccurate ? startTime : eventTime);

                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                            midiData[2] = 0;

                            if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                                lv2_atom_buffer_write(&fEventsIn.iters[fEventsIn.ctrlIndex].atom, mtime, 0, kUridMidiEvent, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&fEventsIn.iters[fEventsIn.ctrlIndex].event, mtime, 0, kUridMidiEvent, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&fEventsIn.iters[fEventsIn.ctrlIndex].midiState, mtime, 3, midiData);
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

                            const uint32_t mtime(isSampleAccurate ? startTime : eventTime);

                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                            midiData[2] = 0;

                            if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                                lv2_atom_buffer_write(&fEventsIn.iters[fEventsIn.ctrlIndex].atom, mtime, 0, kUridMidiEvent, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&fEventsIn.iters[fEventsIn.ctrlIndex].event, mtime, 0, kUridMidiEvent, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&fEventsIn.iters[fEventsIn.ctrlIndex].midiState, mtime, 3, midiData);
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    const EngineMidiEvent& midiEvent(event.midi);

                    const uint8_t* const midiData = midiEvent.size > EngineMidiEvent::kDataSize ? midiEvent.dataExt : midiEvent.data;

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiData));

                    if ((status == MIDI_STATUS_NOTE_OFF || status == MIDI_STATUS_NOTE_ON) && (pData->options & PLUGIN_OPTION_SKIP_SENDING_NOTES))
                        continue;
                    if (status == MIDI_STATUS_CHANNEL_PRESSURE && (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (status == MIDI_STATUS_CONTROL_CHANGE && (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH && (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (status == MIDI_STATUS_PITCH_WHEEL_CONTROL && (pData->options & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    // Fix bad note-off (per LV2 spec)
                    if (status == MIDI_STATUS_NOTE_ON && midiData[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    const uint32_t j     = fEventsIn.ctrlIndex;
                    const uint32_t mtime = isSampleAccurate ? startTime : eventTime;

                    // put back channel in data
                    uint8_t midiData2[4]; // FIXME
                    if (midiEvent.size > 4)
                        continue;
                    {
                        midiData2[0] = uint8_t(status | (event.channel & MIDI_CHANNEL_BIT));
                        std::memcpy(midiData2+1, midiData+1, static_cast<std::size_t>(midiEvent.size-1));
                    }

                    if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                        lv2_atom_buffer_write(&fEventsIn.iters[j].atom, mtime, 0, kUridMidiEvent, midiEvent.size, midiData2);

                    else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                        lv2_event_write(&fEventsIn.iters[j].event, mtime, 0, kUridMidiEvent, midiEvent.size, midiData2);

                    else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                        lv2midi_put_event(&fEventsIn.iters[j].midiState, mtime, midiEvent.size, midiData2);

                    if (status == MIDI_STATUS_NOTE_ON)
                    {
                        pData->postponeNoteOnRtEvent(true, event.channel, midiData[1], midiData[2]);
                    }
                    else if (status == MIDI_STATUS_NOTE_OFF)
                    {
                        pData->postponeNoteOffRtEvent(true, event.channel, midiData[1]);
                    }
                } break;
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(audioIn, audioOut, cvIn, cvOut, frames - timeOffset, timeOffset);

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(audioIn, audioOut, cvIn, cvOut, frames, 0);

        } // End of Plugin processing (no events)

        // --------------------------------------------------------------------------------------------------------
        // Final work

        if (fEventsIn.ctrl != nullptr && fExt.worker != nullptr && fAtomBufferWorkerResp.tryLock())
        {
            if (fAtomBufferWorkerResp.isDataAvailableForReading())
            {
                uint32_t portIndex;
                LV2_Atom* const atom = fAtomBufferRealtime;
                atom->size = fAtomBufferRealtimeSize;

                for (; fAtomBufferWorkerResp.get(portIndex, atom); atom->size = fAtomBufferRealtimeSize)
                {
                    CARLA_SAFE_ASSERT_CONTINUE(atom->type == kUridCarlaAtomWorkerResp);
                    fExt.worker->work_response(fHandle, atom->size, LV2_ATOM_BODY_CONST(atom));
                }
            }

            fAtomBufferWorkerResp.unlock();
        }

        if (fExt.worker != nullptr && fExt.worker->end_run != nullptr)
        {
            fExt.worker->end_run(fHandle);

            if (fHandle2 != nullptr)
                fExt.worker->end_run(fHandle2);
        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (pData->event.portOut != nullptr)
        {
            uint8_t  channel;
            uint16_t param;
            float    value;

            for (uint32_t k=0; k < pData->param.count; ++k)
            {
                if (pData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;

                if (fStrictBounds >= 0 && (pData->param.data[k].hints & PARAMETER_IS_STRICT_BOUNDS) != 0)
                    // plugin is responsible to ensure correct bounds
                    pData->param.ranges[k].fixValue(fParamBuffers[k]);

                if (pData->param.data[k].mappedControlIndex > 0)
                {
                    channel = pData->param.data[k].midiChannel;
                    param   = static_cast<uint16_t>(pData->param.data[k].mappedControlIndex);
                    value   = pData->param.ranges[k].getNormalizedValue(fParamBuffers[k]);
                    pData->event.portOut->writeControlEvent(0, channel, kEngineControlEventTypeParameter,
                                                            param, -1, value);
                }
            }
        } // End of Control Output
#endif

        // --------------------------------------------------------------------------------------------------------
        // Events/MIDI Output

        for (uint32_t i=0; i < fEventsOut.count; ++i)
        {
            uint32_t lastFrame = 0;
            Lv2EventData& evData(fEventsOut.data[i]);

            if (evData.type & CARLA_EVENT_DATA_ATOM)
            {
                const LV2_Atom_Event* ev;
                LV2_Atom_Buffer_Iterator iter;

                uint8_t* data;
                lv2_atom_buffer_begin(&iter, evData.atom);

                for (;;)
                {
                    data = nullptr;
                    ev = lv2_atom_buffer_get(&iter, &data);

                    if (ev == nullptr || ev->body.size == 0 || data == nullptr)
                        break;

                    if (ev->body.type == kUridMidiEvent)
                    {
                        if (evData.port != nullptr)
                        {
                            CARLA_SAFE_ASSERT_CONTINUE(ev->time.frames >= 0);
                            CARLA_SAFE_ASSERT_CONTINUE(ev->body.size < 0xFF);

                            uint32_t currentFrame = static_cast<uint32_t>(ev->time.frames);
                            if (currentFrame < lastFrame)
                                currentFrame = lastFrame;
                            else if (currentFrame >= frames)
                                currentFrame = frames - 1;

                            evData.port->writeMidiEvent(currentFrame, static_cast<uint8_t>(ev->body.size), data);
                        }
                    }
                    else if (fAtomBufferUiOutTmpData != nullptr)
                    {
                        fAtomBufferUiOut.put(&ev->body, evData.rindex);
                    }

                    lv2_atom_buffer_increment(&iter);
                }
            }
            else if ((evData.type & CARLA_EVENT_DATA_EVENT) != 0 && evData.port != nullptr)
            {
                const LV2_Event* ev;
                LV2_Event_Iterator iter;

                uint8_t* data;
                lv2_event_begin(&iter, evData.event);

                for (;;)
                {
                    data = nullptr;
                    ev = lv2_event_get(&iter, &data);

                    if (ev == nullptr || data == nullptr)
                        break;

                    uint32_t currentFrame = ev->frames;
                    if (currentFrame < lastFrame)
                        currentFrame = lastFrame;
                    else if (currentFrame >= frames)
                        currentFrame = frames - 1;

                    if (ev->type == kUridMidiEvent)
                    {
                        CARLA_SAFE_ASSERT_CONTINUE(ev->size < 0xFF);
                        evData.port->writeMidiEvent(currentFrame, static_cast<uint8_t>(ev->size), data);
                    }

                    lv2_event_increment(&iter);
                }
            }
            else if ((evData.type & CARLA_EVENT_DATA_MIDI_LL) != 0 && evData.port != nullptr)
            {
                LV2_MIDIState state = { &evData.midi, frames, 0 };

                uint32_t eventSize;
                double   eventTime;
                uchar* eventData;

                for (;;)
                {
                    eventSize = 0;
                    eventTime = 0.0;
                    eventData = nullptr;
                    lv2midi_get_event(&state, &eventTime, &eventSize, &eventData);

                    if (eventData == nullptr || eventSize == 0)
                        break;

                    CARLA_SAFE_ASSERT_CONTINUE(eventSize < 0xFF);
                    CARLA_SAFE_ASSERT_CONTINUE(eventTime >= 0.0);

                    evData.port->writeMidiEvent(static_cast<uint32_t>(eventTime), static_cast<uint8_t>(eventSize), eventData);
                    lv2midi_step(&state);
                }
            }
        }

        fFirstActive = false;

        // --------------------------------------------------------------------------------------------------------
    }

    bool processSingle(const float* const* const audioIn, float** const audioOut,
                       const float* const* const cvIn, float** const cvOut,
                       const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioIn != nullptr, false);
            CARLA_SAFE_ASSERT_RETURN(fAudioInBuffers != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioOut != nullptr, false);
            CARLA_SAFE_ASSERT_RETURN(fAudioOutBuffers != nullptr, false);
        }
        if (pData->cvIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(cvIn != nullptr, false);
        }
        if (pData->cvOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(cvOut != nullptr, false);
        }

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

#ifndef STOAT_TEST_BUILD
        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else
#endif
        if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                for (uint32_t k=0; k < frames; ++k)
                    audioOut[i][k+timeOffset] = 0.0f;
            }
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
            {
                for (uint32_t k=0; k < frames; ++k)
                    cvOut[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set audio buffers

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
            carla_copyFloats(fAudioInBuffers[i], audioIn[i]+timeOffset, frames);

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
            carla_zeroFloats(fAudioOutBuffers[i], frames);

        // --------------------------------------------------------------------------------------------------------
        // Set CV buffers

        for (uint32_t i=0; i < pData->cvIn.count; ++i)
            carla_copyFloats(fCvInBuffers[i], cvIn[i]+timeOffset, frames);

        for (uint32_t i=0; i < pData->cvOut.count; ++i)
            carla_zeroFloats(fCvOutBuffers[i], frames);

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fDescriptor->run(fHandle, frames);

        if (fHandle2 != nullptr)
            fDescriptor->run(fHandle2, frames);

        // --------------------------------------------------------------------------------------------------------
        // Handle trigger parameters

        for (uint32_t k=0; k < pData->param.count; ++k)
        {
            if (pData->param.data[k].type != PARAMETER_INPUT)
                continue;

            if (pData->param.data[k].hints & PARAMETER_IS_TRIGGER)
            {
                if (carla_isNotEqual(fParamBuffers[k], pData->param.ranges[k].def))
                {
                    fParamBuffers[k] = pData->param.ranges[k].def;
                    pData->postponeParameterChangeRtEvent(true, static_cast<int32_t>(k), fParamBuffers[k]);
                }
            }
        }

        pData->postRtEvents.trySplice();

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
# ifndef BUILD_BRIDGE
                        if (k < pData->latency.frames && pData->latency.buffers != nullptr)
                            bufValue = pData->latency.buffers[c][k];
                        else if (pData->latency.frames < frames)
                            bufValue = fAudioInBuffers[c][k-pData->latency.frames];
                        else
# endif
                            bufValue = fAudioInBuffers[c][k];

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
                        audioOut[i][k+timeOffset] = fAudioOutBuffers[i][k] * pData->postProc.volume;
                }
            }
        } // End of Post-processing

# ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Save latency values for next callback

        if (pData->latency.frames != 0 && pData->latency.buffers != nullptr)
        {
            CARLA_SAFE_ASSERT(timeOffset == 0);
            const uint32_t latframes = pData->latency.frames;

            if (latframes <= frames)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                    carla_copyFloats(pData->latency.buffers[i], audioIn[i]+(frames-latframes), latframes);
            }
            else
            {
                const uint32_t diff = latframes - frames;

                for (uint32_t i=0, k; i<pData->audioIn.count; ++i)
                {
                    // push back buffer by 'frames'
                    for (k=0; k < diff; ++k)
                        pData->latency.buffers[i][k] = pData->latency.buffers[i][k+frames];

                    // put current input at the end
                    for (uint32_t j=0; k < latframes; ++j, ++k)
                        pData->latency.buffers[i][k] = audioIn[i][j];
                }
            }
        }
# endif
#else // BUILD_BRIDGE_ALTERNATIVE_ARCH
        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            for (uint32_t k=0; k < frames; ++k)
                audioOut[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
#endif

        for (uint32_t i=0; i < pData->cvOut.count; ++i)
        {
            for (uint32_t k=0; k < frames; ++k)
                cvOut[i][k+timeOffset] = fCvOutBuffers[i][k];
        }

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("CarlaPluginLV2::bufferSizeChanged(%i) - start", newBufferSize);

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
        {
            if (fAudioInBuffers[i] != nullptr)
                delete[] fAudioInBuffers[i];
            fAudioInBuffers[i] = new float[newBufferSize];
        }

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];
            fAudioOutBuffers[i] = new float[newBufferSize];
        }

        if (fHandle2 == nullptr)
        {
            for (uint32_t i=0; i < pData->audioIn.count; ++i)
            {
                CARLA_ASSERT(fAudioInBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, pData->audioIn.ports[i].rindex, fAudioInBuffers[i]);
            }

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                CARLA_ASSERT(fAudioOutBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, pData->audioOut.ports[i].rindex, fAudioOutBuffers[i]);
            }
        }
        else
        {
            if (pData->audioIn.count > 0)
            {
                CARLA_ASSERT(pData->audioIn.count == 2);
                CARLA_ASSERT(fAudioInBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioInBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  pData->audioIn.ports[0].rindex, fAudioInBuffers[0]);
                fDescriptor->connect_port(fHandle2, pData->audioIn.ports[1].rindex, fAudioInBuffers[1]);
            }

            if (pData->audioOut.count > 0)
            {
                CARLA_ASSERT(pData->audioOut.count == 2);
                CARLA_ASSERT(fAudioOutBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioOutBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  pData->audioOut.ports[0].rindex, fAudioOutBuffers[0]);
                fDescriptor->connect_port(fHandle2, pData->audioOut.ports[1].rindex, fAudioOutBuffers[1]);
            }
        }

        for (uint32_t i=0; i < pData->cvIn.count; ++i)
        {
            if (fCvInBuffers[i] != nullptr)
                delete[] fCvInBuffers[i];
            fCvInBuffers[i] = new float[newBufferSize];

            fDescriptor->connect_port(fHandle, pData->cvIn.ports[i].rindex, fCvInBuffers[i]);

            if (fHandle2 != nullptr)
                fDescriptor->connect_port(fHandle2, pData->cvIn.ports[i].rindex, fCvInBuffers[i]);
        }

        for (uint32_t i=0; i < pData->cvOut.count; ++i)
        {
            if (fCvOutBuffers[i] != nullptr)
                delete[] fCvOutBuffers[i];
            fCvOutBuffers[i] = new float[newBufferSize];

            fDescriptor->connect_port(fHandle, pData->cvOut.ports[i].rindex, fCvOutBuffers[i]);

            if (fHandle2 != nullptr)
                fDescriptor->connect_port(fHandle2, pData->cvOut.ports[i].rindex, fCvOutBuffers[i]);
        }

        const int newBufferSizeInt(static_cast<int>(newBufferSize));

        if (fLv2Options.maxBufferSize != newBufferSizeInt || (fLv2Options.minBufferSize != 1 && fLv2Options.minBufferSize != newBufferSizeInt))
        {
            fLv2Options.maxBufferSize = fLv2Options.nominalBufferSize = newBufferSizeInt;

            if (fLv2Options.minBufferSize != 1)
                fLv2Options.minBufferSize = newBufferSizeInt;

            if (fExt.options != nullptr && fExt.options->set != nullptr)
            {
                LV2_Options_Option options[4];
                carla_zeroStructs(options, 4);

                carla_copyStruct(options[0], fLv2Options.opts[CarlaPluginLV2Options::MaxBlockLenth]);
                carla_copyStruct(options[1], fLv2Options.opts[CarlaPluginLV2Options::NominalBlockLenth]);

                if (fLv2Options.minBufferSize != 1)
                    carla_copyStruct(options[2], fLv2Options.opts[CarlaPluginLV2Options::MinBlockLenth]);

                fExt.options->set(fHandle, options);
            }
        }

        carla_debug("CarlaPluginLV2::bufferSizeChanged(%i) - end", newBufferSize);

        CarlaPlugin::bufferSizeChanged(newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("CarlaPluginLV2::sampleRateChanged(%g) - start", newSampleRate);

        const float sampleRatef = static_cast<float>(newSampleRate);

        if (carla_isNotEqual(fLv2Options.sampleRate, sampleRatef))
        {
            fLv2Options.sampleRate = sampleRatef;

            if (fExt.options != nullptr && fExt.options->set != nullptr)
            {
                LV2_Options_Option options[2];
                carla_copyStruct(options[0], fLv2Options.opts[CarlaPluginLV2Options::SampleRate]);
                carla_zeroStruct(options[1]);

                fExt.options->set(fHandle, options);
            }
        }

        for (uint32_t k=0; k < pData->param.count; ++k)
        {
            if (pData->param.data[k].type != PARAMETER_INPUT)
                continue;
            if (pData->param.special[k] != PARAMETER_SPECIAL_SAMPLE_RATE)
                continue;

            fParamBuffers[k] = sampleRatef;
            pData->postponeParameterChangeRtEvent(true, static_cast<int32_t>(k), fParamBuffers[k]);
            break;
        }

        carla_debug("CarlaPluginLV2::sampleRateChanged(%g) - end", newSampleRate);
    }

    void offlineModeChanged(const bool isOffline) override
    {
        for (uint32_t k=0; k < pData->param.count; ++k)
        {
            if (pData->param.data[k].type == PARAMETER_INPUT && pData->param.special[k] == PARAMETER_SPECIAL_FREEWHEEL)
            {
                fParamBuffers[k] = isOffline ? pData->param.ranges[k].max : pData->param.ranges[k].min;
                pData->postponeParameterChangeRtEvent(true, static_cast<int32_t>(k), fParamBuffers[k]);
                break;
            }
        }
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void initBuffers() const noexcept override
    {
        fEventsIn.initBuffers();
        fEventsOut.initBuffers();

        CarlaPlugin::initBuffers();
    }

    void clearBuffers() noexcept override
    {
        carla_debug("CarlaPluginLV2::clearBuffers() - start");

        if (fAudioInBuffers != nullptr)
        {
            for (uint32_t i=0; i < pData->audioIn.count; ++i)
            {
                if (fAudioInBuffers[i] != nullptr)
                {
                    delete[] fAudioInBuffers[i];
                    fAudioInBuffers[i] = nullptr;
                }
            }

            delete[] fAudioInBuffers;
            fAudioInBuffers = nullptr;
        }

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

        if (fCvInBuffers != nullptr)
        {
            for (uint32_t i=0; i < pData->cvIn.count; ++i)
            {
                if (fCvInBuffers[i] != nullptr)
                {
                    delete[] fCvInBuffers[i];
                    fCvInBuffers[i] = nullptr;
                }
            }

            delete[] fCvInBuffers;
            fCvInBuffers = nullptr;
        }

        if (fCvOutBuffers != nullptr)
        {
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
            {
                if (fCvOutBuffers[i] != nullptr)
                {
                    delete[] fCvOutBuffers[i];
                    fCvOutBuffers[i] = nullptr;
                }
            }

            delete[] fCvOutBuffers;
            fCvOutBuffers = nullptr;
        }

        if (fParamBuffers != nullptr)
        {
            delete[] fParamBuffers;
            fParamBuffers = nullptr;
        }

        fEventsIn.clear(pData->event.portIn);
        fEventsOut.clear(pData->event.portOut);

        CarlaPlugin::clearBuffers();

        carla_debug("CarlaPluginLV2::clearBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    void uiParameterChange(const uint32_t index, const float value) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type != UI::TYPE_NULL || fFilePathURI.isNotEmpty(),);
        CARLA_SAFE_ASSERT_RETURN(index < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(pData->param.data[index].rindex >= 0,);

#ifndef LV2_UIS_ONLY_INPROCESS
        if (fUI.type == UI::TYPE_BRIDGE)
        {
            if (! fPipeServer.isPipeRunning())
                return;
        }
        else
#endif
        {
            if (fUI.handle == nullptr)
                return;
            if (fUI.descriptor == nullptr || fUI.descriptor->port_event == nullptr)
                return;
            if (fNeedsUiClose)
                return;
        }

        ParameterData& pdata(pData->param.data[index]);

        if (pdata.hints & PARAMETER_IS_NOT_SAVED)
        {
            int32_t rindex = pdata.rindex;
            CARLA_SAFE_ASSERT_RETURN(rindex - static_cast<int32_t>(fRdfDescriptor->PortCount) >= 0,);

            rindex -= static_cast<int32_t>(fRdfDescriptor->PortCount);
            CARLA_SAFE_ASSERT_RETURN(rindex < static_cast<int32_t>(fRdfDescriptor->ParameterCount),);

            const char* const uri = fRdfDescriptor->Parameters[rindex].URI;

#ifndef LV2_UIS_ONLY_INPROCESS
            if (fUI.type == UI::TYPE_BRIDGE)
            {
                fPipeServer.writeLv2ParameterMessage(uri, value);
            }
            else
#endif
            if (fEventsIn.ctrl != nullptr)
            {
                uint8_t atomBuf[256];
                LV2_Atom_Forge atomForge;
                initAtomForge(atomForge);
                lv2_atom_forge_set_buffer(&atomForge, atomBuf, sizeof(atomBuf));

                LV2_Atom_Forge_Frame forgeFrame;
                lv2_atom_forge_object(&atomForge, &forgeFrame, kUridNull, kUridPatchSet);

                lv2_atom_forge_key(&atomForge, kUridCarlaParameterChange);
                lv2_atom_forge_bool(&atomForge, true);

                lv2_atom_forge_key(&atomForge, kUridPatchProperty);
                lv2_atom_forge_urid(&atomForge, getCustomURID(uri));

                lv2_atom_forge_key(&atomForge, kUridPatchValue);

                switch (fRdfDescriptor->Parameters[rindex].Type)
                {
                case LV2_PARAMETER_TYPE_BOOL:
                    lv2_atom_forge_bool(&atomForge, value > 0.5f);
                    break;
                case LV2_PARAMETER_TYPE_INT:
                    lv2_atom_forge_int(&atomForge, static_cast<int32_t>(value + 0.5f));
                    break;
                case LV2_PARAMETER_TYPE_LONG:
                    lv2_atom_forge_long(&atomForge, static_cast<int64_t>(value + 0.5f));
                    break;
                case LV2_PARAMETER_TYPE_FLOAT:
                    lv2_atom_forge_float(&atomForge, value);
                    break;
                case LV2_PARAMETER_TYPE_DOUBLE:
                    lv2_atom_forge_double(&atomForge, value);
                    break;
                default:
                    carla_stderr2("uiParameterChange called for invalid parameter, abort!");
                    return;
                }

                lv2_atom_forge_pop(&atomForge, &forgeFrame);

                LV2_Atom* const atom((LV2_Atom*)atomBuf);
                CARLA_SAFE_ASSERT(atom->size < sizeof(atomBuf));

                fUI.descriptor->port_event(fUI.handle,
                                           fEventsIn.ctrl->rindex,
                                           lv2_atom_total_size(atom),
                                           kUridAtomTransferEvent,
                                           atom);
            }
        }
        else
        {
#ifndef LV2_UIS_ONLY_INPROCESS
            if (fUI.type == UI::TYPE_BRIDGE)
            {
                fPipeServer.writeControlMessage(static_cast<uint32_t>(pData->param.data[index].rindex), value);
            }
            else
#endif
            {
                fUI.descriptor->port_event(fUI.handle,
                                           static_cast<uint32_t>(pData->param.data[index].rindex),
                                           sizeof(float), kUridNull, &value);
            }
        }
    }

    void uiMidiProgramChange(const uint32_t index) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type != UI::TYPE_NULL || fFilePathURI.isNotEmpty(),);
        CARLA_SAFE_ASSERT_RETURN(index < pData->midiprog.count,);

#ifndef LV2_UIS_ONLY_INPROCESS
        if (fUI.type == UI::TYPE_BRIDGE)
        {
            if (fPipeServer.isPipeRunning())
                fPipeServer.writeMidiProgramMessage(pData->midiprog.data[index].bank, pData->midiprog.data[index].program);
        }
        else
#endif
        {
            if (fExt.uiprograms != nullptr && fExt.uiprograms->select_program != nullptr && ! fNeedsUiClose)
                fExt.uiprograms->select_program(fUI.handle, pData->midiprog.data[index].bank, pData->midiprog.data[index].program);
        }
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type != UI::TYPE_NULL || fFilePathURI.isNotEmpty(),);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
        CARLA_SAFE_ASSERT_RETURN(velo > 0 && velo < MAX_MIDI_VALUE,);

#if 0
        if (fUI.type == UI::TYPE_BRIDGE)
        {
            if (fPipeServer.isPipeRunning())
                fPipeServer.writeMidiNoteMessage(false, channel, note, velo);
        }
        else
        {
            if (fUI.handle != nullptr && fUI.descriptor != nullptr && fUI.descriptor->port_event != nullptr && fEventsIn.ctrl != nullptr && ! fNeedsUiClose)
            {
                LV2_Atom_MidiEvent midiEv;
                midiEv.atom.type = kUridMidiEvent;
                midiEv.atom.size = 3;
                midiEv.data[0] = uint8_t(MIDI_STATUS_NOTE_ON | (channel & MIDI_CHANNEL_BIT));
                midiEv.data[1] = note;
                midiEv.data[2] = velo;

                fUI.descriptor->port_event(fUI.handle, fEventsIn.ctrl->rindex, lv2_atom_total_size(midiEv), kUridAtomTransferEvent, &midiEv);
            }
        }
#endif
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type != UI::TYPE_NULL || fFilePathURI.isNotEmpty(),);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);

#if 0
        if (fUI.type == UI::TYPE_BRIDGE)
        {
            if (fPipeServer.isPipeRunning())
                fPipeServer.writeMidiNoteMessage(false, channel, note, 0);
        }
        else
        {
            if (fUI.handle != nullptr && fUI.descriptor != nullptr && fUI.descriptor->port_event != nullptr && fEventsIn.ctrl != nullptr && ! fNeedsUiClose)
            {
                LV2_Atom_MidiEvent midiEv;
                midiEv.atom.type = kUridMidiEvent;
                midiEv.atom.size = 3;
                midiEv.data[0] = uint8_t(MIDI_STATUS_NOTE_OFF | (channel & MIDI_CHANNEL_BIT));
                midiEv.data[1] = note;
                midiEv.data[2] = 0;

                fUI.descriptor->port_event(fUI.handle, fEventsIn.ctrl->rindex, lv2_atom_total_size(midiEv), kUridAtomTransferEvent, &midiEv);
            }
        }
#endif
    }

    // -------------------------------------------------------------------
    // Internal helper functions

    void cloneLV2Files(const CarlaPlugin& other) override
    {
        CARLA_SAFE_ASSERT_RETURN(other.getType() == PLUGIN_LV2,);

        const CarlaPluginLV2& otherLV2((const CarlaPluginLV2&)other);

        const File tmpDir(handleStateMapToAbsolutePath(false, false, true, "."));

        if (tmpDir.exists())
            tmpDir.deleteRecursively();

        const File otherStateDir(otherLV2.handleStateMapToAbsolutePath(false, false, false, "."));

        if (otherStateDir.exists())
            otherStateDir.copyDirectoryTo(tmpDir);

        const File otherTmpDir(otherLV2.handleStateMapToAbsolutePath(false, false, true, "."));

        if (otherTmpDir.exists())
            otherTmpDir.copyDirectoryTo(tmpDir);
    }

    void restoreLV2State(const bool temporary) noexcept override
    {
        if (fExt.state == nullptr || fExt.state->restore == nullptr)
            return;

        if (! temporary)
        {
            const File tmpDir(handleStateMapToAbsolutePath(false, false, true, "."));

            if (tmpDir.exists())
                tmpDir.deleteRecursively();
        }

        LV2_State_Status status = LV2_STATE_ERR_UNKNOWN;

        {
            const ScopedSingleProcessLocker spl(this, !fHasThreadSafeRestore);

            try {
                status = fExt.state->restore(fHandle,
                                             carla_lv2_state_retrieve,
                                             this,
                                             LV2_STATE_IS_POD,
                                             temporary ? fFeatures : fStateFeatures);
            } catch(...) {}

            if (fHandle2 != nullptr)
            {
                try {
                    fExt.state->restore(fHandle,
                                        carla_lv2_state_retrieve,
                                        this,
                                        LV2_STATE_IS_POD,
                                        temporary ? fFeatures : fStateFeatures);
                } catch(...) {}
            }
        }

        switch (status)
        {
        case LV2_STATE_SUCCESS:
            carla_debug("CarlaPluginLV2::updateLV2State() - success");
            break;
        case LV2_STATE_ERR_UNKNOWN:
            carla_stderr("CarlaPluginLV2::updateLV2State() - unknown error");
            break;
        case LV2_STATE_ERR_BAD_TYPE:
            carla_stderr("CarlaPluginLV2::updateLV2State() - error, bad type");
            break;
        case LV2_STATE_ERR_BAD_FLAGS:
            carla_stderr("CarlaPluginLV2::updateLV2State() - error, bad flags");
            break;
        case LV2_STATE_ERR_NO_FEATURE:
            carla_stderr("CarlaPluginLV2::updateLV2State() - error, missing feature");
            break;
        case LV2_STATE_ERR_NO_PROPERTY:
            carla_stderr("CarlaPluginLV2::updateLV2State() - error, missing property");
            break;
        case LV2_STATE_ERR_NO_SPACE:
            carla_stderr("CarlaPluginLV2::updateLV2State() - error, insufficient space");
            break;
        }
    }

    // -------------------------------------------------------------------

    bool isRealtimeSafe() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);

        for (uint32_t i=0; i < fRdfDescriptor->FeatureCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Features[i].URI, LV2_CORE__hardRTCapable) == 0)
                return true;
        }

        return false;
    }

    // -------------------------------------------------------------------

    bool isUiBridgeable(const uint32_t uiId) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(uiId < fRdfDescriptor->UICount, false);

#ifndef LV2_UIS_ONLY_INPROCESS
        const LV2_RDF_UI* const rdfUI(&fRdfDescriptor->UIs[uiId]);

        for (uint32_t i=0; i < rdfUI->FeatureCount; ++i)
        {
            const LV2_RDF_Feature& feat(rdfUI->Features[i]);

            if (! feat.Required)
                continue;
            if (std::strcmp(feat.URI, LV2_INSTANCE_ACCESS_URI) == 0)
                return false;
            if (std::strcmp(feat.URI, LV2_DATA_ACCESS_URI) == 0)
                return false;
        }

        // Calf UIs are mostly useless without their special graphs
        // but they can be crashy under certain conditions, so follow user preferences
        if (std::strstr(rdfUI->URI, "http://calf.sourceforge.net/plugins/gui/") != nullptr)
            return pData->engine->getOptions().preferUiBridges;

        // LSP-Plugins UIs make heavy use of URIDs, for which carla right now is very slow
        // FIXME after some optimization, remove this
        if (std::strstr(rdfUI->URI, "http://lsp-plug.in/ui/lv2/") != nullptr)
            return false;

        return true;
#else
        return false;
#endif
    }

    bool isUiResizable() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.rdfDescriptor != nullptr, false);

        for (uint32_t i=0; i < fUI.rdfDescriptor->FeatureCount; ++i)
        {
            if (std::strcmp(fUI.rdfDescriptor->Features[i].URI, LV2_UI__fixedSize) == 0)
                return false;
            if (std::strcmp(fUI.rdfDescriptor->Features[i].URI, LV2_UI__noUserResize) == 0)
                return false;
        }

        return true;
    }

    const char* getUiBridgeBinary(const LV2_Property type) const
    {
        String bridgeBinary(pData->engine->getOptions().binaryDir);

        if (bridgeBinary.isEmpty())
            return nullptr;

        switch (type)
        {
        case LV2_UI_GTK2:
            bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-lv2-gtk2";
            break;
        case LV2_UI_GTK3:
            bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-lv2-gtk3";
            break;
        case LV2_UI_QT4:
            bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-lv2-qt4";
            break;
        case LV2_UI_QT5:
            bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-lv2-qt5";
            break;
        case LV2_UI_COCOA:
            bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-lv2-cocoa";
            break;
        case LV2_UI_WINDOWS:
            bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-lv2-windows";
            break;
        case LV2_UI_X11:
            bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-lv2-x11";
            break;
#if 0
        case LV2_UI_EXTERNAL:
        case LV2_UI_OLD_EXTERNAL:
            bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-lv2-external";
            break;
#endif
        default:
            return nullptr;
        }

#ifdef CARLA_OS_WIN
        bridgeBinary += ".exe";
#endif

        if (! File(bridgeBinary.buffer()).existsAsFile())
            return nullptr;

        return carla_strdup_safe(bridgeBinary);
    }

    // -------------------------------------------------------------------

    void recheckExtensions()
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        carla_debug("CarlaPluginLV2::recheckExtensions()");

        fExt.options  = nullptr;
        fExt.programs = nullptr;
        fExt.state    = nullptr;
        fExt.worker   = nullptr;
        fExt.inlineDisplay = nullptr;

        for (uint32_t i=0; i < fRdfDescriptor->ExtensionCount; ++i)
        {
            const char* const extension = fRdfDescriptor->Extensions[i];
            CARLA_SAFE_ASSERT_CONTINUE(extension != nullptr);

            /**/ if (std::strcmp(extension, LV2_OPTIONS__interface) == 0)
                pData->hints |= PLUGIN_HAS_EXTENSION_OPTIONS;
            else if (std::strcmp(extension, LV2_PROGRAMS__Interface) == 0)
                pData->hints |= PLUGIN_HAS_EXTENSION_PROGRAMS;
            else if (std::strcmp(extension, LV2_STATE__interface) == 0)
                pData->hints |= PLUGIN_HAS_EXTENSION_STATE;
            else if (std::strcmp(extension, LV2_WORKER__interface) == 0)
                pData->hints |= PLUGIN_HAS_EXTENSION_WORKER;
            else if (std::strcmp(extension, LV2_INLINEDISPLAY__interface) == 0)
                pData->hints |= PLUGIN_HAS_EXTENSION_INLINE_DISPLAY;
            else if (std::strcmp(extension, LV2_MIDNAM__interface) == 0)
                pData->hints |= PLUGIN_HAS_EXTENSION_MIDNAM;
            else
                carla_stdout("Plugin '%s' has non-supported extension: '%s'", fRdfDescriptor->URI, extension);
        }

        // Fix for broken plugins, nasty!
        for (uint32_t i=0; i < fRdfDescriptor->FeatureCount; ++i)
        {
            const LV2_RDF_Feature& feature(fRdfDescriptor->Features[i]);

            if (std::strcmp(feature.URI, LV2_INLINEDISPLAY__queue_draw) == 0)
            {
                if (pData->hints & PLUGIN_HAS_EXTENSION_INLINE_DISPLAY)
                    break;

                carla_stdout("Plugin '%s' uses inline-display but does not set extension data, nasty!", fRdfDescriptor->URI);
                pData->hints |= PLUGIN_HAS_EXTENSION_INLINE_DISPLAY;
            }
            else if (std::strcmp(feature.URI, LV2_MIDNAM__update) == 0)
            {
                if (pData->hints & PLUGIN_HAS_EXTENSION_MIDNAM)
                    break;

                carla_stdout("Plugin '%s' uses midnam but does not set extension data, nasty!", fRdfDescriptor->URI);
                pData->hints |= PLUGIN_HAS_EXTENSION_MIDNAM;
            }
        }

        if (fDescriptor->extension_data != nullptr)
        {
            if (pData->hints & PLUGIN_HAS_EXTENSION_OPTIONS)
                fExt.options = (const LV2_Options_Interface*)fDescriptor->extension_data(LV2_OPTIONS__interface);

            if (pData->hints & PLUGIN_HAS_EXTENSION_PROGRAMS)
                fExt.programs = (const LV2_Programs_Interface*)fDescriptor->extension_data(LV2_PROGRAMS__Interface);

            if (pData->hints & PLUGIN_HAS_EXTENSION_STATE)
                fExt.state = (const LV2_State_Interface*)fDescriptor->extension_data(LV2_STATE__interface);

            if (pData->hints & PLUGIN_HAS_EXTENSION_WORKER)
                fExt.worker = (const LV2_Worker_Interface*)fDescriptor->extension_data(LV2_WORKER__interface);

            if (pData->hints & PLUGIN_HAS_EXTENSION_INLINE_DISPLAY)
                fExt.inlineDisplay = (const LV2_Inline_Display_Interface*)fDescriptor->extension_data(LV2_INLINEDISPLAY__interface);

            if (pData->hints & PLUGIN_HAS_EXTENSION_MIDNAM)
                fExt.midnam = (const LV2_Midnam_Interface*)fDescriptor->extension_data(LV2_MIDNAM__interface);

            // check if invalid
            if (fExt.options != nullptr && fExt.options->get == nullptr  && fExt.options->set == nullptr)
                fExt.options = nullptr;

            if (fExt.programs != nullptr && (fExt.programs->get_program == nullptr || fExt.programs->select_program == nullptr))
                fExt.programs = nullptr;

            if (fExt.state != nullptr && (fExt.state->save == nullptr || fExt.state->restore == nullptr))
                fExt.state = nullptr;

            if (fExt.worker != nullptr && fExt.worker->work == nullptr)
                fExt.worker = nullptr;

            if (fExt.inlineDisplay != nullptr)
            {
                if (fExt.inlineDisplay->render != nullptr)
                {
                    pData->hints |= PLUGIN_HAS_INLINE_DISPLAY;
                    pData->setCanDeleteLib(false);
                }
                else
                {
                    fExt.inlineDisplay = nullptr;
                }
            }

            if (fExt.midnam != nullptr && fExt.midnam->midnam == nullptr)
                fExt.midnam = nullptr;
        }

        CARLA_SAFE_ASSERT_RETURN(fLatencyIndex == -1,);

        int32_t iCtrl=0;
        for (uint32_t i=0, count=fRdfDescriptor->PortCount; i<count; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (! LV2_IS_PORT_CONTROL(portTypes))
                continue;

            const CarlaScopedValueSetter<int32_t> svs(iCtrl, iCtrl, iCtrl+1);

            if (! LV2_IS_PORT_OUTPUT(portTypes))
                continue;

            const LV2_Property portDesignation(fRdfDescriptor->Ports[i].Designation);

            if (! LV2_IS_PORT_DESIGNATION_LATENCY(portDesignation))
                continue;

            fLatencyIndex = iCtrl;
            break;
        }
    }

    // -------------------------------------------------------------------

    void updateUi()
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.handle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fUI.descriptor != nullptr,);
        carla_debug("CarlaPluginLV2::updateUi()");

        // update midi program
        if (fExt.uiprograms != nullptr && pData->midiprog.count > 0 && pData->midiprog.current >= 0)
        {
            const MidiProgramData& curData(pData->midiprog.getCurrent());
            fExt.uiprograms->select_program(fUI.handle, curData.bank, curData.program);
        }

        // update control ports
        if (fUI.descriptor->port_event != nullptr)
        {
            float value;
            for (uint32_t i=0; i < pData->param.count; ++i)
            {
                value = getParameterValue(i);
                fUI.descriptor->port_event(fUI.handle, static_cast<uint32_t>(pData->param.data[i].rindex), sizeof(float), kUridNull, &value);
            }
        }
    }

    void inspectAtomForParameterChange(const LV2_Atom* const atom)
    {
        if (atom->type != kUridAtomBlank && atom->type != kUridAtomObject)
            return;

        const LV2_Atom_Object_Body* const objbody = (const LV2_Atom_Object_Body*)(atom + 1);

        if (objbody->otype != kUridPatchSet)
            return;

        const LV2_Atom_URID *property   = NULL;
        const LV2_Atom_Bool *carlaParam = NULL;
        const LV2_Atom      *value      = NULL;

        lv2_atom_object_body_get(atom->size, objbody,
                                  kUridCarlaParameterChange, (const LV2_Atom**)&carlaParam,
                                  kUridPatchProperty,        (const LV2_Atom**)&property,
                                  kUridPatchValue,            &value,
                                  0);

        if (carlaParam != nullptr && carlaParam->body != 0)
            return;

        if (property == nullptr || value == nullptr)
            return;

        switch (value->type)
        {
        case kUridAtomBool:
        case kUridAtomInt:
        //case kUridAtomLong:
        case kUridAtomFloat:
        case kUridAtomDouble:
            break;
        default:
            return;
        }

        uint32_t parameterId;
        if (! getParameterIndexForURID(property->body, parameterId))
            return;

        const uint8_t* const vbody = (const uint8_t*)(value + 1);
        float rvalue;

        switch (value->type)
        {
        case kUridAtomBool:
            rvalue = *(const int32_t*)vbody != 0 ? 1.0f : 0.0f;
            break;
        case kUridAtomInt:
            rvalue = static_cast<float>(*(const int32_t*)vbody);
            break;
        /*
        case kUridAtomLong:
            rvalue = *(int64_t*)vbody;
            break;
        */
        case kUridAtomFloat:
            rvalue = *(const float*)vbody;
            break;
        case kUridAtomDouble:
            rvalue = static_cast<float>(*(const double*)vbody);
            break;
        default:
            rvalue = 0.0f;
            break;
        }

        rvalue = pData->param.getFixedValue(parameterId, rvalue);
        fParamBuffers[parameterId] = rvalue;

        CarlaPlugin::setParameterValue(parameterId, rvalue, false, true, true);
    }

    bool getParameterIndexForURI(const char* const uri, uint32_t& parameterId) noexcept
    {
        parameterId = UINT32_MAX;

        for (uint32_t i=0; i < fRdfDescriptor->ParameterCount; ++i)
        {
            const LV2_RDF_Parameter& rdfParam(fRdfDescriptor->Parameters[i]);

            switch (rdfParam.Type)
            {
            case LV2_PARAMETER_TYPE_BOOL:
            case LV2_PARAMETER_TYPE_INT:
            // case LV2_PARAMETER_TYPE_LONG:
            case LV2_PARAMETER_TYPE_FLOAT:
            case LV2_PARAMETER_TYPE_DOUBLE:
                break;
            default:
                continue;
            }

            if (std::strcmp(rdfParam.URI, uri) == 0)
            {
                const int32_t rindex = static_cast<int32_t>(fRdfDescriptor->PortCount + i);

                for (uint32_t j=0; j < pData->param.count; ++j)
                {
                    if (pData->param.data[j].rindex == rindex)
                    {
                        parameterId = j;
                        break;
                    }
                }
                break;
            }
        }

        return (parameterId != UINT32_MAX);
    }

    bool getParameterIndexForURID(const LV2_URID urid, uint32_t& parameterId) noexcept
    {
        parameterId = UINT32_MAX;

        if (urid >= fCustomURIDs.size())
            return false;

        for (uint32_t i=0; i < fRdfDescriptor->ParameterCount; ++i)
        {
            const LV2_RDF_Parameter& rdfParam(fRdfDescriptor->Parameters[i]);

            switch (rdfParam.Type)
            {
            case LV2_PARAMETER_TYPE_BOOL:
            case LV2_PARAMETER_TYPE_INT:
            // case LV2_PARAMETER_TYPE_LONG:
            case LV2_PARAMETER_TYPE_FLOAT:
            case LV2_PARAMETER_TYPE_DOUBLE:
                break;
            default:
                continue;
            }

            const std::string& uri(fCustomURIDs[urid]);

            if (uri != rdfParam.URI)
                continue;

            const int32_t rindex = static_cast<int32_t>(fRdfDescriptor->PortCount + i);

              for (uint32_t j=0; j < pData->param.count; ++j)
              {
                  if (pData->param.data[j].rindex == rindex)
                  {
                      parameterId = j;
                      break;
                  }
              }
              break;
        }

        return (parameterId != UINT32_MAX);
    }

    // -------------------------------------------------------------------

    LV2_URID getCustomURID(const char* const uri)
    {
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', kUridNull);
        carla_debug("CarlaPluginLV2::getCustomURID(\"%s\")", uri);

        const std::string    s_uri(uri);
        const std::ptrdiff_t s_pos(std::find(fCustomURIDs.begin(), fCustomURIDs.end(), s_uri) - fCustomURIDs.begin());

        if (s_pos <= 0 || s_pos >= INT32_MAX)
            return kUridNull;

        const LV2_URID urid     = static_cast<LV2_URID>(s_pos);
        const LV2_URID uriCount = static_cast<LV2_URID>(fCustomURIDs.size());

        if (urid < uriCount)
            return urid;

        CARLA_SAFE_ASSERT(urid == uriCount);

        fCustomURIDs.push_back(uri);

#ifndef LV2_UIS_ONLY_INPROCESS
        if (fUI.type == UI::TYPE_BRIDGE && fPipeServer.isPipeRunning())
            fPipeServer.writeLv2UridMessage(urid, uri);
#endif

        return urid;
    }

    const char* getCustomURIDString(const LV2_URID urid) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(urid != kUridNull, kUnmapFallback);
        CARLA_SAFE_ASSERT_RETURN(urid < fCustomURIDs.size(), kUnmapFallback);
        carla_debug("CarlaPluginLV2::getCustomURIString(%i)", urid);

        return fCustomURIDs[urid].c_str();
    }

    // -------------------------------------------------------------------

    void handleProgramChanged(const int32_t index)
    {
        CARLA_SAFE_ASSERT_RETURN(index >= -1,);
        carla_debug("CarlaPluginLV2::handleProgramChanged(%i)", index);

        if (index == -1)
        {
            const ScopedSingleProcessLocker spl(this, true);
            return reloadPrograms(false);
        }

        if (index < static_cast<int32_t>(pData->midiprog.count) && fExt.programs != nullptr && fExt.programs->get_program != nullptr)
        {
            if (const LV2_Program_Descriptor* const progDesc = fExt.programs->get_program(fHandle, static_cast<uint32_t>(index)))
            {
                CARLA_SAFE_ASSERT_RETURN(progDesc->name != nullptr,);

                if (pData->midiprog.data[index].name != nullptr)
                    delete[] pData->midiprog.data[index].name;

                pData->midiprog.data[index].name = carla_strdup(progDesc->name);

                if (index == pData->midiprog.current)
                    pData->engine->callback(true, true, ENGINE_CALLBACK_UPDATE, pData->id, 0, 0, 0, 0.0, nullptr);
                else
                    pData->engine->callback(true, true, ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0, 0.0, nullptr);
            }
        }
    }

    // -------------------------------------------------------------------

    LV2_Resize_Port_Status handleResizePort(const uint32_t index, const size_t size)
    {
        CARLA_SAFE_ASSERT_RETURN(size > 0, LV2_RESIZE_PORT_ERR_UNKNOWN);
        carla_debug("CarlaPluginLV2::handleResizePort(%i, " P_SIZE ")", index, size);

        // TODO
        return LV2_RESIZE_PORT_ERR_NO_SPACE;
        (void)index;
    }

    // -------------------------------------------------------------------

    char* handleStateMapToAbstractPath(const bool temporary, const char* const absolutePath) const
    {
        // may already be an abstract path
        if (! File::isAbsolutePath(absolutePath))
            return strdup(absolutePath);

        File projectDir, targetDir;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (const char* const projFolder = pData->engine->getCurrentProjectFolder())
            projectDir = projFolder;
        else
#endif
            projectDir = File::getCurrentWorkingDirectory();

        if (projectDir.isNull())
        {
            carla_stdout("Project directory not set, cannot map absolutePath %s", absolutePath);
            return nullptr;
        }

        String basedir(pData->engine->getName());

        if (temporary)
            basedir += ".tmp";

        targetDir = projectDir.getChildFile(basedir)
                              .getChildFile(getName());

        if (! targetDir.exists())
            targetDir.createDirectory();

        const File wabsolutePath(absolutePath);

        // we may be saving to non-tmp path, let's check
        if (! temporary)
        {
            const File tmpDir = projectDir.getChildFile(basedir + ".tmp")
                                          .getChildFile(getName());

            if (wabsolutePath.getFullPathName().startsWith(tmpDir.getFullPathName()))
            {
                // gotcha, the temporary path is now the real one
                targetDir = tmpDir;
            }
            else if (! wabsolutePath.getFullPathName().startsWith(targetDir.getFullPathName()))
            {
                // seems like a normal save, let's be nice and put a symlink
                const water::String abstractFilename(wabsolutePath.getFileName());
                const File targetPath(targetDir.getChildFile(abstractFilename.toRawUTF8()));

                wabsolutePath.createSymbolicLink(targetPath, true);

                carla_stdout("Creating symlink for '%s' in '%s'", absolutePath, targetDir.getFullPathName().toRawUTF8());
                return strdup(abstractFilename.toRawUTF8());
            }
        }

        carla_stdout("Mapping absolutePath '%s' relative to targetDir '%s'",
                     absolutePath, targetDir.getFullPathName().toRawUTF8());

        return strdup(wabsolutePath.getRelativePathFrom(targetDir).toRawUTF8());
    }

    File handleStateMapToAbsolutePath(const bool createDirIfNeeded,
                                      const bool symlinkIfNeeded,
                                      const bool temporary,
                                      const char* const abstractPath) const
    {
        File targetDir, targetPath;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (const char* const projFolder = pData->engine->getCurrentProjectFolder())
            targetDir = projFolder;
        else
#endif
            targetDir = File::getCurrentWorkingDirectory();

        if (targetDir.isNull())
        {
            carla_stdout("Project directory not set, cannot map abstractPath '%s'", abstractPath);
            return File();
        }

        String basedir(pData->engine->getName());

        if (temporary)
            basedir += ".tmp";

        targetDir = targetDir.getChildFile(basedir)
                             .getChildFile(getName());

        if (createDirIfNeeded && ! targetDir.exists())
            targetDir.createDirectory();

        if (File::isAbsolutePath(abstractPath))
        {
            File wabstractPath(abstractPath);
            targetPath = targetDir.getChildFile(wabstractPath.getFileName().toRawUTF8());

            if (symlinkIfNeeded)
            {
                carla_stdout("Creating symlink for '%s' in '%s'",
                             abstractPath, targetDir.getFullPathName().toRawUTF8());
                wabstractPath.createSymbolicLink(targetPath, true);
            }
        }
        else
        {
            targetPath = targetDir.getChildFile(abstractPath);
            targetDir  = targetPath.getParentDirectory();

            if (createDirIfNeeded && ! targetDir.exists())
                targetDir.createDirectory();
        }

        if (std::strcmp(abstractPath, ".") != 0)
            carla_stdout("Mapping abstractPath '%s' relative to targetDir '%s'",
                         abstractPath, targetDir.getFullPathName().toRawUTF8());

        return targetPath;
    }

    LV2_State_Status handleStateStore(const uint32_t key, const void* const value, const size_t size, const uint32_t type, const uint32_t flags)
    {
        CARLA_SAFE_ASSERT_RETURN(key != kUridNull, LV2_STATE_ERR_NO_PROPERTY);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr, LV2_STATE_ERR_NO_PROPERTY);
        CARLA_SAFE_ASSERT_RETURN(size > 0, LV2_STATE_ERR_NO_PROPERTY);
        CARLA_SAFE_ASSERT_RETURN(type != kUridNull, LV2_STATE_ERR_BAD_TYPE);

        // FIXME linuxsampler does not set POD flag
        // CARLA_SAFE_ASSERT_RETURN(flags & LV2_STATE_IS_POD, LV2_STATE_ERR_BAD_FLAGS);

        carla_debug("CarlaPluginLV2::handleStateStore(%i:\"%s\", %p, " P_SIZE ", %i:\"%s\", %i)",
                    key, carla_lv2_urid_unmap(this, key), value, size, type, carla_lv2_urid_unmap(this, type), flags);

        const char* const skey(carla_lv2_urid_unmap(this, key));
        const char* const stype(carla_lv2_urid_unmap(this, type));

        CARLA_SAFE_ASSERT_RETURN(skey != nullptr && skey != kUnmapFallback, LV2_STATE_ERR_BAD_TYPE);
        CARLA_SAFE_ASSERT_RETURN(stype != nullptr && stype != kUnmapFallback, LV2_STATE_ERR_BAD_TYPE);

        // Check if we already have this key
        for (LinkedList<CustomData>::Itenerator it = pData->custom.begin2(); it.valid(); it.next())
        {
            CustomData& cData(it.getValue(kCustomDataFallbackNC));
            CARLA_SAFE_ASSERT_CONTINUE(cData.isValid());

            if (std::strcmp(cData.key, skey) == 0)
            {
                // found it
                delete[] cData.value;

                if (type == kUridAtomString || type == kUridAtomPath)
                    cData.value = carla_strdup((const char*)value);
                else
                    cData.value = carla_strdup(String::asBase64(value, size));

                return LV2_STATE_SUCCESS;
            }
        }

        // Otherwise store it
        CustomData newData;
        newData.type = carla_strdup(stype);
        newData.key  = carla_strdup(skey);

        if (type == kUridAtomString || type == kUridAtomPath)
            newData.value = carla_strdup((const char*)value);
        else
            newData.value = carla_strdup(String::asBase64(value, size));

        pData->custom.append(newData);

        return LV2_STATE_SUCCESS;

        // unused
        (void)flags;
    }

    const void* handleStateRetrieve(const uint32_t key, size_t* const size, uint32_t* const type, uint32_t* const flags)
    {
        CARLA_SAFE_ASSERT_RETURN(key != kUridNull, nullptr);
        CARLA_SAFE_ASSERT_RETURN(size != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(type != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(flags != nullptr, nullptr);
        carla_debug("CarlaPluginLV2::handleStateRetrieve(%i, %p, %p, %p)", key, size, type, flags);

        const char* const skey(carla_lv2_urid_unmap(this, key));
        CARLA_SAFE_ASSERT_RETURN(skey != nullptr && skey != kUnmapFallback, nullptr);

        const char* stype = nullptr;
        const char* stringData = nullptr;

        for (LinkedList<CustomData>::Itenerator it = pData->custom.begin2(); it.valid(); it.next())
        {
            const CustomData& cData(it.getValue(kCustomDataFallback));
            CARLA_SAFE_ASSERT_CONTINUE(cData.isValid());

            if (std::strcmp(cData.key, skey) == 0)
            {
                stype      = cData.type;
                stringData = cData.value;
                break;
            }
        }

        if (stype == nullptr || stringData == nullptr)
        {
            carla_stderr("Plugin requested value for '%s' which is not available", skey);
            *size = *type = *flags = 0;
            return nullptr;
        }

        *type  = carla_lv2_urid_map(this, stype);
        *flags = LV2_STATE_IS_POD;

        if (*type == kUridAtomString || *type == kUridAtomPath)
        {
            *size = std::strlen(stringData);
            return stringData;
        }

        if (fLastStateChunk != nullptr)
        {
            std::free(fLastStateChunk);
            fLastStateChunk = nullptr;
        }

        std::vector<uint8_t> chunk;
        d_getChunkFromBase64String_impl(chunk, stringData);
        CARLA_SAFE_ASSERT_RETURN(chunk.size() > 0, nullptr);

        fLastStateChunk = std::malloc(chunk.size());
        CARLA_SAFE_ASSERT_RETURN(fLastStateChunk != nullptr, nullptr);

#ifdef CARLA_PROPER_CPP11_SUPPORT
        std::memcpy(fLastStateChunk, chunk.data(), chunk.size());
#else
        std::memcpy(fLastStateChunk, &chunk.front(), chunk.size());
#endif

        *size = chunk.size();
        return fLastStateChunk;
    }

    // -------------------------------------------------------------------

    LV2_Worker_Status handleWorkerSchedule(const uint32_t size, const void* const data)
    {
        CARLA_SAFE_ASSERT_RETURN(fExt.worker != nullptr && fExt.worker->work != nullptr, LV2_WORKER_ERR_UNKNOWN);
        CARLA_SAFE_ASSERT_RETURN(fEventsIn.ctrl != nullptr, LV2_WORKER_ERR_UNKNOWN);
        carla_debug("CarlaPluginLV2::handleWorkerSchedule(%i, %p)", size, data);

        if (pData->engine->isOffline())
        {
            fExt.worker->work(fHandle, carla_lv2_worker_respond, this, size, data);
            return LV2_WORKER_SUCCESS;
        }

        LV2_Atom atom;
        atom.size = size;
        atom.type = kUridCarlaAtomWorkerIn;

        return fAtomBufferWorkerIn.putChunk(&atom, data, fEventsOut.ctrlIndex) ? LV2_WORKER_SUCCESS : LV2_WORKER_ERR_NO_SPACE;
    }

    LV2_Worker_Status handleWorkerRespond(const uint32_t size, const void* const data)
    {
        CARLA_SAFE_ASSERT_RETURN(fExt.worker != nullptr && fExt.worker->work_response != nullptr, LV2_WORKER_ERR_UNKNOWN);
        carla_debug("CarlaPluginLV2::handleWorkerRespond(%i, %p)", size, data);

        LV2_Atom atom;
        atom.size = size;
        atom.type = kUridCarlaAtomWorkerResp;

        return fAtomBufferWorkerResp.putChunk(&atom, data, fEventsIn.ctrlIndex) ? LV2_WORKER_SUCCESS : LV2_WORKER_ERR_NO_SPACE;
    }

    // -------------------------------------------------------------------

    void handleInlineDisplayQueueRedraw()
    {
        switch (pData->engine->getProccessMode())
        {
        case ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
        case ENGINE_PROCESS_MODE_PATCHBAY:
            fInlineDisplayNeedsRedraw = true;
            break;
        default:
            break;
        }
    }

    const LV2_Inline_Display_Image_Surface* renderInlineDisplay(const uint32_t width, const uint32_t height) const
    {
        CARLA_SAFE_ASSERT_RETURN(fExt.inlineDisplay != nullptr && fExt.inlineDisplay->render != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(width > 0, nullptr);
        CARLA_SAFE_ASSERT_RETURN(height > 0, nullptr);

        return fExt.inlineDisplay->render(fHandle, width, height);
    }

    // -------------------------------------------------------------------

    void handleMidnamUpdate()
    {
        CARLA_SAFE_ASSERT_RETURN(fExt.midnam != nullptr,);

        if (fEventsIn.ctrl == nullptr)
            return;

        char* const midnam = fExt.midnam->midnam(fHandle);
        CARLA_SAFE_ASSERT_RETURN(midnam != nullptr,);

        fEventsIn.ctrl->port->setMetaData("http://www.midi.org/dtds/MIDINameDocument10.dtd", midnam, "text/xml");

        if (fExt.midnam->free != nullptr)
            fExt.midnam->free(midnam);
    }

    // -------------------------------------------------------------------

    LV2_ControlInputPort_Change_Status handleCtrlInPortChangeReq(const uint32_t rindex, const float value)
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr, LV2_CONTROL_INPUT_PORT_CHANGE_ERR_UNKNOWN);

        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            if (pData->param.data[i].rindex != static_cast<int32_t>(rindex))
                continue;

            const uint32_t index = i;
            const float fixedValue = pData->param.getFixedValue(index, value);
            fParamBuffers[index] = fixedValue;

            CarlaPlugin::setParameterValueRT(index, fixedValue, 0, true);
            return LV2_CONTROL_INPUT_PORT_CHANGE_SUCCESS;
        }

        return LV2_CONTROL_INPUT_PORT_CHANGE_ERR_INVALID_INDEX;
    }

    // -------------------------------------------------------------------

    void handleExternalUIClosed()
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type == UI::TYPE_EXTERNAL,);
        carla_debug("CarlaPluginLV2::handleExternalUIClosed()");

        fNeedsUiClose = true;
    }

    void handlePluginUIClosed() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type == UI::TYPE_EMBED,);
        CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_debug("CarlaPluginLV2::handlePluginUIClosed()");

        fNeedsUiClose = true;
    }

    void handlePluginUIResized(const uint width, const uint height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type == UI::TYPE_EMBED,);
        CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_debug("CarlaPluginLV2::handlePluginUIResized(%u, %u)", width, height);

        if (fUI.handle != nullptr && fExt.uiresize != nullptr)
            fExt.uiresize->ui_resize(fUI.handle, static_cast<int>(width), static_cast<int>(height));
    }

    // -------------------------------------------------------------------

    uint32_t handleUIPortMap(const char* const symbol) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(symbol != nullptr && symbol[0] != '\0', LV2UI_INVALID_PORT_INDEX);
        carla_debug("CarlaPluginLV2::handleUIPortMap(\"%s\")", symbol);

        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Ports[i].Symbol, symbol) == 0)
                return i;
        }

        return LV2UI_INVALID_PORT_INDEX;
    }

    LV2UI_Request_Value_Status handleUIRequestValue(const LV2_URID key,
                                                    const LV2_URID type,
                                                    const LV2_Feature* const* features)
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type != UI::TYPE_NULL, LV2UI_REQUEST_VALUE_ERR_UNKNOWN);
        carla_debug("CarlaPluginLV2::handleUIRequestValue(%u, %u, %p)", key, type, features);

        if (type != kUridAtomPath)
            return LV2UI_REQUEST_VALUE_ERR_UNSUPPORTED;

        const char* const uri = getCustomURIDString(key);
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri != kUnmapFallback, LV2UI_REQUEST_VALUE_ERR_UNKNOWN);

        // check if a file browser is already open
        if (fUI.fileNeededForURI != nullptr || fUI.fileBrowserOpen)
            return LV2UI_REQUEST_VALUE_BUSY;

        for (uint32_t i=0; i < fRdfDescriptor->ParameterCount; ++i)
        {
            if (fRdfDescriptor->Parameters[i].Type != LV2_PARAMETER_TYPE_PATH)
                continue;
            if (std::strcmp(fRdfDescriptor->Parameters[i].URI, uri) != 0)
                continue;

            // TODO file browser filters, also store label to use for title
            fUI.fileNeededForURI = uri;

            return LV2UI_REQUEST_VALUE_SUCCESS;
        }

        return LV2UI_REQUEST_VALUE_ERR_UNSUPPORTED;

        // may be unused
        (void)features;
    }

    int handleUIResize(const int width, const int height)
    {
        CARLA_SAFE_ASSERT_RETURN(width > 0, 1);
        CARLA_SAFE_ASSERT_RETURN(height > 0, 1);
        carla_debug("CarlaPluginLV2::handleUIResize(%i, %i)", width, height);

        if (fUI.embedded)
        {
            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_EMBED_UI_RESIZED,
                                    pData->id, width, height,
                                    0, 0.0f, nullptr);
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr, 1);
            fUI.window->setSize(static_cast<uint>(width), static_cast<uint>(height), true, true);
        }

        return 0;
    }

    void handleUITouch(const uint32_t rindex, const bool touch)
    {
        carla_debug("CarlaPluginLV2::handleUITouch(%u, %s)", rindex, bool2str(touch));

        uint32_t index = LV2UI_INVALID_PORT_INDEX;

        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            if (pData->param.data[i].rindex != static_cast<int32_t>(rindex))
                continue;
            index = i;
            break;
        }

        CARLA_SAFE_ASSERT_RETURN(index != LV2UI_INVALID_PORT_INDEX,);

        pData->engine->touchPluginParameter(pData->id, index, touch);
    }

    void handleUIWrite(const uint32_t rindex, const uint32_t bufferSize, const uint32_t format, const void* const buffer)
    {
        CARLA_SAFE_ASSERT_RETURN(buffer != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(bufferSize > 0,);
        carla_debug("CarlaPluginLV2::handleUIWrite(%i, %i, %i, %p)", rindex, bufferSize, format, buffer);

        uint32_t index = LV2UI_INVALID_PORT_INDEX;

        switch (format)
        {
        case kUridNull: {
            CARLA_SAFE_ASSERT_RETURN(rindex < fRdfDescriptor->PortCount,);
            CARLA_SAFE_ASSERT_RETURN(bufferSize == sizeof(float),);

            for (uint32_t i=0; i < pData->param.count; ++i)
            {
                if (pData->param.data[i].rindex != static_cast<int32_t>(rindex))
                    continue;
                index = i;
                break;
            }

            CARLA_SAFE_ASSERT_RETURN(index != LV2UI_INVALID_PORT_INDEX,);

            const float value(*(const float*)buffer);

            // check if we should feedback message back to UI
            bool sendGui = false;

            if (const uint32_t notifCount = fUI.rdfDescriptor->PortNotificationCount)
            {
                const char* const portSymbol = fRdfDescriptor->Ports[rindex].Symbol;

                for (uint32_t i=0; i < notifCount; ++i)
                {
                    const LV2_RDF_UI_PortNotification& portNotif(fUI.rdfDescriptor->PortNotifications[i]);

                    if (portNotif.Protocol != LV2_UI_PORT_PROTOCOL_FLOAT)
                        continue;

                    if (portNotif.Symbol != nullptr)
                    {
                        if (std::strcmp(portNotif.Symbol, portSymbol) != 0)
                            continue;
                    }
                    else if (portNotif.Index != rindex)
                    {
                        continue;
                    }

                    sendGui = true;
                    break;
                }
            }

            setParameterValue(index, value, sendGui, true, true);

        } break;

        case kUridAtomTransferAtom:
        case kUridAtomTransferEvent: {
            CARLA_SAFE_ASSERT_RETURN(bufferSize >= sizeof(LV2_Atom),);

            const LV2_Atom* const atom((const LV2_Atom*)buffer);

            // plugins sometimes fail on this, not good...
            const uint32_t totalSize = lv2_atom_total_size(atom);
            const uint32_t paddedSize = lv2_atom_pad_size(totalSize);

            if (bufferSize != totalSize && bufferSize != paddedSize)
                carla_stderr2("Warning: LV2 UI sending atom with invalid size %u! size: %u, padded-size: %u",
                              bufferSize, totalSize, paddedSize);

            for (uint32_t i=0; i < fEventsIn.count; ++i)
            {
                if (fEventsIn.data[i].rindex != rindex)
                    continue;
                index = i;
                break;
            }

            // for bad plugins
            if (index == LV2UI_INVALID_PORT_INDEX)
            {
                CARLA_SAFE_ASSERT(index != LV2UI_INVALID_PORT_INDEX); // FIXME
                index = fEventsIn.ctrlIndex;
            }

            fAtomBufferEvIn.put(atom, index);
        } break;

        default:
            carla_stdout("CarlaPluginLV2::handleUIWrite(%i, %i, %i:\"%s\", %p) - unknown format",
                         rindex, bufferSize, format, carla_lv2_urid_unmap(this, format), buffer);
            break;
        }
    }

    void handleUIBridgeParameter(const char* const uri, const float value)
    {
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr,);
        carla_debug("CarlaPluginLV2::handleUIBridgeParameter(%s, %f)", uri, static_cast<double>(value));

        uint32_t parameterId;

        if (getParameterIndexForURI(uri, parameterId))
            setParameterValue(parameterId, value, false, true, true);
    }

    // -------------------------------------------------------------------

    void handleLilvSetPortValue(const char* const portSymbol, const void* const value, const uint32_t size, const uint32_t type)
    {
        CARLA_SAFE_ASSERT_RETURN(portSymbol != nullptr && portSymbol[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(size > 0,);
        CARLA_SAFE_ASSERT_RETURN(type != kUridNull,);
        carla_debug("CarlaPluginLV2::handleLilvSetPortValue(\"%s\", %p, %i, %i)", portSymbol, value, size, type);

        int32_t rindex = -1;

        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Ports[i].Symbol, portSymbol) == 0)
            {
                rindex = static_cast<int32_t>(i);
                break;
            }
        }

        CARLA_SAFE_ASSERT_RETURN(rindex >= 0,);

        float paramValue;

        switch (type)
        {
        case kUridAtomBool:
            CARLA_SAFE_ASSERT_RETURN(size == sizeof(int32_t),);
            paramValue = *(const int32_t*)value != 0 ? 1.0f : 0.0f;
            break;
        case kUridAtomDouble:
            CARLA_SAFE_ASSERT_RETURN(size == sizeof(double),);
            paramValue = static_cast<float>((*(const double*)value));
            break;
        case kUridAtomFloat:
            CARLA_SAFE_ASSERT_RETURN(size == sizeof(float),);
            paramValue = *(const float*)value;
            break;
        case kUridAtomInt:
            CARLA_SAFE_ASSERT_RETURN(size == sizeof(int32_t),);
            paramValue = static_cast<float>(*(const int32_t*)value);
            break;
        case kUridAtomLong:
            CARLA_SAFE_ASSERT_RETURN(size == sizeof(int64_t),);
            paramValue = static_cast<float>(*(const int64_t*)value);
            break;
        default:
            carla_stdout("CarlaPluginLV2::handleLilvSetPortValue(\"%s\", %p, %i, %i:\"%s\") - unknown type",
                         portSymbol, value, size, type, carla_lv2_urid_unmap(this, type));
            return;
        }

        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            if (pData->param.data[i].rindex == rindex)
            {
                setParameterValueRT(i, paramValue, 0, true);
                break;
            }
        }
    }

    // -------------------------------------------------------------------

    void* getNativeHandle() const noexcept override
    {
        return fHandle;
    }

    const void* getNativeDescriptor() const noexcept override
    {
        return fDescriptor;
    }

#ifndef LV2_UIS_ONLY_INPROCESS
    uintptr_t getUiBridgeProcessId() const noexcept override
    {
        return fPipeServer.isPipeRunning() ? fPipeServer.getPID() : 0;
    }
#endif

    // -------------------------------------------------------------------

public:
    bool init(const CarlaPluginPtr plugin,
              const char* const name, const char* const uri, const uint options,
              const char*& needsArchBridge)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (uri == nullptr || uri[0] == '\0')
        {
            pData->engine->setLastError("null uri");
            return false;
        }

        const EngineOptions& opts(pData->engine->getOptions());

        // ---------------------------------------------------------------
        // Init LV2 World if needed, sets LV2_PATH for lilv

        Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());

        if (opts.pathLV2 != nullptr && opts.pathLV2[0] != '\0')
            lv2World.initIfNeeded(opts.pathLV2);
        else if (const char* const LV2_PATH = std::getenv("LV2_PATH"))
            lv2World.initIfNeeded(LV2_PATH);
        else
            lv2World.initIfNeeded(LILV_DEFAULT_LV2_PATH);

        // ---------------------------------------------------------------
        // get plugin from lv2_rdf (lilv)

        fRdfDescriptor = lv2_rdf_new(uri, true);

        if (fRdfDescriptor == nullptr)
        {
            pData->engine->setLastError("Failed to find the requested plugin");
            return false;
        }

#ifdef ADAPT_FOR_APPLE_SILLICON
        // ---------------------------------------------------------------
        // check if we can open this binary, might need a bridge

        {
            const CarlaMagic magic;

            if (const char* const output = magic.getFileDescription(fRdfDescriptor->Binary))
            {
# ifdef __aarch64__
                if (std::strstr(output, "arm64") == nullptr && std::strstr(output, "x86_64") != nullptr)
                    needsArchBridge = "x86_64";
# else
                if (std::strstr(output, "x86_64") == nullptr && std::strstr(output, "arm64") != nullptr)
                    needsArchBridge = "arm64";
# endif
                if (needsArchBridge != nullptr)
                    return false;
            }
        }
#endif // ADAPT_FOR_APPLE_SILLICON

        // ---------------------------------------------------------------
        // open DLL

#ifdef CARLA_OS_MAC
        // Binary might be in quarentine due to Apple stupid notarization rules, let's remove that if possible
        removeFileFromQuarantine(fRdfDescriptor->Binary);
#endif

        if (! pData->libOpen(fRdfDescriptor->Binary))
        {
            pData->engine->setLastError(pData->libError(fRdfDescriptor->Binary));
            return false;
        }

        // ---------------------------------------------------------------
        // try to get DLL main entry via new mode

        if (const LV2_Lib_Descriptor_Function libDescFn = pData->libSymbol<LV2_Lib_Descriptor_Function>("lv2_lib_descriptor"))
        {
            // -----------------------------------------------------------
            // all ok, get lib descriptor

            const LV2_Lib_Descriptor* const libDesc = libDescFn(fRdfDescriptor->Bundle, nullptr);

            if (libDesc == nullptr)
            {
                pData->engine->setLastError("Could not find the LV2 Descriptor");
                return false;
            }

            // -----------------------------------------------------------
            // get descriptor that matches URI (new mode)

            uint32_t i = 0;
            while ((fDescriptor = libDesc->get_plugin(libDesc->handle, i++)))
            {
                if (std::strcmp(fDescriptor->URI, uri) == 0)
                    break;
            }
        }
        else
        {
            // -----------------------------------------------------------
            // get DLL main entry (old mode)

            const LV2_Descriptor_Function descFn = pData->libSymbol<LV2_Descriptor_Function>("lv2_descriptor");

            if (descFn == nullptr)
            {
                pData->engine->setLastError("Could not find the LV2 Descriptor in the plugin library");
                return false;
            }

            // -----------------------------------------------------------
            // get descriptor that matches URI (old mode)

            uint32_t i = 0;
            while ((fDescriptor = descFn(i++)))
            {
                if (std::strcmp(fDescriptor->URI, uri) == 0)
                    break;
            }
        }

        if (fDescriptor == nullptr)
        {
            pData->engine->setLastError("Could not find the requested plugin URI in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // check supported port-types and features

        bool canContinue = true;

        // Check supported ports
        for (uint32_t j=0; j < fRdfDescriptor->PortCount; ++j)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[j].Types);

            if (! is_lv2_port_supported(portTypes))
            {
                if (! LV2_IS_PORT_OPTIONAL(fRdfDescriptor->Ports[j].Properties))
                {
                    pData->engine->setLastError("Plugin requires a port type that is not currently supported");
                    canContinue = false;
                    break;
                }
            }
        }

        // Check supported features
        for (uint32_t j=0; j < fRdfDescriptor->FeatureCount && canContinue; ++j)
        {
            const LV2_RDF_Feature& feature(fRdfDescriptor->Features[j]);

            if (std::strcmp(feature.URI, LV2_DATA_ACCESS_URI) == 0 || std::strcmp(feature.URI, LV2_INSTANCE_ACCESS_URI) == 0)
            {
                carla_stderr("Plugin DSP wants UI feature '%s', ignoring this", feature.URI);
            }
            else if (std::strcmp(feature.URI, LV2_BUF_SIZE__fixedBlockLength) == 0)
            {
                fNeedsFixedBuffers = true;
            }
            else if (std::strcmp(feature.URI, LV2_PORT_PROPS__supportsStrictBounds) == 0)
            {
                fStrictBounds = feature.Required ? 1 : 0;
            }
            else if (std::strcmp(feature.URI, LV2_STATE__loadDefaultState) == 0)
            {
                fHasLoadDefaultState = true;
            }
            else if (std::strcmp(feature.URI, LV2_STATE__threadSafeRestore) == 0)
            {
                fHasThreadSafeRestore = true;
            }
            else if (feature.Required && ! is_lv2_feature_supported(feature.URI))
            {
                String msg("Plugin wants a feature that is not supported:\n");
                msg += feature.URI;

                canContinue = false;
                pData->engine->setLastError(msg);
                break;
            }
        }

        if (! canContinue)
        {
            // error already set
            return false;
        }

        if (fNeedsFixedBuffers && ! pData->engine->usesConstantBufferSize())
        {
            pData->engine->setLastError("Cannot use this plugin under the current engine.\n"
                                        "The plugin requires a fixed block size which is not possible right now.");
            return false;
        }

        // ---------------------------------------------------------------
        // set icon

        if (std::strncmp(fDescriptor->URI, "http://distrho.sf.net/", 22) == 0)
            pData->iconName = carla_strdup_safe("distrho");

        // ---------------------------------------------------------------
        // set info

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else
            pData->name = pData->engine->getUniquePluginName(fRdfDescriptor->Name);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(plugin);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize options

        const int bufferSize = static_cast<int>(pData->engine->getBufferSize());

        fLv2Options.minBufferSize     = fNeedsFixedBuffers ? bufferSize : 1;
        fLv2Options.maxBufferSize     = bufferSize;
        fLv2Options.nominalBufferSize = bufferSize;
        fLv2Options.sampleRate        = static_cast<float>(pData->engine->getSampleRate());
        fLv2Options.transientWinId    = static_cast<int64_t>(opts.frontendWinId);

        uint32_t eventBufferSize = MAX_DEFAULT_BUFFER_SIZE;

        for (uint32_t j=0; j < fRdfDescriptor->PortCount; ++j)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[j].Types);

            if (LV2_IS_PORT_ATOM_SEQUENCE(portTypes) || LV2_IS_PORT_EVENT(portTypes) || LV2_IS_PORT_MIDI_LL(portTypes))
            {
                if (fRdfDescriptor->Ports[j].MinimumSize > eventBufferSize)
                    eventBufferSize = fRdfDescriptor->Ports[j].MinimumSize;
            }
        }

        fLv2Options.sequenceSize = static_cast<int>(eventBufferSize);

        fLv2Options.bgColor = opts.bgColor;
        fLv2Options.fgColor = opts.fgColor;
        fLv2Options.uiScale = opts.uiScale;

        // ---------------------------------------------------------------
        // initialize features (part 1)

        LV2_Event_Feature* const eventFt = new LV2_Event_Feature;
        eventFt->callback_data           = this;
        eventFt->lv2_event_ref           = carla_lv2_event_ref;
        eventFt->lv2_event_unref         = carla_lv2_event_unref;

        LV2_Log_Log* const logFt = new LV2_Log_Log;
        logFt->handle            = this;
        logFt->printf            = carla_lv2_log_printf;
        logFt->vprintf           = carla_lv2_log_vprintf;

        LV2_State_Free_Path* const stateFreePathFt = new LV2_State_Free_Path;
        stateFreePathFt->handle                    = this;
        stateFreePathFt->free_path                 = carla_lv2_state_free_path;

        LV2_State_Make_Path* const stateMakePathFt = new LV2_State_Make_Path;
        stateMakePathFt->handle                    = this;
        stateMakePathFt->path                      = carla_lv2_state_make_path_tmp;

        LV2_State_Map_Path* const stateMapPathFt = new LV2_State_Map_Path;
        stateMapPathFt->handle                   = this;
        stateMapPathFt->abstract_path            = carla_lv2_state_map_to_abstract_path_tmp;
        stateMapPathFt->absolute_path            = carla_lv2_state_map_to_absolute_path_tmp;

        LV2_Programs_Host* const programsFt = new LV2_Programs_Host;
        programsFt->handle                  = this;
        programsFt->program_changed         = carla_lv2_program_changed;

        LV2_Resize_Port_Resize* const rsPortFt = new LV2_Resize_Port_Resize;
        rsPortFt->data                         = this;
        rsPortFt->resize                       = carla_lv2_resize_port;

        LV2_RtMemPool_Pool* const rtMemPoolFt = new LV2_RtMemPool_Pool;
        lv2_rtmempool_init(rtMemPoolFt);

        LV2_RtMemPool_Pool_Deprecated* const rtMemPoolOldFt = new LV2_RtMemPool_Pool_Deprecated;
        lv2_rtmempool_init_deprecated(rtMemPoolOldFt);

        LV2_URI_Map_Feature* const uriMapFt = new LV2_URI_Map_Feature;
        uriMapFt->callback_data             = this;
        uriMapFt->uri_to_id                 = carla_lv2_uri_to_id;

        LV2_URID_Map* const uridMapFt = new LV2_URID_Map;
        uridMapFt->handle             = this;
        uridMapFt->map                = carla_lv2_urid_map;

        LV2_URID_Unmap* const uridUnmapFt = new LV2_URID_Unmap;
        uridUnmapFt->handle               = this;
        uridUnmapFt->unmap                = carla_lv2_urid_unmap;

        LV2_Worker_Schedule* const workerFt = new LV2_Worker_Schedule;
        workerFt->handle                    = this;
        workerFt->schedule_work             = carla_lv2_worker_schedule;

        LV2_Inline_Display* const inlineDisplay = new LV2_Inline_Display;
        inlineDisplay->handle                   = this;
        inlineDisplay->queue_draw               = carla_lv2_inline_display_queue_draw;

        LV2_Midnam* const midnam = new LV2_Midnam;
        midnam->handle = this;
        midnam->update = carla_lv2_midnam_update;

        LV2_ControlInputPort_Change_Request* const portChangeReq = new LV2_ControlInputPort_Change_Request;
        portChangeReq->handle = this;
        portChangeReq->request_change = carla_lv2_ctrl_in_port_change_req;

        // ---------------------------------------------------------------
        // initialize features (part 2)

        for (uint32_t j=0; j < kFeatureCountPlugin; ++j)
            fFeatures[j] = new LV2_Feature;

        fFeatures[kFeatureIdBufSizeBounded]->URI   = LV2_BUF_SIZE__boundedBlockLength;
        fFeatures[kFeatureIdBufSizeBounded]->data  = nullptr;

        fFeatures[kFeatureIdBufSizeFixed]->URI     = fNeedsFixedBuffers
                                                   ? LV2_BUF_SIZE__fixedBlockLength
                                                   : LV2_BUF_SIZE__boundedBlockLength;
        fFeatures[kFeatureIdBufSizeFixed]->data    = nullptr;

        fFeatures[kFeatureIdBufSizePowerOf2]->URI  = LV2_BUF_SIZE__powerOf2BlockLength;
        fFeatures[kFeatureIdBufSizePowerOf2]->data = nullptr;

        fFeatures[kFeatureIdEvent]->URI          = LV2_EVENT_URI;
        fFeatures[kFeatureIdEvent]->data         = eventFt;

        fFeatures[kFeatureIdHardRtCapable]->URI  = LV2_CORE__hardRTCapable;
        fFeatures[kFeatureIdHardRtCapable]->data = nullptr;

        fFeatures[kFeatureIdInPlaceBroken]->URI  = LV2_CORE__inPlaceBroken;
        fFeatures[kFeatureIdInPlaceBroken]->data = nullptr;

        fFeatures[kFeatureIdIsLive]->URI         = LV2_CORE__isLive;
        fFeatures[kFeatureIdIsLive]->data        = nullptr;

        fFeatures[kFeatureIdLogs]->URI       = LV2_LOG__log;
        fFeatures[kFeatureIdLogs]->data      = logFt;

        fFeatures[kFeatureIdOptions]->URI    = LV2_OPTIONS__options;
        fFeatures[kFeatureIdOptions]->data   = fLv2Options.opts;

        fFeatures[kFeatureIdPrograms]->URI   = LV2_PROGRAMS__Host;
        fFeatures[kFeatureIdPrograms]->data  = programsFt;

        fFeatures[kFeatureIdResizePort]->URI   = LV2_RESIZE_PORT__resize;
        fFeatures[kFeatureIdResizePort]->data  = rsPortFt;

        fFeatures[kFeatureIdRtMemPool]->URI  = LV2_RTSAFE_MEMORY_POOL__Pool;
        fFeatures[kFeatureIdRtMemPool]->data = rtMemPoolFt;

        fFeatures[kFeatureIdRtMemPoolOld]->URI  = LV2_RTSAFE_MEMORY_POOL_DEPRECATED_URI;
        fFeatures[kFeatureIdRtMemPoolOld]->data = rtMemPoolOldFt;

        fFeatures[kFeatureIdStateFreePath]->URI  = LV2_STATE__freePath;
        fFeatures[kFeatureIdStateFreePath]->data = stateFreePathFt;

        fFeatures[kFeatureIdStateMakePath]->URI  = LV2_STATE__makePath;
        fFeatures[kFeatureIdStateMakePath]->data = stateMakePathFt;

        fFeatures[kFeatureIdStateMapPath]->URI   = LV2_STATE__mapPath;
        fFeatures[kFeatureIdStateMapPath]->data  = stateMapPathFt;

        fFeatures[kFeatureIdStrictBounds]->URI   = LV2_PORT_PROPS__supportsStrictBounds;
        fFeatures[kFeatureIdStrictBounds]->data  = nullptr;

        fFeatures[kFeatureIdUriMap]->URI     = LV2_URI_MAP_URI;
        fFeatures[kFeatureIdUriMap]->data    = uriMapFt;

        fFeatures[kFeatureIdUridMap]->URI    = LV2_URID__map;
        fFeatures[kFeatureIdUridMap]->data   = uridMapFt;

        fFeatures[kFeatureIdUridUnmap]->URI  = LV2_URID__unmap;
        fFeatures[kFeatureIdUridUnmap]->data = uridUnmapFt;

        fFeatures[kFeatureIdWorker]->URI     = LV2_WORKER__schedule;
        fFeatures[kFeatureIdWorker]->data    = workerFt;

        fFeatures[kFeatureIdInlineDisplay]->URI  = LV2_INLINEDISPLAY__queue_draw;
        fFeatures[kFeatureIdInlineDisplay]->data = inlineDisplay;

        fFeatures[kFeatureIdMidnam]->URI  = LV2_MIDNAM__update;
        fFeatures[kFeatureIdMidnam]->data = midnam;

        fFeatures[kFeatureIdCtrlInPortChangeReq]->URI  = LV2_CONTROL_INPUT_PORT_CHANGE_REQUEST_URI;
        fFeatures[kFeatureIdCtrlInPortChangeReq]->data = portChangeReq;

        // ---------------------------------------------------------------
        // initialize features (part 3)

        LV2_State_Make_Path* const stateMakePathFt2 = new LV2_State_Make_Path;
        stateMakePathFt2->handle                    = this;
        stateMakePathFt2->path                      = carla_lv2_state_make_path_real;

        LV2_State_Map_Path* const stateMapPathFt2 = new LV2_State_Map_Path;
        stateMapPathFt2->handle                   = this;
        stateMapPathFt2->abstract_path            = carla_lv2_state_map_to_abstract_path_real;
        stateMapPathFt2->absolute_path            = carla_lv2_state_map_to_absolute_path_real;

        for (uint32_t j=0; j < kStateFeatureCountAll; ++j)
            fStateFeatures[j] = new LV2_Feature;

        fStateFeatures[kStateFeatureIdFreePath]->URI  = LV2_STATE__freePath;
        fStateFeatures[kStateFeatureIdFreePath]->data = stateFreePathFt;

        fStateFeatures[kStateFeatureIdMakePath]->URI  = LV2_STATE__makePath;
        fStateFeatures[kStateFeatureIdMakePath]->data = stateMakePathFt2;

        fStateFeatures[kStateFeatureIdMapPath]->URI   = LV2_STATE__mapPath;
        fStateFeatures[kStateFeatureIdMapPath]->data  = stateMapPathFt2;

        fStateFeatures[kStateFeatureIdWorker]->URI     = LV2_WORKER__schedule;
        fStateFeatures[kStateFeatureIdWorker]->data    = workerFt;

        // ---------------------------------------------------------------
        // initialize plugin

        try {
            fHandle = fDescriptor->instantiate(fDescriptor, pData->engine->getSampleRate(), fRdfDescriptor->Bundle, fFeatures);
        } catch(...) {}

        if (fHandle == nullptr)
        {
            pData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        recheckExtensions();

        // ---------------------------------------------------------------
        // set options

        pData->options = 0x0;

        if (fLatencyIndex >= 0 || getMidiOutCount() != 0 || fNeedsFixedBuffers)
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
        else if (options & PLUGIN_OPTION_FIXED_BUFFERS)
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (opts.forceStereo)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;
        else if (options & PLUGIN_OPTION_FORCE_STEREO)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;

        if (getMidiInCount() != 0)
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
        }

        if (fExt.programs != nullptr && (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES) == 0)
        {
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_MAP_PROGRAM_CHANGES))
                pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        }

        // ---------------------------------------------------------------
        // gui stuff

        if (fRdfDescriptor->UICount != 0)
            initUi();

        return true;

        // might be unused
        (void)needsArchBridge;
    }

    // -------------------------------------------------------------------

    void initUi()
    {
        // ---------------------------------------------------------------
        // find the most appropriate ui

        int eQt4, eQt5, eGtk2, eGtk3, eCocoa, eWindows, eX11, iCocoa, iWindows, iX11, iExt, iFinal;
        eQt4 = eQt5 = eGtk2 = eGtk3 = eCocoa = eWindows = eX11 = iCocoa = iWindows = iX11 = iExt = iFinal = -1;

#if defined(LV2_UIS_ONLY_BRIDGES)
        const bool preferUiBridges = true;
#elif defined(BUILD_BRIDGE)
        const bool preferUiBridges = false;
#else
        const bool preferUiBridges = pData->engine->getOptions().preferUiBridges;
#endif
        bool hasShowInterface = false;

        for (uint32_t i=0; i < fRdfDescriptor->UICount; ++i)
        {
            CARLA_SAFE_ASSERT_CONTINUE(fRdfDescriptor->UIs[i].URI != nullptr);

            const int ii(static_cast<int>(i));

            switch (fRdfDescriptor->UIs[i].Type)
            {
            case LV2_UI_QT4:
                if (isUiBridgeable(i))
                    eQt4 = ii;
                break;
            case LV2_UI_QT5:
                if (isUiBridgeable(i))
                    eQt5 = ii;
                break;
            case LV2_UI_GTK2:
                if (isUiBridgeable(i))
                    eGtk2 = ii;
                break;
            case LV2_UI_GTK3:
                if (isUiBridgeable(i))
                    eGtk3 = ii;
                break;
#ifdef CARLA_OS_MAC
            case LV2_UI_COCOA:
                if (isUiBridgeable(i) && preferUiBridges)
                    eCocoa = ii;
                iCocoa = ii;
                break;
#endif
#ifdef CARLA_OS_WIN
            case LV2_UI_WINDOWS:
                if (isUiBridgeable(i) && preferUiBridges)
                    eWindows = ii;
                iWindows = ii;
                break;
#endif
            case LV2_UI_X11:
                if (isUiBridgeable(i) && preferUiBridges)
                    eX11 = ii;
                iX11 = ii;
                break;
            case LV2_UI_EXTERNAL:
            case LV2_UI_OLD_EXTERNAL:
                iExt = ii;
                break;
            default:
                break;
            }
        }

        /**/ if (eQt4 >= 0)
            iFinal = eQt4;
        else if (eQt5 >= 0)
            iFinal = eQt5;
        else if (eGtk2 >= 0)
            iFinal = eGtk2;
        else if (eGtk3 >= 0)
            iFinal = eGtk3;
#ifdef CARLA_OS_MAC
        else if (eCocoa >= 0)
            iFinal = eCocoa;
#endif
#ifdef CARLA_OS_WIN
        else if (eWindows >= 0)
            iFinal = eWindows;
#endif
#ifdef HAVE_X11
        else if (eX11 >= 0)
            iFinal = eX11;
#endif
#ifndef LV2_UIS_ONLY_BRIDGES
# ifdef CARLA_OS_MAC
        else if (iCocoa >= 0)
            iFinal = iCocoa;
# endif
# ifdef CARLA_OS_WIN
        else if (iWindows >= 0)
            iFinal = iWindows;
# endif
# ifdef HAVE_X11
        else if (iX11 >= 0)
            iFinal = iX11;
# endif
#endif
        else if (iExt >= 0)
            iFinal = iExt;

#ifndef BUILD_BRIDGE
        if (iFinal < 0)
#endif
        {
            // no suitable UI found, see if there's one which supports ui:showInterface
            for (uint32_t i=0; i < fRdfDescriptor->UICount && ! hasShowInterface; ++i)
            {
                LV2_RDF_UI* const ui(&fRdfDescriptor->UIs[i]);

                for (uint32_t j=0; j < ui->ExtensionCount; ++j)
                {
                    CARLA_SAFE_ASSERT_CONTINUE(ui->Extensions[j] != nullptr);

                    if (std::strcmp(ui->Extensions[j], LV2_UI__showInterface) != 0)
                        continue;

                    iFinal = static_cast<int>(i);
                    hasShowInterface = true;
                    break;
                }
            }

            if (iFinal < 0)
            {
                carla_stderr("Failed to find an appropriate LV2 UI for this plugin");
                return;
            }
        }

        fUI.rdfDescriptor = &fRdfDescriptor->UIs[iFinal];

        // ---------------------------------------------------------------
        // check supported ui features

        bool canContinue = true;
        bool canDelete = true;

        for (uint32_t i=0; i < fUI.rdfDescriptor->FeatureCount; ++i)
        {
            const char* const uri(fUI.rdfDescriptor->Features[i].URI);
            CARLA_SAFE_ASSERT_CONTINUE(uri != nullptr && uri[0] != '\0');

            if (! is_lv2_ui_feature_supported(uri))
            {
                if (fUI.rdfDescriptor->Features[i].Required)
                {
                    carla_stderr("Plugin UI requires a feature that is not supported:\n%s", uri);
                    canContinue = false;
                    break;
                }

                carla_stderr("Plugin UI wants a feature that is not supported (ignored):\n%s", uri);
            }

            if (std::strcmp(uri, LV2_UI__makeResident) == 0 || std::strcmp(uri, LV2_UI__makeSONameResident) == 0)
                canDelete = false;
            else if (std::strcmp(uri, LV2_UI__requestValue) == 0)
                pData->hints |= PLUGIN_NEEDS_UI_MAIN_THREAD;
        }

        if (! canContinue)
        {
            fUI.rdfDescriptor = nullptr;
            return;
        }

        // ---------------------------------------------------------------
        // initialize ui according to type

        const LV2_Property uiType = fUI.rdfDescriptor->Type;

#ifndef LV2_UIS_ONLY_INPROCESS
        if (
            (iFinal == eQt4     ||
             iFinal == eQt5     ||
             iFinal == eGtk2    ||
             iFinal == eGtk3    ||
             iFinal == eCocoa   ||
             iFinal == eWindows ||
             iFinal == eX11)
#ifdef BUILD_BRIDGE
            && ! hasShowInterface
#endif
            )
        {
            // -----------------------------------------------------------
            // initialize ui-bridge

            if (const char* const bridgeBinary = getUiBridgeBinary(uiType))
            {
                carla_stdout("Will use UI-Bridge for '%s', binary: \"%s\"", pData->name, bridgeBinary);

                String uiTitle;

                if (pData->uiTitle.isNotEmpty())
                {
                    uiTitle = pData->uiTitle;
                }
                else
                {
                    uiTitle = pData->name;
                    uiTitle += " (GUI)";
                }

                fLv2Options.windowTitle = uiTitle.getAndReleaseBuffer();

                fUI.type = UI::TYPE_BRIDGE;
                fPipeServer.setData(bridgeBinary, fRdfDescriptor->URI, fUI.rdfDescriptor->URI);

                delete[] bridgeBinary;
                return;
            }

            if (iFinal == eQt4 || iFinal == eQt5 || iFinal == eGtk2 || iFinal == eGtk3)
            {
                carla_stderr2("Failed to find UI bridge binary for '%s', cannot use UI", pData->name);
                fUI.rdfDescriptor = nullptr;
                return;
            }
        }
#endif

#ifdef LV2_UIS_ONLY_BRIDGES
        carla_stderr2("Failed to get an UI working, canBridge:%s", bool2str(isUiBridgeable(static_cast<uint32_t>(iFinal))));
        fUI.rdfDescriptor = nullptr;
        return;
#endif

        // ---------------------------------------------------------------
        // open UI DLL

#ifdef CARLA_OS_MAC
        // Binary might be in quarentine due to Apple stupid notarization rules, let's remove that if possible
        removeFileFromQuarantine(fUI.rdfDescriptor->Binary);
#endif

        if (! pData->uiLibOpen(fUI.rdfDescriptor->Binary, canDelete))
        {
            carla_stderr2("Could not load UI library, error was:\n%s", pData->libError(fUI.rdfDescriptor->Binary));
            fUI.rdfDescriptor = nullptr;
            return;
        }

        // ---------------------------------------------------------------
        // get UI DLL main entry

        LV2UI_DescriptorFunction uiDescFn = pData->uiLibSymbol<LV2UI_DescriptorFunction>("lv2ui_descriptor");

        if (uiDescFn == nullptr)
        {
            carla_stderr2("Could not find the LV2UI Descriptor in the UI library");
            pData->uiLibClose();
            fUI.rdfDescriptor = nullptr;
            return;
        }

        // ---------------------------------------------------------------
        // get UI descriptor that matches UI URI

        uint32_t i = 0;
        while ((fUI.descriptor = uiDescFn(i++)))
        {
            if (std::strcmp(fUI.descriptor->URI, fUI.rdfDescriptor->URI) == 0)
                break;
        }

        if (fUI.descriptor == nullptr)
        {
            carla_stderr2("Could not find the requested GUI in the plugin UI library");
            pData->uiLibClose();
            fUI.rdfDescriptor = nullptr;
            return;
        }

        // ---------------------------------------------------------------
        // check if ui is usable

        switch (uiType)
        {
        case LV2_UI_NONE:
            carla_stdout("Will use LV2 Show Interface for '%s'", pData->name);
            fUI.type = UI::TYPE_EMBED;
            break;
        case LV2_UI_QT4:
            carla_stdout("Will use LV2 Qt4 UI for '%s', NOT!", pData->name);
            fUI.type = UI::TYPE_EMBED;
            break;
        case LV2_UI_QT5:
            carla_stdout("Will use LV2 Qt5 UI for '%s', NOT!", pData->name);
            fUI.type = UI::TYPE_EMBED;
            break;
        case LV2_UI_GTK2:
            carla_stdout("Will use LV2 Gtk2 UI for '%s', NOT!", pData->name);
            fUI.type = UI::TYPE_EMBED;
            break;
        case LV2_UI_GTK3:
            carla_stdout("Will use LV2 Gtk3 UI for '%s', NOT!", pData->name);
            fUI.type = UI::TYPE_EMBED;
            break;
#ifdef CARLA_OS_MAC
        case LV2_UI_COCOA:
            carla_stdout("Will use LV2 Cocoa UI for '%s'", pData->name);
            fUI.type = UI::TYPE_EMBED;
            break;
#endif
#ifdef CARLA_OS_WIN
        case LV2_UI_WINDOWS:
            carla_stdout("Will use LV2 Windows UI for '%s'", pData->name);
            fUI.type = UI::TYPE_EMBED;
            break;
#endif
        case LV2_UI_X11:
#ifdef HAVE_X11
            carla_stdout("Will use LV2 X11 UI for '%s'", pData->name);
#else
            carla_stdout("Will use LV2 X11 UI for '%s', NOT!", pData->name);
#endif
            fUI.type = UI::TYPE_EMBED;
            break;
        case LV2_UI_EXTERNAL:
        case LV2_UI_OLD_EXTERNAL:
            carla_stdout("Will use LV2 External UI for '%s'", pData->name);
            fUI.type = UI::TYPE_EXTERNAL;
            break;
        }

        if (fUI.type == UI::TYPE_NULL)
        {
            pData->uiLibClose();
            fUI.descriptor = nullptr;
            fUI.rdfDescriptor = nullptr;
            return;
        }

        // ---------------------------------------------------------------
        // initialize ui data

        {
            String uiTitle;

            if (pData->uiTitle.isNotEmpty())
            {
                uiTitle = pData->uiTitle;
            }
            else
            {
                uiTitle = pData->name;
                uiTitle += " (GUI)";
            }

            fLv2Options.windowTitle = uiTitle.getAndReleaseBuffer();
        }

        fLv2Options.opts[CarlaPluginLV2Options::WindowTitle].size  = (uint32_t)std::strlen(fLv2Options.windowTitle);
        fLv2Options.opts[CarlaPluginLV2Options::WindowTitle].value = fLv2Options.windowTitle;

        // ---------------------------------------------------------------
        // initialize ui features (part 1)

        LV2_Extension_Data_Feature* const uiDataFt = new LV2_Extension_Data_Feature;
        uiDataFt->data_access                      = fDescriptor->extension_data;

        LV2UI_Port_Map* const uiPortMapFt = new LV2UI_Port_Map;
        uiPortMapFt->handle               = this;
        uiPortMapFt->port_index           = carla_lv2_ui_port_map;

        LV2UI_Request_Value* const uiRequestValueFt = new LV2UI_Request_Value;
        uiRequestValueFt->handle                    = this;
        uiRequestValueFt->request                   = carla_lv2_ui_request_value;

        LV2UI_Resize* const uiResizeFt = new LV2UI_Resize;
        uiResizeFt->handle             = this;
        uiResizeFt->ui_resize          = carla_lv2_ui_resize;

        LV2UI_Touch* const uiTouchFt = new LV2UI_Touch;
        uiTouchFt->handle            = this;
        uiTouchFt->touch             = carla_lv2_ui_touch;

        LV2_External_UI_Host* const uiExternalHostFt = new LV2_External_UI_Host;
        uiExternalHostFt->ui_closed                  = carla_lv2_external_ui_closed;
        uiExternalHostFt->plugin_human_id            = fLv2Options.windowTitle;

        // ---------------------------------------------------------------
        // initialize ui features (part 2)

        for (uint32_t j=kFeatureCountPlugin; j < kFeatureCountAll; ++j)
            fFeatures[j] = new LV2_Feature;

        fFeatures[kFeatureIdUiDataAccess]->URI      = LV2_DATA_ACCESS_URI;
        fFeatures[kFeatureIdUiDataAccess]->data     = uiDataFt;

        fFeatures[kFeatureIdUiInstanceAccess]->URI  = LV2_INSTANCE_ACCESS_URI;
        fFeatures[kFeatureIdUiInstanceAccess]->data = fHandle;

        fFeatures[kFeatureIdUiIdleInterface]->URI  = LV2_UI__idleInterface;
        fFeatures[kFeatureIdUiIdleInterface]->data = nullptr;

        fFeatures[kFeatureIdUiFixedSize]->URI      = LV2_UI__fixedSize;
        fFeatures[kFeatureIdUiFixedSize]->data     = nullptr;

        fFeatures[kFeatureIdUiMakeResident]->URI   = LV2_UI__makeResident;
        fFeatures[kFeatureIdUiMakeResident]->data  = nullptr;

        fFeatures[kFeatureIdUiMakeResident2]->URI  = LV2_UI__makeSONameResident;
        fFeatures[kFeatureIdUiMakeResident2]->data = nullptr;

        fFeatures[kFeatureIdUiNoUserResize]->URI   = LV2_UI__noUserResize;
        fFeatures[kFeatureIdUiNoUserResize]->data  = nullptr;

        fFeatures[kFeatureIdUiParent]->URI         = LV2_UI__parent;
        fFeatures[kFeatureIdUiParent]->data        = nullptr;

        fFeatures[kFeatureIdUiPortMap]->URI        = LV2_UI__portMap;
        fFeatures[kFeatureIdUiPortMap]->data       = uiPortMapFt;

        fFeatures[kFeatureIdUiPortSubscribe]->URI  = LV2_UI__portSubscribe;
        fFeatures[kFeatureIdUiPortSubscribe]->data = nullptr;

        fFeatures[kFeatureIdUiRequestValue]->URI  = LV2_UI__requestValue;
        fFeatures[kFeatureIdUiRequestValue]->data = uiRequestValueFt;

        fFeatures[kFeatureIdUiResize]->URI       = LV2_UI__resize;
        fFeatures[kFeatureIdUiResize]->data      = uiResizeFt;

        fFeatures[kFeatureIdUiTouch]->URI        = LV2_UI__touch;
        fFeatures[kFeatureIdUiTouch]->data       = uiTouchFt;

        fFeatures[kFeatureIdExternalUi]->URI     = LV2_EXTERNAL_UI__Host;
        fFeatures[kFeatureIdExternalUi]->data    = uiExternalHostFt;

        fFeatures[kFeatureIdExternalUiOld]->URI  = LV2_EXTERNAL_UI_DEPRECATED_URI;
        fFeatures[kFeatureIdExternalUiOld]->data = uiExternalHostFt;

        // ---------------------------------------------------------------
        // initialize ui extensions

        if (fUI.descriptor->extension_data == nullptr)
            return;

        fExt.uiidle     = (const LV2UI_Idle_Interface*)fUI.descriptor->extension_data(LV2_UI__idleInterface);
        fExt.uishow     = (const LV2UI_Show_Interface*)fUI.descriptor->extension_data(LV2_UI__showInterface);
        fExt.uiresize   = (const LV2UI_Resize*)fUI.descriptor->extension_data(LV2_UI__resize);
        fExt.uiprograms = (const LV2_Programs_UI_Interface*)fUI.descriptor->extension_data(LV2_PROGRAMS__UIInterface);

        // check if invalid
        if (fExt.uiidle != nullptr && fExt.uiidle->idle == nullptr)
            fExt.uiidle = nullptr;

        if (fExt.uishow != nullptr && (fExt.uishow->show == nullptr || fExt.uishow->hide == nullptr))
            fExt.uishow = nullptr;

        if (fExt.uiresize != nullptr && fExt.uiresize->ui_resize == nullptr)
            fExt.uiresize = nullptr;

        if (fExt.uiprograms != nullptr && fExt.uiprograms->select_program == nullptr)
            fExt.uiprograms = nullptr;

        // don't use uiidle if external
        if (fUI.type == UI::TYPE_EXTERNAL)
            fExt.uiidle = nullptr;
    }

    // -------------------------------------------------------------------

    void handleTransferAtom(const uint32_t portIndex, const LV2_Atom* const atom)
    {
        CARLA_SAFE_ASSERT_RETURN(atom != nullptr,);
        carla_debug("CarlaPluginLV2::handleTransferAtom(%i, %p)", portIndex, atom);

        fAtomBufferEvIn.put(atom, portIndex);
    }

    void handleUridMap(const LV2_URID urid, const char* const uri)
    {
        CARLA_SAFE_ASSERT_RETURN(urid != kUridNull,);
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0',);
        carla_debug("CarlaPluginLV2::handleUridMap(%i v " P_SIZE ", \"%s\")", urid, fCustomURIDs.size()-1, uri);

        const std::size_t uriCount(fCustomURIDs.size());

        if (urid < uriCount)
        {
            const char* const ourURI(carla_lv2_urid_unmap(this, urid));
            CARLA_SAFE_ASSERT_RETURN(ourURI != nullptr && ourURI != kUnmapFallback,);

            if (std::strcmp(ourURI, uri) != 0)
            {
                carla_stderr2("PLUGIN :: wrong URI '%s' vs '%s'", ourURI,  uri);
            }
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(urid == uriCount,);
            fCustomURIDs.push_back(uri);
        }
    }

    // -------------------------------------------------------------------

    void writeAtomPath(const char* const path, const LV2_URID urid)
    {
        uint8_t atomBuf[4096];
        LV2_Atom_Forge atomForge;
        initAtomForge(atomForge);
        lv2_atom_forge_set_buffer(&atomForge, atomBuf, sizeof(atomBuf));

        LV2_Atom_Forge_Frame forgeFrame;
        lv2_atom_forge_object(&atomForge, &forgeFrame, kUridNull, kUridPatchSet);

        lv2_atom_forge_key(&atomForge, kUridCarlaParameterChange);
        lv2_atom_forge_bool(&atomForge, true);

        lv2_atom_forge_key(&atomForge, kUridPatchProperty);
        lv2_atom_forge_urid(&atomForge, urid);

        lv2_atom_forge_key(&atomForge, kUridPatchValue);
        lv2_atom_forge_path(&atomForge, path, static_cast<uint32_t>(std::strlen(path))+1);

        lv2_atom_forge_pop(&atomForge, &forgeFrame);

        LV2_Atom* const atom((LV2_Atom*)atomBuf);
        CARLA_SAFE_ASSERT(atom->size < sizeof(atomBuf));

        fAtomBufferEvIn.put(atom, fEventsIn.ctrlIndex);
    }

    // -------------------------------------------------------------------

private:
    LV2_Handle   fHandle;
    LV2_Handle   fHandle2;
    LV2_Feature* fFeatures[kFeatureCountAll+1];
    LV2_Feature* fStateFeatures[kStateFeatureCountAll+1];
    const LV2_Descriptor*     fDescriptor;
    const LV2_RDF_Descriptor* fRdfDescriptor;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float** fCvInBuffers;
    float** fCvOutBuffers;
    float*  fParamBuffers;

    bool    fHasLoadDefaultState : 1;
    bool    fHasThreadSafeRestore : 1;
    bool    fNeedsFixedBuffers : 1;
    bool    fNeedsUiClose  : 1;
    bool    fInlineDisplayNeedsRedraw : 1;
    int64_t fInlineDisplayLastRedrawTime;
    int32_t fLatencyIndex; // -1 if invalid
    int     fStrictBounds; // -1 unsupported, 0 optional, 1 required

    Lv2AtomRingBuffer fAtomBufferEvIn;
    Lv2AtomRingBuffer fAtomBufferUiOut;
    Lv2AtomRingBuffer fAtomBufferWorkerIn;
    Lv2AtomRingBuffer fAtomBufferWorkerResp;
    uint8_t*          fAtomBufferUiOutTmpData;
    uint8_t*          fAtomBufferWorkerInTmpData;
    LV2_Atom*         fAtomBufferRealtime;
    uint32_t          fAtomBufferRealtimeSize;

    CarlaPluginLV2EventData fEventsIn;
    CarlaPluginLV2EventData fEventsOut;
    CarlaPluginLV2Options   fLv2Options;
#ifndef LV2_UIS_ONLY_INPROCESS
    CarlaPipeServerLV2      fPipeServer;
#endif

    std::vector<std::string> fCustomURIDs;

    bool fFirstActive; // first process() call after activate()
    void* fLastStateChunk;
    EngineTimeInfo fLastTimeInfo;

    // if plugin provides path parameter, use it as fake "gui"
    String fFilePathURI;

    struct Extensions {
        const LV2_Options_Interface* options;
        const LV2_State_Interface* state;
        const LV2_Worker_Interface* worker;
        const LV2_Inline_Display_Interface* inlineDisplay;
        const LV2_Midnam_Interface* midnam;
        const LV2_Programs_Interface* programs;
        const LV2UI_Idle_Interface* uiidle;
        const LV2UI_Show_Interface* uishow;
        const LV2UI_Resize* uiresize;
        const LV2_Programs_UI_Interface* uiprograms;

        Extensions()
            : options(nullptr),
              state(nullptr),
              worker(nullptr),
              inlineDisplay(nullptr),
              midnam(nullptr),
              programs(nullptr),
              uiidle(nullptr),
              uishow(nullptr),
              uiresize(nullptr),
              uiprograms(nullptr) {}

        CARLA_DECLARE_NON_COPYABLE(Extensions);
    } fExt;

    struct UI {
        enum Type {
            TYPE_NULL = 0,
#ifndef LV2_UIS_ONLY_INPROCESS
            TYPE_BRIDGE,
#endif
            TYPE_EMBED,
            TYPE_EXTERNAL
        };

        Type type;
        LV2UI_Handle handle;
        LV2UI_Widget widget;
        const LV2UI_Descriptor* descriptor;
        const LV2_RDF_UI*       rdfDescriptor;

        bool embedded;
        bool fileBrowserOpen;
        const char* fileNeededForURI;
        CarlaPluginUI* window;

        UI()
            : type(TYPE_NULL),
              handle(nullptr),
              widget(nullptr),
              descriptor(nullptr),
              rdfDescriptor(nullptr),
              embedded(false),
              fileBrowserOpen(false),
              fileNeededForURI(nullptr),
              window(nullptr) {}

        ~UI()
        {
            CARLA_SAFE_ASSERT(handle == nullptr);
            CARLA_SAFE_ASSERT(widget == nullptr);
            CARLA_SAFE_ASSERT(descriptor == nullptr);
            CARLA_SAFE_ASSERT(rdfDescriptor == nullptr);
            CARLA_SAFE_ASSERT(! fileBrowserOpen);
            CARLA_SAFE_ASSERT(fileNeededForURI == nullptr);
            CARLA_SAFE_ASSERT(window == nullptr);
        }

        CARLA_DECLARE_NON_COPYABLE(UI);
    } fUI;

    // -------------------------------------------------------------------
    // Event Feature

    static uint32_t carla_lv2_event_ref(LV2_Event_Callback_Data callback_data, LV2_Event* event)
    {
        CARLA_SAFE_ASSERT_RETURN(callback_data != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(event != nullptr, 0);
        carla_debug("carla_lv2_event_ref(%p, %p)", callback_data, event);

        return 0;
    }

    static uint32_t carla_lv2_event_unref(LV2_Event_Callback_Data callback_data, LV2_Event* event)
    {
        CARLA_SAFE_ASSERT_RETURN(callback_data != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(event != nullptr, 0);
        carla_debug("carla_lv2_event_unref(%p, %p)", callback_data, event);

        return 0;
    }

    // -------------------------------------------------------------------
    // Logs Feature

    static int carla_lv2_log_printf(LV2_Log_Handle handle, LV2_URID type, const char* fmt, ...)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(type != kUridNull, 0);
        CARLA_SAFE_ASSERT_RETURN(fmt != nullptr, 0);

#ifndef DEBUG
        if (type == kUridLogTrace)
            return 0;
#endif

        va_list args;
        va_start(args, fmt);
        const int ret(carla_lv2_log_vprintf(handle, type, fmt, args));
        va_end(args);

        return ret;
    }

    static int carla_lv2_log_vprintf(LV2_Log_Handle handle, LV2_URID type, const char* fmt, va_list ap)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(type != kUridNull, 0);
        CARLA_SAFE_ASSERT_RETURN(fmt != nullptr, 0);

        int ret = 0;

        switch (type)
        {
        case kUridLogError:
            std::fprintf(stderr, "\x1b[31m");
            ret = std::vfprintf(stderr, fmt, ap);
            std::fprintf(stderr, "\x1b[0m");
            break;

        case kUridLogNote:
            ret = std::vfprintf(stdout, fmt, ap);
            break;

        case kUridLogTrace:
#ifdef DEBUG
            std::fprintf(stdout, "\x1b[30;1m");
            ret = std::vfprintf(stdout, fmt, ap);
            std::fprintf(stdout, "\x1b[0m");
#endif
            break;

        case kUridLogWarning:
            ret = std::vfprintf(stderr, fmt, ap);
            break;

        default:
            break;
        }

        return ret;
    }

    // -------------------------------------------------------------------
    // Programs Feature

    static void carla_lv2_program_changed(LV2_Programs_Handle handle, int32_t index)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);
        carla_debug("carla_lv2_program_changed(%p, %i)", handle, index);

        ((CarlaPluginLV2*)handle)->handleProgramChanged(index);
    }

    // -------------------------------------------------------------------
    // Resize Port Feature

    static LV2_Resize_Port_Status carla_lv2_resize_port(LV2_Resize_Port_Feature_Data data, uint32_t index, size_t size)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, LV2_RESIZE_PORT_ERR_UNKNOWN);
        carla_debug("carla_lv2_program_changed(%p, %i, " P_SIZE ")", data, index, size);

        return ((CarlaPluginLV2*)data)->handleResizePort(index, size);
    }

    // -------------------------------------------------------------------
    // State Feature

    static void carla_lv2_state_free_path(LV2_State_Free_Path_Handle handle, char* const path)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);
        carla_debug("carla_lv2_state_free_path(%p, \"%s\")", handle, path);

        std::free(path);
    }

    static char* carla_lv2_state_make_path_real(LV2_State_Make_Path_Handle handle, const char* path)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(path != nullptr, nullptr);
        carla_debug("carla_lv2_state_make_path_real(%p, \"%s\")", handle, path);

        // allow empty paths to mean "current dir"
        if (path[0] == '\0')
            path = ".";

        const File file(((CarlaPluginLV2*)handle)->handleStateMapToAbsolutePath(true, false, false, path));
        return file.isNotNull() ? strdup(file.getFullPathName().toRawUTF8()) : nullptr;
    }

    static char* carla_lv2_state_make_path_tmp(LV2_State_Make_Path_Handle handle, const char* path)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(path != nullptr, nullptr);
        carla_debug("carla_lv2_state_make_path_tmp(%p, \"%s\")", handle, path);

        // allow empty paths to mean "current dir"
        if (path[0] == '\0')
            path = ".";

        const File file(((CarlaPluginLV2*)handle)->handleStateMapToAbsolutePath(true, false, true, path));
        return file.isNotNull() ? strdup(file.getFullPathName().toRawUTF8()) : nullptr;
    }

    static char* carla_lv2_state_map_to_abstract_path_real(LV2_State_Map_Path_Handle handle, const char* const absolute_path)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(absolute_path != nullptr, nullptr);
        carla_debug("carla_lv2_state_map_to_abstract_path_real(%p, \"%s\")", handle, absolute_path);

        // handle invalid empty paths the same way as lilv
        if (absolute_path[0] == '\0')
            return strdup("");

        return ((CarlaPluginLV2*)handle)->handleStateMapToAbstractPath(false, absolute_path);
    }

    static char* carla_lv2_state_map_to_abstract_path_tmp(LV2_State_Map_Path_Handle handle, const char* const absolute_path)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(absolute_path != nullptr, nullptr);
        carla_debug("carla_lv2_state_map_to_abstract_path_tmp(%p, \"%s\")", handle, absolute_path);

        // handle invalid empty paths the same way as lilv
        if (absolute_path[0] == '\0')
            return strdup("");

        return ((CarlaPluginLV2*)handle)->handleStateMapToAbstractPath(true, absolute_path);
    }

    static char* carla_lv2_state_map_to_absolute_path_real(LV2_State_Map_Path_Handle handle, const char* abstract_path)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(abstract_path != nullptr, nullptr);
        carla_debug("carla_lv2_state_map_to_absolute_path_real(%p, \"%s\")", handle, abstract_path);

        // allow empty paths to mean "current dir"
        if (abstract_path[0] == '\0')
            abstract_path = ".";

        const File file(((CarlaPluginLV2*)handle)->handleStateMapToAbsolutePath(true, true, false, abstract_path));
        return file.isNotNull() ? strdup(file.getFullPathName().toRawUTF8()) : nullptr;
    }

    static char* carla_lv2_state_map_to_absolute_path_tmp(LV2_State_Map_Path_Handle handle, const char* abstract_path)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(abstract_path != nullptr, nullptr);
        carla_debug("carla_lv2_state_map_to_absolute_path_tmp(%p, \"%s\")", handle, abstract_path);

        // allow empty paths to mean "current dir"
        if (abstract_path[0] == '\0')
            abstract_path = ".";

        const File file(((CarlaPluginLV2*)handle)->handleStateMapToAbsolutePath(true, true, true, abstract_path));
        return file.isNotNull() ? strdup(file.getFullPathName().toRawUTF8()) : nullptr;
    }

    static LV2_State_Status carla_lv2_state_store(LV2_State_Handle handle, uint32_t key, const void* value, size_t size, uint32_t type, uint32_t flags)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, LV2_STATE_ERR_UNKNOWN);
        carla_debug("carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i)", handle, key, value, size, type, flags);

        return ((CarlaPluginLV2*)handle)->handleStateStore(key, value, size, type, flags);
    }

    static const void* carla_lv2_state_retrieve(LV2_State_Handle handle, uint32_t key, size_t* size, uint32_t* type, uint32_t* flags)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        carla_debug("carla_lv2_state_retrieve(%p, %i, %p, %p, %p)", handle, key, size, type, flags);

        return ((CarlaPluginLV2*)handle)->handleStateRetrieve(key, size, type, flags);
    }

    // -------------------------------------------------------------------
    // URI-Map Feature

    static uint32_t carla_lv2_uri_to_id(LV2_URI_Map_Callback_Data data, const char* map, const char* uri)
    {
        carla_debug("carla_lv2_uri_to_id(%p, \"%s\", \"%s\")", data, map, uri);
        return carla_lv2_urid_map((LV2_URID_Map_Handle*)data, uri);

        // unused
        (void)map;
    }

    // -------------------------------------------------------------------
    // URID Feature

    static LV2_URID carla_lv2_urid_map(LV2_URID_Map_Handle handle, const char* uri)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, kUridNull);
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', kUridNull);
        carla_debug("carla_lv2_urid_map(%p, \"%s\")", handle, uri);

        // Atom types
        if (std::strcmp(uri, LV2_ATOM__Blank) == 0)
            return kUridAtomBlank;
        if (std::strcmp(uri, LV2_ATOM__Bool) == 0)
            return kUridAtomBool;
        if (std::strcmp(uri, LV2_ATOM__Chunk) == 0)
            return kUridAtomChunk;
        if (std::strcmp(uri, LV2_ATOM__Double) == 0)
            return kUridAtomDouble;
        if (std::strcmp(uri, LV2_ATOM__Event) == 0)
            return kUridAtomEvent;
        if (std::strcmp(uri, LV2_ATOM__Float) == 0)
            return kUridAtomFloat;
        if (std::strcmp(uri, LV2_ATOM__Int) == 0)
            return kUridAtomInt;
        if (std::strcmp(uri, LV2_ATOM__Literal) == 0)
            return kUridAtomLiteral;
        if (std::strcmp(uri, LV2_ATOM__Long) == 0)
            return kUridAtomLong;
        if (std::strcmp(uri, LV2_ATOM__Number) == 0)
            return kUridAtomNumber;
        if (std::strcmp(uri, LV2_ATOM__Object) == 0)
            return kUridAtomObject;
        if (std::strcmp(uri, LV2_ATOM__Path) == 0)
            return kUridAtomPath;
        if (std::strcmp(uri, LV2_ATOM__Property) == 0)
            return kUridAtomProperty;
        if (std::strcmp(uri, LV2_ATOM__Resource) == 0)
            return kUridAtomResource;
        if (std::strcmp(uri, LV2_ATOM__Sequence) == 0)
            return kUridAtomSequence;
        if (std::strcmp(uri, LV2_ATOM__Sound) == 0)
            return kUridAtomSound;
        if (std::strcmp(uri, LV2_ATOM__String) == 0)
            return kUridAtomString;
        if (std::strcmp(uri, LV2_ATOM__Tuple) == 0)
            return kUridAtomTuple;
        if (std::strcmp(uri, LV2_ATOM__URI) == 0)
            return kUridAtomURI;
        if (std::strcmp(uri, LV2_ATOM__URID) == 0)
            return kUridAtomURID;
        if (std::strcmp(uri, LV2_ATOM__Vector) == 0)
            return kUridAtomVector;
        if (std::strcmp(uri, LV2_ATOM__atomTransfer) == 0)
            return kUridAtomTransferAtom;
        if (std::strcmp(uri, LV2_ATOM__eventTransfer) == 0)
            return kUridAtomTransferEvent;

        // BufSize types
        if (std::strcmp(uri, LV2_BUF_SIZE__maxBlockLength) == 0)
            return kUridBufMaxLength;
        if (std::strcmp(uri, LV2_BUF_SIZE__minBlockLength) == 0)
            return kUridBufMinLength;
        if (std::strcmp(uri, LV2_BUF_SIZE__nominalBlockLength) == 0)
            return kUridBufNominalLength;
        if (std::strcmp(uri, LV2_BUF_SIZE__sequenceSize) == 0)
            return kUridBufSequenceSize;

        // Log types
        if (std::strcmp(uri, LV2_LOG__Error) == 0)
            return kUridLogError;
        if (std::strcmp(uri, LV2_LOG__Note) == 0)
            return kUridLogNote;
        if (std::strcmp(uri, LV2_LOG__Trace) == 0)
            return kUridLogTrace;
        if (std::strcmp(uri, LV2_LOG__Warning) == 0)
            return kUridLogWarning;

        // Patch types
        if (std::strcmp(uri, LV2_PATCH__Set) == 0)
            return kUridPatchSet;
        if (std::strcmp(uri, LV2_PATCH__property) == 0)
            return kUridPatchProperty;
        if (std::strcmp(uri, LV2_PATCH__subject) == 0)
            return kUridPatchSubject;
        if (std::strcmp(uri, LV2_PATCH__value) == 0)
            return kUridPatchValue;

        // Time types
        if (std::strcmp(uri, LV2_TIME__Position) == 0)
            return kUridTimePosition;
        if (std::strcmp(uri, LV2_TIME__bar) == 0)
            return kUridTimeBar;
        if (std::strcmp(uri, LV2_TIME__barBeat) == 0)
            return kUridTimeBarBeat;
        if (std::strcmp(uri, LV2_TIME__beat) == 0)
            return kUridTimeBeat;
        if (std::strcmp(uri, LV2_TIME__beatUnit) == 0)
            return kUridTimeBeatUnit;
        if (std::strcmp(uri, LV2_TIME__beatsPerBar) == 0)
            return kUridTimeBeatsPerBar;
        if (std::strcmp(uri, LV2_TIME__beatsPerMinute) == 0)
            return kUridTimeBeatsPerMinute;
        if (std::strcmp(uri, LV2_TIME__frame) == 0)
            return kUridTimeFrame;
        if (std::strcmp(uri, LV2_TIME__framesPerSecond) == 0)
            return kUridTimeFramesPerSecond;
        if (std::strcmp(uri, LV2_TIME__speed) == 0)
            return kUridTimeSpeed;
        if (std::strcmp(uri, LV2_KXSTUDIO_PROPERTIES__TimePositionTicksPerBeat) == 0)
            return kUridTimeTicksPerBeat;

        // Others
        if (std::strcmp(uri, LV2_MIDI__MidiEvent) == 0)
            return kUridMidiEvent;
        if (std::strcmp(uri, LV2_PARAMETERS__sampleRate) == 0)
            return kUridParamSampleRate;
        if (std::strcmp(uri, LV2_UI__backgroundColor) == 0)
            return kUridBackgroundColor;
        if (std::strcmp(uri, LV2_UI__foregroundColor) == 0)
            return kUridForegroundColor;
#ifndef CARLA_OS_MAC
        if (std::strcmp(uri, LV2_UI__scaleFactor) == 0)
            return kUridScaleFactor;
#endif
        if (std::strcmp(uri, LV2_UI__windowTitle) == 0)
            return kUridWindowTitle;

        // Custom Carla types
        if (std::strcmp(uri, URI_CARLA_ATOM_WORKER_IN) == 0)
            return kUridCarlaAtomWorkerIn;
        if (std::strcmp(uri, URI_CARLA_ATOM_WORKER_RESP) == 0)
            return kUridCarlaAtomWorkerResp;
        if (std::strcmp(uri, URI_CARLA_PARAMETER_CHANGE) == 0)
            return kUridCarlaParameterChange;
        if (std::strcmp(uri, LV2_KXSTUDIO_PROPERTIES__TransientWindowId) == 0)
            return kUridCarlaTransientWindowId;

        // Custom plugin types
        return ((CarlaPluginLV2*)handle)->getCustomURID(uri);
    }

    static const char* carla_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(urid != kUridNull, nullptr);
        carla_debug("carla_lv2_urid_unmap(%p, %i)", handle, urid);

        switch (urid)
        {
        // Atom types
        case kUridAtomBlank:
            return LV2_ATOM__Blank;
        case kUridAtomBool:
            return LV2_ATOM__Bool;
        case kUridAtomChunk:
            return LV2_ATOM__Chunk;
        case kUridAtomDouble:
            return LV2_ATOM__Double;
        case kUridAtomEvent:
            return LV2_ATOM__Event;
        case kUridAtomFloat:
            return LV2_ATOM__Float;
        case kUridAtomInt:
            return LV2_ATOM__Int;
        case kUridAtomLiteral:
            return LV2_ATOM__Literal;
        case kUridAtomLong:
            return LV2_ATOM__Long;
        case kUridAtomNumber:
            return LV2_ATOM__Number;
        case kUridAtomObject:
            return LV2_ATOM__Object;
        case kUridAtomPath:
            return LV2_ATOM__Path;
        case kUridAtomProperty:
            return LV2_ATOM__Property;
        case kUridAtomResource:
            return LV2_ATOM__Resource;
        case kUridAtomSequence:
            return LV2_ATOM__Sequence;
        case kUridAtomSound:
            return LV2_ATOM__Sound;
        case kUridAtomString:
            return LV2_ATOM__String;
        case kUridAtomTuple:
            return LV2_ATOM__Tuple;
        case kUridAtomURI:
            return LV2_ATOM__URI;
        case kUridAtomURID:
            return LV2_ATOM__URID;
        case kUridAtomVector:
            return LV2_ATOM__Vector;
        case kUridAtomTransferAtom:
            return LV2_ATOM__atomTransfer;
        case kUridAtomTransferEvent:
            return LV2_ATOM__eventTransfer;

        // BufSize types
        case kUridBufMaxLength:
            return LV2_BUF_SIZE__maxBlockLength;
        case kUridBufMinLength:
            return LV2_BUF_SIZE__minBlockLength;
        case kUridBufNominalLength:
            return LV2_BUF_SIZE__nominalBlockLength;
        case kUridBufSequenceSize:
            return LV2_BUF_SIZE__sequenceSize;

        // Log types
        case kUridLogError:
            return LV2_LOG__Error;
        case kUridLogNote:
            return LV2_LOG__Note;
        case kUridLogTrace:
            return LV2_LOG__Trace;
        case kUridLogWarning:
            return LV2_LOG__Warning;

        // Patch types
        case kUridPatchSet:
            return LV2_PATCH__Set;
        case kUridPatchProperty:
            return LV2_PATCH__property;
        case kUridPatchSubject:
            return LV2_PATCH__subject;
        case kUridPatchValue:
            return LV2_PATCH__value;

        // Time types
        case kUridTimePosition:
            return LV2_TIME__Position;
        case kUridTimeBar:
            return LV2_TIME__bar;
        case kUridTimeBarBeat:
            return LV2_TIME__barBeat;
        case kUridTimeBeat:
            return LV2_TIME__beat;
        case kUridTimeBeatUnit:
            return LV2_TIME__beatUnit;
        case kUridTimeBeatsPerBar:
            return LV2_TIME__beatsPerBar;
        case kUridTimeBeatsPerMinute:
            return LV2_TIME__beatsPerMinute;
        case kUridTimeFrame:
            return LV2_TIME__frame;
        case kUridTimeFramesPerSecond:
            return LV2_TIME__framesPerSecond;
        case kUridTimeSpeed:
            return LV2_TIME__speed;
        case kUridTimeTicksPerBeat:
            return LV2_KXSTUDIO_PROPERTIES__TimePositionTicksPerBeat;

        // Others
        case kUridMidiEvent:
            return LV2_MIDI__MidiEvent;
        case kUridParamSampleRate:
            return LV2_PARAMETERS__sampleRate;
        case kUridBackgroundColor:
            return LV2_UI__backgroundColor;
        case kUridForegroundColor:
            return LV2_UI__foregroundColor;
#ifndef CARLA_OS_MAC
        case kUridScaleFactor:
            return LV2_UI__scaleFactor;
#endif
        case kUridWindowTitle:
            return LV2_UI__windowTitle;

        // Custom Carla types
        case kUridCarlaAtomWorkerIn:
            return URI_CARLA_ATOM_WORKER_IN;
        case kUridCarlaAtomWorkerResp:
            return URI_CARLA_ATOM_WORKER_RESP;
        case kUridCarlaParameterChange:
            return URI_CARLA_PARAMETER_CHANGE;
        case kUridCarlaTransientWindowId:
            return LV2_KXSTUDIO_PROPERTIES__TransientWindowId;
        }

        // Custom plugin types
        return ((CarlaPluginLV2*)handle)->getCustomURIDString(urid);
    }

    // -------------------------------------------------------------------
    // Worker Feature

    static LV2_Worker_Status carla_lv2_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, LV2_WORKER_ERR_UNKNOWN);
        carla_debug("carla_lv2_worker_schedule(%p, %i, %p)", handle, size, data);

        return ((CarlaPluginLV2*)handle)->handleWorkerSchedule(size, data);
    }

    static LV2_Worker_Status carla_lv2_worker_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, LV2_WORKER_ERR_UNKNOWN);
        carla_debug("carla_lv2_worker_respond(%p, %i, %p)", handle, size, data);

        return ((CarlaPluginLV2*)handle)->handleWorkerRespond(size, data);
    }

    // -------------------------------------------------------------------
    // Inline Display Feature

    static void carla_lv2_inline_display_queue_draw(LV2_Inline_Display_Handle handle)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);
        // carla_debug("carla_lv2_inline_display_queue_draw(%p)", handle);

        ((CarlaPluginLV2*)handle)->handleInlineDisplayQueueRedraw();
    }

    // -------------------------------------------------------------------
    // Midnam Feature

    static void carla_lv2_midnam_update(LV2_Midnam_Handle handle)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);
        carla_stdout("carla_lv2_midnam_update(%p)", handle);

        ((CarlaPluginLV2*)handle)->handleMidnamUpdate();
    }

    // -------------------------------------------------------------------
    // ControlInputPort change request Feature

    static LV2_ControlInputPort_Change_Status carla_lv2_ctrl_in_port_change_req(
        LV2_ControlInputPort_Change_Request_Handle handle, uint32_t index, float value)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, LV2_CONTROL_INPUT_PORT_CHANGE_ERR_UNKNOWN);
        carla_stdout("carla_lv2_ctrl_in_port_change_req(%p, %u, %f)", handle, index, value);

        return ((CarlaPluginLV2*)handle)->handleCtrlInPortChangeReq(index, value);
    }

    // -------------------------------------------------------------------
    // External UI Feature

    static void carla_lv2_external_ui_closed(LV2UI_Controller controller)
    {
        CARLA_SAFE_ASSERT_RETURN(controller != nullptr,);
        carla_debug("carla_lv2_external_ui_closed(%p)", controller);

        ((CarlaPluginLV2*)controller)->handleExternalUIClosed();
    }

    // -------------------------------------------------------------------
    // UI Port-Map Feature

    static uint32_t carla_lv2_ui_port_map(LV2UI_Feature_Handle handle, const char* symbol)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, LV2UI_INVALID_PORT_INDEX);
        carla_debug("carla_lv2_ui_port_map(%p, \"%s\")", handle, symbol);

        return ((CarlaPluginLV2*)handle)->handleUIPortMap(symbol);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // UI Request Parameter Feature

    static LV2UI_Request_Value_Status carla_lv2_ui_request_value(LV2UI_Feature_Handle handle,
                                                                 LV2_URID key,
                                                                 LV2_URID type,
                                                                 const LV2_Feature* const* features)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, LV2UI_REQUEST_VALUE_ERR_UNKNOWN);
        carla_debug("carla_lv2_ui_request_value(%p, %u, %u, %p)", handle, key, type, features);

        return ((CarlaPluginLV2*)handle)->handleUIRequestValue(key, type, features);
    }

    // -------------------------------------------------------------------
    // UI Resize Feature

    static int carla_lv2_ui_resize(LV2UI_Feature_Handle handle, int width, int height)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, 1);
        carla_debug("carla_lv2_ui_resize(%p, %i, %i)", handle, width, height);

        return ((CarlaPluginLV2*)handle)->handleUIResize(width, height);
    }

    // -------------------------------------------------------------------
    // UI Touch Feature

    static void carla_lv2_ui_touch(LV2UI_Feature_Handle handle, uint32_t port_index, bool touch)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);
        carla_debug("carla_lv2_ui_touch(%p, %u, %s)", handle, port_index, bool2str(touch));

        ((CarlaPluginLV2*)handle)->handleUITouch(port_index, touch);
    }

    // -------------------------------------------------------------------
    // UI Extension

    static void carla_lv2_ui_write_function(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
    {
        CARLA_SAFE_ASSERT_RETURN(controller != nullptr,);
        carla_debug("carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);

        ((CarlaPluginLV2*)controller)->handleUIWrite(port_index, buffer_size, format, buffer);
    }

    // -------------------------------------------------------------------
    // Lilv State

    static void carla_lilv_set_port_value(const char* port_symbol, void* user_data, const void* value, uint32_t size, uint32_t type)
    {
        CARLA_SAFE_ASSERT_RETURN(user_data != nullptr,);
        carla_debug("carla_lilv_set_port_value(\"%s\", %p, %p, %i, %i", port_symbol, user_data, value, size, type);

        ((CarlaPluginLV2*)user_data)->handleLilvSetPortValue(port_symbol, value, size, type);
    }

    // -------------------------------------------------------------------

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginLV2)
};

// -------------------------------------------------------------------------------------------------------------------

#ifndef LV2_UIS_ONLY_INPROCESS
bool CarlaPipeServerLV2::msgReceived(const char* const msg) noexcept
{
    if (std::strcmp(msg, "exiting") == 0)
    {
        closePipeServer();
        fUiState = UiHide;
        return true;
    }

    if (std::strcmp(msg, "control") == 0)
    {
        uint32_t index;
        float value;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(index), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

        try {
            kPlugin->handleUIWrite(index, sizeof(float), kUridNull, &value);
        } CARLA_SAFE_EXCEPTION("magReceived control");

        return true;
    }

    if (std::strcmp(msg, "pcontrol") == 0)
    {
        const char* uri;
        float value;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(uri, true), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

        try {
            kPlugin->handleUIBridgeParameter(uri, value);
        } CARLA_SAFE_EXCEPTION("magReceived pcontrol");

        return true;
    }

    if (std::strcmp(msg, "atom") == 0)
    {
        uint32_t index, atomTotalSize, base64Size;
        const char* base64atom;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(index), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(atomTotalSize), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(base64Size), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(base64atom, false, base64Size), true);

        std::vector<uint8_t> chunk;
        d_getChunkFromBase64String_impl(chunk, base64atom);
        CARLA_SAFE_ASSERT_UINT2_RETURN(chunk.size() >= sizeof(LV2_Atom), chunk.size(), sizeof(LV2_Atom), true);

#ifdef CARLA_PROPER_CPP11_SUPPORT
        const LV2_Atom* const atom((const LV2_Atom*)chunk.data());
#else
        const LV2_Atom* const atom((const LV2_Atom*)&chunk.front());
#endif
        CARLA_SAFE_ASSERT_RETURN(lv2_atom_total_size(atom) == chunk.size(), true);

        try {
            kPlugin->handleUIWrite(index, lv2_atom_total_size(atom), kUridAtomTransferEvent, atom);
        } CARLA_SAFE_EXCEPTION("magReceived atom");

        return true;
    }

    if (std::strcmp(msg, "program") == 0)
    {
        uint32_t index;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(index), true);

        try {
            kPlugin->setMidiProgram(static_cast<int32_t>(index), false, true, true, false);
        } CARLA_SAFE_EXCEPTION("msgReceived program");

        return true;
    }

    if (std::strcmp(msg, "urid") == 0)
    {
        uint32_t urid, size;
        const char* uri;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(urid), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(size), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(uri, false, size), true);

        if (urid != 0)
        {
            try {
                kPlugin->handleUridMap(urid, uri);
            } CARLA_SAFE_EXCEPTION("msgReceived urid");
        }

        return true;
    }

    if (std::strcmp(msg, "reloadprograms") == 0)
    {
        int32_t index;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(index), true);

        try {
            kPlugin->handleProgramChanged(index);
        } CARLA_SAFE_EXCEPTION("handleProgramChanged");

        return true;
    }

    if (std::strcmp(msg, "requestvalue") == 0)
    {
        uint32_t key, type;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(key), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(type), true);

        if (key != 0)
        {
            try {
                kPlugin->handleUIRequestValue(key, type, nullptr);
            } CARLA_SAFE_EXCEPTION("msgReceived requestvalue");
        }

        return true;
    }

    return false;
}
#endif

// -------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newLV2(const Initializer& init)
{
    carla_debug("CarlaPlugin::newLV2({%p, \"%s\", \"%s\"})", init.engine, init.name, init.label);

    std::shared_ptr<CarlaPluginLV2> plugin(new CarlaPluginLV2(init.engine, init.id));

    const char* needsArchBridge = nullptr;
    if (plugin->init(plugin, init.name, init.label, init.options, needsArchBridge))
        return plugin;

   #ifndef CARLA_OS_WASM
    if (needsArchBridge != nullptr)
    {
        String bridgeBinary(init.engine->getOptions().binaryDir);
        bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-native";

        return CarlaPlugin::newBridge(init, BINARY_NATIVE, PLUGIN_LV2, needsArchBridge, bridgeBinary);
    }
   #endif

    return nullptr;
}

// used in CarlaStandalone.cpp
const void* carla_render_inline_display_lv2(const CarlaPluginPtr& plugin, uint32_t width, uint32_t height);

const void* carla_render_inline_display_lv2(const CarlaPluginPtr& plugin, uint32_t width, uint32_t height)
{
    const std::shared_ptr<CarlaPluginLV2>& lv2Plugin((const std::shared_ptr<CarlaPluginLV2>&)plugin);

    return lv2Plugin->renderInlineDisplay(width, height);
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
