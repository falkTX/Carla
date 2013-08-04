/*
 * Carla Native Plugins
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "carla-native-base.cpp"

#include "CarlaString.hpp"

#include "lv2/atom.h"
#include "lv2/buf-size.h"
#include "lv2/instance-access.h"
#include "lv2/options.h"
#include "lv2/state.h"
#include "lv2/ui.h"
#include "lv2/lv2_external_ui.h"

#include <QtCore/Qt>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QFileDialog>
#else
# include <QtGui/QFileDialog>
#endif

// -----------------------------------------------------------------------
// LV2 descriptor functions

class NativePlugin : public LV2_External_UI_Widget
{
public:
    static const uint32_t kMaxMidiEvents = 512;

    NativePlugin(const PluginDescriptor* const desc, const double sampleRate, const char* const bundlePath, const LV2_Feature* const* features)
        : fHandle(nullptr),
          fDescriptor(desc),
          fMidiEventCount(0),
          fIsProcessing(false),
          fBufferSize(0),
          fSampleRate(sampleRate)
    {
        run  = extui_run;
        show = extui_show;
        hide = extui_hide;

        fHost.handle      = this;
        fHost.resourceDir = carla_strdup(bundlePath);
        fHost.uiName      = nullptr;

        fHost.get_buffer_size        = host_get_buffer_size;
        fHost.get_sample_rate        = host_get_sample_rate;
        fHost.is_offline             = host_is_offline;
        fHost.get_time_info          = host_get_time_info;
        fHost.write_midi_event       = host_write_midi_event;
        fHost.ui_parameter_changed   = host_ui_parameter_changed;
        fHost.ui_custom_data_changed = host_ui_custom_data_changed;
        fHost.ui_closed              = host_ui_closed;
        fHost.ui_open_file           = host_ui_open_file;
        fHost.ui_save_file           = host_ui_save_file;
        fHost.dispatcher             = host_dispatcher;

        const LV2_Options_Option* options = nullptr;
        const LV2_URID_Map*       uridMap = nullptr;

        for (int i=0; features[i] != nullptr; ++i)
        {
            if (std::strcmp(features[i]->URI, LV2_OPTIONS__options) == 0)
                options = (const LV2_Options_Option*)features[i]->data;
            else if (std::strcmp(features[i]->URI, LV2_URID__map) == 0)
                uridMap = (const LV2_URID_Map*)features[i]->data;
        }

        if (options == nullptr || uridMap == nullptr)
        {
            carla_stderr("Host don't provides option or urid-map features");
            return;
        }

        fBufferSize = 1024;

        for (int i=0; options[i].key != 0; ++i)
        {
            if (options[i].key == uridMap->map(uridMap->handle, LV2_BUF_SIZE__maxBlockLength))
            {
                if (options[i].type == uridMap->map(uridMap->handle, LV2_ATOM__Int))
                    fBufferSize = *(const int*)options[i].value;
                else
                    carla_stderr("Host provides maxBlockLength but has wrong value type");
                break;
            }
        }

        fUridMap = uridMap;

        fUI.offset += (desc->midiIns > 0) ? desc->midiIns : 1;
        fUI.offset += desc->midiOuts;
        fUI.offset += 1; // freewheel
        fUI.offset += desc->audioIns;
        fUI.offset += desc->audioOuts;
    }

    ~NativePlugin()
    {
        CARLA_ASSERT(fHandle == nullptr);

        if (fHost.resourceDir != nullptr)
        {
            delete[] fHost.resourceDir;
            fHost.resourceDir = nullptr;
        }
    }

    bool init()
    {
        if (fDescriptor->instantiate == nullptr || fDescriptor->process == nullptr)
        {
            carla_stderr("Plugin is missing something...");
            return false;
        }
        if (fBufferSize == 0)
        {
            carla_stderr("Plugin is missing bufferSize value");
            return false;
        }

        fHandle = fDescriptor->instantiate(&fHost);

        if (fHandle == nullptr)
            return false;

        carla_zeroStruct<MidiEvent>(fMidiEvents, kMaxMidiEvents*2);
        carla_zeroStruct<TimeInfo>(fTimeInfo);
        fPorts.init(fDescriptor, fHandle);
        return true;
    }

    // -------------------------------------------------------------------
    // LV2 functions

    void lv2_connect_port(const uint32_t port, void* const dataLocation)
    {
        fPorts.connectPort(fDescriptor, port, dataLocation);
    }

    void lv2_activate()
    {
        if (fDescriptor->activate != nullptr)
            fDescriptor->activate(fHandle);
    }

    void lv2_deactivate()
    {
        if (fDescriptor->deactivate != nullptr)
            fDescriptor->deactivate(fHandle);
    }

    void lv2_cleanup()
    {
        if (fDescriptor->cleanup != nullptr)
            fDescriptor->cleanup(fHandle);
        fHandle = nullptr;
    }

    void lv2_run(const uint32_t frames)
    {
        if (frames == 0)
        {
            updateParameterOutputs();
            return;
        }

        // Check for updated parameters
        float curValue;

        for (uint32_t i=0; i < fPorts.paramCount; ++i)
        {
            CARLA_SAFE_ASSERT_CONTINUE(fPorts.paramsPtr[i] != nullptr)

            curValue = *fPorts.paramsPtr[i];

            if (fPorts.paramsLast[i] != curValue && (fDescriptor->get_parameter_info(fHandle, i)->hints & PARAMETER_IS_OUTPUT) == 0)
            {
                fPorts.paramsLast[i] = curValue;
                fDescriptor->set_parameter_value(fHandle, i, curValue);
            }
        }

        fDescriptor->process(fHandle, fPorts.audioIns, fPorts.audioOuts, frames, fMidiEventCount, fMidiEvents);

        updateParameterOutputs();
    }

    // -------------------------------------------------------------------

    void lv2ui_instantiate(LV2UI_Write_Function writeFunction, LV2UI_Controller controller, LV2UI_Widget* widget,
                           const LV2_Feature* const* features)
    {
        for (int i=0; features[i] != nullptr; ++i)
        {
            if (std::strcmp(features[i]->URI, LV2_EXTERNAL_UI__Host) == 0 ||
                std::strcmp(features[i]->URI, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0)
            {
                fUI.host = (const LV2_External_UI_Host*)features[i]->data;
                break;
            }
        }

        if (fUI.host != nullptr)
            fHost.uiName = fUI.host->plugin_human_id;

        fUI.writeFunction = writeFunction;
        fUI.controller = controller;
        *widget = this;
    }

    void lv2ui_port_event(uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
    {
        if (format != 0 || bufferSize != sizeof(float) || buffer == nullptr)
            return;
        if (portIndex >= fUI.offset || ! fUI.isVisible)
            return;
        if (fDescriptor->ui_set_parameter_value == nullptr)
            return;

        const float value(*(const float*)buffer);
        fDescriptor->ui_set_parameter_value(fHandle, portIndex-fUI.offset, value);
    }

    void lv2ui_cleanup()
    {
        fUI.host = nullptr;
        fUI.writeFunction = nullptr;
        fUI.controller = nullptr;

        if (! fUI.isVisible)
            return;

        if (fDescriptor->ui_show != nullptr)
            fDescriptor->ui_show(fHandle, false);

        fUI.isVisible = false;
    }

    // -------------------------------------------------------------------

protected:
    void handleUiRun()
    {
        // TODO - idle Qt if needed

        if (fDescriptor->ui_idle != nullptr)
            fDescriptor->ui_idle(fHandle);
    }

    void handleUiShow()
    {
        if (fDescriptor->ui_show != nullptr)
            fDescriptor->ui_show(fHandle, true);

        fUI.isVisible = true;
    }

    void handleUiHide()
    {
        if (fDescriptor->ui_show != nullptr)
            fDescriptor->ui_show(fHandle, false);

        fUI.isVisible = false;
    }

    // -------------------------------------------------------------------

    uint32_t handleGetBufferSize()
    {
        return fBufferSize;
    }

    double handleGetSampleRate()
    {
        return fSampleRate;
    }

    bool handleIsOffline()
    {
        CARLA_SAFE_ASSERT_RETURN(fIsProcessing, false);

        return (fPorts.freewheel != nullptr && *fPorts.freewheel > 0.5f);
    }

    const TimeInfo* handleGetTimeInfo()
    {
        CARLA_SAFE_ASSERT_RETURN(fIsProcessing, nullptr);

        return &fTimeInfo;
    }

    bool handleWriteMidiEvent(const MidiEvent* const event)
    {
        CARLA_SAFE_ASSERT_RETURN(fIsProcessing, false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->midiOuts > 0, false);
        CARLA_SAFE_ASSERT_RETURN(event != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(event->data[0] != 0, false);

        if (fMidiEventCount >= kMaxMidiEvents*2)
            return false;

        // reverse-find first free event, and put it there
        for (uint32_t i=(kMaxMidiEvents*2)-1; i >= fMidiEventCount; --i)
        {
            if (fMidiEvents[i].data[0] == 0)
            {
                std::memcpy(&fMidiEvents[i], event, sizeof(MidiEvent));
                break;
            }
        }

        return true;
    }

    void handleUiParameterChanged(const uint32_t index, const float value)
    {
        if (fUI.writeFunction != nullptr && fUI.controller != nullptr)
            fUI.writeFunction(fUI.controller, index+fUI.offset, sizeof(float), 0, &value);
    }

    void handleUiCustomDataChanged(const char* const /*key*/, const char* const /*value*/)
    {
        //storeCustomData(key, value);
    }

    void handleUiClosed()
    {
        if (fUI.host != nullptr && fUI.host->ui_closed != nullptr && fUI.controller != nullptr)
            fUI.host->ui_closed(fUI.controller);

        fUI.host = nullptr;
        fUI.writeFunction = nullptr;
        fUI.controller = nullptr;
        fUI.isVisible = false;
    }

    const char* handleUiOpenFile(const bool isDir, const char* const title, const char* const filter)
    {
        static CarlaString retStr;
        QFileDialog::Options options(isDir ? QFileDialog::ShowDirsOnly : 0x0);

        retStr = QFileDialog::getOpenFileName(nullptr, title, "", filter, nullptr, options).toUtf8().constData();

        return retStr.isNotEmpty() ? (const char*)retStr : nullptr;
    }

    const char* handleUiSaveFile(const bool isDir, const char* const title, const char* const filter)
    {
        static CarlaString retStr;
        QFileDialog::Options options(isDir ? QFileDialog::ShowDirsOnly : 0x0);

        retStr = QFileDialog::getSaveFileName(nullptr, title, "", filter, nullptr, options).toUtf8().constData();

        return retStr.isNotEmpty() ? (const char*)retStr : nullptr;
    }

    intptr_t handleDispatcher(const ::HostDispatcherOpcode opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
        carla_debug("NativePlugin::handleDispatcher(%i, %i, " P_INTPTR ", %p, %f)", opcode, index, value, ptr, opt);

        intptr_t ret = 0;

        switch (opcode)
        {
        case HOST_OPCODE_NULL:
            break;
        case HOST_OPCODE_SET_VOLUME:
            //setVolume(opt, true, true);
            break;
        case HOST_OPCODE_SET_DRYWET:
            //setDryWet(opt, true, true);
            break;
        case HOST_OPCODE_SET_BALANCE_LEFT:
            //setBalanceLeft(opt, true, true);
            break;
        case HOST_OPCODE_SET_BALANCE_RIGHT:
            //setBalanceRight(opt, true, true);
            break;
        case HOST_OPCODE_SET_PANNING:
            //setPanning(opt, true, true);
            break;
        case HOST_OPCODE_GET_PARAMETER_MIDI_CC:
        case HOST_OPCODE_SET_PARAMETER_MIDI_CC:
        case HOST_OPCODE_SET_PROCESS_PRECISION:
        case HOST_OPCODE_UPDATE_PARAMETER:
        case HOST_OPCODE_UPDATE_MIDI_PROGRAM:
        case HOST_OPCODE_RELOAD_PARAMETERS:
        case HOST_OPCODE_RELOAD_MIDI_PROGRAMS:
        case HOST_OPCODE_RELOAD_ALL:
            break;
        case HOST_OPCODE_UI_UNAVAILABLE:
            handleUiClosed();
            break;
        }

        return ret;

        // unused for now
        (void)index;
        (void)value;
        (void)ptr;
    }

    void updateParameterOutputs()
    {
        for (uint32_t i=0; i < fPorts.paramCount; ++i)
        {
            if (fDescriptor->get_parameter_info(fHandle, i)->hints & PARAMETER_IS_OUTPUT)
            {
                fPorts.paramsLast[i] = fDescriptor->get_parameter_value(fHandle, i);

                if (fPorts.paramsPtr[i] != nullptr)
                    *fPorts.paramsPtr[i] = fPorts.paramsLast[i];
            }
        }
    }

    // -------------------------------------------------------------------

