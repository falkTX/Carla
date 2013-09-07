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

// -----------------------------------------------------

CARLA_BRIDGE_START_NAMESPACE

#if 0
}
#endif

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
const uint32_t kFeatureIdLogs             =  0;
const uint32_t kFeatureIdOptions          =  1;
const uint32_t kFeatureIdPrograms         =  2;
const uint32_t kFeatureIdStateMakePath    =  3;
const uint32_t kFeatureIdStateMapPath     =  4;
const uint32_t kFeatureIdUriMap           =  5;
const uint32_t kFeatureIdUridMap          =  6;
const uint32_t kFeatureIdUridUnmap        =  7;
const uint32_t kFeatureIdUiIdle           =  8;
const uint32_t kFeatureIdUiFixedSize      =  9;
const uint32_t kFeatureIdUiMakeResident   = 10;
const uint32_t kFeatureIdUiNoUserResize   = 11;
const uint32_t kFeatureIdUiParent         = 12;
const uint32_t kFeatureIdUiPortMap        = 13;
const uint32_t kFeatureIdUiPortSubscribe  = 14;
const uint32_t kFeatureIdUiResize         = 15;
const uint32_t kFeatureIdUiTouch          = 16;
const uint32_t kFeatureCount              = 17;

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

    CARLA_DECLARE_NON_COPY_STRUCT(Lv2PluginOptions)
};

// -------------------------------------------------------------------------

class CarlaLv2Client : public CarlaBridgeClient
{
public:
    CarlaLv2Client(const char* const uiTitle)
        : CarlaBridgeClient(uiTitle),
          fHandle(nullptr),
          fWidget(nullptr),
#ifdef CARLA_PROPER_CPP11_SUPPORT
          fFeatures{nullptr},
#endif
          fDescriptor(nullptr),
          fRdfDescriptor(nullptr),
          fRdfUiDescriptor(nullptr),
#if defined(BRIDGE_COCOA) || defined(BRIDGE_HWND) || defined(BRIDGE_X11)
          fIsResizable(false)
#else
          fIsResizable(true)
#endif
    {
#ifndef CARLA_PROPER_CPP11_SUPPORT
        carla_fill<LV2_Feature*>(fFeatures, kFeatureCount+1, nullptr);
#endif

        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; ++i)
            fCustomURIDs.append(nullptr);

        // ---------------------------------------------------------------
        // initialize options

        fOptions.minBufferSize = gBufferSize;
        fOptions.maxBufferSize = gBufferSize;
        fOptions.sampleRate    = gSampleRate;

        // ---------------------------------------------------------------
        // initialize features (part 1)

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

        LV2_URI_Map_Feature* const uriMapFt = new LV2_URI_Map_Feature;
        uriMapFt->callback_data             = this;
        uriMapFt->uri_to_id                 = carla_lv2_uri_to_id;

        LV2_URID_Map* const uridMapFt       = new LV2_URID_Map;
        uridMapFt->handle                   = this;
        uridMapFt->map                      = carla_lv2_urid_map;

        LV2_URID_Unmap* const uridUnmapFt   = new LV2_URID_Unmap;
        uridUnmapFt->handle                 = this;
        uridUnmapFt->unmap                  = carla_lv2_urid_unmap;

        LV2UI_Port_Map* const uiPortMapFt = new LV2UI_Port_Map;
        uiPortMapFt->handle               = this;
        uiPortMapFt->port_index           = carla_lv2_ui_port_map;

        LV2UI_Resize* const uiResizeFt    = new LV2UI_Resize;
        uiResizeFt->handle                = this;
        uiResizeFt->ui_resize             = carla_lv2_ui_resize;

        // ---------------------------------------------------------------
        // initialize features (part 2)

        for (uint32_t i=0; i < kFeatureCount; ++i)
            fFeatures[i] = new LV2_Feature;

        fFeatures[kFeatureIdLogs]->URI       = LV2_LOG__log;
        fFeatures[kFeatureIdLogs]->data      = logFt;

        fFeatures[kFeatureIdOptions]->URI    = LV2_OPTIONS__options;
        fFeatures[kFeatureIdOptions]->data   = fOptions.opts;

