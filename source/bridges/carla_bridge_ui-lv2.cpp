/*
 * Carla UI bridge code
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifdef BRIDGE_LV2

#include "carla_bridge_client.hpp"
#include "carla_lv2_utils.hpp"
#include "carla_midi.h"
#include "rtmempool/rtmempool.h"

#include <vector>
#include <QtCore/QDir>

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

// fake values
uint32_t bufferSize = 512;
double   sampleRate = 44100.0;

// static max values
const unsigned int MAX_EVENT_BUFFER = 8192; // 0x2000

// feature ids
const uint32_t lv2_feature_id_bufsize_bounded  =  0;
const uint32_t lv2_feature_id_bufsize_fixed    =  1;
const uint32_t lv2_feature_id_bufsize_powerof2 =  2;
const uint32_t lv2_feature_id_event            =  3;
const uint32_t lv2_feature_id_logs             =  4;
const uint32_t lv2_feature_id_options          =  5;
const uint32_t lv2_feature_id_programs         =  6;
const uint32_t lv2_feature_id_rtmempool        =  7;
const uint32_t lv2_feature_id_state_make_path  =  8;
const uint32_t lv2_feature_id_state_map_path   =  9;
const uint32_t lv2_feature_id_strict_bounds    = 10;
const uint32_t lv2_feature_id_uri_map          = 11;
const uint32_t lv2_feature_id_urid_map         = 12;
const uint32_t lv2_feature_id_urid_unmap       = 13;
const uint32_t lv2_feature_id_ui_parent        = 14;
const uint32_t lv2_feature_id_ui_port_map      = 15;
const uint32_t lv2_feature_id_ui_resize        = 16;
const uint32_t lv2_feature_count               = 17;

// pre-set uri[d] map ids
const uint32_t CARLA_URI_MAP_ID_NULL                = 0;
const uint32_t CARLA_URI_MAP_ID_ATOM_CHUNK          = 1;
const uint32_t CARLA_URI_MAP_ID_ATOM_DOUBLE         = 2;
const uint32_t CARLA_URI_MAP_ID_ATOM_INT            = 3;
const uint32_t CARLA_URI_MAP_ID_ATOM_PATH           = 4;
const uint32_t CARLA_URI_MAP_ID_ATOM_SEQUENCE       = 5;
const uint32_t CARLA_URI_MAP_ID_ATOM_STRING         = 6;
const uint32_t CARLA_URI_MAP_ID_ATOM_WORKER         = 7;
const uint32_t CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM  = 8;
const uint32_t CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT = 9;
const uint32_t CARLA_URI_MAP_ID_BUF_MAX_LENGTH      = 10;
const uint32_t CARLA_URI_MAP_ID_BUF_MIN_LENGTH      = 11;
const uint32_t CARLA_URI_MAP_ID_BUF_SEQUENCE_SIZE   = 12;
const uint32_t CARLA_URI_MAP_ID_LOG_ERROR           = 13;
const uint32_t CARLA_URI_MAP_ID_LOG_NOTE            = 14;
const uint32_t CARLA_URI_MAP_ID_LOG_TRACE           = 15;
const uint32_t CARLA_URI_MAP_ID_LOG_WARNING         = 16;
const uint32_t CARLA_URI_MAP_ID_MIDI_EVENT          = 17;
const uint32_t CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE   = 18;
const uint32_t CARLA_URI_MAP_ID_COUNT               = 19;

// -------------------------------------------------------------------------

struct Lv2PluginOptions {
    uint32_t eventSize;
    uint32_t bufferSize;
    double   sampleRate;
    LV2_Options_Option oNull;
    LV2_Options_Option oMaxBlockLenth;
    LV2_Options_Option oMinBlockLenth;
    LV2_Options_Option oSequenceSize;
    LV2_Options_Option oSampleRate;

    Lv2PluginOptions()
        : eventSize(MAX_EVENT_BUFFER),
          bufferSize(0),
          sampleRate(0.0) {}
};

Lv2PluginOptions lv2Options;

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

        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; i++)
            customURIDs.push_back(nullptr);

        for (uint32_t i=0; i < lv2_feature_count+1; i++)
            features[i] = nullptr;

        // -----------------------------------------------------------------
        // initialize options

        lv2Options.bufferSize = bufferSize;
        lv2Options.sampleRate = sampleRate;

        lv2Options.oNull.key   = CARLA_URI_MAP_ID_NULL;
        lv2Options.oNull.size  = 0;
        lv2Options.oNull.type  = CARLA_URI_MAP_ID_NULL;
        lv2Options.oNull.value = nullptr;

        lv2Options.oMaxBlockLenth.key   = CARLA_URI_MAP_ID_BUF_MAX_LENGTH;
        lv2Options.oMaxBlockLenth.size  = sizeof(uint32_t);
        lv2Options.oMaxBlockLenth.type  = CARLA_URI_MAP_ID_ATOM_INT;
        lv2Options.oMaxBlockLenth.value = &lv2Options.bufferSize;

        lv2Options.oMinBlockLenth.key   = CARLA_URI_MAP_ID_BUF_MIN_LENGTH;
        lv2Options.oMinBlockLenth.size  = sizeof(uint32_t);
        lv2Options.oMinBlockLenth.type  = CARLA_URI_MAP_ID_ATOM_INT;
        lv2Options.oMinBlockLenth.value = &lv2Options.bufferSize;

        lv2Options.oSequenceSize.key   = CARLA_URI_MAP_ID_BUF_SEQUENCE_SIZE;
        lv2Options.oSequenceSize.size  = sizeof(uint32_t);
        lv2Options.oSequenceSize.type  = CARLA_URI_MAP_ID_ATOM_INT;
        lv2Options.oSequenceSize.value = &lv2Options.eventSize;

        lv2Options.oSampleRate.key     = CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE;
        lv2Options.oSampleRate.size    = sizeof(double);
        lv2Options.oSampleRate.type    = CARLA_URI_MAP_ID_ATOM_DOUBLE;
        lv2Options.oSampleRate.value   = &lv2Options.sampleRate;

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
        rtmempool_allocator_init(rtMemPoolFt);

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

        LV2_Options_Option* const optionsFt = new LV2_Options_Option [5];
        optionsFt[0] = lv2Options.oMaxBlockLenth;
        optionsFt[1] = lv2Options.oMinBlockLenth;
        optionsFt[2] = lv2Options.oSequenceSize;
        optionsFt[3] = lv2Options.oSampleRate;
        optionsFt[4] = lv2Options.oNull;

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

        features[lv2_feature_id_uri_map]          = new LV2_Feature;
        features[lv2_feature_id_uri_map]->URI     = LV2_URI_MAP_URI;
        features[lv2_feature_id_uri_map]->data    = uriMapFt;

        features[lv2_feature_id_urid_map]         = new LV2_Feature;
        features[lv2_feature_id_urid_map]->URI    = LV2_URID__map;
        features[lv2_feature_id_urid_map]->data   = uridMapFt;

        features[lv2_feature_id_urid_unmap]       = new LV2_Feature;
        features[lv2_feature_id_urid_unmap]->URI  = LV2_URID__unmap;
        features[lv2_feature_id_urid_unmap]->data = uridUnmapFt;

        features[lv2_feature_id_ui_parent]         = new LV2_Feature;
        features[lv2_feature_id_ui_parent]->URI    = LV2_UI__parent;
#ifdef BRIDGE_LV2_X11
        features[lv2_feature_id_ui_parent]->data   = getContainerId();
#else
        features[lv2_feature_id_ui_parent]->data   = nullptr;
#endif

        features[lv2_feature_id_ui_port_map]           = new LV2_Feature;
        features[lv2_feature_id_ui_port_map]->URI      = LV2_UI__portMap;
        features[lv2_feature_id_ui_port_map]->data     = uiPortMapFt;

        features[lv2_feature_id_ui_resize]             = new LV2_Feature;
        features[lv2_feature_id_ui_resize]->URI        = LV2_UI__resize;
        features[lv2_feature_id_ui_resize]->data       = uiResizeFt;
    }

    ~CarlaLv2Client()
    {
        if (rdf_descriptor)
            delete rdf_descriptor;

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

        for (uint32_t i=0; i < lv2_feature_count; i++)
        {
            if (features[i])
                delete features[i];
        }

        for (size_t i=0; i < customURIDs.size(); i++)
        {
            if (customURIDs[i])
                free((void*)customURIDs[i]);
        }

        customURIDs.clear();
    }

    // ---------------------------------------------------------------------
    // ui initialization

    bool init(const char* pluginURI, const char* uiURI)
    {
        // -----------------------------------------------------------------
        // init

        CarlaBridgeClient::init(pluginURI, uiURI);

        // -----------------------------------------------------------------
        // get plugin from lv2_rdf (lilv)

        lv2World.init();
        rdf_descriptor = lv2_rdf_new(pluginURI);

        if (! rdf_descriptor)
            return false;

        // -----------------------------------------------------------------
        // find requested UI

        for (uint32_t i=0; i < rdf_descriptor->UICount; i++)
        {
            if (strcmp(rdf_descriptor->UIs[i].URI, uiURI) == 0)
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
            if (strcmp(descriptor->URI, uiURI) == 0)
                break;
        }

        if (! descriptor)
            return false;

        // -----------------------------------------------------------
        // initialize UI

        handle = descriptor->instantiate(descriptor, pluginURI, rdf_ui_descriptor->Bundle, carla_lv2_ui_write_function, this, &widget, features);

        if (! handle)
            return false;

        // -----------------------------------------------------------
        // check if not resizable

#ifndef BRIDGE_LV2_X11
        for (uint32_t i=0; i < rdf_ui_descriptor->FeatureCount; i++)
        {
            if (strcmp(rdf_ui_descriptor->Features[i].URI, LV2_UI__fixedSize) == 0 || strcmp(rdf_ui_descriptor->Features[i].URI, LV2_UI__noUserResize) == 0)
            {
                m_resizable = false;
                break;
            }
        }
#endif

        // -----------------------------------------------------------
        // check for known extensions

        for (uint32_t i=0; descriptor->extension_data && i < rdf_ui_descriptor->ExtensionCount; i++)
        {
            if (strcmp(rdf_ui_descriptor->Extensions[i], LV2_PROGRAMS__UIInterface) == 0)
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

    void close()
    {
        CarlaBridgeClient::close();

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

    void setParameter(const int32_t rindex, const double value)
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
        qDebug("CarlaLv2Client::getCustomURID(%s)", uri);
        CARLA_ASSERT(uri);

        if (! uri)
            return CARLA_URI_MAP_ID_NULL;

        for (size_t i=0; i < customURIDs.size(); i++)
        {
            if (customURIDs[i] && strcmp(customURIDs[i], uri) == 0)
                return i;
        }

        customURIDs.push_back(strdup(uri));

        return customURIDs.size()-1;
    }

    const char* getCustomURIString(const LV2_URID urid) const
    {
        qDebug("CarlaLv2Client::getCustomURIString(%i)", urid);
        CARLA_ASSERT(urid > CARLA_URI_MAP_ID_NULL);

        if (urid == CARLA_URI_MAP_ID_NULL)
            return nullptr;
        if (urid < customURIDs.size())
            return customURIDs[urid];

        return nullptr;
    }

    // ---------------------------------------------------------------------

    void handleTransferAtom(const int32_t portIndex, const LV2_Atom* const atom)
    {
        qDebug("CarlaLv2Client::handleTransferEvent(%i, %p)", portIndex, atom);
        CARLA_ASSERT(portIndex >= 0);
        CARLA_ASSERT(atom);

        if (atom && handle && descriptor && descriptor->port_event)
            descriptor->port_event(handle, portIndex, atom->size, CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, atom);
    }

    void handleTransferEvent(const int32_t portIndex, const LV2_Atom* const atom)
    {
        qDebug("CarlaLv2Client::handleTransferEvent(%i, %p)", portIndex, atom);
        CARLA_ASSERT(portIndex >= 0);
        CARLA_ASSERT(atom);

        if (atom && handle && descriptor && descriptor->port_event)
            descriptor->port_event(handle, portIndex, atom->size, CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);
#if 0
        if (handle && descriptor && descriptor->port_event)
        {
            LV2_URID_Map* const URID_Map = (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;
            const LV2_URID uridPatchSet  = getCustomURID(LV2_PATCH__Set);
            const LV2_URID uridPatchBody = getCustomURID(LV2_PATCH__body);

            Sratom*   sratom = sratom_new(URID_Map);
            SerdChunk chunk  = { nullptr, 0 };

            LV2_Atom_Forge forge;
            lv2_atom_forge_init(&forge, URID_Map);
            lv2_atom_forge_set_sink(&forge, sratom_forge_sink, sratom_forge_deref, &chunk);

            LV2_Atom_Forge_Frame refFrame, bodyFrame;
            LV2_Atom_Forge_Ref   ref = lv2_atom_forge_blank(&forge, &refFrame, 1, uridPatchSet);

            lv2_atom_forge_property_head(&forge, uridPatchBody, CARLA_URI_MAP_ID_NULL);
            lv2_atom_forge_blank(&forge, &bodyFrame, 2, CARLA_URI_MAP_ID_NULL);

            //lv2_atom_forge_property_head(&forge, getCustomURID(key), CARLA_URI_MAP_ID_NULL);

            if (strcmp(type, "string") == 0)
                lv2_atom_forge_string(&forge, value, strlen(value));
            else if (strcmp(type, "path") == 0)
                lv2_atom_forge_path(&forge, value, strlen(value));
            else if (strcmp(type, "chunk") == 0)
                lv2_atom_forge_literal(&forge, value, strlen(value), CARLA_URI_MAP_ID_ATOM_CHUNK, CARLA_URI_MAP_ID_NULL);
            //else
            //    lv2_atom_forge_literal(&forge, value, strlen(value), getCustomURID(key), CARLA_URI_MAP_ID_NULL);

            lv2_atom_forge_pop(&forge, &bodyFrame);
            lv2_atom_forge_pop(&forge, &refFrame);

            const LV2_Atom* const atom = lv2_atom_forge_deref(&forge, ref);
            descriptor->port_event(handle, 0, atom->size, CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);

            free((void*)chunk.buf);
            sratom_free(sratom);
        }
#endif
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

        for (uint32_t i=0; i < rdf_descriptor->PortCount; i++)
        {
            if (strcmp(rdf_descriptor->Ports[i].Symbol, symbol) == 0)
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
        if (! (buffer && isOscControlRegistered()))
            return;

        if (format == 0)
        {
            CARLA_ASSERT(buffer);
            CARLA_ASSERT(bufferSize == sizeof(float));

            if (bufferSize != sizeof(float))
                return;

            float value = *(float*)buffer;
            sendOscControl(portIndex, value);
        }
        else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM)
        {
            CARLA_ASSERT(buffer);
            const LV2_Atom* const atom = (const LV2_Atom*)buffer;

            QByteArray chunk((const char*)buffer, bufferSize);
            sendOscLv2TransferAtom(portIndex, getCustomURIString(atom->type), chunk.toBase64().constData());
        }
        else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT)
        {
            CARLA_ASSERT(buffer);
            const LV2_Atom* const atom = (const LV2_Atom*)buffer;

            QByteArray chunk((const char*)buffer, bufferSize);
            sendOscLv2TransferEvent(portIndex, getCustomURIString(atom->type), chunk.toBase64().constData());
        }
    }

    // ----------------- Event Feature ---------------------------------------------------

    static uint32_t carla_lv2_event_ref(const LV2_Event_Callback_Data callback_data, LV2_Event* const event)
    {
        qDebug("CarlaLv2Client::carla_lv2_event_ref(%p, %p)", callback_data, event);
        CARLA_ASSERT(callback_data);
        CARLA_ASSERT(event);

        return 0;
    }

    static uint32_t carla_lv2_event_unref(const LV2_Event_Callback_Data callback_data, LV2_Event* const event)
    {
        qDebug("CarlaLv2Client::carla_lv2_event_unref(%p, %p)", callback_data, event);
        CARLA_ASSERT(callback_data);
        CARLA_ASSERT(event);

        return 0;
    }

    // ----------------- Logs Feature ----------------------------------------------------

    static int carla_lv2_log_printf(const LV2_Log_Handle handle, const LV2_URID type, const char* const fmt, ...)
    {
        qDebug("CarlaLv2Client::carla_lv2_log_printf(%p, %i, %s, ...)", handle, type, fmt);
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
        qDebug("CarlaLv2Client::carla_lv2_log_vprintf(%p, %i, %s, ...)", handle, type, fmt);
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
            qDebug("%s", buf);
            break;
        case CARLA_URI_MAP_ID_LOG_WARNING:
            qWarning("%s", buf);
            break;
        default:
            break;
        }

        return strlen(buf);
    }

    // ----------------- Programs Feature ------------------------------------------------

    static void carla_lv2_program_changed(const LV2_Programs_Handle handle, const int32_t index)
    {
        qDebug("CarlaLv2Client::carla_lv2_program_changed(%p, %i)", handle, index);
        CARLA_ASSERT(handle);

        if (! handle)
            return;

        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        client->handleProgramChanged(index);
    }

    // ----------------- State Feature ---------------------------------------------------

    static char* carla_lv2_state_make_path(const LV2_State_Make_Path_Handle handle, const char* const path)
    {
        qDebug("CarlaLv2Client::carla_lv2_state_make_path(%p, %p)", handle, path);
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
        qDebug("CarlaLv2Client::carla_lv2_state_map_abstract_path(%p, %p)", handle, absolute_path);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(absolute_path);

        if (! absolute_path)
            return nullptr;

        QDir dir(absolute_path);
        return strdup(dir.canonicalPath().toUtf8().constData());
    }

    static char* carla_lv2_state_map_absolute_path(const LV2_State_Map_Path_Handle handle, const char* const abstract_path)
    {
        qDebug("CarlaLv2Client::carla_lv2_state_map_absolute_path(%p, %p)", handle, abstract_path);
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
        qDebug("CarlaLv2Client::carla_lv2_uri_to_id(%p, %s, %s)", data, map, uri);
        return carla_lv2_urid_map((LV2_URID_Map_Handle*)data, uri);
    }

    // ----------------- URID Feature ------------------------------------------

    static LV2_URID carla_lv2_urid_map(const LV2_URID_Map_Handle handle, const char* const uri)
    {
        qDebug("CarlaLv2Client::carla_lv2_urid_map(%p, %s)", handle, uri);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(uri);

        if (! uri)
            return CARLA_URI_MAP_ID_NULL;

        // Atom types
        if (strcmp(uri, LV2_ATOM__Chunk) == 0)
            return CARLA_URI_MAP_ID_ATOM_CHUNK;
        if (strcmp(uri, LV2_ATOM__Double) == 0)
            return CARLA_URI_MAP_ID_ATOM_DOUBLE;
        if (strcmp(uri, LV2_ATOM__Int) == 0)
            return CARLA_URI_MAP_ID_ATOM_INT;
        if (strcmp(uri, LV2_ATOM__Path) == 0)
            return CARLA_URI_MAP_ID_ATOM_PATH;
        if (strcmp(uri, LV2_ATOM__Sequence) == 0)
            return CARLA_URI_MAP_ID_ATOM_SEQUENCE;
        if (strcmp(uri, LV2_ATOM__String) == 0)
            return CARLA_URI_MAP_ID_ATOM_STRING;
        if (strcmp(uri, LV2_ATOM__atomTransfer) == 0)
            return CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM;
        if (strcmp(uri, LV2_ATOM__eventTransfer) == 0)
            return CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT;

        // BufSize types
        if (strcmp(uri, LV2_BUF_SIZE__maxBlockLength) == 0)
            return CARLA_URI_MAP_ID_BUF_MAX_LENGTH;
        if (strcmp(uri, LV2_BUF_SIZE__minBlockLength) == 0)
            return CARLA_URI_MAP_ID_BUF_MIN_LENGTH;
        if (strcmp(uri, LV2_BUF_SIZE__sequenceSize) == 0)
            return CARLA_URI_MAP_ID_BUF_SEQUENCE_SIZE;

        // Log types
        if (strcmp(uri, LV2_LOG__Error) == 0)
            return CARLA_URI_MAP_ID_LOG_ERROR;
        if (strcmp(uri, LV2_LOG__Note) == 0)
            return CARLA_URI_MAP_ID_LOG_NOTE;
        if (strcmp(uri, LV2_LOG__Trace) == 0)
            return CARLA_URI_MAP_ID_LOG_TRACE;
        if (strcmp(uri, LV2_LOG__Warning) == 0)
            return CARLA_URI_MAP_ID_LOG_WARNING;

        // Others
        if (strcmp(uri, LV2_MIDI__MidiEvent) == 0)
            return CARLA_URI_MAP_ID_MIDI_EVENT;
        if (strcmp(uri, LV2_PARAMETERS__sampleRate) == 0)
            return CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE;

        if (! handle)
            return CARLA_URI_MAP_ID_NULL;

        // Custom types
        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        return client->getCustomURID(uri);
    }

    static const char* carla_lv2_urid_unmap(const LV2_URID_Map_Handle handle, const LV2_URID urid)
    {
        qDebug("CarlaLv2Client::carla_lv2_urid_unmap(%p, %i)", handle, urid);
        CARLA_ASSERT(handle);
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

        if (! handle)
            return nullptr;

        // Custom types
        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        return client->getCustomURIString(urid);
    }

    // ----------------- UI Port-Map Feature ---------------------------------------------

    static uint32_t carla_lv2_ui_port_map(const LV2UI_Feature_Handle handle, const char* const symbol)
    {
        qDebug("CarlaLv2Client::carla_lv2_ui_port_map(%p, %s)", handle, symbol);
        CARLA_ASSERT(handle);

        if (! handle)
            return LV2UI_INVALID_PORT_INDEX;

        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        return client->handleUiPortMap(symbol);
    }


    // ----------------- UI Resize Feature -------------------------------------

    static int carla_lv2_ui_resize(const LV2UI_Feature_Handle handle, const int width, const int height)
    {
        qDebug("CarlaLv2Client::carla_lv2_ui_resize(%p, %i, %i)", handle, width, height);
        CARLA_ASSERT(handle);

        if (! handle)
            return 1;

        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        return client->handleUiResize(width, height);
    }

    // ----------------- UI Extension ------------------------------------------

    static void carla_lv2_ui_write_function(const LV2UI_Controller controller, const uint32_t port_index, const uint32_t buffer_size, const uint32_t format, const void* const buffer)
    {
        qDebug("CarlaLv2Client::carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);
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
    LV2_Feature* features[lv2_feature_count+1];

    const LV2_RDF_Descriptor* rdf_descriptor;
    const LV2_RDF_UI* rdf_ui_descriptor;

    const LV2_Programs_UI_Interface* programs;

    bool m_resizable;
    std::vector<const char*> customURIDs;
};

int CarlaBridgeOsc::handleMsgLv2TransferAtom(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgLv2TransferAtom()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(3, "iss");

    if (! client)
        return 1;

    const int32_t portIndex   = argv[0]->i;
    const char* const typeStr = (const char*)&argv[1]->s;
    const char* const atomBuf = (const char*)&argv[2]->s;

    QByteArray chunk;
    chunk = QByteArray::fromBase64(atomBuf);

    LV2_Atom* const atom = (LV2_Atom*)chunk.constData();
    CarlaLv2Client* const lv2Client = (CarlaLv2Client*)client;

    atom->type = lv2Client->getCustomURID(typeStr);
    lv2Client->handleTransferAtom(portIndex, atom);

    return 0;
}

int CarlaBridgeOsc::handleMsgLv2TransferEvent(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgLv2TransferEvent()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(3, "iss");

    if (! client)
        return 1;

    const int32_t portIndex   = argv[0]->i;
    const char* const typeStr = (const char*)&argv[1]->s;
    const char* const atomBuf = (const char*)&argv[2]->s;

    QByteArray chunk;
    chunk = QByteArray::fromBase64(atomBuf);

    LV2_Atom* const atom = (LV2_Atom*)chunk.constData();
    CarlaLv2Client* const lv2Client = (CarlaLv2Client*)client;

    atom->type = lv2Client->getCustomURID(typeStr);
    lv2Client->handleTransferEvent(portIndex, atom);

    return 0;
}

CARLA_BRIDGE_END_NAMESPACE

int main(int argc, char* argv[])
{
    CARLA_BRIDGE_USE_NAMESPACE

    if (argc != 5)
    {
        qWarning("usage: %s <osc-url|\"null\"> <plugin-uri> <ui-uri> <ui-title>", argv[0]);
        return 1;
    }

    const char* oscUrl    = argv[1];
    const char* pluginURI = argv[2];
    const char* uiURI     = argv[3];
    const char* uiTitle   = argv[4];

    const bool useOsc = strcmp(oscUrl, "null");

    // try to get sampleRate value
    const char* const sampleRateStr = getenv("CARLA_SAMPLE_RATE");

    if (sampleRateStr)
        sampleRate = atof(sampleRateStr);

    // Init LV2 client
    CarlaLv2Client client(uiTitle);

    // Init OSC
    if (useOsc && ! client.oscInit(oscUrl))
    {
        return -1;
    }

    // Load UI
    int ret;

    if (client.init(pluginURI, uiURI))
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
    {
        client.oscClose();
    }

    // Close LV2 client
    client.close();

    return ret;
}

#endif // BRIDGE_LV2
