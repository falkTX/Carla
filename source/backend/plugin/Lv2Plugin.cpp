/*
 * Carla LV2 Plugin
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

#include "CarlaPluginInternal.hpp"

#ifdef WANT_LV2

#include "CarlaLv2Utils.hpp"
#include "CarlaLibUtils.hpp"
#include "Lv2AtomQueue.hpp"

#include "../engine/CarlaEngineOsc.hpp"

extern "C" {
#include "rtmempool/rtmempool-lv2.h"
}

// -----------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
}
#endif

const unsigned int MAX_EVENT_BUFFER = 8192; // 0x2000

// Extra Plugin Hints
const unsigned int PLUGIN_HAS_EXTENSION_OPTIONS  = 0x1000;
const unsigned int PLUGIN_HAS_EXTENSION_PROGRAMS = 0x2000;
const unsigned int PLUGIN_HAS_EXTENSION_STATE    = 0x4000;
const unsigned int PLUGIN_HAS_EXTENSION_WORKER   = 0x8000;

// Extra Parameter Hints
const unsigned int PARAMETER_IS_STRICT_BOUNDS = 0x1000;
const unsigned int PARAMETER_IS_TRIGGER       = 0x2000;

// LV2 Event Data/Types
const unsigned int CARLA_EVENT_DATA_ATOM    = 0x01;
const unsigned int CARLA_EVENT_DATA_EVENT   = 0x02;
const unsigned int CARLA_EVENT_DATA_MIDI_LL = 0x04;
const unsigned int CARLA_EVENT_TYPE_MESSAGE = 0x10; // unused
const unsigned int CARLA_EVENT_TYPE_MIDI    = 0x20;
const unsigned int CARLA_EVENT_TYPE_TIME    = 0x40;

// LV2 URI Map Ids
const uint32_t CARLA_URI_MAP_ID_NULL                   =  0;
const uint32_t CARLA_URI_MAP_ID_ATOM_BLANK             =  1;
const uint32_t CARLA_URI_MAP_ID_ATOM_BOOL              =  2;
const uint32_t CARLA_URI_MAP_ID_ATOM_CHUNK             =  3;
const uint32_t CARLA_URI_MAP_ID_ATOM_DOUBLE            =  4;
const uint32_t CARLA_URI_MAP_ID_ATOM_FLOAT             =  5;
const uint32_t CARLA_URI_MAP_ID_ATOM_INT               =  6;
const uint32_t CARLA_URI_MAP_ID_ATOM_LITERAL           =  7;
const uint32_t CARLA_URI_MAP_ID_ATOM_LONG              =  8;
const uint32_t CARLA_URI_MAP_ID_ATOM_PATH              =  9;
const uint32_t CARLA_URI_MAP_ID_ATOM_PROPERTY          = 10;
const uint32_t CARLA_URI_MAP_ID_ATOM_RESOURCE          = 11;
const uint32_t CARLA_URI_MAP_ID_ATOM_SEQUENCE          = 12;
const uint32_t CARLA_URI_MAP_ID_ATOM_STRING            = 13;
const uint32_t CARLA_URI_MAP_ID_ATOM_TUPLE             = 14;
const uint32_t CARLA_URI_MAP_ID_ATOM_URI               = 15;
const uint32_t CARLA_URI_MAP_ID_ATOM_URID              = 16;
const uint32_t CARLA_URI_MAP_ID_ATOM_VECTOR            = 17;
const uint32_t CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM     = 18;
const uint32_t CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT    = 19;
const uint32_t CARLA_URI_MAP_ID_BUF_MAX_LENGTH         = 20;
const uint32_t CARLA_URI_MAP_ID_BUF_MIN_LENGTH         = 21;
const uint32_t CARLA_URI_MAP_ID_BUF_SEQUENCE_SIZE      = 22;
const uint32_t CARLA_URI_MAP_ID_LOG_ERROR              = 23;
const uint32_t CARLA_URI_MAP_ID_LOG_NOTE               = 24;
const uint32_t CARLA_URI_MAP_ID_LOG_TRACE              = 25;
const uint32_t CARLA_URI_MAP_ID_LOG_WARNING            = 26;
const uint32_t CARLA_URI_MAP_ID_TIME_POSITION          = 27; // base type
const uint32_t CARLA_URI_MAP_ID_TIME_BAR               = 28; // values
const uint32_t CARLA_URI_MAP_ID_TIME_BAR_BEAT          = 29;
const uint32_t CARLA_URI_MAP_ID_TIME_BEAT              = 30;
const uint32_t CARLA_URI_MAP_ID_TIME_BEAT_UNIT         = 31;
const uint32_t CARLA_URI_MAP_ID_TIME_BEATS_PER_BAR     = 32;
const uint32_t CARLA_URI_MAP_ID_TIME_BEATS_PER_MINUTE  = 33;
const uint32_t CARLA_URI_MAP_ID_TIME_FRAME             = 34;
const uint32_t CARLA_URI_MAP_ID_TIME_FRAMES_PER_SECOND = 35;
const uint32_t CARLA_URI_MAP_ID_TIME_SPEED             = 36;
const uint32_t CARLA_URI_MAP_ID_MIDI_EVENT             = 37;
const uint32_t CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE      = 38;
const uint32_t CARLA_URI_MAP_ID_COUNT                  = 39;

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
const uint32_t kFeatureIdRtMemPool        = 10;
const uint32_t kFeatureIdStateMakePath    = 11;
const uint32_t kFeatureIdStateMapPath     = 12;
const uint32_t kFeatureIdStrictBounds     = 13;
const uint32_t kFeatureIdUriMap           = 14;
const uint32_t kFeatureIdUridMap          = 15;
const uint32_t kFeatureIdUridUnmap        = 16;
const uint32_t kFeatureIdWorker           = 17;
const uint32_t kFeatureIdUiDataAccess     = 18;
const uint32_t kFeatureIdUiInstanceAccess = 19;
const uint32_t kFeatureIdUiIdle           = 20;
const uint32_t kFeatureIdUiFixedSize      = 21;
const uint32_t kFeatureIdUiMakeResident   = 22;
const uint32_t kFeatureIdUiNoUserResize   = 23;
const uint32_t kFeatureIdUiParent         = 24;
const uint32_t kFeatureIdUiPortMap        = 25;
const uint32_t kFeatureIdUiPortSubscribe  = 26;
const uint32_t kFeatureIdUiResize         = 27;
const uint32_t kFeatureIdUiTouch          = 28;
const uint32_t kFeatureIdExternalUi       = 29;
const uint32_t kFeatureIdExternalUiOld    = 30;
const uint32_t kFeatureCount              = 31;

enum Lv2PluginGuiType {
    PLUGIN_UI_NULL,
    PLUGIN_UI_OSC,
    PLUGIN_UI_QT,
    PLUGIN_UI_PARENT,
    PLUGIN_UI_EXTERNAL
};

struct Lv2EventData {
    uint32_t type;
    uint32_t rindex;
    CarlaEngineEventPort* port;
    union {
        LV2_Atom_Buffer* atom;
        LV2_Event_Buffer* event;
        LV2_MIDI* midi;
        void* _ptr; // value checking
    };

    Lv2EventData()
        : type(0x0),
          rindex(0),
          port(nullptr),
          _ptr(nullptr) {}

    ~Lv2EventData()
    {
        if (port != nullptr)
        {
            delete port;
            port = nullptr;
        }

        if (type & CARLA_EVENT_DATA_ATOM)
        {
            CARLA_ASSERT(atom != nullptr);

            if (atom != nullptr)
            {
                std::free(atom);
                atom = nullptr;
            }
        }
        else if (type & CARLA_EVENT_DATA_EVENT)
        {
            CARLA_ASSERT(event != nullptr);

            if (event != nullptr)
            {
                std::free(event);
                event = nullptr;
            }
        }
        else if (type & CARLA_EVENT_DATA_MIDI_LL)
        {
            CARLA_ASSERT(midi != nullptr && midi->data != nullptr);

            if (midi != nullptr)
            {
                if (midi->data != nullptr)
                {
                    delete[] midi->data;
                    midi->data = nullptr;
                }

                delete midi;
                midi = nullptr;
            }
        }

        CARLA_ASSERT(_ptr == nullptr);

        type = 0x0;
    }

    CARLA_DECLARE_NON_COPY_STRUCT(Lv2EventData)
};

struct Lv2PluginEventData {
    uint32_t count;
    Lv2EventData* data;
    Lv2EventData* ctrl; // default port, either this->data[x] or pData->portIn/Out
    uint32_t ctrlIndex;

    Lv2PluginEventData()
        : count(0),
          data(nullptr),
          ctrl(nullptr),
          ctrlIndex(0) {}

    ~Lv2PluginEventData()
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT(data == nullptr);
        CARLA_ASSERT(ctrl == nullptr);
        CARLA_ASSERT_INT(ctrlIndex == 0, ctrlIndex);
    }

    void createNew(const uint32_t newCount)
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT(data == nullptr);
        CARLA_ASSERT(ctrl == nullptr);
        CARLA_ASSERT_INT(ctrlIndex == 0, ctrlIndex);
        CARLA_ASSERT_INT(newCount > 0, newCount);

        if (data != nullptr || newCount == 0)
            return;

        data  = new Lv2EventData[newCount];
        count = newCount;
    }

    void clear()
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

    void initBuffers()
    {
        for (uint32_t i=0; i < count; ++i)
        {
            if (data[i].port != nullptr && (ctrl == nullptr || data[i].port != ctrl->port))
                data[i].port->initBuffer();
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(Lv2PluginEventData)
};

struct Lv2PluginOptions {
    int maxBufferSize;
    int minBufferSize;
    int sequenceSize;
    double sampleRate;
    LV2_Options_Option optMaxBlockLenth;
    LV2_Options_Option optMinBlockLenth;
    LV2_Options_Option optSequenceSize;
    LV2_Options_Option optSampleRate;
    LV2_Options_Option optNull;
    LV2_Options_Option* opts[5];

    Lv2PluginOptions()
        : maxBufferSize(0),
          minBufferSize(0),
          sequenceSize(MAX_EVENT_BUFFER),
          sampleRate(0.0)
    {
        optMaxBlockLenth.context = LV2_OPTIONS_INSTANCE;
        optMaxBlockLenth.subject = 0;
        optMaxBlockLenth.key     = CARLA_URI_MAP_ID_BUF_MAX_LENGTH;
        optMaxBlockLenth.size    = sizeof(int);
        optMaxBlockLenth.type    = CARLA_URI_MAP_ID_ATOM_INT;
        optMaxBlockLenth.value   = &maxBufferSize;

        optMinBlockLenth.context = LV2_OPTIONS_INSTANCE;
        optMinBlockLenth.subject = 0;
        optMinBlockLenth.key     = CARLA_URI_MAP_ID_BUF_MIN_LENGTH;
        optMinBlockLenth.size    = sizeof(int);
        optMinBlockLenth.type    = CARLA_URI_MAP_ID_ATOM_INT;
        optMinBlockLenth.value   = &minBufferSize;

        optSequenceSize.context = LV2_OPTIONS_INSTANCE;
        optSequenceSize.subject = 0;
        optSequenceSize.key     = CARLA_URI_MAP_ID_BUF_SEQUENCE_SIZE;
        optSequenceSize.size    = sizeof(int);
        optSequenceSize.type    = CARLA_URI_MAP_ID_ATOM_INT;
        optSequenceSize.value   = &sequenceSize;

        optSampleRate.context = LV2_OPTIONS_INSTANCE;
        optSampleRate.subject = 0;
        optSampleRate.key     = CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE;
        optSampleRate.size    = sizeof(double);
        optSampleRate.type    = CARLA_URI_MAP_ID_ATOM_DOUBLE;
        optSampleRate.value   = &sampleRate;

        optNull.context = LV2_OPTIONS_INSTANCE;
        optNull.subject = 0;
        optNull.key     = CARLA_URI_MAP_ID_NULL;
        optNull.size    = 0;
        optNull.type    = CARLA_URI_MAP_ID_NULL;
        optNull.value   = nullptr;

        opts[0] = &optMaxBlockLenth;
        opts[1] = &optMinBlockLenth;
        opts[2] = &optSequenceSize;
        opts[3] = &optSampleRate;
        opts[4] = &optNull;
    }

    CARLA_DECLARE_NON_COPY_STRUCT(Lv2PluginOptions)
};

// -----------------------------------------------------

class Lv2Plugin : public CarlaPlugin/*,
                  public CarlaPluginGui::Callback*/
{
public:
    Lv2Plugin(CarlaEngine* const engine, const unsigned int id)
        : CarlaPlugin(engine, id),
          fHandle(nullptr),
          fHandle2(nullptr),
#ifdef CARLA_PROPER_CPP11_SUPPORT
          fFeatures{nullptr},
#endif
          fDescriptor(nullptr),
          fRdfDescriptor(nullptr),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fParamBuffers(nullptr)
    {
        carla_debug("Lv2Plugin::Lv2Plugin(%p, %i)", engine, id);

#ifndef CARLA_PROPER_CPP11_SUPPORT
        carla_fill<LV2_Feature*>(fFeatures, kFeatureCount+1, nullptr);
#endif

        pData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_LV2_GUI);

        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; ++i)
            fCustomURIDs.append(nullptr);

        fAtomForge.Blank    = CARLA_URI_MAP_ID_ATOM_BLANK;
        fAtomForge.Bool     = CARLA_URI_MAP_ID_ATOM_BOOL;
        fAtomForge.Chunk    = CARLA_URI_MAP_ID_ATOM_CHUNK;
        fAtomForge.Double   = CARLA_URI_MAP_ID_ATOM_DOUBLE;
        fAtomForge.Float    = CARLA_URI_MAP_ID_ATOM_FLOAT;
        fAtomForge.Int      = CARLA_URI_MAP_ID_ATOM_INT;
        fAtomForge.Literal  = CARLA_URI_MAP_ID_ATOM_LITERAL;
        fAtomForge.Long     = CARLA_URI_MAP_ID_ATOM_LONG;
        fAtomForge.Path     = CARLA_URI_MAP_ID_ATOM_PATH;
        fAtomForge.Property = CARLA_URI_MAP_ID_ATOM_PROPERTY;
        fAtomForge.Resource = CARLA_URI_MAP_ID_ATOM_RESOURCE;
        fAtomForge.Sequence = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
        fAtomForge.String   = CARLA_URI_MAP_ID_ATOM_STRING;
        fAtomForge.Tuple    = CARLA_URI_MAP_ID_ATOM_TUPLE;
        fAtomForge.URI      = CARLA_URI_MAP_ID_ATOM_URI;
        fAtomForge.URID     = CARLA_URI_MAP_ID_ATOM_URID;
        fAtomForge.Vector   = CARLA_URI_MAP_ID_ATOM_VECTOR;
    }

    ~Lv2Plugin() override
    {
        carla_debug("Lv2Plugin::~Lv2Plugin()");

        // close UI
        if (fUi.type != PLUGIN_UI_NULL)
        {
            showCustomUI(false);

            if (fUi.type == PLUGIN_UI_OSC)
            {
                pData->osc.thread.stop(pData->engine->getOptions().uiBridgesTimeout);
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
                {
                    const LV2_External_UI_Host* const uiHost((const LV2_External_UI_Host*)fFeatures[kFeatureIdExternalUi]->data);

                    if (uiHost->plugin_human_id != nullptr)
                        delete[] uiHost->plugin_human_id;

                    delete uiHost;
                }

                fUi.descriptor = nullptr;
                pData->uiLibClose();
            }

            fUi.rdfDescriptor = nullptr;
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

        if (fFeatures[kFeatureIdRtMemPool] != nullptr && fFeatures[kFeatureIdRtMemPool]->data != nullptr)
            delete (LV2_RtMemPool_Pool*)fFeatures[kFeatureIdRtMemPool]->data;

        if (fFeatures[kFeatureIdUriMap] != nullptr && fFeatures[kFeatureIdUriMap]->data != nullptr)
            delete (LV2_URI_Map_Feature*)fFeatures[kFeatureIdUriMap]->data;

        if (fFeatures[kFeatureIdUridMap] != nullptr && fFeatures[kFeatureIdUridMap]->data != nullptr)
            delete (LV2_URID_Map*)fFeatures[kFeatureIdUridMap]->data;

        if (fFeatures[kFeatureIdUridUnmap] != nullptr && fFeatures[kFeatureIdUridUnmap]->data != nullptr)
            delete (LV2_URID_Unmap*)fFeatures[kFeatureIdUridUnmap]->data;

        if (fFeatures[kFeatureIdWorker] != nullptr && fFeatures[kFeatureIdWorker]->data != nullptr)
            delete (LV2_Worker_Schedule*)fFeatures[kFeatureIdWorker]->data;

        for (uint32_t i=0; i < kFeatureCount; ++i)
        {
            if (fFeatures[i] != nullptr)
            {
                delete fFeatures[i];
                fFeatures[i] = nullptr;
            }
        }

        for (List<const char*>::Itenerator it = fCustomURIDs.begin(); it.valid(); it.next())
        {
            const char*& uri(*it);

            if (uri != nullptr)
            {
                delete[] uri;
                uri = nullptr;
            }
        }

        fCustomURIDs.clear();

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_LV2;
    }

    PluginCategory getCategory() const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

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

        return getPluginCategoryFromName(pData->name);
    }

    long getUniqueId() const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        return fRdfDescriptor->UniqueID;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getMidiInCount() const noexcept override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        uint32_t count = 0;

        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (LV2_IS_PORT_INPUT(portTypes) && LV2_PORT_SUPPORTS_MIDI_EVENT(portTypes))
                count += 1;
        }

        return count;
    }

    uint32_t getMidiOutCount() const noexcept override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        uint32_t count = 0;

        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (LV2_IS_PORT_OUTPUT(portTypes) && LV2_PORT_SUPPORTS_MIDI_EVENT(portTypes))
                count += 1;
        }

        return count;
    }

    uint32_t getParameterScalePointCount(const uint32_t parameterId) const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < pData->param.count);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port& port(fRdfDescriptor->Ports[rindex]);
            return port.ScalePointCount;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int getOptionsAvailable() const override
    {
        const uint32_t hasMidiIn(getMidiInCount() > 0);

        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (! (hasMidiIn || needsFixedBuffer()))
            options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (pData->engine->getProccessMode() != ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        {
            if (pData->options & PLUGIN_OPTION_FORCE_STEREO)
                options |= PLUGIN_OPTION_FORCE_STEREO;
            else if (pData->audioIn.count <= 1 && pData->audioOut.count <= 1 && (pData->audioIn.count != 0 || pData->audioOut.count != 0))
                options |= PLUGIN_OPTION_FORCE_STEREO;
        }

        if (hasMidiIn)
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const override
    {
        CARLA_ASSERT(fParamBuffers != nullptr);
        CARLA_ASSERT(parameterId < pData->param.count);

        if (pData->param.data[parameterId].hints & PARAMETER_IS_STRICT_BOUNDS)
            pData->param.ranges[parameterId].fixValue(fParamBuffers[parameterId]);

        return fParamBuffers[parameterId];
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < pData->param.count);
        CARLA_ASSERT(scalePointId < getParameterScalePointCount(parameterId));

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port& port(fRdfDescriptor->Ports[rindex]);

            if (scalePointId < port.ScalePointCount)
            {
                const LV2_RDF_PortScalePoint& portScalePoint(port.ScalePoints[scalePointId]);
                return portScalePoint.Value;
            }
        }

        return 0.0f;
    }

    void getLabel(char* const strBuf) const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(fRdfDescriptor->URI != nullptr);

        if (fRdfDescriptor->URI != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->URI, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf) const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        if (fRdfDescriptor->Author != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Author, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf) const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        if (fRdfDescriptor->License != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->License, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf) const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        if (fRdfDescriptor->Name != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < pData->param.count);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
            std::strncpy(strBuf, fRdfDescriptor->Ports[rindex].Name, STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterSymbol(const uint32_t parameterId, char* const strBuf) const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < pData->param.count);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
            std::strncpy(strBuf, fRdfDescriptor->Ports[rindex].Symbol, STR_MAX);
        else
            CarlaPlugin::getParameterSymbol(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < pData->param.count);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port& port(fRdfDescriptor->Ports[rindex]);

            if (LV2_HAVE_PORT_UNIT_SYMBOL(port.Unit.Hints) && port.Unit.Symbol != nullptr)
            {
                std::strncpy(strBuf, port.Unit.Symbol, STR_MAX);
                return;
            }
            else if (LV2_HAVE_PORT_UNIT_UNIT(port.Unit.Hints))
            {
                switch (port.Unit.Unit)
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

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < pData->param.count);
        CARLA_ASSERT(scalePointId < getParameterScalePointCount(parameterId));

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port& port(fRdfDescriptor->Ports[rindex]);

            if (scalePointId < port.ScalePointCount)
            {
                const LV2_RDF_PortScalePoint& portScalePoint(port.ScalePoints[scalePointId]);

                if (portScalePoint.Label != nullptr)
                {
                    std::strncpy(strBuf, portScalePoint.Label, STR_MAX);
                    return;
                }
            }
        }

        CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    void prepareForSave() override
    {
        CARLA_ASSERT(fHandle != nullptr);

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

        //QString guiTitle(QString("%1 (GUI)").arg((const char*)pData->name));

        //if (pData->gui != nullptr)
            //pData->gui->setWindowTitle(guiTitle);

        if (fFeatures[kFeatureIdExternalUi] != nullptr && fFeatures[kFeatureIdExternalUi]->data != nullptr)
        {
            LV2_External_UI_Host* const uiHost((LV2_External_UI_Host*)fFeatures[kFeatureIdExternalUi]->data);

            if (uiHost->plugin_human_id != nullptr)
                delete[] uiHost->plugin_human_id;

            //uiHost->plugin_human_id = carla_strdup(guiTitle.toUtf8().constData());
        }
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(parameterId < pData->param.count);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fParamBuffers[parameterId] = fixedValue;

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(type != nullptr);
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);
        carla_debug("Lv2Plugin::setCustomData(%s, %s, %s, %s)", type, key, value, bool2str(sendGui));

        if (type == nullptr)
            return carla_stderr2("Lv2Plugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is invalid", type, key, value, bool2str(sendGui));

        if (key == nullptr)
            return carla_stderr2("Lv2Plugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

        if (value == nullptr)
            return carla_stderr2("Lv2Plugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

        CarlaPlugin::setCustomData(type, key, value, sendGui);

        // FIXME - we should only call this once, after all data is stored

        if (fExt.state != nullptr)
        {
            LV2_State_Status status;

            {
                const ScopedSingleProcessLocker spl(this, true);

                status = fExt.state->restore(fHandle, carla_lv2_state_retrieve, this, 0, fFeatures);

                if (fHandle2 != nullptr)
                    fExt.state->restore(fHandle, carla_lv2_state_retrieve, this, 0, fFeatures);
            }

            switch (status)
            {
            case LV2_STATE_SUCCESS:
                carla_debug("Lv2Plugin::setCustomData(\"%s\", \"%s\", <value>, %s) - success", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_UNKNOWN:
                carla_stderr("Lv2Plugin::setCustomData(\"%s\", \"%s\", <value>, %s) - unknown error", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_BAD_TYPE:
                carla_stderr("Lv2Plugin::setCustomData(\"%s\", \"%s\", <value>, %s) - error, bad type", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_BAD_FLAGS:
                carla_stderr("Lv2Plugin::setCustomData(\"%s\", \"%s\", <value>, %s) - error, bad flags", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_NO_FEATURE:
                carla_stderr("Lv2Plugin::setCustomData(\"%s\", \"%s\", <value>, %s) - error, missing feature", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_NO_PROPERTY:
                carla_stderr("Lv2Plugin::setCustomData(\"%s\", \"%s\", <value>, %s) - error, missing property", type, key, bool2str(sendGui));
                break;
            }
        }

        if (sendGui)
        {
            //CustomData cdata;
            //cdata.type  = type;
            //cdata.key   = key;
            //cdata.value = value;
            //uiTransferCustomData(&cdata);
        }
    }

    void setProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(fRdfDescriptor->PresetCount));
        CARLA_ASSERT(sendGui || sendOsc || sendCallback); // never call this from RT

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(fRdfDescriptor->PresetCount))
            return;

        if (index >= 0 && index < static_cast<int32_t>(fRdfDescriptor->PresetCount))
        {
            Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());

            if (const LilvState* state = lv2World.getState(fRdfDescriptor->Presets[index].URI, (const LV2_URID_Map*)fFeatures[kFeatureIdUridMap]->data))
            {
                const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

                lilv_state_restore(state, fExt.state, fHandle, carla_lilv_set_port_value, this, 0, fFeatures);

                if (fHandle2 != nullptr)
                    lilv_state_restore(state, fExt.state, fHandle2, carla_lilv_set_port_value, this, 0, fFeatures);
            }
        }

        CarlaPlugin::setProgram(index, sendGui, sendOsc, sendCallback);
    }

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count));

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(pData->midiprog.count))
            return;

        if (index >= 0 && fExt.programs != nullptr && fExt.programs->select_program != nullptr)
        {
            const uint32_t bank(pData->midiprog.data[index].bank);
            const uint32_t program(pData->midiprog.data[index].program);

            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            fExt.programs->select_program(fHandle, bank, program);

            if (fHandle2 != nullptr)
                fExt.programs->select_program(fHandle2, bank, program);
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void showCustomUI(const bool yesNo) override
    {
        if (fUi.type == PLUGIN_UI_NULL)
            return;

        if (fUi.type == PLUGIN_UI_OSC)
        {
            if (yesNo)
            {
                pData->osc.thread.start();
            }
            else
            {
                if (pData->osc.data.target != nullptr)
                {
                    osc_send_hide(pData->osc.data);
                    osc_send_quit(pData->osc.data);
                    pData->osc.data.free();
                }

                pData->osc.thread.stop(pData->engine->getOptions().uiBridgesTimeout);
            }

            return;
        }

        // take some precautions
        CARLA_ASSERT(fUi.descriptor != nullptr);
        CARLA_ASSERT(fUi.rdfDescriptor != nullptr);

        if (fUi.descriptor == nullptr)
            return;
        if (fUi.rdfDescriptor == nullptr)
            return;

        if (yesNo)
        {
            CARLA_ASSERT(fUi.descriptor->instantiate != nullptr);

            if (fUi.descriptor->instantiate == nullptr)
                return;
        }
        else
        {
            CARLA_ASSERT(fUi.descriptor->cleanup != nullptr);

            if (fUi.handle == nullptr)
                return;
            if (fUi.descriptor->cleanup == nullptr)
                return;
        }

        if (fUi.type == PLUGIN_UI_EXTERNAL)
        {
            if (yesNo)
            {
                if (fUi.handle == nullptr)
                {
                    fUi.widget = nullptr;
                    fUi.handle = fUi.descriptor->instantiate(fUi.descriptor, fRdfDescriptor->URI, fUi.rdfDescriptor->Bundle,
                                                             carla_lv2_ui_write_function, this, &fUi.widget, fFeatures);
                }

                CARLA_ASSERT(fUi.handle != nullptr);
                CARLA_ASSERT(fUi.widget != nullptr);

                if (fUi.handle == nullptr || fUi.widget == nullptr)
                {
                    fUi.handle = nullptr;
                    fUi.widget = nullptr;
                    pData->engine->callback(ENGINE_CALLBACK_ERROR, pData->id, 0, 0, 0.0f, "Plugin refused to open its own UI");
                    pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
                    return;
                }

                updateUi();

                LV2_EXTERNAL_UI_SHOW((LV2_External_UI_Widget*)fUi.widget);
            }
            else
            {
                CARLA_ASSERT(fUi.widget != nullptr);

                if (fUi.widget != nullptr)
                    LV2_EXTERNAL_UI_HIDE((LV2_External_UI_Widget*)fUi.widget);

                fUi.descriptor->cleanup(fUi.handle);
                fUi.handle = nullptr;
                fUi.widget = nullptr;
            }
        }
        else // means PLUGIN_UI_PARENT || PLUGIN_UI_QT
        {
#if 0
            if (yesNo)
            {
                if (pData->gui == nullptr)
                {
                    // TODO
                    CarlaPluginGui::Options guiOptions;
                    guiOptions.parented  = (fUi.type == PLUGIN_UI_PARENT);
                    guiOptions.resizable = isUiResizable();

                    pData->gui = new CarlaPluginGui(pData->engine, this, guiOptions, pData->guiGeometry);
                }

                if (fUi.type == PLUGIN_UI_PARENT)
                {
                    fFeatures[kFeatureIdUiParent]->data = pData->gui->getContainerWinId();
                    fFeatures[kFeatureIdUiParent]->URI  = LV2_UI__parent;
                }

                if (fUi.handle == nullptr)
                {
                    fUi.widget = nullptr;
                    fUi.handle = fUi.descriptor->instantiate(fUi.descriptor, fRdfDescriptor->URI, fUi.rdfDescriptor->Bundle,
                                                             carla_lv2_ui_write_function, this, &fUi.widget, fFeatures);
                }

                CARLA_ASSERT(fUi.handle != nullptr);
                CARLA_ASSERT(fUi.widget != nullptr);

                if (fUi.handle == nullptr || fUi.widget == nullptr)
                {
                    fUi.handle = nullptr;
                    fUi.widget = nullptr;

                    pData->guiGeometry = pData->gui->saveGeometry();
                    pData->gui->close();
                    delete pData->gui;
                    pData->gui = nullptr;

                    pData->engine->callback(CALLBACK_ERROR, pData->id, 0, 0, 0.0f, "Plugin refused to open its own UI");
                    pData->engine->callback(CALLBACK_SHOW_GUI, pData->id, -1, 0, 0.0f, nullptr);
                    return;
                }

                if (fUi.type == PLUGIN_UI_QT)
                    pData->gui->setWidget((QWidget*)fUi.widget);

                updateUi();

                pData->gui->setWindowTitle(QString("%1 (GUI)").arg((const char*)pData->name));
                pData->gui->show();
            }
            else
            {
                fUi.descriptor->cleanup(fUi.handle);
                fUi.handle = nullptr;
                fUi.widget = nullptr;

                if (pData->gui != nullptr)
                {
                    pData->guiGeometry = pData->gui->saveGeometry();
                    pData->gui->close();
                    delete pData->gui;
                    pData->gui = nullptr;
                }
            }
#endif
        }
    }

    void idle() override
    {
        //if (fUi.type == PLUGIN_UI_NULL)
        //    return CarlaPlugin::idleGui();

        //if (! fAtomQueueOut.isEmpty())
        {
            Lv2AtomQueue tmpQueue;
            tmpQueue.copyDataFromQueue(fAtomQueueOut);

            uint32_t portIndex;
            const LV2_Atom* atom;
            const bool hasPortEvent(fUi.handle != nullptr && fUi.descriptor != nullptr && fUi.descriptor->port_event != nullptr);

            while (tmpQueue.get(&atom, &portIndex))
            {
                //carla_stdout("OUTPUT message IN GUI REMOVED FROM BUFFER");

                if (fUi.type == PLUGIN_UI_OSC)
                {
                    if (pData->osc.data.target != nullptr)
                    {
                        //QByteArray chunk((const char*)atom, lv2_atom_total_size(atom));
                        //osc_send_lv2_atom_transfer(pData->osc.data, portIndex, chunk.toBase64().constData());
                    }
                }
                else if (fUi.type != PLUGIN_UI_NULL)
                {
                    if (hasPortEvent)
                        fUi.descriptor->port_event(fUi.handle, portIndex, lv2_atom_total_size(atom), CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);
                }
            }
        }

        if (fUi.handle != nullptr && fUi.descriptor != nullptr)
        {
            if (fUi.type == PLUGIN_UI_EXTERNAL && fUi.widget != nullptr)
                LV2_EXTERNAL_UI_RUN((LV2_External_UI_Widget*)fUi.widget);

            if (fExt.uiidle != nullptr && fExt.uiidle->idle(fUi.handle) != 0)
            {
                showCustomUI(false);
                pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
            }
        }

        CarlaPlugin::idle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        carla_debug("Lv2Plugin::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        const float sampleRate(static_cast<float>(pData->engine->getSampleRate()));
        const uint32_t portCount(static_cast<uint32_t>(fRdfDescriptor->PortCount));

        uint32_t aIns, aOuts, cvIns, cvOuts, params, j;
        aIns = aOuts = cvIns = cvOuts = params = 0;
        List<unsigned int> evIns, evOuts;

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

        // check extensions
        fExt.options  = nullptr;
        fExt.programs = nullptr;
        fExt.state    = nullptr;
        fExt.worker   = nullptr;

        if (fDescriptor->extension_data != nullptr)
        {
            if (pData->extraHints & PLUGIN_HAS_EXTENSION_OPTIONS)
                fExt.options = (const LV2_Options_Interface*)fDescriptor->extension_data(LV2_OPTIONS__interface);

            if (pData->extraHints & PLUGIN_HAS_EXTENSION_PROGRAMS)
                fExt.programs = (const LV2_Programs_Interface*)fDescriptor->extension_data(LV2_PROGRAMS__Interface);

            if (pData->extraHints & PLUGIN_HAS_EXTENSION_STATE)
                fExt.state = (const LV2_State_Interface*)fDescriptor->extension_data(LV2_STATE__interface);

            if (pData->extraHints & PLUGIN_HAS_EXTENSION_WORKER)
                fExt.worker = (const LV2_Worker_Interface*)fDescriptor->extension_data(LV2_WORKER__interface);
        }

        if ((pData->options & PLUGIN_OPTION_FORCE_STEREO) != 0 && (aIns == 1 || aOuts == 1) && fExt.state == nullptr && fExt.worker == nullptr)
        {
            if (fHandle2 == nullptr)
                fHandle2 = fDescriptor->instantiate(fDescriptor, sampleRate, fRdfDescriptor->Bundle, fFeatures);

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
            fCvIn.createNew(cvIns);
            needsCtrlIn = true;
        }

        if (cvOuts > 0)
        {
            fCvOut.createNew(cvOuts);
            needsCtrlOut = true;
        }

        if (params > 0)
        {
            pData->param.createNew(params+cvIns+cvOuts);

            fParamBuffers = new float[params+cvIns+cvOuts];
#ifdef USE_JUCE
            FloatVectorOperations::clear(fParamBuffers, params+cvIns+cvOuts);
#else
#endif
        }

        if (evIns.count() > 0)
        {
            const size_t count(evIns.count());

            fEventsIn.createNew(count);

            for (j=0; j < count; ++j)
            {
                const uint32_t& type(evIns.getAt(j));

                if (type == CARLA_EVENT_DATA_ATOM)
                {
                    fEventsIn.data[j].type = CARLA_EVENT_DATA_ATOM;
                    fEventsIn.data[j].atom = lv2_atom_buffer_new(MAX_EVENT_BUFFER, CARLA_URI_MAP_ID_ATOM_SEQUENCE, true);
                }
                else if (type == CARLA_EVENT_DATA_EVENT)
                {
                    fEventsIn.data[j].type  = CARLA_EVENT_DATA_EVENT;
                    fEventsIn.data[j].event = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
                }
                else if (type == CARLA_EVENT_DATA_MIDI_LL)
                {
                    fEventsIn.data[j].type = CARLA_EVENT_DATA_MIDI_LL;
                    fEventsIn.data[j].midi = new LV2_MIDI;
                    fEventsIn.data[j].midi->event_count = 0;
                    fEventsIn.data[j].midi->capacity    = MAX_EVENT_BUFFER;
                    fEventsIn.data[j].midi->size        = 0;
                    fEventsIn.data[j].midi->data        = new unsigned char[MAX_EVENT_BUFFER];
                }
            }
        }

        if (evOuts.count() > 0)
        {
            const size_t count(evOuts.count());

            fEventsOut.createNew(count);

            for (j=0; j < count; ++j)
            {
                const uint32_t& type(evOuts.getAt(j));

                if (type == CARLA_EVENT_DATA_ATOM)
                {
                    fEventsOut.data[j].type = CARLA_EVENT_DATA_ATOM;
                    fEventsOut.data[j].atom = lv2_atom_buffer_new(MAX_EVENT_BUFFER, CARLA_URI_MAP_ID_ATOM_SEQUENCE, false);
                }
                else if (type == CARLA_EVENT_DATA_EVENT)
                {
                    fEventsOut.data[j].type  = CARLA_EVENT_DATA_EVENT;
                    fEventsOut.data[j].event = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
                }
                else if (type == CARLA_EVENT_DATA_MIDI_LL)
                {
                    fEventsOut.data[j].type = CARLA_EVENT_DATA_MIDI_LL;
                    fEventsOut.data[j].midi = new LV2_MIDI;
                    fEventsOut.data[j].midi->event_count = 0;
                    fEventsOut.data[j].midi->capacity    = MAX_EVENT_BUFFER;
                    fEventsOut.data[j].midi->size        = 0;
                    fEventsOut.data[j].midi->data        = new unsigned char[MAX_EVENT_BUFFER];
                }
            }
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        for (uint32_t i=0, iAudioIn=0, iAudioOut=0, iCvIn=0, iCvOut=0, iEvIn=0, iEvOut=0, iCtrl=0; i < portCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            portName.clear();

            if (LV2_IS_PORT_AUDIO(portTypes) || LV2_IS_PORT_ATOM_SEQUENCE(portTypes) || LV2_IS_PORT_CV(portTypes) || LV2_IS_PORT_EVENT(portTypes) || LV2_IS_PORT_MIDI_LL(portTypes))
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
                    j = iAudioIn++;
                    pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true);
                    pData->audioIn.ports[j].rindex = i;

                    if (forcedStereoIn)
                    {
                        portName += "_2";
                        pData->audioIn.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true);
                        pData->audioIn.ports[1].rindex = i;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    j = iAudioOut++;
                    pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
                    pData->audioOut.ports[j].rindex = i;

                    if (forcedStereoOut)
                    {
                        portName += "_2";
                        pData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
                        pData->audioOut.ports[1].rindex = i;
                    }
                }
                else
                    carla_stderr("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LV2_IS_PORT_CV(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    j = iCvIn++;
                    fCvIn.ports[j].port    = (CarlaEngineCVPort*)pData->client->addPort(kEnginePortTypeCV, portName, true);
                    fCvIn.ports[j].rindex  = i;
                    fCvIn.ports[j].param   = params + j;

                    fDescriptor->connect_port(fHandle, i, fCvIn.ports[j].port->getBuffer());

                    if (fHandle2 != nullptr)
                        fDescriptor->connect_port(fHandle2, i, fCvIn.ports[j].port->getBuffer());

                    j = fCvIn.ports[j].param;
                    pData->param.data[j].index  = j;
                    pData->param.data[j].rindex = i;
                    pData->param.data[j].hints  = PARAMETER_IS_INPUT|PARAMETER_IS_ENABLED|PARAMETER_IS_READ_ONLY;
                    pData->param.data[j].midiChannel = 0;
                    pData->param.data[j].midiCC = -1;
                    pData->param.ranges[j].min = -1.0f;
                    pData->param.ranges[j].max = 1.0f;
                    pData->param.ranges[j].def = 0.0f;
                    pData->param.ranges[j].step = 0.01f;
                    pData->param.ranges[j].stepSmall = 0.001f;
                    pData->param.ranges[j].stepLarge = 0.1f;
                    fParamBuffers[j] = 0.0f;

                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    j = iCvOut++;
                    fCvOut.ports[j].port    = (CarlaEngineCVPort*)pData->client->addPort(kEnginePortTypeCV, portName, false);
                    fCvOut.ports[j].rindex  = i;
                    fCvOut.ports[j].param   = params + cvIns + j;

                    fDescriptor->connect_port(fHandle, i, fCvOut.ports[j].port->getBuffer());

                    if (fHandle2 != nullptr)
                        fDescriptor->connect_port(fHandle2, i, fCvOut.ports[j].port->getBuffer());

                    j = fCvOut.ports[j].param;
                    pData->param.data[j].index  = j;
                    pData->param.data[j].rindex = i;
                    pData->param.data[j].hints  = PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE;
                    pData->param.data[j].midiChannel = 0;
                    pData->param.data[j].midiCC = -1;
                    pData->param.ranges[j].min = -1.0f;
                    pData->param.ranges[j].max = 1.0f;
                    pData->param.ranges[j].def = 0.0f;
                    pData->param.ranges[j].step = 0.01f;
                    pData->param.ranges[j].stepSmall = 0.001f;
                    pData->param.ranges[j].stepLarge = 0.1f;
                    fParamBuffers[j] = 0.0f;
                }
                else
                    carla_stderr("WARNING - Got a broken Port (CV, but not input or output)");
            }
            else if (LV2_IS_PORT_ATOM_SEQUENCE(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    j = iEvIn++;

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
                            fEventsIn.data[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsIn.ctrl = &fEventsIn.data[j];
                            fEventsIn.ctrlIndex = j;
                        }
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    j = iEvOut++;

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
                            fEventsOut.data[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsOut.ctrl = &fEventsOut.data[j];
                            fEventsOut.ctrlIndex = j;
                        }
                    }
                }
                else
                    carla_stderr("WARNING - Got a broken Port (Atom-Sequence, but not input or output)");
            }
            else if (LV2_IS_PORT_EVENT(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    j = iEvIn++;

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
                            fEventsIn.data[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsIn.ctrl = &fEventsIn.data[j];
                            fEventsIn.ctrlIndex = j;
                        }
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    j = iEvOut++;

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
                            fEventsOut.data[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsOut.ctrl = &fEventsOut.data[j];
                            fEventsOut.ctrlIndex = j;
                        }
                    }
                }
                else
                    carla_stderr("WARNING - Got a broken Port (Event, but not input or output)");
            }
            else if (LV2_IS_PORT_MIDI_LL(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    j = iEvIn++;

                    fDescriptor->connect_port(fHandle, i, fEventsIn.data[j].midi);

                    if (fHandle2 != nullptr)
                        fDescriptor->connect_port(fHandle2, i, fEventsIn.data[j].midi);

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
                        fEventsIn.data[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsIn.ctrl = &fEventsIn.data[j];
                            fEventsIn.ctrlIndex = j;
                        }
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    j = iEvOut++;

                    fDescriptor->connect_port(fHandle, i, fEventsOut.data[j].midi);

                    if (fHandle2 != nullptr)
                        fDescriptor->connect_port(fHandle2, i, fEventsOut.data[j].midi);

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
                        fEventsOut.data[j].port = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsOut.ctrl = &fEventsOut.data[j];
                            fEventsOut.ctrlIndex = j;
                        }
                    }
                }
                else
                    carla_stderr("WARNING - Got a broken Port (MIDI, but not input or output)");
            }
            else if (LV2_IS_PORT_CONTROL(portTypes))
            {
                const LV2_Property portProps(fRdfDescriptor->Ports[i].Properties);
                const LV2_Property portDesignation(fRdfDescriptor->Ports[i].Designation);
                const LV2_RDF_PortPoints portPoints(fRdfDescriptor->Ports[i].Points);

                j = iCtrl++;
                pData->param.data[j].index  = j;
                pData->param.data[j].rindex = i;
                pData->param.data[j].hints  = 0x0;
                pData->param.data[j].midiChannel = 0;
                pData->param.data[j].midiCC = -1;

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

                if (min > max)
                    max = min;
                else if (max < min)
                    min = max;

                // stupid hack for ir.lv2 (broken plugin)
                if (std::strcmp(fRdfDescriptor->URI, "http://factorial.hu/plugins/lv2/ir") == 0 && std::strncmp(fRdfDescriptor->Ports[i].Name, "FileHash", 8) == 0)
                {
                    min = 0.0f;
                    max = (float)0xffffff;
                }

                if (max - min == 0.0f)
                {
                    carla_stderr2("WARNING - Broken plugin parameter '%s': max - min == 0.0f", fRdfDescriptor->Ports[i].Name);
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

                if (LV2_IS_PORT_SAMPLE_RATE(portProps))
                {
                    min *= sampleRate;
                    max *= sampleRate;
                    def *= sampleRate;
                    pData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

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

                        //pData->param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        pData->param.data[j].hints = 0x0;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_FREEWHEELING(portDesignation))
                    {
                        //pData->param.data[j].type = PARAMETER_LV2_FREEWHEEL;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_TIME(portDesignation))
                    {
                        //pData->param.data[j].type = PARAMETER_LV2_TIME;
                    }
                    else
                    {
                        pData->param.data[j].hints |= PARAMETER_IS_INPUT;
                        pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlIn = true;
                    }

                    // MIDI CC value
                    const LV2_RDF_PortMidiMap& portMidiMap(fRdfDescriptor->Ports[i].MidiMap);

                    if (LV2_IS_PORT_MIDI_MAP_CC(portMidiMap.Type))
                    {
                        if (! MIDI_IS_CONTROL_BANK_SELECT(portMidiMap.Number))
                            pData->param.data[j].midiCC = portMidiMap.Number;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    if (LV2_IS_PORT_DESIGNATION_LATENCY(portDesignation))
                    {
                        min = 0.0f;
                        max = sampleRate;
                        def = 0.0f;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        //pData->param.data[j].type  = PARAMETER_LATENCY;
                        pData->param.data[j].hints = 0x0;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_SAMPLE_RATE(portDesignation))
                    {
                        def = sampleRate;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        //pData->param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        pData->param.data[j].hints = 0x0;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_FREEWHEELING(portDesignation))
                    {
                        carla_stderr("Plugin has freewheeling output port, this should not happen!");
                    }
                    else if (LV2_IS_PORT_DESIGNATION_TIME(portDesignation))
                    {
                        //pData->param.data[j].type = PARAMETER_LV2_TIME;
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
                    carla_stderr2("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LV2_IS_PORT_ENUMERATION(portProps))
                    pData->param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                if (LV2_IS_PORT_LOGARITHMIC(portProps))
                    pData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                if (LV2_IS_PORT_TRIGGER(portProps))
                    pData->param.data[j].hints |= PARAMETER_IS_TRIGGER;

                if (LV2_IS_PORT_STRICT_BOUNDS(portProps))
                    pData->param.data[j].hints |= PARAMETER_IS_STRICT_BOUNDS;

                // check if parameter is not enabled or automable
                if (LV2_IS_PORT_NOT_ON_GUI(portProps))
                {
                    pData->param.data[j].hints &= ~(PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE);
                }
                else if (LV2_IS_PORT_CAUSES_ARTIFACTS(portProps) || LV2_IS_PORT_EXPENSIVE(portProps) || LV2_IS_PORT_NOT_AUTOMATIC(portProps))
                    pData->param.data[j].hints &= ~PARAMETER_IS_AUTOMABLE;

                pData->param.ranges[j].min = min;
                pData->param.ranges[j].max = max;
                pData->param.ranges[j].def = def;
                pData->param.ranges[j].step = step;
                pData->param.ranges[j].stepSmall = stepSmall;
                pData->param.ranges[j].stepLarge = stepLarge;

                // Start parameters in their default values
                //if (pData->param.data[j].type != PARAMETER_LV2_FREEWHEEL)
                    fParamBuffers[j] = def;
                //else
                //    fParamBuffers[j] = min;

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

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true);
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

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        if (fEventsIn.ctrl != nullptr && fEventsIn.ctrl->port == nullptr)
            fEventsIn.ctrl->port = pData->event.portIn;

        if (fEventsOut.ctrl != nullptr && fEventsOut.ctrl->port == nullptr)
            fEventsOut.ctrl->port = pData->event.portOut;

        if (forcedStereoIn || forcedStereoOut)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;
        else
            pData->options &= ~PLUGIN_OPTION_FORCE_STEREO;

        // plugin hints
        pData->hints = 0x0;

        if (isRealtimeSafe())
            pData->hints |= PLUGIN_IS_RTSAFE;

        if (fUi.type != PLUGIN_UI_NULL)
        {
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;

            if (fUi.type == PLUGIN_UI_QT || fUi.type == PLUGIN_UI_PARENT)
                pData->hints |= PLUGIN_NEEDS_SINGLE_THREAD;
        }

        //if (LV2_IS_GENERATOR(fRdfDescriptor->Type[0], fRdfDescriptor->Type[1]))
        //    pData->hints |= PLUGIN_IS_SYNTH;

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints &= ~PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        if (fExt.state != nullptr || fExt.worker != nullptr)
        {
            if ((aIns == 0 || aIns == 2) && (aOuts == 0 || aOuts == 2) && evIns.count() <= 1 && evOuts.count() <= 1)
                pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;
        }
        else
        {
            if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0) && evIns.count() <= 1 && evOuts.count() <= 1)
                pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;
        }

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        evIns.clear();
        evOuts.clear();

        carla_debug("Lv2Plugin::reload() - end");
    }

    void reloadPrograms(const bool init) override
    {
        carla_debug("Lv2Plugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount  = pData->midiprog.count;
        const int32_t current = pData->midiprog.current;

        // special LV2 programs handling
        if (init)
        {
            pData->prog.clear();

            const uint32_t count(fRdfDescriptor->PresetCount);

            if (count > 0)
            {
                pData->prog.createNew(count);

                for (i=0; i < count; ++i)
                    pData->prog.names[i] = carla_strdup(fRdfDescriptor->Presets[i].Label);
            }
        }

        // Delete old programs
        pData->midiprog.clear();

        // Query new programs
        uint32_t count = 0;
        if (fExt.programs != nullptr && fExt.programs->get_program != nullptr && fExt.programs->select_program != nullptr)
        {
            while (fExt.programs->get_program(fHandle, count))
                ++count;
        }

        if (count > 0)
        {
            pData->midiprog.createNew(count);

            // Update data
            for (i=0; i < count; ++i)
            {
                const LV2_Program_Descriptor* const pdesc(fExt.programs->get_program(fHandle, i));
                CARLA_ASSERT(pdesc != nullptr);
                CARLA_ASSERT(pdesc->name != nullptr);

                pData->midiprog.data[i].bank    = pdesc->bank;
                pData->midiprog.data[i].program = pdesc->program;
                pData->midiprog.data[i].name    = carla_strdup(pdesc->name);
            }
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (pData->engine->isOscControlRegistered())
        {
            pData->engine->oscSend_control_set_midi_program_count(pData->id, count);

            for (i=0; i < count; ++i)
                pData->engine->oscSend_control_set_midi_program_data(pData->id, i, pData->midiprog.data[i].bank, pData->midiprog.data[i].program, pData->midiprog.data[i].name);
        }
#endif

        if (init)
        {
            if (count > 0)
            {
                setMidiProgram(0, false, false, false);
            }
            else
            {
                // load default state
                Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());

                if (const LilvState* state = lv2World.getState(fDescriptor->URI, (const LV2_URID_Map*)fFeatures[kFeatureIdUridMap]->data))
                {
                    lilv_state_restore(state, fExt.state, fHandle, carla_lilv_set_port_value, this, 0, fFeatures);

                    if (fHandle2 != nullptr)
                        lilv_state_restore(state, fExt.state, fHandle2, carla_lilv_set_port_value, this, 0, fFeatures);
                }
            }
        }
        else
        {
            // Check if current program is invalid
            bool programChanged = false;

            if (count == oldCount+1)
            {
                // one midi program added, probably created by user
                pData->midiprog.current = oldCount;
                programChanged = true;
            }
            else if (current < 0 && count > 0)
            {
                // programs exist now, but not before
                pData->midiprog.current = 0;
                programChanged = true;
            }
            else if (current >= 0 && count == 0)
            {
                // programs existed before, but not anymore
                pData->midiprog.current = -1;
                programChanged = true;
            }
            else if (current >= static_cast<int32_t>(count))
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

    void activate() override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (fDescriptor->activate != nullptr)
        {
            fDescriptor->activate(fHandle);

            if (fHandle2 != nullptr)
                fDescriptor->activate(fHandle2);
        }

        fFirstActive = true;
    }

    void deactivate() override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (fDescriptor->deactivate != nullptr)
        {
            fDescriptor->deactivate(fHandle);

            if (fHandle2 != nullptr)
                fDescriptor->deactivate(fHandle2);
        }
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames) override
    {
        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (i=0; i < pData->audioOut.count; ++i)
            {
#ifdef USE_JUCE
                FloatVectorOperations::clear(outBuffer[i], frames);
#else
#endif
            }

            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Handle events from different APIs

        LV2_Atom_Buffer_Iterator evInAtomIters[fEventsIn.count];
        LV2_Event_Iterator       evInEventIters[fEventsIn.count];
        LV2_MIDIState            evInMidiStates[fEventsIn.count];

        for (i=0; i < fEventsIn.count; ++i)
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
                fEventsIn.data[i].midi->event_count = 0;
                fEventsIn.data[i].midi->size        = 0;
                evInMidiStates[i].midi = fEventsIn.data[i].midi;
                evInMidiStates[i].frame_count = frames;
                evInMidiStates[i].position    = 0;
            }
        }

        for (i=0; i < fEventsOut.count; ++i)
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

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            uint8_t midiData[3] = { 0 };

            if (fEventsIn.ctrl != nullptr && (fEventsIn.ctrl->type & CARLA_EVENT_TYPE_MIDI) != 0)
            {
                k = fEventsIn.ctrlIndex;

                if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                {
                    for (i=0; i < MAX_MIDI_CHANNELS; ++i)
                    {
                        midiData[0] = MIDI_STATUS_CONTROL_CHANGE + i;
                        midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                            lv2_atom_buffer_write(&evInAtomIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&evInEventIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&evInMidiStates[k], 0, 3, midiData);

                        midiData[0] = MIDI_STATUS_CONTROL_CHANGE + i;
                        midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                            lv2_atom_buffer_write(&evInAtomIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&evInEventIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&evInMidiStates[k], 0, 3, midiData);
                    }
                }
                else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
                {
                    for (k=0; k < MAX_MIDI_NOTE; ++k)
                    {
                        midiData[0] = MIDI_STATUS_NOTE_OFF + pData->ctrlChannel;
                        midiData[1] = k;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                            lv2_atom_buffer_write(&evInAtomIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&evInEventIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&evInMidiStates[k], 0, 3, midiData);
                    }
                }
            }

            //if (pData->latency > 0)
            {
                //for (i=0; i < pData->audioIn.count; ++i)
                //    FloatVectorOperations::clear(pData->latencyBuffers[i], pData->latency);
            }

            pData->needsReset = false;

            CARLA_PROCESS_CONTINUE_CHECK;
        }

        // --------------------------------------------------------------------------------------------------------
        // TimeInfo

        const EngineTimeInfo& timeInfo(pData->engine->getTimeInfo());

        if (fFirstActive || fLastTimeInfo != timeInfo)
        {
            bool doPostRt;
            int32_t rindex;

            // update input ports
            for (k=0; k < pData->param.count; ++k)
            {
                //if (pData->param.data[k].type != PARAMETER_LV2_TIME)
                    continue;

                doPostRt = false;
                rindex = pData->param.data[k].rindex;

                CARLA_ASSERT(rindex >= 0 && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount));

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
                        fParamBuffers[k] = timeInfo.frame;
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND:
                    break;

                // BBT
                case LV2_PORT_DESIGNATION_TIME_BAR:
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && fLastTimeInfo.bbt.bar != timeInfo.bbt.bar)
                    {
                        fParamBuffers[k] = timeInfo.bbt.bar - 1;
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_BAR_BEAT:
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && (fLastTimeInfo.bbt.tick != timeInfo.bbt.tick ||
                                                                              fLastTimeInfo.bbt.ticksPerBeat != timeInfo.bbt.ticksPerBeat))
                    {
                        fParamBuffers[k] = timeInfo.bbt.beat - 1 + (double(timeInfo.bbt.tick) / timeInfo.bbt.ticksPerBeat);
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_BEAT:
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && fLastTimeInfo.bbt.beat != timeInfo.bbt.beat)
                    {
                        fParamBuffers[k] = timeInfo.bbt.beat - 1;
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_BEAT_UNIT:
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && fLastTimeInfo.bbt.beatType != timeInfo.bbt.beatType)
                    {
                        fParamBuffers[k] = timeInfo.bbt.beatType;
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR:
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && fLastTimeInfo.bbt.beatsPerBar != timeInfo.bbt.beatsPerBar)
                    {
                        fParamBuffers[k] = timeInfo.bbt.beatsPerBar;
                        doPostRt = true;
                    }
                    break;
                case LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE:
                    if ((timeInfo.valid & EngineTimeInfo::kValidBBT) != 0 && fLastTimeInfo.bbt.beatsPerMinute != timeInfo.bbt.beatsPerMinute)
                    {
                        fParamBuffers[k] = timeInfo.bbt.beatsPerMinute;
                        doPostRt = true;
                    }
                    break;
                }

                if (doPostRt)
                    pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 1, fParamBuffers[k]);
            }

            for (i = 0; i < fEventsIn.count; ++i)
            {
                if ((fEventsIn.data[i].type & CARLA_EVENT_DATA_ATOM) == 0 || (fEventsIn.data[i].type & CARLA_EVENT_TYPE_TIME) == 0)
                    continue;

                uint8_t timeInfoBuf[256] = { 0 };
                lv2_atom_forge_set_buffer(&fAtomForge, timeInfoBuf, sizeof(timeInfoBuf));

                LV2_Atom_Forge_Frame forgeFrame;
                lv2_atom_forge_blank(&fAtomForge, &forgeFrame, 1, CARLA_URI_MAP_ID_TIME_POSITION);
                lv2_atom_forge_property_head(&fAtomForge, CARLA_URI_MAP_ID_TIME_SPEED, 0);
                lv2_atom_forge_float(&fAtomForge, timeInfo.playing ? 1.0f : 0.0f);
                lv2_atom_forge_property_head(&fAtomForge, CARLA_URI_MAP_ID_TIME_FRAME, 0);
                lv2_atom_forge_long(&fAtomForge, timeInfo.frame);

                if (timeInfo.valid & EngineTimeInfo::kValidBBT)
                {
                    lv2_atom_forge_property_head(&fAtomForge, CARLA_URI_MAP_ID_TIME_BAR, 0);
                    lv2_atom_forge_long(&fAtomForge, timeInfo.bbt.bar - 1);
                    lv2_atom_forge_property_head(&fAtomForge, CARLA_URI_MAP_ID_TIME_BAR_BEAT, 0);
                    lv2_atom_forge_float(&fAtomForge, timeInfo.bbt.beat - 1 + (double(timeInfo.bbt.tick) / timeInfo.bbt.ticksPerBeat));
                    lv2_atom_forge_property_head(&fAtomForge, CARLA_URI_MAP_ID_TIME_BEAT, 0);
                    lv2_atom_forge_long(&fAtomForge, timeInfo.bbt.beat -1);
                    lv2_atom_forge_property_head(&fAtomForge, CARLA_URI_MAP_ID_TIME_BEAT_UNIT, 0);
                    lv2_atom_forge_float(&fAtomForge, timeInfo.bbt.beatType);
                    lv2_atom_forge_property_head(&fAtomForge, CARLA_URI_MAP_ID_TIME_BEATS_PER_BAR, 0);
                    lv2_atom_forge_float(&fAtomForge, timeInfo.bbt.beatsPerBar);
                    lv2_atom_forge_property_head(&fAtomForge, CARLA_URI_MAP_ID_TIME_BEATS_PER_MINUTE, 0);
                    lv2_atom_forge_float(&fAtomForge, timeInfo.bbt.beatsPerMinute);
                }

                LV2_Atom* const atom((LV2_Atom*)timeInfoBuf);
                lv2_atom_buffer_write(&evInAtomIters[i], 0, 0, atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));

                CARLA_ASSERT(atom->size < 256);
            }

            pData->postRtEvents.trySplice();

            std::memcpy(&fLastTimeInfo, &timeInfo, sizeof(EngineTimeInfo));

            CARLA_PROCESS_CONTINUE_CHECK;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (fEventsIn.ctrl != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // Message Input

            if (fAtomQueueIn.tryLock())
            {
                //if (! fAtomQueueIn.isEmpty())
                {
                    uint32_t portIndex;
                    const LV2_Atom* atom;

                    k = fEventsIn.ctrlIndex;

                    while (fAtomQueueIn.get(&atom, &portIndex))
                    {
                        carla_debug("Event input message sent to plugin DSP, type %i:\"%s\", size:%i/%i",
                                    atom->type, carla_lv2_urid_unmap(this, atom->type),
                                    atom->size, lv2_atom_total_size(atom)
                                    );

                        if (! lv2_atom_buffer_write(&evInAtomIters[k], 0, 0, atom->type, atom->size, LV2_ATOM_BODY_CONST(atom)))
                        {
                            carla_stdout("Event input buffer full, 1 message lost");
                            break;
                        }
                    }
                }

                fAtomQueueIn.unlock();
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
                    k = fEventsIn.ctrlIndex;

                    while (! pData->extNotes.data.isEmpty())
                    {
                        const ExternalMidiNote& note(pData->extNotes.data.getFirst(true));

                        CARLA_ASSERT(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                        uint8_t midiEvent[3];
                        midiEvent[0]  = (note.velo > 0) ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
                        midiEvent[0] += note.channel;
                        midiEvent[1]  = note.note;
                        midiEvent[2]  = note.velo;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                            lv2_atom_buffer_write(&evInAtomIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiEvent);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&evInEventIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiEvent);

                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&evInMidiStates[k], 0, 3, midiEvent);
                    }
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;
            bool sampleAccurate  = (pData->options & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t time, nEvents = (fEventsIn.ctrl->port != nullptr) ? fEventsIn.ctrl->port->getEventCount() : 0;
            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;
            uint32_t nextBankId = 0;

            if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                nextBankId = pData->midiprog.data[pData->midiprog.current].bank;

            for (i=0; i < nEvents; ++i)
            {
                const EngineEvent& event(fEventsIn.ctrl->port->getEvent(i));

                time = event.time;

                if (time >= frames)
                    continue;

                CARLA_ASSERT_INT2(time >= timeOffset, time, timeOffset);

                if (time > timeOffset && sampleAccurate)
                {
                    if (processSingle(inBuffer, outBuffer, time - timeOffset, timeOffset))
                    {
                        startTime  = 0;
                        timeOffset = time;

                        if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                            nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
                        else
                            nextBankId = 0;

                        // reset iters
                        k = fEventsIn.ctrlIndex;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                        {
                            lv2_atom_buffer_reset(fEventsIn.data[k].atom, true);
                            lv2_atom_buffer_begin(&evInAtomIters[k], fEventsIn.data[k].atom);
                        }
                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                        {
                            lv2_event_buffer_reset(fEventsIn.data[k].event, LV2_EVENT_AUDIO_STAMP, fEventsIn.data[k].event->data);
                            lv2_event_begin(&evInEventIters[k], fEventsIn.data[k].event);
                        }
                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                        {
                            fEventsIn.data[k].midi->event_count = 0;
                            fEventsIn.data[k].midi->size        = 0;
                            evInMidiStates[k].position = time;
                        }
                    }
                    else
                        startTime += timeOffset;
                }

                // Control change
                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl:
                {
                    const EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter:
                    {
#ifndef BUILD_BRIDGE
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) > 0)
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
                            }
                        }
#endif

                        // Control plugin parameters
                        for (k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_INPUT) == 0)
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
                        }

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param <= 0x5F)
                        {
                            uint8_t midiData[3];
                            midiData[0] = MIDI_STATUS_CONTROL_CHANGE + i;
                            midiData[1] = ctrlEvent.param;
                            midiData[2] = ctrlEvent.value*127.0f;

                            if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                                lv2_atom_buffer_write(&evInAtomIters[fEventsIn.ctrlIndex], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&evInEventIters[fEventsIn.ctrlIndex], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiData);

                            else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&evInMidiStates[fEventsIn.ctrlIndex], 0, 3, midiData);
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == pData->ctrlChannel && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == pData->ctrlChannel && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t nextProgramId = ctrlEvent.param;

                            for (k=0; k < pData->midiprog.count; ++k)
                            {
                                if (pData->midiprog.data[k].bank == nextBankId && pData->midiprog.data[k].program == nextProgramId)
                                {
                                    setMidiProgram(k, false, false, false);
                                    pData->postponeRtEvent(kPluginPostRtEventMidiProgramChange, k, 0, 0.0f);
                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            const uint32_t mtime(sampleAccurate ? startTime : time);

                            uint8_t midiData[3];
                            midiData[0] = MIDI_STATUS_CONTROL_CHANGE + i;
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
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }

                            const uint32_t mtime(sampleAccurate ? startTime : time);

                            uint8_t midiData[3];
                            midiData[0] = MIDI_STATUS_CONTROL_CHANGE + i;
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
                    }

                    break;
                }

                case kEngineEventTypeMidi:
                {
                    const EngineMidiEvent& midiEvent(event.midi);

                    uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                    uint8_t channel = event.channel;
                    uint32_t mtime  = sampleAccurate ? startTime : time;

                    if (MIDI_IS_STATUS_CHANNEL_PRESSURE(status) && (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (MIDI_IS_STATUS_CONTROL_CHANGE(status) && (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status) && (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status) && (pData->options & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    // Fix bad note-off
                    if (status == MIDI_STATUS_NOTE_ON && midiEvent.data[2] == 0)
                        status -= 0x10;

                    k = fEventsIn.ctrlIndex;

                    if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                        lv2_atom_buffer_write(&evInAtomIters[k], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, midiEvent.size, midiEvent.data);

                    else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                        lv2_event_write(&evInEventIters[k], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, midiEvent.size, midiEvent.data);

                    else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                        lv2midi_put_event(&evInMidiStates[k], mtime, midiEvent.size, midiEvent.data);

                    if (status == MIDI_STATUS_NOTE_ON)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, channel, midiEvent.data[1], midiEvent.data[2]);
                    else if (status == MIDI_STATUS_NOTE_OFF)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, channel, midiEvent.data[1], 0.0f);

                    break;
                }
                }
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(inBuffer, outBuffer, frames - timeOffset, timeOffset);

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(inBuffer, outBuffer, frames, 0);

        } // End of Plugin processing (no events)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (fEventsOut.ctrl != nullptr)
        {
            if (fEventsOut.ctrl->type & CARLA_EVENT_DATA_ATOM)
            {
                const uint32_t rindex(fEventsOut.ctrl->rindex);
                const LV2_Atom_Event* ev;
                LV2_Atom_Buffer_Iterator iter;

                uint8_t* data;
                lv2_atom_buffer_begin(&iter, fEventsOut.ctrl->atom);

                while (true)
                {
                    data = nullptr;
                    ev = lv2_atom_buffer_get(&iter, &data);

                    if (ev == nullptr || data == nullptr)
                        break;

                    if (ev->body.type == CARLA_URI_MAP_ID_MIDI_EVENT && fEventsOut.ctrl->port != nullptr)
                        fEventsOut.ctrl->port->writeMidiEvent(ev->time.frames, ev->body.size, data);

                    else if (ev->body.type == CARLA_URI_MAP_ID_ATOM_BLANK)
                        fAtomQueueOut.put(&ev->body, rindex);

                    lv2_atom_buffer_increment(&iter);
                }
            }
            else if ((fEventsOut.ctrl->type & CARLA_EVENT_DATA_EVENT) != 0 && fEventsOut.ctrl->port != nullptr)
            {
                const LV2_Event* ev;
                LV2_Event_Iterator iter;

                uint8_t* data;
                lv2_event_begin(&iter, fEventsOut.ctrl->event);

                while (true)
                {
                    data = nullptr;
                    ev = lv2_event_get(&iter, &data);

                    if (ev == nullptr || data == nullptr)
                        break;

                    if (ev->type == CARLA_URI_MAP_ID_MIDI_EVENT)
                        fEventsOut.ctrl->port->writeMidiEvent(ev->frames, ev->size, data);

                    lv2_event_increment(&iter);
                }
            }
            else if ((fEventsOut.ctrl->type & CARLA_EVENT_DATA_MIDI_LL) != 0 && fEventsOut.ctrl->port != nullptr)
            {
                LV2_MIDIState state = { fEventsOut.ctrl->midi, frames, 0 };

                uint32_t eventSize;
                double   eventTime;
                unsigned char* eventData;

                while (lv2midi_get_event(&state, &eventTime, &eventSize, &eventData) < frames)
                {
                    if (eventData == nullptr || eventSize == 0)
                        break;

                    fEventsOut.ctrl->port->writeMidiEvent(eventTime, eventSize, eventData);
                    lv2midi_step(&state);
                }
            }
        }

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (pData->event.portOut != nullptr)
        {
            uint8_t  channel;
            uint16_t param;
            float    value;

            for (k=0; k < pData->param.count; ++k)
            {
                if (pData->param.data[k].hints & PARAMETER_IS_INPUT)
                    continue;

                if (pData->param.data[k].hints & PARAMETER_IS_STRICT_BOUNDS)
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

        CARLA_PROCESS_CONTINUE_CHECK;

#if 0
        // --------------------------------------------------------------------------------------------------------
        // Final work

        if (fExt.worker != nullptr && fExt.worker->end_run != nullptr)
        {
            fExt.worker->end_run(fHandle);

            if (fHandle2 != nullptr)
                fExt.worker->end_run(fHandle2);
        }
#endif

        fFirstActive = false;

        // --------------------------------------------------------------------------------------------------------
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_ASSERT(frames > 0);

        if (frames == 0)
            return false;

        if (pData->audioIn.count > 0)
        {
            CARLA_ASSERT(inBuffer != nullptr);
            if (inBuffer == nullptr)
                return false;
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_ASSERT(outBuffer != nullptr);
            if (outBuffer == nullptr)
                return false;
        }

        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (i=0; i < pData->audioOut.count; ++i)
            {
                for (k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        for (i=0; i < pData->audioIn.count; ++i)
        {
#ifdef USE_JUCE
            FloatVectorOperations::copy(fAudioInBuffers[i], inBuffer[i]+timeOffset, frames);
#else
#endif
        }

        for (i=0; i < pData->audioOut.count; ++i)
        {
#ifdef USE_JUCE
            FloatVectorOperations::clear(fAudioOutBuffers[i], frames);
#else
#endif
        }

        // --------------------------------------------------------------------------------------------------------
        // Set CV input buffers

        for (i=0; i < fCvIn.count; ++i)
        {
            const uint32_t cvIndex(fCvIn.ports[i].param);
            const float    cvValue((fCvIn.ports[i].port->getBuffer()+timeOffset)[0]);

            if (fParamBuffers[cvIndex] != cvValue)
            {
                fParamBuffers[cvIndex] = cvValue;
                pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(cvIndex), 0, cvValue);
            }
        }

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fDescriptor->run(fHandle, frames);

        if (fHandle2 != nullptr)
            fDescriptor->run(fHandle2, frames);

        // --------------------------------------------------------------------------------------------------------
        // Special Parameters

        for (k=0; k < pData->param.count; ++k)
        {
            if ((pData->param.data[k].hints & PARAMETER_IS_INPUT) == 0)
                continue;

            if (pData->param.data[k].hints & PARAMETER_IS_TRIGGER)
            {
                if (fParamBuffers[k] != pData->param.ranges[k].def)
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
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && pData->postProc.dryWet != 1.0f;
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && (pData->postProc.balanceLeft != -1.0f || pData->postProc.balanceRight != 1.0f);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (k=0; k < frames; ++k)
                    {
                        // TODO
                        //if (k < pData->latency && pData->latency < frames)
                        //    bufValue = (pData->audioIn.count == 1) ? pData->latencyBuffers[0][k] : pData->latencyBuffers[i][k];
                        //else
                        //    bufValue = (pData->audioIn.count == 1) ? inBuffer[0][k-m_latency] : inBuffer[i][k-m_latency];

                        bufValue = fAudioInBuffers[(pData->audioIn.count == 1) ? 0 : i][k];
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
#ifdef USE_JUCE
                        FloatVectorOperations::copy(oldBufLeft, fAudioOutBuffers[i], frames);
#else
#endif
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (k=0; k < frames; ++k)
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
                    for (k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k] * pData->postProc.volume;
                }
            }

# if 0
            // Latency, save values for next callback, TODO
            if (pData->latency > 0 && pData->latency < frames)
            {
                for (i=0; i < pData->audioIn.count; ++i)
                    FloatVectorOperations::copy(pData->latencyBuffers[i], inBuffer[i] + (frames - pData->latency), pData->latency);
            }
# endif
        } // End of Post-processing
#else
        for (i=0; i < pData->audioOut.count; ++i)
        {
            for (k=0; k < frames; ++k)
                outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
#endif

        // --------------------------------------------------------------------------------------------------------
        // Set CV output buffers

        for (i=0; i < fCvOut.count; ++i)
        {
            const uint32_t cvIndex(fCvOut.ports[i].param);
            const float    cvValue(fCvOut.ports[i].port->getBuffer()[0]);

#if 0
            fCvOut.ports[i].port->writeBuffer(frames, timeOffset);
#endif
            fParamBuffers[cvIndex] = cvValue;
        }

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("Lv2Plugin::bufferSizeChanged(%i) - start", newBufferSize);

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

        for (uint32_t i=0; i < fCvIn.count; ++i)
        {
            if (CarlaEngineCVPort* const port = fCvIn.ports[i].port)
            {
                //port->setBufferSize(newBufferSize);

                fDescriptor->connect_port(fHandle, fCvIn.ports[i].rindex, port->getBuffer());

                if (fHandle2 != nullptr)
                    fDescriptor->connect_port(fHandle2, fCvIn.ports[i].rindex, port->getBuffer());
            }
        }

        for (uint32_t i=0; i < fCvOut.count; ++i)
        {
            if (CarlaEngineCVPort* const port = fCvOut.ports[i].port)
            {
                //port->setBufferSize(newBufferSize);

                fDescriptor->connect_port(fHandle, fCvOut.ports[i].rindex, port->getBuffer());

                if (fHandle2 != nullptr)
                    fDescriptor->connect_port(fHandle2, fCvOut.ports[i].rindex, port->getBuffer());
            }
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

        if (fLv2Options.maxBufferSize != static_cast<int>(newBufferSize) || (fLv2Options.minBufferSize > 1 && fLv2Options.minBufferSize != static_cast<int>(newBufferSize)))
        {
            fLv2Options.maxBufferSize = newBufferSize;

            if (fLv2Options.minBufferSize > 1)
                fLv2Options.minBufferSize = newBufferSize;

            if (fExt.options != nullptr && fExt.options->set != nullptr)
            {
                fExt.options->set(fHandle, &fLv2Options.optMinBlockLenth);
                fExt.options->set(fHandle, &fLv2Options.optMaxBlockLenth);
            }
        }

        carla_debug("Lv2Plugin::bufferSizeChanged(%i) - end", newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("Lv2Plugin::sampleRateChanged(%g) - start", newSampleRate);

        if (fLv2Options.sampleRate != newSampleRate)
        {
            fLv2Options.sampleRate = newSampleRate;

            if (fExt.options != nullptr && fExt.options->set != nullptr)
                fExt.options->set(fHandle, &fLv2Options.optSampleRate);
        }

        for (uint32_t k=0; k < pData->param.count; ++k)
        {
            //if (pData->param.data[k].type == PARAMETER_SAMPLE_RATE)
            {
                fParamBuffers[k] = newSampleRate;
                pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 1, fParamBuffers[k]);
            }
        }

        carla_debug("Lv2Plugin::sampleRateChanged(%g) - end", newSampleRate);
    }

    void offlineModeChanged(const bool isOffline) override
    {
        for (uint32_t k=0; k < pData->param.count; ++k)
        {
            //if (pData->param.data[k].type == PARAMETER_LV2_FREEWHEEL)
            {
                fParamBuffers[k] = isOffline ? pData->param.ranges[k].max : pData->param.ranges[k].min;
                pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 1, fParamBuffers[k]);
            }
        }
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void initBuffers() override
    {
        fEventsIn.initBuffers();
        fEventsOut.initBuffers();
        fCvIn.initBuffers();
        fCvOut.initBuffers();

        CarlaPlugin::initBuffers();
    }

    void clearBuffers() override
    {
        carla_debug("Lv2Plugin::clearBuffers() - start");

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

        if (fParamBuffers != nullptr)
        {
            delete[] fParamBuffers;
            fParamBuffers = nullptr;
        }

        fEventsIn.clear();
        fEventsOut.clear();
        fCvIn.clear();
        fCvOut.clear();

        CarlaPlugin::clearBuffers();

        carla_debug("Lv2Plugin::clearBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    void uiParameterChange(const uint32_t index, const float value) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(index < pData->param.count);

        if (fDescriptor == nullptr || fHandle == nullptr)
            return;
        if (index >= pData->param.count)
            return;

        if (fUi.type == PLUGIN_UI_OSC)
        {
            if (pData->osc.data.target != nullptr)
                osc_send_control(pData->osc.data, pData->param.data[index].rindex, value);
        }
        else
        {
            if (fUi.handle != nullptr && fUi.descriptor != nullptr && fUi.descriptor->port_event != nullptr)
                fUi.descriptor->port_event(fUi.handle, pData->param.data[index].rindex, sizeof(float), 0, &value);
        }
    }

    void uiMidiProgramChange(const uint32_t index) override
    {
        CARLA_ASSERT(index < pData->midiprog.count);

        if (index >= pData->midiprog.count)
            return;

        if (fUi.type == PLUGIN_UI_OSC)
        {
            if (pData->osc.data.target != nullptr)
                osc_send_midi_program(pData->osc.data, pData->midiprog.data[index].bank, pData->midiprog.data[index].program);
        }
        else
        {
            if (fExt.uiprograms != nullptr && fExt.uiprograms->select_program != nullptr)
                fExt.uiprograms->select_program(fUi.handle, pData->midiprog.data[index].bank, pData->midiprog.data[index].program);
        }
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) override
    {
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(note < MAX_MIDI_NOTE);
        CARLA_ASSERT(velo > 0 && velo < MAX_MIDI_VALUE);

        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (note >= MAX_MIDI_NOTE)
            return;
        if (velo >= MAX_MIDI_VALUE)
            return;

        if (fUi.type == PLUGIN_UI_OSC)
        {
            if (pData->osc.data.target != nullptr)
            {
                uint8_t midiData[4] = { 0 };
                midiData[1] = MIDI_STATUS_NOTE_ON + channel;
                midiData[2] = note;
                midiData[3] = velo;
                osc_send_midi(pData->osc.data, midiData);
            }
        }
        else
        {
            if (fUi.handle != nullptr && fUi.descriptor != nullptr && fUi.descriptor->port_event != nullptr && fEventsIn.ctrl != nullptr)
            {
                LV2_Atom_MidiEvent midiEv;
                midiEv.event.time.frames = 0;
                midiEv.event.body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                midiEv.event.body.size   = 3;
                midiEv.data[0] = MIDI_STATUS_NOTE_OFF + channel;
                midiEv.data[1] = note;
                midiEv.data[2] = velo;

                fUi.descriptor->port_event(fUi.handle, fEventsIn.ctrl->rindex, 3, CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, &midiEv);
            }
        }
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note) override
    {
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(note < MAX_MIDI_NOTE);

        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (note >= MAX_MIDI_NOTE)
            return;

        if (fUi.type == PLUGIN_UI_OSC)
        {
            if (pData->osc.data.target != nullptr)
            {
                uint8_t midiData[4] = { 0 };
                midiData[1] = MIDI_STATUS_NOTE_OFF + channel;
                midiData[2] = note;
                osc_send_midi(pData->osc.data, midiData);
            }
        }
        else
        {
            if (fUi.handle != nullptr && fUi.descriptor != nullptr && fUi.descriptor->port_event != nullptr && fEventsIn.ctrl != nullptr)
            {
                LV2_Atom_MidiEvent midiEv;
                midiEv.event.time.frames = 0;
                midiEv.event.body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                midiEv.event.body.size   = 3;
                midiEv.data[0] = MIDI_STATUS_NOTE_OFF + channel;
                midiEv.data[1] = note;
                midiEv.data[2] = 0;

                fUi.descriptor->port_event(fUi.handle, fEventsIn.ctrl->rindex, 3, CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, &midiEv);
            }
        }
    }

    // -------------------------------------------------------------------

protected:
//     void guiClosedCallback() override
//     {
//         showGui(false);
//         pData->engine->callback(CALLBACK_SHOW_GUI, pData->id, 0, 0, 0.0f, nullptr);
//     }

    // -------------------------------------------------------------------

    LV2_URID getCustomURID(const char* const uri)
    {
        CARLA_ASSERT(uri != nullptr);
        carla_debug("Lv2Plugin::getCustomURID(\"%s\")", uri);

        if (uri == nullptr)
            return CARLA_URI_MAP_ID_NULL;

        for (size_t i=0; i < fCustomURIDs.count(); ++i)
        {
            const char*& thisUri(fCustomURIDs.getAt(i));
            if (thisUri != nullptr && std::strcmp(thisUri, uri) == 0)
                return i;
        }

        fCustomURIDs.append(carla_strdup(uri));

        const LV2_URID urid(fCustomURIDs.count()-1);

        if (fUi.type == PLUGIN_UI_OSC && pData->osc.data.target != nullptr)
            osc_send_lv2_urid_map(pData->osc.data, urid, uri);

        return urid;
    }

    const char* getCustomURIString(const LV2_URID urid)
    {
        CARLA_ASSERT(urid != CARLA_URI_MAP_ID_NULL);
        CARLA_ASSERT_INT2(urid < fCustomURIDs.count(), urid, fCustomURIDs.count());
        carla_debug("Lv2Plugin::getCustomURIString(%i)", urid);

        if (urid == CARLA_URI_MAP_ID_NULL)
            return nullptr;
        if (urid < fCustomURIDs.count())
            return fCustomURIDs.getAt(urid);

        return nullptr;
    }

    // -------------------------------------------------------------------

    void handleProgramChanged(const int32_t index)
    {
        CARLA_ASSERT_INT(index >= -1, index);
        carla_debug("Lv2Plugin::handleProgramChanged(%i)", index);

        if (index == -1)
        {
            const ScopedSingleProcessLocker spl(this, true);
            return reloadPrograms(false);
        }

        if (index >= 0 && index < static_cast<int32_t>(pData->midiprog.count) && fExt.programs != nullptr && fExt.programs->get_program != nullptr)
        {
            if (const LV2_Program_Descriptor* progDesc = fExt.programs->get_program(fHandle, index))
            {
                CARLA_ASSERT(progDesc->name != nullptr);

                if (pData->midiprog.data[index].name != nullptr)
                    delete[] pData->midiprog.data[index].name;

                pData->midiprog.data[index].name = carla_strdup(progDesc->name ? progDesc->name : "");

                if (index == pData->midiprog.current)
                    pData->engine->callback(ENGINE_CALLBACK_UPDATE, pData->id, 0, 0, 0.0, nullptr);
                else
                    pData->engine->callback(ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0.0, nullptr);
            }
        }
    }

    // -------------------------------------------------------------------

    LV2_State_Status handleStateStore(const uint32_t key, const void* const value, const size_t size, const uint32_t type, const uint32_t flags)
    {
        CARLA_ASSERT(key != CARLA_URI_MAP_ID_NULL);
        CARLA_ASSERT(value != nullptr);
        CARLA_ASSERT(size > 0);
        carla_debug("Lv2Plugin::handleStateStore(%i, %p, " P_SIZE ", %i, %i)", key, value, size, type, flags);

        // basic checks
        if (key == CARLA_URI_MAP_ID_NULL)
        {
            carla_stderr2("Lv2Plugin::handleStateStore(%i, %p, " P_SIZE ", %i, %i) - invalid key", key, value, size, type, flags);
            return LV2_STATE_ERR_NO_PROPERTY;
        }

        if (value == nullptr || size == 0)
        {
            carla_stderr2("Lv2Plugin::handleStateStore(%i, %p, " P_SIZE ", %i, %i) - invalid value", key, value, size, type, flags);
            return LV2_STATE_ERR_NO_PROPERTY;
        }

        if ((flags & LV2_STATE_IS_POD) == 0)
        {
            carla_stderr2("Lv2Plugin::handleStateStore(%i, %p, " P_SIZE ", %i, %i) - invalid flags", key, value, size, type, flags);
            return LV2_STATE_ERR_BAD_FLAGS;
        }

        const char* const stype(carla_lv2_urid_unmap(this, type));

        if (stype == nullptr)
        {
            carla_stderr2("Lv2Plugin::handleStateStore(%i, %p, " P_SIZE ", %i, %i) - invalid type", key, value, size, type, flags);
            return LV2_STATE_ERR_BAD_TYPE;
        }

        const char* const uriKey(carla_lv2_urid_unmap(this, key));

        if (uriKey == nullptr)
        {
            carla_stderr2("Lv2Plugin::handleStateStore(%i, %p, " P_SIZE ", %i, %i) - invalid key URI", key, value, size, type, flags);
            return LV2_STATE_ERR_NO_PROPERTY;
        }

        // Check if we already have this key
        for (List<CustomData>::Itenerator it = pData->custom.begin(); it.valid(); it.next())
        {
            CustomData& data(*it);

            if (std::strcmp(data.key, uriKey) == 0)
            {
                if (data.value != nullptr)
                    delete[] data.value;

                if (std::strcmp(stype, LV2_ATOM__String) == 0 || std::strcmp(stype, LV2_ATOM__Path) == 0)
                    data.value = carla_strdup((const char*)value);
                //else
                    //data.value = carla_strdup(QByteArray((const char*)value, size).toBase64().constData());

                return LV2_STATE_SUCCESS;
            }
        }

        // Otherwise store it
        CustomData newData;
        newData.type = carla_strdup(stype);
        newData.key  = carla_strdup(uriKey);

        if (std::strcmp(stype, LV2_ATOM__String) == 0 || std::strcmp(stype, LV2_ATOM__Path) == 0)
            newData.value = carla_strdup((const char*)value);
        //else
            //newData.value = carla_strdup(QByteArray((const char*)value, size).toBase64().constData());

        pData->custom.append(newData);

        return LV2_STATE_SUCCESS;
    }

    const void* handleStateRetrieve(const uint32_t key, size_t* const size, uint32_t* const type, uint32_t* const flags)
    {
        CARLA_ASSERT(key != CARLA_URI_MAP_ID_NULL);
        carla_debug("Lv2Plugin::handleStateRetrieve(%i, %p, %p, %p)", key, size, type, flags);

        // basic checks
        if (key == CARLA_URI_MAP_ID_NULL)
        {
            carla_stderr2("Lv2Plugin::handleStateRetrieve(%i, %p, %p, %p) - invalid key", key, size, type, flags);
            return nullptr;
        }

        if (size == nullptr || type == nullptr || flags == nullptr)
        {
            carla_stderr2("Lv2Plugin::handleStateRetrieve(%i, %p, %p, %p) - invalid data", key, size, type, flags);
            return nullptr;
        }

        const char* const uriKey(carla_lv2_urid_unmap(this, key));

        if (uriKey == nullptr)
        {
            carla_stderr2("Lv2Plugin::handleStateRetrieve(%i, %p, %p, %p) - failed to find key", key, size, type, flags);
            return nullptr;
        }

        const char* stype = nullptr;
        const char* stringData = nullptr;

        for (List<CustomData>::Itenerator it = pData->custom.begin(); it.valid(); it.next())
        {
            CustomData& data(*it);

            if (std::strcmp(data.key, uriKey) == 0)
            {
                stype      = data.type;
                stringData = data.value;
                break;
            }
        }

        if (stringData == nullptr)
        {
            carla_stderr2("Lv2Plugin::handleStateRetrieve(%i, %p, %p, %p) - invalid key '%s'", key, size, type, flags, uriKey);
            return nullptr;
        }

        *type  = carla_lv2_urid_map(this, stype);
        *flags = LV2_STATE_IS_POD;

        if (std::strcmp(stype, LV2_ATOM__String) == 0 || std::strcmp(stype, LV2_ATOM__Path) == 0)
        {
            *size = std::strlen(stringData);
            return stringData;
        }
        else
        {
//             static QByteArray chunk;
//             chunk = QByteArray::fromBase64(stringData);
//             *size = chunk.size();
//             return chunk.constData();
            return nullptr;
        }
    }

    // -------------------------------------------------------------------

    LV2_Worker_Status handleWorkerSchedule(const uint32_t size, const void* const data)
    {
        carla_stdout("Lv2Plugin::handleWorkerSchedule(%i, %p)", size, data);

        if (fExt.worker == nullptr || fExt.worker->work == nullptr)
        {
            carla_stderr("Lv2Plugin::handleWorkerSchedule(%i, %p) - plugin has no worker", size, data);
            return LV2_WORKER_ERR_UNKNOWN;
        }

        //if (pData->engine->isOffline())
        fExt.worker->work(fHandle, carla_lv2_worker_respond, this, size, data);
        //else
        //    postponeEvent(PluginPostEventCustom, size, 0, 0.0, data);

        return LV2_WORKER_SUCCESS;
    }

    LV2_Worker_Status handleWorkerRespond(const uint32_t size, const void* const data)
    {
        carla_stdout("Lv2Plugin::handleWorkerRespond(%i, %p)", size, data);

#if 0
        LV2_Atom_Worker workerAtom;
        workerAtom.atom.type = CARLA_URI_MAP_ID_ATOM_WORKER;
        workerAtom.atom.size = sizeof(LV2_Atom_Worker_Body);
        workerAtom.body.size = size;
        workerAtom.body.data = data;

        atomQueueIn.put(0, (const LV2_Atom*)&workerAtom);
#endif

        return LV2_WORKER_SUCCESS;
    }

    // -------------------------------------------------------------------

    void handleExternalUiClosed()
    {
        CARLA_ASSERT(fUi.type == PLUGIN_UI_EXTERNAL);

        if (fUi.handle != nullptr && fUi.descriptor != nullptr && fUi.descriptor->cleanup != nullptr)
            fUi.descriptor->cleanup(fUi.handle);

        fUi.handle = nullptr;
        fUi.widget = nullptr;
        pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
    }

    uint32_t handleUiPortMap(const char* const symbol)
    {
        CARLA_ASSERT(symbol != nullptr);

        if (symbol == nullptr)
            return LV2UI_INVALID_PORT_INDEX;

        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Ports[i].Symbol, symbol) == 0)
                return i;
        }

        return LV2UI_INVALID_PORT_INDEX;
    }

    int handleUiResize(const int width, const int height)
    {
        CARLA_ASSERT(width > 0);
        CARLA_ASSERT(height > 0);

        if (width <= 0 || height <= 0)
            return 1;

        //if (pData->gui != nullptr)
        //    pData->gui->setSize(width, height);

        return 0;
    }

    void handleUiWrite(const uint32_t rindex, const uint32_t bufferSize, const uint32_t format, const void* const buffer)
    {
        if (format == 0)
        {
            CARLA_ASSERT(buffer != nullptr);
            CARLA_ASSERT(bufferSize == sizeof(float));

            if (buffer == nullptr || bufferSize != sizeof(float))
                return;

            const float value(*(const float*)buffer);

            for (uint32_t i=0; i < pData->param.count; ++i)
            {
                if (pData->param.data[i].rindex == static_cast<int32_t>(rindex))
                {
                    if (fParamBuffers[i] != value)
                        setParameterValue(i, value, false, true, true);
                    break;
                }
            }
        }
        else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM || format == CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT)
        {
            CARLA_ASSERT(bufferSize != 0);
            CARLA_ASSERT(buffer != nullptr);

            if (bufferSize == 0 || buffer == nullptr)
                return;

            fAtomQueueIn.put((const LV2_Atom*)buffer, rindex);
        }
        else
        {
            carla_stdout("Lv2Plugin::handleUiWrite(%i, %i, %i:\"%s\", %p) - unknown format", rindex, bufferSize, format, carla_lv2_urid_unmap(this, format), buffer);
        }
    }

    void handleLilvSetPortValue(const char* const portSymbol, const void* const value, uint32_t size, uint32_t type)
    {
        CARLA_ASSERT(portSymbol != nullptr);
        CARLA_ASSERT(value != nullptr);
        CARLA_ASSERT(size > 0);
        CARLA_ASSERT(type != CARLA_URI_MAP_ID_NULL);

        if (portSymbol == nullptr)
            return;
        if (value == nullptr)
            return;
        if (size == 0)
            return;
        if (type == CARLA_URI_MAP_ID_NULL)
            return;

        const int32_t rindex(handleUiPortMap(portSymbol));

        if (rindex < 0)
            return;

        if (size == sizeof(float) && type == CARLA_URI_MAP_ID_ATOM_FLOAT)
        {
            const float valuef(*(const float*)value);

            for (uint32_t i=0; i < pData->param.count; ++i)
            {
                if (pData->param.data[i].rindex == rindex)
                {
                    setParameterValue(i, valuef, true, true, true);
                    break;
                }
            }
        }
    }

    // -------------------------------------------------------------------

    bool isRealtimeSafe() const
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        for (uint32_t i=0; i < fRdfDescriptor->FeatureCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Features[i].URI, LV2_CORE__hardRTCapable) == 0)
                return true;
        }

        return false;
    }

    bool needsFixedBuffer() const
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        for (uint32_t i=0; i < fRdfDescriptor->FeatureCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Features[i].URI, LV2_BUF_SIZE__fixedBlockLength) == 0)
                return true;
        }

        return false;
    }

    // -------------------------------------------------------------------

    const char* getUiBridgePath(const LV2_Property type) const
    {
        const EngineOptions& options(pData->engine->getOptions());

        switch (type)
        {
//         case LV2_UI_GTK2:
//             return options.bridge_lv2Gtk2;
//         case LV2_UI_GTK3:
//             return options.bridge_lv2Gtk3;
//         case LV2_UI_QT4:
//             return options.bridge_lv2Qt4;
//         case LV2_UI_QT5:
//             return options.bridge_lv2Qt5;
//         case LV2_UI_COCOA:
//             return options.bridge_lv2Cocoa;
//         case LV2_UI_WINDOWS:
//             return options.bridge_lv2Win;
//         case LV2_UI_X11:
//             return options.bridge_lv2X11;
        default:
            return nullptr;
        }
    }

    bool isUiBridgeable(const uint32_t uiId) const
    {
        const LV2_RDF_UI& rdfUi(fRdfDescriptor->UIs[uiId]);

        for (uint32_t i=0; i < rdfUi.FeatureCount; ++i)
        {
            if (std::strcmp(rdfUi.Features[i].URI, LV2_INSTANCE_ACCESS_URI) == 0)
                return false;
            if (std::strcmp(rdfUi.Features[i].URI, LV2_DATA_ACCESS_URI) == 0)
                return false;
        }

        return true;
    }

    bool isUiResizable() const
    {
        for (uint32_t i=0; i < fUi.rdfDescriptor->FeatureCount; ++i)
        {
            if (std::strcmp(fUi.rdfDescriptor->Features[i].URI, LV2_UI__fixedSize) == 0)
                return false;
            if (std::strcmp(fUi.rdfDescriptor->Features[i].URI, LV2_UI__noUserResize) == 0)
                return false;
        }

        return true;
    }

    void updateUi()
    {
        CARLA_ASSERT(fUi.handle != nullptr);
        CARLA_ASSERT(fUi.descriptor != nullptr);

        fExt.uiidle = nullptr;
        fExt.uiprograms = nullptr;

        if (fUi.descriptor->extension_data != nullptr)
        {
            fExt.uiidle     = (const LV2UI_Idle_Interface*)fUi.descriptor->extension_data(LV2_UI__idleInterface);
            fExt.uiprograms = (const LV2_Programs_UI_Interface*)fUi.descriptor->extension_data(LV2_PROGRAMS__UIInterface);

            // check if invalid
            if (fExt.uiidle != nullptr && fExt.uiidle->idle == nullptr)
                fExt.uiidle = nullptr;

            if (fExt.uiprograms != nullptr && fExt.uiprograms->select_program == nullptr)
                fExt.uiprograms = nullptr;

            // update midi program
            if (fExt.uiprograms && pData->midiprog.count > 0 && pData->midiprog.current >= 0)
                fExt.uiprograms->select_program(fUi.handle, pData->midiprog.data[pData->midiprog.current].bank,
                                                            pData->midiprog.data[pData->midiprog.current].program);
        }

        if (fUi.descriptor->port_event != nullptr)
        {
            // update control ports
            float value;
            for (uint32_t i=0; i < pData->param.count; ++i)
            {
                value = getParameterValue(i);
                fUi.descriptor->port_event(fUi.handle, pData->param.data[i].rindex, sizeof(float), CARLA_URI_MAP_ID_NULL, &value);
            }
        }
    }

    // -------------------------------------------------------------------