private:
    // Native data
    PluginHandle   fHandle;
    HostDescriptor fHost;
    const PluginDescriptor* const fDescriptor;

    uint32_t  fMidiEventCount;
    MidiEvent fMidiEvents[kMaxMidiEvents*2];
    TimeInfo  fTimeInfo;

    bool fIsProcessing;

    // Lv2 host data
    uint32_t fBufferSize;
    double   fSampleRate;

    const LV2_URID_Map* fUridMap;

    struct UI {
        const LV2_External_UI_Host* host;
        LV2UI_Write_Function writeFunction;
        LV2UI_Controller controller;
        bool isVisible;
        uint32_t offset;

        UI()
            : host(nullptr),
              writeFunction(nullptr),
              controller(nullptr),
              isVisible(false),
              offset(0) {}
    } fUI;

    struct Ports {
        LV2_Atom_Sequence** eventsIn;
        LV2_Atom_Sequence** midiOuts;
        float**  audioIns;
        float**  audioOuts;
        float*   freewheel;
        uint32_t paramCount;
        float*   paramsLast;
        float**  paramsPtr;

        Ports()
            : eventsIn(nullptr),
              midiOuts(nullptr),
              audioIns(nullptr),
              audioOuts(nullptr),
              freewheel(nullptr),
              paramCount(0),
              paramsLast(nullptr),
              paramsPtr(nullptr) {}

        ~Ports()
        {
            if (eventsIn != nullptr)
            {
                delete[] eventsIn;
                eventsIn = nullptr;
            }

            if (midiOuts != nullptr)
            {
                delete[] midiOuts;
                midiOuts = nullptr;
            }

            if (audioIns != nullptr)
            {
                delete[] audioIns;
                audioIns = nullptr;
            }

            if (audioOuts != nullptr)
            {
                delete[] audioOuts;
                audioOuts = nullptr;
            }

            if (paramsLast != nullptr)
            {
                delete[] paramsLast;
                paramsLast = nullptr;
            }

            if (paramsPtr != nullptr)
            {
                delete[] paramsPtr;
                paramsPtr = nullptr;
            }
        }

        void init(const PluginDescriptor* const desc, PluginHandle handle)
        {
            if (desc->midiIns > 0)
            {
                eventsIn = new LV2_Atom_Sequence*[desc->midiIns];

                for (uint32_t i=0; i < desc->midiIns; ++i)
                    eventsIn[i] = nullptr;
            }
            else
            {
                eventsIn = new LV2_Atom_Sequence*[1];
                eventsIn[0] = nullptr;
            }

            if (desc->midiOuts > 0)
            {
                midiOuts = new LV2_Atom_Sequence*[desc->midiOuts];

                for (uint32_t i=0; i < desc->midiOuts; ++i)
                    midiOuts[i] = nullptr;
            }

            if (desc->audioIns > 0)
            {
                audioIns = new float*[desc->audioIns];

                for (uint32_t i=0; i < desc->audioIns; ++i)
                    audioIns[i] = nullptr;
            }

            if (desc->audioOuts > 0)
            {
                audioOuts = new float*[desc->audioOuts];

                for (uint32_t i=0; i < desc->audioOuts; ++i)
                    audioOuts[i] = nullptr;
            }

            if (desc->get_parameter_count != nullptr && desc->get_parameter_info != nullptr && desc->get_parameter_value != nullptr && desc->set_parameter_value != nullptr)
            {
                paramCount = desc->get_parameter_count(handle);

                if (paramCount > 0)
                {
                    paramsLast = new float[paramCount];
                    paramsPtr  = new float*[paramCount];

                    for (uint32_t i=0; i < paramCount; ++i)
                    {
                        paramsLast[i] = desc->get_parameter_value(handle, i);
                        paramsPtr[i] = nullptr;
                    }
                }
            }
        }

        void connectPort(const PluginDescriptor* const desc, const uint32_t port, void* const dataLocation)
        {
            uint32_t index = 0;

            if (port == index++)
            {
                eventsIn[0] = (LV2_Atom_Sequence*)dataLocation;
                return;
            }

            for (uint32_t i=1; i < desc->midiIns; ++i)
            {
                if (port == index++)
                {
                    eventsIn[i] = (LV2_Atom_Sequence*)dataLocation;
                    return;
                }
            }

            for (uint32_t i=0; i < desc->midiOuts; ++i)
            {
                if (port == index++)
                {
                    midiOuts[i] = (LV2_Atom_Sequence*)dataLocation;
                    return;
                }
            }

            if (port == index++)
            {
                freewheel = (float*)dataLocation;
                return;
            }

            for (uint32_t i=0; i < desc->audioIns; ++i)
            {
                if (port == index++)
                {
                    audioIns[i] = (float*)dataLocation;
                    return;
                }
            }

            for (uint32_t i=0; i < desc->audioOuts; ++i)
            {
                if (port == index++)
                {
                    audioOuts[i] = (float*)dataLocation;
                    return;
                }
            }

            for (uint32_t i=0; i < paramCount; ++i)
            {
                if (port == index++)
                {
                    paramsPtr[i] = (float*)dataLocation;
                    return;
                }
            }
        }

    } fPorts;

    // -------------------------------------------------------------------

    #define handlePtr ((NativePlugin*)_this_)

    static void extui_run(LV2_External_UI_Widget* _this_)
    {
        handlePtr->handleUiRun();
    }

    static void extui_show(LV2_External_UI_Widget* _this_)
    {
        handlePtr->handleUiShow();
    }

    static void extui_hide(LV2_External_UI_Widget* _this_)
    {
        handlePtr->handleUiHide();
    }

    #undef handlePtr

    // -------------------------------------------------------------------

    #define handlePtr ((NativePlugin*)handle)

    static uint32_t host_get_buffer_size(HostHandle handle)
    {
        return handlePtr->handleGetBufferSize();
    }

    static double host_get_sample_rate(HostHandle handle)
    {
        return handlePtr->handleGetSampleRate();
    }

    static bool host_is_offline(HostHandle handle)
    {
        return handlePtr->handleIsOffline();
    }

    static const TimeInfo* host_get_time_info(HostHandle handle)
    {
        return handlePtr->handleGetTimeInfo();
    }

    static bool host_write_midi_event(HostHandle handle, const ::MidiEvent* event)
    {
        return handlePtr->handleWriteMidiEvent(event);
    }

    static void host_ui_parameter_changed(HostHandle handle, uint32_t index, float value)
    {
        handlePtr->handleUiParameterChanged(index, value);
    }

    static void host_ui_custom_data_changed(HostHandle handle, const char* key, const char* value)
    {
        handlePtr->handleUiCustomDataChanged(key, value);
    }

    static void host_ui_closed(HostHandle handle)
    {
        handlePtr->handleUiClosed();
    }

    static const char* host_ui_open_file(HostHandle handle, bool isDir, const char* title, const char* filter)
    {
        return handlePtr->handleUiOpenFile(isDir, title, filter);
    }

    static const char* host_ui_save_file(HostHandle handle, bool isDir, const char* title, const char* filter)
    {
        return handlePtr->handleUiSaveFile(isDir, title, filter);
    }

    static intptr_t host_dispatcher(HostHandle handle, HostDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        return handlePtr->handleDispatcher(opcode, index, value, ptr, opt);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativePlugin)
};

