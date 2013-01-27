/*
 * Carla LV2 Plugin
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

#include "carla_plugin.hpp"

#ifdef WANT_LV2

#include "carla_lib_utils.hpp"
#include "carla_lv2_utils.hpp"

#include "lv2_atom_queue.hpp"
#include "rtmempool/rtmempool.h"

#include <set>

#include <QtCore/QDir>
#include <QtGui/QDialog>
#include <QtGui/QLayout>

#ifdef WANT_SUIL
#include <suil/suil.h>
struct SuilInstanceImpl {
    void*                   lib_handle;
    const LV2UI_Descriptor* descriptor;
    LV2UI_Handle            handle;
    // ...
};
#endif

CARLA_BACKEND_START_NAMESPACE

/*!
 * @defgroup CarlaBackendLv2Plugin Carla Backend LV2 Plugin
 *
 * The Carla Backend LV2 Plugin.\n
 * http://lv2plug.in/
 * @{
 */

// static max values
const unsigned int MAX_EVENT_BUFFER = 8192; // 0x2000

/*!
 * @defgroup PluginHints Plugin Hints
 * @{
 */
const unsigned int PLUGIN_HAS_EXTENSION_PROGRAMS = 0x1000; //!< LV2 Plugin has Programs extension
const unsigned int PLUGIN_HAS_EXTENSION_STATE    = 0x2000; //!< LV2 Plugin has State extension
const unsigned int PLUGIN_HAS_EXTENSION_WORKER   = 0x4000; //!< LV2 Plugin has Worker extension
/**@}*/

/*!
 * @defgroup ParameterHints Parameter Hints
 * @{
 */
const unsigned int PARAMETER_IS_STRICT_BOUNDS = 0x1000; //!< LV2 Parameter needs strict bounds
const unsigned int PARAMETER_IS_TRIGGER       = 0x2000; //!< LV2 Parameter is trigger (current value should be changed to its default after run())
/**@}*/

/*!
 * @defgroup Lv2FeatureIds LV2 Feature Ids
 *
 * Static index list of the internal LV2 Feature Ids.
 * @{
 */
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
const uint32_t lv2_feature_id_worker           = 14;
const uint32_t lv2_feature_id_data_access      = 15;
const uint32_t lv2_feature_id_instance_access  = 16;
const uint32_t lv2_feature_id_ui_parent        = 17;
const uint32_t lv2_feature_id_ui_port_map      = 18;
const uint32_t lv2_feature_id_ui_resize        = 19;
const uint32_t lv2_feature_id_external_ui      = 20;
const uint32_t lv2_feature_id_external_ui_old  = 21;
const uint32_t lv2_feature_count               = 22;
/**@}*/

/*!
 * @defgroup Lv2EventTypes LV2 Event Data/Types
 *
 * Data and buffer types for LV2 EventData ports.
 * @{
 */
const unsigned int CARLA_EVENT_DATA_ATOM      = 0x01;
const unsigned int CARLA_EVENT_DATA_EVENT     = 0x02;
const unsigned int CARLA_EVENT_DATA_MIDI_LL   = 0x04;
const unsigned int CARLA_EVENT_TYPE_MESSAGE   = 0x10;
const unsigned int CARLA_EVENT_TYPE_MIDI      = 0x20;
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
/**@}*/

struct Lv2EventData {
    uint32_t type;
    uint32_t rindex;
    CarlaEngineMidiPort* port;
    union {
        LV2_Atom_Sequence* atom;
        LV2_Event_Buffer* event;
        LV2_MIDI* midi;
    };

    Lv2EventData()
        : type(0x0),
          rindex(0),
          port(nullptr) {}
};

struct Lv2PluginEventData {
    uint32_t count;
    Lv2EventData* data;

    Lv2PluginEventData()
        : count(0),
          data(nullptr) {}
};

struct Lv2PluginOptions {
    bool init;
    uint32_t eventSize;
    uint32_t bufferSize;
    double   sampleRate;
    LV2_Options_Option oNull;
    LV2_Options_Option oMaxBlockLenth;
    LV2_Options_Option oMinBlockLenth;
    LV2_Options_Option oSequenceSize;
    LV2_Options_Option oSampleRate;

    Lv2PluginOptions()
        : init(false),
          eventSize(MAX_EVENT_BUFFER),
          bufferSize(0),
          sampleRate(0.0) {}
};

Lv2PluginOptions lv2Options;

LV2_Atom_Event* getLv2AtomEvent(LV2_Atom_Sequence* const atom, const uint32_t offset)
{
    return (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, atom) + offset);
}

class Lv2Plugin : public CarlaPlugin
{
public:
    Lv2Plugin(CarlaEngine* const engine, const unsigned short id)
        : CarlaPlugin(engine, id)
    {
        qDebug("Lv2Plugin::Lv2Plugin()");

        m_type   = PLUGIN_LV2;
        m_count += 1;

        handle = h2 = nullptr;
        descriptor  = nullptr;
        rdf_descriptor = nullptr;

        ext.state      = nullptr;
        ext.worker     = nullptr;
        ext.programs   = nullptr;
        ext.uiprograms = nullptr;

        ui.lib = nullptr;
        ui.handle = nullptr;
        ui.descriptor = nullptr;
        ui.rdf_descriptor = nullptr;

        evIn.count = 0;
        evIn.data  = nullptr;

        evOut.count = 0;
        evOut.data  = nullptr;

        paramBuffers = nullptr;

        gui.type = GUI_NONE;
        gui.resizable = false;
        gui.width  = 0;
        gui.height = 0;

#ifdef WANT_SUIL
        suil.handle = nullptr;
        suil.host   = nullptr;
#endif

        lastTimePosPlaying = false;
        lastTimePosFrame   = 0;

        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; i++)
            customURIDs.push_back(nullptr);

        for (uint32_t i=0; i < lv2_feature_count+1; i++)
            features[i] = nullptr;

        if (! lv2Options.init)
        {
            lv2Options.init = true;
            lv2Options.bufferSize = x_engine->getBufferSize();
            lv2Options.sampleRate = x_engine->getSampleRate();

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
        }

        // init static data if this is the first lv2-plugin loaded
        if (m_count == 1)
        {
            ft.event                  = new LV2_Event_Feature;
            ft.event->callback_data   = this;
            ft.event->lv2_event_ref   = carla_lv2_event_ref;
            ft.event->lv2_event_unref = carla_lv2_event_unref;

            ft.log          = new LV2_Log_Log;
            ft.log->handle  = this;
            ft.log->printf  = carla_lv2_log_printf;
            ft.log->vprintf = carla_lv2_log_vprintf;

            ft.rtMemPool = new LV2_RtMemPool_Pool;
            rtmempool_allocator_init(ft.rtMemPool);

            ft.stateMakePath         = new LV2_State_Make_Path;
            ft.stateMakePath->handle = this;
            ft.stateMakePath->path   = carla_lv2_state_make_path;

            ft.stateMapPath                = new LV2_State_Map_Path;
            ft.stateMapPath->handle        = this;
            ft.stateMapPath->abstract_path = carla_lv2_state_map_abstract_path;
            ft.stateMapPath->absolute_path = carla_lv2_state_map_absolute_path;

            ft.options    = new LV2_Options_Option [5];
            ft.options[0] = lv2Options.oMaxBlockLenth;
            ft.options[1] = lv2Options.oMinBlockLenth;
            ft.options[2] = lv2Options.oSequenceSize;
            ft.options[3] = lv2Options.oSampleRate;
            ft.options[4] = lv2Options.oNull;
        }