public:
    bool init(const char* const name, const char* const uri)
    {
        CARLA_ASSERT(pData->engine != nullptr);
        CARLA_ASSERT(pData->client == nullptr);
        CARLA_ASSERT(uri != nullptr);

        // ---------------------------------------------------------------
        // first checks

        if (pData->engine == nullptr)
        {
            return false;
        }

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (uri == nullptr)
        {
            pData->engine->setLastError("null uri");
            return false;
        }

        // ---------------------------------------------------------------
        // get plugin from lv2_rdf (lilv)

        Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());
        lv2World.init();

        fRdfDescriptor = lv2_rdf_new(uri, true);

        if (fRdfDescriptor == nullptr)
        {
            pData->engine->setLastError("Failed to find the requested plugin in the LV2 Bundle");
            return false;
        }

        // ---------------------------------------------------------------
        // open DLL

        if (! pData->libOpen(fRdfDescriptor->Binary))
        {
            pData->engine->setLastError(lib_error(fRdfDescriptor->Binary));
            return false;
        }

        // ---------------------------------------------------------------
        // initialize options

        fLv2Options.minBufferSize = 1;
        fLv2Options.maxBufferSize = pData->engine->getBufferSize();
        fLv2Options.sampleRate    = pData->engine->getSampleRate();

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

        LV2_RtMemPool_Pool* const rtMemPoolFt = new LV2_RtMemPool_Pool;
        lv2_rtmempool_init(rtMemPoolFt);

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

        for (uint32_t i=0; i < kFeatureIdWorker+1; ++i)
            fFeatures[i] = new LV2_Feature;

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

        fFeatures[kFeatureIdRtMemPool]->URI  = LV2_RTSAFE_MEMORY_POOL__Pool;
        fFeatures[kFeatureIdRtMemPool]->data = rtMemPoolFt;

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

        // check if it's possible to use non-fixed buffer size
        if (! needsFixedBuffer())
            fFeatures[kFeatureIdBufSizeFixed]->URI = LV2_BUF_SIZE__boundedBlockLength;

        // ---------------------------------------------------------------
        // get DLL main entry

