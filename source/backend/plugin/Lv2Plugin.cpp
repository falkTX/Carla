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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaPluginInternal.hpp"

#ifdef WANT_LV2

#include "CarlaPluginGui.hpp"
#include "CarlaLv2Utils.hpp"

#include <QtCore/QDir>

extern "C" {
#include "rtmempool/rtmempool-lv2.h"
}

// -----------------------------------------------------
// Our LV2 World class object

Lv2WorldClass gLv2World;

// -----------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
}
#endif

/*!
 * @defgroup PluginHints Plugin Hints
 * @{
 */
const unsigned int PLUGIN_HAS_EXTENSION_OPTIONS  = 0x100; //!< LV2 Plugin has Options extension
const unsigned int PLUGIN_HAS_EXTENSION_PROGRAMS = 0x200; //!< LV2 Plugin has Programs extension
const unsigned int PLUGIN_HAS_EXTENSION_STATE    = 0x400; //!< LV2 Plugin has State extension
const unsigned int PLUGIN_HAS_EXTENSION_WORKER   = 0x800; //!< LV2 Plugin has Worker extension
/**@}*/

/*!
 * @defgroup ParameterHints Parameter Hints
 * @{
 */
const unsigned int PARAMETER_IS_STRICT_BOUNDS = 0x1000; //!< LV2 Parameter needs strict bounds
const unsigned int PARAMETER_IS_TRIGGER       = 0x2000; //!< LV2 Parameter is trigger (current value should be changed to its default after run())
/**@}*/

/*!
 * @defgroup Lv2EventTypes LV2 Event Data/Types
 *
 * Data and buffer types for LV2 EventData ports.
 * @{
 */
const unsigned int CARLA_EVENT_DATA_ATOM    = 0x01;
const unsigned int CARLA_EVENT_DATA_EVENT   = 0x02;
const unsigned int CARLA_EVENT_DATA_MIDI_LL = 0x04;
const unsigned int CARLA_EVENT_TYPE_MESSAGE = 0x10;
const unsigned int CARLA_EVENT_TYPE_MIDI    = 0x20;
const unsigned int CARLA_EVENT_TYPE_TIME    = 0x40;
/**@}*/

/*!
 * @defgroup Lv2UriMapIds LV2 URI Map Ids
 *
 * Static index list of the internal LV2 URI Map Ids.
 * @{
 */
const uint32_t CARLA_URI_MAP_ID_NULL                = 0;
const uint32_t CARLA_URI_MAP_ID_ATOM_CHUNK          = 1;
const uint32_t CARLA_URI_MAP_ID_ATOM_DOUBLE         = 2;
const uint32_t CARLA_URI_MAP_ID_ATOM_INT            = 3;
const uint32_t CARLA_URI_MAP_ID_ATOM_PATH           = 4;
const uint32_t CARLA_URI_MAP_ID_ATOM_SEQUENCE       = 5;
const uint32_t CARLA_URI_MAP_ID_ATOM_STRING         = 6;
const uint32_t CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM  = 7;
const uint32_t CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT = 8;
const uint32_t CARLA_URI_MAP_ID_BUF_MAX_LENGTH      = 9;
const uint32_t CARLA_URI_MAP_ID_BUF_MIN_LENGTH      = 10;
const uint32_t CARLA_URI_MAP_ID_BUF_SEQUENCE_SIZE   = 11;
const uint32_t CARLA_URI_MAP_ID_LOG_ERROR           = 12;
const uint32_t CARLA_URI_MAP_ID_LOG_NOTE            = 13;
const uint32_t CARLA_URI_MAP_ID_LOG_TRACE           = 14;
const uint32_t CARLA_URI_MAP_ID_LOG_WARNING         = 15;
const uint32_t CARLA_URI_MAP_ID_MIDI_EVENT          = 16;
const uint32_t CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE   = 17;
const uint32_t CARLA_URI_MAP_ID_TIME_POSITION       = 18;
const uint32_t CARLA_URI_MAP_ID_COUNT               = 19;
/**@}*/

/*!
 * @defgroup Lv2FeatureIds LV2 Feature Ids
 *
 * Static index list of the internal LV2 Feature Ids.
 * @{
 */
const uint32_t kFeatureIdBufSizeBounded   =  0;
const uint32_t kFeatureIdBufSizeFixed     =  1;
const uint32_t kFeatureIdBufSizePowerOf2  =  2;
const uint32_t kFeatureIdEvent            =  3;
const uint32_t kFeatureIdLogs             =  4;
const uint32_t kFeatureIdOptions          =  5;
const uint32_t kFeatureIdPrograms         =  6;
const uint32_t kFeatureIdRtMemPool        =  7;
const uint32_t kFeatureIdStateMakePath    =  8;
const uint32_t kFeatureIdStateMapPath     =  9;
const uint32_t kFeatureIdStrictBounds     = 10;
const uint32_t kFeatureIdUriMap           = 11;
const uint32_t kFeatureIdUridMap          = 12;
const uint32_t kFeatureIdUridUnmap        = 13;
const uint32_t kFeatureIdWorker           = 14;
const uint32_t kFeatureIdUiDataAccess     = 15;
const uint32_t kFeatureIdUiInstanceAccess = 16;
const uint32_t kFeatureIdUiParent         = 17;
const uint32_t kFeatureIdUiPortMap        = 18;
const uint32_t kFeatureIdUiResize         = 19;
const uint32_t kFeatureIdExternalUi       = 20;
const uint32_t kFeatureIdExternalUiOld    = 21;
const uint32_t kFeatureCount              = 22;
/**@}*/

const unsigned int MAX_EVENT_BUFFER = 8192; // 0x2000

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
        LV2_Atom_Sequence* atom;
        LV2_Event_Buffer* event;
        LV2_MIDI* midi;
    };

    Lv2EventData()
        : type(0x0),
          rindex(0),
          port(nullptr) {}

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
                    delete[] midi->data;
                delete midi;

                midi->data = nullptr;
                midi = nullptr;
            }
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(Lv2EventData)
};

struct Lv2PluginEventData {
    uint32_t count;
    Lv2EventData* data;
    Lv2EventData* ctrl; // default port
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
        CARLA_ASSERT(ctrlIndex == 0);
    }

    void createNew(const uint32_t newCount)
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT(data == nullptr);
        CARLA_ASSERT(ctrl == nullptr);
        CARLA_ASSERT(ctrlIndex == 0);
        CARLA_ASSERT_INT(newCount > 0, newCount);

        if (data != nullptr || newCount == 0)
            return;

        data  = new Lv2EventData[newCount];
        count = newCount;

        ctrl      = nullptr;
        ctrlIndex = 0;
    }

    void clear()
    {
        if (data != nullptr)
        {
            delete[] data;
            data = nullptr;
        }

        count = 0;

        ctrl      = nullptr;
        ctrlIndex = 0;
    }

    void initBuffers(CarlaEngine* const engine)
    {
        for (uint32_t i=0; i < count; ++i)
        {
            if (data[i].port != nullptr)
                data[i].port->initBuffer(engine);
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(Lv2PluginEventData)
};

struct Lv2PluginOptions {
    int minBufferSize;
    int maxBufferSize;
    int sequenceSize;
    double sampleRate;
    LV2_Options_Option optMinBlockLenth;
    LV2_Options_Option optMaxBlockLenth;
    LV2_Options_Option optSequenceSize;
    LV2_Options_Option optSampleRate;
    LV2_Options_Option optNull;
    LV2_Options_Option* opts[5];

    Lv2PluginOptions()
        : minBufferSize(0),
          maxBufferSize(0),
          sequenceSize(MAX_EVENT_BUFFER),
          sampleRate(0.0)
    {
        optMinBlockLenth.context = LV2_OPTIONS_INSTANCE;
        optMinBlockLenth.subject = 0;
        optMinBlockLenth.key     = CARLA_URI_MAP_ID_BUF_MIN_LENGTH;
        optMinBlockLenth.size    = sizeof(int);
        optMinBlockLenth.type    = CARLA_URI_MAP_ID_ATOM_INT;
        optMinBlockLenth.value   = &minBufferSize;

        optMaxBlockLenth.context = LV2_OPTIONS_INSTANCE;
        optMaxBlockLenth.subject = 0;
        optMaxBlockLenth.key     = CARLA_URI_MAP_ID_BUF_MAX_LENGTH;
        optMaxBlockLenth.size    = sizeof(int);
        optMaxBlockLenth.type    = CARLA_URI_MAP_ID_ATOM_INT;
        optMaxBlockLenth.value   = &maxBufferSize;

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

        opts[0] = &optMinBlockLenth;
        opts[1] = &optMaxBlockLenth;
        opts[2] = &optSequenceSize;
        opts[3] = &optSampleRate;
        opts[4] = &optNull;
    }

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(Lv2PluginOptions)
};

LV2_Atom_Event* getLv2AtomEvent(LV2_Atom_Sequence* const atom, const uint32_t offset)
{
    return (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, atom) + offset);
}

// -----------------------------------------------------