        fFeatures[kFeatureIdPrograms]->URI   = LV2_PROGRAMS__Host;
        fFeatures[kFeatureIdPrograms]->data  = programsFt;

        fFeatures[kFeatureIdStateMakePath]->URI  = LV2_STATE__makePath;
        fFeatures[kFeatureIdStateMakePath]->data = stateMakePathFt;

        fFeatures[kFeatureIdStateMapPath]->URI   = LV2_STATE__mapPath;
        fFeatures[kFeatureIdStateMapPath]->data  = stateMapPathFt;

        fFeatures[kFeatureIdUriMap]->URI     = LV2_URI_MAP_URI;
        fFeatures[kFeatureIdUriMap]->data    = uriMapFt;

        fFeatures[kFeatureIdUridMap]->URI    = LV2_URID__map;
        fFeatures[kFeatureIdUridMap]->data   = uridMapFt;

        fFeatures[kFeatureIdUridUnmap]->URI  = LV2_URID__unmap;
        fFeatures[kFeatureIdUridUnmap]->data = uridUnmapFt;

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
    }

    ~CarlaLv2Client() override
    {
        if (fRdfDescriptor != nullptr)
            delete fRdfDescriptor;

        delete (LV2_Log_Log*)fFeatures[kFeatureIdLogs]->data;
        delete (LV2_State_Make_Path*)fFeatures[kFeatureIdStateMakePath]->data;
        delete (LV2_State_Map_Path*)fFeatures[kFeatureIdStateMapPath]->data;
        delete (LV2_Programs_Host*)fFeatures[kFeatureIdPrograms]->data;
        delete (LV2_URI_Map_Feature*)fFeatures[kFeatureIdUriMap]->data;
        delete (LV2_URID_Map*)fFeatures[kFeatureIdUridMap]->data;
        delete (LV2_URID_Unmap*)fFeatures[kFeatureIdUridUnmap]->data;
        delete (LV2UI_Port_Map*)fFeatures[kFeatureIdUiPortMap]->data;
        delete (LV2UI_Resize*)fFeatures[kFeatureIdUiResize]->data;

        for (uint32_t i=0; i < kFeatureCount; ++i)
        {
            if (fFeatures[i] != nullptr)
            {
                delete fFeatures[i];
                fFeatures[i] = nullptr;
            }
        }

        for (NonRtList<const char*>::Itenerator it = fCustomURIDs.begin(); it.valid(); it.next())
        {
            const char*& uri(*it);

            if (uri != nullptr)
            {
                delete[] uri;
                uri = nullptr;
            }
        }

        fCustomURIDs.clear();
    }

    // ---------------------------------------------------------------------
    // ui initialization

    bool uiInit(const char* pluginURI, const char* uiURI) override
    {
        // -----------------------------------------------------------------
        // init

        CarlaBridgeClient::uiInit(pluginURI, uiURI);

        // -----------------------------------------------------------------
        // get plugin from lv2_rdf (lilv)

        gLv2World.init();
        fRdfDescriptor = lv2_rdf_new(pluginURI, false);

        if (fRdfDescriptor == nullptr)
            return false;

        // -----------------------------------------------------------------
        // find requested UI

        for (uint32_t i=0; i < fRdfDescriptor->UICount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->UIs[i].URI, uiURI) == 0)
            {
                fRdfUiDescriptor = &fRdfDescriptor->UIs[i];
                break;
            }
        }

        if (fRdfUiDescriptor == nullptr)
            return false;

        // -----------------------------------------------------------------
        // open DLL

        if (! uiLibOpen(fRdfUiDescriptor->Binary))
            return false;

        // -----------------------------------------------------------------
        // get DLL main entry

        const LV2UI_DescriptorFunction ui_descFn = (LV2UI_DescriptorFunction)uiLibSymbol("lv2ui_descriptor");

        if (ui_descFn == nullptr)
            return false;

        // -----------------------------------------------------------
        // get descriptor that matches URI

        uint32_t i = 0;
        while ((fDescriptor = ui_descFn(i++)))
        {
            if (std::strcmp(fDescriptor->URI, uiURI) == 0)
                break;
        }

        if (fDescriptor == nullptr)
            return false;

        // -----------------------------------------------------------
        // initialize UI