#if 0
        const LV2_Lib_Descriptor_Function libDescFn = (LV2_Lib_Descriptor_Function)pData->libSymbol("lv2_lib_descriptor");

        if (libDescFn != nullptr)
        {
            // -----------------------------------------------------------
            // get lib descriptor

            const LV2_Lib_Descriptor* libFn = nullptr; //descLibFn(fRdfDescriptor->Bundle, features);

            if (libFn == nullptr || libFn->get_plugin == nullptr)
            {
                pData->engine->setLastError("Plugin failed to return library descriptor");
                return false;
            }

            // -----------------------------------------------------------
            // get descriptor that matches URI

            uint32_t i = 0;
            while ((fDescriptor = libFn->get_plugin(libFn->handle, i++)))
            {
                if (std::strcmp(fDescriptor->URI, uri) == 0)
                    break;
            }

            if (fDescriptor == nullptr)
                libFn->cleanup(libFn->handle);
        else
#endif
        {
            const LV2_Descriptor_Function descFn = (LV2_Descriptor_Function)pData->libSymbol("lv2_descriptor");

            if (descFn == nullptr)
            {
                pData->engine->setLastError("Could not find the LV2 Descriptor in the plugin library");
                return false;
            }

            // -----------------------------------------------------------
            // get descriptor that matches URI

            uint32_t i = 0;
            while ((fDescriptor = descFn(i++)))
            {
                carla_debug("LV2 Init @%i => '%s' vs '%s'", i, fDescriptor->URI, uri);
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
        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (! is_lv2_port_supported(portTypes))
            {
                if (! LV2_IS_PORT_OPTIONAL(fRdfDescriptor->Ports[i].Properties))
                {
                    pData->engine->setLastError("Plugin requires a port type that is not currently supported");
                    canContinue = false;
                    break;
                }
            }
        }

        // Check supported features
        for (uint32_t i=0; i < fRdfDescriptor->FeatureCount && canContinue; ++i)
        {
            if (LV2_IS_FEATURE_REQUIRED(fRdfDescriptor->Features[i].Type) && ! is_lv2_feature_supported(fRdfDescriptor->Features[i].URI))
            {
//                 QString msg(QString("Plugin requires a feature that is not supported:\n%1").arg(fRdfDescriptor->Features[i].URI));
//                 pData->engine->setLastError(msg.toUtf8().constData());
                canContinue = false;
                break;
            }
        }

        // Check extensions
        for (uint32_t i=0; i < fRdfDescriptor->ExtensionCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_OPTIONS__interface) == 0)
                pData->extraHints |= PLUGIN_HAS_EXTENSION_OPTIONS;
            else if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_PROGRAMS__Interface) == 0)
                pData->extraHints |= PLUGIN_HAS_EXTENSION_PROGRAMS;
            else if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_STATE__interface) == 0)
                pData->extraHints |= PLUGIN_HAS_EXTENSION_STATE;
            else if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_WORKER__interface) == 0)
                pData->extraHints |= PLUGIN_HAS_EXTENSION_WORKER;
            else
                carla_stdout("Plugin has non-supported extension: '%s'", fRdfDescriptor->Extensions[i]);
        }

        if (! canContinue)
        {
            // error already set
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr)
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
        // initialize plugin

        fHandle = fDescriptor->instantiate(fDescriptor, pData->engine->getSampleRate(), fRdfDescriptor->Bundle, fFeatures);

        if (fHandle == nullptr)
        {
            pData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // load plugin settings

        {
            // set default options
            pData->options = 0x0;

            pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

            if (getMidiInCount() > 0 || needsFixedBuffer())
                pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

            if (pData->engine->getOptions().forceStereo)
                pData->options |= PLUGIN_OPTION_FORCE_STEREO;

            if (getMidiInCount() > 0)
            {
                pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
                pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            }

            // load settings
            pData->idStr  = "LV2/";
            pData->idStr += uri;
            pData->options = pData->loadSettings(pData->options, getOptionsAvailable());

            // ignore settings, we need this anyway
            if (getMidiInCount() > 0 || needsFixedBuffer())
                pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
        }

        // ---------------------------------------------------------------
        // gui stuff

        if (fRdfDescriptor->UICount == 0)
            return true;

        // -----------------------------------------------------------
        // find more appropriate ui

        int eQt4, eQt5, eCocoa, eWindows, eX11, eGtk2, eGtk3, iCocoa, iWindows, iX11, iQt4, iQt5, iExt, iFinal;
        eQt4 = eQt5 = eCocoa = eWindows = eX11 = eGtk2 = eGtk3 = iQt4 = iQt5 = iCocoa = iWindows = iX11 = iExt = iFinal = -1;

        const bool preferUiBridges(true);
//#ifdef BUILD_BRIDGE
//        const bool preferUiBridges(pData->engine->getOptions().preferUiBridges);
//#else
//        const bool preferUiBridges(pData->engine->getOptions().preferUiBridges && (pData->hints & PLUGIN_IS_BRIDGE) == 0);
//#endif

        for (uint32_t i=0; i < fRdfDescriptor->UICount; ++i)
        {
            CARLA_ASSERT(fRdfDescriptor->UIs[i].URI != nullptr);

            if (fRdfDescriptor->UIs[i].URI == nullptr)
            {
                carla_stderr("Plugin has an UI without a valid URI");
                continue;
            }

            switch (fRdfDescriptor->UIs[i].Type)
            {
#if 0//(QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
            case LV2_UI_QT4:
                if (isUiBridgeable(i))
                    eQt4 = i;
                break;
#else
            case LV2_UI_QT4:
                if (isUiBridgeable(i) && preferUiBridges)
                    eQt4 = i;
                iQt4 = i;
                break;
#endif

#if 0//(QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
            case LV2_UI_QT5:
                if (isUiBridgeable(i) && preferUiBridges)
                    eQt5 = i;
                iQt5 = i;
                break;
#else
            case LV2_UI_QT5:
                if (isUiBridgeable(i) && preferUiBridges)
                    eQt5 = i;
                break;
#endif

#ifdef CARLA_OS_MAC
            case LV2_UI_COCOA:
                if (isUiBridgeable(i) && preferUiBridges)
                    eCocoa = i;
                iCocoa = i;
                break;
#endif

#ifdef CARLA_OS_WIN
            case LV2_UI_WINDOWS:
                if (isUiBridgeable(i) && preferUiBridges)
                    eWindows = i;
                iWindows = i;
                break;
#endif

            case LV2_UI_X11:
                if (isUiBridgeable(i) && preferUiBridges)
                    eX11 = i;
#ifdef Q_WS_X11
                iX11 = i;
#endif
                break;

            case LV2_UI_GTK2:
                if (isUiBridgeable(i))
                    eGtk2 = i;
                break;

            case LV2_UI_GTK3:
                if (isUiBridgeable(i))
                    eGtk3 = i;
                break;

            case LV2_UI_EXTERNAL:
            case LV2_UI_OLD_EXTERNAL:
                iExt = i;
                break;

            default:
                break;
            }
        }

        if (eQt4 >= 0)
            iFinal = eQt4;
        else if (eQt5 >= 0)
            iFinal = eQt5;
        else if (eCocoa >= 0)
            iFinal = eCocoa;
        else if (eWindows >= 0)
            iFinal = eWindows;
        else if (eX11 >= 0)
            iFinal = eX11;
        else if (iQt4 >= 0)
            iFinal = iQt4;
        else if (iQt5 >= 0)
            iFinal = iQt5;
        else if (iCocoa >= 0)
            iFinal = iCocoa;
        else if (iWindows >= 0)
            iFinal = iWindows;
        else if (iX11 >= 0)
            iFinal = iX11;
        else if (iExt >= 0)
            iFinal = iExt;
        else if (eGtk2 >= 0)
            iFinal = eGtk2;
        else if (eGtk3 >= 0)
            iFinal = eGtk3;

        if (iFinal < 0)
        {
            carla_stderr("Failed to find an appropriate LV2 UI for this plugin");
            return true;
        }

        fUi.rdfDescriptor = &fRdfDescriptor->UIs[iFinal];

        // -----------------------------------------------------------
        // check supported ui features

        canContinue = true;

        for (uint32_t i=0; i < fUi.rdfDescriptor->FeatureCount; ++i)
        {
            if (LV2_IS_FEATURE_REQUIRED(fUi.rdfDescriptor->Features[i].Type) && ! is_lv2_ui_feature_supported(fUi.rdfDescriptor->Features[i].URI))
            {
                carla_stderr2("Plugin UI requires a feature that is not supported:\n%s", fUi.rdfDescriptor->Features[i].URI);
                canContinue = false;
                break;
            }
        }

        if (! canContinue)
        {
            fUi.rdfDescriptor = nullptr;
            return true;
        }

        // -----------------------------------------------------------
        // initialize ui according to type

        const LV2_Property uiType(fUi.rdfDescriptor->Type);

        if (iFinal == eQt4 || iFinal == eQt5 || iFinal == eCocoa || iFinal == eWindows || iFinal == eX11 || iFinal == eGtk2 || iFinal == eGtk3)
        {
            // -------------------------------------------------------
            // initialize ui bridge

            if (const char* const oscBinary = getUiBridgePath(uiType))
            {
                fUi.type = PLUGIN_UI_OSC;
                pData->osc.thread.setOscData(oscBinary, fDescriptor->URI, fUi.rdfDescriptor->URI);
            }
        }
        else
        {
            // -------------------------------------------------------
            // open UI DLL

            if (! pData->uiLibOpen(fUi.rdfDescriptor->Binary))
            {
                carla_stderr2("Could not load UI library, error was:\n%s", lib_error(fUi.rdfDescriptor->Binary));
                fUi.rdfDescriptor = nullptr;
                return true;
            }

            // -------------------------------------------------------
            // get UI DLL main entry

            LV2UI_DescriptorFunction uiDescFn = (LV2UI_DescriptorFunction)pData->uiLibSymbol("lv2ui_descriptor");

            if (uiDescFn == nullptr)
            {
                carla_stderr2("Could not find the LV2UI Descriptor in the UI library");
                pData->uiLibClose();
                fUi.rdfDescriptor = nullptr;
                return true;
            }

            // -------------------------------------------------------
            // get UI descriptor that matches UI URI

            uint32_t i = 0;
            while ((fUi.descriptor = uiDescFn(i++)))
            {
                if (std::strcmp(fUi.descriptor->URI, fUi.rdfDescriptor->URI) == 0)
                    break;
            }

            if (fUi.descriptor == nullptr)
            {
                carla_stderr2("Could not find the requested GUI in the plugin UI library");
                pData->uiLibClose();
                fUi.rdfDescriptor = nullptr;
                return true;
            }

            // -------------------------------------------------------
            // check if ui is usable

            switch (uiType)
            {
#if 0//(QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
            case LV2_UI_QT5:
                carla_debug("Will use LV2 Qt5 UI");
                fUi.type = PLUGIN_UI_QT;
                break;
#else
            case LV2_UI_QT4:
                carla_debug("Will use LV2 Qt4 UI");
                fUi.type = PLUGIN_UI_QT;
                break;
#endif

#ifdef CARLA_OS_MAC
            case LV2_UI_COCOA:
                carla_debug("Will use LV2 Cocoa UI");
                fUi.type = PLUGIN_UI_PARENT;
                break;
#endif

#ifdef CARLA_OS_WIN
            case LV2_UI_WINDOWS:
                carla_debug("Will use LV2 Windows UI");
                fUi.type = PLUGIN_UI_PARENT;
                break;
#endif

#ifdef Q_WS_X11
            case LV2_UI_X11:
                carla_debug("Will use LV2 X11 UI");
                fUi.type = PLUGIN_UI_PARENT;
                break;
#endif

            case LV2_UI_GTK2:
                carla_debug("Will use LV2 Gtk2 UI, NOT!");
                break;

            case LV2_UI_GTK3:
                carla_debug("Will use LV2 Gtk3 UI, NOT!");
                break;

            case LV2_UI_EXTERNAL:
            case LV2_UI_OLD_EXTERNAL:
                carla_debug("Will use LV2 External UI");
                fUi.type = PLUGIN_UI_EXTERNAL;
                break;
            }

            if (fUi.type == PLUGIN_UI_NULL)
            {
                pData->uiLibClose();
                fUi.descriptor = nullptr;
                fUi.rdfDescriptor = nullptr;
                return true;
            }

            // -------------------------------------------------------
            // initialize ui features (part 1)

            //QString guiTitle(QString("%1 (GUI)").arg((const char*)pData->name));

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
            uiExternalHostFt->plugin_human_id            = nullptr; //carla_strdup(guiTitle.toUtf8().constData());

            // -------------------------------------------------------
            // initialize ui features (part 2)

            for (uint32_t i=kFeatureIdUiDataAccess; i < kFeatureCount; ++i)
                fFeatures[i] = new LV2_Feature;

            fFeatures[kFeatureIdUiDataAccess]->URI      = LV2_DATA_ACCESS_URI;
            fFeatures[kFeatureIdUiDataAccess]->data     = uiDataFt;

            fFeatures[kFeatureIdUiInstanceAccess]->URI  = LV2_INSTANCE_ACCESS_URI;
            fFeatures[kFeatureIdUiInstanceAccess]->data = fHandle;

            fFeatures[kFeatureIdUiIdle]->URI           = LV2_UI__idle;
            fFeatures[kFeatureIdUiIdle]->data          = nullptr;

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
        }

        return true;
    }

    // -------------------------------------------------------------------

    void handleTransferAtom(const uint32_t portIndex, const LV2_Atom* const atom)
    {
        CARLA_ASSERT(atom != nullptr);
        carla_debug("Lv2Plugin::handleTransferAtom(%i, %p)", portIndex, atom);

        fAtomQueueIn.put(atom, portIndex);
    }

    void handleUridMap(const LV2_URID urid, const char* const uri)
    {
        CARLA_ASSERT(urid != CARLA_URI_MAP_ID_NULL);
        CARLA_ASSERT(uri != nullptr);
        carla_debug("Lv2Plugin::handleUridMap(%i, \"%s\")", urid, uri);

        CARLA_SAFE_ASSERT_INT2(urid == fCustomURIDs.count(), urid, fCustomURIDs.count());

        if (urid != fCustomURIDs.count())
            return;

        fCustomURIDs.append(carla_strdup(uri));
    }

    // -------------------------------------------------------------------

