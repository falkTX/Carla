/*
 * Carla LV2 Plugin
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

// testing macros
// #define LV2_UIS_ONLY_BRIDGES
// #define LV2_UIS_ONLY_INPROCESS

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#include "CarlaLv2Utils.hpp"

#include "CarlaBase64Utils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaPipeUtils.hpp"
#include "CarlaPluginUI.hpp"
#include "Lv2AtomRingBuffer.hpp"

#include "../engine/CarlaEngineOsc.hpp"
#include "../modules/lilv/config/lilv_config.h"

extern "C" {
#include "rtmempool/rtmempool-lv2.h"
}

#include "juce_core.h"

using juce::File;

#define URI_CARLA_ATOM_WORKER "http://kxstudio.sf.net/ns/carla/atomWorker"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------

// Maximum default buffer size
const uint MAX_DEFAULT_BUFFER_SIZE = 8192; // 0x2000

// Extra Plugin Hints
const uint PLUGIN_HAS_EXTENSION_OPTIONS  = 0x1000;
const uint PLUGIN_HAS_EXTENSION_PROGRAMS = 0x2000;
const uint PLUGIN_HAS_EXTENSION_STATE    = 0x4000;
const uint PLUGIN_HAS_EXTENSION_WORKER   = 0x8000;

// Extra Parameter Hints
const uint PARAMETER_IS_STRICT_BOUNDS = 0x1000;
const uint PARAMETER_IS_TRIGGER       = 0x2000;

// LV2 Event Data/Types
const uint CARLA_EVENT_DATA_ATOM    = 0x01;
const uint CARLA_EVENT_DATA_EVENT   = 0x02;
const uint CARLA_EVENT_DATA_MIDI_LL = 0x04;
const uint CARLA_EVENT_TYPE_MESSAGE = 0x10; // unused
const uint CARLA_EVENT_TYPE_MIDI    = 0x20;
const uint CARLA_EVENT_TYPE_TIME    = 0x40;

// LV2 URI Map Ids
const uint32_t CARLA_URI_MAP_ID_NULL                   =  0;
const uint32_t CARLA_URI_MAP_ID_ATOM_BLANK             =  1;
const uint32_t CARLA_URI_MAP_ID_ATOM_BOOL              =  2;
const uint32_t CARLA_URI_MAP_ID_ATOM_CHUNK             =  3;
const uint32_t CARLA_URI_MAP_ID_ATOM_DOUBLE            =  4;
const uint32_t CARLA_URI_MAP_ID_ATOM_EVENT             =  5;
const uint32_t CARLA_URI_MAP_ID_ATOM_FLOAT             =  6;
const uint32_t CARLA_URI_MAP_ID_ATOM_INT               =  7;
const uint32_t CARLA_URI_MAP_ID_ATOM_LITERAL           =  8;
const uint32_t CARLA_URI_MAP_ID_ATOM_LONG              =  9;
const uint32_t CARLA_URI_MAP_ID_ATOM_NUMBER            = 10;
const uint32_t CARLA_URI_MAP_ID_ATOM_OBJECT            = 11;
const uint32_t CARLA_URI_MAP_ID_ATOM_PATH              = 12;
const uint32_t CARLA_URI_MAP_ID_ATOM_PROPERTY          = 13;
const uint32_t CARLA_URI_MAP_ID_ATOM_RESOURCE          = 14;
const uint32_t CARLA_URI_MAP_ID_ATOM_SEQUENCE          = 15;
const uint32_t CARLA_URI_MAP_ID_ATOM_SOUND             = 16;
const uint32_t CARLA_URI_MAP_ID_ATOM_STRING            = 17;
const uint32_t CARLA_URI_MAP_ID_ATOM_TUPLE             = 18;
const uint32_t CARLA_URI_MAP_ID_ATOM_URI               = 19;
const uint32_t CARLA_URI_MAP_ID_ATOM_URID              = 20;
const uint32_t CARLA_URI_MAP_ID_ATOM_VECTOR            = 21;
const uint32_t CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM     = 22;
const uint32_t CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT    = 23;
const uint32_t CARLA_URI_MAP_ID_BUF_MAX_LENGTH         = 24;
const uint32_t CARLA_URI_MAP_ID_BUF_MIN_LENGTH         = 25;
const uint32_t CARLA_URI_MAP_ID_BUF_NOMINAL_LENGTH     = 26;
const uint32_t CARLA_URI_MAP_ID_BUF_SEQUENCE_SIZE      = 27;
const uint32_t CARLA_URI_MAP_ID_LOG_ERROR              = 28;
const uint32_t CARLA_URI_MAP_ID_LOG_NOTE               = 29;
const uint32_t CARLA_URI_MAP_ID_LOG_TRACE              = 30;
const uint32_t CARLA_URI_MAP_ID_LOG_WARNING            = 31;
const uint32_t CARLA_URI_MAP_ID_TIME_POSITION          = 32; // base type
const uint32_t CARLA_URI_MAP_ID_TIME_BAR               = 33; // values
const uint32_t CARLA_URI_MAP_ID_TIME_BAR_BEAT          = 34;
const uint32_t CARLA_URI_MAP_ID_TIME_BEAT              = 35;
const uint32_t CARLA_URI_MAP_ID_TIME_BEAT_UNIT         = 36;
const uint32_t CARLA_URI_MAP_ID_TIME_BEATS_PER_BAR     = 37;
const uint32_t CARLA_URI_MAP_ID_TIME_BEATS_PER_MINUTE  = 38;
const uint32_t CARLA_URI_MAP_ID_TIME_FRAME             = 39;
const uint32_t CARLA_URI_MAP_ID_TIME_FRAMES_PER_SECOND = 40;
const uint32_t CARLA_URI_MAP_ID_TIME_SPEED             = 41;
const uint32_t CARLA_URI_MAP_ID_TIME_TICKS_PER_BEAT    = 42;
const uint32_t CARLA_URI_MAP_ID_MIDI_EVENT             = 43;
const uint32_t CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE      = 44;
const uint32_t CARLA_URI_MAP_ID_UI_WINDOW_TITLE        = 45;
const uint32_t CARLA_URI_MAP_ID_CARLA_ATOM_WORKER      = 46;
const uint32_t CARLA_URI_MAP_ID_CARLA_TRANSIENT_WIN_ID = 47;
const uint32_t CARLA_URI_MAP_ID_COUNT                  = 48;

// LV2 Feature Ids
const uint32_t kFeatureIdBufSizeBounded   =  0;
const uint32_t kFeatureIdBufSizeFixed     =  1;
const uint32_t kFeatureIdBufSizePowerOf2  =  2;
const uint32_t kFeatureIdEvent            =  3;
const uint32_t kFeatureIdHardRtCapable    =  4;
const uint32_t kFeatureIdInPlaceBroken    =  5;
const uint32_t kFeatureIdIsLive           =  6;
const uint32_t kFeatureIdLogs             =  7;
const uint32_t kFeatureIdOptions          =  8;
const uint32_t kFeatureIdPrograms         =  9;
const uint32_t kFeatureIdResizePort       = 10;
const uint32_t kFeatureIdRtMemPool        = 11;
const uint32_t kFeatureIdRtMemPoolOld     = 12;
const uint32_t kFeatureIdStateMakePath    = 13;
const uint32_t kFeatureIdStateMapPath     = 14;
const uint32_t kFeatureIdStrictBounds     = 15;
const uint32_t kFeatureIdUriMap           = 16;
const uint32_t kFeatureIdUridMap          = 17;
const uint32_t kFeatureIdUridUnmap        = 18;
const uint32_t kFeatureIdWorker           = 19;
const uint32_t kFeatureCountPlugin        = 20;
const uint32_t kFeatureIdUiDataAccess     = 20;
const uint32_t kFeatureIdUiInstanceAccess = 21;
const uint32_t kFeatureIdUiIdleInterface  = 22;
const uint32_t kFeatureIdUiFixedSize      = 23;
const uint32_t kFeatureIdUiMakeResident   = 24;
const uint32_t kFeatureIdUiNoUserResize   = 25;
const uint32_t kFeatureIdUiParent         = 26;
const uint32_t kFeatureIdUiPortMap        = 27;
const uint32_t kFeatureIdUiPortSubscribe  = 28;
const uint32_t kFeatureIdUiResize         = 29;
const uint32_t kFeatureIdUiTouch          = 30;
const uint32_t kFeatureIdExternalUi       = 31;
const uint32_t kFeatureIdExternalUiOld    = 32;
const uint32_t kFeatureCountAll           = 33;

// -----------------------------------------------------

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

        const uint32_t rtype(type);
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

    CARLA_DECLARE_NON_COPY_STRUCT(Lv2EventData)
};

struct CarlaPluginLV2EventData {
    uint32_t count;
    Lv2EventData* data;
    Lv2EventData* ctrl; // default port, either this->data[x] or pData->portIn/Out
    uint32_t ctrlIndex;

    CarlaPluginLV2EventData() noexcept
        : count(0),
          data(nullptr),
          ctrl(nullptr),
          ctrlIndex(0) {}

    ~CarlaPluginLV2EventData() noexcept
    {
        CARLA_SAFE_ASSERT_INT(count == 0, count);
        CARLA_SAFE_ASSERT(data == nullptr);
        CARLA_SAFE_ASSERT(ctrl == nullptr);
        CARLA_SAFE_ASSERT_INT(ctrlIndex == 0, ctrlIndex);
    }

    void createNew(const uint32_t newCount)
    {
        CARLA_SAFE_ASSERT_INT(count == 0, count);
        CARLA_SAFE_ASSERT_INT(ctrlIndex == 0, ctrlIndex);
        CARLA_SAFE_ASSERT_RETURN(data == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(ctrl == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(newCount > 0,);

        data  = new Lv2EventData[newCount];
        count = newCount;

        ctrl      = nullptr;
        ctrlIndex = 0;
    }

    void clear() noexcept
    {
        if (data != nullptr)
        {
            for (uint32_t i=0; i < count; ++i)
            {
                if (data[i].port != nullptr && ctrl != nullptr && data[i].port == ctrl->port)
                    data[i].port = nullptr;
            }

            delete[] data;
            data = nullptr;
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

    CARLA_DECLARE_NON_COPY_STRUCT(CarlaPluginLV2EventData)
};

// -----------------------------------------------------

struct CarlaPluginLV2Options {
    enum OptIndex {
        MaxBlockLenth = 0,
        MinBlockLenth,
        NominalBlockLenth,
        SequenceSize,
        SampleRate,
        FrontendWinId,
        WindowTitle,
        Null,
        Count
    };

    int maxBufferSize;
    int minBufferSize;
    int nominalBufferSize;
    int sequenceSize;
    double sampleRate;
    int64_t frontendWinId;
    const char* windowTitle;
    LV2_Options_Option opts[Count];

    CarlaPluginLV2Options() noexcept
        : maxBufferSize(0),
          minBufferSize(0),
          sequenceSize(MAX_DEFAULT_BUFFER_SIZE),
          sampleRate(0.0),
          frontendWinId(0),
          windowTitle(nullptr)
    {
        LV2_Options_Option& optMaxBlockLenth(opts[MaxBlockLenth]);
        optMaxBlockLenth.context = LV2_OPTIONS_INSTANCE;
        optMaxBlockLenth.subject = 0;
        optMaxBlockLenth.key     = CARLA_URI_MAP_ID_BUF_MAX_LENGTH;
        optMaxBlockLenth.size    = sizeof(int);
        optMaxBlockLenth.type    = CARLA_URI_MAP_ID_ATOM_INT;
        optMaxBlockLenth.value   = &maxBufferSize;

        LV2_Options_Option& optMinBlockLenth(opts[MinBlockLenth]);
        optMinBlockLenth.context = LV2_OPTIONS_INSTANCE;
        optMinBlockLenth.subject = 0;
        optMinBlockLenth.key     = CARLA_URI_MAP_ID_BUF_MIN_LENGTH;
        optMinBlockLenth.size    = sizeof(int);
        optMinBlockLenth.type    = CARLA_URI_MAP_ID_ATOM_INT;
        optMinBlockLenth.value   = &minBufferSize;

        LV2_Options_Option& optNominalBlockLenth(opts[NominalBlockLenth]);
        optNominalBlockLenth.context = LV2_OPTIONS_INSTANCE;
        optNominalBlockLenth.subject = 0;
        optNominalBlockLenth.key     = CARLA_URI_MAP_ID_BUF_NOMINAL_LENGTH;
        optNominalBlockLenth.size    = sizeof(int);
        optNominalBlockLenth.type    = CARLA_URI_MAP_ID_ATOM_INT;
        optNominalBlockLenth.value   = &nominalBufferSize;

        LV2_Options_Option& optSequenceSize(opts[SequenceSize]);
        optSequenceSize.context = LV2_OPTIONS_INSTANCE;
        optSequenceSize.subject = 0;
        optSequenceSize.key     = CARLA_URI_MAP_ID_BUF_SEQUENCE_SIZE;
        optSequenceSize.size    = sizeof(int);
        optSequenceSize.type    = CARLA_URI_MAP_ID_ATOM_INT;
        optSequenceSize.value   = &sequenceSize;

        LV2_Options_Option& optSampleRate(opts[SampleRate]);
        optSampleRate.context = LV2_OPTIONS_INSTANCE;
        optSampleRate.subject = 0;
        optSampleRate.key     = CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE;
        optSampleRate.size    = sizeof(double);
        optSampleRate.type    = CARLA_URI_MAP_ID_ATOM_DOUBLE;
        optSampleRate.value   = &sampleRate;

        LV2_Options_Option& optFrontendWinId(opts[FrontendWinId]);
        optFrontendWinId.context = LV2_OPTIONS_INSTANCE;
        optFrontendWinId.subject = 0;
        optFrontendWinId.key     = CARLA_URI_MAP_ID_CARLA_TRANSIENT_WIN_ID;
        optFrontendWinId.size    = sizeof(int64_t);
        optFrontendWinId.type    = CARLA_URI_MAP_ID_ATOM_LONG;
        optFrontendWinId.value   = &frontendWinId;

        LV2_Options_Option& optWindowTitle(opts[WindowTitle]);
        optWindowTitle.context = LV2_OPTIONS_INSTANCE;
        optWindowTitle.subject = 0;
        optWindowTitle.key     = CARLA_URI_MAP_ID_UI_WINDOW_TITLE;
        optWindowTitle.size    = 0;
        optWindowTitle.type    = CARLA_URI_MAP_ID_ATOM_STRING;
        optWindowTitle.value   = nullptr;

        LV2_Options_Option& optNull(opts[Null]);
        optNull.context = LV2_OPTIONS_INSTANCE;
        optNull.subject = 0;
        optNull.key     = CARLA_URI_MAP_ID_NULL;
        optNull.size    = 0;
        optNull.type    = CARLA_URI_MAP_ID_NULL;
        optNull.value   = nullptr;
    }

    ~CarlaPluginLV2Options() noexcept
    {
        LV2_Options_Option& optWindowTitle(opts[WindowTitle]);

        optWindowTitle.size  = 0;
        optWindowTitle.value = nullptr;

        if (windowTitle != nullptr)
        {
            delete[] windowTitle;
            windowTitle = nullptr;
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(CarlaPluginLV2Options);
};

// -----------------------------------------------------------------------

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

    bool startPipeServer() noexcept
    {
        const ScopedEngineEnvironmentLocker _seel(kEngine);
        const ScopedEnvVar _sev1("LV2_PATH", kEngine->getOptions().pathLV2);
#ifdef CARLA_OS_LINUX
        const ScopedEnvVar _sev2("LD_PRELOAD", nullptr);
#endif

        return CarlaPipeServer::startPipeServer(fFilename, fPluginURI, fUiURI);
    }

    void writeUiOptionsMessage(const double sampleRate, const bool useTheme, const bool useThemeColors, const char* const windowTitle, uintptr_t transientWindowId) const noexcept
    {
        char tmpBuf[0xff+1];
        tmpBuf[0xff] = '\0';

        const CarlaMutexLocker cml(getPipeLock());
        const ScopedLocale csl;

        _writeMsgBuffer("uiOptions\n", 10);

        {
            std::snprintf(tmpBuf, 0xff, "%g\n", sampleRate);
            _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));

            std::snprintf(tmpBuf, 0xff, "%s\n", bool2str(useTheme));
            _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));

            std::snprintf(tmpBuf, 0xff, "%s\n", bool2str(useThemeColors));
            _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));

            writeAndFixMessage(windowTitle != nullptr ? windowTitle : "");

            std::snprintf(tmpBuf, 0xff, P_INTPTR "\n", transientWindowId);
            _writeMsgBuffer(tmpBuf, std::strlen(tmpBuf));
        }

        flushMessages();
    }

    void writeUiTitleMessage(const char* const title) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(title != nullptr && title[0] != '\0',);

        const CarlaMutexLocker cml(getPipeLock());

        _writeMsgBuffer("uiTitle\n", 8);
        writeAndFixMessage(title);
        flushMessages();
    }

protected:
    // returns true if msg was handled
    bool msgReceived(const char* const msg) noexcept override;

private:
    CarlaEngine*    const kEngine;
    CarlaPluginLV2* const kPlugin;

    CarlaString fFilename;
    CarlaString fPluginURI;
    CarlaString fUiURI;
    UiState     fUiState;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPipeServerLV2)
};

// -----------------------------------------------------

class CarlaPluginLV2 : public CarlaPlugin,
                       private CarlaPluginUI::CloseCallback
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
          fCanInit2(true),
          fNeedsUiClose(false),
          fLatencyChanged(false),
          fLatencyIndex(-1),
          fAtomBufferIn(),
          fAtomBufferOut(),
          fAtomForge(),
          fEventsIn(),
          fEventsOut(),
          fLv2Options(),
          fPipeServer(engine, this),
          fCustomURIDs(),
          fFirstActive(true),
          fLastStateChunk(nullptr),
          fLastTimeInfo(),
          fExt(),
          fUI()
    {
        carla_debug("CarlaPluginLV2::CarlaPluginLV2(%p, %i)", engine, id);

        carla_zeroPointers(fFeatures, kFeatureCountAll+1);

        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; ++i)
            fCustomURIDs.append(nullptr);

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
        fAtomForge.Blank    = CARLA_URI_MAP_ID_ATOM_BLANK;
        fAtomForge.Bool     = CARLA_URI_MAP_ID_ATOM_BOOL;
        fAtomForge.Chunk    = CARLA_URI_MAP_ID_ATOM_CHUNK;
        fAtomForge.Double   = CARLA_URI_MAP_ID_ATOM_DOUBLE;
        fAtomForge.Float    = CARLA_URI_MAP_ID_ATOM_FLOAT;
        fAtomForge.Int      = CARLA_URI_MAP_ID_ATOM_INT;
        fAtomForge.Literal  = CARLA_URI_MAP_ID_ATOM_LITERAL;
        fAtomForge.Long     = CARLA_URI_MAP_ID_ATOM_LONG;
        fAtomForge.Object   = CARLA_URI_MAP_ID_ATOM_OBJECT;
        fAtomForge.Path     = CARLA_URI_MAP_ID_ATOM_PATH;
        fAtomForge.Property = CARLA_URI_MAP_ID_ATOM_PROPERTY;
        fAtomForge.Resource = CARLA_URI_MAP_ID_ATOM_RESOURCE;
        fAtomForge.Sequence = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
        fAtomForge.String   = CARLA_URI_MAP_ID_ATOM_STRING;
        fAtomForge.Tuple    = CARLA_URI_MAP_ID_ATOM_TUPLE;
        fAtomForge.URI      = CARLA_URI_MAP_ID_ATOM_URI;
        fAtomForge.URID     = CARLA_URI_MAP_ID_ATOM_URID;
        fAtomForge.Vector   = CARLA_URI_MAP_ID_ATOM_VECTOR;
#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif
    }

    ~CarlaPluginLV2() override
    {
        carla_debug("CarlaPluginLV2::~CarlaPluginLV2()");

        // close UI
        if (fUI.type != UI::TYPE_NULL)
        {
            showCustomUI(false);

            if (fUI.type == UI::TYPE_BRIDGE)
            {
                fPipeServer.stopPipeServer(pData->engine->getOptions().uiBridgesTimeout);
            }
            else
            {
                if (fFeatures[kFeatureIdUiDataAccess] != nullptr && fFeatures[kFeatureIdUiDataAccess]->data != nullptr)
                    delete (LV2_Extension_Data_Feature*)fFeatures[kFeatureIdUiDataAccess]->data;

                if (fFeatures[kFeatureIdUiPortMap] != nullptr && fFeatures[kFeatureIdUiPortMap]->data != nullptr)
                    delete (LV2UI_Port_Map*)fFeatures[kFeatureIdUiPortMap]->data;

                if (fFeatures[kFeatureIdUiResize] != nullptr && fFeatures[kFeatureIdUiResize]->data != nullptr)
                    delete (LV2UI_Resize*)fFeatures[kFeatureIdUiResize]->data;

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
            pData->client->deactivate();

        if (pData->active)
        {
            deactivate();
            pData->active = false;
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

        for (uint32_t i=0; i < kFeatureCountAll; ++i)
        {
            if (fFeatures[i] != nullptr)
            {
                delete fFeatures[i];
                fFeatures[i] = nullptr;
            }
        }

        for (LinkedList<const char*>::Itenerator it = fCustomURIDs.begin2(); it.valid(); it.next())
        {
            const char* const uri(it.getValue());

            if (uri != nullptr)
                delete[] uri;
        }

        fCustomURIDs.clear();

        if (fLastStateChunk != nullptr)
        {
            std::free(fLastStateChunk);
            fLastStateChunk = nullptr;
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

    int64_t getUniqueId() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, 0);

        return static_cast<int64_t>(fRdfDescriptor->UniqueID);
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

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        const bool hasMidiIn(getMidiInCount() > 0);

        uint options = 0x0;

        if (fExt.programs != nullptr)
            options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (fLatencyIndex == -1 && ! (hasMidiIn || needsFixedBuffer()))
            options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (fCanInit2 && pData->engine->getProccessMode() != ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        {
            if (pData->options & PLUGIN_OPTION_FORCE_STEREO)
                options |= PLUGIN_OPTION_FORCE_STEREO;
            else if (pData->audioIn.count <= 1 && pData->audioOut.count <= 1 && (pData->audioIn.count != 0 || pData->audioOut.count != 0))
                options |= PLUGIN_OPTION_FORCE_STEREO;
        }

        if (hasMidiIn)
        {
            options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        if (pData->param.data[parameterId].hints & PARAMETER_IS_STRICT_BOUNDS)
            pData->param.ranges[parameterId].fixValue(fParamBuffers[parameterId]);

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

    void getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor->URI != nullptr,);

        std::strncpy(strBuf, fRdfDescriptor->URI, STR_MAX);
    }

    void getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);

        if (fRdfDescriptor->Author != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Author, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);

        if (fRdfDescriptor->License != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->License, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);

        if (fRdfDescriptor->Name != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
            std::strncpy(strBuf, fRdfDescriptor->Ports[rindex].Name, STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterSymbol(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
            std::strncpy(strBuf, fRdfDescriptor->Ports[rindex].Symbol, STR_MAX);
        else
            CarlaPlugin::getParameterSymbol(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);

            if (LV2_HAVE_PORT_UNIT_SYMBOL(port->Unit.Hints) && port->Unit.Symbol != nullptr)
            {
                std::strncpy(strBuf, port->Unit.Symbol, STR_MAX);
                return;
            }

            if (LV2_HAVE_PORT_UNIT_UNIT(port->Unit.Hints))
            {
                switch (port->Unit.Unit)
                {
                case LV2_PORT_UNIT_BAR:
                    std::strncpy(strBuf, "bars", STR_MAX);
                    return;
                case LV2_PORT_UNIT_BEAT:
                    std::strncpy(strBuf, "beats", STR_MAX);
                    return;
                case LV2_PORT_UNIT_BPM:
                    std::strncpy(strBuf, "BPM", STR_MAX);
                    return;
                case LV2_PORT_UNIT_CENT:
                    std::strncpy(strBuf, "ct", STR_MAX);
                    return;
                case LV2_PORT_UNIT_CM:
                    std::strncpy(strBuf, "cm", STR_MAX);
                    return;
                case LV2_PORT_UNIT_COEF:
                    std::strncpy(strBuf, "(coef)", STR_MAX);
                    return;
                case LV2_PORT_UNIT_DB:
                    std::strncpy(strBuf, "dB", STR_MAX);
                    return;
                case LV2_PORT_UNIT_DEGREE:
                    std::strncpy(strBuf, "deg", STR_MAX);
                    return;
                case LV2_PORT_UNIT_FRAME:
                    std::strncpy(strBuf, "frames", STR_MAX);
                    return;
                case LV2_PORT_UNIT_HZ:
                    std::strncpy(strBuf, "Hz", STR_MAX);
                    return;
                case LV2_PORT_UNIT_INCH:
                    std::strncpy(strBuf, "in", STR_MAX);
                    return;
                case LV2_PORT_UNIT_KHZ:
                    std::strncpy(strBuf, "kHz", STR_MAX);
                    return;
                case LV2_PORT_UNIT_KM:
                    std::strncpy(strBuf, "km", STR_MAX);
                    return;
                case LV2_PORT_UNIT_M:
                    std::strncpy(strBuf, "m", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MHZ:
                    std::strncpy(strBuf, "MHz", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MIDINOTE:
                    std::strncpy(strBuf, "note", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MILE:
                    std::strncpy(strBuf, "mi", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MIN:
                    std::strncpy(strBuf, "min", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MM:
                    std::strncpy(strBuf, "mm", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MS:
                    std::strncpy(strBuf, "ms", STR_MAX);
                    return;
                case LV2_PORT_UNIT_OCT:
                    std::strncpy(strBuf, "oct", STR_MAX);
                    return;
                case LV2_PORT_UNIT_PC:
                    std::strncpy(strBuf, "%", STR_MAX);
                    return;
                case LV2_PORT_UNIT_S:
                    std::strncpy(strBuf, "s", STR_MAX);
                    return;
                case LV2_PORT_UNIT_SEMITONE:
                    std::strncpy(strBuf, "semi", STR_MAX);
                    return;
                }
            }
        }

        CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);
            CARLA_SAFE_ASSERT_RETURN(scalePointId < port->ScalePointCount,);

            const LV2_RDF_PortScalePoint* const portScalePoint(&port->ScalePoints[scalePointId]);

            if (portScalePoint->Label != nullptr)
            {
                std::strncpy(strBuf, portScalePoint->Label, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    void prepareForSave() override
    {
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fExt.state != nullptr && fExt.state->save != nullptr)
        {
            fExt.state->save(fHandle, carla_lv2_state_store, this, LV2_STATE_IS_POD, fFeatures);

            if (fHandle2 != nullptr)
                fExt.state->save(fHandle2, carla_lv2_state_store, this, LV2_STATE_IS_POD, fFeatures);
        }
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setName(const char* const newName) override
    {
        CarlaPlugin::setName(newName);

        if (fLv2Options.windowTitle == nullptr)
            return;

        CarlaString guiTitle(pData->name);
        guiTitle += " (GUI)";

        delete[] fLv2Options.windowTitle;
        fLv2Options.windowTitle = guiTitle.dup();

        fLv2Options.opts[CarlaPluginLV2Options::WindowTitle].size  = (uint32_t)std::strlen(fLv2Options.windowTitle);
        fLv2Options.opts[CarlaPluginLV2Options::WindowTitle].value = fLv2Options.windowTitle;

        if (fFeatures[kFeatureIdExternalUi] != nullptr && fFeatures[kFeatureIdExternalUi]->data != nullptr)
            ((LV2_External_UI_Host*)fFeatures[kFeatureIdExternalUi]->data)->plugin_human_id = fLv2Options.windowTitle;

        if (fPipeServer.isPipeRunning())
            fPipeServer.writeUiTitleMessage(fLv2Options.windowTitle);

#ifndef LV2_UIS_ONLY_BRIDGES
        if (fUI.window != nullptr)
            fUI.window->setTitle(fLv2Options.windowTitle);
#endif
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fParamBuffers[parameterId] = fixedValue;

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
        carla_debug("CarlaPluginLV2::setCustomData(%s, %s, %s, %s)", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_TYPE_PROPERTY) == 0)
            return CarlaPlugin::setCustomData(type, key, value, sendGui);

        // we should only call state restore once
        // so inject this in CarlaPlugin::loadSaveState
        if (std::strcmp(type, CUSTOM_DATA_TYPE_STRING) == 0 && std::strcmp(key, "CarlaLoadLv2StateNow") == 0 && std::strcmp(value, "true") == 0)
        {
            if (fExt.state == nullptr)
                return;

            LV2_State_Status status = LV2_STATE_ERR_UNKNOWN;

            {
                const ScopedSingleProcessLocker spl(this, true);

                try {
                    status = fExt.state->restore(fHandle, carla_lv2_state_retrieve, this, 0, fFeatures);
                } catch(...) {}

                if (fHandle2 != nullptr)
                {
                    try {
                        fExt.state->restore(fHandle, carla_lv2_state_retrieve, this, 0, fFeatures);
                    } catch(...) {}
                }
            }

            switch (status)
            {
            case LV2_STATE_SUCCESS:
                carla_debug("CarlaPluginLV2::setCustomData(\"%s\", \"%s\", <value>, %s) - success", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_UNKNOWN:
                carla_stderr("CarlaPluginLV2::setCustomData(\"%s\", \"%s\", <value>, %s) - unknown error", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_BAD_TYPE:
                carla_stderr("CarlaPluginLV2::setCustomData(\"%s\", \"%s\", <value>, %s) - error, bad type", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_BAD_FLAGS:
                carla_stderr("CarlaPluginLV2::setCustomData(\"%s\", \"%s\", <value>, %s) - error, bad flags", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_NO_FEATURE:
                carla_stderr("CarlaPluginLV2::setCustomData(\"%s\", \"%s\", <value>, %s) - error, missing feature", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_NO_PROPERTY:
                carla_stderr("CarlaPluginLV2::setCustomData(\"%s\", \"%s\", <value>, %s) - error, missing property", type, key, bool2str(sendGui));
                break;
            }
            return;
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->prog.count),);

        if (index >= 0 && index < static_cast<int32_t>(fRdfDescriptor->PresetCount))
        {
            if (LilvState* const state = Lv2WorldClass::getInstance().getStateFromURI(fRdfDescriptor->Presets[index].URI, (const LV2_URID_Map*)fFeatures[kFeatureIdUridMap]->data))
            {
                const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

                lilv_state_restore(state, fExt.state, fHandle, carla_lilv_set_port_value, this, 0, fFeatures);

                if (fHandle2 != nullptr)
                    lilv_state_restore(state, fExt.state, fHandle2, carla_lilv_set_port_value, this, 0, fFeatures);

                lilv_state_free(state);
            }
        }

        CarlaPlugin::setProgram(index, sendGui, sendOsc, sendCallback);
    }

    void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);

        if (index >= 0 && fExt.programs != nullptr && fExt.programs->select_program != nullptr)
        {
            const uint32_t bank(pData->midiprog.data[index].bank);
            const uint32_t program(pData->midiprog.data[index].program);

            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            try {
                fExt.programs->select_program(fHandle, bank, program);
            } catch(...) {}

            if (fHandle2 != nullptr)
            {
                try {
                    fExt.programs->select_program(fHandle2, bank, program);
                } catch(...) {}
            }
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type != UI::TYPE_NULL,);

        const uintptr_t frontendWinId(pData->engine->getOptions().frontendWinId);

        if (! yesNo)
            pData->transientTryCounter = 0;

        if (fUI.type == UI::TYPE_BRIDGE)
        {
            if (yesNo)
            {
                if (fPipeServer.isPipeRunning())
                {
                    fPipeServer.writeFocusMessage();
                    return;
                }

                if (! fPipeServer.startPipeServer())
                {
                    pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
                    return;
                }

                for (std::size_t i=CARLA_URI_MAP_ID_COUNT, count=fCustomURIDs.count(); i < count; ++i)
                    fPipeServer.writeLv2UridMessage(static_cast<uint32_t>(i), fCustomURIDs.getAt(i, nullptr));

                fPipeServer.writeUiOptionsMessage(pData->engine->getSampleRate(), true, true, fLv2Options.windowTitle, frontendWinId);

                // send control ports
                for (uint32_t i=0; i < pData->param.count; ++i)
                    fPipeServer.writeControlMessage(static_cast<uint32_t>(pData->param.data[i].rindex), getParameterValue(i));

                fPipeServer.writeShowMessage();
#ifndef BUILD_BRIDGE
                if (fUI.rdfDescriptor->Type == LV2_UI_MOD)
                    pData->tryTransient();
#endif
            }
            else
            {
                fPipeServer.stopPipeServer(pData->engine->getOptions().uiBridgesTimeout);
            }
            return;
        }

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
                if (fUI.type == UI::TYPE_EMBED && fUI.window == nullptr)
                {
                    const char* msg = nullptr;

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
                        fUI.window = CarlaPluginUI::newCocoa(this, frontendWinId, isUiResizable());
# else
                        msg = "UI is for MacOS only";
# endif
                        break;

                    case LV2_UI_WINDOWS:
# ifdef CARLA_OS_WIN
                        fUI.window = CarlaPluginUI::newWindows(this, frontendWinId, isUiResizable());
# else
                        msg = "UI is for Windows only";
# endif
                        break;

                    case LV2_UI_X11:
# ifdef HAVE_X11
                        fUI.window = CarlaPluginUI::newX11(this, frontendWinId, isUiResizable());
# else
                        msg = "UI is only for systems with X11";
# endif
                        break;

                    default:
                        msg = "Unknown UI type";
                        break;
                    }

                    if (fUI.window == nullptr && fExt.uishow == nullptr)
                        return pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, -1, 0, 0.0f, msg);

                    if (fUI.window != nullptr)
                        fFeatures[kFeatureIdUiParent]->data = fUI.window->getPtr();
                }
#endif
                if (fUI.window != nullptr)
                    fUI.window->setTitle(fLv2Options.windowTitle);

                fUI.widget = nullptr;
                fUI.handle = fUI.descriptor->instantiate(fUI.descriptor, fRdfDescriptor->URI, fUI.rdfDescriptor->Bundle,
                                                         carla_lv2_ui_write_function, this, &fUI.widget, fFeatures);
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

                return pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, -1, 0, 0.0f, "Plugin refused to open its own UI");
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
# ifndef BUILD_BRIDGE
                    pData->tryTransient();
# endif
                }
            }
            else
#endif
            {
                LV2_EXTERNAL_UI_SHOW((LV2_External_UI_Widget*)fUI.widget);
#ifndef BUILD_BRIDGE
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
        }
    }

#if 0 // TODO
    void idle() override
    {
        if (fLatencyChanged && fLatencyIndex != -1)
        {
            fLatencyChanged = false;

            const int32_t latency(static_cast<int32_t>(fParamBuffers[fLatencyIndex]));

            if (latency >= 0)
            {
                const uint32_t ulatency(static_cast<uint32_t>(latency));

                if (pData->latency != ulatency)
                {
                    carla_stdout("latency changed to %i", latency);

                    const ScopedSingleProcessLocker sspl(this, true);

                    pData->latency = ulatency;
                    pData->client->setLatency(ulatency);
#ifndef BUILD_BRIDGE
                    pData->recreateLatencyBuffers();
#endif
                }
            }
            else
                carla_safe_assert_int("latency >= 0", __FILE__, __LINE__, latency);
        }

        CarlaPlugin::idle();
    }
#endif

    void uiIdle() override
    {
        if (fAtomBufferOut.isDataAvailableForReading())
        {
            uint8_t dumpBuf[fAtomBufferOut.getSize()];

            Lv2AtomRingBuffer tmpRingBuffer(fAtomBufferOut, dumpBuf);
            CARLA_SAFE_ASSERT(tmpRingBuffer.isDataAvailableForReading());

            uint32_t portIndex;
            const LV2_Atom* atom;
            const bool hasPortEvent(fUI.handle != nullptr && fUI.descriptor != nullptr &&
                                    fUI.descriptor->port_event != nullptr && ! fNeedsUiClose);

            for (; tmpRingBuffer.get(atom, portIndex);)
            {
                if (atom->type == CARLA_URI_MAP_ID_CARLA_ATOM_WORKER)
                {
                    CARLA_SAFE_ASSERT_CONTINUE(fExt.worker != nullptr && fExt.worker->work != nullptr);
                    fExt.worker->work(fHandle, carla_lv2_worker_respond, this, atom->size, LV2_ATOM_BODY_CONST(atom));
                }
                else if (fUI.type == UI::TYPE_BRIDGE)
                {
                    if (fPipeServer.isPipeRunning())
                        fPipeServer.writeLv2AtomMessage(portIndex, atom);
                }
                else
                {
                    if (hasPortEvent)
                        fUI.descriptor->port_event(fUI.handle, portIndex, lv2_atom_total_size(atom), CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);
                }
            }
        }

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
                // nobreak
            case CarlaPipeServerLV2::UiCrashed:
                pData->transientTryCounter = 0;
                pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
                break;
            }
        }
        else
        {
            // TODO - detect if ui-bridge crashed
        }

        if (fNeedsUiClose)
        {
            fNeedsUiClose = false;
            showCustomUI(false);
            pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
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
                pass();
            else if (fUI.handle != nullptr && fExt.uiidle != nullptr && fExt.uiidle->idle(fUI.handle) != 0)
            {
                showCustomUI(false);
                pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
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

        const uint32_t eventBufferSize(static_cast<uint32_t>(fLv2Options.sequenceSize)+0xff);

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        for (uint32_t i=0; i < portCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

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

        if ((pData->options & PLUGIN_OPTION_FORCE_STEREO) != 0 && (aIns == 1 || aOuts == 1) && fExt.state == nullptr && fExt.worker == nullptr)
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
            FloatVectorOperations::clear(fParamBuffers, static_cast<int>(params));
        }

        if (const uint32_t count = static_cast<uint32_t>(evIns.count()))
        {
            fEventsIn.createNew(count);

            for (uint32_t i=0; i < count; ++i)
            {
                const uint32_t& type(evIns.getAt(i, 0x0));

                if (type == CARLA_EVENT_DATA_ATOM)
                {
                    fEventsIn.data[i].type = CARLA_EVENT_DATA_ATOM;
                    fEventsIn.data[i].atom = lv2_atom_buffer_new(eventBufferSize, CARLA_URI_MAP_ID_NULL, CARLA_URI_MAP_ID_ATOM_SEQUENCE, true);
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
                const uint32_t& type(evOuts.getAt(i, 0x0));

                if (type == CARLA_EVENT_DATA_ATOM)
                {
                    fEventsOut.data[i].type = CARLA_EVENT_DATA_ATOM;
                    fEventsOut.data[i].atom = lv2_atom_buffer_new(eventBufferSize, CARLA_URI_MAP_ID_NULL, CARLA_URI_MAP_ID_ATOM_SEQUENCE, false);
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
        CarlaString portName;

        for (uint32_t i=0, iAudioIn=0, iAudioOut=0, iCvIn=0, iCvOut=0, iEvIn=0, iEvOut=0, iCtrl=0; i < portCount; ++i)
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
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    const uint32_t j = iCvIn++;
                    pData->cvIn.ports[j].port   = (CarlaEngineCVPort*)pData->client->addPort(kEnginePortTypeCV, portName, true, j);
                    pData->cvIn.ports[j].rindex = i;
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    const uint32_t j = iCvOut++;
                    pData->cvOut.ports[j].port   = (CarlaEngineCVPort*)pData->client->addPort(kEnginePortTypeCV, portName, false, j);
                    pData->cvOut.ports[j].rindex = i;
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
                        pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlIn = true;
                    }

                    // MIDI CC value
                    const LV2_RDF_PortMidiMap& portMidiMap(fRdfDescriptor->Ports[i].MidiMap);

                    if (LV2_IS_PORT_MIDI_MAP_CC(portMidiMap.Type))
                    {
                        if (portMidiMap.Number < MAX_MIDI_CONTROL && ! MIDI_IS_CONTROL_BANK_SELECT(portMidiMap.Number))
                            pData->param.data[j].midiCC = static_cast<int16_t>(portMidiMap.Number);
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
                        CARLA_SAFE_ASSERT(fLatencyIndex == static_cast<int32_t>(j));
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
                        pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
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

                // check if parameter is not enabled or automable
                if (LV2_IS_PORT_NOT_ON_GUI(portProps))
                    pData->param.data[j].hints &= ~(PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE);
                else if (LV2_IS_PORT_CAUSES_ARTIFACTS(portProps) || LV2_IS_PORT_EXPENSIVE(portProps))
                    pData->param.data[j].hints &= ~PARAMETER_IS_AUTOMABLE;
                else if (LV2_IS_PORT_NOT_AUTOMATIC(portProps) || LV2_IS_PORT_NON_AUTOMABLE(portProps))
                    pData->param.data[j].hints &= ~PARAMETER_IS_AUTOMABLE;

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

        if (fExt.worker != nullptr || (fUI.type != UI::TYPE_NULL && fEventsIn.count > 0 && (fEventsIn.data[0].type & CARLA_EVENT_DATA_ATOM) != 0))
            fAtomBufferIn.createBuffer(eventBufferSize);

        if (fExt.worker != nullptr || (fUI.type != UI::TYPE_NULL && fEventsOut.count > 0 && (fEventsOut.data[0].type & CARLA_EVENT_DATA_ATOM) != 0))
            fAtomBufferOut.createBuffer(eventBufferSize);

        if (fEventsIn.ctrl != nullptr && fEventsIn.ctrl->port == nullptr)
            fEventsIn.ctrl->port = pData->event.portIn;

        if (fEventsOut.ctrl != nullptr && fEventsOut.ctrl->port == nullptr)
            fEventsOut.ctrl->port = pData->event.portOut;

        if (fCanInit2 && (forcedStereoIn || forcedStereoOut))
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;
        else
            pData->options &= ~PLUGIN_OPTION_FORCE_STEREO;

        // plugin hints
        pData->hints = 0x0;

        if (isRealtimeSafe())
            pData->hints |= PLUGIN_IS_RTSAFE;

        if (fUI.type != UI::TYPE_NULL)
        {
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;

            if (fUI.type == UI::TYPE_EMBED || fUI.type == UI::TYPE_EXTERNAL)
                pData->hints |= PLUGIN_NEEDS_UI_MAIN_THREAD;
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

        if (! fCanInit2)
        {
            // can't run in rack
        }
        else if (fExt.state != nullptr || fExt.worker != nullptr)
        {
            if ((aIns == 0 || aIns == 2) && (aOuts == 0 || aOuts == 2) && evIns.count() <= 1 && evOuts.count() <= 1)
                pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;
        }
        else
        {
            if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0) && evIns.count() <= 1 && evOuts.count() <= 1)
                pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;
        }

#if 0 // TODO
        // check latency
        if (fLatencyIndex >= 0)
        {
            // we need to pre-run the plugin so it can update its latency control-port

            float tmpIn [(aIns > 0)  ? aIns  : 1][2];
            float tmpOut[(aOuts > 0) ? aOuts : 1][2];

            for (uint32_t j=0; j < aIns; ++j)
            {
                tmpIn[j][0] = 0.0f;
                tmpIn[j][1] = 0.0f;

                try {
                    fDescriptor->connect_port(fHandle, pData->audioIn.ports[j].rindex, tmpIn[j]);
                } CARLA_SAFE_EXCEPTION("LV2 connect_port latency input");
            }

            for (uint32_t j=0; j < aOuts; ++j)
            {
                tmpOut[j][0] = 0.0f;
                tmpOut[j][1] = 0.0f;

                try {
                    fDescriptor->connect_port(fHandle, pData->audioOut.ports[j].rindex, tmpOut[j]);
                } CARLA_SAFE_EXCEPTION("LV2 connect_port latency output");
            }

            if (fDescriptor->activate != nullptr)
            {
                try {
                    fDescriptor->activate(fHandle);
                } CARLA_SAFE_EXCEPTION("LV2 latency activate");
            }

            try {
                fDescriptor->run(fHandle, 2);
            } CARLA_SAFE_EXCEPTION("LV2 latency run");

            if (fDescriptor->deactivate != nullptr)
            {
                try {
                    fDescriptor->deactivate(fHandle);
                } CARLA_SAFE_EXCEPTION("LV2 latency deactivate");
            }

            const int32_t latency(static_cast<int32_t>(fParamBuffers[fLatencyIndex]));

            if (latency >= 0)
            {
                const uint32_t ulatency(static_cast<uint32_t>(latency));

                if (pData->latency != ulatency)
                {
                    carla_stdout("latency = %i", latency);

                    pData->latency = ulatency;
                    pData->client->setLatency(ulatency);
#ifndef BUILD_BRIDGE
                    pData->recreateLatencyBuffers();
#endif
                }
            }
            else
                carla_safe_assert_int("latency >= 0", __FILE__, __LINE__, latency);

            fLatencyChanged = false;
        }
#endif

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        evIns.clear();
        evOuts.clear();

        if (pData->active)
            activate();

        carla_debug("CarlaPluginLV2::reload() - end");
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

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
        // Update OSC Names
        if (pData->engine->isOscControlRegistered() && pData->id < pData->engine->getCurrentPluginCount())
        {
            pData->engine->oscSend_control_set_midi_program_count(pData->id, newCount);

            for (uint32_t i=0; i < newCount; ++i)
                pData->engine->oscSend_control_set_midi_program_data(pData->id, i, pData->midiprog.data[i].bank, pData->midiprog.data[i].program, pData->midiprog.data[i].name);
        }
#endif

        if (doInit)
        {
            if (newCount > 0)
            {
                setMidiProgram(0, false, false, false);
            }
            else
            {
                // load default state
                if (LilvState* const state = Lv2WorldClass::getInstance().getStateFromURI(fDescriptor->URI, (const LV2_URID_Map*)fFeatures[kFeatureIdUridMap]->data))
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
                setMidiProgram(pData->midiprog.current, true, true, true);

            pData->engine->callback(ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0.0f, nullptr);
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

    void process(const float** const audioIn, float** const audioOut, const float** const cvIn, float** const cvOut, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(audioOut[i], static_cast<int>(frames));
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
                FloatVectorOperations::clear(cvOut[i], static_cast<int>(frames));
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event itenerators from different APIs (input)

        LV2_Atom_Buffer_Iterator evInAtomIters[fEventsIn.count];
        LV2_Event_Iterator       evInEventIters[fEventsIn.count];
        LV2_MIDIState            evInMidiStates[fEventsIn.count];

        for (uint32_t i=0; i < fEventsIn.count; ++i)
        {
            if (fEventsIn.data[i].type & CARLA_EVENT_DATA_ATOM)
            {
                lv2_atom_buffer_reset(fEventsIn.data[i].atom, true);
                lv2_atom_buffer_begin(&evInAtomIters[i], fEventsIn.data[i].atom);
            }
            else if (fEventsIn.data[i].type & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(fEventsIn.data[i].event, LV2_EVENT_AUDIO_STAMP, fEventsIn.data[i].event->data);
                lv2_event_begin(&evInEventIters[i], fEventsIn.data[i].event);
            }
            else if (fEventsIn.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
            {
                fEventsIn.data[i].midi.event_count = 0;
                fEventsIn.data[i].midi.size        = 0;
                evInMidiStates[i].midi        = &fEventsIn.data[i].midi;
                evInMidiStates[i].frame_count = frames;
                evInMidiStates[i].position    = 0;
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
            uint8_t midiData[3] = { 0, 0, 0 };

            if (fEventsIn.ctrl != nullptr && (fEventsIn.ctrl->type & CARLA_EVENT_TYPE_MIDI) != 0)
            {
                const uint32_t j = fEventsIn.ctrlIndex;
                CARLA_SAFE_ASSERT(j < fEventsIn.count);

                if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                {
                    for (uint8_t i=0; i < MAX_MIDI_CHANNELS; ++i)
                    {
                        midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT));
                        midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                            lv2_atom_buffer_write(&evInAtomIters[j], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&evInEventIters[j], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&evInMidiStates[j], 0.0, 3, midiData);

                        midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT));
                        midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                            lv2_atom_buffer_write(&evInAtomIters[j], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&evInEventIters[j], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&evInMidiStates[j], 0.0, 3, midiData);
                    }
                }
                else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
                {
                    for (uint8_t k=0; k < MAX_MIDI_NOTE; ++k)
                    {
                        midiData[0] = uint8_t(MIDI_STATUS_NOTE_OFF | (pData->ctrlChannel & MIDI_CHANNEL_BIT));
                        midiData[1] = k;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                            lv2_atom_buffer_write(&evInAtomIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&evInEventIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&evInMidiStates[k], 0.0, 3, midiData);
                    }
                }
            }

#ifndef BUILD_BRIDGE
#if 0 // TODO
            if (pData->latency > 0)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                    FloatVectorOperations::clear(pData->latencyBuffers[i], static_cast<int>(pData->latency));
            }
#endif
#endif

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // TimeInfo

        const EngineTimeInfo& timeInfo(pData->engine->getTimeInfo());

        if (fFirstActive || fLastTimeInfo != timeInfo)
        {
            bool doPostRt;
            int32_t rindex;

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
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && fLastTimeInfo.bbt.bar != timeInfo.bbt.bar)
                    {
                        fParamBuffers[k] = static_cast<float>(timeInfo.bbt.bar - 1);
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_BAR_BEAT:
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && (fLastTimeInfo.bbt.tick != timeInfo.bbt.tick ||
                                                                              fLastTimeInfo.bbt.beat != timeInfo.bbt.beat))
                    {
                        fParamBuffers[k] = static_cast<float>(static_cast<double>(timeInfo.bbt.beat) - 1.0 + (static_cast<double>(timeInfo.bbt.tick) / timeInfo.bbt.ticksPerBeat));
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_BEAT:
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && fLastTimeInfo.bbt.beat != timeInfo.bbt.beat)
                    {
                        fParamBuffers[k] = static_cast<float>(timeInfo.bbt.beat - 1);
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_BEAT_UNIT:
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && carla_isNotEqual(fLastTimeInfo.bbt.beatType, timeInfo.bbt.beatType))
                    {
                        fParamBuffers[k] = timeInfo.bbt.beatType;
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR:
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && carla_isNotEqual(fLastTimeInfo.bbt.beatsPerBar, timeInfo.bbt.beatsPerBar))
                    {
                        fParamBuffers[k] = timeInfo.bbt.beatsPerBar;
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE:
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && carla_isNotEqual(fLastTimeInfo.bbt.beatsPerMinute, timeInfo.bbt.beatsPerMinute))
                    {
                        fParamBuffers[k] = static_cast<float>(timeInfo.bbt.beatsPerMinute);
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_TICKS_PER_BEAT:
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && carla_isNotEqual(fLastTimeInfo.bbt.ticksPerBeat, timeInfo.bbt.ticksPerBeat))
                    {
                        fParamBuffers[k] = static_cast<float>(timeInfo.bbt.ticksPerBeat);
                        doPostRt = true;
                    }
                    break;
                }

                if (doPostRt)
                    pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 1, fParamBuffers[k]);
            }

            for (uint32_t i=0; i < fEventsIn.count; ++i)
            {
                if ((fEventsIn.data[i].type & CARLA_EVENT_DATA_ATOM) == 0 || (fEventsIn.data[i].type & CARLA_EVENT_TYPE_TIME) == 0)
                    continue;

                uint8_t timeInfoBuf[256];
                lv2_atom_forge_set_buffer(&fAtomForge, timeInfoBuf, sizeof(timeInfoBuf));

                LV2_Atom_Forge_Frame forgeFrame;
                lv2_atom_forge_object(&fAtomForge, &forgeFrame, CARLA_URI_MAP_ID_NULL, CARLA_URI_MAP_ID_TIME_POSITION);

                lv2_atom_forge_key(&fAtomForge, CARLA_URI_MAP_ID_TIME_SPEED);
                lv2_atom_forge_float(&fAtomForge, timeInfo.playing ? 1.0f : 0.0f);

                lv2_atom_forge_key(&fAtomForge, CARLA_URI_MAP_ID_TIME_FRAME);
                lv2_atom_forge_long(&fAtomForge, static_cast<int64_t>(timeInfo.frame));

                if (timeInfo.valid & EngineTimeInfo::kValidBBT)
                {
                    lv2_atom_forge_key(&fAtomForge, CARLA_URI_MAP_ID_TIME_BAR);
                    lv2_atom_forge_long(&fAtomForge, timeInfo.bbt.bar - 1);

                    lv2_atom_forge_key(&fAtomForge, CARLA_URI_MAP_ID_TIME_BAR_BEAT);
                    lv2_atom_forge_float(&fAtomForge, static_cast<float>(static_cast<double>(timeInfo.bbt.beat) - 1.0 + (static_cast<double>(timeInfo.bbt.tick) / timeInfo.bbt.ticksPerBeat)));

                    lv2_atom_forge_key(&fAtomForge, CARLA_URI_MAP_ID_TIME_BEAT);
                    lv2_atom_forge_double(&fAtomForge, timeInfo.bbt.beat - 1);

                    lv2_atom_forge_key(&fAtomForge, CARLA_URI_MAP_ID_TIME_BEAT_UNIT);
                    lv2_atom_forge_int(&fAtomForge, static_cast<int32_t>(timeInfo.bbt.beatType));

                    lv2_atom_forge_key(&fAtomForge, CARLA_URI_MAP_ID_TIME_BEATS_PER_BAR);
                    lv2_atom_forge_float(&fAtomForge, timeInfo.bbt.beatsPerBar);

                    lv2_atom_forge_key(&fAtomForge, CARLA_URI_MAP_ID_TIME_BEATS_PER_MINUTE);
                    lv2_atom_forge_float(&fAtomForge, static_cast<float>(timeInfo.bbt.beatsPerMinute));

                    lv2_atom_forge_key(&fAtomForge, CARLA_URI_MAP_ID_TIME_TICKS_PER_BEAT);
                    lv2_atom_forge_double(&fAtomForge, timeInfo.bbt.ticksPerBeat);
                }

                lv2_atom_forge_pop(&fAtomForge, &forgeFrame);

                LV2_Atom* const atom((LV2_Atom*)timeInfoBuf);
                CARLA_SAFE_ASSERT_BREAK(atom->size < 256);

                // send only deprecated blank object for now
                lv2_atom_buffer_write(&evInAtomIters[i], 0, 0, CARLA_URI_MAP_ID_ATOM_BLANK, atom->size, LV2_ATOM_BODY_CONST(atom));

                // for atom:object
                //lv2_atom_buffer_write(&evInAtomIters[i], 0, 0, atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));
            }

            pData->postRtEvents.trySplice();

            carla_copyStruct(fLastTimeInfo, timeInfo);
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (fEventsIn.ctrl != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // Message Input

            if (fAtomBufferIn.tryLock())
            {
                if (fAtomBufferIn.isDataAvailableForReading())
                {
                    const LV2_Atom* atom;
                    uint32_t j, portIndex;

                    for (; fAtomBufferIn.get(atom, portIndex);)
                    {
                        j = (portIndex < fEventsIn.count) ? portIndex : fEventsIn.ctrlIndex;

                        if (atom->type == CARLA_URI_MAP_ID_CARLA_ATOM_WORKER)
                        {
                            CARLA_SAFE_ASSERT_CONTINUE(fExt.worker != nullptr && fExt.worker->work_response != nullptr);
                            fExt.worker->work_response(fHandle, atom->size, LV2_ATOM_BODY_CONST(atom));
                        }
                        else if (! lv2_atom_buffer_write(&evInAtomIters[j], 0, 0, atom->type, atom->size, LV2_ATOM_BODY_CONST(atom)))
                        {
                            carla_stdout("Event input buffer full, at least 1 message lost");
                            continue;
                        }
                    }
                }

                fAtomBufferIn.unlock();
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
                        const ExternalMidiNote& note(it.getValue());

                        CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                        uint8_t midiEvent[3];
                        midiEvent[0] = uint8_t((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT));
                        midiEvent[1] = note.note;
                        midiEvent[2] = note.velo;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                            lv2_atom_buffer_write(&evInAtomIters[j], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiEvent);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&evInEventIters[j], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiEvent);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&evInMidiStates[j], 0.0, 3, midiEvent);
                    }

                    pData->extNotes.data.clear();
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE
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

            const uint32_t numEvents = (fEventsIn.ctrl->port != nullptr) ? fEventsIn.ctrl->port->getEventCount() : 0;

            for (uint32_t i=0; i < numEvents; ++i)
            {
                const EngineEvent& event(fEventsIn.ctrl->port->getEvent(i));

                if (event.time >= frames)
                    continue;

                CARLA_ASSERT_INT2(event.time >= timeOffset, event.time, timeOffset);

                if (isSampleAccurate && event.time > timeOffset)
                {
                    if (processSingle(audioIn, audioOut, cvIn, cvOut, event.time - timeOffset, timeOffset))
                    {
                        startTime  = 0;
                        timeOffset = event.time;

                        if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                            nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
                        else
                            nextBankId = 0;

                        // reset iters
                        const uint32_t j = fEventsIn.ctrlIndex;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                        {
                            lv2_atom_buffer_reset(fEventsIn.data[j].atom, true);
                            lv2_atom_buffer_begin(&evInAtomIters[j], fEventsIn.data[j].atom);
                        }
                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                        {
                            lv2_event_buffer_reset(fEventsIn.data[j].event, LV2_EVENT_AUDIO_STAMP, fEventsIn.data[j].event->data);
                            lv2_event_begin(&evInEventIters[j], fEventsIn.data[j].event);
                        }
                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                        {
                            fEventsIn.data[j].midi.event_count = 0;
                            fEventsIn.data[j].midi.size        = 0;
                            evInMidiStates[j].position = event.time;
                        }
                    }
                    else
                        startTime += timeOffset;
                }

                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl: {
                    const EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter: {
#ifndef BUILD_BRIDGE
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                                break;
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                                break;
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
                            {
                                float left, right;
                                value = ctrlEvent.value/0.5f - 1.0f;

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

                                setBalanceLeft(left, false, false);
                                setBalanceRight(right, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                                break;
                            }
                        }
#endif
                        // Control plugin parameters
                        uint32_t k;
                        for (k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            float value;

                            if (pData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5f) ? pData->param.ranges[k].min : pData->param.ranges[k].max;
                            }
                            else
                            {
                                value = pData->param.ranges[k].getUnnormalizedValue(ctrlEvent.value);

                                if (pData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, value);
                            break;
                        }

                        // check if event is already handled
                        if (k != pData->param.count)
                            break;

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_CONTROL)
                        {
                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = uint8_t(ctrlEvent.param);
                            midiData[2] = uint8_t(ctrlEvent.value*127.0f);

                            const uint32_t mtime(isSampleAccurate ? startTime : event.time);

                            if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                                lv2_atom_buffer_write(&evInAtomIters[fEventsIn.ctrlIndex], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&evInEventIters[fEventsIn.ctrlIndex], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&evInMidiStates[fEventsIn.ctrlIndex], mtime, 3, midiData);
                        }
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

                            const uint32_t mtime(isSampleAccurate ? startTime : event.time);

                            if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                                lv2_atom_buffer_write(&evInAtomIters[fEventsIn.ctrlIndex], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&evInEventIters[fEventsIn.ctrlIndex], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&evInMidiStates[fEventsIn.ctrlIndex], mtime, 3, midiData);
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
                                        const int32_t index(static_cast<int32_t>(k));
                                        setMidiProgram(index, false, false, false);
                                        pData->postponeRtEvent(kPluginPostRtEventMidiProgramChange, index, 0, 0.0f);
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

                            const uint32_t mtime(isSampleAccurate ? startTime : event.time);

                            if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                                lv2_atom_buffer_write(&evInAtomIters[fEventsIn.ctrlIndex], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 2, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&evInEventIters[fEventsIn.ctrlIndex], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 2, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&evInMidiStates[fEventsIn.ctrlIndex], mtime, 2, midiData);
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            const uint32_t mtime(isSampleAccurate ? startTime : event.time);

                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                            midiData[2] = 0;

                            if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                                lv2_atom_buffer_write(&evInAtomIters[fEventsIn.ctrlIndex], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&evInEventIters[fEventsIn.ctrlIndex], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&evInMidiStates[fEventsIn.ctrlIndex], mtime, 3, midiData);
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }
#endif

                            const uint32_t mtime(isSampleAccurate ? startTime : event.time);

                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                            midiData[2] = 0;

                            if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                                lv2_atom_buffer_write(&evInAtomIters[fEventsIn.ctrlIndex], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&evInEventIters[fEventsIn.ctrlIndex], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&evInMidiStates[fEventsIn.ctrlIndex], mtime, 3, midiData);
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    const EngineMidiEvent& midiEvent(event.midi);

                    const uint8_t* const midiData(midiEvent.size > EngineMidiEvent::kDataSize ? midiEvent.dataExt : midiEvent.data);

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiData));

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
                    const uint32_t mtime = isSampleAccurate ? startTime : event.time;

                    // put back channel in data
                    uint8_t midiData2[midiEvent.size];
                    midiData2[0] = uint8_t(status | (event.channel & MIDI_CHANNEL_BIT));
                    std::memcpy(midiData2+1, midiData+1, static_cast<std::size_t>(midiEvent.size-1));

                    if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                        lv2_atom_buffer_write(&evInAtomIters[j], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, midiEvent.size, midiData2);

                    else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                        lv2_event_write(&evInEventIters[j], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, midiEvent.size, midiData2);

                    else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                        lv2midi_put_event(&evInMidiStates[j], mtime, midiEvent.size, midiData2);

                    if (status == MIDI_STATUS_NOTE_ON)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, event.channel, midiData[1], midiData[2]);
                    else if (status == MIDI_STATUS_NOTE_OFF)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, event.channel, midiData[1], 0.0f);
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

#ifndef BUILD_BRIDGE
#if 0 // TODO
        // --------------------------------------------------------------------------------------------------------
        // Latency, save values for next callback

        if (fLatencyIndex != -1)
        {
            if (pData->latency != static_cast<uint32_t>(fParamBuffers[fLatencyIndex]))
            {
                fLatencyChanged = true;
            }
            else if (pData->latency > 0)
            {
                if (pData->latency <= frames)
                {
                    for (uint32_t i=0; i < pData->audioIn.count; ++i)
                        FloatVectorOperations::copy(pData->latencyBuffers[i], audioIn[i]+(frames-pData->latency), static_cast<int>(pData->latency));
                }
                else
                {
                    for (uint32_t i=0, j, k; i < pData->audioIn.count; ++i)
                    {
                        for (k=0; k < pData->latency-frames; ++k)
                            pData->latencyBuffers[i][k] = pData->latencyBuffers[i][k+frames];
                        for (j=0; k < pData->latency; ++j, ++k)
                            pData->latencyBuffers[i][k] = audioIn[i][j];
                    }
                }
            }
        }
#endif
#endif

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (fEventsOut.ctrl != nullptr)
        {
            if (fEventsOut.ctrl->type & CARLA_EVENT_DATA_ATOM)
            {
                const LV2_Atom_Event* ev;
                LV2_Atom_Buffer_Iterator iter;

                uint8_t* data;
                lv2_atom_buffer_begin(&iter, fEventsOut.ctrl->atom);

                for (;;)
                {
                    data = nullptr;
                    ev = lv2_atom_buffer_get(&iter, &data);

                    if (ev == nullptr || ev->body.size == 0 || data == nullptr)
                        break;

                    if (ev->body.type == CARLA_URI_MAP_ID_MIDI_EVENT)
                    {
                        if (fEventsOut.ctrl->port != nullptr)
                        {
                            CARLA_SAFE_ASSERT_CONTINUE(ev->time.frames >= 0);
                            CARLA_SAFE_ASSERT_CONTINUE(ev->body.size < 0xFF);
                            fEventsOut.ctrl->port->writeMidiEvent(static_cast<uint32_t>(ev->time.frames), static_cast<uint8_t>(ev->body.size), data);
                        }
                    }
                    else //if (ev->body.type == CARLA_URI_MAP_ID_ATOM_BLANK)
                    {
                        //carla_stdout("Got out event, %s", carla_lv2_urid_unmap(this, ev->body.type));
                        fAtomBufferOut.put(&ev->body, fEventsOut.ctrl->rindex);
                    }

                    lv2_atom_buffer_increment(&iter);
                }
            }
            else if ((fEventsOut.ctrl->type & CARLA_EVENT_DATA_EVENT) != 0 && fEventsOut.ctrl->port != nullptr)
            {
                const LV2_Event* ev;
                LV2_Event_Iterator iter;

                uint8_t* data;
                lv2_event_begin(&iter, fEventsOut.ctrl->event);

                for (;;)
                {
                    data = nullptr;
                    ev = lv2_event_get(&iter, &data);

                    if (ev == nullptr || data == nullptr)
                        break;

                    if (ev->type == CARLA_URI_MAP_ID_MIDI_EVENT)
                    {
                        CARLA_SAFE_ASSERT_CONTINUE(ev->size < 0xFF);
                        fEventsOut.ctrl->port->writeMidiEvent(ev->frames, static_cast<uint8_t>(ev->size), data);
                    }

                    lv2_event_increment(&iter);
                }
            }
            else if ((fEventsOut.ctrl->type & CARLA_EVENT_DATA_MIDI_LL) != 0 && fEventsOut.ctrl->port != nullptr)
            {
                LV2_MIDIState state = { &fEventsOut.ctrl->midi, frames, 0 };

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

                    fEventsOut.ctrl->port->writeMidiEvent(static_cast<uint32_t>(eventTime), static_cast<uint8_t>(eventSize), eventData);
                    lv2midi_step(&state);
                }
            }
        }

#ifndef BUILD_BRIDGE
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

                pData->param.ranges[k].fixValue(fParamBuffers[k]);

                if (pData->param.data[k].midiCC > 0)
                {
                    channel = pData->param.data[k].midiChannel;
                    param   = static_cast<uint16_t>(pData->param.data[k].midiCC);
                    value   = pData->param.ranges[k].getNormalizedValue(fParamBuffers[k]);
                    pData->event.portOut->writeControlEvent(0, channel, kEngineControlEventTypeParameter, param, value);
                }
            }
        } // End of Control Output
#endif

        // --------------------------------------------------------------------------------------------------------
        // Final work

        if (fExt.worker != nullptr && fExt.worker->end_run != nullptr)
        {
            fExt.worker->end_run(fHandle);

            if (fHandle2 != nullptr)
                fExt.worker->end_run(fHandle2);
        }

        fFirstActive = false;

        // --------------------------------------------------------------------------------------------------------
    }

    bool processSingle(const float** const audioIn, float** const audioOut, const float** const cvIn, float** const cvOut, const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioIn != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioOut != nullptr, false);
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

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
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
            FloatVectorOperations::copy(fAudioInBuffers[i], audioIn[i]+timeOffset, static_cast<int>(frames));

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
            FloatVectorOperations::clear(fAudioOutBuffers[i], static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------
        // Set CV buffers

        for (uint32_t i=0; i < pData->cvIn.count; ++i)
            FloatVectorOperations::copy(fCvInBuffers[i], cvIn[i]+timeOffset, static_cast<int>(frames));

        for (uint32_t i=0; i < pData->cvOut.count; ++i)
            FloatVectorOperations::clear(fCvOutBuffers[i], static_cast<int>(frames));

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
                    pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, fParamBuffers[k]);
                }
            }
        }

        pData->postRtEvents.trySplice();

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && carla_isNotEqual(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));
            const bool isMono    = (pData->audioIn.count == 1);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (uint32_t k=0; k < frames; ++k)
                    {
#if 0 // TODO
                        if (k < pData->latency)
                            bufValue = pData->latencyBuffers[isMono ? 0 : i][k];
                        else if (pData->latency < frames)
                            bufValue = fAudioInBuffers[isMono ? 0 : i][k-pData->latency];
                        else
#endif
                            bufValue = fAudioInBuffers[isMono ? 0 : i][k];

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
                        FloatVectorOperations::copy(oldBufLeft, fAudioOutBuffers[i], static_cast<int>(frames));
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

#else // BUILD_BRIDGE
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
                fExt.options->set(fHandle, &fLv2Options.opts[CarlaPluginLV2Options::MaxBlockLenth]);
                fExt.options->set(fHandle, &fLv2Options.opts[CarlaPluginLV2Options::NominalBlockLenth]);

                if (fLv2Options.minBufferSize != 1)
                    fExt.options->set(fHandle, &fLv2Options.opts[CarlaPluginLV2Options::MinBlockLenth]);
            }
        }

        carla_debug("CarlaPluginLV2::bufferSizeChanged(%i) - end", newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("CarlaPluginLV2::sampleRateChanged(%g) - start", newSampleRate);

        if (carla_isNotEqual(fLv2Options.sampleRate, newSampleRate))
        {
            fLv2Options.sampleRate = newSampleRate;

            if (fExt.options != nullptr && fExt.options->set != nullptr)
                fExt.options->set(fHandle, &fLv2Options.opts[CarlaPluginLV2Options::SampleRate]);
        }

        for (uint32_t k=0; k < pData->param.count; ++k)
        {
            if (pData->param.data[k].type == PARAMETER_INPUT && pData->param.special[k] == PARAMETER_SPECIAL_SAMPLE_RATE)
            {
                fParamBuffers[k] = static_cast<float>(newSampleRate);
                pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 1, fParamBuffers[k]);
                break;
            }
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
                pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 1, fParamBuffers[k]);
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

        fEventsIn.clear();
        fEventsOut.clear();

        CarlaPlugin::clearBuffers();

        carla_debug("CarlaPluginLV2::clearBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    void uiParameterChange(const uint32_t index, const float value) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type != UI::TYPE_NULL,);
        CARLA_SAFE_ASSERT_RETURN(index < pData->param.count,);

        if (fUI.type == UI::TYPE_BRIDGE)
        {
            if (fPipeServer.isPipeRunning())
                fPipeServer.writeControlMessage(static_cast<uint32_t>(pData->param.data[index].rindex), value);
        }
        else
        {
            if (fUI.handle != nullptr && fUI.descriptor != nullptr && fUI.descriptor->port_event != nullptr && ! fNeedsUiClose)
            {
                CARLA_SAFE_ASSERT_RETURN(pData->param.data[index].rindex >= 0,);
                fUI.descriptor->port_event(fUI.handle, static_cast<uint32_t>(pData->param.data[index].rindex), sizeof(float), CARLA_URI_MAP_ID_NULL, &value);
            }
        }
    }

    void uiMidiProgramChange(const uint32_t index) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type != UI::TYPE_NULL,);
        CARLA_SAFE_ASSERT_RETURN(index < pData->midiprog.count,);

        if (fUI.type == UI::TYPE_BRIDGE)
        {
            if (fPipeServer.isPipeRunning())
                fPipeServer.writeProgramMessage(index);
        }
        else
        {
            if (fExt.uiprograms != nullptr && fExt.uiprograms->select_program != nullptr && ! fNeedsUiClose)
                fExt.uiprograms->select_program(fUI.handle, pData->midiprog.data[index].bank, pData->midiprog.data[index].program);
        }
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type != UI::TYPE_NULL,);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
        CARLA_SAFE_ASSERT_RETURN(velo > 0 && velo < MAX_MIDI_VALUE,);

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
                midiEv.atom.type = CARLA_URI_MAP_ID_MIDI_EVENT;
                midiEv.atom.size = 3;
                midiEv.data[0] = uint8_t(MIDI_STATUS_NOTE_ON | (channel & MIDI_CHANNEL_BIT));
                midiEv.data[1] = note;
                midiEv.data[2] = velo;

                fUI.descriptor->port_event(fUI.handle, fEventsIn.ctrl->rindex, lv2_atom_total_size(midiEv), CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, &midiEv);
            }
        }
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.type != UI::TYPE_NULL,);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);

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
                midiEv.atom.type = CARLA_URI_MAP_ID_MIDI_EVENT;
                midiEv.atom.size = 3;
                midiEv.data[0] = uint8_t(MIDI_STATUS_NOTE_OFF | (channel & MIDI_CHANNEL_BIT));
                midiEv.data[1] = note;
                midiEv.data[2] = 0;

                fUI.descriptor->port_event(fUI.handle, fEventsIn.ctrl->rindex, lv2_atom_total_size(midiEv), CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, &midiEv);
            }
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

    bool needsFixedBuffer() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);

        for (uint32_t i=0; i < fRdfDescriptor->FeatureCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Features[i].URI, LV2_BUF_SIZE__fixedBlockLength) == 0)
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
        CarlaString bridgeBinary(pData->engine->getOptions().binaryDir);

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
        case LV2_UI_EXTERNAL:
        case LV2_UI_OLD_EXTERNAL:
            bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-lv2-external";
            break;
        case LV2_UI_MOD:
            bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-lv2-modgui";
            break;
        default:
            return nullptr;
        }

#ifdef CARLA_OS_WIN
        bridgeBinary += ".exe";
#endif

        if (! File(bridgeBinary.buffer()).existsAsFile())
            return nullptr;

        return bridgeBinary.dup();
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

        if (fRdfDescriptor->ExtensionCount == 0 || fDescriptor->extension_data == nullptr)
            return;

        for (uint32_t i=0; i < fRdfDescriptor->ExtensionCount; ++i)
        {
            CARLA_SAFE_ASSERT_CONTINUE(fRdfDescriptor->Extensions[i] != nullptr);

            if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_OPTIONS__interface) == 0)
                pData->hints |= PLUGIN_HAS_EXTENSION_OPTIONS;
            else if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_PROGRAMS__Interface) == 0)
                pData->hints |= PLUGIN_HAS_EXTENSION_PROGRAMS;
            else if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_STATE__interface) == 0)
                pData->hints |= PLUGIN_HAS_EXTENSION_STATE;
            else if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_WORKER__interface) == 0)
                pData->hints |= PLUGIN_HAS_EXTENSION_WORKER;
            else
                carla_stdout("Plugin has non-supported extension: '%s'", fRdfDescriptor->Extensions[i]);
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

            // check if invalid
            if (fExt.options != nullptr && fExt.options->get == nullptr  && fExt.options->set == nullptr)
                fExt.options = nullptr;

            if (fExt.programs != nullptr && (fExt.programs->get_program == nullptr || fExt.programs->select_program == nullptr))
                fExt.programs = nullptr;

            if (fExt.state != nullptr && (fExt.state->save == nullptr || fExt.state->restore == nullptr))
                fExt.state = nullptr;

            if (fExt.worker != nullptr && fExt.worker->work == nullptr)
                fExt.worker = nullptr;
        }

        CARLA_SAFE_ASSERT_RETURN(fLatencyIndex == -1,);

        for (uint32_t i=0, count=fRdfDescriptor->PortCount, iCtrl=0; i<count; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (! LV2_IS_PORT_CONTROL(portTypes))
                continue;

            iCtrl++;

            if (! LV2_IS_PORT_OUTPUT(portTypes))
                continue;

            const LV2_Property portDesignation(fRdfDescriptor->Ports[i].Designation);

            if (! LV2_IS_PORT_DESIGNATION_LATENCY(portDesignation))
                continue;

            fLatencyIndex = static_cast<int32_t>(iCtrl);
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
                fUI.descriptor->port_event(fUI.handle, static_cast<uint32_t>(pData->param.data[i].rindex), sizeof(float), CARLA_URI_MAP_ID_NULL, &value);
            }
        }
    }

    // -------------------------------------------------------------------

    LV2_URID getCustomURID(const char* const uri)
    {
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', CARLA_URI_MAP_ID_NULL);
        carla_debug("CarlaPluginLV2::getCustomURID(\"%s\")", uri);

        for (size_t i=0; i < fCustomURIDs.count(); ++i)
        {
            const char* const thisUri(fCustomURIDs.getAt(i, nullptr));
            if (thisUri != nullptr && std::strcmp(thisUri, uri) == 0)
                return static_cast<LV2_URID>(i);
        }

        const LV2_URID urid(static_cast<LV2_URID>(fCustomURIDs.count()));

        fCustomURIDs.append(carla_strdup(uri));

        if (fUI.type == UI::TYPE_BRIDGE && fPipeServer.isPipeRunning())
            fPipeServer.writeLv2UridMessage(urid, uri);

        return urid;
    }

    const char* getCustomURIDString(const LV2_URID urid) const noexcept
    {
        static const char* const sFallback = "urn:null";
        CARLA_SAFE_ASSERT_RETURN(urid != CARLA_URI_MAP_ID_NULL, sFallback);
        CARLA_SAFE_ASSERT_RETURN(urid < fCustomURIDs.count(), sFallback);
        carla_debug("CarlaPluginLV2::getCustomURIString(%i)", urid);

        return fCustomURIDs.getAt(urid, sFallback);
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
                    pData->engine->callback(ENGINE_CALLBACK_UPDATE, pData->id, 0, 0, 0.0, nullptr);
                else
                    pData->engine->callback(ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0.0, nullptr);
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

    LV2_State_Status handleStateStore(const uint32_t key, const void* const value, const size_t size, const uint32_t type, const uint32_t flags)
    {
        CARLA_SAFE_ASSERT_RETURN(key != CARLA_URI_MAP_ID_NULL, LV2_STATE_ERR_NO_PROPERTY);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr, LV2_STATE_ERR_NO_PROPERTY);
        CARLA_SAFE_ASSERT_RETURN(size > 0, LV2_STATE_ERR_NO_PROPERTY);
        CARLA_SAFE_ASSERT_RETURN(type != CARLA_URI_MAP_ID_NULL, LV2_STATE_ERR_BAD_TYPE);
        CARLA_SAFE_ASSERT_RETURN(flags & LV2_STATE_IS_POD, LV2_STATE_ERR_BAD_FLAGS);
        carla_debug("CarlaPluginLV2::handleStateStore(%i:\"%s\", %p, " P_SIZE ", %i:\"%s\", %i)", key, carla_lv2_urid_unmap(this, key), value, size, type, carla_lv2_urid_unmap(this, type), flags);

        const char* const skey(carla_lv2_urid_unmap(this, key));
        const char* const stype(carla_lv2_urid_unmap(this, type));

        CARLA_SAFE_ASSERT_RETURN(skey != nullptr, LV2_STATE_ERR_BAD_TYPE);
        CARLA_SAFE_ASSERT_RETURN(stype != nullptr, LV2_STATE_ERR_BAD_TYPE);

        // Check if we already have this key
        for (LinkedList<CustomData>::Itenerator it = pData->custom.begin2(); it.valid(); it.next())
        {
            CustomData& data(it.getValue());

            if (std::strcmp(data.key, skey) == 0)
            {
                // found it
                if (data.value != nullptr)
                    delete[] data.value;

                if (type == CARLA_URI_MAP_ID_ATOM_STRING || type == CARLA_URI_MAP_ID_ATOM_PATH)
                    data.value = carla_strdup((const char*)value);
                else
                    data.value = CarlaString::asBase64(value, size).dup();

                return LV2_STATE_SUCCESS;
            }
        }

        // Otherwise store it
        CustomData newData;
        newData.type = carla_strdup(stype);
        newData.key  = carla_strdup(skey);

        if (type == CARLA_URI_MAP_ID_ATOM_STRING || type == CARLA_URI_MAP_ID_ATOM_PATH)
            newData.value = carla_strdup((const char*)value);
        else
            newData.value = CarlaString::asBase64(value, size).dup();

        pData->custom.append(newData);

        return LV2_STATE_SUCCESS;
    }

    const void* handleStateRetrieve(const uint32_t key, size_t* const size, uint32_t* const type, uint32_t* const flags)
    {
        CARLA_SAFE_ASSERT_RETURN(key != CARLA_URI_MAP_ID_NULL, nullptr);
        CARLA_SAFE_ASSERT_RETURN(size != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(type != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(flags != nullptr, nullptr);
        carla_debug("CarlaPluginLV2::handleStateRetrieve(%i, %p, %p, %p)", key, size, type, flags);

        const char* const skey(carla_lv2_urid_unmap(this, key));

        CARLA_SAFE_ASSERT_RETURN(skey != nullptr, nullptr);

        const char* stype = nullptr;
        const char* stringData = nullptr;

        for (LinkedList<CustomData>::Itenerator it = pData->custom.begin2(); it.valid(); it.next())
        {
            const CustomData& data(it.getValue());

            if (std::strcmp(data.key, skey) == 0)
            {
                stype      = data.type;
                stringData = data.value;
                break;
            }
        }

        CARLA_SAFE_ASSERT_RETURN(stype != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(stringData != nullptr, nullptr);

        *type  = carla_lv2_urid_map(this, stype);
        *flags = LV2_STATE_IS_POD;

        if (*type == CARLA_URI_MAP_ID_ATOM_STRING || *type == CARLA_URI_MAP_ID_ATOM_PATH)
        {
            *size = std::strlen(stringData);
            return stringData;
        }
        else
        {
            if (fLastStateChunk != nullptr)
            {
                std::free(fLastStateChunk);
                fLastStateChunk = nullptr;
            }

            std::vector<uint8_t> chunk(carla_getChunkFromBase64String(stringData));
            CARLA_SAFE_ASSERT_RETURN(chunk.size() > 0, nullptr);

            fLastStateChunk = std::malloc(chunk.size());
            CARLA_SAFE_ASSERT_RETURN(fLastStateChunk != nullptr, nullptr);

            std::memcpy(fLastStateChunk, chunk.data(), chunk.size());

            *size = chunk.size();
            return fLastStateChunk;
        }
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
        atom.type = CARLA_URI_MAP_ID_CARLA_ATOM_WORKER;

        return fAtomBufferOut.putChunk(&atom, data, fEventsOut.ctrlIndex) ? LV2_WORKER_SUCCESS : LV2_WORKER_ERR_NO_SPACE;
    }

    LV2_Worker_Status handleWorkerRespond(const uint32_t size, const void* const data)
    {
        carla_debug("CarlaPluginLV2::handleWorkerRespond(%i, %p)", size, data);

        LV2_Atom atom;
        atom.size = size;
        atom.type = CARLA_URI_MAP_ID_CARLA_ATOM_WORKER;

        return fAtomBufferIn.putChunk(&atom, data, fEventsIn.ctrlIndex) ? LV2_WORKER_SUCCESS : LV2_WORKER_ERR_NO_SPACE;
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

    int handleUIResize(const int width, const int height)
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr, 1);
        CARLA_SAFE_ASSERT_RETURN(width > 0, 1);
        CARLA_SAFE_ASSERT_RETURN(height > 0, 1);
        carla_debug("CarlaPluginLV2::handleUIResize(%i, %i)", width, height);

        fUI.window->setSize(static_cast<uint>(width), static_cast<uint>(height), true);

        return 0;
    }

    void handleUIWrite(const uint32_t rindex, const uint32_t bufferSize, const uint32_t format, const void* const buffer)
    {
        CARLA_SAFE_ASSERT_RETURN(buffer != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(bufferSize > 0,);
        carla_debug("CarlaPluginLV2::handleUIWrite(%i, %i, %i, %p)", rindex, bufferSize, format, buffer);

        uint32_t index = LV2UI_INVALID_PORT_INDEX;

        switch (format)
        {
        case CARLA_URI_MAP_ID_NULL: {
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

            //if (carla_isNotEqual(fParamBuffers[index], value))
            setParameterValue(index, value, false, true, true);

        } break;

        case CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM:
        case CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT: {
            CARLA_SAFE_ASSERT_RETURN(bufferSize >= sizeof(LV2_Atom),);

            const LV2_Atom* const atom((const LV2_Atom*)buffer);

            // plugins sometimes fail on this, not good...
            CARLA_SAFE_ASSERT_INT2(bufferSize == lv2_atom_total_size(atom), bufferSize, atom->size);

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

            fAtomBufferIn.put(atom, index);
        } break;

        default:
            carla_stdout("CarlaPluginLV2::handleUIWrite(%i, %i, %i:\"%s\", %p) - unknown format", rindex, bufferSize, format, carla_lv2_urid_unmap(this, format), buffer);
            break;
        }
    }

    // -------------------------------------------------------------------

    void handleLilvSetPortValue(const char* const portSymbol, const void* const value, const uint32_t size, const uint32_t type)
    {
        CARLA_SAFE_ASSERT_RETURN(portSymbol != nullptr && portSymbol[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(size > 0,);
        CARLA_SAFE_ASSERT_RETURN(type != CARLA_URI_MAP_ID_NULL,);
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
        case CARLA_URI_MAP_ID_ATOM_BOOL:
            CARLA_SAFE_ASSERT_RETURN(size == sizeof(bool),);
            paramValue = (*(const bool*)value) ? 1.0f : 0.0f;
            break;
        case CARLA_URI_MAP_ID_ATOM_DOUBLE:
            CARLA_SAFE_ASSERT_RETURN(size == sizeof(double),);
            paramValue = static_cast<float>((*(const double*)value));
            break;
        case CARLA_URI_MAP_ID_ATOM_FLOAT:
            CARLA_SAFE_ASSERT_RETURN(size == sizeof(float),);
            paramValue = (*(const float*)value);
            break;
        case CARLA_URI_MAP_ID_ATOM_INT:
            CARLA_SAFE_ASSERT_RETURN(size == sizeof(int32_t),);
            paramValue = static_cast<float>((*(const int32_t*)value));
            break;
        case CARLA_URI_MAP_ID_ATOM_LONG:
            CARLA_SAFE_ASSERT_RETURN(size == sizeof(int64_t),);
            paramValue = static_cast<float>((*(const int64_t*)value));
            break;
        default:
            carla_stdout("CarlaPluginLV2::handleLilvSetPortValue(\"%s\", %p, %i, %i:\"%s\") - unknown type", portSymbol, value, size, type, carla_lv2_urid_unmap(this, type));
            return;
        }

        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            if (pData->param.data[i].rindex == rindex)
            {
                setParameterValue(i, paramValue, true, true, true);
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

    uintptr_t getUiBridgeProcessId() const noexcept override
    {
        return fPipeServer.isPipeRunning() ? fPipeServer.getPID() : 0;
    }

    // -------------------------------------------------------------------

public:
    bool init(const char* const name, const char* const uri)
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

        // ---------------------------------------------------------------
        // Init LV2 World if needed, sets LV2_PATH for lilv

        Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());

        if (pData->engine->getOptions().pathLV2 != nullptr && pData->engine->getOptions().pathLV2[0] != '\0')
            lv2World.initIfNeeded(pData->engine->getOptions().pathLV2);
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

        // ---------------------------------------------------------------
        // open DLL

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
            else if (feature.Required && ! is_lv2_feature_supported(feature.URI))
            {
                CarlaString msg("Plugin wants a feature that is not supported:\n");
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

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else
            pData->name = pData->engine->getUniquePluginName(fRdfDescriptor->Name);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(this);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize options

        fLv2Options.minBufferSize     = 1;
        fLv2Options.maxBufferSize     = static_cast<int>(pData->engine->getBufferSize());
        fLv2Options.nominalBufferSize = fLv2Options.maxBufferSize;
        fLv2Options.sampleRate        = pData->engine->getSampleRate();
        fLv2Options.frontendWinId     = static_cast<int64_t>(pData->engine->getOptions().frontendWinId);

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

        // ---------------------------------------------------------------
        // initialize features (part 1)

        LV2_Event_Feature* const eventFt = new LV2_Event_Feature;
        eventFt->callback_data           = this;
        eventFt->lv2_event_ref           = carla_lv2_event_ref;
        eventFt->lv2_event_unref         = carla_lv2_event_unref;

        LV2_Log_Log* const logFt         = new LV2_Log_Log;
        logFt->handle                    = this;
        logFt->printf                    = carla_lv2_log_printf;
        logFt->vprintf                   = carla_lv2_log_vprintf;

        LV2_State_Make_Path* const stateMakePathFt = new LV2_State_Make_Path;
        stateMakePathFt->handle                    = this;
        stateMakePathFt->path                      = carla_lv2_state_make_path;

        LV2_State_Map_Path* const stateMapPathFt   = new LV2_State_Map_Path;
        stateMapPathFt->handle                     = this;
        stateMapPathFt->abstract_path              = carla_lv2_state_map_abstract_path;
        stateMapPathFt->absolute_path              = carla_lv2_state_map_absolute_path;

        LV2_Programs_Host* const programsFt   = new LV2_Programs_Host;
        programsFt->handle                    = this;
        programsFt->program_changed           = carla_lv2_program_changed;

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

        LV2_URID_Map* const uridMapFt       = new LV2_URID_Map;
        uridMapFt->handle                   = this;
        uridMapFt->map                      = carla_lv2_urid_map;

        LV2_URID_Unmap* const uridUnmapFt   = new LV2_URID_Unmap;
        uridUnmapFt->handle                 = this;
        uridUnmapFt->unmap                  = carla_lv2_urid_unmap;

        LV2_Worker_Schedule* const workerFt = new LV2_Worker_Schedule;
        workerFt->handle                    = this;
        workerFt->schedule_work             = carla_lv2_worker_schedule;

        // ---------------------------------------------------------------
        // initialize features (part 2)

        for (uint32_t j=0; j < kFeatureCountPlugin; ++j)
            fFeatures[j] = new LV2_Feature;

        fFeatures[kFeatureIdBufSizeBounded]->URI   = LV2_BUF_SIZE__boundedBlockLength;
        fFeatures[kFeatureIdBufSizeBounded]->data  = nullptr;

        fFeatures[kFeatureIdBufSizeFixed]->URI     = LV2_BUF_SIZE__fixedBlockLength;
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

        // if a fixed buffer is not needed, skip its feature
        if (! needsFixedBuffer())
            fFeatures[kFeatureIdBufSizeFixed]->URI = LV2_BUF_SIZE__boundedBlockLength;
        else
            fLv2Options.minBufferSize = fLv2Options.maxBufferSize;

        // if power of 2 is not possible, skip its feature FIXME
        //if (fLv2Options.maxBufferSize)
        //    fFeatures[kFeatureIdBufSizePowerOf2]->URI = LV2_BUF_SIZE__boundedBlockLength;

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

        if (std::strcmp(uri, "http://hyperglitch.com/dev/VocProc") == 0)
            fCanInit2 = false;

        recheckExtensions();

        // ---------------------------------------------------------------
        // set default options

        pData->options  = 0x0;

        if (fExt.programs != nullptr && getCategory() == PLUGIN_CATEGORY_SYNTH)
            pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (fLatencyIndex >= 0 || getMidiInCount() > 0 || needsFixedBuffer())
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (fCanInit2 && pData->engine->getOptions().forceStereo)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;

        if (getMidiInCount() > 0)
        {
            pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
            pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        // ---------------------------------------------------------------
        // gui stuff

        if (fRdfDescriptor->UICount != 0)
            initUi();

        return true;
    }

    // -------------------------------------------------------------------

    void initUi()
    {
        // ---------------------------------------------------------------
        // find the most appropriate ui

        int eQt4, eQt5, eGtk2, eGtk3, eCocoa, eWindows, eX11, eExt, eMod, iCocoa, iWindows, iX11, iExt, iFinal;
        eQt4 = eQt5 = eGtk2 = eGtk3 = eCocoa = eWindows = eX11 = eExt = eMod = iCocoa = iWindows = iX11 = iExt = iFinal = -1;

#if defined(BUILD_BRIDGE)
        const bool preferUiBridges(false);
#elif defined(LV2_UIS_ONLY_BRIDGES)
        const bool preferUiBridges(true);
#else
        const bool preferUiBridges(pData->engine->getOptions().preferUiBridges && (pData->hints & PLUGIN_IS_BRIDGE) == 0);
#endif

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
            case LV2_UI_COCOA:
                if (isUiBridgeable(i) && preferUiBridges)
                    eCocoa = ii;
                iCocoa = ii;
                break;
            case LV2_UI_WINDOWS:
                if (isUiBridgeable(i) && preferUiBridges)
                    eWindows = ii;
                iWindows = ii;
                break;
            case LV2_UI_X11:
                if (isUiBridgeable(i) && preferUiBridges)
                    eX11 = ii;
                iX11 = ii;
                break;
            case LV2_UI_EXTERNAL:
            case LV2_UI_OLD_EXTERNAL:
                if (isUiBridgeable(i))
                    eExt = ii;
                iExt = ii;
                break;
            case LV2_UI_MOD:
                eMod = ii;
                break;
            default:
                break;
            }
        }

        if (eQt4 >= 0)
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
        //else if (eExt >= 0) // TODO
        //    iFinal = eExt;
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

        if (iFinal < 0)
        {
            // no suitable UI found, see if there's one which supports ui:showInterface
            bool hasShowInterface = false;

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

            if (! hasShowInterface)
            {
                if (eMod < 0)
                {
                    carla_stderr("Failed to find an appropriate LV2 UI for this plugin");
                    return;
                }

                // use MODGUI as last resort
                iFinal = eMod;
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
                carla_stderr("Plugin UI requires a feature that is not supported:\n%s", uri);

                if (fUI.rdfDescriptor->Features[i].Required)
                {
                    canContinue = false;
                    break;
                }
            }
            if (std::strcmp(uri, LV2_UI__makeResident) == 0)
                canDelete = false;
        }

        if (! canContinue)
        {
            fUI.rdfDescriptor = nullptr;
            return;
        }

        // ---------------------------------------------------------------
        // initialize ui according to type

        const LV2_Property uiType(fUI.rdfDescriptor->Type);

        if (iFinal == eQt4 || iFinal == eQt5 || iFinal == eGtk2 || iFinal == eGtk3 ||
            iFinal == eCocoa || iFinal == eWindows || iFinal == eX11 || iFinal == eExt || iFinal == eMod)
        {
            // -----------------------------------------------------------
            // initialize ui-bridge

            if (const char* const bridgeBinary = getUiBridgeBinary(uiType))
            {
                carla_stdout("Will use UI-Bridge, binary: \"%s\"", bridgeBinary);

                CarlaString guiTitle(pData->name);
                guiTitle += " (GUI)";
                fLv2Options.windowTitle = guiTitle.dup();

                fUI.type = UI::TYPE_BRIDGE;
                fPipeServer.setData(bridgeBinary, fRdfDescriptor->URI, fUI.rdfDescriptor->URI);

                delete[] bridgeBinary;
                return;
            }

            if (iFinal == eQt4 || iFinal == eQt5 || iFinal == eGtk2 || iFinal == eGtk3)
            {
                carla_stderr2("Failed to find UI bridge binary, cannot use UI");
                fUI.rdfDescriptor = nullptr;
                return;
            }
        }

#ifdef LV2_UIS_ONLY_BRIDGES
        carla_stderr2("Failed to get an UI working, canBridge:%s", bool2str(isUiBridgeable(static_cast<uint32_t>(iFinal))));
        fUI.rdfDescriptor = nullptr;
        return;
#endif

        // ---------------------------------------------------------------
        // open UI DLL

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
        case LV2_UI_QT4:
            carla_stdout("Will use LV2 Qt4 UI, NOT!");
            fUI.type = UI::TYPE_EMBED;
            break;
        case LV2_UI_QT5:
            carla_stdout("Will use LV2 Qt5 UI, NOT!");
            fUI.type = UI::TYPE_EMBED;
            break;
        case LV2_UI_GTK2:
            carla_stdout("Will use LV2 Gtk2 UI, NOT!");
            fUI.type = UI::TYPE_EMBED;
            break;
        case LV2_UI_GTK3:
            carla_stdout("Will use LV2 Gtk3 UI, NOT!");
            fUI.type = UI::TYPE_EMBED;
            break;
#ifdef CARLA_OS_MAC
        case LV2_UI_COCOA:
            carla_stdout("Will use LV2 Cocoa UI");
            fUI.type = UI::TYPE_EMBED;
            break;
#endif
#ifdef CARLA_OS_WIN
        case LV2_UI_WINDOWS:
            carla_stdout("Will use LV2 Windows UI");
            fUI.type = UI::TYPE_EMBED;
            break;
#endif
        case LV2_UI_X11:
#ifdef HAVE_X11
            carla_stdout("Will use LV2 X11 UI");
#else
            carla_stdout("Will use LV2 X11 UI, NOT!");
#endif
            fUI.type = UI::TYPE_EMBED;
            break;
        case LV2_UI_EXTERNAL:
        case LV2_UI_OLD_EXTERNAL:
            carla_stdout("Will use LV2 External UI");
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

        CarlaString guiTitle(pData->name);
        guiTitle += " (GUI)";
        fLv2Options.windowTitle = guiTitle.dup();

        fLv2Options.opts[CarlaPluginLV2Options::WindowTitle].size  = (uint32_t)std::strlen(fLv2Options.windowTitle);
        fLv2Options.opts[CarlaPluginLV2Options::WindowTitle].value = fLv2Options.windowTitle;

        // ---------------------------------------------------------------
        // initialize ui features (part 1)

        LV2_Extension_Data_Feature* const uiDataFt = new LV2_Extension_Data_Feature;
        uiDataFt->data_access                      = fDescriptor->extension_data;

        LV2UI_Port_Map* const uiPortMapFt = new LV2UI_Port_Map;
        uiPortMapFt->handle               = this;
        uiPortMapFt->port_index           = carla_lv2_ui_port_map;

        LV2UI_Resize* const uiResizeFt    = new LV2UI_Resize;
        uiResizeFt->handle                = this;
        uiResizeFt->ui_resize             = carla_lv2_ui_resize;

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

        fFeatures[kFeatureIdUiNoUserResize]->URI   = LV2_UI__noUserResize;
        fFeatures[kFeatureIdUiNoUserResize]->data  = nullptr;

        fFeatures[kFeatureIdUiParent]->URI         = LV2_UI__parent;
        fFeatures[kFeatureIdUiParent]->data        = nullptr;

        fFeatures[kFeatureIdUiPortMap]->URI        = LV2_UI__portMap;
        fFeatures[kFeatureIdUiPortMap]->data       = uiPortMapFt;

        fFeatures[kFeatureIdUiPortSubscribe]->URI  = LV2_UI__portSubscribe;
        fFeatures[kFeatureIdUiPortSubscribe]->data = nullptr;

        fFeatures[kFeatureIdUiResize]->URI       = LV2_UI__resize;
        fFeatures[kFeatureIdUiResize]->data      = uiResizeFt;

        fFeatures[kFeatureIdUiTouch]->URI        = LV2_UI__touch;
        fFeatures[kFeatureIdUiTouch]->data       = nullptr;

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

        fAtomBufferIn.put(atom, portIndex);
    }

    void handleUridMap(const LV2_URID urid, const char* const uri)
    {
        CARLA_SAFE_ASSERT_RETURN(urid != CARLA_URI_MAP_ID_NULL,);
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0',);
        carla_debug("CarlaPluginLV2::handleUridMap(%i v " P_SIZE ", \"%s\")", urid, fCustomURIDs.count()-1, uri);

        if (urid < fCustomURIDs.count())
        {
            const char* const ourURI(carla_lv2_urid_unmap(this, urid));
            CARLA_SAFE_ASSERT_RETURN(ourURI != nullptr,);

            if (std::strcmp(ourURI, uri) != 0)
            {
                carla_stderr2("PLUGIN :: wrong URI '%s' vs '%s'", ourURI,  uri);
            }
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(urid == fCustomURIDs.count(),);
            fCustomURIDs.append(carla_strdup(uri));
        }
    }

    // -------------------------------------------------------------------

private:
    LV2_Handle   fHandle;
    LV2_Handle   fHandle2;
    LV2_Feature* fFeatures[kFeatureCountAll+1];
    const LV2_Descriptor*     fDescriptor;
    const LV2_RDF_Descriptor* fRdfDescriptor;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float** fCvInBuffers;
    float** fCvOutBuffers;
    float*  fParamBuffers;

    bool    fCanInit2; // some plugins don't like 2 instances
    bool    fNeedsUiClose;
    bool    fLatencyChanged;
    int32_t fLatencyIndex; // -1 if invalid

    Lv2AtomRingBuffer fAtomBufferIn;
    Lv2AtomRingBuffer fAtomBufferOut;
    LV2_Atom_Forge    fAtomForge;

    CarlaPluginLV2EventData fEventsIn;
    CarlaPluginLV2EventData fEventsOut;
    CarlaPluginLV2Options   fLv2Options;
    CarlaPipeServerLV2      fPipeServer;

    LinkedList<const char*> fCustomURIDs;

    bool fFirstActive; // first process() call after activate()
    void* fLastStateChunk;
    EngineTimeInfo fLastTimeInfo;

    struct Extensions {
        const LV2_Options_Interface* options;
        const LV2_State_Interface* state;
        const LV2_Worker_Interface* worker;
        const LV2_Programs_Interface* programs;
        const LV2UI_Idle_Interface* uiidle;
        const LV2UI_Show_Interface* uishow;
        const LV2UI_Resize* uiresize;
        const LV2_Programs_UI_Interface* uiprograms;

        Extensions()
            : options(nullptr),
              state(nullptr),
              worker(nullptr),
              programs(nullptr),
              uiidle(nullptr),
              uishow(nullptr),
              uiresize(nullptr),
              uiprograms(nullptr) {}

        CARLA_DECLARE_NON_COPY_STRUCT(Extensions);
    } fExt;

    struct UI {
        enum Type {
            TYPE_NULL = 0,
            TYPE_BRIDGE,
            TYPE_EMBED,
            TYPE_EXTERNAL
        };

        Type type;
        LV2UI_Handle handle;
        LV2UI_Widget widget;
        const LV2UI_Descriptor* descriptor;
        const LV2_RDF_UI*       rdfDescriptor;

        CarlaPluginUI* window;

        UI()
            : type(TYPE_NULL),
              handle(nullptr),
              widget(nullptr),
              descriptor(nullptr),
              rdfDescriptor(nullptr),
              window(nullptr) {}

        ~UI()
        {
            CARLA_ASSERT(handle == nullptr);
            CARLA_ASSERT(widget == nullptr);
            CARLA_ASSERT(descriptor == nullptr);
            CARLA_ASSERT(rdfDescriptor == nullptr);
            CARLA_ASSERT(window == nullptr);
        }

        CARLA_DECLARE_NON_COPY_STRUCT(UI);
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
        CARLA_SAFE_ASSERT_RETURN(type != CARLA_URI_MAP_ID_NULL, 0);
        CARLA_SAFE_ASSERT_RETURN(fmt != nullptr, 0);

#ifndef DEBUG
        if (type == CARLA_URI_MAP_ID_LOG_TRACE)
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
        CARLA_SAFE_ASSERT_RETURN(type != CARLA_URI_MAP_ID_NULL, 0);
        CARLA_SAFE_ASSERT_RETURN(fmt != nullptr, 0);

#ifndef DEBUG
        if (type == CARLA_URI_MAP_ID_LOG_TRACE)
            return 0;
#endif

        int ret = 0;

        switch (type)
        {
        case CARLA_URI_MAP_ID_LOG_ERROR:
            std::fprintf(stderr, "\x1b[31m");
            ret = std::vfprintf(stderr, fmt, ap);
            std::fprintf(stderr, "\x1b[0m");
            break;

        case CARLA_URI_MAP_ID_LOG_NOTE:
            ret = std::vfprintf(stdout, fmt, ap);
            break;

        case CARLA_URI_MAP_ID_LOG_TRACE:
#ifdef DEBUG
            std::fprintf(stdout, "\x1b[30;1m");
            ret = std::vfprintf(stdout, fmt, ap);
            std::fprintf(stdout, "\x1b[0m");
#endif
            break;

        case CARLA_URI_MAP_ID_LOG_WARNING:
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

    static char* carla_lv2_state_make_path(LV2_State_Make_Path_Handle handle, const char* path)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(path != nullptr && path[0] != '\0', nullptr);
        carla_debug("carla_lv2_state_make_path(%p, \"%s\")", handle, path);

        File file;

        if (File::isAbsolutePath(path))
            file = File(path);
        else
            file = File::getCurrentWorkingDirectory().getChildFile(path);

        file.getParentDirectory().createDirectory();

        return strdup(file.getFullPathName().toRawUTF8());
    }

    static char* carla_lv2_state_map_abstract_path(LV2_State_Map_Path_Handle handle, const char* absolute_path)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, strdup(""));
        CARLA_SAFE_ASSERT_RETURN(absolute_path != nullptr && absolute_path[0] != '\0', strdup(""));
        carla_debug("carla_lv2_state_map_abstract_path(%p, \"%s\")", handle, absolute_path);

        // may already be an abstract path
        if (! File::isAbsolutePath(absolute_path))
            return strdup(absolute_path);

        return strdup(File(absolute_path).getRelativePathFrom(File::getCurrentWorkingDirectory()).toRawUTF8());
    }

    static char* carla_lv2_state_map_absolute_path(LV2_State_Map_Path_Handle handle, const char* abstract_path)
    {
        const char* const cwd(File::getCurrentWorkingDirectory().getFullPathName().toRawUTF8());
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, strdup(cwd));
        CARLA_SAFE_ASSERT_RETURN(abstract_path != nullptr && abstract_path[0] != '\0', strdup(cwd));
        carla_debug("carla_lv2_state_map_absolute_path(%p, \"%s\")", handle, abstract_path);

        // may already be an absolute path
        if (File::isAbsolutePath(abstract_path))
            return strdup(abstract_path);

        return strdup(File::getCurrentWorkingDirectory().getChildFile(abstract_path).getFullPathName().toRawUTF8());
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
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, CARLA_URI_MAP_ID_NULL);
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', CARLA_URI_MAP_ID_NULL);
        carla_debug("carla_lv2_urid_map(%p, \"%s\")", handle, uri);

        // Atom types
        if (std::strcmp(uri, LV2_ATOM__Blank) == 0)
            return CARLA_URI_MAP_ID_ATOM_BLANK;
        if (std::strcmp(uri, LV2_ATOM__Bool) == 0)
            return CARLA_URI_MAP_ID_ATOM_BOOL;
        if (std::strcmp(uri, LV2_ATOM__Chunk) == 0)
            return CARLA_URI_MAP_ID_ATOM_CHUNK;
        if (std::strcmp(uri, LV2_ATOM__Double) == 0)
            return CARLA_URI_MAP_ID_ATOM_DOUBLE;
        if (std::strcmp(uri, LV2_ATOM__Event) == 0)
            return CARLA_URI_MAP_ID_ATOM_EVENT;
        if (std::strcmp(uri, LV2_ATOM__Float) == 0)
            return CARLA_URI_MAP_ID_ATOM_FLOAT;
        if (std::strcmp(uri, LV2_ATOM__Int) == 0)
            return CARLA_URI_MAP_ID_ATOM_INT;
        if (std::strcmp(uri, LV2_ATOM__Literal) == 0)
            return CARLA_URI_MAP_ID_ATOM_LITERAL;
        if (std::strcmp(uri, LV2_ATOM__Long) == 0)
            return CARLA_URI_MAP_ID_ATOM_LONG;
        if (std::strcmp(uri, LV2_ATOM__Number) == 0)
            return CARLA_URI_MAP_ID_ATOM_NUMBER;
        if (std::strcmp(uri, LV2_ATOM__Object) == 0)
            return CARLA_URI_MAP_ID_ATOM_OBJECT;
        if (std::strcmp(uri, LV2_ATOM__Path) == 0)
            return CARLA_URI_MAP_ID_ATOM_PATH;
        if (std::strcmp(uri, LV2_ATOM__Property) == 0)
            return CARLA_URI_MAP_ID_ATOM_PROPERTY;
        if (std::strcmp(uri, LV2_ATOM__Resource) == 0)
            return CARLA_URI_MAP_ID_ATOM_RESOURCE;
        if (std::strcmp(uri, LV2_ATOM__Sequence) == 0)
            return CARLA_URI_MAP_ID_ATOM_SEQUENCE;
        if (std::strcmp(uri, LV2_ATOM__Sound) == 0)
            return CARLA_URI_MAP_ID_ATOM_SOUND;
        if (std::strcmp(uri, LV2_ATOM__String) == 0)
            return CARLA_URI_MAP_ID_ATOM_STRING;
        if (std::strcmp(uri, LV2_ATOM__Tuple) == 0)
            return CARLA_URI_MAP_ID_ATOM_TUPLE;
        if (std::strcmp(uri, LV2_ATOM__URI) == 0)
            return CARLA_URI_MAP_ID_ATOM_URI;
        if (std::strcmp(uri, LV2_ATOM__URID) == 0)
            return CARLA_URI_MAP_ID_ATOM_URID;
        if (std::strcmp(uri, LV2_ATOM__Vector) == 0)
            return CARLA_URI_MAP_ID_ATOM_VECTOR;
        if (std::strcmp(uri, LV2_ATOM__atomTransfer) == 0)
            return CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM;
        if (std::strcmp(uri, LV2_ATOM__eventTransfer) == 0)
            return CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT;

        // BufSize types
        if (std::strcmp(uri, LV2_BUF_SIZE__maxBlockLength) == 0)
            return CARLA_URI_MAP_ID_BUF_MAX_LENGTH;
        if (std::strcmp(uri, LV2_BUF_SIZE__minBlockLength) == 0)
            return CARLA_URI_MAP_ID_BUF_MIN_LENGTH;
        if (std::strcmp(uri, LV2_BUF_SIZE__nominalBlockLength) == 0)
            return CARLA_URI_MAP_ID_BUF_NOMINAL_LENGTH;
        if (std::strcmp(uri, LV2_BUF_SIZE__sequenceSize) == 0)
            return CARLA_URI_MAP_ID_BUF_SEQUENCE_SIZE;

        // Log types
        if (std::strcmp(uri, LV2_LOG__Error) == 0)
            return CARLA_URI_MAP_ID_LOG_ERROR;
        if (std::strcmp(uri, LV2_LOG__Note) == 0)
            return CARLA_URI_MAP_ID_LOG_NOTE;
        if (std::strcmp(uri, LV2_LOG__Trace) == 0)
            return CARLA_URI_MAP_ID_LOG_TRACE;
        if (std::strcmp(uri, LV2_LOG__Warning) == 0)
            return CARLA_URI_MAP_ID_LOG_WARNING;

        // Time types
        if (std::strcmp(uri, LV2_TIME__Position) == 0)
            return CARLA_URI_MAP_ID_TIME_POSITION;
        if (std::strcmp(uri, LV2_TIME__bar) == 0)
            return CARLA_URI_MAP_ID_TIME_BAR;
        if (std::strcmp(uri, LV2_TIME__barBeat) == 0)
            return CARLA_URI_MAP_ID_TIME_BAR_BEAT;
        if (std::strcmp(uri, LV2_TIME__beat) == 0)
            return CARLA_URI_MAP_ID_TIME_BEAT;
        if (std::strcmp(uri, LV2_TIME__beatUnit) == 0)
            return CARLA_URI_MAP_ID_TIME_BEAT_UNIT;
        if (std::strcmp(uri, LV2_TIME__beatsPerBar) == 0)
            return CARLA_URI_MAP_ID_TIME_BEATS_PER_BAR;
        if (std::strcmp(uri, LV2_TIME__beatsPerMinute) == 0)
            return CARLA_URI_MAP_ID_TIME_BEATS_PER_MINUTE;
        if (std::strcmp(uri, LV2_TIME__frame) == 0)
            return CARLA_URI_MAP_ID_TIME_FRAME;
        if (std::strcmp(uri, LV2_TIME__framesPerSecond) == 0)
            return CARLA_URI_MAP_ID_TIME_FRAMES_PER_SECOND;
        if (std::strcmp(uri, LV2_TIME__speed) == 0)
            return CARLA_URI_MAP_ID_TIME_SPEED;
        if (std::strcmp(uri, LV2_KXSTUDIO_PROPERTIES__TimePositionTicksPerBeat) == 0)
            return CARLA_URI_MAP_ID_TIME_TICKS_PER_BEAT;

        // Others
        if (std::strcmp(uri, LV2_MIDI__MidiEvent) == 0)
            return CARLA_URI_MAP_ID_MIDI_EVENT;
        if (std::strcmp(uri, LV2_PARAMETERS__sampleRate) == 0)
            return CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE;
        if (std::strcmp(uri, LV2_UI__windowTitle) == 0)
            return CARLA_URI_MAP_ID_UI_WINDOW_TITLE;

        // Custom
        if (std::strcmp(uri, LV2_KXSTUDIO_PROPERTIES__TransientWindowId) == 0)
            return CARLA_URI_MAP_ID_CARLA_TRANSIENT_WIN_ID;
        if (std::strcmp(uri, URI_CARLA_ATOM_WORKER) == 0)
            return CARLA_URI_MAP_ID_CARLA_ATOM_WORKER;

        // Custom types
        return ((CarlaPluginLV2*)handle)->getCustomURID(uri);
    }

    static const char* carla_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(urid != CARLA_URI_MAP_ID_NULL, nullptr);
        carla_debug("carla_lv2_urid_unmap(%p, %i)", handle, urid);

        // Atom types
        if (urid == CARLA_URI_MAP_ID_ATOM_BLANK)
            return LV2_ATOM__Blank;
        if (urid == CARLA_URI_MAP_ID_ATOM_BOOL)
            return LV2_ATOM__Bool;
        if (urid == CARLA_URI_MAP_ID_ATOM_CHUNK)
            return LV2_ATOM__Chunk;
        if (urid == CARLA_URI_MAP_ID_ATOM_DOUBLE)
            return LV2_ATOM__Double;
        if (urid == CARLA_URI_MAP_ID_ATOM_EVENT)
            return LV2_ATOM__Event;
        if (urid == CARLA_URI_MAP_ID_ATOM_FLOAT)
            return LV2_ATOM__Float;
        if (urid == CARLA_URI_MAP_ID_ATOM_INT)
            return LV2_ATOM__Int;
        if (urid == CARLA_URI_MAP_ID_ATOM_LITERAL)
            return LV2_ATOM__Literal;
        if (urid == CARLA_URI_MAP_ID_ATOM_LONG)
            return LV2_ATOM__Long;
        if (urid == CARLA_URI_MAP_ID_ATOM_NUMBER)
            return LV2_ATOM__Number;
        if (urid == CARLA_URI_MAP_ID_ATOM_OBJECT)
            return LV2_ATOM__Object;
        if (urid == CARLA_URI_MAP_ID_ATOM_PATH)
            return LV2_ATOM__Path;
        if (urid == CARLA_URI_MAP_ID_ATOM_PROPERTY)
            return LV2_ATOM__Property;
        if (urid == CARLA_URI_MAP_ID_ATOM_RESOURCE)
            return LV2_ATOM__Resource;
        if (urid == CARLA_URI_MAP_ID_ATOM_SEQUENCE)
            return LV2_ATOM__Sequence;
        if (urid == CARLA_URI_MAP_ID_ATOM_SOUND)
            return LV2_ATOM__Sound;
        if (urid == CARLA_URI_MAP_ID_ATOM_STRING)
            return LV2_ATOM__String;
        if (urid == CARLA_URI_MAP_ID_ATOM_TUPLE)
            return LV2_ATOM__Tuple;
        if (urid == CARLA_URI_MAP_ID_ATOM_URI)
            return LV2_ATOM__URI;
        if (urid == CARLA_URI_MAP_ID_ATOM_URID)
            return LV2_ATOM__URID;
        if (urid == CARLA_URI_MAP_ID_ATOM_VECTOR)
            return LV2_ATOM__Vector;
        if (urid == CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM)
            return LV2_ATOM__atomTransfer;
        if (urid == CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT)
            return LV2_ATOM__eventTransfer;

        // BufSize types
        if (urid == CARLA_URI_MAP_ID_BUF_MAX_LENGTH)
            return LV2_BUF_SIZE__maxBlockLength;
        if (urid == CARLA_URI_MAP_ID_BUF_MIN_LENGTH)
            return LV2_BUF_SIZE__minBlockLength;
        if (urid == CARLA_URI_MAP_ID_BUF_NOMINAL_LENGTH)
            return LV2_BUF_SIZE__nominalBlockLength;
        if (urid == CARLA_URI_MAP_ID_BUF_SEQUENCE_SIZE)
            return LV2_BUF_SIZE__sequenceSize;

        // Log types
        if (urid == CARLA_URI_MAP_ID_LOG_ERROR)
            return LV2_LOG__Error;
        if (urid == CARLA_URI_MAP_ID_LOG_NOTE)
            return LV2_LOG__Note;
        if (urid == CARLA_URI_MAP_ID_LOG_TRACE)
            return LV2_LOG__Trace;
        if (urid == CARLA_URI_MAP_ID_LOG_WARNING)
            return LV2_LOG__Warning;

        // Time types
        if (urid == CARLA_URI_MAP_ID_TIME_POSITION)
            return LV2_TIME__Position;
        if (urid == CARLA_URI_MAP_ID_TIME_BAR)
            return LV2_TIME__bar;
        if (urid == CARLA_URI_MAP_ID_TIME_BAR_BEAT)
            return LV2_TIME__barBeat;
        if (urid == CARLA_URI_MAP_ID_TIME_BEAT)
            return LV2_TIME__beat;
        if (urid == CARLA_URI_MAP_ID_TIME_BEAT_UNIT)
            return LV2_TIME__beatUnit;
        if (urid == CARLA_URI_MAP_ID_TIME_BEATS_PER_BAR)
            return LV2_TIME__beatsPerBar;
        if (urid == CARLA_URI_MAP_ID_TIME_BEATS_PER_MINUTE)
            return LV2_TIME__beatsPerMinute;
        if (urid == CARLA_URI_MAP_ID_TIME_FRAME)
            return LV2_TIME__frame;
        if (urid == CARLA_URI_MAP_ID_TIME_FRAMES_PER_SECOND)
            return LV2_TIME__framesPerSecond;
        if (urid == CARLA_URI_MAP_ID_TIME_SPEED)
            return LV2_TIME__speed;
        if (urid == CARLA_URI_MAP_ID_TIME_TICKS_PER_BEAT)
            return LV2_KXSTUDIO_PROPERTIES__TimePositionTicksPerBeat;

        // Others
        if (urid == CARLA_URI_MAP_ID_MIDI_EVENT)
            return LV2_MIDI__MidiEvent;
        if (urid == CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE)
            return LV2_PARAMETERS__sampleRate;
        if (urid == CARLA_URI_MAP_ID_UI_WINDOW_TITLE)
            return LV2_UI__windowTitle;

        // Custom
        if (urid == CARLA_URI_MAP_ID_CARLA_ATOM_WORKER)
            return URI_CARLA_ATOM_WORKER;
        if (urid == CARLA_URI_MAP_ID_CARLA_TRANSIENT_WIN_ID)
            return LV2_KXSTUDIO_PROPERTIES__TransientWindowId;

        // Custom types
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

    // -------------------------------------------------------------------
    // UI Resize Feature

    static int carla_lv2_ui_resize(LV2UI_Feature_Handle handle, int width, int height)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, 1);
        carla_debug("carla_lv2_ui_resize(%p, %i, %i)", handle, width, height);

        return ((CarlaPluginLV2*)handle)->handleUIResize(width, height);
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
            kPlugin->handleUIWrite(index, sizeof(float), CARLA_URI_MAP_ID_NULL, &value);
        } CARLA_SAFE_EXCEPTION("magReceived control");

        return true;
    }

    if (std::strcmp(msg, "atom") == 0)
    {
        uint32_t index, size;
        const char* base64atom;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(index), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(size), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(base64atom), true);

        std::vector<uint8_t> chunk(carla_getChunkFromBase64String(base64atom));
        delete[] base64atom;
        CARLA_SAFE_ASSERT_RETURN(chunk.size() >= sizeof(LV2_Atom), true);

        const LV2_Atom* const atom((const LV2_Atom*)chunk.data());
        CARLA_SAFE_ASSERT_RETURN(lv2_atom_total_size(atom) == chunk.size(), true);

        try {
            kPlugin->handleUIWrite(index, lv2_atom_total_size(atom), CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);
        } CARLA_SAFE_EXCEPTION("magReceived atom");

        return true;
    }

    if (std::strcmp(msg, "program") == 0)
    {
        uint32_t index;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(index), true);

        try {
            kPlugin->setMidiProgram(static_cast<int32_t>(index), false, true, true);
        } CARLA_SAFE_EXCEPTION("msgReceived program");

        return true;
    }

    if (std::strcmp(msg, "urid") == 0)
    {
        uint32_t urid;
        const char* uri;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(urid), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(uri), true);

        if (urid != 0)
        {
            try {
                kPlugin->handleUridMap(urid, uri);
            } CARLA_SAFE_EXCEPTION("msgReceived urid");
        }

        delete[] uri;
        return true;
    }

    return false;
}

// -------------------------------------------------------------------------------------------------------------------

CarlaPlugin* CarlaPlugin::newLV2(const Initializer& init)
{
    carla_debug("CarlaPlugin::newLV2({%p, \"%s\", \"%s\", " P_INT64 "})", init.engine, init.name, init.label, init.uniqueId);

    CarlaPluginLV2* const plugin(new CarlaPluginLV2(init.engine, init.id));

    if (! plugin->init(init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    return plugin;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
