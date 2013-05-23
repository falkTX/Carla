/*
 * Carla Bridge UI, LV2 version
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

#include "CarlaBridgeClient.hpp"
#include "CarlaLv2Utils.hpp"
#include "CarlaMIDI.h"
#include "RtList.hpp"

#include <QtCore/QDir>

extern "C" {
#include "rtmempool/rtmempool-lv2.h"
}

// -----------------------------------------------------
// Our LV2 World class object

Lv2WorldClass gLv2World;

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

// fake values
static uint32_t gBufferSize = 1024;
static double   gSampleRate = 44100.0;

// static max values
const unsigned int MAX_EVENT_BUFFER = 8192; // 0x2000

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

// -------------------------------------------------------------------------

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

        opts[0] = &optMinBlockLenth;
        opts[1] = &optMaxBlockLenth;
        opts[2] = &optSequenceSize;
        opts[3] = &optSampleRate;
        opts[4] = &optNull;
    }

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(Lv2PluginOptions)
};

// -------------------------------------------------------------------------

class CarlaLv2Client : public CarlaBridgeClient
{
public:
    CarlaLv2Client(const char* const uiTitle)
        : CarlaBridgeClient(uiTitle)
    {
        handle = nullptr;
        widget = nullptr;
        descriptor = nullptr;

        rdf_descriptor = nullptr;
        rdf_ui_descriptor = nullptr;

        programs = nullptr;

#ifdef BRIDGE_LV2_X11
        m_resizable = false;
#else
        m_resizable = true;
#endif

        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; ++i)
            customURIDs.push_back(nullptr);

        for (uint32_t i=0; i < kFeatureCount+1; ++i)
            features[i] = nullptr;

        // -----------------------------------------------------------------
        // initialize features

        LV2_Event_Feature* const eventFt = new LV2_Event_Feature;
        eventFt->callback_data           = this;
        eventFt->lv2_event_ref           = carla_lv2_event_ref;
        eventFt->lv2_event_unref         = carla_lv2_event_unref;

        LV2_Log_Log* const logFt         = new LV2_Log_Log;
        logFt->handle                    = this;
        logFt->printf                    = carla_lv2_log_printf;
        logFt->vprintf                   = carla_lv2_log_vprintf;

        LV2_Programs_Host* const programsFt        = new LV2_Programs_Host;
        programsFt->handle                         = this;
        programsFt->program_changed                = carla_lv2_program_changed;

        LV2_RtMemPool_Pool* const rtMemPoolFt      = new LV2_RtMemPool_Pool;
        lv2_rtmempool_init(rtMemPoolFt);

        LV2_State_Make_Path* const stateMakePathFt = new LV2_State_Make_Path;
        stateMakePathFt->handle                    = this;
        stateMakePathFt->path                      = carla_lv2_state_make_path;

        LV2_State_Map_Path* const stateMapPathFt   = new LV2_State_Map_Path;
        stateMapPathFt->handle                     = this;
        stateMapPathFt->abstract_path              = carla_lv2_state_map_abstract_path;
        stateMapPathFt->absolute_path              = carla_lv2_state_map_absolute_path;

        LV2_URI_Map_Feature* const uriMapFt = new LV2_URI_Map_Feature;
        uriMapFt->callback_data             = this;
        uriMapFt->uri_to_id                 = carla_lv2_uri_to_id;

        LV2_URID_Map* const uridMapFt       = new LV2_URID_Map;
        uridMapFt->handle                   = this;
        uridMapFt->map                      = carla_lv2_urid_map;

        LV2_URID_Unmap* const uridUnmapFt   = new LV2_URID_Unmap;
        uridUnmapFt->handle                 = this;
        uridUnmapFt->unmap                  = carla_lv2_urid_unmap;

        LV2UI_Port_Map* const uiPortMapFt   = new LV2UI_Port_Map;
        uiPortMapFt->handle                 = this;
        uiPortMapFt->port_index             = carla_lv2_ui_port_map;

        LV2UI_Resize* const uiResizeFt      = new LV2UI_Resize;
        uiResizeFt->handle                  = this;
        uiResizeFt->ui_resize               = carla_lv2_ui_resize;

#if 0
        features[lv2_feature_id_bufsize_bounded]        = new LV2_Feature;
        features[lv2_feature_id_bufsize_bounded]->URI   = LV2_BUF_SIZE__boundedBlockLength;
        features[lv2_feature_id_bufsize_bounded]->data  = nullptr;

        features[lv2_feature_id_bufsize_fixed]          = new LV2_Feature;
        features[lv2_feature_id_bufsize_fixed]->URI     = LV2_BUF_SIZE__fixedBlockLength;
        features[lv2_feature_id_bufsize_fixed]->data    = nullptr;

        features[lv2_feature_id_bufsize_powerof2]       = new LV2_Feature;
        features[lv2_feature_id_bufsize_powerof2]->URI  = LV2_BUF_SIZE__powerOf2BlockLength;
        features[lv2_feature_id_bufsize_powerof2]->data = nullptr;

        features[lv2_feature_id_event]          = new LV2_Feature;
        features[lv2_feature_id_event]->URI     = LV2_EVENT_URI;
        features[lv2_feature_id_event]->data    = eventFt;

        features[lv2_feature_id_logs]           = new LV2_Feature;
        features[lv2_feature_id_logs]->URI      = LV2_LOG__log;
        features[lv2_feature_id_logs]->data     = logFt;

        features[lv2_feature_id_options]        = new LV2_Feature;
        features[lv2_feature_id_options]->URI   = LV2_OPTIONS__options;
        features[lv2_feature_id_options]->data  = optionsFt;

        features[lv2_feature_id_programs]       = new LV2_Feature;
        features[lv2_feature_id_programs]->URI  = LV2_PROGRAMS__Host;
        features[lv2_feature_id_programs]->data = programsFt;

        features[lv2_feature_id_rtmempool]       = new LV2_Feature;
        features[lv2_feature_id_rtmempool]->URI  = LV2_RTSAFE_MEMORY_POOL__Pool;
        features[lv2_feature_id_rtmempool]->data = rtMemPoolFt;

        features[lv2_feature_id_state_make_path]       = new LV2_Feature;
        features[lv2_feature_id_state_make_path]->URI  = LV2_STATE__makePath;
        features[lv2_feature_id_state_make_path]->data = stateMakePathFt;

        features[lv2_feature_id_state_map_path]        = new LV2_Feature;
        features[lv2_feature_id_state_map_path]->URI   = LV2_STATE__mapPath;
        features[lv2_feature_id_state_map_path]->data  = stateMapPathFt;

        features[lv2_feature_id_strict_bounds]         = new LV2_Feature;
        features[lv2_feature_id_strict_bounds]->URI    = LV2_PORT_PROPS__supportsStrictBounds;
        features[lv2_feature_id_strict_bounds]->data   = nullptr;

        features[lv2_feature_id_uri_map]           = new LV2_Feature;
        features[lv2_feature_id_uri_map]->URI      = LV2_URI_MAP_URI;
        features[lv2_feature_id_uri_map]->data     = uriMapFt;

        features[lv2_feature_id_urid_map]          = new LV2_Feature;
        features[lv2_feature_id_urid_map]->URI     = LV2_URID__map;
        features[lv2_feature_id_urid_map]->data    = uridMapFt;

        features[lv2_feature_id_urid_unmap]        = new LV2_Feature;
        features[lv2_feature_id_urid_unmap]->URI   = LV2_URID__unmap;
        features[lv2_feature_id_urid_unmap]->data  = uridUnmapFt;

        features[lv2_feature_id_ui_parent]         = new LV2_Feature;
        features[lv2_feature_id_ui_parent]->URI    = LV2_UI__parent;
        features[lv2_feature_id_ui_parent]->data   = nullptr;

        features[lv2_feature_id_ui_port_map]       = new LV2_Feature;
        features[lv2_feature_id_ui_port_map]->URI  = LV2_UI__portMap;
        features[lv2_feature_id_ui_port_map]->data = uiPortMapFt;

        features[lv2_feature_id_ui_resize]         = new LV2_Feature;
        features[lv2_feature_id_ui_resize]->URI    = LV2_UI__resize;
        features[lv2_feature_id_ui_resize]->data   = uiResizeFt;
#endif
    }

    ~CarlaLv2Client()
    {
        if (rdf_descriptor)
            delete rdf_descriptor;

#if 0
        const LV2_Options_Option* const options = (const LV2_Options_Option*)features[lv2_feature_id_options]->data;
        delete[] options;

        delete (LV2_Event_Feature*)features[lv2_feature_id_event]->data;
        delete (LV2_Log_Log*)features[lv2_feature_id_logs]->data;
        delete (LV2_Programs_Host*)features[lv2_feature_id_programs]->data;
        delete (LV2_RtMemPool_Pool*)features[lv2_feature_id_rtmempool]->data;
        delete (LV2_State_Make_Path*)features[lv2_feature_id_state_make_path]->data;
        delete (LV2_State_Map_Path*)features[lv2_feature_id_state_map_path]->data;
        delete (LV2_URI_Map_Feature*)features[lv2_feature_id_uri_map]->data;
        delete (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;
        delete (LV2_URID_Unmap*)features[lv2_feature_id_urid_unmap]->data;
        delete (LV2UI_Port_Map*)features[lv2_feature_id_ui_port_map]->data;
        delete (LV2UI_Resize*)features[lv2_feature_id_ui_resize]->data;
#endif

        for (uint32_t i=0; i < kFeatureCount; ++i)
        {
            if (features[i] != nullptr)
                delete features[i];
        }

        for (size_t i=0; i < customURIDs.size(); ++i)
        {
            if (customURIDs[i])
                free((void*)customURIDs[i]);
        }

        customURIDs.clear();
    }

    // ---------------------------------------------------------------------
    // ui initialization

    bool uiInit(const char* pluginURI, const char* uiURI)
    {
        // -----------------------------------------------------------------
        // init

        CarlaBridgeClient::uiInit(pluginURI, uiURI);

        // -----------------------------------------------------------------
        // get plugin from lv2_rdf (lilv)

        gLv2World.init();
        rdf_descriptor = lv2_rdf_new(pluginURI);

        if (! rdf_descriptor)
            return false;

        // -----------------------------------------------------------------
        // find requested UI

        for (uint32_t i=0; i < rdf_descriptor->UICount; ++i)
        {
            if (std::strcmp(rdf_descriptor->UIs[i].URI, uiURI) == 0)
            {
                rdf_ui_descriptor = &rdf_descriptor->UIs[i];
                break;
            }
        }

        if (! rdf_ui_descriptor)
            return false;

        // -----------------------------------------------------------------
        // open DLL

        if (! uiLibOpen(rdf_ui_descriptor->Binary))
            return false;

        // -----------------------------------------------------------------
        // get DLL main entry

        const LV2UI_DescriptorFunction ui_descFn = (LV2UI_DescriptorFunction)uiLibSymbol("lv2ui_descriptor");

        if (! ui_descFn)
            return false;

        // -----------------------------------------------------------
        // get descriptor that matches URI

        uint32_t i = 0;
        while ((descriptor = ui_descFn(i++)))
        {
            if (std::strcmp(descriptor->URI, uiURI) == 0)
                break;
        }

        if (! descriptor)
            return false;

        // -----------------------------------------------------------
        // initialize UI

#ifdef BRIDGE_LV2_X11
        features[lv2_feature_id_ui_parent]->data = getContainerId();
#endif

        handle = descriptor->instantiate(descriptor, pluginURI, rdf_ui_descriptor->Bundle, carla_lv2_ui_write_function, this, &widget, features);

        if (! handle)
            return false;

        // -----------------------------------------------------------
        // check if not resizable

#ifndef BRIDGE_LV2_X11
        for (uint32_t i=0; i < rdf_ui_descriptor->FeatureCount; ++i)
        {
            if (std::strcmp(rdf_ui_descriptor->Features[i].URI, LV2_UI__fixedSize) == 0 || std::strcmp(rdf_ui_descriptor->Features[i].URI, LV2_UI__noUserResize) == 0)
            {
                m_resizable = false;
                break;
            }
        }
#endif

        // -----------------------------------------------------------
        // check for known extensions

        for (uint32_t i=0; descriptor->extension_data && i < rdf_ui_descriptor->ExtensionCount; ++i)
        {
            if (std::strcmp(rdf_ui_descriptor->Extensions[i], LV2_PROGRAMS__UIInterface) == 0)
            {
                programs = (LV2_Programs_UI_Interface*)descriptor->extension_data(LV2_PROGRAMS__UIInterface);

                if (programs && ! programs->select_program)
                    // invalid
                    programs = nullptr;

                break;
            }
        }

        return true;
    }

    void uiClose()
    {
        CarlaBridgeClient::uiClose();

        if (handle && descriptor && descriptor->cleanup)
            descriptor->cleanup(handle);

        uiLibClose();
    }

    // ---------------------------------------------------------------------
    // ui management

    void* getWidget() const
    {
        return widget;
    }

    bool isResizable() const
    {
        return m_resizable;
    }

    bool needsReparent() const
    {
#ifdef BRIDGE_LV2_X11
        return true;
#else
        return false;
#endif
    }

    // ---------------------------------------------------------------------
    // processing

    void setParameter(const int32_t rindex, const float value)
    {
        CARLA_ASSERT(handle && descriptor);

        if (handle && descriptor && descriptor->port_event)
        {
            float fvalue = value;
            descriptor->port_event(handle, rindex, sizeof(float), 0, &fvalue);
        }
    }

    void setProgram(const uint32_t)
    {
    }

    void setMidiProgram(const uint32_t bank, const uint32_t program)
    {
        CARLA_ASSERT(handle);

        if (handle && programs)
            programs->select_program(handle, bank, program);
    }

    void noteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
    {
        CARLA_ASSERT(handle && descriptor);

        if (handle && descriptor && descriptor->port_event)
        {
            LV2_Atom_MidiEvent midiEv;
            midiEv.event.time.frames = 0;
            midiEv.event.body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
            midiEv.event.body.size   = 3;
            midiEv.data[0] = MIDI_STATUS_NOTE_ON + channel;
            midiEv.data[1] = note;
            midiEv.data[2] = velo;

            descriptor->port_event(handle, 0, 3, CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, &midiEv);
        }
    }

    void noteOff(const uint8_t channel, const uint8_t note)
    {
        CARLA_ASSERT(handle && descriptor);

        if (handle && descriptor && descriptor->port_event)
        {
            LV2_Atom_MidiEvent midiEv;
            midiEv.event.time.frames = 0;
            midiEv.event.body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
            midiEv.event.body.size   = 3;
            midiEv.data[0] = MIDI_STATUS_NOTE_OFF + channel;
            midiEv.data[1] = note;
            midiEv.data[2] = 0;

            descriptor->port_event(handle, 0, 3, CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, &midiEv);
        }
    }

    // ---------------------------------------------------------------------

    uint32_t getCustomURID(const char* const uri)
    {
        carla_debug("CarlaLv2Client::getCustomURID(%s)", uri);
        CARLA_ASSERT(uri);

        if (! uri)
            return CARLA_URI_MAP_ID_NULL;

        for (size_t i=0; i < customURIDs.size(); ++i)
        {
            if (customURIDs[i] && std::strcmp(customURIDs[i], uri) == 0)
                return i;
        }

        customURIDs.push_back(strdup(uri));

        return customURIDs.size()-1;
    }

    const char* getCustomURIString(const LV2_URID urid) const
    {
        carla_debug("CarlaLv2Client::getCustomURIString(%i)", urid);
        CARLA_ASSERT(urid > CARLA_URI_MAP_ID_NULL);

        if (urid == CARLA_URI_MAP_ID_NULL)
            return nullptr;
        if (urid < customURIDs.size())
            return customURIDs[urid];

        return nullptr;
    }

    // ---------------------------------------------------------------------

    void handleAtomTransfer(const uint32_t portIndex, const LV2_Atom* const atom)
    {
        CARLA_ASSERT(atom != nullptr);
        carla_debug("CarlaLv2Client::handleTransferEvent(%i, %p)", portIndex, atom);

        if (atom != nullptr && handle != nullptr && descriptor != nullptr && descriptor->port_event != nullptr)
            descriptor->port_event(handle, portIndex, lv2_atom_total_size(atom), CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, atom);
    }

    void handleUridMap(const LV2_URID urid, const char* const uri)
    {
        CARLA_ASSERT(urid != CARLA_URI_MAP_ID_NULL);
        CARLA_ASSERT(uri != nullptr);
        carla_debug("CarlaLv2Client::handleUridMap(%i, \"%s\")", urid, uri);

        // TODO
    }

    // ---------------------------------------------------------------------

    void handleProgramChanged(int32_t /*index*/)
    {
        if (isOscControlRegistered())
            sendOscConfigure("reloadprograms", "");
    }

    uint32_t handleUiPortMap(const char* const symbol)
    {
        CARLA_ASSERT(symbol);

        if (! symbol)
            return LV2UI_INVALID_PORT_INDEX;

        for (uint32_t i=0; i < rdf_descriptor->PortCount; ++i)
        {
            if (std::strcmp(rdf_descriptor->Ports[i].Symbol, symbol) == 0)
                return i;
        }

        return LV2UI_INVALID_PORT_INDEX;
    }


    int handleUiResize(int width, int height)
    {
        CARLA_ASSERT(width > 0);
        CARLA_ASSERT(height > 0);

        if (width <= 0 || height <= 0)
            return 1;

        toolkitResize(width, height);

        return 0;
    }

    void handleUiWrite(uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
    {
        if (buffer == nullptr || ! isOscControlRegistered())
            return;

        if (format == 0)
        {
            CARLA_ASSERT(bufferSize == sizeof(float));

            if (bufferSize != sizeof(float))
                return;

            const float value(*(const float*)buffer);

            sendOscControl(portIndex, value);
        }
        else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM || CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT)
        {
            CARLA_ASSERT(bufferSize != 0);

            if (bufferSize == 0)
                return;

            QByteArray chunk((const char*)buffer, bufferSize);
            sendOscLv2AtomTransfer(portIndex, chunk.toBase64().constData());
        }
    }

    // ----------------- Event Feature ---------------------------------------------------

    static uint32_t carla_lv2_event_ref(const LV2_Event_Callback_Data callback_data, LV2_Event* const event)
    {
        carla_debug("CarlaLv2Client::carla_lv2_event_ref(%p, %p)", callback_data, event);
        CARLA_ASSERT(callback_data);
        CARLA_ASSERT(event);

        return 0;
    }

    static uint32_t carla_lv2_event_unref(const LV2_Event_Callback_Data callback_data, LV2_Event* const event)
    {
        carla_debug("CarlaLv2Client::carla_lv2_event_unref(%p, %p)", callback_data, event);
        CARLA_ASSERT(callback_data);
        CARLA_ASSERT(event);

        return 0;
    }

    // ----------------- Logs Feature ----------------------------------------------------

    static int carla_lv2_log_printf(const LV2_Log_Handle handle, const LV2_URID type, const char* const fmt, ...)
    {
        carla_debug("CarlaLv2Client::carla_lv2_log_printf(%p, %i, %s, ...)", handle, type, fmt);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(type > 0);

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

    static int carla_lv2_log_vprintf(const LV2_Log_Handle handle, const LV2_URID type, const char* const fmt, va_list ap)
    {
        carla_debug("CarlaLv2Client::carla_lv2_log_vprintf(%p, %i, %s, ...)", handle, type, fmt);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(type > 0);

#ifndef DEBUG
        if (type == CARLA_URI_MAP_ID_LOG_TRACE)
            return 0;
#endif

        char buf[8196];
        vsprintf(buf, fmt, ap);

        if (*buf == 0)
            return 0;

        switch (type)
        {
        case CARLA_URI_MAP_ID_LOG_ERROR:
            qCritical("%s", buf);
            break;
        case CARLA_URI_MAP_ID_LOG_NOTE:
            printf("%s\n", buf);
            break;
        case CARLA_URI_MAP_ID_LOG_TRACE:
            carla_debug("%s", buf);
            break;
        case CARLA_URI_MAP_ID_LOG_WARNING:
            carla_stderr("%s", buf);
            break;
        default:
            break;
        }

        return std::strlen(buf);
    }

    // ----------------- Programs Feature ------------------------------------------------

    static void carla_lv2_program_changed(const LV2_Programs_Handle handle, const int32_t index)
    {
        carla_debug("CarlaLv2Client::carla_lv2_program_changed(%p, %i)", handle, index);
        CARLA_ASSERT(handle);

        if (! handle)
            return;

        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        client->handleProgramChanged(index);
    }

    // ----------------- State Feature ---------------------------------------------------

    static char* carla_lv2_state_make_path(const LV2_State_Make_Path_Handle handle, const char* const path)
    {
        carla_debug("CarlaLv2Client::carla_lv2_state_make_path(%p, %p)", handle, path);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(path);

        if (! path)
            return nullptr;

        QDir dir;
        dir.mkpath(path);
        return strdup(path);
    }

    static char* carla_lv2_state_map_abstract_path(const LV2_State_Map_Path_Handle handle, const char* const absolute_path)
    {
        carla_debug("CarlaLv2Client::carla_lv2_state_map_abstract_path(%p, %p)", handle, absolute_path);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(absolute_path);

        if (! absolute_path)
            return nullptr;

        QDir dir(absolute_path);
        return strdup(dir.canonicalPath().toUtf8().constData());
    }

    static char* carla_lv2_state_map_absolute_path(const LV2_State_Map_Path_Handle handle, const char* const abstract_path)
    {
        carla_debug("CarlaLv2Client::carla_lv2_state_map_absolute_path(%p, %p)", handle, abstract_path);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(abstract_path);

        if (! abstract_path)
            return nullptr;

        QDir dir(abstract_path);
        return strdup(dir.absolutePath().toUtf8().constData());
    }

    // ----------------- URI-Map Feature ---------------------------------------

    static uint32_t carla_lv2_uri_to_id(const LV2_URI_Map_Callback_Data data, const char* const map, const char* const uri)
    {
        carla_debug("CarlaLv2Client::carla_lv2_uri_to_id(%p, %s, %s)", data, map, uri);
        return carla_lv2_urid_map((LV2_URID_Map_Handle*)data, uri);
    }

    // ----------------- URID Feature ------------------------------------------

    static LV2_URID carla_lv2_urid_map(const LV2_URID_Map_Handle handle, const char* const uri)
    {
        carla_debug("CarlaLv2Client::carla_lv2_urid_map(%p, %s)", handle, uri);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(uri);

        if (! uri)
            return CARLA_URI_MAP_ID_NULL;

        // Atom types
        if (std::strcmp(uri, LV2_ATOM__Blank) == 0)
            return CARLA_URI_MAP_ID_ATOM_BLANK;
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

        if (! handle)
            return CARLA_URI_MAP_ID_NULL;

        // Custom types
        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        return client->getCustomURID(uri);
    }

    static const char* carla_lv2_urid_unmap(const LV2_URID_Map_Handle handle, const LV2_URID urid)
    {
        carla_debug("CarlaLv2Client::carla_lv2_urid_unmap(%p, %i)", handle, urid);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(urid > CARLA_URI_MAP_ID_NULL);

        if (urid == CARLA_URI_MAP_ID_NULL)
            return nullptr;

        // Atom types
        if (urid == CARLA_URI_MAP_ID_ATOM_BLANK)
            return LV2_ATOM__Blank;
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

        if (! handle)
            return nullptr;

        // Custom types
        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        return client->getCustomURIString(urid);
    }

    // ----------------- UI Port-Map Feature ---------------------------------------------

    static uint32_t carla_lv2_ui_port_map(const LV2UI_Feature_Handle handle, const char* const symbol)
    {
        carla_debug("CarlaLv2Client::carla_lv2_ui_port_map(%p, %s)", handle, symbol);
        CARLA_ASSERT(handle);

        if (! handle)
            return LV2UI_INVALID_PORT_INDEX;

        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        return client->handleUiPortMap(symbol);
    }


    // ----------------- UI Resize Feature -------------------------------------

    static int carla_lv2_ui_resize(const LV2UI_Feature_Handle handle, const int width, const int height)
    {
        carla_debug("CarlaLv2Client::carla_lv2_ui_resize(%p, %i, %i)", handle, width, height);
        CARLA_ASSERT(handle);

        if (! handle)
            return 1;

        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        return client->handleUiResize(width, height);
    }

    // ----------------- UI Extension ------------------------------------------

    static void carla_lv2_ui_write_function(const LV2UI_Controller controller, const uint32_t port_index, const uint32_t buffer_size, const uint32_t format, const void* const buffer)
    {
        carla_debug("CarlaLv2Client::carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);
        CARLA_ASSERT(controller);

        if (! controller)
            return;

        CarlaLv2Client* const client = (CarlaLv2Client*)controller;
        client->handleUiWrite(port_index, buffer_size, format, buffer);
    }

private:
    LV2UI_Handle handle;
    LV2UI_Widget widget;
    const LV2UI_Descriptor* descriptor;
    LV2_Feature* features[kFeatureCount+1];

    const LV2_RDF_Descriptor* rdf_descriptor;
    const LV2_RDF_UI* rdf_ui_descriptor;

    const LV2_Programs_UI_Interface* programs;

    bool m_resizable;
    std::vector<const char*> customURIDs;
};

#define lv2ClientPtr ((CarlaLv2Client*)kClient)

int CarlaBridgeOsc::handleMsgLv2AtomTransfer(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "is");
    carla_debug("CarlaBridgeOsc::handleMsgLv2AtomTransfer()");

    if (kClient == nullptr)
        return 1;

    const int32_t   portIndex = argv[0]->i;
    const char* const atomBuf = (const char*)&argv[1]->s;

    if (portIndex < 0)
        return 0;

    QByteArray chunk;
    chunk = QByteArray::fromBase64(atomBuf);

    LV2_Atom* const atom = (LV2_Atom*)chunk.constData();

    lv2ClientPtr->handleAtomTransfer(portIndex, atom);
    return 0;
}