// -----------------------------------------------------------------------
// LV2 descriptor functions

static LV2_Handle lv2_instantiate(const LV2_Descriptor* lv2Descriptor, double sampleRate, const char* bundlePath, const LV2_Feature* const* features)
{
    carla_debug("lv2_instantiate(%p, %g, %s, %p)", lv2Descriptor, sampleRate, bundlePath, features);

    const PluginDescriptor* pluginDesc  = nullptr;
    const char*             pluginLabel = nullptr;

    if (std::strncmp(lv2Descriptor->URI, "http://kxstudio.sf.net/carla/plugins/", 37) == 0)
        pluginLabel = lv2Descriptor->URI+37;
    else if (std::strcmp(lv2Descriptor->URI, "http://kxstudio.sf.net/carla") == 0)
        pluginLabel = lv2Descriptor->URI+23;

    if (pluginLabel == nullptr)
    {
        carla_stderr("Failed to find carla native plugin with URI: \"%s\"", lv2Descriptor->URI);
        return nullptr;
    }

    carla_debug("lv2_instantiate() - looking up label %s", pluginLabel);

    for (NonRtList<const PluginDescriptor*>::Itenerator it = sPluginDescsMgr.descs.begin(); it.valid(); it.next())
    {
        const PluginDescriptor*& tmpDesc(*it);

        if (std::strcmp(tmpDesc->label, pluginLabel) == 0)
        {
            pluginDesc = tmpDesc;
            break;
        }
    }

    if (pluginDesc == nullptr)
    {
        carla_stderr("Failed to find carla native plugin with label: \"%s\"", pluginLabel);
        return nullptr;
    }

    NativePlugin* const plugin(new NativePlugin(pluginDesc, sampleRate, bundlePath, features));

    if (! plugin->init())
    {
        carla_stderr("Failed to init plugin");
        delete plugin;
        return nullptr;
    }

    return (LV2_Handle)plugin;
}