        lv2World.init();
    }

    ~Lv2Plugin()
    {
        qDebug("Lv2Plugin::~Lv2Plugin()");
        m_count -= 1;

        // close UI
        if (m_hints & PLUGIN_HAS_GUI)
        {
            showGui(false);

            switch (gui.type)
            {
            case GUI_NONE:
            case GUI_INTERNAL_QT4:
            case GUI_INTERNAL_COCOA:
            case GUI_INTERNAL_HWND:
            case GUI_INTERNAL_X11:
            case GUI_EXTERNAL_LV2:
                break;

            case GUI_EXTERNAL_SUIL:
#ifdef WANT_SUIL
                if (ui.widget)
                    ((QWidget*)ui.widget)->close();
#endif
                break;

            case GUI_EXTERNAL_OSC:
                if (osc.thread)
                {
                    // Wait a bit first, try safe quit, then force kill
                    if (osc.thread->isRunning() && ! osc.thread->wait(x_engine->getOptions().oscUiTimeout))
                    {
                        qWarning("Failed to properly stop LV2 OSC GUI thread");
                        osc.thread->terminate();
                    }

                    delete osc.thread;
                }
                break;
            }

#ifdef WANT_SUIL
            if (suil.handle)
            {
                suil_instance_free(suil.handle);

                if (suil.host)
                    suil_host_free(suil.host);

                ui.handle = nullptr;
                ui.descriptor = nullptr;
            }
#endif

            if (ui.handle && ui.descriptor && ui.descriptor->cleanup)
                ui.descriptor->cleanup(ui.handle);

            if (features[lv2_feature_id_data_access] && features[lv2_feature_id_data_access]->data)
                delete (LV2_Extension_Data_Feature*)features[lv2_feature_id_data_access]->data;

            if (features[lv2_feature_id_ui_port_map] && features[lv2_feature_id_ui_port_map]->data)
                delete (LV2UI_Port_Map*)features[lv2_feature_id_ui_port_map]->data;

            if (features[lv2_feature_id_ui_resize] && features[lv2_feature_id_ui_resize]->data)
                delete (LV2UI_Resize*)features[lv2_feature_id_ui_resize]->data;

            if (features[lv2_feature_id_external_ui] && features[lv2_feature_id_external_ui]->data)
            {
                const LV2_External_UI_Host* const uiHost = (const LV2_External_UI_Host*)features[lv2_feature_id_external_ui]->data;

                if (uiHost->plugin_human_id)
                    free((void*)uiHost->plugin_human_id);

                delete uiHost;
            }

            uiLibClose();
        }

        if (descriptor)
        {
            if (descriptor->deactivate && m_activeBefore)
            {
                if (handle)
                    descriptor->deactivate(handle);
                if (h2)
                    descriptor->deactivate(h2);
            }

            if (descriptor->cleanup)
            {
                if (handle)
                    descriptor->cleanup(handle);
                if (h2)
                    descriptor->cleanup(h2);
            }
        }

        if (rdf_descriptor)
            delete rdf_descriptor;

        if (features[lv2_feature_id_programs] && features[lv2_feature_id_programs]->data)
            delete (LV2_Programs_Host*)features[lv2_feature_id_programs]->data;

        if (features[lv2_feature_id_uri_map] && features[lv2_feature_id_uri_map]->data)
            delete (LV2_URI_Map_Feature*)features[lv2_feature_id_uri_map]->data;

        if (features[lv2_feature_id_urid_map] && features[lv2_feature_id_urid_map]->data)
            delete (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;

        if (features[lv2_feature_id_urid_unmap] && features[lv2_feature_id_urid_unmap]->data)
            delete (LV2_URID_Unmap*)features[lv2_feature_id_urid_unmap]->data;

        if (features[lv2_feature_id_worker] && features[lv2_feature_id_worker]->data)
            delete (LV2_Worker_Schedule*)features[lv2_feature_id_worker]->data;

        if (! x_engine->getOptions().processHighPrecision)
        {
            features[lv2_feature_id_bufsize_fixed]    = nullptr;
            features[lv2_feature_id_bufsize_powerof2] = nullptr;
        }

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

        // cleanup all static data if this is the last lv2-plugin loaded
        if (m_count == 0)
        {
            for (auto it = libDescs.begin(); it != libDescs.end(); it++)
            {
                const LV2_Lib_Descriptor* libDesc(*it);
                libDesc->cleanup(libDesc->handle);
            }

            libDescs.clear();

            if (ft.event)
                delete ft.event;

            if (ft.log)
                delete ft.log;

            if (ft.options)
                delete[] ft.options;

            if (ft.rtMemPool)
                delete ft.rtMemPool;

            if (ft.stateMakePath)
                delete ft.stateMakePath;

            if (ft.stateMapPath)
                delete ft.stateMapPath;

            ft.event = nullptr;
            ft.log   = nullptr;
            ft.options   = nullptr;
            ft.rtMemPool = nullptr;
            ft.stateMakePath = nullptr;
            ft.stateMapPath  = nullptr;
        }
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        CARLA_ASSERT(rdf_descriptor);

        if (rdf_descriptor)
        {
            LV2_Property cat1 = rdf_descriptor->Type[0];
            LV2_Property cat2 = rdf_descriptor->Type[1];

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
        }

        return getPluginCategoryFromName(m_name);
    }

    long uniqueId()
    {
        CARLA_ASSERT(rdf_descriptor);

        return rdf_descriptor ? rdf_descriptor->UniqueID : 0;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t midiInCount()
    {
        uint32_t i, count = 0;

        for (i=0; i < evIn.count; i++)
        {
            if (evIn.data[i].type & CARLA_EVENT_TYPE_MIDI)
                count += 1;
        }

        return count;
    }

    uint32_t midiOutCount()
    {
        uint32_t i, count = 0;

        for (i=0; i < evOut.count; i++)
        {
            if (evOut.data[i].type & CARLA_EVENT_TYPE_MIDI)
                count += 1;
        }

        return count;
    }

    uint32_t parameterScalePointCount(const uint32_t parameterId)
    {
        CARLA_ASSERT(parameterId < param.count);
        CARLA_ASSERT(rdf_descriptor);

        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LV2_RDF_Port* const port = &rdf_descriptor->Ports[rindex];
            return port->ScalePointCount;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double getParameterValue(const uint32_t parameterId)
    {
        CARLA_ASSERT(parameterId < param.count);

        return paramBuffers[parameterId];
    }

    double getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId)
    {
        CARLA_ASSERT(parameterId < param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LV2_RDF_Port* const port = &rdf_descriptor->Ports[rindex];

            if (scalePointId < port->ScalePointCount)
            {
                const LV2_RDF_PortScalePoint* const portScalePoint = &port->ScalePoints[scalePointId];
                return portScalePoint->Value;
            }
        }

        return 0.0;
    }

    void getLabel(char* const strBuf)
    {
        CARLA_ASSERT(rdf_descriptor);

        if (rdf_descriptor && rdf_descriptor->URI)
            strncpy(strBuf, rdf_descriptor->URI, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        CARLA_ASSERT(rdf_descriptor);

        if (rdf_descriptor && rdf_descriptor->Author)
            strncpy(strBuf, rdf_descriptor->Author, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        CARLA_ASSERT(rdf_descriptor);

        if (rdf_descriptor && rdf_descriptor->License)
            strncpy(strBuf, rdf_descriptor->License, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        CARLA_ASSERT(rdf_descriptor);

        if (rdf_descriptor && rdf_descriptor->Name)
            strncpy(strBuf, rdf_descriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(rdf_descriptor);
        CARLA_ASSERT(parameterId < param.count);

        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
            strncpy(strBuf, rdf_descriptor->Ports[rindex].Name, STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterSymbol(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(rdf_descriptor);
        CARLA_ASSERT(parameterId < param.count);

        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
            strncpy(strBuf, rdf_descriptor->Ports[rindex].Symbol, STR_MAX);
        else
            CarlaPlugin::getParameterSymbol(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(rdf_descriptor);
        CARLA_ASSERT(parameterId < param.count);

        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LV2_RDF_Port* const port = &rdf_descriptor->Ports[rindex];

            if (LV2_HAVE_PORT_UNIT_SYMBOL(port->Unit.Hints) && port->Unit.Symbol)
                strncpy(strBuf, port->Unit.Symbol, STR_MAX);

            else if (LV2_HAVE_PORT_UNIT_UNIT(port->Unit.Hints))
            {
                switch (port->Unit.Unit)
                {
                case LV2_PORT_UNIT_BAR:
                    strncpy(strBuf, "bars", STR_MAX);
                    return;
                case LV2_PORT_UNIT_BEAT:
                    strncpy(strBuf, "beats", STR_MAX);
                    return;
                case LV2_PORT_UNIT_BPM:
                    strncpy(strBuf, "BPM", STR_MAX);
                    return;
                case LV2_PORT_UNIT_CENT:
                    strncpy(strBuf, "ct", STR_MAX);
                    return;
                case LV2_PORT_UNIT_CM:
                    strncpy(strBuf, "cm", STR_MAX);
                    return;
                case LV2_PORT_UNIT_COEF:
                    strncpy(strBuf, "(coef)", STR_MAX);
                    return;
                case LV2_PORT_UNIT_DB:
                    strncpy(strBuf, "dB", STR_MAX);
                    return;
                case LV2_PORT_UNIT_DEGREE:
                    strncpy(strBuf, "deg", STR_MAX);
                    return;
                case LV2_PORT_UNIT_FRAME:
                    strncpy(strBuf, "frames", STR_MAX);
                    return;
                case LV2_PORT_UNIT_HZ:
                    strncpy(strBuf, "Hz", STR_MAX);
                    return;
                case LV2_PORT_UNIT_INCH:
                    strncpy(strBuf, "in", STR_MAX);
                    return;
                case LV2_PORT_UNIT_KHZ:
                    strncpy(strBuf, "kHz", STR_MAX);
                    return;
                case LV2_PORT_UNIT_KM:
                    strncpy(strBuf, "km", STR_MAX);
                    return;
                case LV2_PORT_UNIT_M:
                    strncpy(strBuf, "m", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MHZ:
                    strncpy(strBuf, "MHz", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MIDINOTE:
                    strncpy(strBuf, "note", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MILE:
                    strncpy(strBuf, "mi", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MIN:
                    strncpy(strBuf, "min", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MM:
                    strncpy(strBuf, "mm", STR_MAX);
                    return;
                case LV2_PORT_UNIT_MS:
                    strncpy(strBuf, "ms", STR_MAX);
                    return;
                case LV2_PORT_UNIT_OCT:
                    strncpy(strBuf, "oct", STR_MAX);
                    return;
                case LV2_PORT_UNIT_PC:
                    strncpy(strBuf, "%", STR_MAX);
                    return;
                case LV2_PORT_UNIT_S:
                    strncpy(strBuf, "s", STR_MAX);
                    return;
                case LV2_PORT_UNIT_SEMITONE:
                    strncpy(strBuf, "semi", STR_MAX);
                    return;
                }
            }
        }

        CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf)
    {
        CARLA_ASSERT(rdf_descriptor);
        CARLA_ASSERT(parameterId < param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LV2_RDF_Port* const port = &rdf_descriptor->Ports[rindex];

            if (scalePointId < port->ScalePointCount)
            {
                const LV2_RDF_PortScalePoint* const portScalePoint = &port->ScalePoints[scalePointId];

                if (portScalePoint->Label)
                {
                    strncpy(strBuf, portScalePoint->Label, STR_MAX);
                    return;
                }
            }
        }

        CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    }

    void getGuiInfo(GuiType* const type, bool* const resizable)
    {
        CARLA_ASSERT(type);
        CARLA_ASSERT(resizable);

        *type      = gui.type;
        *resizable = gui.resizable;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, double value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        CARLA_ASSERT(parameterId < param.count);

        paramBuffers[parameterId] = fixParameterValue(value, param.ranges[parameterId]);

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui)
    {
        CARLA_ASSERT(type);
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);

        if (! type)
            return qCritical("Lv2Plugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is invalid", type, key, value, bool2str(sendGui));

        if (! key)
            return qCritical("Lv2Plugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

        if (! value)
            return qCritical("Lv2Plugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

        CarlaPlugin::setCustomData(type, key, value, sendGui);

        if (ext.state)
        {
            LV2_State_Status status;

            if (x_engine->isOffline())
            {
                const CarlaEngine::ScopedLocker m(x_engine);
                status = ext.state->restore(handle, carla_lv2_state_retrieve, this, 0, features);
                if (h2) ext.state->restore(h2, carla_lv2_state_retrieve, this, 0, features);
            }
            else
            {
                const CarlaPlugin::ScopedDisabler m(this);
                status = ext.state->restore(handle, carla_lv2_state_retrieve, this, 0, features);
                if (h2) ext.state->restore(h2, carla_lv2_state_retrieve, this, 0, features);
            }

            switch (status)
            {
            case LV2_STATE_SUCCESS:
                qDebug("Lv2Plugin::setCustomData(\"%s\", \"%s\", <value>, %s) - success", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_UNKNOWN:
                qWarning("Lv2Plugin::setCustomData(\"%s\", \"%s\", <value>, %s) - unknown error", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_BAD_TYPE:
                qWarning("Lv2Plugin::setCustomData(\"%s\", \"%s\", <value>, %s) - error, bad type", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_BAD_FLAGS:
                qWarning("Lv2Plugin::setCustomData(\"%s\", \"%s\", <value>, %s) - error, bad flags", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_NO_FEATURE:
                qWarning("Lv2Plugin::setCustomData(\"%s\", \"%s\", <value>, %s) - error, missing feature", type, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_NO_PROPERTY:
                qWarning("Lv2Plugin::setCustomData(\"%s\", \"%s\", <value>, %s) - error, missing property", type, key, bool2str(sendGui));
                break;
            }
        }

        if (sendGui)
        {
            CustomData cdata;
            cdata.type  = type;
            cdata.key   = key;
            cdata.value = value;
            uiTransferCustomData(&cdata);
        }
    }

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
    {
        CARLA_ASSERT(index >= -1 && index < (int32_t)midiprog.count);

        if (index < -1)
            index = -1;
        else if (index > (int32_t)midiprog.count)
            return;

        if (ext.programs && index >= 0)
        {
            if (x_engine->isOffline())
            {
                const CarlaEngine::ScopedLocker m(x_engine, block);
                ext.programs->select_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
                if (h2) ext.programs->select_program(h2, midiprog.data[index].bank, midiprog.data[index].program);
            }
            else
            {
                const ScopedDisabler m(this, block);
                ext.programs->select_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
                if (h2) ext.programs->select_program(h2, midiprog.data[index].bank, midiprog.data[index].program);
            }
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, block);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void setGuiContainer(GuiContainer* const container)
    {
        qDebug("Lv2Plugin::setGuiContainer(%p)", container);
        CARLA_ASSERT(container);

        switch(gui.type)
        {
        case GUI_NONE:
            break;

        case GUI_INTERNAL_QT4:
            if (ui.widget)
            {
                QDialog* const dialog = (QDialog*)container->parent();
                QWidget* const widget = (QWidget*)ui.widget;
                CARLA_ASSERT(dialog);
                CARLA_ASSERT(dialog->layout());
                CARLA_ASSERT(widget);
                container->setVisible(false);
                dialog->layout()->addWidget(widget);
                widget->adjustSize();
                widget->setParent(dialog);
                widget->show();
            }
            break;

        case GUI_INTERNAL_COCOA:
        case GUI_INTERNAL_HWND:
        case GUI_INTERNAL_X11:
            if (ui.descriptor)
            {
                features[lv2_feature_id_ui_parent]->data = (void*)container->winId();
                ui.handle = ui.descriptor->instantiate(ui.descriptor, descriptor->URI, ui.rdf_descriptor->Bundle, carla_lv2_ui_write_function, this, &ui.widget, features);
                updateUi();
            }
            break;

        case GUI_EXTERNAL_LV2:
        case GUI_EXTERNAL_SUIL:
        case GUI_EXTERNAL_OSC:
            break;
        }
    }

    void showGui(const bool yesNo)
    {
        qDebug("Lv2Plugin::showGui(%s)", bool2str(yesNo));

        switch(gui.type)
        {
        case GUI_NONE:
        case GUI_INTERNAL_QT4:
            break;

        case GUI_INTERNAL_COCOA:
        case GUI_INTERNAL_HWND:
        case GUI_INTERNAL_X11:
            if (yesNo && gui.width > 0 && gui.height > 0)
                x_engine->callback(CALLBACK_RESIZE_GUI, m_id, gui.width, gui.height, 0.0, nullptr);
            break;

        case GUI_EXTERNAL_LV2:
            if (yesNo && ! ui.handle)
                initExternalUi();

            if (ui.handle && ui.descriptor && ui.widget)
            {
                if (yesNo)
                {
                    LV2_EXTERNAL_UI_SHOW((LV2_External_UI_Widget*)ui.widget);
                }
                else
                {
                    LV2_EXTERNAL_UI_HIDE((LV2_External_UI_Widget*)ui.widget);

                    if (rdf_descriptor->Author && strcmp(rdf_descriptor->Author, "linuxDSP") == 0)
                    {
                        qWarning("linuxDSP LV2 UI hack (force close instead of hide)");

                        if (ui.descriptor->cleanup)
                            ui.descriptor->cleanup(ui.handle);

                        ui.handle = nullptr;
                    }
                }
            }
            else
                // failed to init UI
                x_engine->callback(CALLBACK_SHOW_GUI, m_id, -1, 0, 0.0, nullptr);

            break;

        case GUI_EXTERNAL_SUIL:
#ifdef WANT_SUIL
            if (ui.widget)
            {
                QWidget* const widget = (QWidget*)ui.widget;

                if (yesNo)
                {
                    if (! suil.pos.isNull())
                        widget->restoreGeometry(suil.pos);
                }
                else
                    suil.pos = widget->saveGeometry();

                widget->setVisible(yesNo);
            }
#endif
            break;

        case GUI_EXTERNAL_OSC:
            CARLA_ASSERT(osc.thread);

            if (! osc.thread)
            {
                qCritical("Lv2Plugin::showGui(%s) - attempt to show gui, but it does not exist!", bool2str(yesNo));
                return;
            }

            if (yesNo)
            {
                osc.thread->start();
            }
            else
            {
                if (osc.data.target)
                {
                    osc_send_hide(&osc.data);
                    osc_send_quit(&osc.data);
                    osc.data.free();
                }

                if (! osc.thread->wait(500))
                    osc.thread->quit();
            }
            break;
        }
    }

    void idleGui()
    {
        if ((gui.type == GUI_EXTERNAL_OSC && osc.data.target) || (ui.handle && ui.descriptor))
        {
            // Update event ports
            if (! atomQueueOut.isEmpty())
            {
                Lv2AtomQueue queue;
                queue.copyDataFrom(&atomQueueOut);

                uint32_t portIndex;
                const LV2_Atom* atom;

                while (queue.get(&portIndex, &atom))
                {
                    if (gui.type == GUI_EXTERNAL_OSC)
                    {
                        QByteArray chunk((const char*)atom, sizeof(LV2_Atom) + atom->size);
                        osc_send_lv2_transfer_event(&osc.data, portIndex, getCustomURIString(atom->type), chunk.toBase64().constData());
                    }
                    else
                    {
                        if (ui.descriptor->port_event)
                            ui.descriptor->port_event(ui.handle, portIndex, atom->size, CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);
                    }
                }
            }

            // Update external UI
            if (gui.type == GUI_EXTERNAL_LV2 && ui.widget)
                LV2_EXTERNAL_UI_RUN((LV2_External_UI_Widget*)ui.widget);
        }

        CarlaPlugin::idleGui();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("Lv2Plugin::reload() - start");
        CARLA_ASSERT(descriptor && rdf_descriptor);

        const ProcessMode processMode(x_engine->getOptions().processMode);

        // Safely disable plugin for reload
        const ScopedDisabler m(this);

        if (x_client->isActive())
            x_client->deactivate();

        // Remove client ports
        removeClientPorts();

        // Delete old data
        deleteBuffers();

        uint32_t aIns, aOuts, cvIns, cvOuts, params, j;
        aIns = aOuts = cvIns = cvOuts = params = 0;
        std::vector<uint32_t> evIns, evOuts;

        const double sampleRate  = x_engine->getSampleRate();
        const uint32_t portCount = rdf_descriptor->PortCount;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        for (uint32_t i=0; i < portCount; i++)
        {
            const LV2_Property portTypes = rdf_descriptor->Ports[i].Types;

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
                    evIns.push_back(CARLA_EVENT_DATA_ATOM);
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                    evOuts.push_back(CARLA_EVENT_DATA_ATOM);
            }
            else if (LV2_IS_PORT_EVENT(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                    evIns.push_back(CARLA_EVENT_DATA_EVENT);
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                    evOuts.push_back(CARLA_EVENT_DATA_EVENT);
            }
            else if (LV2_IS_PORT_MIDI_LL(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                    evIns.push_back(CARLA_EVENT_DATA_MIDI_LL);
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                    evOuts.push_back(CARLA_EVENT_DATA_MIDI_LL);
            }
            else if (LV2_IS_PORT_CONTROL(portTypes))
                params += 1;
        }

        // check extensions
        ext.state      = nullptr;
        ext.worker     = nullptr;
        ext.programs   = nullptr;

        if (descriptor->extension_data)
        {
            if (m_hints & PLUGIN_HAS_EXTENSION_PROGRAMS)
                ext.programs = (const LV2_Programs_Interface*)descriptor->extension_data(LV2_PROGRAMS__Interface);

            if (m_hints & PLUGIN_HAS_EXTENSION_STATE)
                ext.state = (const LV2_State_Interface*)descriptor->extension_data(LV2_STATE__interface);

            if (m_hints & PLUGIN_HAS_EXTENSION_WORKER)
                ext.worker = (const LV2_Worker_Interface*)descriptor->extension_data(LV2_WORKER__interface);
        }

        if (x_engine->getOptions().forceStereo && (aIns == 1 || aOuts == 1) && ! (h2 || ext.state || ext.worker))
        {
            h2 = descriptor->instantiate(descriptor, sampleRate, rdf_descriptor->Bundle, features);

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
            aIn.ports    = new CarlaEngineAudioPort*[aIns];
            aIn.rindexes = new uint32_t[aIns];
        }

        if (aOuts > 0)
        {
            aOut.ports    = new CarlaEngineAudioPort*[aOuts];
            aOut.rindexes = new uint32_t[aOuts];
        }

        if (evIns.size() > 0)
        {
            const size_t size = evIns.size();
            evIn.data = new Lv2EventData[size];

            for (j=0; j < size; j++)
            {
                evIn.data[j].port = nullptr;
                evIn.data[j].type = 0;

                if (evIns[j] == CARLA_EVENT_DATA_ATOM)
                {
                    evIn.data[j].type = CARLA_EVENT_DATA_ATOM;
                    evIn.data[j].atom = (LV2_Atom_Sequence*)malloc(sizeof(LV2_Atom_Sequence) + MAX_EVENT_BUFFER);
                    evIn.data[j].atom->atom.size = sizeof(LV2_Atom_Sequence_Body);
                    evIn.data[j].atom->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                    evIn.data[j].atom->body.unit = CARLA_URI_MAP_ID_NULL;
                    evIn.data[j].atom->body.pad  = 0;
                }
                else if (evIns[j] == CARLA_EVENT_DATA_EVENT)
                {
                    evIn.data[j].type  = CARLA_EVENT_DATA_EVENT;
                    evIn.data[j].event = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
                }
                else if (evIns[j] == CARLA_EVENT_DATA_MIDI_LL)
                {
                    evIn.data[j].type  = CARLA_EVENT_DATA_MIDI_LL;
                    evIn.data[j].midi  = new LV2_MIDI;
                    evIn.data[j].midi->capacity = MAX_EVENT_BUFFER;
                    evIn.data[j].midi->data     = new unsigned char [MAX_EVENT_BUFFER];
                }
            }
        }

        if (evOuts.size() > 0)
        {
            const size_t size = evOuts.size();
            evOut.data = new Lv2EventData[size];

            for (j=0; j < size; j++)
            {
                evOut.data[j].port = nullptr;
                evOut.data[j].type = 0;

                if (evOuts[j] == CARLA_EVENT_DATA_ATOM)
                {
                    evOut.data[j].type = CARLA_EVENT_DATA_ATOM;
                    evOut.data[j].atom = (LV2_Atom_Sequence*)malloc(sizeof(LV2_Atom_Sequence) + MAX_EVENT_BUFFER);
                    evOut.data[j].atom->atom.size = sizeof(LV2_Atom_Sequence_Body);
                    evOut.data[j].atom->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                    evOut.data[j].atom->body.unit = CARLA_URI_MAP_ID_NULL;
                    evOut.data[j].atom->body.pad  = 0;
                }
                else if (evOuts[j] == CARLA_EVENT_DATA_EVENT)
                {
                    evOut.data[j].type  = CARLA_EVENT_DATA_EVENT;
                    evOut.data[j].event = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
                }
                else if (evOuts[j] == CARLA_EVENT_DATA_MIDI_LL)
                {
                    evOut.data[j].type  = CARLA_EVENT_DATA_MIDI_LL;
                    evOut.data[j].midi  = new LV2_MIDI;
                    evOut.data[j].midi->capacity = MAX_EVENT_BUFFER;
                    evOut.data[j].midi->data     = new unsigned char [MAX_EVENT_BUFFER];
                }
            }
        }

        if (params > 0)
        {
            param.data   = new ParameterData[params];
            param.ranges = new ParameterRanges[params];
            paramBuffers = new float[params];
        }

        bool needsCtrlIn  = false;
        bool needsCtrlOut = false;

        const int   portNameSize = x_engine->maxPortNameSize();
        CarlaString portName;

        for (uint32_t i=0; i < portCount; i++)
        {
            const LV2_Property portTypes = rdf_descriptor->Ports[i].Types;

            if (LV2_IS_PORT_AUDIO(portTypes) || LV2_IS_PORT_ATOM_SEQUENCE(portTypes) || LV2_IS_PORT_CV(portTypes) || LV2_IS_PORT_EVENT(portTypes) || LV2_IS_PORT_MIDI_LL(portTypes))
            {
                portName.clear();

                if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = m_name;
                    portName += ":";
                }

                portName += rdf_descriptor->Ports[i].Name;
                portName.truncate(portNameSize);
            }

            if (LV2_IS_PORT_AUDIO(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    j = aIn.count++;
                    aIn.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, true);
                    aIn.rindexes[j] = i;

                    if (forcedStereoIn)
                    {
                        portName += "_2";
                        aIn.ports[1]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, true);
                        aIn.rindexes[1] = i;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    j = aOut.count++;
                    aOut.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
                    aOut.rindexes[j] = i;
                    needsCtrlIn = true;

                    if (forcedStereoOut)
                    {
                        portName += "_2";
                        aOut.ports[1]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
                        aOut.rindexes[1] = i;
                    }
                }
                else
                    qWarning("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LV2_IS_PORT_CV(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    qWarning("WARNING - CV Ports are not supported yet");
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    qWarning("WARNING - CV Ports are not supported yet");
                }
                else
                    qWarning("WARNING - Got a broken Port (CV, but not input or output)");

                descriptor->connect_port(handle, i, nullptr);
                if (h2) descriptor->connect_port(h2, i, nullptr);
            }
            else if (LV2_IS_PORT_ATOM_SEQUENCE(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    j = evIn.count++;
                    descriptor->connect_port(handle, i, evIn.data[j].atom);
                    if (h2) descriptor->connect_port(h2, i, evIn.data[j].atom);

                    evIn.data[j].rindex = i;

                    if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                    {
                        evIn.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                        evIn.data[j].port  = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, true);
                    }
                    if (portTypes & LV2_PORT_DATA_PATCH_MESSAGE)
                    {
                        evIn.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    j = evOut.count++;
                    descriptor->connect_port(handle, i, evOut.data[j].atom);
                    if (h2) descriptor->connect_port(h2, i, evOut.data[j].atom);

                    evOut.data[j].rindex = i;

                    if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                    {
                        evOut.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                        evOut.data[j].port  = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, false);
                    }
                    if (portTypes & LV2_PORT_DATA_PATCH_MESSAGE)
                    {
                        evOut.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    }
                }
                else
                    qWarning("WARNING - Got a broken Port (Atom Sequence, but not input or output)");
            }
            else if (LV2_IS_PORT_EVENT(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    j = evIn.count++;
                    descriptor->connect_port(handle, i, evIn.data[j].event);
                    if (h2) descriptor->connect_port(h2, i, evIn.data[j].event);

                    evIn.data[j].rindex = i;

                    if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                    {
                        evIn.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                        evIn.data[j].port  = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, true);
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    j = evOut.count++;
                    descriptor->connect_port(handle, i, evOut.data[j].event);
                    if (h2) descriptor->connect_port(h2, i, evOut.data[j].event);

                    evOut.data[j].rindex = i;

                    if (portTypes & LV2_PORT_DATA_MIDI_EVENT)
                    {
                        evOut.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                        evOut.data[j].port  = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, false);
                    }
                }
                else
                    qWarning("WARNING - Got a broken Port (Event, but not input or output)");
            }
            else if (LV2_IS_PORT_MIDI_LL(portTypes))
            {
                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    j = evIn.count++;
                    descriptor->connect_port(handle, i, evIn.data[j].midi);
                    if (h2) descriptor->connect_port(h2, i, evIn.data[j].midi);

                    evIn.data[j].type  |= CARLA_EVENT_TYPE_MIDI;
                    evIn.data[j].port   = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, true);
                    evIn.data[j].rindex = i;
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    j = evOut.count++;
                    descriptor->connect_port(handle, i, evOut.data[j].midi);
                    if (h2) descriptor->connect_port(h2, i, evOut.data[j].midi);

                    evOut.data[j].type  |= CARLA_EVENT_TYPE_MIDI;
                    evOut.data[j].port   = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, false);
                    evOut.data[j].rindex = i;
                }
                else
                    qWarning("WARNING - Got a broken Port (Midi, but not input or output)");
            }
            else if (LV2_IS_PORT_CONTROL(portTypes))
            {
                const LV2_Property portProps = rdf_descriptor->Ports[i].Properties;
                const LV2_Property portDesignation = rdf_descriptor->Ports[i].Designation;
                const LV2_RDF_PortPoints portPoints = rdf_descriptor->Ports[i].Points;

                j = param.count++;
                param.data[j].index  = j;
                param.data[j].rindex = i;
                param.data[j].hints  = 0;
                param.data[j].midiChannel = 0;
                param.data[j].midiCC = -1;

                double min, max, def, step, stepSmall, stepLarge;

                // min value
                if (LV2_HAVE_MINIMUM_PORT_POINT(portPoints.Hints))
                    min = portPoints.Minimum;
                else
                    min = 0.0;

                // max value
                if (LV2_HAVE_MAXIMUM_PORT_POINT(portPoints.Hints))
                    max = portPoints.Maximum;
                else
                    max = 1.0;

                if (min > max)
                    max = min;
                else if (max < min)
                    min = max;

                // stupid hack for ir.lv2 (broken plugin)
                if (strcmp(rdf_descriptor->URI, "http://factorial.hu/plugins/lv2/ir") == 0 && strncmp(rdf_descriptor->Ports[i].Name, "FileHash", 8) == 0)
                {
                    min = 0.0;
                    max = 16777215.0; // 0xffffff
                }

                if (max - min == 0.0)
                {
                    qWarning("Broken plugin parameter: max - min == 0");
                    max = min + 0.1;
                }

                // default value
                if (LV2_HAVE_DEFAULT_PORT_POINT(portPoints.Hints))
                {
                    def = portPoints.Default;
                }
                else
                {
                    // no default value
                    if (min < 0.0 && max > 0.0)
                        def = 0.0;
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
                    param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LV2_IS_PORT_TOGGLED(portProps))
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LV2_IS_PORT_INTEGER(portProps))
                {
                    step = 1.0;
                    stepSmall = 1.0;
                    stepLarge = 10.0;
                    param.data[j].hints |= PARAMETER_IS_INTEGER;
                }
                else
                {
                    double range = max - min;
                    step = range/100.0;
                    stepSmall = range/1000.0;
                    stepLarge = range/10.0;
                }

                if (LV2_IS_PORT_INPUT(portTypes))
                {
                    if (LV2_IS_PORT_DESIGNATION_LATENCY(portDesignation))
                    {
                        qWarning("Plugin has latency input port, this should not happen!");
                    }
                    else if (LV2_IS_PORT_DESIGNATION_SAMPLE_RATE(portDesignation))
                    {
                        def = sampleRate;
                        step = 1.0;
                        stepSmall = 1.0;
                        stepLarge = 1.0;

                        param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        param.data[j].hints = 0;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_FREEWHEELING(portDesignation))
                    {
                        param.data[j].type = PARAMETER_LV2_FREEWHEEL;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_TIME(portDesignation))
                    {
                        param.data[j].type = PARAMETER_LV2_TIME;
                    }
                    else
                    {
                        param.data[j].type = PARAMETER_INPUT;
                        param.data[j].hints |= PARAMETER_IS_ENABLED;
                        param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlIn = true;
                    }

                    // MIDI CC value
                    const LV2_RDF_PortMidiMap* const portMidiMap = &rdf_descriptor->Ports[i].MidiMap;
                    if (LV2_IS_PORT_MIDI_MAP_CC(portMidiMap->Type))
                    {
                        if (! MIDI_IS_CONTROL_BANK_SELECT(portMidiMap->Number))
                            param.data[j].midiCC = portMidiMap->Number;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(portTypes))
                {
                    if (LV2_IS_PORT_DESIGNATION_LATENCY(portDesignation))
                    {
                        min = 0.0;
                        max = sampleRate;
                        def = 0.0;
                        step = 1.0;
                        stepSmall = 1.0;
                        stepLarge = 1.0;

                        param.data[j].type  = PARAMETER_LATENCY;
                        param.data[j].hints = 0;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_SAMPLE_RATE(portDesignation))
                    {
                        def = sampleRate;
                        step = 1.0;
                        stepSmall = 1.0;
                        stepLarge = 1.0;

                        param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        param.data[j].hints = 0;
                    }
                    else if (LV2_IS_PORT_DESIGNATION_FREEWHEELING(portDesignation))
                    {
                        qWarning("Plugin has freewheeling output port, this should not happen!");
                    }
                    else if (LV2_IS_PORT_DESIGNATION_TIME(portDesignation))
                    {
                        param.data[j].type = PARAMETER_LV2_TIME;
                    }
                    else
                    {
                        param.data[j].type   = PARAMETER_OUTPUT;
                        param.data[j].hints |= PARAMETER_IS_ENABLED;
                        param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlOut = true;
                    }
                }
                else
                {
                    param.data[j].type = PARAMETER_UNKNOWN;
                    qWarning("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LV2_IS_PORT_ENUMERATION(portProps))
                    param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                if (LV2_IS_PORT_LOGARITHMIC(portProps))
                    param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                if (LV2_IS_PORT_TRIGGER(portProps))
                    param.data[j].hints |= PARAMETER_IS_TRIGGER;

                if (LV2_IS_PORT_STRICT_BOUNDS(portProps))
                    param.data[j].hints |= PARAMETER_IS_STRICT_BOUNDS;

                // check if parameter is not enabled or automable
                if (LV2_IS_PORT_NOT_ON_GUI(portProps))
                    param.data[j].hints &= ~PARAMETER_IS_ENABLED;

                if (LV2_IS_PORT_CAUSES_ARTIFACTS(portProps) || LV2_IS_PORT_EXPENSIVE(portProps) || LV2_IS_PORT_NOT_AUTOMATIC(portProps))
                    param.data[j].hints &= ~PARAMETER_IS_AUTOMABLE;

                param.ranges[j].min = min;
                param.ranges[j].max = max;
                param.ranges[j].def = def;
                param.ranges[j].step = step;
                param.ranges[j].stepSmall = stepSmall;
                param.ranges[j].stepLarge = stepLarge;

                // Start parameters in their default values
                paramBuffers[j] = def;

                descriptor->connect_port(handle, i, &paramBuffers[j]);
                if (h2) descriptor->connect_port(h2, i, &paramBuffers[j]);
            }
            else
            {
                // Port Type not supported, but it's optional anyway
                descriptor->connect_port(handle, i, nullptr);
                if (h2) descriptor->connect_port(h2, i, nullptr);
            }
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = m_name;
                portName += ":";
            }

            portName += "control-in";
            portName.truncate(portNameSize);

            param.portCin = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, true);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = m_name;
                portName += ":";
            }

            portName += "control-out";
            portName.truncate(portNameSize);

            param.portCout = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, false);
        }

        aIn.count   = aIns;
        aOut.count  = aOuts;
        evIn.count  = evIns.size();
        evOut.count = evOuts.size();
        param.count = params;

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE | PLUGIN_CAN_FORCE_STEREO);

        if (LV2_IS_GENERATOR(rdf_descriptor->Type[0], rdf_descriptor->Type[1]))
            m_hints |= PLUGIN_IS_SYNTH;

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts%2 == 0)
            m_hints |= PLUGIN_CAN_BALANCE;

        if (ext.state || ext.worker)
        {
            if ((aIns == 0 || aIns == 2) && (aOuts == 0 || aOuts == 2) && evIn.count <= 1 && evOut.count <= 1)
                m_hints |= PLUGIN_CAN_FORCE_STEREO;
        }
        else
        {
            if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0) && evIn.count <= 1 && evOut.count <= 1)
                m_hints |= PLUGIN_CAN_FORCE_STEREO;
        }

        // check latency
        if (m_hints & PLUGIN_CAN_DRYWET)
        {
            bool hasLatency = false;
            m_latency = 0;

            for (uint32_t i=0; i < param.count; i++)
            {
                if (param.data[i].type == PARAMETER_LATENCY)
                {
                    // pre-run so plugin can update latency control-port
                    float tmpIn[2][aIns];
                    float tmpOut[2][aOuts];

                    for (j=0; j < aIn.count; j++)
                    {
                        tmpIn[j][0] = 0.0f;
                        tmpIn[j][1] = 0.0f;

                        if (j == 0 || ! h2)
                            descriptor->connect_port(handle, aIn.rindexes[j], tmpIn[j]);
                    }

                    for (j=0; j < aOut.count; j++)
                    {
                        tmpOut[j][0] = 0.0f;
                        tmpOut[j][1] = 0.0f;

                        if (j == 0 || ! h2)
                            descriptor->connect_port(handle, aOut.rindexes[j], tmpOut[j]);
                    }

                    if (descriptor->activate)
                        descriptor->activate(handle);

                    descriptor->run(handle, 2);

                    if (descriptor->deactivate)
                        descriptor->deactivate(handle);

                    m_latency = rint(paramBuffers[i]);
                    hasLatency = true;
                    break;
                }
            }

            if (hasLatency)
            {
                x_client->setLatency(m_latency);
                recreateLatencyBuffers();
            }
        }

        reloadPrograms(true);

        x_client->activate();

        qDebug("Lv2Plugin::reload() - end");
    }

    void reloadPrograms(const bool init)
    {
        qDebug("Lv2Plugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount = midiprog.count;

        // Delete old programs
        if (midiprog.count > 0)
        {
            for (i=0; i < midiprog.count; i++)
            {
                if (midiprog.data[i].name)
                    free((void*)midiprog.data[i].name);
            }

            delete[] midiprog.data;
        }

        midiprog.count = 0;
        midiprog.data  = nullptr;

        // Query new programs
        if (ext.programs && ext.programs->get_program && ext.programs->select_program)
        {
            while (ext.programs->get_program(handle, midiprog.count))
                midiprog.count += 1;
        }

        if (midiprog.count > 0)
            midiprog.data = new MidiProgramData[midiprog.count];

        // Update data
        for (i=0; i < midiprog.count; i++)
        {
            const LV2_Program_Descriptor* const pdesc = ext.programs->get_program(handle, i);
            CARLA_ASSERT(pdesc);
            CARLA_ASSERT(pdesc->name);

            midiprog.data[i].bank    = pdesc->bank;
            midiprog.data[i].program = pdesc->program;
            midiprog.data[i].name    = strdup(pdesc->name ? pdesc->name : "");
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (x_engine->isOscControlRegistered())
        {
            x_engine->osc_send_control_set_midi_program_count(m_id, midiprog.count);

            for (i=0; i < midiprog.count; i++)
                x_engine->osc_send_control_set_midi_program_data(m_id, i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);
        }
#endif

        if (init)
        {
            if (midiprog.count > 0)
                setMidiProgram(0, false, false, false, true);
        }
        else
        {
            x_engine->callback(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0, nullptr);

            // Check if current program is invalid
            bool programChanged = false;

            if (midiprog.count == oldCount+1)
            {
                // one midi program added, probably created by user
                midiprog.current = oldCount;
                programChanged   = true;
            }
            else if (midiprog.current >= (int32_t)midiprog.count)
            {
                // current midi program > count
                midiprog.current = 0;
                programChanged   = true;
            }
            else if (midiprog.current < 0 && midiprog.count > 0)
            {
                // programs exist now, but not before
                midiprog.current = 0;
                programChanged   = true;
            }
            else if (midiprog.current >= 0 && midiprog.count == 0)
            {
                // programs existed before, but not anymore
                midiprog.current = -1;
                programChanged   = true;
            }

            if (programChanged)
                setMidiProgram(midiprog.current, true, true, true, true);
        }
    }

    void prepareForSave()
    {
        if (ext.state && ext.state->save)
        {
            ext.state->save(handle, carla_lv2_state_store, this, LV2_STATE_IS_POD, features);
            if (h2) ext.state->save(h2, carla_lv2_state_store, this, LV2_STATE_IS_POD, features);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t framesOffset)
    {
        uint32_t i, k;
        uint32_t midiEventCount = 0;

        double aInsPeak[2]  = { 0.0 };
        double aOutsPeak[2] = { 0.0 };

        // handle events from different APIs
        uint32_t evInAtomOffsets[evIn.count];
        LV2_Event_Iterator evInEventIters[evIn.count];
        LV2_MIDIState evInMidiStates[evIn.count];

        for (i=0; i < evIn.count; i++)
        {
            if (evIn.data[i].type & CARLA_EVENT_DATA_ATOM)
            {
                evInAtomOffsets[i] = 0;
                evIn.data[i].atom->atom.size = 0;
                evIn.data[i].atom->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                evIn.data[i].atom->body.unit = CARLA_URI_MAP_ID_NULL;
                evIn.data[i].atom->body.pad  = 0;
            }
            else if (evIn.data[i].type & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(evIn.data[i].event, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(evIn.data[i].event + 1));
                lv2_event_begin(&evInEventIters[i], evIn.data[i].event);
            }
            else if (evIn.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
            {
                evInMidiStates[i].midi = evIn.data[i].midi;
                evInMidiStates[i].frame_count = frames;
                evInMidiStates[i].position = 0;
                evInMidiStates[i].midi->event_count = 0;
                evInMidiStates[i].midi->size = 0;
            }
        }

        for (i=0; i < evOut.count; i++)
        {
            if (evOut.data[i].type & CARLA_EVENT_DATA_ATOM)
            {
                evOut.data[i].atom->atom.size = 0;
                evOut.data[i].atom->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                evOut.data[i].atom->body.unit = CARLA_URI_MAP_ID_NULL;
                evOut.data[i].atom->body.pad  = 0;
            }
            else if (evOut.data[i].type & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(evOut.data[i].event, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(evOut.data[i].event + 1));
            }
            else if (evOut.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
            {
                // not needed
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (aIn.count > 0 && x_engine->getOptions().processMode != PROCESS_MODE_CONTINUOUS_RACK)
        {
            if (aIn.count == 1)
            {
                for (k=0; k < frames; k++)
                {
                    if (std::abs(inBuffer[0][k]) > aInsPeak[0])
                        aInsPeak[0] = std::abs(inBuffer[0][k]);
                }
            }
            else if (aIn.count > 1)
            {
                for (k=0; k < frames; k++)
                {
                    if (std::abs(inBuffer[0][k]) > aInsPeak[0])
                        aInsPeak[0] = std::abs(inBuffer[0][k]);

                    if (std::abs(inBuffer[1][k]) > aInsPeak[1])
                        aInsPeak[1] = std::abs(inBuffer[1][k]);
                }
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (param.portCin && m_active && m_activeBefore)
        {
            bool allNotesOffSent = false;

            const CarlaEngineControlEvent* cinEvent;
            uint32_t time, nEvents = param.portCin->getEventCount();

            uint32_t nextBankId = 0;
            if (midiprog.current >= 0 && midiprog.count > 0)
                nextBankId = midiprog.data[midiprog.current].bank;

            for (i=0; i < nEvents; i++)
            {
                cinEvent = param.portCin->getEvent(i);

                if (! cinEvent)
                    continue;

                time = cinEvent->time - framesOffset;

                if (time >= frames)
                    continue;

                // Control change
                switch (cinEvent->type)
                {
                case CarlaEngineNullEvent:
                    break;

                case CarlaEngineParameterChangeEvent:
                {
                    double value;

                    // Control backend stuff
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (MIDI_IS_CONTROL_BREATH_CONTROLLER(cinEvent->parameter) && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            value = cinEvent->value;
                            setDryWet(value, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_DRYWET, 0, value);
                            continue;
                        }

                        if (MIDI_IS_CONTROL_CHANNEL_VOLUME(cinEvent->parameter) && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            value = cinEvent->value*127/100;
                            setVolume(value, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_VOLUME, 0, value);
                            continue;
                        }

                        if (MIDI_IS_CONTROL_BALANCE(cinEvent->parameter) && (m_hints & PLUGIN_CAN_BALANCE) > 0)
                        {
                            double left, right;
                            value = cinEvent->value/0.5 - 1.0;

                            if (value < 0.0)
                            {
                                left  = -1.0;
                                right = (value*2)+1.0;
                            }
                            else if (value > 0.0)
                            {
                                left  = (value*2)-1.0;
                                right = 1.0;
                            }
                            else
                            {
                                left  = -1.0;
                                right = 1.0;
                            }

                            setBalanceLeft(left, false, false);
                            setBalanceRight(right, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                            continue;
                        }
                    }

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].midiChannel != cinEvent->channel)
                            continue;
                        if (param.data[k].midiCC != cinEvent->parameter)
                            continue;
                        if (param.data[k].type != PARAMETER_INPUT)
                            continue;

                        if (param.data[k].hints & PARAMETER_IS_AUTOMABLE)
                        {
                            if (param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = cinEvent->value < 0.5 ? param.ranges[k].min : param.ranges[k].max;
                            }
                            else
                            {
                                value = cinEvent->value * (param.ranges[k].max - param.ranges[k].min) + param.ranges[k].min;

                                if (param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeEvent(PluginPostEventParameterChange, k, 0, value);
                        }
                    }

                    break;
                }

                case CarlaEngineMidiBankChangeEvent:
                    if (cinEvent->channel == m_ctrlInChannel)
                        nextBankId = rint(cinEvent->value);
                    break;

                case CarlaEngineMidiProgramChangeEvent:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        uint32_t nextProgramId = rint(cinEvent->value);

                        for (k=0; k < midiprog.count; k++)
                        {
                            if (midiprog.data[k].bank == nextBankId && midiprog.data[k].program == nextProgramId)
                            {
                                setMidiProgram(k, false, false, false, false);
                                postponeEvent(PluginPostEventMidiProgramChange, k, 0, 0.0);
                                break;
                            }
                        }
                    }
                    break;

                case CarlaEngineAllSoundOffEvent:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (evIn.count > 0 && ! allNotesOffSent)
                            sendMidiAllNotesOff();

                        if (descriptor->deactivate)
                        {
                            descriptor->deactivate(handle);
                            if (h2) descriptor->deactivate(h2);
                        }

                        if (descriptor->activate)
                        {
                            descriptor->activate(handle);
                            if (h2) descriptor->activate(h2);
                        }

                        postponeEvent(PluginPostEventParameterChange, PARAMETER_ACTIVE, 0, 0.0);
                        postponeEvent(PluginPostEventParameterChange, PARAMETER_ACTIVE, 0, 1.0);

                        allNotesOffSent = true;
                    }
                    break;

                case CarlaEngineAllNotesOffEvent:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (evIn.count > 0 && ! allNotesOffSent)
                            sendMidiAllNotesOff();

                        allNotesOffSent = true;
                    }
                    break;
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Event Input

        if (evIn.count > 0 && m_active && m_activeBefore)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            {
                engineMidiLock();

                for (i=0; i < MAX_MIDI_EVENTS && midiEventCount < MAX_MIDI_EVENTS; i++)
                {
                    if (extMidiNotes[i].channel < 0)
                        break;

                    uint8_t midiEvent[3] = { 0 };
                    midiEvent[0] = m_ctrlInChannel + extMidiNotes[i].velo ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
                    midiEvent[1] = extMidiNotes[i].note;
                    midiEvent[2] = extMidiNotes[i].velo;

                    // send to first midi input
                    for (k=0; k < evIn.count; k++)
                    {
                        if (evIn.data[k].type & CARLA_EVENT_TYPE_MIDI)
                        {
                            if (evIn.data[k].type & CARLA_EVENT_DATA_ATOM)
                            {
                                LV2_Atom_Event* const aev = getLv2AtomEvent(evIn.data[k].atom, evInAtomOffsets[k]);
                                aev->time.frames = 0;
                                aev->body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                                aev->body.size   = 3;
                                memcpy(LV2_ATOM_BODY(&aev->body), midiEvent, 3);

                                const uint32_t evInPadSize    = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + 3);
                                evInAtomOffsets[k]           += evInPadSize;
                                evIn.data[k].atom->atom.size += evInPadSize;
                            }
                            else if (evIn.data[k].type & CARLA_EVENT_DATA_EVENT)
                            {
                                lv2_event_write(&evInEventIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiEvent);
                            }
                            else if (evIn.data[k].type & CARLA_EVENT_DATA_MIDI_LL)
                            {
                                lv2midi_put_event(&evInMidiStates[k], 0, 3, midiEvent);
                            }

                            break;
                        }
                    }

                    extMidiNotes[i].channel = -1; // mark as invalid
                    midiEventCount += 1;
                }

                engineMidiUnlock();

            } // End of MIDI Input (External)

            CARLA_PROCESS_CONTINUE_CHECK;

            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (System)

            for (i=0; i < evIn.count; i++)
            {
                if (! evIn.data[i].port)
                    continue;

                const CarlaEngineMidiEvent* minEvent;
                uint32_t time, nEvents = evIn.data[i].port->getEventCount();

                for (k=0; k < nEvents && midiEventCount < MAX_MIDI_EVENTS; k++)
                {
                    minEvent = evIn.data[i].port->getEvent(k);

                    if (! minEvent)
                        continue;

                    time = minEvent->time - framesOffset;

                    if (time >= frames)
                        continue;

                    uint8_t status  = minEvent->data[0];
                    uint8_t channel = status & 0x0F;

                    // Fix bad note-off
                    if (MIDI_IS_STATUS_NOTE_ON(status) && minEvent->data[2] == 0)
                        status -= 0x10;

                    // only write supported status types
                    if (MIDI_IS_STATUS_NOTE_OFF(status) || MIDI_IS_STATUS_NOTE_ON(status) || MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status) || MIDI_IS_STATUS_AFTERTOUCH(status) || MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                    {
                        if (evIn.data[i].type & CARLA_EVENT_DATA_ATOM)
                        {
                            LV2_Atom_Event* const aev = getLv2AtomEvent(evIn.data[i].atom, evInAtomOffsets[i]);
                            aev->time.frames = time;
                            aev->body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                            aev->body.size   = minEvent->size;
                            memcpy(LV2_ATOM_BODY(&aev->body), minEvent->data, minEvent->size);

                            const uint32_t evInPadSize    = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + minEvent->size);
                            evInAtomOffsets[i]           += evInPadSize;
                            evIn.data[i].atom->atom.size += evInPadSize;
                        }
                        else if (evIn.data[i].type & CARLA_EVENT_DATA_EVENT)
                        {
                            lv2_event_write(&evInEventIters[i], time, 0, CARLA_URI_MAP_ID_MIDI_EVENT, minEvent->size, minEvent->data);
                        }
                        else if (evIn.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
                        {
                            lv2midi_put_event(&evInMidiStates[i], time, minEvent->size, minEvent->data);
                        }

                        if (MIDI_IS_STATUS_NOTE_OFF(status))
                            postponeEvent(PluginPostEventNoteOff, channel, minEvent->data[1], 0.0);
                        else if (MIDI_IS_STATUS_NOTE_ON(status))
                            postponeEvent(PluginPostEventNoteOn, channel, minEvent->data[1], minEvent->data[2]);
                    }

                    midiEventCount += 1;
                }
            } // End of MIDI Input (System)

            // ----------------------------------------------------------------------------------------------------
            // Message Input

            {
                for (i=0; i < evIn.count; i++)
                {
                    if (! evIn.data[i].type & CARLA_EVENT_TYPE_MESSAGE)
                        continue;

#if 0
                     // send transport info if changed
                    const CarlaTimeInfo* const timeInfo = x_engine->getTimeInfo();

                    if (timeInfo->playing != lastTimePosPlaying || timeInfo->frame != lastTimePosFrame)
                    {
                        uint8_t posBuf[256];
                        LV2_Atom* const lv2Pos = (LV2_Atom*)posBuf;

                        LV2_Atom_Forge tempForge;
                        LV2_Atom_Forge* const forge = &tempForge;
                        lv2_atom_forge_set_buffer(forge, posBuf, sizeof(uint8_t)*256);

                        //LV2_Atom_Forge_Frame frame;
                        //lv2_atom_forge_blank(forge, &frame, 1, jalv->urids.time_Position);
                        //lv2_atom_forge_property_head(forge, jalv->urids.time_frame, 0);
                        lv2_atom_forge_long(forge, timeInfo->frame);
                        //lv2_atom_forge_property_head(forge, jalv->urids.time_position, 0);
                        lv2_atom_forge_long(forge, timeInfo->time);
                        //lv2_atom_forge_property_head(forge, jalv->urids.time_speed, 0);
                        lv2_atom_forge_float(forge, timeInfo->playing ? 1.0 : 0.0);

                        if (timeInfo->valid & CarlaEngineTimeBBT)
                        {
                            //lv2_atom_forge_property_head(forge, jalv->urids.time_bar, 0);
                            lv2_atom_forge_float(forge, timeInfo->bbt.bar - 1);

                            //lv2_atom_forge_property_head(forge, jalv->urids.time_barBeat, 0);
                            lv2_atom_forge_float(forge, timeInfo->bbt.beat - 1 + (timeInfo->bbt.tick / timeInfo->bbt.ticks_per_beat));

                            //lv2_atom_forge_property_head(forge, jalv->urids.time_beat, 0);
                            lv2_atom_forge_float(forge, timeInfo->bbt.beat - 1); // FIXME: -1 ?

                            //lv2_atom_forge_property_head(forge, jalv->urids.time_beatUnit, 0);
                            lv2_atom_forge_float(forge, timeInfo->bbt.beat_type);

                            //lv2_atom_forge_property_head(forge, jalv->urids.time_beatsPerBar, 0);
                            lv2_atom_forge_float(forge, timeInfo->bbt.beats_per_bar);

                            //lv2_atom_forge_property_head(forge, jalv->urids.time_beatsPerMinute, 0);
                            lv2_atom_forge_float(forge, timeInfo->bbt.beats_per_minute);
                        }

                        LV2_Atom_Event* const aev = getLv2AtomEvent(evIn.data[i].atom, evInAtomOffsets[i]);
                        aev->time.frames = framesOffset;
                        aev->body.type   = lv2Pos->type;
                        aev->body.size   = lv2Pos->size;
                        memcpy(LV2_ATOM_BODY(&aev->body), LV2_ATOM_BODY(lv2Pos), lv2Pos->size);

                        const uint32_t evInPadSize    = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + lv2Pos->size);
                        evInAtomOffsets[i]           += evInPadSize;
                        evIn.data[i].atom->atom.size += evInPadSize;

                        lastTimePosPlaying = timeInfo->playing;
                        lastTimePosFrame   = timeInfo->playing ? timeInfo->frame + frames : timeInfo->frame;
                    }
#endif

                    atomQueueIn.lock();

                    if (! atomQueueIn.isEmpty())
                    {
                        uint32_t portIndex;
                        const LV2_Atom* atom;

                        while (atomQueueIn.get(&portIndex, &atom))
                        {
                            if (atom->type == CARLA_URI_MAP_ID_ATOM_WORKER)
                            {
                                const LV2_Atom_Worker* const atomWorker = (const LV2_Atom_Worker*)atom;
                                ext.worker->work_response(handle, atomWorker->body.size, atomWorker->body.data);
                                continue;
                            }

                            LV2_Atom_Event* const aev = getLv2AtomEvent(evIn.data[i].atom, evInAtomOffsets[i]);
                            aev->time.frames = framesOffset;
                            aev->body.type   = atom->type;
                            aev->body.size   = atom->size;
                            memcpy(LV2_ATOM_BODY(&aev->body), LV2_ATOM_BODY(atom), atom->size);

                            const uint32_t evInPadSize    = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + atom->size);
                            evInAtomOffsets[i]           += evInPadSize;
                            evIn.data[i].atom->atom.size += evInPadSize;
                        }
                    }

                    atomQueueIn.unlock();
                }
            }

        } // End of Event Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Special Parameters

        {
            int32_t rindex;
            const CarlaEngineTimeInfo* const timeInfo = x_engine->getTimeInfo();

            for (k=0; k < param.count; k++)
            {
                if (param.data[k].type == PARAMETER_LATENCY)
                {
                    // TODO
                }
                else if (param.data[k].type == PARAMETER_LV2_FREEWHEEL)
                {
                    setParameterValue(k, x_engine->isOffline() ? 1.0 : 0.0, false, false, false);
                }
                else if (param.data[k].type == PARAMETER_LV2_TIME)
                {
                    rindex = param.data[k].rindex;
                    CARLA_ASSERT(rindex >= 0 && rindex < (int32_t)rdf_descriptor->PortCount);

                    switch (rdf_descriptor->Ports[rindex].Designation)
                    {
                    // Non-BBT
                    case LV2_PORT_DESIGNATION_TIME_FRAME:
                        setParameterValue(k, timeInfo->frame, false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND:
                        break;
                    case LV2_PORT_DESIGNATION_TIME_POSITION:
                        setParameterValue(k, timeInfo->time, false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_SPEED:
                        setParameterValue(k, timeInfo->playing ? 1.0 : 0.0, false, false, false);
                        break;
                    // BBT
                    case LV2_PORT_DESIGNATION_TIME_BAR:
                        if (timeInfo->valid & CarlaEngineTimeBBT)
                            setParameterValue(k, timeInfo->bbt.bar - 1, false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_BAR_BEAT:
                        if (timeInfo->valid & CarlaEngineTimeBBT)
                            setParameterValue(k, float(timeInfo->bbt.beat - 1) + (float(timeInfo->bbt.tick) / timeInfo->bbt.ticks_per_beat), false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_BEAT:
                        if (timeInfo->valid & CarlaEngineTimeBBT)
                            setParameterValue(k, timeInfo->bbt.beat - 1, false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_BEAT_UNIT:
                        if (timeInfo->valid & CarlaEngineTimeBBT)
                            setParameterValue(k, timeInfo->bbt.beat_type, false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR:
                        if (timeInfo->valid & CarlaEngineTimeBBT)
                            setParameterValue(k, timeInfo->bbt.beats_per_bar, false, false, false);
                        break;
                    case LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE:
                        if (timeInfo->valid & CarlaEngineTimeBBT)
                            setParameterValue(k, timeInfo->bbt.beats_per_minute, false, false, false);
                        break;
                    }
                }
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (! m_activeBefore)
            {
                if (evIn.count > 0)
                {
                    for (i=0; i < MAX_MIDI_CHANNELS; i++)
                    {
                        uint8_t midiEvent1[2] = { 0 };
                        midiEvent1[0] = MIDI_STATUS_CONTROL_CHANGE + i;
                        midiEvent1[1] = MIDI_CONTROL_ALL_SOUND_OFF;

                        uint8_t midiEvent2[2] = { 0 };
                        midiEvent2[0] = MIDI_STATUS_CONTROL_CHANGE + i;
                        midiEvent2[1] = MIDI_CONTROL_ALL_SOUND_OFF;

                        // send to all midi inputs
                        for (k=0; k < evIn.count; k++)
                        {
                            if (evIn.data[k].type & CARLA_EVENT_TYPE_MIDI)
                            {
                                if (evIn.data[k].type & CARLA_EVENT_DATA_ATOM)
                                {
                                    const uint32_t padSize = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + 2);

                                    // all sound off
                                    LV2_Atom_Event* const aev1 = getLv2AtomEvent(evIn.data[k].atom, evInAtomOffsets[k]);
                                    aev1->time.frames = 0;
                                    aev1->body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                                    aev1->body.size   = 2;
                                    memcpy(LV2_ATOM_BODY(&aev1->body), midiEvent1, 2);

                                    evInAtomOffsets[k]           += padSize;
                                    evIn.data[k].atom->atom.size += padSize;

                                    // all notes off
                                    LV2_Atom_Event* const aev2 = getLv2AtomEvent(evIn.data[k].atom, evInAtomOffsets[k]);
                                    aev2->time.frames = 0;
                                    aev2->body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                                    aev2->body.size   = 2;
                                    memcpy(LV2_ATOM_BODY(&aev2->body), midiEvent2, 2);

                                    evInAtomOffsets[k]           += padSize;
                                    evIn.data[k].atom->atom.size += padSize;
                                }
                                else if (evIn.data[k].type & CARLA_EVENT_DATA_EVENT)
                                {
                                    lv2_event_write(&evInEventIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 2, midiEvent1);
                                    lv2_event_write(&evInEventIters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 2, midiEvent2);
                                }
                                else if (evIn.data[k].type & CARLA_EVENT_DATA_MIDI_LL)
                                {
                                    lv2midi_put_event(&evInMidiStates[k], 0, 2, midiEvent1);
                                    lv2midi_put_event(&evInMidiStates[k], 0, 2, midiEvent2);
                                }
                            }
                        }
                    }

                    //midiEventCount = MAX_MIDI_CHANNELS*2;
                }

                if (m_latency > 0)
                {
                    for (i=0; i < aIn.count; i++)
                        memset(m_latencyBuffers[i], 0, sizeof(float)*m_latency);
                }

                if (descriptor->activate)
                {
                    descriptor->activate(handle);
                    if (h2) descriptor->activate(h2);
                }
            }

            for (i=0; i < aIn.count; i++)
            {
                if (i == 0 || ! h2) descriptor->connect_port(handle, aIn.rindexes[i], inBuffer[i]);
                else if (i == 1)    descriptor->connect_port(h2, aIn.rindexes[i], inBuffer[i]);
            }

            for (i=0; i < aOut.count; i++)
            {
                if (i == 0 || ! h2) descriptor->connect_port(handle, aOut.rindexes[i], outBuffer[i]);
                else if (i == 1)    descriptor->connect_port(h2, aOut.rindexes[i], outBuffer[i]);
            }

            descriptor->run(handle, frames);
            if (h2) descriptor->run(h2, frames);

            if (ext.worker && ext.worker->end_run)
            {
                ext.worker->end_run(handle);
                if (h2) ext.worker->end_run(h2);
            }
        }
        else
        {
            if (m_activeBefore)
            {
                if (descriptor->deactivate)
                {
                    descriptor->deactivate(handle);
                    if (h2) descriptor->deactivate(h2);
                }
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        if (m_active)
        {
            bool do_drywet  = (m_hints & PLUGIN_CAN_DRYWET) > 0 && x_dryWet != 1.0;
            bool do_volume  = (m_hints & PLUGIN_CAN_VOLUME) > 0 && x_volume != 1.0;
            bool do_balance = (m_hints & PLUGIN_CAN_BALANCE) > 0 && (x_balanceLeft != -1.0 || x_balanceRight != 1.0);

            double bal_rangeL, bal_rangeR;
            float bufValue, oldBufLeft[do_balance ? frames : 1];

            for (i=0; i < aOut.count; i++)
            {
                // Dry/Wet
                if (do_drywet)
                {
                    for (k=0; k < frames; k++)
                    {
                        if (k < m_latency && m_latency < frames)
                            bufValue = (aIn.count == 1) ? m_latencyBuffers[0][k] : m_latencyBuffers[i][k];
                        else
                            bufValue = (aIn.count == 1) ? inBuffer[0][k-m_latency] : inBuffer[i][k-m_latency];

                        outBuffer[i][k] = (outBuffer[i][k]*x_dryWet)+(bufValue*(1.0-x_dryWet));
                    }
                }

                // Balance
                if (do_balance)
                {
                    if (i%2 == 0)
                        memcpy(&oldBufLeft, outBuffer[i], sizeof(float)*frames);

                    bal_rangeL = (x_balanceLeft+1.0)/2;
                    bal_rangeR = (x_balanceRight+1.0)/2;

                    for (k=0; k < frames; k++)
                    {
                        if (i%2 == 0)
                        {
                            // left output
                            outBuffer[i][k]  = oldBufLeft[k]*(1.0-bal_rangeL);
                            outBuffer[i][k] += outBuffer[i+1][k]*(1.0-bal_rangeR);
                        }
                        else
                        {
                            // right
                            outBuffer[i][k]  = outBuffer[i][k]*bal_rangeR;
                            outBuffer[i][k] += oldBufLeft[k]*bal_rangeL;
                        }
                    }
                }

                // Volume
                if (do_volume)
                {
                    for (k=0; k < frames; k++)
                        outBuffer[i][k] *= x_volume;
                }

                // Output VU
                if (x_engine->getOptions().processMode != PROCESS_MODE_CONTINUOUS_RACK)
                {
                    for (k=0; i < 2 && k < frames; k++)
                    {
                        if (std::abs(outBuffer[i][k]) > aOutsPeak[i])
                            aOutsPeak[i] = std::abs(outBuffer[i][k]);
                    }
                }
            }

            // Latency, save values for next callback
            if (m_latency > 0 && m_latency < frames)
            {
                for (i=0; i < aIn.count; i++)
                    memcpy(m_latencyBuffers[i], inBuffer[i] + (frames - m_latency), sizeof(float)*m_latency);
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aOut.count; i++)
                carla_zeroF(outBuffer[i], frames);

            aOutsPeak[0] = 0.0;
            aOutsPeak[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (param.portCout && m_active)
        {
            double value;

            for (k=0; k < param.count; k++)
            {
                if (param.data[k].type == PARAMETER_OUTPUT)
                {
                    fixParameterValue(paramBuffers[k], param.ranges[k]);

                    if (param.data[k].midiCC > 0)
                    {
                        value = (paramBuffers[k] - param.ranges[k].min) / (param.ranges[k].max - param.ranges[k].min);
                        param.portCout->writeEvent(CarlaEngineParameterChangeEvent, framesOffset, param.data[k].midiChannel, param.data[k].midiCC, value);
                    }
                }
            }
        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Event Output

        if (evOut.count > 0 && m_active)
        {
            atomQueueOut.lock();

            for (i=0; i < evOut.count; i++)
            {
                // midi events need the midi port to send events to
                if ((evOut.data[i].type & CARLA_EVENT_TYPE_MIDI) > 0 && ! evOut.data[i].port)
                    continue;

                if (evOut.data[i].type & CARLA_EVENT_DATA_ATOM)
                {
                    int32_t size   = evOut.data[i].atom->atom.size - sizeof(LV2_Atom_Sequence_Body);
                    int32_t offset = 0;

                    while (offset < size)
                    {
                        qDebug("output event??, offset:%i, size:%i", offset, size);
                        const LV2_Atom_Event* const aev = (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, evOut.data[i].atom) + offset);

                        if ((! aev) || aev->body.type == CARLA_URI_MAP_ID_NULL)
                            break;

                        qDebug("output event ------------------------------ YES!");

                        if (aev->body.type == CARLA_URI_MAP_ID_MIDI_EVENT)
                        {
                            const unsigned char* const data = (unsigned char*)LV2_ATOM_BODY(&aev->body);
                            evOut.data[i].port->writeEvent(aev->time.frames, data, aev->body.size);
                        }
                        else if (aev->body.type == CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT)
                        {
                            if (! atomQueueOut.isFull())
                                atomQueueOut.put(evOut.data[i].rindex, &aev->body);
                        }

                        offset += lv2_atom_pad_size(sizeof(LV2_Atom_Event) + aev->body.size);
                    }
                }
                else if (evOut.data[i].type & CARLA_EVENT_DATA_EVENT)
                {
                    const LV2_Event* ev;
                    LV2_Event_Iterator iter;

                    uint8_t* data;
                    lv2_event_begin(&iter, evOut.data[i].event);

                    for (k=0; k < iter.buf->event_count; k++)
                    {
                        ev = lv2_event_get(&iter, &data);

                        if (ev && ev->type == CARLA_URI_MAP_ID_MIDI_EVENT && data)
                            evOut.data[i].port->writeEvent(ev->frames, data, ev->size);

                        lv2_event_increment(&iter);
                    }
                }
                else if (evOut.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
                {
                    LV2_MIDIState state = { evOut.data[i].midi, frames, 0 };

                    uint32_t eventSize;
                    double   eventTime;
                    unsigned char* eventData;

                    while (lv2midi_get_event(&state, &eventTime, &eventSize, &eventData) < frames)
                    {
                        evOut.data[i].port->writeEvent(eventTime, eventData, eventSize);
                        lv2midi_step(&state);
                    }
                }
            }

            atomQueueOut.unlock();

        } // End of Event Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        x_engine->setInputPeak(m_id, 0, aInsPeak[0]);
        x_engine->setInputPeak(m_id, 1, aInsPeak[1]);
        x_engine->setOutputPeak(m_id, 0, aOutsPeak[0]);
        x_engine->setOutputPeak(m_id, 1, aOutsPeak[1]);

        m_activeBefore = m_active;
    }

    void bufferSizeChanged(const uint32_t newBufferSize)
    {
        lv2Options.bufferSize = newBufferSize;
    }

    // -------------------------------------------------------------------
    // Post-poned events

    void postEventHandleCustom(const int32_t size, const int32_t, const double, const void* const data)
    {
        qDebug("Lv2Plugin::postEventHandleCustom(%i, %p)", size, data);
        CARLA_ASSERT(ext.worker && ext.worker->work);

        if (ext.worker && ext.worker->work)
            ext.worker->work(handle, carla_lv2_worker_respond, this, size, data);
    }

    void uiParameterChange(const uint32_t index, const double value)
    {
        CARLA_ASSERT(index < param.count);

        if (index >= param.count)
            return;

        if (gui.type == GUI_EXTERNAL_OSC)
        {
            if (osc.data.target)
                osc_send_control(&osc.data, param.data[index].rindex, value);
        }
        else
        {
            if (ui.handle && ui.descriptor && ui.descriptor->port_event)
            {
                float valueF = value;
                ui.descriptor->port_event(ui.handle, param.data[index].rindex, sizeof(float), 0, &valueF);
            }
        }
    }

    void uiMidiProgramChange(const uint32_t index)
    {
        CARLA_ASSERT(index < midiprog.count);

        if (index >= midiprog.count)
            return;

        if (gui.type == GUI_EXTERNAL_OSC)
        {
            if (osc.data.target)
                osc_send_midi_program(&osc.data, midiprog.data[index].bank, midiprog.data[index].program);
        }
        else
        {
            if (ext.uiprograms)
                ext.uiprograms->select_program(ui.handle, midiprog.data[index].bank, midiprog.data[index].program);
        }
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
    {
        CARLA_ASSERT(channel < 16);
        CARLA_ASSERT(note < 128);
        CARLA_ASSERT(velo > 0 && velo < 128);

        if (gui.type == GUI_EXTERNAL_OSC)
        {
            if (osc.data.target)
            {
                uint8_t midiData[4] = { 0 };
                midiData[1] = MIDI_STATUS_NOTE_ON + channel;
                midiData[2] = note;
                midiData[3] = velo;
                osc_send_midi(&osc.data, midiData);
            }
        }
        else
        {
            if (ui.handle && ui.descriptor && ui.descriptor->port_event)
            {
                LV2_Atom_MidiEvent midiEv;
                midiEv.event.time.frames = 0;
                midiEv.event.body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                midiEv.event.body.size   = 3;
                midiEv.data[0] = MIDI_STATUS_NOTE_OFF + channel;
                midiEv.data[1] = note;
                midiEv.data[2] = velo;

                ui.descriptor->port_event(ui.handle, 0, 3, CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, &midiEv);
            }
        }
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note)
    {
        CARLA_ASSERT(channel < 16);
        CARLA_ASSERT(note < 128);

        if (gui.type == GUI_EXTERNAL_OSC)
        {
            if (osc.data.target)
            {
                uint8_t midiData[4] = { 0 };
                midiData[1] = MIDI_STATUS_NOTE_OFF + channel;
                midiData[2] = note;
                osc_send_midi(&osc.data, midiData);
            }
        }
        else
        {
            if (ui.handle && ui.descriptor && ui.descriptor->port_event)
            {
                LV2_Atom_MidiEvent midiEv;
                midiEv.event.time.frames = 0;
                midiEv.event.body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                midiEv.event.body.size   = 3;
                midiEv.data[0] = MIDI_STATUS_NOTE_OFF + channel;
                midiEv.data[1] = note;
                midiEv.data[2] = 0;

                ui.descriptor->port_event(ui.handle, 0, 3, CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, &midiEv);
            }
        }
    }

    // -------------------------------------------------------------------
    // Cleanup

    void removeClientPorts()
    {
        qDebug("Lv2Plugin::removeClientPorts() - start");

        for (uint32_t i=0; i < evIn.count; i++)
        {
            if (evIn.data[i].port)
            {
                delete evIn.data[i].port;
                evIn.data[i].port = nullptr;
            }
        }

        for (uint32_t i=0; i < evOut.count; i++)
        {
            if (evOut.data[i].port)
            {
                delete evOut.data[i].port;
                evOut.data[i].port = nullptr;
            }
        }

        CarlaPlugin::removeClientPorts();

        qDebug("Lv2Plugin::removeClientPorts() - end");
    }

    void initBuffers()
    {
        uint32_t i;

        for (i=0; i < evIn.count; i++)
        {
            if (evIn.data[i].port)
                evIn.data[i].port->initBuffer(x_engine);
        }

        for (uint32_t i=0; i < evOut.count; i++)
        {
            if (evOut.data[i].port)
                evOut.data[i].port->initBuffer(x_engine);
        }

        CarlaPlugin::initBuffers();
    }

    void deleteBuffers()
    {
        qDebug("Lv2Plugin::deleteBuffers() - start");

        if (evIn.count > 0)
        {
            for (uint32_t i=0; i < evIn.count; i++)
            {
                if (evIn.data[i].type & CARLA_EVENT_DATA_ATOM)
                {
                    free(evIn.data[i].atom);
                }
                else if (evIn.data[i].type & CARLA_EVENT_DATA_EVENT)
                {
                    free(evIn.data[i].event);
                }
                else if (evIn.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
                {
                    delete[] evIn.data[i].midi->data;
                    delete evIn.data[i].midi;
                }
            }

            delete[] evIn.data;
        }

        if (evOut.count > 0)
        {
            for (uint32_t i=0; i < evOut.count; i++)
            {
                if (evOut.data[i].type & CARLA_EVENT_DATA_ATOM)
                {
                    free(evOut.data[i].atom);
                }
                else if (evOut.data[i].type & CARLA_EVENT_DATA_EVENT)
                {
                    free(evOut.data[i].event);
                }
                else if (evOut.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
                {
                    delete[] evOut.data[i].midi->data;
                    delete evOut.data[i].midi;
                }
            }

            delete[] evOut.data;
        }

        if (param.count > 0)
            delete[] paramBuffers;

        evIn.count = 0;
        evIn.data  = nullptr;

        evOut.count = 0;
        evOut.data  = nullptr;

        paramBuffers = nullptr;

        CarlaPlugin::deleteBuffers();

        qDebug("Lv2Plugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------

    uint32_t getCustomURID(const char* const uri)
    {
        qDebug("Lv2Plugin::getCustomURID(%s)", uri);
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
        qDebug("Lv2Plugin::getCustomURIString(%i)", urid);
        CARLA_ASSERT(urid > CARLA_URI_MAP_ID_NULL);

        if (urid == CARLA_URI_MAP_ID_NULL)
            return nullptr;
        if (urid < customURIDs.size())
            return customURIDs[urid];

        return nullptr;
    }

    // -------------------------------------------------------------------

    void handleTransferAtom(const int32_t portIndex, const LV2_Atom* const atom)
    {
        qDebug("Lv2Plugin::handleTransferAtom(%i, %p)", portIndex, atom);
        CARLA_ASSERT(portIndex >= 0);
        CARLA_ASSERT(atom);

        atomQueueIn.put(portIndex, atom);
    }

    void handleTransferEvent(const int32_t portIndex, const LV2_Atom* const atom)
    {
        qDebug("Lv2Plugin::handleTransferEvent(%i, %p)", portIndex, atom);
        CARLA_ASSERT(portIndex >= 0);
        CARLA_ASSERT(atom);

        atomQueueIn.put(portIndex, atom);
    }

    // -------------------------------------------------------------------

    void handleProgramChanged(const int32_t index)
    {
        if (index == -1)
        {
            const CarlaPlugin::ScopedDisabler m(this);
            reloadPrograms(false);
        }
        else
        {
            if (index >= 0 && index < (int32_t)prog.count && ext.programs)
            {
                const char* const progName = ext.programs->get_program(handle, index)->name;
                CARLA_ASSERT(progName);

                if (prog.names[index])
                    free((void*)prog.names[index]);

                prog.names[index] = strdup(progName ? progName : "");
            }
        }

        x_engine->callback(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0, nullptr);
    }

    LV2_State_Status handleStateStore(const uint32_t key, const void* const value, const size_t size, const uint32_t type, const uint32_t flags)
    {
        CARLA_ASSERT(key > 0);
        CARLA_ASSERT(value);

        const char* const stype  = getCustomURIString(type);
        const char* const uriKey = getCustomURIString(key);

        // do basic checks
        if (! key)
        {
            qWarning("Lv2Plugin::handleStateStore(%i, %p, " P_SIZE ", %i, %i) - invalid key", key, value, size, type, flags);
            return LV2_STATE_ERR_NO_PROPERTY;
        }

        if (! value)
        {
            qWarning("Lv2Plugin::handleStateStore(%i, %p, " P_SIZE ", %i, %i) - invalid value", key, value, size, type, flags);
            return LV2_STATE_ERR_NO_PROPERTY;
        }

        if (! uriKey)
        {
            qWarning("Lv2Plugin::handleStateStore(%i, %p, " P_SIZE ", %i, %i) - invalid key URI", key, value, size, type, flags);
            return LV2_STATE_ERR_NO_PROPERTY;
        }

        if (! flags & LV2_STATE_IS_POD)
        {
            qWarning("Lv2Plugin::handleStateStore(%i, %p, " P_SIZE ", %i, %i) - invalid flags", key, value, size, type, flags);
            return LV2_STATE_ERR_BAD_FLAGS;
        }

        if (! stype)
        {
            qCritical("Lv2Plugin::handleStateStore(%i, %p, " P_SIZE ", %i, %i) - invalid type", key, value, size, type, flags);
            return LV2_STATE_ERR_BAD_TYPE;
        }

        // Check if we already have this key
        for (size_t i=0; i < custom.size(); i++)
        {
            if (strcmp(custom[i].key, uriKey) == 0)
            {
                if (custom[i].value)
                    free((void*)custom[i].value);

                if (strcmp(stype, LV2_ATOM__String) == 0 || strcmp(stype, LV2_ATOM__Path) == 0)
                    custom[i].value = strdup((const char*)value);
                else
                    custom[i].value = strdup(QByteArray((const char*)value, size).toBase64().constData());

                return LV2_STATE_SUCCESS;
            }
        }

        // Otherwise store it
        CustomData newData;
        newData.type = strdup(stype);
        newData.key  = strdup(uriKey);

        if (strcmp(stype, LV2_ATOM__String) == 0 || strcmp(stype, LV2_ATOM__Path) == 0)
            newData.value = strdup((const char*)value);
        else
            newData.value = strdup(QByteArray((const char*)value, size).toBase64().constData());

        custom.push_back(newData);

        return LV2_STATE_SUCCESS;
    }

    const void* handleStateRetrieve(const uint32_t key, size_t* const size, uint32_t* const type, uint32_t* const flags)
    {
        CARLA_ASSERT(key > CARLA_URI_MAP_ID_NULL);

        const char* const uriKey = getCustomURIString(key);

        if (! uriKey)
        {
            qCritical("Lv2Plugin::handleStateRetrieve(%i, %p, %p, %p) - failed to find key", key, size, type, flags);
            return nullptr;
        }

        const char* stype = nullptr;
        const char* stringData = nullptr;

        for (size_t i=0; i < custom.size(); i++)
        {
            if (strcmp(custom[i].key, uriKey) == 0)
            {
                stype      = custom[i].type;
                stringData = custom[i].value;
                break;
            }
        }

        if (! stringData)
        {
            qCritical("Lv2Plugin::handleStateRetrieve(%i, %p, %p, %p) - invalid key '%s'", key, size, type, flags, uriKey);
            return nullptr;
        }

        *size  = 0;
        *type  = key;
        *flags = LV2_STATE_IS_POD;

        if (strcmp(stype, LV2_ATOM__String) == 0)
        {
            *size = strlen(stringData);
            return stringData;
        }
        else if (strcmp(stype, LV2_ATOM__Path) == 0)
        {
            *size = strlen(stringData);
            return stringData;
        }
        else
        {
            static QByteArray chunk;
            chunk = QByteArray::fromBase64(stringData);
            *size = chunk.size();
            return chunk.constData();
        }

        qCritical("Lv2Plugin::handleStateRetrieve(%i, %p, %p, %p) - invalid key type '%s'", key, size, type, flags, stype);
        return nullptr;
    }

    LV2_Worker_Status handleWorkerSchedule(const uint32_t size, const void* const data)
    {
        if (! ext.worker)
        {
            qWarning("Lv2Plugin::handleWorkerSchedule(%i, %p) - plugin has no worker", size, data);
            return LV2_WORKER_ERR_UNKNOWN;
        }

        if (x_engine->isOffline())
            ext.worker->work(handle, carla_lv2_worker_respond, this, size, data);
        else
            postponeEvent(PluginPostEventCustom, size, 0, 0.0, data);

        return LV2_WORKER_SUCCESS;
    }

    LV2_Worker_Status handleWorkerRespond(const uint32_t size, const void* const data)
    {
        LV2_Atom_Worker workerAtom;
        workerAtom.atom.type = CARLA_URI_MAP_ID_ATOM_WORKER;
        workerAtom.atom.size = sizeof(LV2_Atom_Worker_Body);
        workerAtom.body.size = size;
        workerAtom.body.data = data;

        atomQueueIn.put(0, (const LV2_Atom*)&workerAtom);

        return LV2_WORKER_SUCCESS;
    }

    void handleExternalUiClosed()
    {
        if (ui.handle && ui.descriptor && ui.descriptor->cleanup)
            ui.descriptor->cleanup(ui.handle);

        ui.handle = nullptr;
        x_engine->callback(CALLBACK_SHOW_GUI, m_id, 0, 0, 0.0, nullptr);
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

    int handleUiResize(const int width, const int height)
    {
        CARLA_ASSERT(width > 0);
        CARLA_ASSERT(height > 0);

        if (width <= 0 || height <= 0)
            return 1;

        gui.width  = width;
        gui.height = height;
        x_engine->callback(CALLBACK_RESIZE_GUI, m_id, width, height, 0.0, nullptr);

        return 0;
    }

    void handleUiWrite(const uint32_t rindex, const uint32_t bufferSize, const uint32_t format, const void* const buffer)
    {
        if (format == 0)
        {
            CARLA_ASSERT(buffer);
            CARLA_ASSERT(bufferSize == sizeof(float));

            if (bufferSize != sizeof(float) || ! buffer)
                return;

            float value = *(float*)buffer;

            for (uint32_t i=0; i < param.count; i++)
            {
                if (param.data[i].rindex == (int32_t)rindex)
                    return setParameterValue(i, value, false, true, true);
            }
        }
        else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM)
        {
            CARLA_ASSERT(buffer);

            if (! buffer)
                return;

            const LV2_Atom* const atom = (const LV2_Atom*)buffer;
            handleTransferAtom(rindex, atom);
        }
        else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT)
        {
            CARLA_ASSERT(buffer);

            if (! buffer)
                return;

            const LV2_Atom* const atom = (const LV2_Atom*)buffer;
            handleTransferEvent(rindex, atom);
        }
    }

    // -------------------------------------------------------------------

    const char* getUiBridgePath(const LV2_Property type)
    {
        const CarlaEngineOptions options(x_engine->getOptions());

        switch (type)
        {
        case LV2_UI_GTK2:
            return options.bridge_lv2gtk2;
        case LV2_UI_GTK3:
            return options.bridge_lv2gtk3;
        case LV2_UI_QT4:
            return options.bridge_lv2qt4;
        case LV2_UI_COCOA:
            return options.bridge_lv2cocoa;
        case LV2_UI_WINDOWS:
            return options.bridge_lv2win;
        case LV2_UI_X11:
            return options.bridge_lv2x11;
        default:
            return nullptr;
        }
    }

    bool isUiBridgeable(const uint32_t uiId)
    {
        CARLA_ASSERT(rdf_descriptor);
        CARLA_ASSERT(uiId < rdf_descriptor->UICount);

        if (uiId >= rdf_descriptor->UICount)
            return false;

        const LV2_RDF_UI* const rdf_ui = &rdf_descriptor->UIs[uiId];

        CARLA_ASSERT(rdf_ui && rdf_ui->URI);

        // Calf Analyzer is useless without instance-data
        if (strcmp(rdf_ui->URI, "http://calf.sourceforge.net/plugins/Analyzer") == 0)
            return false;

        for (uint32_t i=0; i < rdf_ui->FeatureCount; i++)
        {
            CARLA_ASSERT(rdf_ui->Features[i].URI);

            if (strcmp(rdf_ui->Features[i].URI, LV2_INSTANCE_ACCESS_URI) == 0 || strcmp(rdf_ui->Features[i].URI, LV2_DATA_ACCESS_URI) == 0)
                return false;
        }

        return true;
    }

    bool isUiResizable()
    {
        CARLA_ASSERT(ui.rdf_descriptor);

        if (! ui.rdf_descriptor)
            return false;

        for (uint32_t i=0; i < ui.rdf_descriptor->FeatureCount; i++)
        {
            if (strcmp(ui.rdf_descriptor->Features[i].URI, LV2_UI__fixedSize) == 0 || strcmp(ui.rdf_descriptor->Features[i].URI, LV2_UI__noUserResize) == 0)
                return false;
        }

        return true;
    }

    void initExternalUi()
    {
        qDebug("Lv2Plugin::initExternalUi()");

        ui.widget = nullptr;
        ui.handle = ui.descriptor->instantiate(ui.descriptor, descriptor->URI, ui.rdf_descriptor->Bundle, carla_lv2_ui_write_function, this, &ui.widget, features);

        if (ui.handle && ui.widget)
        {
            updateUi();
        }
        else
        {
            qWarning("Lv2Plugin::initExternalUi() - failed to instantiate UI");
            ui.handle = nullptr;
            ui.widget = nullptr;
            x_engine->callback(CALLBACK_SHOW_GUI, m_id, -1, 0, 0.0, nullptr);
        }
    }

    void uiTransferCustomData(const CustomData* const cdata)
    {
        if (! (cdata->type && ui.handle && ui.descriptor && ui.descriptor->port_event))
            return;

        LV2_URID_Map* const URID_Map = (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;
        const LV2_URID uridPatchSet  = getCustomURID(LV2_PATCH__Set);
        const LV2_URID uridPatchBody = getCustomURID(LV2_PATCH__body);

        Sratom* const sratom = sratom_new(URID_Map);
        SerdChunk     chunk  = { nullptr, 0 };

        LV2_Atom_Forge forge;
        lv2_atom_forge_init(&forge, URID_Map);
        lv2_atom_forge_set_sink(&forge, sratom_forge_sink, sratom_forge_deref, &chunk);

        LV2_Atom_Forge_Frame refFrame, bodyFrame;
        LV2_Atom_Forge_Ref   ref = lv2_atom_forge_blank(&forge, &refFrame, 1, uridPatchSet);

        lv2_atom_forge_property_head(&forge, uridPatchBody, CARLA_URI_MAP_ID_NULL);
        lv2_atom_forge_blank(&forge, &bodyFrame, 2, CARLA_URI_MAP_ID_NULL);

        lv2_atom_forge_property_head(&forge, getCustomURID(cdata->key), CARLA_URI_MAP_ID_NULL);

        if (strcmp(cdata->type, LV2_ATOM__String) == 0)
            lv2_atom_forge_string(&forge, cdata->value, strlen(cdata->value));
        else if (strcmp(cdata->type, LV2_ATOM__Path) == 0)
            lv2_atom_forge_path(&forge, cdata->value, strlen(cdata->value));
        else if (strcmp(cdata->type, LV2_ATOM__Chunk) == 0)
            lv2_atom_forge_literal(&forge, cdata->value, strlen(cdata->value), CARLA_URI_MAP_ID_ATOM_CHUNK, CARLA_URI_MAP_ID_NULL);
        else
            lv2_atom_forge_literal(&forge, cdata->value, strlen(cdata->value), getCustomURID(cdata->key), CARLA_URI_MAP_ID_NULL);

        lv2_atom_forge_pop(&forge, &bodyFrame);
        lv2_atom_forge_pop(&forge, &refFrame);

        uint32_t portIndex = evIn.count > 0 ? evIn.data[0].rindex : 0;

        if (const LV2_Atom* const atom = lv2_atom_forge_deref(&forge, ref))
            ui.descriptor->port_event(ui.handle, portIndex, atom->size, CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);

        free((void*)chunk.buf);
        sratom_free(sratom);
    }

    void updateUi()
    {
        ext.uiprograms = nullptr;

        if (ui.handle && ui.descriptor)
        {
            if (ui.descriptor->extension_data)
            {
                ext.uiprograms = (const LV2_Programs_UI_Interface*)ui.descriptor->extension_data(LV2_PROGRAMS__UIInterface);

                if (ext.uiprograms && ! ext.uiprograms->select_program)
                    // invalid
                    ext.uiprograms = nullptr;

                if (ext.uiprograms && midiprog.count > 0 && midiprog.current >= 0)
                    ext.uiprograms->select_program(ui.handle, midiprog.data[midiprog.current].bank, midiprog.data[midiprog.current].program);
            }

            if (ui.descriptor->port_event)
            {
                // update control ports
                float valueF;
                for (uint32_t i=0; i < param.count; i++)
                {
                    valueF = getParameterValue(i);
                    ui.descriptor->port_event(ui.handle, param.data[i].rindex, sizeof(float), CARLA_URI_MAP_ID_NULL, &valueF);
                }
            }
        }
    }

    // ----------------- Event Feature ---------------------------------------------------

    static uint32_t carla_lv2_event_ref(const LV2_Event_Callback_Data callback_data, LV2_Event* const event)
    {
        qDebug("Lv2Plugin::carla_lv2_event_ref(%p, %p)", callback_data, event);
        CARLA_ASSERT(callback_data);
        CARLA_ASSERT(event);

        return 0;
    }

    static uint32_t carla_lv2_event_unref(const LV2_Event_Callback_Data callback_data, LV2_Event* const event)
    {
        qDebug("Lv2Plugin::carla_lv2_event_unref(%p, %p)", callback_data, event);
        CARLA_ASSERT(callback_data);
        CARLA_ASSERT(event);

        return 0;
    }

    // ----------------- Logs Feature ----------------------------------------------------

    static int carla_lv2_log_printf(const LV2_Log_Handle handle, const LV2_URID type, const char* const fmt, ...)
    {
        qDebug("Lv2Plugin::carla_lv2_log_printf(%p, %i, \"%s\", ...)", handle, type, fmt);
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
        qDebug("Lv2Plugin::carla_lv2_log_vprintf(%p, %i, \"%s\", ...)", handle, type, fmt);
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
        qDebug("Lv2Plugin::carla_lv2_program_changed(%p, %i)", handle, index);
        CARLA_ASSERT(handle);

        if (! handle)
            return;

        Lv2Plugin* const plugin = (Lv2Plugin*)handle;
        plugin->handleProgramChanged(index);
    }

    // ----------------- State Feature ---------------------------------------------------

    static char* carla_lv2_state_make_path(const LV2_State_Make_Path_Handle handle, const char* const path)
    {
        qDebug("Lv2Plugin::carla_lv2_state_make_path(%p, \"%s\")", handle, path);
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
        qDebug("Lv2Plugin::carla_lv2_state_map_abstract_path(%p, \"%s\")", handle, absolute_path);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(absolute_path);

        if (! absolute_path)
            return nullptr;

        QDir dir(absolute_path);
        return strdup(dir.canonicalPath().toUtf8().constData());
    }

    static char* carla_lv2_state_map_absolute_path(const LV2_State_Map_Path_Handle handle, const char* const abstract_path)
    {
        qDebug("Lv2Plugin::carla_lv2_state_map_absolute_path(%p, \"%s\")", handle, abstract_path);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(abstract_path);

        if (! abstract_path)
            return nullptr;

        QDir dir(abstract_path);
        return strdup(dir.absolutePath().toUtf8().constData());
    }

    static LV2_State_Status carla_lv2_state_store(const LV2_State_Handle handle, const uint32_t key, const void* const value, const size_t size, const uint32_t type, const uint32_t flags)
    {
        qDebug("Lv2Plugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i)", handle, key, value, size, type, flags);
        CARLA_ASSERT(handle);

        if (! handle)
            return LV2_STATE_ERR_UNKNOWN;

        Lv2Plugin* const plugin = (Lv2Plugin*)handle;
        return plugin->handleStateStore(key, value, size, type, flags);
    }

    static const void* carla_lv2_state_retrieve(const LV2_State_Handle handle, const uint32_t key, size_t* const size, uint32_t* const type, uint32_t* const flags)
    {
        qDebug("Lv2Plugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p)", handle, key, size, type, flags);
        CARLA_ASSERT(handle);

        if (! handle)
            return nullptr;

        Lv2Plugin* const plugin = (Lv2Plugin*)handle;
        return plugin->handleStateRetrieve(key, size, type, flags);
    }

    // ----------------- URI-Map Feature -------------------------------------------------

    static uint32_t carla_lv2_uri_to_id(const LV2_URI_Map_Callback_Data data, const char* const map, const char* const uri)
    {
        qDebug("Lv2Plugin::carla_lv2_uri_to_id(%p, \"%s\", \"%s\")", data, map, uri);
        return carla_lv2_urid_map((LV2_URID_Map_Handle*)data, uri);
    }

    // ----------------- URID Feature ----------------------------------------------------

    static LV2_URID carla_lv2_urid_map(const LV2_URID_Map_Handle handle, const char* const uri)
    {
        qDebug("Lv2Plugin::carla_lv2_urid_map(%p, \"%s\")", handle, uri);
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
        Lv2Plugin* const plugin = (Lv2Plugin*)handle;
        return plugin->getCustomURID(uri);
    }

    static const char* carla_lv2_urid_unmap(const LV2_URID_Map_Handle handle, const LV2_URID urid)
    {
        qDebug("Lv2Plugin::carla_lv2_urid_unmap(%p, %i)", handle, urid);
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
        if (urid == CARLA_URI_MAP_ID_ATOM_WORKER)
            return nullptr; // not a valid URID, only used internally
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
        Lv2Plugin* const plugin = (Lv2Plugin*)handle;
        return plugin->getCustomURIString(urid);
    }

    // ----------------- Worker Feature --------------------------------------------------

    static LV2_Worker_Status carla_lv2_worker_schedule(const LV2_Worker_Schedule_Handle handle, const uint32_t size, const void* const data)
    {
        qDebug("Lv2Plugin::carla_lv2_worker_schedule(%p, %i, %p)", handle, size, data);
        CARLA_ASSERT(handle);

        if (! handle)
            return LV2_WORKER_ERR_UNKNOWN;

        Lv2Plugin* const plugin = (Lv2Plugin*)handle;
        return plugin->handleWorkerSchedule(size, data);
    }

    static LV2_Worker_Status carla_lv2_worker_respond(const LV2_Worker_Respond_Handle handle, const uint32_t size, const void* const data)
    {
        qDebug("Lv2Plugin::carla_lv2_worker_respond(%p, %i, %p)", handle, size, data);
        CARLA_ASSERT(handle);

        if (! handle)
            return LV2_WORKER_ERR_UNKNOWN;

        Lv2Plugin* const plugin = (Lv2Plugin*)handle;
        return plugin->handleWorkerRespond(size, data);
    }

    // ----------------- UI Port-Map Feature ---------------------------------------------

    static uint32_t carla_lv2_ui_port_map(const LV2UI_Feature_Handle handle, const char* const symbol)
    {
        qDebug("Lv2Plugin::carla_lv2_ui_port_map(%p, \"%s\")", handle, symbol);
        CARLA_ASSERT(handle);

        if (! handle)
            return LV2UI_INVALID_PORT_INDEX;

        Lv2Plugin* const plugin = (Lv2Plugin*)handle;
        return plugin->handleUiPortMap(symbol);
    }

    // ----------------- UI Resize Feature -----------------------------------------------

    static int carla_lv2_ui_resize(const LV2UI_Feature_Handle handle, const int width, const int height)
    {
        qDebug("Lv2Plugin::carla_lv2_ui_resize(%p, %i, %i)", handle, width, height);
        CARLA_ASSERT(handle);

        if (! handle)
            return 1;

        Lv2Plugin* const plugin = (Lv2Plugin*)handle;
        return plugin->handleUiResize(width, height);
    }

    // ----------------- External UI Feature ---------------------------------------------

    static void carla_lv2_external_ui_closed(const LV2UI_Controller controller)
    {
        qDebug("Lv2Plugin::carla_lv2_external_ui_closed(%p)", controller);
        CARLA_ASSERT(controller);

        if (! controller)
            return;

        Lv2Plugin* const plugin = (Lv2Plugin*)controller;
        plugin->handleExternalUiClosed();
    }

    // ----------------- UI Extension ----------------------------------------------------

    static void carla_lv2_ui_write_function(const LV2UI_Controller controller, const uint32_t port_index, const uint32_t buffer_size, const uint32_t format, const void* const buffer)
    {
        qDebug("Lv2Plugin::carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);
        CARLA_ASSERT(controller);

        if (! controller)
            return;

        Lv2Plugin* const plugin = (Lv2Plugin*)controller;
        plugin->handleUiWrite(port_index, buffer_size, format, buffer);
    }

    // -------------------------------------------------------------------

    bool uiLibOpen(const char* const filename)
    {
        ui.lib = lib_open(filename);
        return bool(ui.lib);
    }

    bool uiLibClose()
    {
        if (ui.lib)
            return lib_close(ui.lib);
        return false;
    }

    void* uiLibSymbol(const char* const symbol)
    {
        if (ui.lib)
            return lib_symbol(ui.lib, symbol);
        return nullptr;
    }

    // -------------------------------------------------------------------

    bool init(const char* const bundle, const char* const name, const char* const URI)
    {
        // ---------------------------------------------------------------
        // get plugin from lv2_rdf (lilv)

        rdf_descriptor = lv2_rdf_new(URI);

        if (! rdf_descriptor)
        {
            x_engine->setLastError("Failed to find the requested plugin in the LV2 Bundle");
            return false;
        }

        // ---------------------------------------------------------------
        // open DLL

        if (! libOpen(rdf_descriptor->Binary))
        {
            x_engine->setLastError(libError(rdf_descriptor->Binary));
            return false;
        }

        // ---------------------------------------------------------------
        // initialize features (part 1)

        LV2_Programs_Host* const programsFt = new LV2_Programs_Host;
        programsFt->handle                  = this;
        programsFt->program_changed         = carla_lv2_program_changed;

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

        features[lv2_feature_id_bufsize_bounded]       = new LV2_Feature;
        features[lv2_feature_id_bufsize_bounded]->URI  = LV2_BUF_SIZE__boundedBlockLength;
        features[lv2_feature_id_bufsize_bounded]->data = nullptr;

        if (x_engine->getOptions().processHighPrecision)
        {
            features[lv2_feature_id_bufsize_fixed]          = new LV2_Feature;
            features[lv2_feature_id_bufsize_fixed]->URI     = LV2_BUF_SIZE__fixedBlockLength;
            features[lv2_feature_id_bufsize_fixed]->data    = nullptr;

            features[lv2_feature_id_bufsize_powerof2]       = new LV2_Feature;
            features[lv2_feature_id_bufsize_powerof2]->URI  = LV2_BUF_SIZE__powerOf2BlockLength;
            features[lv2_feature_id_bufsize_powerof2]->data = nullptr;
        }
        else
        {
            // fake, used only to keep a valid features-array
            features[lv2_feature_id_bufsize_fixed]    = features[lv2_feature_id_bufsize_bounded];
            features[lv2_feature_id_bufsize_powerof2] = features[lv2_feature_id_bufsize_bounded];
        }

        features[lv2_feature_id_event]          = new LV2_Feature;
        features[lv2_feature_id_event]->URI     = LV2_EVENT_URI;
        features[lv2_feature_id_event]->data    = ft.event;

        features[lv2_feature_id_logs]           = new LV2_Feature;
        features[lv2_feature_id_logs]->URI      = LV2_LOG__log;
        features[lv2_feature_id_logs]->data     = ft.log;

        features[lv2_feature_id_options]        = new LV2_Feature;
        features[lv2_feature_id_options]->URI   = LV2_OPTIONS__options;
        features[lv2_feature_id_options]->data  = ft.options;

        features[lv2_feature_id_programs]       = new LV2_Feature;
        features[lv2_feature_id_programs]->URI  = LV2_PROGRAMS__Host;
        features[lv2_feature_id_programs]->data = programsFt;

        features[lv2_feature_id_rtmempool]       = new LV2_Feature;
        features[lv2_feature_id_rtmempool]->URI  = LV2_RTSAFE_MEMORY_POOL__Pool;
        features[lv2_feature_id_rtmempool]->data = ft.rtMemPool;

        features[lv2_feature_id_state_make_path]       = new LV2_Feature;
        features[lv2_feature_id_state_make_path]->URI  = LV2_STATE__makePath;
        features[lv2_feature_id_state_make_path]->data = ft.stateMakePath;

        features[lv2_feature_id_state_map_path]        = new LV2_Feature;
        features[lv2_feature_id_state_map_path]->URI   = LV2_STATE__mapPath;
        features[lv2_feature_id_state_map_path]->data  = ft.stateMapPath;

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

        features[lv2_feature_id_worker]           = new LV2_Feature;
        features[lv2_feature_id_worker]->URI      = LV2_WORKER__schedule;
        features[lv2_feature_id_worker]->data     = workerFt;

        // ---------------------------------------------------------------
        // get DLL main entry

        const LV2_Lib_Descriptor_Function descLibFn = (LV2_Lib_Descriptor_Function)libSymbol("lv2_lib_descriptor");

        if (descLibFn)
        {
            // -----------------------------------------------------------
            // get lib descriptor

            const LV2_Lib_Descriptor* libDesc = descLibFn(rdf_descriptor->Bundle, features);

            if (! libDesc)
            {
                x_engine->setLastError("Plugin failed to return library descriptor");
                return false;
            }

            // -----------------------------------------------------------
            // get descriptor that matches URI

            uint32_t i = 0;
            while ((descriptor = libDesc->get_plugin(libDesc->handle, i++)))
            {
                if (strcmp(descriptor->URI, URI) == 0)
                    break;
            }

            if (libDescs.count(libDesc) > 0)
            {
                if (! descriptor)
                    libDesc->cleanup(libDesc->handle);
            }
            else
                libDescs.insert(libDesc);
        }
        else
        {
            const LV2_Descriptor_Function descFn = (LV2_Descriptor_Function)libSymbol("lv2_descriptor");

            // -----------------------------------------------------------
            // if no descriptor function found, return error

            if (! descFn)
            {
                x_engine->setLastError("Could not find the LV2 Descriptor in the plugin library");
                return false;
            }

            // -----------------------------------------------------------
            // get descriptor that matches URI

            uint32_t i = 0;
            while ((descriptor = descFn(i++)))
            {
                if (strcmp(descriptor->URI, URI) == 0)
                    break;
            }
        }

        if (! descriptor)
        {
            x_engine->setLastError("Could not find the requested plugin URI in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // check supported port-types and features

        bool canContinue = true;

        // Check supported ports
        for (uint32_t i=0; i < rdf_descriptor->PortCount; i++)
        {
            LV2_Property portTypes = rdf_descriptor->Ports[i].Types;
            if (! bool(LV2_IS_PORT_AUDIO(portTypes) || LV2_IS_PORT_CONTROL(portTypes) || LV2_IS_PORT_ATOM_SEQUENCE(portTypes) || LV2_IS_PORT_EVENT(portTypes) || LV2_IS_PORT_MIDI_LL(portTypes)))
            {
                if (! LV2_IS_PORT_OPTIONAL(rdf_descriptor->Ports[i].Properties))
                {
                    x_engine->setLastError("Plugin requires a port that is not currently supported");
                    canContinue = false;
                    break;
                }
            }
        }

        // Check supported features
        for (uint32_t i=0; i < rdf_descriptor->FeatureCount && canContinue; i++)
        {
            if (LV2_IS_FEATURE_REQUIRED(rdf_descriptor->Features[i].Type) && ! is_lv2_feature_supported(rdf_descriptor->Features[i].URI))
            {
                QString msg = QString("Plugin requires a feature that is not supported:\n%1").arg(rdf_descriptor->Features[i].URI);
                x_engine->setLastError(msg.toUtf8().constData());
                canContinue = false;
                break;
            }
        }

        // Check extensions
        for (uint32_t i=0; i < rdf_descriptor->ExtensionCount; i++)
        {
            if (strcmp(rdf_descriptor->Extensions[i], LV2_PROGRAMS__Interface) == 0)
                m_hints |= PLUGIN_HAS_EXTENSION_PROGRAMS;
            else if (strcmp(rdf_descriptor->Extensions[i], LV2_STATE__interface) == 0)
                m_hints |= PLUGIN_HAS_EXTENSION_STATE;
            else if (strcmp(rdf_descriptor->Extensions[i], LV2_WORKER__interface) == 0)
                m_hints |= PLUGIN_HAS_EXTENSION_WORKER;
            else
                qDebug("Plugin has non-supported extension: '%s'", rdf_descriptor->Extensions[i]);
        }

        if (! canContinue)
        {
            // error already set
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        m_filename = strdup(bundle);

        if (name)
            m_name = x_engine->getUniquePluginName(name);
        else
            m_name = x_engine->getUniquePluginName(rdf_descriptor->Name);

        // ---------------------------------------------------------------
        // register client

        x_client = x_engine->addClient(this);

        if (! x_client->isOk())
        {
            x_engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin

        handle = descriptor->instantiate(descriptor, x_engine->getSampleRate(), rdf_descriptor->Bundle, features);

        if (! handle)
        {
            x_engine->setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // gui stuff

        if (rdf_descriptor->UICount == 0)
            return true;

        // -----------------------------------------------------------
        // find more appropriate ui

        int eQt4, eCocoa, eHWND, eX11, eGtk2, eGtk3, iCocoa, iHWND, iX11, iQt4, iExt, iSuil, iFinal;
        eQt4 = eCocoa = eHWND = eX11 = eGtk2 = eGtk3 = iQt4 = iCocoa = iHWND = iX11 = iExt = iSuil = iFinal = -1;

#ifdef BUILD_BRIDGE
        const bool preferUiBridges = x_engine->getOptions().preferUiBridges;
#else
        const bool preferUiBridges = (x_engine->getOptions().preferUiBridges && (m_hints & PLUGIN_IS_BRIDGE) == 0);
#endif

        for (uint32_t i=0; i < rdf_descriptor->UICount; i++)
        {
            CARLA_ASSERT(rdf_descriptor->UIs[i].URI);

            if (! rdf_descriptor->UIs[i].URI)
            {
                qWarning("Plugin has an UI without a valid URI");
                continue;
            }

            switch (rdf_descriptor->UIs[i].Type.Value)
            {
            case LV2_UI_QT4:
                if (isUiBridgeable(i) && preferUiBridges)
                    eQt4 = i;
                iQt4 = i;
                break;

            case LV2_UI_COCOA:
                if (isUiBridgeable(i) && preferUiBridges)
                    eCocoa = i;
                iCocoa = i;
                break;

            case LV2_UI_WINDOWS:
                if (isUiBridgeable(i) && preferUiBridges)
                    eHWND = i;
                iHWND = i;
                break;

            case LV2_UI_X11:
                if (isUiBridgeable(i) && preferUiBridges)
                    eX11 = i;
                iX11 = i;
                break;

            case LV2_UI_GTK2:
#ifdef WANT_SUIL // FIXME
                if (isUiBridgeable(i) && preferUiBridges)
                    eGtk2 = i;
#else
                if (isUiBridgeable(i))
                    eGtk2 = i;
#endif
#ifdef WANT_SUIL
                iSuil = i;
#endif
                break;

            case LV2_UI_GTK3:
                if (isUiBridgeable(i))
                    eGtk3 = i;
                break;

            case LV2_UI_EXTERNAL:
            case LV2_UI_OLD_EXTERNAL:
                // Calf Analyzer is useless using external-ui
                if (strcmp(rdf_descriptor->URI, "http://calf.sourceforge.net/plugins/Analyzer") != 0)
                    iExt = i;
                break;

            default:
                break;
            }
        }

        if (eQt4 >= 0)
            iFinal = eQt4;
        else if (eCocoa >= 0)
            iFinal = eCocoa;
        else if (eHWND >= 0)
            iFinal = eHWND;
        else if (eX11 >= 0)
            iFinal = eX11;
        else if (eGtk2 >= 0)
            iFinal = eGtk2;
        else if (eGtk3 >= 0)
            iFinal = eGtk3;
        else if (iQt4 >= 0)
            iFinal = iQt4;
        else if (iCocoa >= 0)
            iFinal = iCocoa;
        else if (iHWND >= 0)
            iFinal = iHWND;
        else if (iX11 >= 0)
            iFinal = iX11;
        else if (iExt >= 0)
            iFinal = iExt;
        else if (iSuil >= 0)
            iFinal = iSuil;

#ifndef WANT_SUIL
        const bool isBridged = (iFinal == eQt4 || iFinal == eCocoa || iFinal == eHWND || iFinal == eX11 || iFinal == eGtk2 || iFinal == eGtk3);
#else
        const bool isBridged = false;
        const bool isSuil = (iFinal == iSuil && !isBridged);
#endif

        if (iFinal < 0)
        {
            qWarning("Failed to find an appropriate LV2 UI for this plugin");
            return true;
        }

        ui.rdf_descriptor = &rdf_descriptor->UIs[iFinal];

        // -----------------------------------------------------------
        // check supported ui features

        canContinue = true;

        for (uint32_t i=0; i < ui.rdf_descriptor->FeatureCount; i++)
        {
            if (LV2_IS_FEATURE_REQUIRED(ui.rdf_descriptor->Features[i].Type) && is_lv2_ui_feature_supported(ui.rdf_descriptor->Features[i].URI) == false)
            {
                qCritical("Plugin UI requires a feature that is not supported:\n%s", ui.rdf_descriptor->Features[i].URI);
                canContinue = false;
                break;
            }
        }

        if (! canContinue)
        {
            ui.rdf_descriptor = nullptr;
            return true;
        }

#ifdef WANT_SUIL
        if (isSuil)
        {
            // -------------------------------------------------------
            // init suil host

            suil.host = suil_host_new(carla_lv2_ui_write_function, carla_lv2_ui_port_map, nullptr, nullptr);
        }
        else
#endif
        {
            // -------------------------------------------------------
            // open DLL

            if (! uiLibOpen(ui.rdf_descriptor->Binary))
            {
                qCritical("Could not load UI library, error was:\n%s", libError(ui.rdf_descriptor->Binary));
                ui.rdf_descriptor = nullptr;
                return true;
            }

            // -------------------------------------------------------
            // get DLL main entry

            LV2UI_DescriptorFunction ui_descFn = (LV2UI_DescriptorFunction)uiLibSymbol("lv2ui_descriptor");

            if (! ui_descFn)
            {
                qCritical("Could not find the LV2UI Descriptor in the UI library");
                uiLibClose();
                ui.lib = nullptr;
                ui.rdf_descriptor = nullptr;
                return true;
            }

            // -------------------------------------------------------
            // get descriptor that matches URI

            uint32_t i = 0;
            while ((ui.descriptor = ui_descFn(i++)))
            {
                if (strcmp(ui.descriptor->URI, ui.rdf_descriptor->URI) == 0)
                    break;
            }

            if (! ui.descriptor)
            {
                qCritical("Could not find the requested GUI in the plugin UI library");
                uiLibClose();
                ui.lib = nullptr;
                ui.rdf_descriptor = nullptr;
                return true;
            }
        }

        // -----------------------------------------------------------
        // initialize ui according to type

        const LV2_Property uiType = ui.rdf_descriptor->Type.Value;

        if (isBridged)
        {
            // -------------------------------------------------------
            // initialize ui bridge

            if (const char* const oscBinary = getUiBridgePath(uiType))
            {
                gui.type = GUI_EXTERNAL_OSC;
                osc.thread = new CarlaPluginThread(x_engine, this, CarlaPluginThread::PLUGIN_THREAD_LV2_GUI);
                osc.thread->setOscData(oscBinary, descriptor->URI, ui.descriptor->URI);
            }
        }
        else
        {
            // -------------------------------------------------------
            // initialize ui features

            QString guiTitle = QString("%1 (GUI)").arg(m_name);

            LV2_Extension_Data_Feature* const uiDataFt = new LV2_Extension_Data_Feature;
            uiDataFt->data_access                      = descriptor->extension_data;

            LV2UI_Port_Map* const uiPortMapFt = new LV2UI_Port_Map;
            uiPortMapFt->handle               = this;
            uiPortMapFt->port_index           = carla_lv2_ui_port_map;

            LV2UI_Resize* const uiResizeFt    = new LV2UI_Resize;
            uiResizeFt->handle                = this;
            uiResizeFt->ui_resize             = carla_lv2_ui_resize;

            LV2_External_UI_Host* const uiExternalHostFt = new LV2_External_UI_Host;
            uiExternalHostFt->ui_closed                  = carla_lv2_external_ui_closed;
            uiExternalHostFt->plugin_human_id            = strdup(guiTitle.toUtf8().constData());

            features[lv2_feature_id_data_access]           = new LV2_Feature;
            features[lv2_feature_id_data_access]->URI      = LV2_DATA_ACCESS_URI;
            features[lv2_feature_id_data_access]->data     = uiDataFt;

            features[lv2_feature_id_instance_access]       = new LV2_Feature;
            features[lv2_feature_id_instance_access]->URI  = LV2_INSTANCE_ACCESS_URI;
            features[lv2_feature_id_instance_access]->data = handle;

            features[lv2_feature_id_ui_parent]             = new LV2_Feature;
            features[lv2_feature_id_ui_parent]->URI        = LV2_UI__parent;
            features[lv2_feature_id_ui_parent]->data       = nullptr;

            features[lv2_feature_id_ui_port_map]           = new LV2_Feature;
            features[lv2_feature_id_ui_port_map]->URI      = LV2_UI__portMap;
            features[lv2_feature_id_ui_port_map]->data     = uiPortMapFt;

            features[lv2_feature_id_ui_resize]             = new LV2_Feature;
            features[lv2_feature_id_ui_resize]->URI        = LV2_UI__resize;
            features[lv2_feature_id_ui_resize]->data       = uiResizeFt;

            features[lv2_feature_id_external_ui]           = new LV2_Feature;
            features[lv2_feature_id_external_ui]->URI      = LV2_EXTERNAL_UI__Host;
            features[lv2_feature_id_external_ui]->data     = uiExternalHostFt;

            features[lv2_feature_id_external_ui_old]       = new LV2_Feature;
            features[lv2_feature_id_external_ui_old]->URI  = LV2_EXTERNAL_UI_DEPRECATED_URI;
            features[lv2_feature_id_external_ui_old]->data = uiExternalHostFt;

            // -------------------------------------------------------
            // initialize ui

            switch (uiType)
            {
            case LV2_UI_QT4:
                qDebug("Will use LV2 Qt4 UI");
                gui.type      = GUI_INTERNAL_QT4;
                gui.resizable = isUiResizable();
                ui.handle     = ui.descriptor->instantiate(ui.descriptor, descriptor->URI, ui.rdf_descriptor->Bundle, carla_lv2_ui_write_function, this, &ui.widget, features);
                m_hints      |= PLUGIN_USES_SINGLE_THREAD;
                break;

            case LV2_UI_COCOA:
                qDebug("Will use LV2 Cocoa UI");
                gui.type      = GUI_INTERNAL_COCOA;
                gui.resizable = isUiResizable();
                break;

            case LV2_UI_WINDOWS:
                qDebug("Will use LV2 Windows UI");
                gui.type      = GUI_INTERNAL_HWND;
                gui.resizable = isUiResizable();
                break;

            case LV2_UI_X11:
                qDebug("Will use LV2 X11 UI");
                gui.type      = GUI_INTERNAL_X11;
                gui.resizable = isUiResizable();
                break;

            case LV2_UI_GTK2:
#ifdef WANT_SUIL
                qDebug("Will use LV2 Gtk2 UI (suil)");
                gui.type      = GUI_EXTERNAL_SUIL;
                gui.resizable = isUiResizable();
                suil.handle   = suil_instance_new(suil.host, this, LV2_UI__Qt4UI, rdf_descriptor->URI, ui.rdf_descriptor->URI, ui.rdf_descriptor->Type.URI, ui.rdf_descriptor->Bundle, ui.rdf_descriptor->Binary, features);
                m_hints      |= PLUGIN_USES_SINGLE_THREAD;

                if (suil.handle)
                {
                    ui.handle     = ((SuilInstanceImpl*)suil.handle)->handle;
                    ui.descriptor = ((SuilInstanceImpl*)suil.handle)->descriptor;
                    ui.widget     = suil_instance_get_widget(suil.handle);

                    if (ui.widget)
                    {
                        QWidget* const widget = (QWidget*)ui.widget;
                        widget->setWindowTitle(guiTitle);
                    }
                }
#else
                qDebug("Will use LV2 Gtk2 UI, NOT!");
#endif
                break;

            case LV2_UI_GTK3:
                qDebug("Will use LV2 Gtk3 UI, NOT!");
                break;

            case LV2_UI_EXTERNAL:
            case LV2_UI_OLD_EXTERNAL:
                qDebug("Will use LV2 External UI");
                gui.type = GUI_EXTERNAL_LV2;
                break;
            }
        }

        if (gui.type != GUI_NONE)
            m_hints |= PLUGIN_HAS_GUI;

        return true;
    }

private:
    LV2_Handle handle, h2;
    const LV2_Descriptor* descriptor;
    const LV2_RDF_Descriptor* rdf_descriptor;
    LV2_Feature* features[lv2_feature_count+1];

    struct {
        const LV2_State_Interface* state;
        const LV2_Worker_Interface* worker;
        const LV2_Programs_Interface* programs;
        const LV2_Programs_UI_Interface* uiprograms;
    } ext;

    struct {
        void* lib;
        LV2UI_Handle handle;
        LV2UI_Widget widget;
        const LV2UI_Descriptor* descriptor;
        const LV2_RDF_UI* rdf_descriptor;
    } ui;

    struct {
        GuiType type;
        bool resizable;
        int width;
        int height;
    } gui;

#ifdef WANT_SUIL
    struct {
        SuilHost* host;
        SuilInstance* handle;
        QByteArray pos;
    } suil;
#endif

    Lv2AtomQueue atomQueueIn;
    Lv2AtomQueue atomQueueOut;
    Lv2PluginEventData evIn;
    Lv2PluginEventData evOut;
    float* paramBuffers;
    std::vector<const char*> customURIDs;

    bool     lastTimePosPlaying;
    uint32_t lastTimePosFrame;

    static unsigned int m_count;
    static std::set<const LV2_Lib_Descriptor*> libDescs;

    static struct Ft {
        LV2_Event_Feature* event;
        LV2_Log_Log* log;
        LV2_Options_Option* options;
        LV2_RtMemPool_Pool* rtMemPool;
        LV2_State_Make_Path* stateMakePath;
        LV2_State_Map_Path* stateMapPath;
    } ft;
};

/**@}*/

// -------------------------------------------------------------------
// static data

unsigned int Lv2Plugin::m_count = 0;
std::set<const LV2_Lib_Descriptor*> Lv2Plugin::libDescs;

Lv2Plugin::Ft Lv2Plugin::ft = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

// -------------------------------------------------------------------------------------------------------------------

int CarlaEngineOsc::handleMsgLv2AtomTransfer(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgLv2AtomTransfer()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(3, "iss");

    const int32_t portIndex   = argv[0]->i;
    const char* const typeStr = (const char*)&argv[1]->s;
    const char* const atomBuf = (const char*)&argv[2]->s;

    QByteArray chunk;
    chunk = QByteArray::fromBase64(atomBuf);

    LV2_Atom* const atom = (LV2_Atom*)chunk.constData();
    CarlaBackend::Lv2Plugin* const lv2plugin = (CarlaBackend::Lv2Plugin*)plugin;

    atom->type = lv2plugin->getCustomURID(typeStr);
    lv2plugin->handleTransferAtom(portIndex, atom);

    return 0;
}

int CarlaEngineOsc::handleMsgLv2EventTransfer(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgLv2EventTransfer()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(3, "iss");

    const int32_t portIndex   = argv[0]->i;
    const char* const typeStr = (const char*)&argv[1]->s;
    const char* const atomBuf = (const char*)&argv[2]->s;

    QByteArray chunk;
    chunk = QByteArray::fromBase64(atomBuf);

    LV2_Atom* const atom = (LV2_Atom*)chunk.constData();
    CarlaBackend::Lv2Plugin* const lv2plugin = (CarlaBackend::Lv2Plugin*)plugin;

    atom->type = lv2plugin->getCustomURID(typeStr);
    lv2plugin->handleTransferEvent(portIndex, atom);

    return 0;
}

CARLA_BACKEND_END_NAMESPACE

#else // WANT_LV2
# warning Building without LV2 support
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newLV2(const initializer& init)
{
    qDebug("CarlaPlugin::newLV2(%p, \"%s\", \"%s\", \"%s\")", init.engine, init.filename, init.name, init.label);

#ifdef WANT_LV2
    short id = init.engine->getNewPluginId();

    if (id < 0 || id > init.engine->maxPluginNumber())
    {
        init.engine->setLastError("Maximum number of plugins reached");
        return nullptr;
    }

    Lv2Plugin* const plugin = new Lv2Plugin(init.engine, id);

    if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getOptions().processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (! (plugin->hints() & PLUGIN_CAN_FORCE_STEREO))
        {
            init.engine->setLastError("Carla's rack mode can only work with Mono (simple) or Stereo LV2 plugins, sorry!");
            delete plugin;
            return nullptr;
        }
    }

    plugin->registerToOscClient();
    plugin->updateUi();

    return plugin;
#else
    init.engine->setLastError("LV2 support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