#if defined(BRIDGE_COCOA) || defined(BRIDGE_HWND) || defined(BRIDGE_X11)
        fFeatures[kFeatureIdUiParent]->data = getContainerId();
#endif

        fHandle = fDescriptor->instantiate(fDescriptor, pluginURI, fRdfUiDescriptor->Bundle, carla_lv2_ui_write_function, this, &fWidget, fFeatures);

        if (fHandle == nullptr)
            return false;

        // -----------------------------------------------------------
        // check if not resizable

        for (uint32_t i=0; i < fRdfUiDescriptor->FeatureCount && fIsResizable; ++i)
        {
            if (std::strcmp(fRdfUiDescriptor->Features[i].URI, LV2_UI__fixedSize) == 0 || std::strcmp(fRdfUiDescriptor->Features[i].URI, LV2_UI__noUserResize) == 0)
            {
                fIsResizable = false;
                break;
            }
        }

        // -----------------------------------------------------------
        // check for known extensions

        if (fDescriptor->extension_data != nullptr)
        {
            fExt.programs = (const LV2_Programs_UI_Interface*)fDescriptor->extension_data(LV2_PROGRAMS__UIInterface);
            fExt.options  = (const LV2_Options_Interface*)fDescriptor->extension_data(LV2_OPTIONS__interface);
            fExt.idle     = (const LV2UI_Idle_Interface*)fDescriptor->extension_data(LV2_UI__idleInterface);

            // check if invalid
            if (fExt.programs != nullptr && fExt.programs->select_program == nullptr)
                fExt.programs = nullptr;
            if (fExt.idle != nullptr && fExt.idle->idle == nullptr)
                fExt.idle = nullptr;
        }

        return true;
    }

    void uiIdle() override
    {
        if (fHandle != nullptr && fExt.idle != nullptr)
            fExt.idle->idle(fHandle);
    }

    void uiClose() override
    {
        CarlaBridgeClient::uiClose();

        if (fHandle && fDescriptor && fDescriptor->cleanup)
            fDescriptor->cleanup(fHandle);

        uiLibClose();
    }

    // ---------------------------------------------------------------------
    // ui management

    void* getWidget() const override
    {
        return fWidget;
    }

    bool isResizable() const override
    {
        return fIsResizable;
    }

    bool needsReparent() const override
    {
#if defined(BRIDGE_COCOA) || defined(BRIDGE_HWND) || defined(BRIDGE_X11)
        return true;
#else
        return false;
#endif
    }

    // ---------------------------------------------------------------------
    // ui processing

    void setParameter(const int32_t rindex, const float value) override
    {
        CARLA_ASSERT(fHandle != nullptr && fDescriptor != nullptr);

        if (fHandle != nullptr && fDescriptor != nullptr && fDescriptor->port_event != nullptr)
            fDescriptor->port_event(fHandle, rindex, sizeof(float), 0, &value);
    }

    void setProgram(const uint32_t) override
    {
    }

    void setMidiProgram(const uint32_t bank, const uint32_t program) override
    {
        CARLA_ASSERT(fHandle != nullptr);

        if (fHandle != nullptr && fExt.programs != nullptr)
            fExt.programs->select_program(fHandle, bank, program);
    }

    void noteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) override
    {
        CARLA_ASSERT(fHandle != nullptr && fDescriptor != nullptr);

        if (fHandle != nullptr && fDescriptor != nullptr && fDescriptor->port_event != nullptr)
        {
            LV2_Atom_MidiEvent midiEv;
            midiEv.event.time.frames = 0;
            midiEv.event.body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
            midiEv.event.body.size   = 3;
            midiEv.data[0] = MIDI_STATUS_NOTE_ON + channel;
            midiEv.data[1] = note;
            midiEv.data[2] = velo;

            fDescriptor->port_event(fHandle, 0, 3, CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, &midiEv);
        }
    }

    void noteOff(const uint8_t channel, const uint8_t note) override
    {
        CARLA_ASSERT(fHandle != nullptr && fDescriptor != nullptr);

        if (fHandle != nullptr && fDescriptor != nullptr && fDescriptor->port_event != nullptr)
        {
            LV2_Atom_MidiEvent midiEv;
            midiEv.event.time.frames = 0;
            midiEv.event.body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
            midiEv.event.body.size   = 3;
            midiEv.data[0] = MIDI_STATUS_NOTE_OFF + channel;
            midiEv.data[1] = note;
            midiEv.data[2] = 0;

            fDescriptor->port_event(fHandle, 0, 3, CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, &midiEv);
        }
    }

    // ---------------------------------------------------------------------

    LV2_URID getCustomURID(const char* const uri)
    {
        CARLA_ASSERT(uri != nullptr);
        carla_debug("CarlaLv2Client::getCustomURID(\"%s\")", uri);

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

        if (isOscControlRegistered())
            sendOscLv2UridMap(urid, uri);

        return urid;
    }

    const char* getCustomURIString(const LV2_URID urid)
    {
        CARLA_ASSERT(urid != CARLA_URI_MAP_ID_NULL);
        CARLA_ASSERT_INT2(urid < fCustomURIDs.count(), urid, fCustomURIDs.count());
        carla_debug("CarlaLv2Client::getCustomURIString(%i)", urid);

        if (urid == CARLA_URI_MAP_ID_NULL)
            return nullptr;
        if (urid < fCustomURIDs.count())
            return fCustomURIDs.getAt(urid);

        return nullptr;
    }

    // ---------------------------------------------------------------------

    void handleProgramChanged(const int32_t /*index*/)
    {
        if (isOscControlRegistered())
            sendOscConfigure("reloadprograms", "");
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

        toolkitResize(width, height);

        return 0;
    }

    void handleUiWrite(uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
    {
        if (format == 0)
        {
            CARLA_ASSERT(buffer != nullptr);
            CARLA_ASSERT(bufferSize == sizeof(float));

            if (bufferSize != sizeof(float))
                return;

            if (buffer == nullptr || bufferSize != sizeof(float))
                return;

            const float value(*(const float*)buffer);

            if (isOscControlRegistered())
                sendOscControl(portIndex, value);
        }
        else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM || CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT)
        {
            CARLA_ASSERT(bufferSize != 0);
            CARLA_ASSERT(buffer != nullptr);

            if (bufferSize == 0 || buffer == nullptr)
                return;

            if (isOscControlRegistered())
                sendOscLv2AtomTransfer(portIndex, QByteArray((const char*)buffer, bufferSize).toBase64().constData());
        }
        else
        {
            carla_stdout("CarlaLv2Client::handleUiWrite(%i, %i, %i:\"%s\", %p) - unknown format", portIndex, bufferSize, format, carla_lv2_urid_unmap(this, format), buffer);
        }
    }

    // ---------------------------------------------------------------------

    void handleAtomTransfer(const uint32_t portIndex, const LV2_Atom* const atom)
    {
        CARLA_ASSERT(atom != nullptr);
        carla_debug("CarlaLv2Client::handleTransferEvent(%i, %p)", portIndex, atom);

        if (atom != nullptr && fHandle != nullptr && fDescriptor != nullptr && fDescriptor->port_event != nullptr)
            fDescriptor->port_event(fHandle, portIndex, lv2_atom_total_size(atom), CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);
    }

    void handleUridMap(const LV2_URID urid, const char* const uri)
    {
        CARLA_ASSERT(urid != CARLA_URI_MAP_ID_NULL);
        CARLA_ASSERT(uri != nullptr);
        carla_debug("CarlaLv2Client::handleUridMap(%i, \"%s\")", urid, uri);

        // TODO
    }