private:
    LV2_Handle   fHandle;
    LV2_Handle   fHandle2;
    LV2_Feature* fFeatures[kFeatureCount+1];
    const LV2_Descriptor*     fDescriptor;
    const LV2_RDF_Descriptor* fRdfDescriptor;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float*  fParamBuffers;

    Lv2AtomQueue   fAtomQueueIn;
    Lv2AtomQueue   fAtomQueueOut;
    LV2_Atom_Forge fAtomForge;

    PluginCVData fCvIn;
    PluginCVData fCvOut;
    Lv2PluginEventData fEventsIn;
    Lv2PluginEventData fEventsOut;
    Lv2PluginOptions   fLv2Options;

    List<const char*> fCustomURIDs;

    bool fFirstActive; // first process() call after activate()
    EngineTimeInfo fLastTimeInfo;

    struct Extensions {
        const LV2_Options_Interface* options;
        const LV2_State_Interface* state;
        const LV2_Worker_Interface* worker;
        const LV2_Programs_Interface* programs;
        const LV2UI_Idle_Interface* uiidle;
        const LV2_Programs_UI_Interface* uiprograms;

        Extensions()
            : options(nullptr),
              state(nullptr),
              worker(nullptr),
              programs(nullptr),
              uiidle(nullptr),
              uiprograms(nullptr) {}
    } fExt;

    struct UI {
        Lv2PluginGuiType type;
        LV2UI_Handle handle;
        LV2UI_Widget widget;
        const LV2UI_Descriptor* descriptor;
        const LV2_RDF_UI*       rdfDescriptor;

        UI()
            : type(PLUGIN_UI_NULL),
              handle(nullptr),
              widget(nullptr),
              descriptor(nullptr),
              rdfDescriptor(nullptr) {}

        ~UI()
        {
            CARLA_ASSERT(handle == nullptr);
            CARLA_ASSERT(widget == nullptr);
            CARLA_ASSERT(descriptor == nullptr);
            CARLA_ASSERT(rdfDescriptor == nullptr);
        }

    } fUi;

    // -------------------------------------------------------------------
    // Event Feature

    static uint32_t carla_lv2_event_ref(LV2_Event_Callback_Data callback_data, LV2_Event* event)
    {
        carla_debug("carla_lv2_event_ref(%p, %p)", callback_data, event);
        CARLA_ASSERT(callback_data != nullptr);
        CARLA_ASSERT(event != nullptr);

        return 0;
    }

    static uint32_t carla_lv2_event_unref(LV2_Event_Callback_Data callback_data, LV2_Event* event)
    {
        carla_debug("carla_lv2_event_unref(%p, %p)", callback_data, event);
        CARLA_ASSERT(callback_data != nullptr);
        CARLA_ASSERT(event != nullptr);

        return 0;
    }

    // -------------------------------------------------------------------
    // Logs Feature

    static int carla_lv2_log_printf(LV2_Log_Handle handle, LV2_URID type, const char* fmt, ...)
    {
        CARLA_ASSERT(handle != nullptr);
        CARLA_ASSERT(type > CARLA_URI_MAP_ID_NULL);

#ifndef DEBUG
        if (type == CARLA_URI_MAP_ID_LOG_TRACE)
            return 0;
#endif

        va_list args;
        va_start(args, fmt);
        const int ret = carla_lv2_log_vprintf(handle, type, fmt, args);
        va_end(args);

        return ret;
    }

    static int carla_lv2_log_vprintf(LV2_Log_Handle handle, LV2_URID type, const char* fmt, va_list ap)
    {
        CARLA_ASSERT(handle != nullptr);
        CARLA_ASSERT(type > CARLA_URI_MAP_ID_NULL);

#ifndef DEBUG
        if (type == CARLA_URI_MAP_ID_LOG_TRACE)
            return 0;
#endif

        int ret = 0;

        switch (type)
        {
        case CARLA_URI_MAP_ID_LOG_ERROR:
#ifndef CARLA_OS_WIN
            std::fprintf(stderr, "\x1b[31m");
#endif
            ret = std::vfprintf(stderr, fmt, ap);
#ifndef CARLA_OS_WIN
            std::fprintf(stderr, "\x1b[0m");
#endif
            break;

        case CARLA_URI_MAP_ID_LOG_NOTE:
            ret = std::vfprintf(stdout, fmt, ap);
            break;

        case CARLA_URI_MAP_ID_LOG_TRACE:
#ifdef DEBUG
# ifndef CARLA_OS_WIN
            std::fprintf(stdout, "\x1b[30;1m");
# endif
            ret = std::vfprintf(stdout, fmt, ap);
# ifndef CARLA_OS_WIN
            std::fprintf(stdout, "\x1b[0m");
# endif
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
        carla_debug("carla_lv2_program_changed(%p, %i)", handle, index);
        CARLA_ASSERT(handle != nullptr);

        if (handle == nullptr)
            return;

        ((Lv2Plugin*)handle)->handleProgramChanged(index);
    }

    // -------------------------------------------------------------------
    // State Feature

    static char* carla_lv2_state_make_path(LV2_State_Make_Path_Handle handle, const char* path)
    {
        carla_debug("carla_lv2_state_make_path(%p, \"%s\")", handle, path);
        CARLA_ASSERT(handle != nullptr);
        CARLA_ASSERT(path != nullptr);

        if (path == nullptr)
            return nullptr;

        //QDir dir;
        //dir.mkpath(path);
        //return strdup(path);
        return nullptr;
    }

    static char* carla_lv2_state_map_abstract_path(LV2_State_Map_Path_Handle handle, const char* absolute_path)
    {
        carla_debug("carla_lv2_state_map_abstract_path(%p, \"%s\")", handle, absolute_path);
        CARLA_ASSERT(handle != nullptr);
        CARLA_ASSERT(absolute_path != nullptr);

        if (absolute_path == nullptr)
            return nullptr;

        //QDir dir(absolute_path);
        //return strdup(dir.canonicalPath().toUtf8().constData());
        return nullptr;
    }

    static char* carla_lv2_state_map_absolute_path(LV2_State_Map_Path_Handle handle, const char* abstract_path)
    {
        carla_debug("carla_lv2_state_map_absolute_path(%p, \"%s\")", handle, abstract_path);
        CARLA_ASSERT(handle != nullptr);
        CARLA_ASSERT(abstract_path != nullptr);

        if (abstract_path == nullptr)
            return nullptr;

        //QDir dir(abstract_path);
        //return strdup(dir.absolutePath().toUtf8().constData());
        return nullptr;
    }

    static LV2_State_Status carla_lv2_state_store(LV2_State_Handle handle, uint32_t key, const void* value, size_t size, uint32_t type, uint32_t flags)
    {
        carla_debug("carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i)", handle, key, value, size, type, flags);
        CARLA_ASSERT(handle != nullptr);

        if (handle == nullptr)
            return LV2_STATE_ERR_UNKNOWN;

        return ((Lv2Plugin*)handle)->handleStateStore(key, value, size, type, flags);
    }

    static const void* carla_lv2_state_retrieve(LV2_State_Handle handle, uint32_t key, size_t* size, uint32_t* type, uint32_t* flags)
    {
        carla_debug("carla_lv2_state_retrieve(%p, %i, %p, %p, %p)", handle, key, size, type, flags);
        CARLA_ASSERT(handle != nullptr);

        if (handle == nullptr)
            return nullptr;

        return ((Lv2Plugin*)handle)->handleStateRetrieve(key, size, type, flags);
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
        CARLA_ASSERT(handle != nullptr);
        CARLA_ASSERT(uri != nullptr);
        carla_debug("carla_lv2_urid_map(%p, \"%s\")", handle, uri);

        if (uri == nullptr)
            return CARLA_URI_MAP_ID_NULL;

        // Atom types
        if (std::strcmp(uri, LV2_ATOM__Blank) == 0)
            return CARLA_URI_MAP_ID_ATOM_BLANK;
        if (std::strcmp(uri, LV2_ATOM__Bool) == 0)
            return CARLA_URI_MAP_ID_ATOM_BOOL;
        if (std::strcmp(uri, LV2_ATOM__Chunk) == 0)
            return CARLA_URI_MAP_ID_ATOM_CHUNK;
        if (std::strcmp(uri, LV2_ATOM__Double) == 0)
            return CARLA_URI_MAP_ID_ATOM_DOUBLE;
        if (std::strcmp(uri, LV2_ATOM__Float) == 0)
            return CARLA_URI_MAP_ID_ATOM_FLOAT;
        if (std::strcmp(uri, LV2_ATOM__Int) == 0)
            return CARLA_URI_MAP_ID_ATOM_INT;
        if (std::strcmp(uri, LV2_ATOM__Literal) == 0)
            return CARLA_URI_MAP_ID_ATOM_LITERAL;
        if (std::strcmp(uri, LV2_ATOM__Long) == 0)
            return CARLA_URI_MAP_ID_ATOM_LONG;
        if (std::strcmp(uri, LV2_ATOM__Path) == 0)
            return CARLA_URI_MAP_ID_ATOM_PATH;
        if (std::strcmp(uri, LV2_ATOM__Property) == 0)
            return CARLA_URI_MAP_ID_ATOM_PROPERTY;
        if (std::strcmp(uri, LV2_ATOM__Resource) == 0)
            return CARLA_URI_MAP_ID_ATOM_RESOURCE;
        if (std::strcmp(uri, LV2_ATOM__Sequence) == 0)
            return CARLA_URI_MAP_ID_ATOM_SEQUENCE;
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

        // Others
        if (std::strcmp(uri, LV2_MIDI__MidiEvent) == 0)
            return CARLA_URI_MAP_ID_MIDI_EVENT;
        if (std::strcmp(uri, LV2_PARAMETERS__sampleRate) == 0)
            return CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE;

        if (handle == nullptr)
            return CARLA_URI_MAP_ID_NULL;

        // Custom types
        return ((Lv2Plugin*)handle)->getCustomURID(uri);
    }

    static const char* carla_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
    {
        carla_debug("carla_lv2_urid_unmap(%p, %i)", handle, urid);
        CARLA_ASSERT(handle != nullptr);
        CARLA_ASSERT(urid > CARLA_URI_MAP_ID_NULL);

        if (urid == CARLA_URI_MAP_ID_NULL)
            return nullptr;

        // Atom types
        if (urid == CARLA_URI_MAP_ID_ATOM_BLANK)
            return LV2_ATOM__Blank;
        if (urid == CARLA_URI_MAP_ID_ATOM_BOOL)
            return LV2_ATOM__Bool;
        if (urid == CARLA_URI_MAP_ID_ATOM_CHUNK)
            return LV2_ATOM__Chunk;
        if (urid == CARLA_URI_MAP_ID_ATOM_DOUBLE)
            return LV2_ATOM__Double;
        if (urid == CARLA_URI_MAP_ID_ATOM_FLOAT)
            return LV2_ATOM__Float;
        if (urid == CARLA_URI_MAP_ID_ATOM_INT)
            return LV2_ATOM__Int;
        if (urid == CARLA_URI_MAP_ID_ATOM_LITERAL)
            return LV2_ATOM__Literal;
        if (urid == CARLA_URI_MAP_ID_ATOM_LONG)
            return LV2_ATOM__Long;
        if (urid == CARLA_URI_MAP_ID_ATOM_PATH)
            return LV2_ATOM__Path;
        if (urid == CARLA_URI_MAP_ID_ATOM_PROPERTY)
            return LV2_ATOM__Property;
        if (urid == CARLA_URI_MAP_ID_ATOM_RESOURCE)
            return LV2_ATOM__Resource;
        if (urid == CARLA_URI_MAP_ID_ATOM_SEQUENCE)
            return LV2_ATOM__Sequence;
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

        // Others
        if (urid == CARLA_URI_MAP_ID_MIDI_EVENT)
            return LV2_MIDI__MidiEvent;
        if (urid == CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE)
            return LV2_PARAMETERS__sampleRate;

        if (handle == nullptr)
            return nullptr;

        // Custom types
        return ((Lv2Plugin*)handle)->getCustomURIString(urid);
    }

    // -------------------------------------------------------------------
    // Worker Feature

    static LV2_Worker_Status carla_lv2_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void* data)
    {
        CARLA_ASSERT(handle != nullptr);
        carla_debug("carla_lv2_worker_schedule(%p, %i, %p)", handle, size, data);

        if (handle == nullptr)
            return LV2_WORKER_ERR_UNKNOWN;

        return ((Lv2Plugin*)handle)->handleWorkerSchedule(size, data);
    }

    static LV2_Worker_Status carla_lv2_worker_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
    {
        CARLA_ASSERT(handle != nullptr);
        carla_debug("carla_lv2_worker_respond(%p, %i, %p)", handle, size, data);

        if (handle == nullptr)
            return LV2_WORKER_ERR_UNKNOWN;

        return ((Lv2Plugin*)handle)->handleWorkerRespond(size, data);
    }

    // -------------------------------------------------------------------
    // UI Port-Map Feature

    static uint32_t carla_lv2_ui_port_map(LV2UI_Feature_Handle handle, const char* symbol)
    {
        carla_debug("carla_lv2_ui_port_map(%p, \"%s\")", handle, symbol);
        CARLA_ASSERT(handle);

        if (handle == nullptr)
            return LV2UI_INVALID_PORT_INDEX;

        return ((Lv2Plugin*)handle)->handleUiPortMap(symbol);
    }

    // -------------------------------------------------------------------
    // UI Resize Feature

    static int carla_lv2_ui_resize(LV2UI_Feature_Handle handle, int width, int height)
    {
        carla_debug("carla_lv2_ui_resize(%p, %i, %i)", handle, width, height);
        CARLA_ASSERT(handle != nullptr);

        if (handle == nullptr)
            return 1;

        return ((Lv2Plugin*)handle)->handleUiResize(width, height);
    }

    // -------------------------------------------------------------------
    // External UI Feature

    static void carla_lv2_external_ui_closed(LV2UI_Controller controller)
    {
        carla_debug("carla_lv2_external_ui_closed(%p)", controller);
        CARLA_ASSERT(controller != nullptr);

        if (controller == nullptr)
            return;

        ((Lv2Plugin*)controller)->handleExternalUiClosed();
    }

    // -------------------------------------------------------------------
    // UI Extension

    static void carla_lv2_ui_write_function(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
    {
        CARLA_ASSERT(controller != nullptr);
        carla_debug("carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);

        if (controller == nullptr)
            return;

        ((Lv2Plugin*)controller)->handleUiWrite(port_index, buffer_size, format, buffer);
    }

    // -------------------------------------------------------------------
    // Lilv State

    static void carla_lilv_set_port_value(const char* port_symbol, void* user_data, const void* value, uint32_t size, uint32_t type)
    {
        CARLA_ASSERT(user_data != nullptr);
        carla_debug("carla_lilv_set_port_value(\"%s\", %p, %p, %i, %i)", port_symbol, user_data, value, size, type);

        if (user_data == nullptr)
            return;

        ((Lv2Plugin*)user_data)->handleLilvSetPortValue(port_symbol, value, size, type);
    }

    // -------------------------------------------------------------------

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Lv2Plugin)
};

