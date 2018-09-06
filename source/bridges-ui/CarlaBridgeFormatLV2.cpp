/*
 * Carla Bridge UI
 * Copyright (C) 2011-2017 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBridgeFormat.hpp"
#include "CarlaBridgeToolkit.hpp"

#include "CarlaLibUtils.hpp"
#include "CarlaLv2Utils.hpp"
#include "CarlaMIDI.h"
#include "LinkedList.hpp"

#include "water/files/File.h"

#include <string>
#include <vector>

#define URI_CARLA_ATOM_WORKER "http://kxstudio.sf.net/ns/carla/atomWorker"

using water::File;

CARLA_BRIDGE_UI_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

static double gInitialSampleRate = 44100.0;
static const char* const kNullWindowTitle = "TestUI";
static const uint32_t kNullWindowTitleSize = 6;


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
    kUridWindowTitle,
    kUridCarlaAtomWorker,
    kUridCarlaTransientWindowId,
    kUridCount
};

// LV2 Feature Ids
enum CarlaLv2Features {
    // DSP features
    kFeatureIdLogs = 0,
    kFeatureIdOptions,
    kFeatureIdPrograms,
    kFeatureIdStateMakePath,
    kFeatureIdStateMapPath,
    kFeatureIdUriMap,
    kFeatureIdUridMap,
    kFeatureIdUridUnmap,
    kFeatureIdUiIdleInterface,
    kFeatureIdUiFixedSize,
    kFeatureIdUiMakeResident,
    kFeatureIdUiMakeResident2,
    kFeatureIdUiNoUserResize,
    kFeatureIdUiParent,
    kFeatureIdUiPortMap,
    kFeatureIdUiPortSubscribe,
    kFeatureIdUiResize,
    kFeatureIdUiTouch,
    kFeatureCount
};

// --------------------------------------------------------------------------------------------------------------------

struct Lv2PluginOptions {
    enum OptIndex {
        SampleRate,
        TransientWinId,
        WindowTitle,
        Null,
        Count
    };

    float sampleRate;
    int64_t transientWinId;
    const char* windowTitle;
    LV2_Options_Option opts[Count];

    Lv2PluginOptions() noexcept
        : sampleRate(gInitialSampleRate),
          transientWinId(0),
          windowTitle(kNullWindowTitle)
    {
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
        optWindowTitle.size    = kNullWindowTitleSize;
        optWindowTitle.type    = kUridAtomString;
        optWindowTitle.value   = kNullWindowTitle;

        LV2_Options_Option& optNull(opts[Null]);
        optNull.context = LV2_OPTIONS_INSTANCE;
        optNull.subject = 0;
        optNull.key     = kUridNull;
        optNull.size    = 0;
        optNull.type    = kUridNull;
        optNull.value   = nullptr;
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CarlaLv2Client : public CarlaBridgeFormat
{
public:
    CarlaLv2Client()
        : CarlaBridgeFormat(),
          fHandle(nullptr),
          fWidget(nullptr),
          fDescriptor(nullptr),
          fRdfDescriptor(nullptr),
          fRdfUiDescriptor(nullptr),
          fControlDesignatedPort(0),
          fLv2Options(),
          fUiOptions(),
          fCustomURIDs(kUridCount, std::string("urn:null")),
          fExt()
    {
        CARLA_SAFE_ASSERT(fCustomURIDs.size() == kUridCount);

        carla_zeroPointers(fFeatures, kFeatureCount+1);

        // ------------------------------------------------------------------------------------------------------------
        // initialize features (part 1)

        LV2_Log_Log* const logFt = new LV2_Log_Log;
        logFt->handle            = this;
        logFt->printf            = carla_lv2_log_printf;
        logFt->vprintf           = carla_lv2_log_vprintf;

        LV2_State_Make_Path* const stateMakePathFt = new LV2_State_Make_Path;
        stateMakePathFt->handle                    = this;
        stateMakePathFt->path                      = carla_lv2_state_make_path;

        LV2_State_Map_Path* const stateMapPathFt = new LV2_State_Map_Path;
        stateMapPathFt->handle                   = this;
        stateMapPathFt->abstract_path            = carla_lv2_state_map_abstract_path;
        stateMapPathFt->absolute_path            = carla_lv2_state_map_absolute_path;

        LV2_Programs_Host* const programsFt = new LV2_Programs_Host;
        programsFt->handle                  = this;
        programsFt->program_changed         = carla_lv2_program_changed;

        LV2_URI_Map_Feature* const uriMapFt = new LV2_URI_Map_Feature;
        uriMapFt->callback_data             = this;
        uriMapFt->uri_to_id                 = carla_lv2_uri_to_id;

        LV2_URID_Map* const uridMapFt = new LV2_URID_Map;
        uridMapFt->handle             = this;
        uridMapFt->map                = carla_lv2_urid_map;

        LV2_URID_Unmap* const uridUnmapFt = new LV2_URID_Unmap;
        uridUnmapFt->handle               = this;
        uridUnmapFt->unmap                = carla_lv2_urid_unmap;

        LV2UI_Port_Map* const uiPortMapFt = new LV2UI_Port_Map;
        uiPortMapFt->handle               = this;
        uiPortMapFt->port_index           = carla_lv2_ui_port_map;

        LV2UI_Resize* const uiResizeFt    = new LV2UI_Resize;
        uiResizeFt->handle                = this;
        uiResizeFt->ui_resize             = carla_lv2_ui_resize;

        // ------------------------------------------------------------------------------------------------------------
        // initialize features (part 2)

        for (uint32_t i=0; i < kFeatureCount; ++i)
            fFeatures[i] = new LV2_Feature;

        fFeatures[kFeatureIdLogs]->URI       = LV2_LOG__log;
        fFeatures[kFeatureIdLogs]->data      = logFt;

        fFeatures[kFeatureIdOptions]->URI    = LV2_OPTIONS__options;
        fFeatures[kFeatureIdOptions]->data   = fLv2Options.opts;

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

        fFeatures[kFeatureIdUiResize]->URI  = LV2_UI__resize;
        fFeatures[kFeatureIdUiResize]->data = uiResizeFt;

        fFeatures[kFeatureIdUiTouch]->URI   = LV2_UI__touch;
        fFeatures[kFeatureIdUiTouch]->data  = nullptr;
    }

    ~CarlaLv2Client() override
    {
        if (fHandle != nullptr && fDescriptor != nullptr && fDescriptor->cleanup != nullptr)
        {
            fDescriptor->cleanup(fHandle);
            fHandle = nullptr;
        }

        if (fRdfDescriptor != nullptr)
        {
            delete fRdfDescriptor;
            fRdfDescriptor = nullptr;
        }

        fRdfUiDescriptor = nullptr;

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
    }

    // ----------------------------------------------------------------------------------------------------------------
    // UI initialization

    bool init(const int argc, const char* argv[]) override
    {
        const char* pluginURI = argv[1];
        const char* uiURI     = argv[2];

        // ------------------------------------------------------------------------------------------------------------
        // load plugin

        Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());
        lv2World.initIfNeeded(std::getenv("LV2_PATH"));

#if 0
        Lilv::Node bundleNode(lv2World.new_file_uri(nullptr, uiBundle));
        CARLA_SAFE_ASSERT_RETURN(bundleNode.is_uri(), false);

        CarlaString sBundle(bundleNode.as_uri());

        if (! sBundle.endsWith("/"))
           sBundle += "/";

        lv2World.load_bundle(sBundle);
#endif

        // ------------------------------------------------------------------------------------------------------------
        // get plugin from lv2_rdf (lilv)

        fRdfDescriptor = lv2_rdf_new(pluginURI, false);
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, false);

        // ------------------------------------------------------------------------------------------------------------
        // find requested UI

        for (uint32_t i=0; i < fRdfDescriptor->UICount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->UIs[i].URI, uiURI) == 0)
            {
                fRdfUiDescriptor = &fRdfDescriptor->UIs[i];
                break;
            }
        }

        CARLA_SAFE_ASSERT_RETURN(fRdfUiDescriptor != nullptr, false);

        // ------------------------------------------------------------------------------------------------------------
        // check if not resizable

        for (uint32_t i=0; i < fRdfUiDescriptor->FeatureCount; ++i)
        {
            if (std::strcmp(fRdfUiDescriptor->Features[i].URI, LV2_UI__fixedSize   ) == 0 ||
                std::strcmp(fRdfUiDescriptor->Features[i].URI, LV2_UI__noUserResize) == 0)
            {
                fUiOptions.isResizable = false;
                break;
            }
        }

        // ------------------------------------------------------------------------------------------------------------
        // init UI

        if (! CarlaBridgeFormat::init(argc, argv))
            return false;

        // ------------------------------------------------------------------------------------------------------------
        // open DLL

        if (! libOpen(fRdfUiDescriptor->Binary))
        {
            carla_stderr("Failed to load UI binary, error was:\n%s", libError());
            return false;
        }

        // ------------------------------------------------------------------------------------------------------------
        // get DLL main entry

        const LV2UI_DescriptorFunction ui_descFn = (LV2UI_DescriptorFunction)libSymbol("lv2ui_descriptor");

        if (ui_descFn == nullptr)
            return false;

        // ------------------------------------------------------------------------------------------------------------
        // get descriptor that matches URI

        for (uint32_t i=0; (fDescriptor = ui_descFn(i++)) != nullptr;)
        {
            if (std::strcmp(fDescriptor->URI, uiURI) == 0)
                break;
        }

        if (fDescriptor == nullptr)
        {
            carla_stderr("Failed to find UI descriptor");
            return false;
        }

        // ------------------------------------------------------------------------------------------------------------
        // initialize UI

#if defined(BRIDGE_COCOA) || defined(BRIDGE_HWND) || defined(BRIDGE_X11)
        fFeatures[kFeatureIdUiParent]->data = fToolkit->getContainerId();
#endif

        fHandle = fDescriptor->instantiate(fDescriptor, fRdfDescriptor->URI, fRdfUiDescriptor->Bundle,
                                           carla_lv2_ui_write_function, this, &fWidget, fFeatures);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, false);

#if defined(BRIDGE_COCOA) || defined(BRIDGE_HWND) || defined(BRIDGE_X11)
        if (fWidget != nullptr)
            fToolkit->setChildWindow(fWidget);
#endif

        // ------------------------------------------------------------------------------------------------------------
        // check for known extensions

        if (fDescriptor->extension_data != nullptr)
        {
            fExt.options  = (const LV2_Options_Interface*)fDescriptor->extension_data(LV2_OPTIONS__interface);
            fExt.programs = (const LV2_Programs_UI_Interface*)fDescriptor->extension_data(LV2_PROGRAMS__UIInterface);
            fExt.idle     = (const LV2UI_Idle_Interface*)fDescriptor->extension_data(LV2_UI__idleInterface);
            fExt.resize   = (const LV2UI_Resize*)fDescriptor->extension_data(LV2_UI__resize);

            // check if invalid
            if (fExt.programs != nullptr && fExt.programs->select_program == nullptr)
                fExt.programs = nullptr;
            if (fExt.idle != nullptr && fExt.idle->idle == nullptr)
                fExt.idle = nullptr;
            if (fExt.resize != nullptr && fExt.resize->ui_resize == nullptr)
                fExt.resize = nullptr;
        }

        for (uint32_t i=0; i<fRdfDescriptor->PortCount; ++i)
        {
            if (LV2_IS_PORT_DESIGNATION_CONTROL(fRdfDescriptor->Ports[i].Designation))
            {
                fControlDesignatedPort = i;
                break;
            }
        }

        return true;
    }

    void idleUI() override
    {
#if defined(BRIDGE_COCOA) || defined(BRIDGE_HWND) || defined(BRIDGE_X11)
        if (fHandle != nullptr && fExt.idle != nullptr && fExt.idle->idle(fHandle) != 0)
        {
            if (isPipeRunning() && ! fQuitReceived)
                writeExitingMessageAndWait();
        }
#endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // UI management

    void* getWidget() const noexcept override
    {
        return fWidget;
    }

    const Options& getOptions() const noexcept override
    {
        return fUiOptions;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // DSP Callbacks

    void dspParameterChanged(const uint32_t index, const float value) override
    {
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,)
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->port_event == nullptr)
            return;

        fDescriptor->port_event(fHandle, index, sizeof(float), kUridNull, &value);
    }

    void dspProgramChanged(const uint32_t index) override
    {
        carla_stderr2("dspProgramChanged(%i) - not handled", index);
    }

    void dspMidiProgramChanged(const uint32_t bank, const uint32_t program) override
    {
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,)

        if (fExt.programs == nullptr)
            return;

        fExt.programs->select_program(fHandle, bank, program);
    }

    void dspStateChanged(const char* const, const char* const) override
    {
    }

    void dspNoteReceived(const bool onOff, const uint8_t channel, const uint8_t note, const uint8_t velocity) override
    {
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,)
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->port_event == nullptr)
            return;

        LV2_Atom_MidiEvent midiEv;
        midiEv.atom.type = kUridMidiEvent;
        midiEv.atom.size = 3;
        midiEv.data[0] = uint8_t((onOff ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (channel & MIDI_CHANNEL_BIT));
        midiEv.data[1] = note;
        midiEv.data[2] = velocity;

        fDescriptor->port_event(fHandle, fControlDesignatedPort, lv2_atom_total_size(midiEv), kUridAtomTransferEvent, &midiEv);
    }

    void dspAtomReceived(const uint32_t portIndex, const LV2_Atom* const atom) override
    {
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(atom != nullptr,);

        if (fDescriptor->port_event == nullptr)
            return;

        fDescriptor->port_event(fHandle, portIndex, lv2_atom_total_size(atom), kUridAtomTransferEvent, atom);
    }

    void dspURIDReceived(const LV2_URID urid, const char* const uri) override
    {
        CARLA_SAFE_ASSERT_RETURN(urid == fCustomURIDs.size(),);
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0',);

        fCustomURIDs.push_back(uri);
    }

    void uiOptionsChanged(const double sampleRate, const bool useTheme, const bool useThemeColors, const char* const windowTitle, uintptr_t transientWindowId) override
    {
        carla_debug("CarlaLv2Client::uiOptionsChanged(%g, %s, %s, \"%s\", " P_UINTPTR ")", sampleRate, bool2str(useTheme), bool2str(useThemeColors), windowTitle, transientWindowId);

        const float sampleRatef = static_cast<float>(sampleRate);

        if (carla_isNotEqual(fLv2Options.sampleRate, sampleRatef))
        {
            fLv2Options.sampleRate = sampleRatef;

            if (fExt.options != nullptr && fExt.options->set != nullptr)
            {
                LV2_Options_Option options[2];
                carla_zeroStructs(options, 2);

                LV2_Options_Option& optSampleRate(options[0]);
                optSampleRate.context = LV2_OPTIONS_INSTANCE;
                optSampleRate.subject = 0;
                optSampleRate.key     = kUridParamSampleRate;
                optSampleRate.size    = sizeof(float);
                optSampleRate.type    = kUridAtomFloat;
                optSampleRate.value   = &fLv2Options.sampleRate;

                fExt.options->set(fHandle, options);
            }
        }

        if (fLv2Options.windowTitle != nullptr && fLv2Options.windowTitle != kNullWindowTitle)
            delete[] fLv2Options.windowTitle;

        fLv2Options.transientWinId = static_cast<int64_t>(transientWindowId);
        fLv2Options.windowTitle    = carla_strdup_safe(windowTitle);

        if (fLv2Options.windowTitle == nullptr)
            fLv2Options.windowTitle = kNullWindowTitle;
        else
            fUiOptions.windowTitle = fLv2Options.windowTitle;

        fLv2Options.opts[Lv2PluginOptions::WindowTitle].size  = (uint32_t)std::strlen(fLv2Options.windowTitle);
        fLv2Options.opts[Lv2PluginOptions::WindowTitle].value = fLv2Options.windowTitle;

        fUiOptions.useTheme          = useTheme;
        fUiOptions.useThemeColors    = useThemeColors;
        fUiOptions.transientWindowId = transientWindowId;
    }

    void uiResized(const uint width, const uint height) override
    {
        if (fHandle != nullptr && fExt.resize != nullptr)
            fExt.resize->ui_resize(fHandle, static_cast<int>(width), static_cast<int>(height));
    }

    // ----------------------------------------------------------------------------------------------------------------

    LV2_URID getCustomURID(const char* const uri)
    {
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', kUridNull);
        carla_debug("CarlaLv2Client::getCustomURID(\"%s\")", uri);

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

        if (isPipeRunning())
            writeLv2UridMessage(urid, uri);

        return urid;
    }

    const char* getCustomURIDString(const LV2_URID urid) const noexcept
    {
        static const char* const sFallback = "urn:null";
        CARLA_SAFE_ASSERT_RETURN(urid != kUridNull, sFallback);
        CARLA_SAFE_ASSERT_RETURN(urid < fCustomURIDs.size(), sFallback);
        carla_debug("CarlaLv2Client::getCustomURIDString(%i)", urid);

        return fCustomURIDs[urid].c_str();
    }

    // ----------------------------------------------------------------------------------------------------------------

    void handleProgramChanged(const int32_t index)
    {
        if (isPipeRunning())
            writeReloadProgramsMessage(index);
    }

    uint32_t handleUiPortMap(const char* const symbol)
    {
        CARLA_SAFE_ASSERT_RETURN(symbol != nullptr && symbol[0] != '\0', LV2UI_INVALID_PORT_INDEX);
        carla_debug("CarlaLv2Client::handleUiPortMap(\"%s\")", symbol);

        for (uint32_t i=0; i < fRdfDescriptor->PortCount; ++i)
        {
            if (std::strcmp(fRdfDescriptor->Ports[i].Symbol, symbol) == 0)
                return i;
        }

        return LV2UI_INVALID_PORT_INDEX;
    }

    int handleUiResize(const int width, const int height)
    {
        CARLA_SAFE_ASSERT_RETURN(fToolkit != nullptr, 1);
        CARLA_SAFE_ASSERT_RETURN(width > 0, 1);
        CARLA_SAFE_ASSERT_RETURN(height > 0, 1);
        carla_debug("CarlaLv2Client::handleUiResize(%i, %i)", width, height);

        fToolkit->setSize(static_cast<uint>(width), static_cast<uint>(height));
        return 0;
    }

    void handleUiWrite(uint32_t rindex, uint32_t bufferSize, uint32_t format, const void* buffer)
    {
        CARLA_SAFE_ASSERT_RETURN(buffer != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(bufferSize > 0,);
        carla_debug("CarlaLv2Client::handleUiWrite(%i, %i, %i, %p)", rindex, bufferSize, format, buffer);

        switch (format)
        {
        case kUridNull:
            CARLA_SAFE_ASSERT_RETURN(bufferSize == sizeof(float),);

            if (isPipeRunning())
            {
                const float value(*(const float*)buffer);
                writeControlMessage(rindex, value);
            }
            break;

        case kUridAtomTransferAtom:
        case kUridAtomTransferEvent:
            CARLA_SAFE_ASSERT_RETURN(bufferSize >= sizeof(LV2_Atom),);

            if (isPipeRunning())
            {
                const LV2_Atom* const atom((const LV2_Atom*)buffer);

                // plugins sometimes fail on this, not good...
                const uint32_t totalSize =  lv2_atom_total_size(atom);
                const uint32_t paddedSize = lv2_atom_pad_size(totalSize);

                if (bufferSize != totalSize && bufferSize != paddedSize)
                    carla_stderr2("Warning: LV2 UI sending atom with invalid size! size: %u, padded-size: %u", totalSize, paddedSize);

                writeLv2AtomMessage(rindex, atom);
            }
            break;

        default:
            carla_stderr("CarlaLv2Client::handleUiWrite(%i, %i, %i:\"%s\", %p) - unknown format",
                         rindex, bufferSize, format, carla_lv2_urid_unmap(this, format), buffer);
            break;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    LV2UI_Handle fHandle;
    LV2UI_Widget fWidget;
    LV2_Feature* fFeatures[kFeatureCount+1];

    const LV2UI_Descriptor*   fDescriptor;
    const LV2_RDF_Descriptor* fRdfDescriptor;
    const LV2_RDF_UI*         fRdfUiDescriptor;
    uint32_t                  fControlDesignatedPort;
    Lv2PluginOptions          fLv2Options;

    Options fUiOptions;
    std::vector<std::string> fCustomURIDs;

    struct Extensions {
        const LV2_Options_Interface* options;
        const LV2_Programs_UI_Interface* programs;
        const LV2UI_Idle_Interface* idle;
        const LV2UI_Resize* resize;

        Extensions()
            : options(nullptr),
              programs(nullptr),
              idle(nullptr),
              resize(nullptr) {}
    } fExt;

    // ----------------------------------------------------------------------------------------------------------------
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

    // ----------------------------------------------------------------------------------------------------------------
    // Programs Feature

    static void carla_lv2_program_changed(LV2_Programs_Handle handle, int32_t index)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);
        carla_debug("carla_lv2_program_changed(%p, %i)", handle, index);

        ((CarlaLv2Client*)handle)->handleProgramChanged(index);
    }

    // ----------------------------------------------------------------------------------------------------------------
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

    // ----------------------------------------------------------------------------------------------------------------
    // URI-Map Feature

    static uint32_t carla_lv2_uri_to_id(LV2_URI_Map_Callback_Data data, const char* map, const char* uri)
    {
        carla_debug("carla_lv2_uri_to_id(%p, \"%s\", \"%s\")", data, map, uri);
        return carla_lv2_urid_map((LV2_URID_Map_Handle*)data, uri);

        // unused
        (void)map;
    }

    // ----------------------------------------------------------------------------------------------------------------
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
        if (std::strcmp(uri, LV2_UI__windowTitle) == 0)
            return kUridWindowTitle;

        // Custom Carla types
        if (std::strcmp(uri, URI_CARLA_ATOM_WORKER) == 0)
            return kUridCarlaAtomWorker;
        if (std::strcmp(uri, LV2_KXSTUDIO_PROPERTIES__TransientWindowId) == 0)
            return kUridCarlaTransientWindowId;

        // Custom plugin types
        return ((CarlaLv2Client*)handle)->getCustomURID(uri);
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
        case kUridWindowTitle:
            return LV2_UI__windowTitle;

        // Custom Carla types
        case kUridCarlaAtomWorker:
            return URI_CARLA_ATOM_WORKER;
        case kUridCarlaTransientWindowId:
            return LV2_KXSTUDIO_PROPERTIES__TransientWindowId;
        }

        // Custom types
        return ((CarlaLv2Client*)handle)->getCustomURIDString(urid);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // UI Port-Map Feature

    static uint32_t carla_lv2_ui_port_map(LV2UI_Feature_Handle handle, const char* symbol)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, LV2UI_INVALID_PORT_INDEX);
        carla_debug("carla_lv2_ui_port_map(%p, \"%s\")", handle, symbol);

        return ((CarlaLv2Client*)handle)->handleUiPortMap(symbol);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // UI Resize Feature

    static int carla_lv2_ui_resize(LV2UI_Feature_Handle handle, int width, int height)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr, 1);
        carla_debug("carla_lv2_ui_resize(%p, %i, %i)", handle, width, height);

        return ((CarlaLv2Client*)handle)->handleUiResize(width, height);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // UI Extension

    static void carla_lv2_ui_write_function(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
    {
        CARLA_SAFE_ASSERT_RETURN(controller != nullptr,);
        carla_debug("carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);

        ((CarlaLv2Client*)controller)->handleUiWrite(port_index, buffer_size, format, buffer);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaLv2Client)
};

// --------------------------------------------------------------------------------------------------------------------

CARLA_BRIDGE_UI_END_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    CARLA_BRIDGE_UI_USE_NAMESPACE

    if (argc < 3)
    {
        carla_stderr("usage: %s <plugin-uri> <ui-uri>", argv[0]);
        return 1;
    }

    const bool testingModeOnly = (argc != 7);

    // try to get sampleRate value
    if (const char* const sampleRateStr = std::getenv("CARLA_SAMPLE_RATE"))
        gInitialSampleRate = std::atof(sampleRateStr);

    // Init LV2 client
    CarlaLv2Client client;

    // Load UI
    int ret;

    if (client.init(argc, argv))
    {
        client.exec(testingModeOnly);
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}

// --------------------------------------------------------------------------------------------------------------------