#define instancePtr ((NativePlugin*)instance)

static void lv2_connect_port(LV2_Handle instance, uint32_t port, void* dataLocation)
{
    instancePtr->lv2_connect_port(port, dataLocation);
}

static void lv2_activate(LV2_Handle instance)
{
    carla_debug("lv2_activate(%p)", instance);
    instancePtr->lv2_activate();
}

static void lv2_run(LV2_Handle instance, uint32_t sampleCount)
{
    instancePtr->lv2_run(sampleCount);
}

static void lv2_deactivate(LV2_Handle instance)
{
    carla_debug("lv2_deactivate(%p)", instance);
    instancePtr->lv2_deactivate();
}

static void lv2_cleanup(LV2_Handle instance)
{
    carla_debug("lv2_cleanup(%p)", instance);
    instancePtr->lv2_cleanup();
    delete instancePtr;
}

#if 0
static uint32_t lv2_get_options(LV2_Handle instance, LV2_Options_Option* options)
{
    carla_debug("lv2_()", );
    return instancePtr->lv2_get_options(options);
}

static uint32_t lv2_set_options(LV2_Handle instance, const LV2_Options_Option* options)
{
    carla_debug("lv2_()", );
    return instancePtr->lv2_set_options(options);
}

static LV2_State_Status lv2_save(LV2_Handle instance, LV2_State_Store_Function store, LV2_State_Handle handle, uint32_t flags, const LV2_Feature* const* features)
{
    carla_debug("lv2_()", );
    return instancePtr->lv2_save(store, handle, flags, features);
}