// -------------------------------------------------------------------------------------------------------------------

#define lv2PluginPtr ((Lv2Plugin*)plugin)

int CarlaEngineOsc::handleMsgLv2AtomTransfer(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "is");
    carla_debug("CarlaOsc::handleMsgLv2AtomTransfer()");

    const int32_t portIndex   = argv[0]->i;
    const char* const atomBuf = (const char*)&argv[1]->s;

    if (portIndex < 0)
        return 0;

//     QByteArray chunk;
//     chunk = QByteArray::fromBase64(atomBuf);
//
//     LV2_Atom* const atom = (LV2_Atom*)chunk.data();
//     lv2PluginPtr->handleTransferAtom(portIndex, atom);
    return 0;
}

int CarlaEngineOsc::handleMsgLv2UridMap(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "is");
    carla_debug("CarlaOsc::handleMsgLv2EventTransfer()");

    const int32_t urid    = argv[0]->i;
    const char* const uri = (const char*)&argv[1]->s;

    if (urid <= 0)
        return 0;

    lv2PluginPtr->handleUridMap(urid, uri);
    return 0;
}

#undef lv2PluginPtr

CARLA_BACKEND_END_NAMESPACE

#else // WANT_LV2
# warning Building without LV2 support
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newLV2(const Initializer& init)
{
    carla_debug("CarlaPlugin::newLV2({%p, \"%s\", \"%s\"})", init.engine, init.name, init.label);

#ifdef WANT_LV2
    Lv2Plugin* const plugin(new Lv2Plugin(init.engine, init.id));

    if (! plugin->init(init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && ! plugin->canRunInRack())
    {
        init.engine->setLastError("Carla's rack mode can only work with Mono or Stereo LV2 plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("LV2 support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
