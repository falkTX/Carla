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
#include "LinkedList.hpp"

#include <QtCore/QDir>

extern "C" {
#include "rtmempool/rtmempool-lv2.h"
}

// -----------------------------------------------------

CARLA_BRIDGE_START_NAMESPACE

#if 0
}
#endif

static uint32_t gBufferSize = 1024;
static double   gSampleRate = 44100.0;

// Maximum default buffer size
const unsigned int MAX_DEFAULT_BUFFER_SIZE = 8192; // 0x2000

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
const uint32_t CARLA_URI_MAP_ID_ATOM_WORKER            = 22; // custom
const uint32_t CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM     = 23;
const uint32_t CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT    = 24;
const uint32_t CARLA_URI_MAP_ID_BUF_MAX_LENGTH         = 25;
const uint32_t CARLA_URI_MAP_ID_BUF_MIN_LENGTH         = 26;
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
const uint32_t CARLA_URI_MAP_ID_MIDI_EVENT             = 42;
const uint32_t CARLA_URI_MAP_ID_PARAM_SAMPLE_RATE      = 43;
const uint32_t CARLA_URI_MAP_ID_COUNT                  = 44;

// LV2 Feature Ids
const uint32_t kFeatureIdLogs             =  0;
const uint32_t kFeatureIdOptions          =  1;
const uint32_t kFeatureIdPrograms         =  2;
const uint32_t kFeatureIdStateMakePath    =  3;
const uint32_t kFeatureIdStateMapPath     =  4;
const uint32_t kFeatureIdUriMap           =  5;
const uint32_t kFeatureIdUridMap          =  6;
const uint32_t kFeatureIdUridUnmap        =  7;
const uint32_t kFeatureIdUiIdleInterface  =  8;
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
    enum OptIndex {
        MaxBlockLenth = 0,
        MinBlockLenth,
        SequenceSize,
        SampleRate,
        Null
    };

    int maxBufferSize;
    int minBufferSize;
    int sequenceSize;
    double sampleRate;
    LV2_Options_Option opts[5];

    Lv2PluginOptions()
        : maxBufferSize(0),
          minBufferSize(0),
          sequenceSize(MAX_DEFAULT_BUFFER_SIZE),
          sampleRate(0.0)
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

        LV2_Options_Option& optNull(opts[Null]);
        optNull.context = LV2_OPTIONS_INSTANCE;
        optNull.subject = 0;
        optNull.key     = CARLA_URI_MAP_ID_NULL;
        optNull.size    = 0;
        optNull.type    = CARLA_URI_MAP_ID_NULL;
        optNull.value   = nullptr;
    }
};

// -------------------------------------------------------------------------

class CarlaLv2Client : public CarlaBridgeClient
{
public:
    CarlaLv2Client(const char* const uiTitle)
        : CarlaBridgeClient(uiTitle),
          fHandle(nullptr),
          fWidget(nullptr),
          fDescriptor(nullptr),
          fRdfDescriptor(nullptr),
          fRdfUiDescriptor(nullptr),
#if defined(BRIDGE_COCOA) || defined(BRIDGE_HWND) || defined(BRIDGE_X11)
          fIsResizable(false)
#else
          fIsResizable(true)
#endif
    {
        carla_fill<LV2_Feature*>(fFeatures, kFeatureCount+1, nullptr);

        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; ++i)
            fCustomURIDs.append(nullptr);

        // FIXME
        fCustomURIDs.append(carla_strdup("http://drobilla.net/ns/ingen#GraphUIGtk2"));
        fCustomURIDs.append(carla_strdup("http://lv2plug.in/ns/ext/state#interface"));
        fCustomURIDs.append(carla_strdup("ingen:/root"));
        fCustomURIDs.append(carla_strdup("ingen:/root/"));

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

        fFeatures[kFeatureIdUiResize]->URI  = LV2_UI__resize;
        fFeatures[kFeatureIdUiResize]->data = uiResizeFt;

        fFeatures[kFeatureIdUiTouch]->URI   = LV2_UI__touch;
        fFeatures[kFeatureIdUiTouch]->data  = nullptr;
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