class Lv2Plugin : public CarlaPlugin,
                  public CarlaPluginGui::Callback
{
public:
    Lv2Plugin(CarlaEngine* const engine, const unsigned int id)
        : CarlaPlugin(engine, id),
          fHandle(nullptr),
          fHandle2(nullptr),
          fFeatures{nullptr},
          fDescriptor(nullptr),
          fRdfDescriptor(nullptr),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fParamBuffers(nullptr)
    {
        carla_debug("Lv2Plugin::Lv2Plugin(%p, %i)", engine, id);

        kData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_LV2_GUI);

        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; ++i)
            fCustomURIDs.append(nullptr);
    }

    ~Lv2Plugin() override
    {
        carla_debug("Lv2Plugin::~Lv2Plugin()");

        // close UI
        if (fUi.type != PLUGIN_UI_NULL)
        {
            showGui(false);

            if (fUi.type == PLUGIN_UI_OSC)
            {
                // Wait a bit first, then force kill
                if (kData->osc.thread.isRunning() && ! kData->osc.thread.wait(kData->engine->getOptions().oscUiTimeout))
                {
                    carla_stderr("LV2 OSC-GUI thread still running, forcing termination now");
                    kData->osc.thread.terminate();
                }
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
                kData->uiLibClose();
            }

            fUi.rdfDescriptor = nullptr;
        }

        kData->singleMutex.lock();
        kData->masterMutex.lock();

        if (kData->active)
        {
            deactivate();
            kData->active = false;
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
                delete fFeatures[i];
        }

        for (auto it = fCustomURIDs.begin(); it.valid(); it.next())
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

    PluginType type() const override
    {
        return PLUGIN_LV2;
    }

    PluginCategory category() override
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

        return getPluginCategoryFromName(fName);
    }

    long uniqueId() const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        return fRdfDescriptor->UniqueID;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t midiInCount() const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        uint32_t i, count = 0;

        for (i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (LV2_IS_PORT_INPUT(portTypes) && LV2_PORT_SUPPORTS_MIDI_EVENT(portTypes))
                count += 1;
        }

        return count;
    }

    uint32_t midiOutCount() const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        uint32_t i, count = 0;

        for (i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (LV2_IS_PORT_OUTPUT(portTypes) && LV2_PORT_SUPPORTS_MIDI_EVENT(portTypes))
                count += 1;
        }

        return count;
    }

    uint32_t parameterScalePointCount(const uint32_t parameterId) const override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const int32_t rindex(kData->param.data[parameterId].rindex);

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

    unsigned int availableOptions() override
    {
        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (! needsFixedBuffer())
            options |= PLUGIN_OPTION_FIXED_BUFFER;

        if (kData->engine->getProccessMode() != PROCESS_MODE_CONTINUOUS_RACK)
        {
            if (fOptions & PLUGIN_OPTION_FORCE_STEREO)
                options |= PLUGIN_OPTION_FORCE_STEREO;
            else if (kData->audioIn.count <= 1 && kData->audioOut.count <= 1 && (kData->audioIn.count != 0 || kData->audioOut.count != 0))
                options |= PLUGIN_OPTION_FORCE_STEREO;
        }

        if (midiInCount() > 0)
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) override
    {
        CARLA_ASSERT(fParamBuffers != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        return fParamBuffers[parameterId];
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        const int32_t rindex(kData->param.data[parameterId].rindex);

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

    void getLabel(char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(fRdfDescriptor->URI != nullptr);

        if (fRdfDescriptor->URI != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->URI, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        if (fRdfDescriptor->Author != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Author, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        if (fRdfDescriptor->License != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->License, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        if (fRdfDescriptor->Name != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const int32_t rindex(kData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
            std::strncpy(strBuf, fRdfDescriptor->Ports[rindex].Name, STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterSymbol(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const int32_t rindex(kData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
            strncpy(strBuf, fRdfDescriptor->Ports[rindex].Symbol, STR_MAX);
        else
            CarlaPlugin::getParameterSymbol(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const int32_t rindex(kData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LV2_RDF_Port& port(fRdfDescriptor->Ports[rindex]);

            if (LV2_HAVE_PORT_UNIT_SYMBOL(port.Unit.Hints) && port.Unit.Symbol)
                std::strncpy(strBuf, port.Unit.Symbol, STR_MAX);

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

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) override
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        const int32_t rindex(kData->param.data[parameterId].rindex);

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

    // nothing

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        const float fixedValue(kData->param.fixValue(parameterId, value));
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

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(kData->midiprog.count));

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(kData->midiprog.count))
            return;

        if (index >= 0 && fExt.programs != nullptr && fExt.programs->select_program != nullptr)
        {
            const uint32_t bank    = kData->midiprog.data[index].bank;
            const uint32_t program = kData->midiprog.data[index].program;

            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            fExt.programs->select_program(fHandle, bank, program);

            if (fHandle2 != nullptr)
                fExt.programs->select_program(fHandle2, bank, program);
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo) override
    {
        if (fUi.type == PLUGIN_UI_NULL)
            return;

        if (fUi.type == PLUGIN_UI_OSC)
        {
            if (yesNo)
            {
                kData->osc.thread.start();
            }
            else
            {
                if (kData->osc.data.target != nullptr)
                {
                    osc_send_hide(&kData->osc.data);
                    osc_send_quit(&kData->osc.data);
                    kData->osc.data.free();
                }

                if (kData->osc.thread.isRunning() && ! kData->osc.thread.wait(kData->engine->getOptions().oscUiTimeout))
                    kData->osc.thread.terminate();
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
                fUi.widget = nullptr;

                if (fUi.handle == nullptr)
                    fUi.handle = fUi.descriptor->instantiate(fUi.descriptor, fRdfDescriptor->URI, fUi.rdfDescriptor->Bundle,
                                                             carla_lv2_ui_write_function, this, &fUi.widget, fFeatures);

                CARLA_ASSERT(fUi.handle != nullptr);
                CARLA_ASSERT(fUi.widget != nullptr);

                if (fUi.handle == nullptr || fUi.widget == nullptr)
                {
                    fUi.handle = nullptr;
                    fUi.widget = nullptr;
                    kData->engine->callback(CALLBACK_ERROR, fId, 0, 0, 0.0f, "Plugin refused to open its own UI");
                    kData->engine->callback(CALLBACK_SHOW_GUI, fId, 0, 0, 0.0f, nullptr);
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
            if (yesNo)
            {
                if (kData->gui == nullptr)
                {
                    // TODO
                    CarlaPluginGui::Options guiOptions;
                    guiOptions.parented  = (fUi.type == PLUGIN_UI_PARENT);
                    guiOptions.resizable = isUiResizable();

                    kData->gui = new CarlaPluginGui(kData->engine, this, guiOptions);
                }

                if (fUi.type == PLUGIN_UI_PARENT)
                {
                    fFeatures[kFeatureIdUiParent]->data = kData->gui->getContainerWinId();
                    fFeatures[kFeatureIdUiParent]->URI  = LV2_UI__parent;
                }

                fUi.widget = nullptr;

                if (fUi.handle == nullptr)
                    fUi.handle = fUi.descriptor->instantiate(fUi.descriptor, fRdfDescriptor->URI, fUi.rdfDescriptor->Bundle,
                                                             carla_lv2_ui_write_function, this, &fUi.widget, fFeatures);

                CARLA_ASSERT(fUi.handle != nullptr);
                CARLA_ASSERT(fUi.widget != nullptr);

                if (fUi.handle == nullptr || fUi.widget == nullptr)
                {
                    fUi.handle = nullptr;
                    fUi.widget = nullptr;

                    kData->gui->close();
                    delete kData->gui;
                    kData->gui = nullptr;

                    kData->engine->callback(CALLBACK_ERROR, fId, 0, 0, 0.0f, "Plugin refused to open its own UI");
                    kData->engine->callback(CALLBACK_SHOW_GUI, fId, -1, 0, 0.0f, nullptr);
                    return;
                }

                if (fUi.type == PLUGIN_UI_QT)
                    kData->gui->setWidget((QWidget*)fUi.widget);

                updateUi();

                kData->gui->setWindowTitle(QString("%1 (GUI)").arg((const char*)fName));
                kData->gui->show();
            }
            else
            {
                fUi.descriptor->cleanup(fUi.handle);
                fUi.handle = nullptr;
                fUi.widget = nullptr;

                if (kData->gui != nullptr)
                {
                    kData->gui->close();
                    delete kData->gui;
                    kData->gui = nullptr;
                }
            }
        }
    }

    void idleGui() override
    {
        if (fUi.handle != nullptr && fUi.descriptor != nullptr)
        {
            if (fUi.type == PLUGIN_UI_EXTERNAL && fUi.widget != nullptr)
                LV2_EXTERNAL_UI_RUN((LV2_External_UI_Widget*)fUi.widget);

            if (fExt.uiidle != nullptr)
            {
                if (fExt.uiidle->idle(fUi.handle) != 0)
                {
                    showGui(false);
                    kData->engine->callback(CALLBACK_SHOW_GUI, fId, 0, 0, 0.0f, nullptr);
                }
            }
        }

        CarlaPlugin::idleGui();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        carla_debug("Lv2Plugin::reload() - start");
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fRdfDescriptor != nullptr);

        if (kData->engine == nullptr)
            return;
        if (fHandle == nullptr)
            return;
        if (fDescriptor == nullptr)
            return;
        if (fRdfDescriptor == nullptr)
            return;

        const ProcessMode processMode(kData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (kData->active)
            deactivate();

        clearBuffers();

        const float   sampleRate = (float)kData->engine->getSampleRate();
        const uint32_t portCount = fRdfDescriptor->PortCount;

        uint32_t aIns, aOuts, cvIns, cvOuts, params, j;
        aIns = aOuts = cvIns = cvOuts = params = 0;
        NonRtList<uint32_t> evIns, evOuts;

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
            if (kData->extraHints & PLUGIN_HAS_EXTENSION_OPTIONS)
                fExt.options = (const LV2_Options_Interface*)fDescriptor->extension_data(LV2_OPTIONS__interface);

            if (kData->extraHints & PLUGIN_HAS_EXTENSION_PROGRAMS)
                fExt.programs = (const LV2_Programs_Interface*)fDescriptor->extension_data(LV2_PROGRAMS__Interface);

            if (kData->extraHints & PLUGIN_HAS_EXTENSION_STATE)
                fExt.state = (const LV2_State_Interface*)fDescriptor->extension_data(LV2_STATE__interface);

            if (kData->extraHints & PLUGIN_HAS_EXTENSION_WORKER)
                fExt.worker = (const LV2_Worker_Interface*)fDescriptor->extension_data(LV2_WORKER__interface);
        }

        if ((fOptions & PLUGIN_OPTION_FORCE_STEREO) != 0 && (aIns == 1 || aOuts == 1) && fExt.state == nullptr && fExt.worker == nullptr)
        {
            if (fHandle2 == nullptr)
                fHandle2 = fDescriptor->instantiate(fDescriptor, sampleRate, fRdfDescriptor->Bundle, fFeatures);

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

        if (aIns > 0)
        {
            kData->audioIn.createNew(aIns);
            fAudioInBuffers = new float*[aIns];

            for (uint32_t i=0; i < aIns; ++i)
                fAudioInBuffers[i] = nullptr;
        }

        if (aOuts > 0)
        {
            kData->audioOut.createNew(aOuts);
            fAudioOutBuffers = new float*[aOuts];
            needsCtrlIn = true;

            for (uint32_t i=0; i < aOuts; ++i)
                fAudioOutBuffers[i] = nullptr;
        }

        if (params > 0)
        {
            kData->param.createNew(params);
            fParamBuffers = new float[params];

            for (uint32_t i=0; i < params; ++i)
                fParamBuffers[i] = 0.0f;
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
                    fEventsIn.data[j].atom = (LV2_Atom_Sequence*)std::malloc(sizeof(LV2_Atom_Sequence) + MAX_EVENT_BUFFER);
                    fEventsIn.data[j].atom->atom.size = 0;
                    fEventsIn.data[j].atom->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                    fEventsIn.data[j].atom->body.unit = CARLA_URI_MAP_ID_NULL;
                    fEventsIn.data[j].atom->body.pad  = 0;
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
                    fEventsIn.data[j].midi->capacity = MAX_EVENT_BUFFER;
                    fEventsIn.data[j].midi->data     = new unsigned char[MAX_EVENT_BUFFER];
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
                    fEventsOut.data[j].atom = (LV2_Atom_Sequence*)std::malloc(sizeof(LV2_Atom_Sequence) + MAX_EVENT_BUFFER);
                    fEventsOut.data[j].atom->atom.size = 0;
                    fEventsOut.data[j].atom->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                    fEventsOut.data[j].atom->body.unit = CARLA_URI_MAP_ID_NULL;
                    fEventsOut.data[j].atom->body.pad  = 0;
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
                    fEventsOut.data[j].midi->capacity = MAX_EVENT_BUFFER;
                    fEventsOut.data[j].midi->data     = new unsigned char[MAX_EVENT_BUFFER];
                }
            }
        }

        const uint portNameSize(kData->engine->maxPortNameSize());
        CarlaString portName;

        for (uint32_t i=0, iAudioIn=0, iAudioOut=0, iEvIn=0, iEvOut=0, iCtrl=0; i < portCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            portName.clear();

            if (LV2_IS_PORT_AUDIO(portTypes) || LV2_IS_PORT_ATOM_SEQUENCE(portTypes) || LV2_IS_PORT_CV(portTypes) || LV2_IS_PORT_EVENT(portTypes) || LV2_IS_PORT_MIDI_LL(portTypes))
            {
                if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = fName;
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
                    kData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, true);
                    kData->audioIn.ports[j].rindex = i;

                    if (forcedStereoIn)
                    {
                        portName += "_2";
                        kData->audioIn.ports[1].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, true);
                        kData->audioIn.ports[1].rindex = i;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    j = iAudioOut++;
                    kData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
                    kData->audioOut.ports[j].rindex = i;

                    if (forcedStereoOut)
                    {
                        portName += "_2";
                        kData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
                        kData->audioOut.ports[1].rindex = i;
                    }
                }
                else
                    carla_stderr("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LV2_IS_PORT_CV(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    carla_stderr("WARNING - CV Ports are not supported yet");
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    carla_stderr("WARNING - CV Ports are not supported yet");
                }
                else
                    carla_stderr("WARNING - Got a broken Port (CV, but not input or output)");

                fDescriptor->connect_port(fHandle, i, nullptr);

                if (fHandle2 != nullptr)
                    fDescriptor->connect_port(fHandle2, i, nullptr);
            }
            else if (LV2_IS_PORT_ATOM_SEQUENCE(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    j = iEvIn++;

                    fDescriptor->connect_port(fHandle, i, fEventsIn.data[j].atom);

                    if (fHandle2 != nullptr)
                        fDescriptor->connect_port(fHandle2, i, fEventsIn.data[j].atom);

                    fEventsIn.data[j].rindex = i;

                    if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                    {
                        fEventsIn.data[j].type |= CARLA_EVENT_TYPE_MIDI;

                        if (evIns.count() == 1)
                        {
                            needsCtrlIn = true;
                            fEventsIn.ctrl = &fEventsIn.data[j];
                            fEventsIn.ctrlIndex = j;
                        }
                        else
                        {
                            fEventsIn.data[j].port = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, true);

                            if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                            {
                                fEventsIn.ctrl = &fEventsIn.data[j];
                                fEventsIn.ctrlIndex = j;
                            }
                        }
                    }
                    if (portTypes & LV2_PORT_DATA_PATCH_MESSAGE)
                    {
                        fEventsIn.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    }
                    if (portTypes & LV2_PORT_DATA_TIME_POSITION)
                    {
                        fEventsIn.data[j].type |= CARLA_EVENT_TYPE_TIME;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    j = iEvOut++;

                    fDescriptor->connect_port(fHandle, i, fEventsOut.data[j].atom);

                    if (fHandle2 != nullptr)
                        fDescriptor->connect_port(fHandle2, i, fEventsOut.data[j].atom);

                    fEventsOut.data[j].rindex = i;

                    if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                    {
                        fEventsOut.data[j].type |= CARLA_EVENT_TYPE_MIDI;

                        if (evOuts.count() == 1)
                        {
                            needsCtrlOut = true;
                            fEventsOut.ctrl = &fEventsOut.data[j];
                            fEventsOut.ctrlIndex = j;
                        }
                        else
                        {
                            fEventsOut.data[j].port = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, false);

                            if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                            {
                                fEventsOut.ctrl = &fEventsOut.data[j];
                                fEventsOut.ctrlIndex = j;
                            }
                        }
                    }
                    if (portTypes & LV2_PORT_DATA_PATCH_MESSAGE)
                    {
                        fEventsOut.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    }
                    if (portTypes & LV2_PORT_DATA_TIME_POSITION)
                    {
                        fEventsOut.data[j].type |= CARLA_EVENT_TYPE_TIME;
                    }
                }
                else
                    carla_stderr("WARNING - Got a broken Port (Atom Sequence, but not input or output)");
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
                    {
                        fEventsIn.data[j].type |= CARLA_EVENT_TYPE_MIDI;

                        if (evIns.count() == 1)
                        {
                            needsCtrlIn = true;
                            fEventsIn.ctrl = &fEventsIn.data[j];
                            fEventsIn.ctrlIndex = j;
                        }
                        else
                        {
                            fEventsIn.data[j].port = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, true);

                            if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                            {
                                fEventsIn.ctrl = &fEventsIn.data[j];
                                fEventsIn.ctrlIndex = j;
                            }
                        }
                    }
                    if (portTypes & LV2_PORT_DATA_PATCH_MESSAGE)
                    {
                        fEventsIn.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    }
                    if (portTypes & LV2_PORT_DATA_TIME_POSITION)
                    {
                        fEventsIn.data[j].type |= CARLA_EVENT_TYPE_TIME;
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
                    {
                        fEventsOut.data[j].type |= CARLA_EVENT_TYPE_MIDI;

                        if (evOuts.count() == 1)
                        {
                            needsCtrlOut = true;
                            fEventsOut.ctrl = &fEventsOut.data[j];
                            fEventsOut.ctrlIndex = j;
                        }
                        else
                        {
                            fEventsOut.data[j].port  = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, false);

                            if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                            {
                                fEventsOut.ctrl = &fEventsOut.data[j];
                                fEventsOut.ctrlIndex = j;
                            }
                        }
                    }
                    if (portTypes & LV2_PORT_DATA_PATCH_MESSAGE)
                    {
                        fEventsOut.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    }
                    if (portTypes & LV2_PORT_DATA_TIME_POSITION)
                    {
                        fEventsOut.data[j].type |= CARLA_EVENT_TYPE_TIME;
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
                        fEventsIn.data[j].port = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, true);

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
                        fEventsOut.data[j].port = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, false);

                        if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
                        {
                            fEventsOut.ctrl = &fEventsOut.data[j];
                            fEventsOut.ctrlIndex = j;
                        }
                    }
                }
                else
                    carla_stderr("WARNING - Got a broken Port (Midi, but not input or output)");
            }
            else if (LV2_IS_PORT_CONTROL(portTypes))
            {
                const LV2_Property portProps(fRdfDescriptor->Ports[i].Properties);
                const LV2_Property portDesignation(fRdfDescriptor->Ports[i].Designation);
                const LV2_RDF_PortPoints portPoints(fRdfDescriptor->Ports[i].Points);

                j = iCtrl++;
                kData->param.data[j].index  = j;
                kData->param.data[j].rindex = i;
                kData->param.data[j].hints  = 0x0;
                kData->param.data[j].midiChannel = 0;
                kData->param.data[j].midiCC = -1;

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
                    kData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LV2_IS_PORT_TOGGLED(portProps))
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    kData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LV2_IS_PORT_INTEGER(portProps))
                {
                    step = 1.0f;
                    stepSmall = 1.0f;
                    stepLarge = 10.0f;
                    kData->param.data[j].hints |= PARAMETER_IS_INTEGER;
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

                        kData->param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        kData->param.data[j].hints = 0x0;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_FREEWHEELING(portDesignation))
                    {
                        kData->param.data[j].type = PARAMETER_LV2_FREEWHEEL;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_TIME(portDesignation))
                    {
                        kData->param.data[j].type = PARAMETER_LV2_TIME;
                    }
                    else
                    {
                        kData->param.data[j].type = PARAMETER_INPUT;
                        kData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        kData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlIn = true;
                    }

                    // MIDI CC value
                    const LV2_RDF_PortMidiMap& portMidiMap(fRdfDescriptor->Ports[i].MidiMap);
                    if (LV2_IS_PORT_MIDI_MAP_CC(portMidiMap.Type))
                    {
                        if (! MIDI_IS_CONTROL_BANK_SELECT(portMidiMap.Number))
                            kData->param.data[j].midiCC = portMidiMap.Number;
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

                        kData->param.data[j].type  = PARAMETER_LATENCY;
                        kData->param.data[j].hints = 0;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_SAMPLE_RATE(portDesignation))
                    {
                        def = sampleRate;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        kData->param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        kData->param.data[j].hints = 0;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_FREEWHEELING(portDesignation))
                    {
                        carla_stderr("Plugin has freewheeling output port, this should not happen!");
                    }
                    else if (LV2_IS_PORT_DESIGNATION_TIME(portDesignation))
                    {
                        kData->param.data[j].type = PARAMETER_LV2_TIME;
                    }
                    else
                    {
                        kData->param.data[j].type   = PARAMETER_OUTPUT;
                        kData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        kData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlOut = true;
                    }
                }
                else
                {
                    kData->param.data[j].type = PARAMETER_UNKNOWN;
                    carla_stderr2("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LV2_IS_PORT_ENUMERATION(portProps))
                    kData->param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                if (LV2_IS_PORT_LOGARITHMIC(portProps))
                    kData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                if (LV2_IS_PORT_TRIGGER(portProps))
                    kData->param.data[j].hints |= PARAMETER_IS_TRIGGER;

                if (LV2_IS_PORT_STRICT_BOUNDS(portProps))
                    kData->param.data[j].hints |= PARAMETER_IS_STRICT_BOUNDS;

                // check if parameter is not enabled or automable
                if (LV2_IS_PORT_NOT_ON_GUI(portProps))
                    kData->param.data[j].hints &= ~PARAMETER_IS_ENABLED;

                if (LV2_IS_PORT_CAUSES_ARTIFACTS(portProps) || LV2_IS_PORT_EXPENSIVE(portProps) || LV2_IS_PORT_NOT_AUTOMATIC(portProps))
                    kData->param.data[j].hints &= ~PARAMETER_IS_AUTOMABLE;

                kData->param.ranges[j].min = min;
                kData->param.ranges[j].max = max;
                kData->param.ranges[j].def = def;
                kData->param.ranges[j].step = step;
                kData->param.ranges[j].stepSmall = stepSmall;
                kData->param.ranges[j].stepLarge = stepLarge;

                // Start parameters in their default values
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

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "events-in";
            portName.truncate(portNameSize);

            kData->event.portIn = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "events-out";
            portName.truncate(portNameSize);

            kData->event.portOut = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        if (fEventsIn.ctrl != nullptr && fEventsIn.ctrl->port == nullptr)
            fEventsIn.ctrl->port = kData->event.portIn;

        if (fEventsOut.ctrl != nullptr && fEventsOut.ctrl->port == nullptr)
            fEventsOut.ctrl->port = kData->event.portOut;

        if (forcedStereoIn || forcedStereoOut)
            fOptions |= PLUGIN_OPTION_FORCE_STEREO;
        else
            fOptions &= ~PLUGIN_OPTION_FORCE_STEREO;

        // plugin hints
        fHints = 0x0;

        if (fUi.type != PLUGIN_UI_NULL)
        {
            fHints |= PLUGIN_HAS_GUI;

            if (fUi.type == PLUGIN_UI_QT || fUi.type == PLUGIN_UI_PARENT)
                fHints |= PLUGIN_HAS_SINGLE_THREAD;
        }

        if (LV2_IS_GENERATOR(fRdfDescriptor->Type[0], fRdfDescriptor->Type[1]))
            fHints |= PLUGIN_IS_SYNTH;

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            fHints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            fHints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            fHints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        kData->extraHints &= ~PLUGIN_HINT_CAN_RUN_RACK;

        if (fExt.state != nullptr || fExt.worker != nullptr)
        {
            if ((aIns == 0 || aIns == 2) && (aOuts == 0 || aOuts == 2) && evIns.count() <= 1 && evOuts.count() <= 1)
                kData->extraHints |= PLUGIN_HINT_CAN_RUN_RACK;
        }
        else
        {
            if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0) && evIns.count() <= 1 && evOuts.count() <= 1)
                kData->extraHints |= PLUGIN_HINT_CAN_RUN_RACK;
        }

        bufferSizeChanged(kData->engine->getBufferSize());
        reloadPrograms(true);

        if (kData->active)
            activate();

        evIns.clear();
        evOuts.clear();

        carla_debug("Lv2Plugin::reload() - end");
    }

    void reloadPrograms(const bool init) override
    {
        carla_debug("DssiPlugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount  = kData->midiprog.count;
        const int32_t current = kData->midiprog.current;

        // Delete old programs
        kData->midiprog.clear();

        // Query new programs
        uint32_t count = 0;
        if (fExt.programs != nullptr && fExt.programs->get_program != nullptr && fExt.programs->select_program != nullptr)
        {
            while (fExt.programs->get_program(fHandle, count))
                count++;
        }

        if (count > 0)
        {
            kData->midiprog.createNew(count);

            // Update data
            for (i=0; i < count; ++i)
            {
                const LV2_Program_Descriptor* const pdesc(fExt.programs->get_program(fHandle, i));
                CARLA_ASSERT(pdesc != nullptr);
                CARLA_ASSERT(pdesc->name != nullptr);

                kData->midiprog.data[i].bank    = static_cast<uint32_t>(pdesc->bank);
                kData->midiprog.data[i].program = static_cast<uint32_t>(pdesc->program);
                kData->midiprog.data[i].name    = carla_strdup(pdesc->name);
            }
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (kData->engine->isOscControlRegistered())
        {
            kData->engine->osc_send_control_set_midi_program_count(fId, count);

            for (i=0; i < count; ++i)
                kData->engine->osc_send_control_set_midi_program_data(fId, i, kData->midiprog.data[i].bank, kData->midiprog.data[i].program, kData->midiprog.data[i].name);
        }
#endif

        if (init)
        {
            if (count > 0)
                setMidiProgram(0, false, false, false);
        }
        else
        {
            // Check if current program is invalid
            bool programChanged = false;

            if (count == oldCount+1)
            {
                // one midi program added, probably created by user
                kData->midiprog.current = oldCount;
                programChanged = true;
            }
            else if (current < 0 && count > 0)
            {
                // programs exist now, but not before
                kData->midiprog.current = 0;
                programChanged = true;
            }
            else if (current >= 0 && count == 0)
            {
                // programs existed before, but not anymore
                kData->midiprog.current = -1;
                programChanged = true;
            }
            else if (current >= static_cast<int32_t>(count))
            {
                // current midi program > count
                kData->midiprog.current = 0;
                programChanged = true;
            }
            else
            {
                // no change
                kData->midiprog.current = current;
            }

            if (programChanged)
                setMidiProgram(kData->midiprog.current, true, true, true);

            kData->engine->callback(CALLBACK_RELOAD_PROGRAMS, fId, 0, 0, 0.0f, nullptr);
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

        if (! kData->active)
        {
            // disable any output sound
            for (i=0; i < kData->audioOut.count; ++i)
                carla_zeroFloat(outBuffer[i], frames);

            return;
        }

        // handle events from different APIs
        uint32_t           evInAtomOffsets[fEventsIn.count];
        LV2_Event_Iterator evInEventIters[fEventsIn.count];
        LV2_MIDIState      evInMidiStates[fEventsIn.count];

        for (i=0; i < fEventsIn.count; ++i)
        {
            if (fEventsIn.data[i].type & CARLA_EVENT_DATA_ATOM)
            {
                evInAtomOffsets[i] = 0;
                fEventsIn.data[i].atom->atom.size = 0;
                fEventsIn.data[i].atom->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                fEventsIn.data[i].atom->body.unit = CARLA_URI_MAP_ID_NULL;
                fEventsIn.data[i].atom->body.pad  = 0;
            }
            else if (fEventsIn.data[i].type & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(fEventsIn.data[i].event, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(fEventsIn.data[i].event + 1));
                lv2_event_begin(&evInEventIters[i], fEventsIn.data[i].event);
            }
            else if (fEventsIn.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
            {
                evInMidiStates[i].midi = fEventsIn.data[i].midi;
                evInMidiStates[i].frame_count = frames;
                evInMidiStates[i].position = 0;
                evInMidiStates[i].midi->event_count = 0;
                evInMidiStates[i].midi->size = 0;
            }
        }

        for (i=0; i < fEventsOut.count; ++i)
        {
            if (fEventsOut.data[i].type & CARLA_EVENT_DATA_ATOM)
            {
                fEventsOut.data[i].atom->atom.size = 0;
                fEventsOut.data[i].atom->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                fEventsOut.data[i].atom->body.unit = CARLA_URI_MAP_ID_NULL;
                fEventsOut.data[i].atom->body.pad  = 0;
            }
            else if (fEventsOut.data[i].type & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(fEventsOut.data[i].event, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(fEventsOut.data[i].event + 1));
            }
            else if (fEventsOut.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
            {
                // not needed
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (kData->needsReset)
        {
            // TODO!
            if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                //for (unsigned char j=0, l=MAX_MIDI_CHANNELS; j < MAX_MIDI_CHANNELS; ++j)
                {
                }
            }
            else
            {
            }

            if (kData->latency > 0)
            {
                //for (i=0; i < kData->audioIn.count; ++i)
                //    carla_zeroFloat(kData->latencyBuffers[i], kData->latency);
            }

            kData->needsReset = false;
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Special Parameters

        {
            int32_t rindex;
            const EngineTimeInfo& timeInfo(kData->engine->getTimeInfo());

            for (k=0; k < kData->param.count; ++k)
            {
                if (kData->param.data[k].type == PARAMETER_LATENCY)
                {
                    // nothing
                }
                else if (kData->param.data[k].type == PARAMETER_LV2_FREEWHEEL)
                {
                    setParameterValue(k, kData->engine->isOffline() ? 1.0f : 0.0f, false, false, false);
                }
                else if (kData->param.data[k].type == PARAMETER_LV2_TIME)
                {
                    rindex = kData->param.data[k].rindex;
                    CARLA_ASSERT(rindex >= 0 && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount));

                    switch (fRdfDescriptor->Ports[rindex].Designation)
                    {
                    // Non-BBT
                    case LV2_PORT_DESIGNATION_TIME_FRAME:
                        setParameterValue(k, timeInfo.frame, false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND:
                        break;
                    //case LV2_PORT_DESIGNATION_TIME_POSITION:
                    //    setParameterValue(k, timeInfo.usecs, false, false, false);
                    //    break;
                    case LV2_PORT_DESIGNATION_TIME_SPEED:
                        setParameterValue(k, timeInfo.playing ? 1.0f : 0.0f, false, false, false);
                        break;
                    // BBT
                    case LV2_PORT_DESIGNATION_TIME_BAR:
                        if (timeInfo.valid & EngineTimeInfo::ValidBBT)
                            setParameterValue(k, timeInfo.bbt.bar - 1, false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_BAR_BEAT:
                        if (timeInfo.valid & EngineTimeInfo::ValidBBT)
                            setParameterValue(k, float(timeInfo.bbt.beat - 1) + (float(timeInfo.bbt.tick) / timeInfo.bbt.ticksPerBeat), false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_BEAT:
                        if (timeInfo.valid & EngineTimeInfo::ValidBBT)
                            setParameterValue(k, timeInfo.bbt.beat - 1, false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_BEAT_UNIT:
                        if (timeInfo.valid & EngineTimeInfo::ValidBBT)
                            setParameterValue(k, timeInfo.bbt.beatType, false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR:
                        if (timeInfo.valid & EngineTimeInfo::ValidBBT)
                            setParameterValue(k, timeInfo.bbt.beatsPerBar, false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE:
                        if (timeInfo.valid & EngineTimeInfo::ValidBBT)
                            setParameterValue(k, timeInfo.bbt.beatsPerMinute, false, false, false);
                        break;
                    }
                }
            }
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (fEventsIn.ctrl != nullptr && fEventsIn.ctrl->port != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (kData->extNotes.mutex.tryLock())
            {
                k = fEventsIn.ctrlIndex;

                while (! kData->extNotes.data.isEmpty())
                {
                    const ExternalMidiNote& note(kData->extNotes.data.getFirst(true));

                    CARLA_ASSERT(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    uint8_t midiEvent[3] = { 0 };
                    midiEvent[0]  = (note.velo > 0) ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
                    midiEvent[0] += note.channel;
                    midiEvent[1]  = note.note;
                    midiEvent[2]  = note.velo;

                    if (fEventsIn.ctrl->type & CARLA_EVENT_TYPE_MIDI)
                    {
                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                        {
                            const uint32_t evInPadSize(lv2_atom_pad_size(sizeof(LV2_Atom_Event) + 3));

                            if (evInAtomOffsets[k] + evInPadSize >= MAX_EVENT_BUFFER)
                                continue;

                            LV2_Atom_Event* const aev = getLv2AtomEvent(fEventsIn.ctrl->atom, evInAtomOffsets[k]);
                            aev->time.frames = 0;
                            aev->body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                            aev->body.size   = 3;
                            std::memcpy(LV2_ATOM_BODY(&aev->body), midiEvent, 3);

                            evInAtomOffsets[k] += evInPadSize;
                            fEventsIn.ctrl->atom->atom.size = evInAtomOffsets[k];
                        }
                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                        {
                            lv2_event_write(&evInEventIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiEvent);
                        }
                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                        {
                            lv2midi_put_event(&evInMidiStates[k], 0, 3, midiEvent);
                        }
                    }
                }

                kData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;
            bool sampleAccurate  = (fOptions & PLUGIN_OPTION_FIXED_BUFFER) == 0;

            uint32_t time, nEvents = fEventsIn.ctrl->port->getEventCount();
            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;

            uint32_t nextBankId = 0;
            if (kData->midiprog.current >= 0 && kData->midiprog.count > 0)
                nextBankId = kData->midiprog.data[kData->midiprog.current].bank;

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

                        if (kData->midiprog.current >= 0 && kData->midiprog.count > 0)
                            nextBankId = kData->midiprog.data[kData->midiprog.current].bank;
                        else
                            nextBankId = 0;

                        // reset iters
                        k = fEventsIn.ctrlIndex;

                        if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                        {
                            evInAtomOffsets[k] = 0;
                            fEventsIn.ctrl->atom->atom.size = 0;
                        }
                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                        {
                            lv2_event_buffer_reset(fEventsIn.ctrl->event, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(fEventsIn.ctrl->event + 1));
                            lv2_event_begin(&evInEventIters[k], fEventsIn.ctrl->event);
                        }
                        else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                        {
                            evInMidiStates[k].position = 0;
                            evInMidiStates[k].midi->event_count = 0;
                            evInMidiStates[k].midi->size = 0;
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
                    const EngineControlEvent& ctrlEvent = event.ctrl;

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter:
                    {
                        // Control backend stuff
                        if (event.channel == kData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (fHints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                                continue;
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (fHints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                                continue;
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (fHints & PLUGIN_CAN_BALANCE) > 0)
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
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                                continue;
                            }
                        }

                        // Control plugin parameters
                        for (k=0; k < kData->param.count; ++k)
                        {
                            if (kData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (kData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (kData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((kData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            float value;

                            if (kData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5f) ? kData->param.ranges[k].min : kData->param.ranges[k].max;
                            }
                            else
                            {
                                value = kData->param.ranges[i].unnormalizeValue(ctrlEvent.value);

                                if (kData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, value);
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == kData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == kData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t nextProgramId = ctrlEvent.param;

                            for (k=0; k < kData->midiprog.count; ++k)
                            {
                                if (kData->midiprog.data[k].bank == nextBankId && kData->midiprog.data[k].program == nextProgramId)
                                {
                                    setMidiProgram(k, false, false, false);
                                    postponeRtEvent(kPluginPostRtEventMidiProgramChange, k, 0, 0.0f);
                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (event.channel == kData->ctrlChannel)
                        {
                            if (! allNotesOffSent)
                            {
                                sendMidiAllNotesOff();
                                allNotesOffSent = true;
                            }

                            postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_ACTIVE, 0, 0.0f);
                            postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_ACTIVE, 0, 1.0f);
                        }

                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            // TODO
                        }

                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (event.channel == kData->ctrlChannel)
                        {
                            if (! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOff();
                            }
                        }

                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            // TODO
                        }

                        break;
                    }

                    break;
                }

                case kEngineEventTypeMidi:
                {
                    const EngineMidiEvent& midiEvent = event.midi;

                    uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                    uint8_t channel = event.channel;
                    uint32_t mtime  = sampleAccurate ? startTime : time;

                    if (MIDI_IS_STATUS_AFTERTOUCH(status) && (fOptions & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (MIDI_IS_STATUS_CONTROL_CHANGE(status) && (fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status) && (fOptions & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status) && (fOptions & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    // Fix bad note-off
                    if (status == MIDI_STATUS_NOTE_ON && midiEvent.data[2] == 0)
                        status -= 0x10;

                    k = fEventsIn.ctrlIndex;

                    if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_ATOM)
                    {
                        const uint32_t evInPadSize(lv2_atom_pad_size(sizeof(LV2_Atom_Event) + midiEvent.size));

                        if (evInAtomOffsets[k] + evInPadSize >= MAX_EVENT_BUFFER)
                            continue;

                        LV2_Atom_Event* const aev(getLv2AtomEvent(fEventsIn.ctrl->atom, evInAtomOffsets[k]));
                        aev->time.frames = mtime;
                        aev->body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                        aev->body.size   = midiEvent.size;
                        std::memcpy(LV2_ATOM_BODY(&aev->body), midiEvent.data, midiEvent.size);

                        evInAtomOffsets[k] += evInPadSize;
                        fEventsIn.ctrl->atom->atom.size = evInAtomOffsets[k];
                    }
                    else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_EVENT)
                    {
                        lv2_event_write(&evInEventIters[k], mtime, 0, CARLA_URI_MAP_ID_MIDI_EVENT, midiEvent.size, midiEvent.data);
                    }
                    else if (fEventsIn.ctrl->type & CARLA_EVENT_DATA_MIDI_LL)
                    {
                        lv2midi_put_event(&evInMidiStates[k], mtime, midiEvent.size, midiEvent.data);
                    }

                    if (status == MIDI_STATUS_NOTE_ON)
                        postponeRtEvent(kPluginPostRtEventNoteOn, channel, midiEvent.data[1], midiEvent.data[2]);
                    else if (status == MIDI_STATUS_NOTE_OFF)
                        postponeRtEvent(kPluginPostRtEventNoteOff, channel, midiEvent.data[1], 0.0f);

                    break;
                }
                }
            }

            kData->postRtEvents.trySplice();

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
        // Control Output

        if (kData->event.portOut != nullptr)
        {
            uint8_t  channel;
            uint16_t param;
            float    value;

            for (k=0; k < kData->param.count; ++k)
            {
                if (kData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;

                kData->param.ranges[k].fixValue(fParamBuffers[k]);

                if (kData->param.data[k].midiCC > 0)
                {
                    channel = kData->param.data[k].midiChannel;
                    param   = static_cast<uint16_t>(kData->param.data[k].midiCC);
                    value   = kData->param.ranges[k].normalizeValue(fParamBuffers[k]);
                    kData->event.portOut->writeControlEvent(0, channel, kEngineControlEventTypeParameter, param, value);
                }
            }

        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Final work

        if (fExt.worker != nullptr && fExt.worker->end_run != nullptr)
        {
            fExt.worker->end_run(fHandle);

            if (fHandle2 != nullptr)
                fExt.worker->end_run(fHandle2);
        }

        // --------------------------------------------------------------------------------------------------------
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_ASSERT(frames > 0);

        if (frames == 0)
            return false;

        if (kData->audioIn.count > 0)
        {
            CARLA_ASSERT(inBuffer != nullptr);
            if (inBuffer == nullptr)
                return false;
        }
        if (kData->audioOut.count > 0)
        {
            CARLA_ASSERT(outBuffer != nullptr);
            if (outBuffer == nullptr)
                return false;
        }

        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (kData->engine->isOffline())
        {
            kData->singleMutex.lock();
        }
        else if (! kData->singleMutex.tryLock())
        {
            for (i=0; i < kData->audioOut.count; ++i)
            {
                for (k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        for (i=0; i < kData->audioIn.count; ++i)
            carla_copyFloat(fAudioInBuffers[i], inBuffer[i]+timeOffset, frames);
        for (i=0; i < kData->audioOut.count; ++i)
            carla_zeroFloat(fAudioOutBuffers[i], frames);

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fDescriptor->run(fHandle, frames);

        if (fHandle2 != nullptr)
            fDescriptor->run(fHandle2, frames);

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (fHints & PLUGIN_CAN_DRYWET) != 0 && kData->postProc.dryWet != 1.0f;
            const bool doBalance = (fHints & PLUGIN_CAN_BALANCE) != 0 && (kData->postProc.balanceLeft != -1.0f || kData->postProc.balanceRight != 1.0f);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < kData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (k=0; k < frames; ++k)
                    {
                        // TODO
                        //if (k < kData->latency && kData->latency < frames)
                        //    bufValue = (kData->audioIn.count == 1) ? kData->latencyBuffers[0][k] : kData->latencyBuffers[i][k];
                        //else
                        //    bufValue = (kData->audioIn.count == 1) ? inBuffer[0][k-m_latency] : inBuffer[i][k-m_latency];

                        bufValue = fAudioInBuffers[(kData->audioIn.count == 1) ? 0 : i][k];
                        fAudioOutBuffers[i][k] = (fAudioOutBuffers[i][k] * kData->postProc.dryWet) + (bufValue * (1.0f - kData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < kData->audioOut.count);
                        carla_copyFloat(oldBufLeft, fAudioOutBuffers[i], frames);
                    }

                    float balRangeL = (kData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (kData->postProc.balanceRight + 1.0f)/2.0f;

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
                        outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k] * kData->postProc.volume;
                }
            }

#if 0
            // Latency, save values for next callback, TODO
            if (kData->latency > 0 && kData->latency < frames)
            {
                for (i=0; i < kData->audioIn.count; ++i)
                    carla_copyFloat(kData->latencyBuffers[i], inBuffer[i] + (frames - kData->latency), kData->latency);
            }
#endif
        } // End of Post-processing

        // --------------------------------------------------------------------------------------------------------

        kData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("Lv2Plugin::bufferSizeChanged(%i) - start", newBufferSize);

        for (uint32_t i=0; i < kData->audioIn.count; ++i)
        {
            if (fAudioInBuffers[i] != nullptr)
                delete[] fAudioInBuffers[i];
            fAudioInBuffers[i] = new float[newBufferSize];
        }

        for (uint32_t i=0; i < kData->audioOut.count; ++i)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];
            fAudioOutBuffers[i] = new float[newBufferSize];
        }

        if (fHandle2 == nullptr)
        {
            for (uint32_t i=0; i < kData->audioIn.count; ++i)
            {
                CARLA_ASSERT(fAudioInBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, kData->audioIn.ports[i].rindex, fAudioInBuffers[i]);
            }

            for (uint32_t i=0; i < kData->audioOut.count; ++i)
            {
                CARLA_ASSERT(fAudioOutBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, kData->audioOut.ports[i].rindex, fAudioOutBuffers[i]);
            }
        }
        else
        {
            if (kData->audioIn.count > 0)
            {
                CARLA_ASSERT(kData->audioIn.count == 2);
                CARLA_ASSERT(fAudioInBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioInBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  kData->audioIn.ports[0].rindex, fAudioInBuffers[0]);
                fDescriptor->connect_port(fHandle2, kData->audioIn.ports[1].rindex, fAudioInBuffers[1]);
            }

            if (kData->audioOut.count > 0)
            {
                CARLA_ASSERT(kData->audioOut.count == 2);
                CARLA_ASSERT(fAudioOutBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioOutBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  kData->audioOut.ports[0].rindex, fAudioOutBuffers[0]);
                fDescriptor->connect_port(fHandle2, kData->audioOut.ports[1].rindex, fAudioOutBuffers[1]);
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

        carla_debug("Lv2Plugin::sampleRateChanged(%g) - end", newSampleRate);
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void initBuffers() override
    {
        fEventsIn.initBuffers(kData->engine);
        fEventsOut.initBuffers(kData->engine);

        CarlaPlugin::initBuffers();
    }

    void clearBuffers() override
    {
        carla_debug("Lv2Plugin::clearBuffers() - start");

        if (fAudioInBuffers != nullptr)
        {
            for (uint32_t i=0; i < kData->audioIn.count; ++i)
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
            for (uint32_t i=0; i < kData->audioOut.count; ++i)
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

        CarlaPlugin::clearBuffers();

        carla_debug("Lv2Plugin::clearBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    void uiParameterChange(const uint32_t index, const float value) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(index < kData->param.count);

        if (fDescriptor == nullptr || fHandle == nullptr)
            return;
        if (index >= kData->param.count)
            return;

        if (fUi.type == PLUGIN_UI_OSC)
        {
            if (kData->osc.data.target != nullptr)
                osc_send_control(&kData->osc.data, kData->param.data[index].rindex, value);
        }
        else
        {
            if (fUi.handle != nullptr && fUi.descriptor != nullptr && fUi.descriptor->port_event != nullptr)
                fUi.descriptor->port_event(fUi.handle, kData->param.data[index].rindex, sizeof(float), 0, &value);
        }
    }

    void uiMidiProgramChange(const uint32_t index) override
    {
        CARLA_ASSERT(index < kData->midiprog.count);

        if (index >= kData->midiprog.count)
            return;

        if (fUi.type == PLUGIN_UI_OSC)
        {
            if (kData->osc.data.target != nullptr)
                osc_send_midi_program(&kData->osc.data, kData->midiprog.data[index].bank, kData->midiprog.data[index].program);
        }
        else
        {
            if (fExt.uiprograms != nullptr && fExt.uiprograms->select_program != nullptr)
                fExt.uiprograms->select_program(fUi.handle, kData->midiprog.data[index].bank, kData->midiprog.data[index].program);
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
            if (kData->osc.data.target != nullptr)
            {
                uint8_t midiData[4] = { 0 };
                midiData[1] = MIDI_STATUS_NOTE_ON + channel;
                midiData[2] = note;
                midiData[3] = velo;
                osc_send_midi(&kData->osc.data, midiData);
            }
        }
        else
        {
            if (fUi.handle != nullptr && fUi.descriptor != nullptr && fUi.descriptor->port_event != nullptr)
            {
                LV2_Atom_MidiEvent midiEv;
                midiEv.event.time.frames = 0;
                midiEv.event.body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                midiEv.event.body.size   = 3;
                midiEv.data[0] = MIDI_STATUS_NOTE_OFF + channel;
                midiEv.data[1] = note;
                midiEv.data[2] = velo;

                fUi.descriptor->port_event(fUi.handle, 0, 3, CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, &midiEv);
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
            if (kData->osc.data.target != nullptr)
            {
                uint8_t midiData[4] = { 0 };
                midiData[1] = MIDI_STATUS_NOTE_OFF + channel;
                midiData[2] = note;
                osc_send_midi(&kData->osc.data, midiData);
            }
        }
        else
        {
            if (fUi.handle != nullptr && fUi.descriptor != nullptr && fUi.descriptor->port_event != nullptr)
            {
                LV2_Atom_MidiEvent midiEv;
                midiEv.event.time.frames = 0;
                midiEv.event.body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                midiEv.event.body.size   = 3;
                midiEv.data[0] = MIDI_STATUS_NOTE_OFF + channel;
                midiEv.data[1] = note;
                midiEv.data[2] = 0;

                fUi.descriptor->port_event(fUi.handle, 0, 3, CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, &midiEv);
            }
        }
    }

    // -------------------------------------------------------------------

protected:
    void guiClosedCallback() override
    {
        showGui(false);
        kData->engine->callback(CALLBACK_SHOW_GUI, fId, 0, 0, 0.0f, nullptr);
    }

    // -------------------------------------------------------------------

    uint32_t getCustomURID(const char* const uri)
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

        return fCustomURIDs.count()-1;
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
        if (index == -1)
        {
            const CarlaPlugin::ScopedDisabler m(this);
            return reloadPrograms(false);
        }

        if (index >= 0 && index < static_cast<int32_t>(kData->midiprog.count) && fExt.programs != nullptr && fExt.programs->get_program != nullptr)
        {
            if (const LV2_Program_Descriptor* progDesc = fExt.programs->get_program(fHandle, index))
            {
                CARLA_ASSERT(progDesc->name != nullptr);

                if (kData->midiprog.data[index].name != nullptr)
                    delete[] kData->midiprog.data[index].name;

                kData->midiprog.data[index].name = carla_strdup(progDesc->name ? progDesc->name : "");

                if (index == kData->midiprog.current)
                    kData->engine->callback(CALLBACK_UPDATE, fId, 0, 0, 0.0, nullptr);
                else
                    kData->engine->callback(CALLBACK_RELOAD_PROGRAMS, fId, 0, 0, 0.0, nullptr);
            }
        }
    }

    // -------------------------------------------------------------------

    LV2_State_Status handleStateStore(const uint32_t key, const void* const value, const size_t size, const uint32_t type, const uint32_t flags)
    {
        CARLA_ASSERT(key != CARLA_URI_MAP_ID_NULL);
        CARLA_ASSERT(value != nullptr);
        CARLA_ASSERT(size > 0);

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
        for (auto it = kData->custom.begin(); it.valid(); it.next())
        {
            CustomData& data(*it);

            if (std::strcmp(data.key, uriKey) == 0)
            {
                if (data.value != nullptr)
                    delete[] data.value;

                if (std::strcmp(stype, LV2_ATOM__String) == 0 || std::strcmp(stype, LV2_ATOM__Path) == 0)
                    data.value = carla_strdup((const char*)value);
                else
                    data.value = carla_strdup(QByteArray((const char*)value, size).toBase64().constData());

                return LV2_STATE_SUCCESS;
            }
        }

        // Otherwise store it
        CustomData newData;
        newData.type = carla_strdup(stype);
        newData.key  = carla_strdup(uriKey);

        if (std::strcmp(stype, LV2_ATOM__String) == 0 || std::strcmp(stype, LV2_ATOM__Path) == 0)
            newData.value = carla_strdup((const char*)value);
        else
            newData.value = carla_strdup(QByteArray((const char*)value, size).toBase64().constData());

        kData->custom.append(newData);

        return LV2_STATE_SUCCESS;
    }

    const void* handleStateRetrieve(const uint32_t key, size_t* const size, uint32_t* const type, uint32_t* const flags)
    {
        CARLA_ASSERT(key != CARLA_URI_MAP_ID_NULL);

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

        for (auto it = kData->custom.begin(); it.valid(); it.next())
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

        *type  = key;
        *flags = LV2_STATE_IS_POD;

        if (std::strcmp(stype, LV2_ATOM__String) == 0 || std::strcmp(stype, LV2_ATOM__Path) == 0)
        {
            *size = std::strlen(stringData);
            return stringData;
        }
        else
        {
            static QByteArray chunk;
            chunk = QByteArray::fromBase64(stringData);
            *size = chunk.size();
            return chunk.constData();
        }
    }

    // -------------------------------------------------------------------

    LV2_Worker_Status handleWorkerSchedule(const uint32_t size, const void* const data)
    {
        carla_stdout("Lv2Plugin::handleWorkerSchedule(%i, %p)", size, data);

#if 0
        if (! ext.worker)
        {
            carla_stderr("Lv2Plugin::handleWorkerSchedule(%i, %p) - plugin has no worker", size, data);
            return LV2_WORKER_ERR_UNKNOWN;
        }

        if (x_engine->isOffline())
            ext.worker->work(handle, carla_lv2_worker_respond, this, size, data);
        else
            postponeEvent(PluginPostEventCustom, size, 0, 0.0, data);
#endif

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
        kData->engine->callback(CALLBACK_SHOW_GUI, fId, 0, 0, 0.0, nullptr);
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
        CARLA_ASSERT(kData->gui != nullptr);
        CARLA_ASSERT(width > 0);
        CARLA_ASSERT(height > 0);

        if (width <= 0 || height <= 0)
            return 1;

        if (kData->gui != nullptr)
            kData->gui->setSize(width, height);

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

            float value = *(float*)buffer;

            for (uint32_t i=0; i < kData->param.count; ++i)
            {
                if (kData->param.data[i].rindex == static_cast<int32_t>(rindex))
                    return setParameterValue(i, value, false, true, true);
            }
        }
        else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM)
        {
            CARLA_ASSERT(buffer != nullptr);

            if (buffer == nullptr)
                return;

            //const LV2_Atom* const atom = (const LV2_Atom*)buffer;
            //handleTransferAtom(rindex, atom);
        }
        else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT)
        {
            CARLA_ASSERT(buffer != nullptr);

            if (buffer == nullptr)
                return;

            //const LV2_Atom* const atom = (const LV2_Atom*)buffer;
            //handleTransferEvent(rindex, atom);
        }
    }

    // -------------------------------------------------------------------

    bool needsFixedBuffer()
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

    const char* getUiBridgePath(const LV2_Property type)
    {
        const EngineOptions& options(kData->engine->getOptions());

        switch (type)
        {
        case LV2_UI_GTK2:
            return options.bridge_lv2Gtk2;
        case LV2_UI_GTK3:
            return options.bridge_lv2Gtk3;
        case LV2_UI_QT4:
            return options.bridge_lv2Qt4;
        case LV2_UI_QT5:
            return options.bridge_lv2Qt5;
        case LV2_UI_COCOA:
            return options.bridge_lv2Cocoa;
        case LV2_UI_WINDOWS:
            return options.bridge_lv2Win;
        case LV2_UI_X11:
            return options.bridge_lv2X11;
        default:
            return nullptr;
        }
    }

    bool isUiBridgeable(const uint32_t uiId)
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

    bool isUiResizable()
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
            if (fExt.uiprograms && kData->midiprog.count > 0 && kData->midiprog.current >= 0)
                fExt.uiprograms->select_program(fUi.handle, kData->midiprog.data[kData->midiprog.current].bank,
                                                            kData->midiprog.data[kData->midiprog.current].program);
        }

        if (fUi.descriptor->port_event != nullptr)
        {
            // update control ports
            float value;
            for (uint32_t i=0; i < kData->param.count; ++i)
            {
                value = getParameterValue(i);
                fUi.descriptor->port_event(fUi.handle, kData->param.data[i].rindex, sizeof(float), CARLA_URI_MAP_ID_NULL, &value);
            }
        }
    }

    // -------------------------------------------------------------------

public:
    bool init(const char* const name, const char* const uri)
    {
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(kData->client == nullptr);
        CARLA_ASSERT(uri != nullptr);

        // ---------------------------------------------------------------
        // first checks

        if (kData->engine == nullptr)
        {
            return false;
        }

        if (kData->client != nullptr)
        {
            kData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (uri == nullptr)
        {
            kData->engine->setLastError("null uri");
            return false;
        }

        // ---------------------------------------------------------------
        // get plugin from lv2_rdf (lilv)

        gLv2World.init();

        fRdfDescriptor = lv2_rdf_new(uri);

        if (fRdfDescriptor == nullptr)
        {
            kData->engine->setLastError("Failed to find the requested plugin in the LV2 Bundle");
            return false;
        }

        // ---------------------------------------------------------------
        // open DLL

        if (! kData->libOpen(fRdfDescriptor->Binary))
        {
            kData->engine->setLastError(kData->libError(fRdfDescriptor->Binary));
            return false;
        }

        // ---------------------------------------------------------------
        // initialize options

        fLv2Options.minBufferSize = 1;
        fLv2Options.maxBufferSize = kData->engine->getBufferSize();
        fLv2Options.sampleRate    = kData->engine->getSampleRate();

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

        fFeatures[kFeatureIdBufSizeBounded]        = new LV2_Feature;
        fFeatures[kFeatureIdBufSizeBounded]->URI   = LV2_BUF_SIZE__boundedBlockLength;
        fFeatures[kFeatureIdBufSizeBounded]->data  = nullptr;

        fFeatures[kFeatureIdBufSizeFixed]          = new LV2_Feature;
        fFeatures[kFeatureIdBufSizeFixed]->URI     = LV2_BUF_SIZE__fixedBlockLength;
        fFeatures[kFeatureIdBufSizeFixed]->data    = nullptr;

        fFeatures[kFeatureIdBufSizePowerOf2]       = new LV2_Feature;
        fFeatures[kFeatureIdBufSizePowerOf2]->URI  = LV2_BUF_SIZE__powerOf2BlockLength;
        fFeatures[kFeatureIdBufSizePowerOf2]->data = nullptr;

        fFeatures[kFeatureIdEvent]          = new LV2_Feature;
        fFeatures[kFeatureIdEvent]->URI     = LV2_EVENT_URI;
        fFeatures[kFeatureIdEvent]->data    = eventFt;

        fFeatures[kFeatureIdLogs]           = new LV2_Feature;
        fFeatures[kFeatureIdLogs]->URI      = LV2_LOG__log;
        fFeatures[kFeatureIdLogs]->data     = logFt;

        fFeatures[kFeatureIdOptions]        = new LV2_Feature;
        fFeatures[kFeatureIdOptions]->URI   = LV2_OPTIONS__options;
        fFeatures[kFeatureIdOptions]->data  = fLv2Options.opts;

        fFeatures[kFeatureIdPrograms]       = new LV2_Feature;
        fFeatures[kFeatureIdPrograms]->URI  = LV2_PROGRAMS__Host;
        fFeatures[kFeatureIdPrograms]->data = programsFt;

        fFeatures[kFeatureIdRtMemPool]       = new LV2_Feature;
        fFeatures[kFeatureIdRtMemPool]->URI  = LV2_RTSAFE_MEMORY_POOL__Pool;
        fFeatures[kFeatureIdRtMemPool]->data = rtMemPoolFt;

        fFeatures[kFeatureIdStateMakePath]       = new LV2_Feature;
        fFeatures[kFeatureIdStateMakePath]->URI  = LV2_STATE__makePath;
        fFeatures[kFeatureIdStateMakePath]->data = stateMakePathFt;

        fFeatures[kFeatureIdStateMapPath]        = new LV2_Feature;
        fFeatures[kFeatureIdStateMapPath]->URI   = LV2_STATE__mapPath;
        fFeatures[kFeatureIdStateMapPath]->data  = stateMapPathFt;

        fFeatures[kFeatureIdStrictBounds]         = new LV2_Feature;
        fFeatures[kFeatureIdStrictBounds]->URI    = LV2_PORT_PROPS__supportsStrictBounds;
        fFeatures[kFeatureIdStrictBounds]->data   = nullptr;

        fFeatures[kFeatureIdUriMap]          = new LV2_Feature;
        fFeatures[kFeatureIdUriMap]->URI     = LV2_URI_MAP_URI;
        fFeatures[kFeatureIdUriMap]->data    = uriMapFt;

        fFeatures[kFeatureIdUridMap]         = new LV2_Feature;
        fFeatures[kFeatureIdUridMap]->URI    = LV2_URID__map;
        fFeatures[kFeatureIdUridMap]->data   = uridMapFt;

        fFeatures[kFeatureIdUridUnmap]       = new LV2_Feature;
        fFeatures[kFeatureIdUridUnmap]->URI  = LV2_URID__unmap;
        fFeatures[kFeatureIdUridUnmap]->data = uridUnmapFt;

        fFeatures[kFeatureIdWorker]           = new LV2_Feature;
        fFeatures[kFeatureIdWorker]->URI      = LV2_WORKER__schedule;
        fFeatures[kFeatureIdWorker]->data     = workerFt;

        if (! needsFixedBuffer())
            fFeatures[kFeatureIdBufSizeFixed]->URI = LV2_BUF_SIZE__boundedBlockLength;

        // ---------------------------------------------------------------
        // get DLL main entry

#if 0
        const LV2_Lib_Descriptor_Function libDescFn = (LV2_Lib_Descriptor_Function)kData->libSymbol("lv2_lib_descriptor");

        if (libDescFn != nullptr)
        {
            // -----------------------------------------------------------
            // get lib descriptor

            const LV2_Lib_Descriptor* libFn = nullptr; //descLibFn(fRdfDescriptor->Bundle, features);

            if (libFn == nullptr || libFn->get_plugin == nullptr)
            {
                kData->engine->setLastError("Plugin failed to return library descriptor");
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
            const LV2_Descriptor_Function descFn = (LV2_Descriptor_Function)kData->libSymbol("lv2_descriptor");

            if (descFn == nullptr)
            {
                kData->engine->setLastError("Could not find the LV2 Descriptor in the plugin library");
                return false;
            }

            // -----------------------------------------------------------
            // get descriptor that matches URI

            uint32_t i = 0;
            while ((fDescriptor = descFn(i++)))
            {
                if (std::strcmp(fDescriptor->URI, uri) == 0)
                    break;
            }
        }

        if (fDescriptor == nullptr)
        {
            kData->engine->setLastError("Could not find the requested plugin URI in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // check supported port-types and features

        bool canContinue = true;

        // Check supported ports
        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            const LV2_Property portTypes(fRdfDescriptor->Ports[i].Types);

            if (! (LV2_IS_PORT_AUDIO(portTypes) || LV2_IS_PORT_CONTROL(portTypes) || LV2_IS_PORT_ATOM_SEQUENCE(portTypes) || LV2_IS_PORT_EVENT(portTypes) || LV2_IS_PORT_MIDI_LL(portTypes)))
            {
                if (! LV2_IS_PORT_OPTIONAL(fRdfDescriptor->Ports[i].Properties))
                {
                    kData->engine->setLastError("Plugin requires a port type that is not currently supported");
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
                QString msg(QString("Plugin requires a feature that is not supported:\n%1").arg(fRdfDescriptor->Features[i].URI));
                kData->engine->setLastError(msg.toUtf8().constData());
                canContinue = false;
                break;
            }
        }

        // Check extensions
        for (uint32_t i=0; i < fRdfDescriptor->ExtensionCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_OPTIONS__interface) == 0)
                kData->extraHints |= PLUGIN_HAS_EXTENSION_OPTIONS;
            else if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_PROGRAMS__Interface) == 0)
                kData->extraHints |= PLUGIN_HAS_EXTENSION_PROGRAMS;
            else if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_STATE__interface) == 0)
                kData->extraHints |= PLUGIN_HAS_EXTENSION_STATE;
            else if (std::strcmp(fRdfDescriptor->Extensions[i], LV2_WORKER__interface) == 0)
                kData->extraHints |= PLUGIN_HAS_EXTENSION_WORKER;
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
            fName = kData->engine->getUniquePluginName(name);
        else
            fName = kData->engine->getUniquePluginName(fRdfDescriptor->Name);

        // ---------------------------------------------------------------
        // register client

        kData->client = kData->engine->addClient(this);

        if (kData->client == nullptr || ! kData->client->isOk())
        {
            kData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin

        fHandle = fDescriptor->instantiate(fDescriptor, kData->engine->getSampleRate(), fRdfDescriptor->Bundle, fFeatures);

        if (fHandle == nullptr)
        {
            kData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // load plugin settings

        {
            // set default options
            fOptions = 0x0;

            fOptions |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

            if (needsFixedBuffer())
                fOptions |= PLUGIN_OPTION_FIXED_BUFFER;

            if (kData->engine->getOptions().forceStereo)
                fOptions |= PLUGIN_OPTION_FORCE_STEREO;

            if (midiInCount() > 0)
            {
                fOptions |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                fOptions |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                fOptions |= PLUGIN_OPTION_SEND_PITCHBEND;
                fOptions |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            }

            // load settings
            kData->idStr  = "LV2/";
            kData->idStr += uri;
            fOptions = kData->loadSettings(fOptions, availableOptions());

            // ignore settings, we need this anyway
            if (needsFixedBuffer())
                fOptions |= PLUGIN_OPTION_FIXED_BUFFER;
        }

        // ---------------------------------------------------------------
        // gui stuff

        if (fRdfDescriptor->UICount == 0)
            return true;

        // -----------------------------------------------------------
        // find more appropriate ui

        int eQt4, eQt5, eCocoa, eWindows, eX11, eGtk2, eGtk3, iCocoa, iWindows, iX11, iQt4, iQt5, iExt, iFinal;
        eQt4 = eQt5 = eCocoa = eWindows = eX11 = eGtk2 = eGtk3 = iQt4 = iQt5 = iCocoa = iWindows = iX11 = iExt = iFinal = -1;

#ifdef BUILD_BRIDGE
        const bool preferUiBridges(kData->engine->getOptions().preferUiBridges);
#else
        const bool preferUiBridges(kData->engine->getOptions().preferUiBridges && (fHints & PLUGIN_IS_BRIDGE) == 0);
#endif

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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
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
                kData->osc.thread.setOscData(oscBinary, fDescriptor->URI, fUi.rdfDescriptor->URI);
            }
        }
        else
        {
            // -------------------------------------------------------
            // open UI DLL

            if (! kData->uiLibOpen(fUi.rdfDescriptor->Binary))
            {
                carla_stderr2("Could not load UI library, error was:\n%s", kData->libError(fUi.rdfDescriptor->Binary));
                fUi.rdfDescriptor = nullptr;
                return true;
            }

            // -------------------------------------------------------
            // get UI DLL main entry

            LV2UI_DescriptorFunction uiDescFn = (LV2UI_DescriptorFunction)kData->uiLibSymbol("lv2ui_descriptor");

            if (uiDescFn == nullptr)
            {
                carla_stderr2("Could not find the LV2UI Descriptor in the UI library");
                kData->uiLibClose();
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
                kData->uiLibClose();
                fUi.rdfDescriptor = nullptr;
                return true;
            }

            // -------------------------------------------------------
            // check if ui is usable

            switch (uiType)
            {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
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
                kData->uiLibClose();
                fUi.descriptor = nullptr;
                fUi.rdfDescriptor = nullptr;
                return true;
            }

            // -------------------------------------------------------
            // initialize ui features

            QString guiTitle(QString("%1 (GUI)").arg((const char*)fName));

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
            uiExternalHostFt->plugin_human_id            = carla_strdup(guiTitle.toUtf8().constData());

            fFeatures[kFeatureIdUiDataAccess]           = new LV2_Feature;
            fFeatures[kFeatureIdUiDataAccess]->URI      = LV2_DATA_ACCESS_URI;
            fFeatures[kFeatureIdUiDataAccess]->data     = uiDataFt;

            fFeatures[kFeatureIdUiInstanceAccess]       = new LV2_Feature;
            fFeatures[kFeatureIdUiInstanceAccess]->URI  = LV2_INSTANCE_ACCESS_URI;
            fFeatures[kFeatureIdUiInstanceAccess]->data = fHandle;

            fFeatures[kFeatureIdUiParent]             = new LV2_Feature;
            fFeatures[kFeatureIdUiParent]->URI        = LV2_UI__parent;
            fFeatures[kFeatureIdUiParent]->data       = nullptr;

            fFeatures[kFeatureIdUiPortMap]           = new LV2_Feature;
            fFeatures[kFeatureIdUiPortMap]->URI      = LV2_UI__portMap;
            fFeatures[kFeatureIdUiPortMap]->data     = uiPortMapFt;

            fFeatures[kFeatureIdUiResize]             = new LV2_Feature;
            fFeatures[kFeatureIdUiResize]->URI        = LV2_UI__resize;
            fFeatures[kFeatureIdUiResize]->data       = uiResizeFt;

            fFeatures[kFeatureIdExternalUi]           = new LV2_Feature;
            fFeatures[kFeatureIdExternalUi]->URI      = LV2_EXTERNAL_UI__Host;
            fFeatures[kFeatureIdExternalUi]->data     = uiExternalHostFt;

            fFeatures[kFeatureIdExternalUiOld]       = new LV2_Feature;
            fFeatures[kFeatureIdExternalUiOld]->URI  = LV2_EXTERNAL_UI_DEPRECATED_URI;
            fFeatures[kFeatureIdExternalUiOld]->data = uiExternalHostFt;
        }

        return true;
    }

private:
    LV2_Handle   fHandle;
    LV2_Handle   fHandle2;
    LV2_Feature* fFeatures[kFeatureCount+1];
    const LV2_Descriptor*     fDescriptor;
    const LV2_RDF_Descriptor* fRdfDescriptor;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float*  fParamBuffers;

    Lv2PluginEventData fEventsIn;
    Lv2PluginEventData fEventsOut;
    Lv2PluginOptions   fLv2Options;

    NonRtList<const char*> fCustomURIDs;

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

        QDir dir;
        dir.mkpath(path);
        return strdup(path);
    }

    static char* carla_lv2_state_map_abstract_path(LV2_State_Map_Path_Handle handle, const char* absolute_path)
    {
        carla_debug("carla_lv2_state_map_abstract_path(%p, \"%s\")", handle, absolute_path);
        CARLA_ASSERT(handle != nullptr);
        CARLA_ASSERT(absolute_path != nullptr);

        if (absolute_path == nullptr)
            return nullptr;

        QDir dir(absolute_path);
        return strdup(dir.canonicalPath().toUtf8().constData());
    }

    static char* carla_lv2_state_map_absolute_path(LV2_State_Map_Path_Handle handle, const char* abstract_path)
    {
        carla_debug("carla_lv2_state_map_absolute_path(%p, \"%s\")", handle, abstract_path);
        CARLA_ASSERT(handle != nullptr);
        CARLA_ASSERT(abstract_path != nullptr);

        if (abstract_path == nullptr)
            return nullptr;

        QDir dir(abstract_path);
        return strdup(dir.absolutePath().toUtf8().constData());
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
        if (std::strcmp(uri, LV2_ATOM__Chunk) == 0)
            return CARLA_URI_MAP_ID_ATOM_CHUNK;
        if (std::strcmp(uri, LV2_ATOM__Double) == 0)
            return CARLA_URI_MAP_ID_ATOM_DOUBLE;
        if (std::strcmp(uri, LV2_ATOM__Int) == 0)
            return CARLA_URI_MAP_ID_ATOM_INT;
        if (std::strcmp(uri, LV2_ATOM__Path) == 0)
            return CARLA_URI_MAP_ID_ATOM_PATH;
        if (std::strcmp(uri, LV2_ATOM__Sequence) == 0)
            return CARLA_URI_MAP_ID_ATOM_SEQUENCE;
        if (std::strcmp(uri, LV2_ATOM__String) == 0)
            return CARLA_URI_MAP_ID_ATOM_STRING;
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

        // Others
        if (std::strcmp(uri, LV2_MIDI__MidiEvent) == 0)
            return CARLA_URI_MAP_ID_MIDI_EVENT;
        if (std::strcmp(uri, LV2_PARAMETERS__sampleRate) == 0)
            return CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE;
        if (std::strcmp(uri, LV2_TIME__Position) == 0)
            return CARLA_URI_MAP_ID_TIME_POSITION;

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
        if (urid == CARLA_URI_MAP_ID_ATOM_CHUNK)
            return LV2_ATOM__Chunk;
        if (urid == CARLA_URI_MAP_ID_ATOM_DOUBLE)
            return LV2_ATOM__Double;
        if (urid == CARLA_URI_MAP_ID_ATOM_INT)
            return LV2_ATOM__Int;
        if (urid == CARLA_URI_MAP_ID_ATOM_PATH)
            return LV2_ATOM__Path;
        if (urid == CARLA_URI_MAP_ID_ATOM_SEQUENCE)
            return LV2_ATOM__Sequence;
        if (urid == CARLA_URI_MAP_ID_ATOM_STRING)
            return LV2_ATOM__String;
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

        // Others
        if (urid == CARLA_URI_MAP_ID_MIDI_EVENT)
            return LV2_MIDI__MidiEvent;
        if (urid == CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE)
            return LV2_PARAMETERS__sampleRate;
        if (urid == CARLA_URI_MAP_ID_TIME_POSITION)
            return LV2_TIME__Position;

        if (handle == nullptr)
            return nullptr;

        // Custom types
        return ((Lv2Plugin*)handle)->getCustomURIString(urid);
    }

    // -------------------------------------------------------------------
    // Worker Feature

    static LV2_Worker_Status carla_lv2_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void* data)
    {
        carla_debug("carla_lv2_worker_schedule(%p, %i, %p)", handle, size, data);
        CARLA_ASSERT(handle != nullptr);

        if (handle == nullptr)
            return LV2_WORKER_ERR_UNKNOWN;

        return ((Lv2Plugin*)handle)->handleWorkerSchedule(size, data);
    }

    static LV2_Worker_Status carla_lv2_worker_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
    {
        carla_debug("carla_lv2_worker_respond(%p, %i, %p)", handle, size, data);
        CARLA_ASSERT(handle != nullptr);

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
        carla_debug("carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);
        CARLA_ASSERT(controller != nullptr);

        if (controller == nullptr)
            return;

        ((Lv2Plugin*)controller)->handleUiWrite(port_index, buffer_size, format, buffer);
    }

    // -------------------------------------------------------------------

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Lv2Plugin)
};

CARLA_BACKEND_END_NAMESPACE

#else // WANT_VST
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

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && ! CarlaPluginProtectedData::canRunInRack(plugin))
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