private:
    LV2UI_Handle fHandle;
    LV2UI_Widget fWidget;
    LV2_Feature* fFeatures[kFeatureCount+1];

    const LV2UI_Descriptor*   fDescriptor;
    const LV2_RDF_Descriptor* fRdfDescriptor;
    const LV2_RDF_UI*         fRdfUiDescriptor;
    Lv2PluginOptions          fOptions;

    bool fIsResizable;
    NonRtList<const char*> fCustomURIDs;

    struct Extensions {
        const LV2_Options_Interface* options;
        const LV2UI_Idle_Interface* idle;
        const LV2_Programs_UI_Interface* programs;

        Extensions()
            : options(nullptr),
              idle(nullptr),
              programs(nullptr) {}
    } fExt;

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
        carla_debug("CarlaLv2Client::carla_lv2_program_changed(%p, %i)", handle, index);
        CARLA_ASSERT(handle != nullptr);

        if (handle == nullptr)
            return;

        ((CarlaLv2Client*)handle)->handleProgramChanged(index);
    }

    // -------------------------------------------------------------------
    // State Feature

    static char* carla_lv2_state_make_path(LV2_State_Make_Path_Handle handle, const char* path)
    {
        carla_debug("CarlaLv2Client::carla_lv2_state_make_path(%p, \"%s\")", handle, path);
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
        carla_debug("CarlaLv2Client::carla_lv2_state_map_abstract_path(%p, \"%s\")", handle, absolute_path);
        CARLA_ASSERT(handle != nullptr);
        CARLA_ASSERT(absolute_path != nullptr);

        if (absolute_path == nullptr)
            return nullptr;

        QDir dir(absolute_path);
        return strdup(dir.canonicalPath().toUtf8().constData());
    }

    static char* carla_lv2_state_map_absolute_path(LV2_State_Map_Path_Handle handle, const char* abstract_path)
    {
        carla_debug("CarlaLv2Client::carla_lv2_state_map_absolute_path(%p, \"%s\")", handle, abstract_path);
        CARLA_ASSERT(handle != nullptr);
        CARLA_ASSERT(abstract_path != nullptr);

        if (abstract_path == nullptr)
            return nullptr;

        QDir dir(abstract_path);
        return strdup(dir.absolutePath().toUtf8().constData());
    }

    // -------------------------------------------------------------------
    // URI-Map Feature

    static uint32_t carla_lv2_uri_to_id(LV2_URI_Map_Callback_Data data, const char* map, const char* uri)
    {
        carla_debug("CarlaLv2Client::carla_lv2_uri_to_id(%p, \"%s\", \"%s\")", data, map, uri);
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
        carla_debug("CarlaLv2Client::carla_lv2_urid_map(%p, \"%s\")", handle, uri);

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
        return ((CarlaLv2Client*)handle)->getCustomURID(uri);
    }

    static const char* carla_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
    {
        carla_debug("CarlaLv2Client::carla_lv2_urid_unmap(%p, %i)", handle, urid);
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
        return ((CarlaLv2Client*)handle)->getCustomURIString(urid);
    }

    // -------------------------------------------------------------------
    // UI Port-Map Feature

    static uint32_t carla_lv2_ui_port_map(LV2UI_Feature_Handle handle, const char* symbol)
    {
        carla_debug("CarlaLv2Client::carla_lv2_ui_port_map(%p, \"%s\")", handle, symbol);
        CARLA_ASSERT(handle);

        if (handle == nullptr)
            return LV2UI_INVALID_PORT_INDEX;

        return ((CarlaLv2Client*)handle)->handleUiPortMap(symbol);
    }

    // -------------------------------------------------------------------
    // UI Resize Feature

    static int carla_lv2_ui_resize(LV2UI_Feature_Handle handle, int width, int height)
    {
        carla_debug("CarlaLv2Client::carla_lv2_ui_resize(%p, %i, %i)", handle, width, height);
        CARLA_ASSERT(handle != nullptr);

        if (handle == nullptr)
            return 1;

        return ((CarlaLv2Client*)handle)->handleUiResize(width, height);
    }

    // -------------------------------------------------------------------
    // UI Extension

    static void carla_lv2_ui_write_function(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
    {
        CARLA_ASSERT(controller != nullptr);
        carla_debug("CarlaLv2Client::carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);

        if (controller == nullptr)
            return;

        ((CarlaLv2Client*)controller)->handleUiWrite(port_index, buffer_size, format, buffer);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaLv2Client)
};

#define lv2ClientPtr ((CarlaLv2Client*)fClient)

int CarlaBridgeOsc::handleMsgLv2AtomTransfer(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "is");
    carla_debug("CarlaBridgeOsc::handleMsgLv2AtomTransfer()");

    if (fClient == nullptr)
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

    if (fClient == nullptr)
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

    const bool useOsc(std::strcmp(oscUrl, "null") != 0);

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
        carla_stderr("Failed to load LV2 UI");
        ret = 1;
    }

    // Close OSC
    if (useOsc)
        client.oscClose();

    // Close LV2 client
    client.uiClose();

    return ret;
}