int CarlaBridgeOsc::handleMsgLv2UridMap(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "is");
    carla_debug("CarlaBridgeOsc::handleMsgLv2UridMap()");

    if (kClient == nullptr)
        return 1;

    const int32_t    urid = argv[0]->i;
    const char* const uri = (const char*)&argv[1]->s;

    if (urid <= 0)
        return 0;

    lv2ClientPtr->handleUridMap(urid, uri);
    return 0;
}

#undef lv2ClientPtr

CARLA_BRIDGE_END_NAMESPACE

int main(int argc, char* argv[])
{
    CARLA_BRIDGE_USE_NAMESPACE

    if (argc != 5)
    {
        carla_stderr("usage: %s <osc-url|\"null\"> <plugin-uri> <ui-uri> <ui-title>", argv[0]);
        return 1;
    }

    const char* oscUrl    = argv[1];
    const char* pluginURI = argv[2];
    const char* uiURI     = argv[3];
    const char* uiTitle   = argv[4];

    const bool useOsc = std::strcmp(oscUrl, "null");

    // try to get sampleRate value
    if (const char* const sampleRateStr = getenv("CARLA_SAMPLE_RATE"))
        gSampleRate = atof(sampleRateStr);

    // Init LV2 client
    CarlaLv2Client client(uiTitle);

    // Init OSC
    if (useOsc)
        client.oscInit(oscUrl);

    // Load UI
    int ret;

    if (client.uiInit(pluginURI, uiURI))
    {
        client.toolkitExec(!useOsc);
        ret = 0;
    }
    else
    {
        qCritical("Failed to load LV2 UI");
        ret = 1;
    }

    // Close OSC
    if (useOsc)
        client.oscClose();

    // Close LV2 client
    client.uiClose();

    return ret;
}