        for (LinkedList<const char*>::Itenerator it = fCustomURIDs.begin(); it.valid(); it.next())
        {
            const char* const uri(it.getValue());

            if (uri != nullptr)
                delete[] uri;
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

        fRdfDescriptor = lv2_rdf_new(pluginURI, true);

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
        {
            carla_stderr("Failed to find requested UI");
            return false;
        }

        // -----------------------------------------------------------------
        // open DLL

        if (! uiLibOpen(fRdfUiDescriptor->Binary))
        {
            carla_stderr("Failed to load UI binary, error was:\n%s", uiLibError());
            return false;
        }

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
        {
            carla_stderr("Failed to find UI descriptor");
            return false;
        }

        // -----------------------------------------------------------
        // initialize UI

#if defined(BRIDGE_COCOA) || defined(BRIDGE_HWND) || defined(BRIDGE_X11)
        fFeatures[kFeatureIdUiParent]->data = getContainerId();
#endif

        fHandle = fDescriptor->instantiate(fDescriptor, pluginURI, fRdfUiDescriptor->Bundle, carla_lv2_ui_write_function, this, &fWidget, fFeatures);

        if (fHandle == nullptr)
        {
            carla_stderr("Failed to init UI");
            return false;
        }

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
            const char* const thisUri(fCustomURIDs.getAt(i));
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

        if (urid < fCustomURIDs.count())
        {
            const char* const ourURI(carla_lv2_urid_unmap(this, urid));
            CARLA_SAFE_ASSERT_RETURN(ourURI != nullptr,);

            if (std::strcmp(ourURI, uri) != 0)
            {
                carla_stderr2("UI :: wrong URI '%s' vs '%s'", ourURI,  uri);
            }
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(urid == fCustomURIDs.count(),);
            fCustomURIDs.append(carla_strdup(uri));
        }
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
    LinkedList<const char*> fCustomURIDs;

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

        ((CarlaLv2Client*)handle)->handleProgramChanged(index);
    }

    // -------------------------------------------------------------------
    // State Feature

    static char* carla_lv2_state_make_path(LV2_State_Make_Path_Handle handle, const char* path)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(path != nullptr && path[0] != '\0', nullptr);
        carla_debug("carla_lv2_state_make_path(%p, \"%s\")", handle, path);

        QDir dir;
        dir.mkpath(path);
        return strdup(path);
    }

    static char* carla_lv2_state_map_abstract_path(LV2_State_Map_Path_Handle handle, const char* absolute_path)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(absolute_path != nullptr && absolute_path[0] != '\0', nullptr);
        carla_debug("carla_lv2_state_map_abstract_path(%p, \"%s\")", handle, absolute_path);

        QDir dir(absolute_path);
        return strdup(dir.canonicalPath().toUtf8().constData());
    }

    static char* carla_lv2_state_map_absolute_path(LV2_State_Map_Path_Handle handle, const char* abstract_path)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(abstract_path != nullptr && abstract_path[0] != '\0', nullptr);
        carla_debug("carla_lv2_state_map_absolute_path(%p, \"%s\")", handle, abstract_path);

        QDir dir(abstract_path);
        return strdup(dir.absolutePath().toUtf8().constData());
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

        // Custom types
        return ((CarlaLv2Client*)handle)->getCustomURID(uri);
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
        if (urid == CARLA_URI_MAP_ID_ATOM_WORKER)
            return nullptr; // custom
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

        // Custom types
        return ((CarlaLv2Client*)handle)->getCustomURIString(urid);
    }

    // -------------------------------------------------------------------
    // UI Port-Map Feature

    static uint32_t carla_lv2_ui_port_map(LV2UI_Feature_Handle handle, const char* symbol)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, LV2UI_INVALID_PORT_INDEX);
        carla_debug("carla_lv2_ui_port_map(%p, \"%s\")", handle, symbol);

        return ((CarlaLv2Client*)handle)->handleUiPortMap(symbol);
    }

    // -------------------------------------------------------------------
    // UI Resize Feature

    static int carla_lv2_ui_resize(LV2UI_Feature_Handle handle, int width, int height)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, 1);
        carla_debug("carla_lv2_ui_resize(%p, %i, %i)", handle, width, height);

        return ((CarlaLv2Client*)handle)->handleUiResize(width, height);
    }

    // -------------------------------------------------------------------
    // UI Extension

    static void carla_lv2_ui_write_function(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
    {
        CARLA_SAFE_ASSERT_RETURN(controller != nullptr,);
        carla_debug("carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);

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
        ret = 1;
    }

    // Close OSC
    if (useOsc)
        client.oscClose();

    // Close LV2 client
    client.uiClose();

    return ret;
}