static LV2_State_Status lv2_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle handle, uint32_t flags, const LV2_Feature* const* features)
{
    carla_debug("lv2_()", );
    return instancePtr->lv2_restore(retrieve, handle, flags, features);
}
#endif

static const void* lv2_extension_data(const char* uri)
{
    carla_debug("lv2_extension_data(%s)", uri);

#if 0
    static const LV2_Options_Interface options = { lv2_get_options, lv2_set_options };
    static const LV2_State_Interface   state   = { lv2_save, lv2_restore };

    if (std::strcmp(uri, LV2_OPTIONS__interface) == 0)
        return &options;
    if (std::strcmp(uri, LV2_STATE__interface) == 0)
        return &state;
#endif

    return nullptr;
}

#undef instancePtr

// -----------------------------------------------------------------------
// Startup code

static LV2UI_Handle lv2ui_instantiate(const LV2UI_Descriptor*, const char*, const char*, LV2UI_Write_Function writeFunction,
                                      LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    NativePlugin* plugin = nullptr;

    for (int i=0; features[i] != nullptr; ++i)
    {
        if (std::strcmp(features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0)
        {
            plugin = (NativePlugin*)features[i]->data;
            break;
        }
    }

    if (plugin == nullptr)
    {
        carla_stderr("Host doesn't support instance-access, cannot show UI");
        return nullptr;
    }

    plugin->lv2ui_instantiate(writeFunction, controller, widget, features);

    return (LV2UI_Handle)plugin;
}

#define uiPtr ((NativePlugin*)ui)

static void lv2ui_port_event(LV2UI_Handle ui, uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
{
    carla_debug("lv2ui_port_event(%p, %i, %i, %i, %p)", ui, portIndex, bufferSize, format, buffer);
    uiPtr->lv2ui_port_event(portIndex, bufferSize, format, buffer);
}

static void lv2ui_cleanup(LV2UI_Handle ui)
{
    carla_debug("lv2ui_cleanup(%p)", ui);
    uiPtr->lv2ui_cleanup();
}

#undef uiPtr

// -----------------------------------------------------------------------
// Startup code

CARLA_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    carla_debug("lv2_descriptor(%i)", index);

    if (index >= sPluginDescsMgr.descs.count())
    {
        carla_debug("lv2_descriptor(%i) - out of bounds", index);
        return nullptr;
    }
    if (index < sPluginDescsMgr.lv2Descs.count())
    {
        carla_debug("lv2_descriptor(%i) - found previously allocated", index);
        return sPluginDescsMgr.lv2Descs.getAt(index);
    }

    const PluginDescriptor*& pluginDesc(sPluginDescsMgr.descs.getAt(index));

    CarlaString tmpURI;

    if (std::strcmp(pluginDesc->label, "carla") == 0)
    {
        tmpURI = "http://kxstudio.sf.net/carla";
    }
    else
    {
        tmpURI  = "http://kxstudio.sf.net/carla/plugins/";
        tmpURI += pluginDesc->label;
    }

    carla_debug("lv2_descriptor(%i) - not found, allocating new with uri: %s", index, (const char*)tmpURI);

    const LV2_Descriptor* const lv2Desc(new const LV2_Descriptor{
    /* URI            */ carla_strdup(tmpURI),
    /* instantiate    */ lv2_instantiate,
    /* connect_port   */ lv2_connect_port,
    /* activate       */ lv2_activate,
    /* run            */ lv2_run,
    /* deactivate     */ lv2_deactivate,
    /* cleanup        */ lv2_cleanup,
    /* extension_data */ lv2_extension_data
    });

    sPluginDescsMgr.lv2Descs.append(lv2Desc);

    return sPluginDescsMgr.lv2Descs.getLast();
}

CARLA_EXPORT const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    carla_debug("lv2ui_descriptor(%i)", index);

    static const LV2UI_Descriptor lv2UiDesc = {
    /* URI            */ "http://kxstudio.sf.net/carla#UI",
    /* instantiate    */ lv2ui_instantiate,
    /* cleanup        */ lv2ui_cleanup,
    /* port_event     */ lv2ui_port_event,
    /* extension_data */ nullptr
    };

    static const LV2UI_Descriptor lv2UiDescOld = {
    /* URI            */ "http://kxstudio.sf.net/carla#UIold",
    /* instantiate    */ lv2ui_instantiate,
    /* cleanup        */ lv2ui_cleanup,
    /* port_event     */ lv2ui_port_event,
    /* extension_data */ nullptr
    };

    switch (index)
    {
    case 0:
        return &lv2UiDesc;
    case 1:
        return &lv2UiDescOld;
    default:
        return nullptr;
    }
}

// -----------------------------------------------------------------------
