/*
 * Carla Native Plugins
 * Copyright (C) 2013-2018 Filipe Coelho <falktx@falktx.com>
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

#define CARLA_NATIVE_PLUGIN_LV2
#include "carla-base.cpp"

#include "CarlaLv2Utils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaString.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Carla Internal Plugin API exposed as LV2 plugin

class NativePlugin : public Lv2PluginBaseClass<NativeTimeInfo>
{
public:
    static const uint32_t kMaxMidiEvents = 512;

    NativePlugin(const NativePluginDescriptor* const desc,
                 const double sampleRate,
                 const char* const bundlePath,
                 const LV2_Feature* const* const features)
        : Lv2PluginBaseClass(sampleRate, features),
          fHandle(nullptr),
          fHost(),
          fDescriptor(desc),
#ifdef CARLA_PROPER_CPP11_SUPPORT
          fProgramDesc({0, 0, nullptr}),
#endif
          fMidiEventCount(0),
          fWorkerUISignal(0)
    {
        carla_zeroStruct(fHost);

        if (! loadedInProperHost())
            return;

        CarlaString resourceDir(bundlePath);
        resourceDir += CARLA_OS_SEP_STR "resources" CARLA_OS_SEP_STR;

        fHost.handle      = this;
        fHost.resourceDir = resourceDir.dupSafe();
        fHost.uiName      = nullptr;
        fHost.uiParentId  = 0;

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
    }

    ~NativePlugin()
    {
        CARLA_SAFE_ASSERT(fHandle == nullptr);

        if (fHost.resourceDir != nullptr)
        {
            delete[] fHost.resourceDir;
            fHost.resourceDir = nullptr;
        }

        if (fHost.uiName != nullptr)
        {
            delete[] fHost.uiName;
            fHost.uiName = nullptr;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    bool init()
    {
        if (fHost.resourceDir == nullptr)
            return false;

        if (fDescriptor->instantiate == nullptr || fDescriptor->process == nullptr)
        {
            carla_stderr("Plugin is missing something...");
            return false;
        }

        carla_zeroStructs(fMidiEvents, kMaxMidiEvents);

        fHandle = fDescriptor->instantiate(&fHost);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, false);

        fPorts.usesTime     = fDescriptor->hints & NATIVE_PLUGIN_USES_TIME;
        fPorts.numAudioIns  = fDescriptor->audioIns;
        fPorts.numAudioOuts = fDescriptor->audioOuts;
        fPorts.numMidiIns   = std::max(fDescriptor->midiIns, 1U);
        fPorts.numMidiOuts  = std::max(fDescriptor->midiOuts, 1U);

        if (fDescriptor->get_parameter_count != nullptr &&
            fDescriptor->get_parameter_info  != nullptr &&
            fDescriptor->get_parameter_value != nullptr &&
            fDescriptor->set_parameter_value != nullptr)
        {
            fPorts.numParams = fDescriptor->get_parameter_count(fHandle);
        }

        fPorts.init();

        if (fPorts.numParams > 0)
        {
            for (uint32_t i=0; i < fPorts.numParams; ++i)
            {
                fPorts.paramsLast[i] = fDescriptor->get_parameter_value(fHandle, i);
                fPorts.paramsOut [i] = fDescriptor->get_parameter_info(fHandle, i)->hints & NATIVE_PARAMETER_IS_OUTPUT;
            }
        }

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // LV2 functions

    void lv2_activate()
    {
        CARLA_SAFE_ASSERT_RETURN(! fIsActive,);

        resetTimeInfo();

        if (fDescriptor->activate != nullptr)
            fDescriptor->activate(fHandle);

        fIsActive = true;
    }

    void lv2_deactivate()
    {
        CARLA_SAFE_ASSERT_RETURN(fIsActive,);

        fIsActive = false;

        if (fDescriptor->deactivate != nullptr)
            fDescriptor->deactivate(fHandle);
    }

    void lv2_cleanup()
    {
        if (fIsActive)
        {
            carla_stderr("Warning: Host forgot to call deactivate!");
            fIsActive = false;

            if (fDescriptor->deactivate != nullptr)
                fDescriptor->deactivate(fHandle);
        }

        if (fDescriptor->cleanup != nullptr)
            fDescriptor->cleanup(fHandle);

        fHandle = nullptr;
    }

    // ----------------------------------------------------------------------------------------------------------------

    void lv2_run(const uint32_t frames)
    {
        if (! lv2_pre_run(frames))
        {
            updateParameterOutputs();
            return;
        }

        if (fPorts.numMidiIns > 0)
        {
            fMidiEventCount = 0;
            carla_zeroStructs(fMidiEvents, kMaxMidiEvents);

            for (uint32_t i=0; i < fPorts.numMidiIns; ++i)
            {
                const LV2_Atom_Sequence* const eventsIn(fPorts.eventsIn[i]);
                CARLA_SAFE_ASSERT_CONTINUE(eventsIn != nullptr);

                LV2_ATOM_SEQUENCE_FOREACH(eventsIn, event)
                {
                    if (event == nullptr)
                        continue;

                    if (event->body.type == fURIs.uiEvents && fWorkerUISignal != -1)
                    {
                        fWorkerUISignal = 1;
                        const char* const msg((const char*)(event + 1));
                        const size_t msgSize = std::strlen(msg);
                        //std::puts(msg);
                        fWorker->schedule_work(fWorker->handle, msgSize+1, msg);
                        continue;
                    }

                    if (event->body.type != fURIs.midiEvent)
                        continue;
                    if (event->body.size > 4)
                        continue;
                    if (event->time.frames >= frames)
                        break;

                    const uint8_t* const data((const uint8_t*)(event + 1));

                    NativeMidiEvent& nativeEvent(fMidiEvents[fMidiEventCount++]);

                    nativeEvent.port = (uint8_t)i;
                    nativeEvent.size = (uint8_t)event->body.size;
                    nativeEvent.time = (uint32_t)event->time.frames;

                    uint32_t j=0;
                    for (uint32_t size=event->body.size; j<size; ++j)
                        nativeEvent.data[j] = data[j];
                    for (; j<4; ++j)
                        nativeEvent.data[j] = 0;

                    if (fMidiEventCount >= kMaxMidiEvents)
                        break;
                }
            }
        }

        // FIXME
        fDescriptor->process(fHandle,
                             const_cast<float**>(fPorts.audioIns), fPorts.audioOuts, frames,
                             fMidiEvents, fMidiEventCount);

        if (fWorkerUISignal == -1 && fPorts.numMidiOuts > 0)
        {
            const char* const msg = "quit";
            const size_t msgSize  = 5;

            LV2_Atom_Sequence* const seq(fPorts.midiOuts[0]);
            Ports::MidiOutData& mData(fPorts.midiOutData[0]);

            if (sizeof(LV2_Atom_Event) + msgSize <= mData.capacity - mData.offset)
            {
                LV2_Atom_Event* const aev = (LV2_Atom_Event*)(LV2_ATOM_CONTENTS(LV2_Atom_Sequence, seq) + mData.offset);

                aev->time.frames = 0;
                aev->body.size   = msgSize;
                aev->body.type   = fURIs.uiEvents;
                std::memcpy(LV2_ATOM_BODY(&aev->body), msg, msgSize);

                const uint32_t size = lv2_atom_pad_size(static_cast<uint32_t>(sizeof(LV2_Atom_Event) + msgSize));
                mData.offset       += size;
                seq->atom.size     += size;

                fWorkerUISignal = 0;
            }
        }

        lv2_post_run(frames);
        updateParameterOutputs();
    }

    // ----------------------------------------------------------------------------------------------------------------

    const LV2_Program_Descriptor* lv2_get_program(const uint32_t index)
    {
        if (fDescriptor->category == NATIVE_PLUGIN_CATEGORY_SYNTH)
            return nullptr;
        if (fDescriptor->get_midi_program_count == nullptr)
            return nullptr;
        if (fDescriptor->get_midi_program_info == nullptr)
            return nullptr;
        if (index >= fDescriptor->get_midi_program_count(fHandle))
            return nullptr;

        const NativeMidiProgram* const midiProg(fDescriptor->get_midi_program_info(fHandle, index));

        if (midiProg == nullptr)
            return nullptr;

        fProgramDesc.bank    = midiProg->bank;
        fProgramDesc.program = midiProg->program;
        fProgramDesc.name    = midiProg->name;

        return &fProgramDesc;
    }

    void lv2_select_program(uint32_t bank, uint32_t program)
    {
        if (fDescriptor->category == NATIVE_PLUGIN_CATEGORY_SYNTH)
            return;
        if (fDescriptor->set_midi_program == nullptr)
            return;

        fDescriptor->set_midi_program(fHandle, 0, bank, program);

        for (uint32_t i=0; i < fPorts.numParams; ++i)
        {
            fPorts.paramsLast[i] = fDescriptor->get_parameter_value(fHandle, i);

            if (fPorts.paramsPtr[i] != nullptr)
                *fPorts.paramsPtr[i] = fPorts.paramsLast[i];
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    LV2_State_Status lv2_save(const LV2_State_Store_Function store, const LV2_State_Handle handle,
                              const uint32_t /*flags*/, const LV2_Feature* const* const /*features*/) const
    {
        if ((fDescriptor->hints & NATIVE_PLUGIN_USES_STATE) == 0 || fDescriptor->get_state == nullptr)
            return LV2_STATE_ERR_NO_FEATURE;

        if (char* const state = fDescriptor->get_state(fHandle))
        {
            store(handle, fUridMap->map(fUridMap->handle, "http://kxstudio.sf.net/ns/carla/chunk"), state, std::strlen(state)+1, fURIs.atomString, LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE);
            std::free(state);
            return LV2_STATE_SUCCESS;
        }

        return LV2_STATE_ERR_UNKNOWN;
    }

    LV2_State_Status lv2_restore(const LV2_State_Retrieve_Function retrieve, const LV2_State_Handle handle,
                                 uint32_t flags, const LV2_Feature* const* const /*features*/) const
    {
        if ((fDescriptor->hints & NATIVE_PLUGIN_USES_STATE) == 0 || fDescriptor->set_state == nullptr)
            return LV2_STATE_ERR_NO_FEATURE;

        size_t   size = 0;
        uint32_t type = 0;
        const void* const data = retrieve(handle, fUridMap->map(fUridMap->handle, "http://kxstudio.sf.net/ns/carla/chunk"), &size, &type, &flags);

        if (size == 0)
            return LV2_STATE_ERR_UNKNOWN;
        if (type == 0)
            return LV2_STATE_ERR_UNKNOWN;
        if (data == nullptr)
            return LV2_STATE_ERR_UNKNOWN;
        if (type != fURIs.atomString)
            return LV2_STATE_ERR_BAD_TYPE;

        fDescriptor->set_state(fHandle, (const char*)data);

        return LV2_STATE_SUCCESS;
    }

    // ----------------------------------------------------------------------------------------------------------------

    LV2_Worker_Status lv2_work(LV2_Worker_Respond_Function, LV2_Worker_Respond_Handle, uint32_t, const void* data)
    {
        const char* const msg = (const char*)data;

        /**/ if (std::strcmp(msg, "show") == 0)
        {
            handleUiShow();
        }
        else if (std::strcmp(msg, "hide") == 0)
        {
            handleUiHide();
        }
        else if (std::strcmp(msg, "idle") == 0)
        {
            handleUiRun();
        }
        else if (std::strcmp(msg, "quit") == 0)
        {
            handleUiRun();
        }
        else
        {
            carla_stdout("lv2_work unknown msg '%s'", msg);
            return LV2_WORKER_ERR_UNKNOWN;
        }

        return LV2_WORKER_SUCCESS;
    }

    LV2_Worker_Status lv2_work_resp(uint32_t /*size*/, const void* /*body*/)
    {
        return LV2_WORKER_SUCCESS;
    }

    // ----------------------------------------------------------------------------------------------------------------

    void lv2ui_instantiate(LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
                           LV2UI_Widget* widget, const LV2_Feature* const* features, const bool isEmbed)
    {
        fUI.writeFunction = writeFunction;
        fUI.controller = controller;
        fUI.isEmbed = isEmbed;

        if (fHost.uiName != nullptr)
        {
            delete[] fHost.uiName;
            fHost.uiName = nullptr;
        }

#ifdef CARLA_OS_LINUX
        // ---------------------------------------------------------------
        // show embed UI if needed

        if (isEmbed)
        {
            intptr_t parentId = 0;
            const LV2UI_Resize* uiResize = nullptr;

            for (int i=0; features[i] != nullptr; ++i)
            {
                if (std::strcmp(features[i]->URI, LV2_UI__parent) == 0)
                {
                    parentId = (intptr_t)features[i]->data;
                }
                else if (std::strcmp(features[i]->URI, LV2_UI__resize) == 0)
                {
                    uiResize = (const LV2UI_Resize*)features[i]->data;
                }
            }

            // -----------------------------------------------------------
            // see if the host can really embed the UI

            if (parentId != 0)
            {
                // wait for remote side to be ready
                fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_NULL, (int32_t)0xDEADF00D, 0xC0C0B00B, nullptr, 0.0f);

                if (uiResize && uiResize->ui_resize != nullptr)
                    uiResize->ui_resize(uiResize->handle, 740, 512);

                fHost.uiName = carla_strdup(fDescriptor->name);
                fUI.isVisible = true;

                char strBuf[0xff+1];
                strBuf[0xff] = '\0';
                std::snprintf(strBuf, 0xff, P_INTPTR, parentId);

                carla_setenv("CARLA_PLUGIN_EMBED_WINID", strBuf);

                fDescriptor->ui_show(fHandle, true);

                carla_setenv("CARLA_PLUGIN_EMBED_WINID", "0");

                const intptr_t winId(fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_NULL, (int32_t)0xDEADF00D, 0xC0C0B00B, nullptr, 0.0f));
                CARLA_SAFE_ASSERT_RETURN(winId != 0,);

                *widget = (LV2UI_Widget)winId;
                return;
            }
        }
#endif

        // ---------------------------------------------------------------
        // see if the host supports external-ui

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
        {
            fHost.uiName = carla_strdup(fUI.host->plugin_human_id);
            *widget = (LV2_External_UI_Widget_Compat*)this;
            return;
        }

        // ---------------------------------------------------------------
        // no external-ui support, use showInterface

        for (int i=0; features[i] != nullptr; ++i)
        {
            if (std::strcmp(features[i]->URI, LV2_OPTIONS__options) != 0)
                continue;

            const LV2_Options_Option* const options((const LV2_Options_Option*)features[i]->data);
            CARLA_SAFE_ASSERT_BREAK(options != nullptr);

            for (int j=0; options[j].key != 0; ++j)
            {
                if (options[j].key != fUridMap->map(fUridMap->handle, LV2_UI__windowTitle))
                    continue;

                const char* const title((const char*)options[j].value);
                CARLA_SAFE_ASSERT_BREAK(title != nullptr && title[0] != '\0');

                fHost.uiName = carla_strdup(title);
                break;
            }
            break;
        }

        if (fHost.uiName == nullptr)
            fHost.uiName = carla_strdup(fDescriptor->name);

        *widget = nullptr;
    }

    void lv2ui_port_event(uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer) const
    {
        if (format != 0 || bufferSize != sizeof(float) || buffer == nullptr)
            return;
        if (portIndex >= fPorts.indexOffset || ! fUI.isVisible)
            return;
        if (fDescriptor->ui_set_parameter_value == nullptr)
            return;

        const float value(*(const float*)buffer);
        fDescriptor->ui_set_parameter_value(fHandle, portIndex-fPorts.indexOffset, value);
    }

    // ----------------------------------------------------------------------------------------------------------------

    void lv2ui_select_program(uint32_t bank, uint32_t program) const
    {
        if (fDescriptor->category == NATIVE_PLUGIN_CATEGORY_SYNTH)
            return;
        if (fDescriptor->ui_set_midi_program == nullptr)
            return;

        fDescriptor->ui_set_midi_program(fHandle, 0, bank, program);
    }

    // ----------------------------------------------------------------------------------------------------------------

protected:
    void handleUiRun() const override
    {
        if (fDescriptor->ui_idle != nullptr)
            fDescriptor->ui_idle(fHandle);
    }

    void handleUiShow() override
    {
        if (fDescriptor->ui_show != nullptr)
            fDescriptor->ui_show(fHandle, true);

        fUI.isVisible = true;
    }

    void handleUiHide() override
    {
        if (fDescriptor->ui_show != nullptr)
            fDescriptor->ui_show(fHandle, false);

        fUI.isVisible = false;
    }

    // ----------------------------------------------------------------------------------------------------------------

    void handleParameterValueChanged(const uint32_t index, const float value) override
    {
        fDescriptor->set_parameter_value(fHandle, index, value);
    }

    void handleBufferSizeChanged(const uint32_t bufferSize) override
    {
        if (fDescriptor->dispatcher == nullptr)
            return;

        fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_BUFFER_SIZE_CHANGED, 0, bufferSize, nullptr, 0.0f);
    }

    void handleSampleRateChanged(const double sampleRate) override
    {
        if (fDescriptor->dispatcher == nullptr)
            return;

        fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_SAMPLE_RATE_CHANGED, 0, 0, nullptr, (float)sampleRate);
    }

    // ----------------------------------------------------------------------------------------------------------------

    bool handleWriteMidiEvent(const NativeMidiEvent* const event)
    {
        CARLA_SAFE_ASSERT_RETURN(fPorts.numMidiOuts > 0, false);
        CARLA_SAFE_ASSERT_RETURN(event != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(event->size > 0, false);

        const uint8_t port(event->port);
        CARLA_SAFE_ASSERT_RETURN(port < fPorts.numMidiOuts, false);

        LV2_Atom_Sequence* const seq(fPorts.midiOuts[port]);
        CARLA_SAFE_ASSERT_RETURN(seq != nullptr, false);

        Ports::MidiOutData& mData(fPorts.midiOutData[port]);

        if (sizeof(LV2_Atom_Event) + event->size > mData.capacity - mData.offset)
            return false;

        LV2_Atom_Event* const aev = (LV2_Atom_Event*)(LV2_ATOM_CONTENTS(LV2_Atom_Sequence, seq) + mData.offset);

        aev->time.frames = event->time;
        aev->body.size   = event->size;
        aev->body.type   = fURIs.midiEvent;
        std::memcpy(LV2_ATOM_BODY(&aev->body), event->data, event->size);

        const uint32_t size = lv2_atom_pad_size(static_cast<uint32_t>(sizeof(LV2_Atom_Event) + event->size));
        mData.offset       += size;
        seq->atom.size     += size;

        return true;
    }

    void handleUiParameterChanged(const uint32_t index, const float value) const
    {
        if (fWorkerUISignal)
        {
        }
        else if (fUI.writeFunction != nullptr && fUI.controller != nullptr)
                 fUI.writeFunction(fUI.controller, index+fPorts.indexOffset, sizeof(float), 0, &value);
    }

    void handleUiCustomDataChanged(const char* const /*key*/, const char* const /*value*/) const
    {
        //storeCustomData(key, value);
    }

    void handleUiClosed()
    {
        fUI.isVisible = false;

        if (fWorkerUISignal)
            fWorkerUISignal = -1;

        if (fUI.host != nullptr && fUI.host->ui_closed != nullptr && fUI.controller != nullptr)
            fUI.host->ui_closed(fUI.controller);

        fUI.host = nullptr;
        fUI.writeFunction = nullptr;
        fUI.controller = nullptr;
    }

    const char* handleUiOpenFile(const bool /*isDir*/, const char* const /*title*/, const char* const /*filter*/) const
    {
        // TODO
        return nullptr;
    }

    const char* handleUiSaveFile(const bool /*isDir*/, const char* const /*title*/, const char* const /*filter*/) const
    {
        // TODO
        return nullptr;
    }

    intptr_t handleDispatcher(const NativeHostDispatcherOpcode opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
        carla_debug("NativePlugin::handleDispatcher(%i, %i, " P_INTPTR ", %p, %f)", opcode, index, value, ptr, opt);

        intptr_t ret = 0;

        switch (opcode)
        {
        case NATIVE_HOST_OPCODE_NULL:
        case NATIVE_HOST_OPCODE_UPDATE_PARAMETER:
        case NATIVE_HOST_OPCODE_UPDATE_MIDI_PROGRAM:
        case NATIVE_HOST_OPCODE_RELOAD_PARAMETERS:
        case NATIVE_HOST_OPCODE_RELOAD_MIDI_PROGRAMS:
        case NATIVE_HOST_OPCODE_RELOAD_ALL:
        case NATIVE_HOST_OPCODE_HOST_IDLE:
        case NATIVE_HOST_OPCODE_INTERNAL_PLUGIN:
            // nothing
            break;
        case NATIVE_HOST_OPCODE_UI_UNAVAILABLE:
            handleUiClosed();
            break;
        }

        return ret;

        // unused for now
        (void)index;
        (void)value;
        (void)ptr;
        (void)opt;
    }

    void updateParameterOutputs()
    {
        float value;

        for (uint32_t i=0; i < fPorts.numParams; ++i)
        {
            if (! fPorts.paramsOut[i])
                continue;

            fPorts.paramsLast[i] = value = fDescriptor->get_parameter_value(fHandle, i);

            if (fPorts.paramsPtr[i] != nullptr)
                *fPorts.paramsPtr[i] = value;
        }
    }

    // -------------------------------------------------------------------

private:
    // Native data
    NativePluginHandle   fHandle;
    NativeHostDescriptor fHost;
    const NativePluginDescriptor* const fDescriptor;
    LV2_Program_Descriptor              fProgramDesc;

    uint32_t        fMidiEventCount;
    NativeMidiEvent fMidiEvents[kMaxMidiEvents];

    int fWorkerUISignal;

    // -------------------------------------------------------------------

    #define handlePtr ((NativePlugin*)handle)

    static uint32_t host_get_buffer_size(NativeHostHandle handle)
    {
        return handlePtr->fBufferSize;
    }

    static double host_get_sample_rate(NativeHostHandle handle)
    {
        return handlePtr->fSampleRate;
    }

    static bool host_is_offline(NativeHostHandle handle)
    {
        return handlePtr->fIsOffline;
    }

    static const NativeTimeInfo* host_get_time_info(NativeHostHandle handle)
    {
        return &(handlePtr->fTimeInfo);
    }

    static bool host_write_midi_event(NativeHostHandle handle, const NativeMidiEvent* event)
    {
        return handlePtr->handleWriteMidiEvent(event);
    }

    static void host_ui_parameter_changed(NativeHostHandle handle, uint32_t index, float value)
    {
        handlePtr->handleUiParameterChanged(index, value);
    }

    static void host_ui_custom_data_changed(NativeHostHandle handle, const char* key, const char* value)
    {
        handlePtr->handleUiCustomDataChanged(key, value);
    }

    static void host_ui_closed(NativeHostHandle handle)
    {
        handlePtr->handleUiClosed();
    }

    static const char* host_ui_open_file(NativeHostHandle handle, bool isDir, const char* title, const char* filter)
    {
        return handlePtr->handleUiOpenFile(isDir, title, filter);
    }

    static const char* host_ui_save_file(NativeHostHandle handle, bool isDir, const char* title, const char* filter)
    {
        return handlePtr->handleUiSaveFile(isDir, title, filter);
    }

    static intptr_t host_dispatcher(NativeHostHandle handle, NativeHostDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        return handlePtr->handleDispatcher(opcode, index, value, ptr, opt);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativePlugin)
};

// -----------------------------------------------------------------------
// LV2 plugin descriptor functions

static LV2_Handle lv2_instantiate(const LV2_Descriptor* lv2Descriptor, double sampleRate, const char* bundlePath, const LV2_Feature* const* features)
{
    carla_debug("lv2_instantiate(%p, %g, %s, %p)", lv2Descriptor, sampleRate, bundlePath, features);

    const NativePluginDescriptor* pluginDesc  = nullptr;
    const char*                   pluginLabel = nullptr;

    if (std::strncmp(lv2Descriptor->URI, "http://kxstudio.sf.net/carla/plugins/", 37) == 0)
        pluginLabel = lv2Descriptor->URI+37;

    if (pluginLabel == nullptr)
    {
        carla_stderr("Failed to find carla native plugin with URI \"%s\"", lv2Descriptor->URI);
        return nullptr;
    }

    carla_debug("lv2_instantiate() - looking up label \"%s\"", pluginLabel);

    PluginListManager& plm(PluginListManager::getInstance());

    for (LinkedList<const NativePluginDescriptor*>::Itenerator it = plm.descs.begin2(); it.valid(); it.next())
    {
        const NativePluginDescriptor* const& tmpDesc(it.getValue(nullptr));
        CARLA_SAFE_ASSERT_CONTINUE(tmpDesc != nullptr);

        if (std::strcmp(tmpDesc->label, pluginLabel) == 0)
        {
            pluginDesc = tmpDesc;
            break;
        }
    }

    if (pluginDesc == nullptr)
    {
        carla_stderr("Failed to find carla native plugin with label \"%s\"", pluginLabel);
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

static uint32_t lv2_get_options(LV2_Handle instance, LV2_Options_Option* options)
{
    carla_debug("lv2_get_options(%p, %p)", instance, options);
    return instancePtr->lv2_get_options(options);
}

static uint32_t lv2_set_options(LV2_Handle instance, const LV2_Options_Option* options)
{
    carla_debug("lv2_set_options(%p, %p)", instance, options);
    return instancePtr->lv2_set_options(options);
}

static const LV2_Program_Descriptor* lv2_get_program(LV2_Handle instance, uint32_t index)
{
    carla_debug("lv2_get_program(%p, %i)", instance, index);
    return instancePtr->lv2_get_program(index);
}

static void lv2_select_program(LV2_Handle instance, uint32_t bank, uint32_t program)
{
    carla_debug("lv2_select_program(%p, %i, %i)", instance, bank, program);
    return instancePtr->lv2_select_program(bank, program);
}

static LV2_State_Status lv2_save(LV2_Handle instance, LV2_State_Store_Function store, LV2_State_Handle handle, uint32_t flags, const LV2_Feature* const* features)
{
    carla_debug("lv2_save(%p, %p, %p, %i, %p)", instance, store, handle, flags, features);
    return instancePtr->lv2_save(store, handle, flags, features);
}

static LV2_State_Status lv2_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle handle, uint32_t flags, const LV2_Feature* const* features)
{
    carla_debug("lv2_restore(%p, %p, %p, %i, %p)", instance, retrieve, handle, flags, features);
    return instancePtr->lv2_restore(retrieve, handle, flags, features);
}

static LV2_Worker_Status lv2_work(LV2_Handle instance, LV2_Worker_Respond_Function respond, LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
{
    carla_debug("work(%p, %p, %p, %u, %p)", instance, respond, handle, size, data);
    return instancePtr->lv2_work(respond, handle, size, data);
}

LV2_Worker_Status lv2_work_resp(LV2_Handle instance, uint32_t size, const void* body)
{
    carla_debug("work_resp(%p, %u, %p)", instance, size, body);
    return instancePtr->lv2_work_resp(size, body);
}

static const void* lv2_extension_data(const char* uri)
{
    carla_debug("lv2_extension_data(\"%s\")", uri);

    static const LV2_Options_Interface  options  = { lv2_get_options, lv2_set_options };
    static const LV2_Programs_Interface programs = { lv2_get_program, lv2_select_program };
    static const LV2_State_Interface    state    = { lv2_save, lv2_restore };
    static const LV2_Worker_Interface   worker   = { lv2_work, lv2_work_resp, nullptr };

    if (std::strcmp(uri, LV2_OPTIONS__interface) == 0)
        return &options;
    if (std::strcmp(uri, LV2_PROGRAMS__Interface) == 0)
        return &programs;
    if (std::strcmp(uri, LV2_STATE__interface) == 0)
        return &state;
    if (std::strcmp(uri, LV2_WORKER__interface) == 0)
        return &worker;

    return nullptr;
}

#undef instancePtr

// -----------------------------------------------------------------------
// LV2 UI descriptor functions

static LV2UI_Handle lv2ui_instantiate(LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
                                      LV2UI_Widget* widget, const LV2_Feature* const* features, const bool isEmbed)
{
    carla_debug("lv2ui_instantiate(..., %p, %p, %p)", writeFunction, controller, widget, features);
#ifndef CARLA_OS_LINUX
    CARLA_SAFE_ASSERT_RETURN(! isEmbed, nullptr);
#endif

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

    plugin->lv2ui_instantiate(writeFunction, controller, widget, features, isEmbed);

    return (LV2UI_Handle)plugin;
}

#ifdef CARLA_OS_LINUX
static LV2UI_Handle lv2ui_instantiate_embed(const LV2UI_Descriptor*, const char*, const char*,
                                            LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
                                            LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    return lv2ui_instantiate(writeFunction, controller, widget, features, true);
}
#endif

static LV2UI_Handle lv2ui_instantiate_external(const LV2UI_Descriptor*, const char*, const char*,
                                               LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
                                               LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    return lv2ui_instantiate(writeFunction, controller, widget, features, false);
}

#define uiPtr ((NativePlugin*)ui)

static void lv2ui_port_event(LV2UI_Handle ui, uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
{
    carla_debug("lv2ui_port_eventxx(%p, %i, %i, %i, %p)", ui, portIndex, bufferSize, format, buffer);
    uiPtr->lv2ui_port_event(portIndex, bufferSize, format, buffer);
}

static void lv2ui_cleanup(LV2UI_Handle ui)
{
    carla_debug("lv2ui_cleanup(%p)", ui);
    uiPtr->lv2ui_cleanup();
}

static void lv2ui_select_program(LV2UI_Handle ui, uint32_t bank, uint32_t program)
{
    carla_debug("lv2ui_select_program(%p, %i, %i)", ui, bank, program);
    uiPtr->lv2ui_select_program(bank, program);
}

static int lv2ui_idle(LV2UI_Handle ui)
{
    return uiPtr->lv2ui_idle();
}

static int lv2ui_show(LV2UI_Handle ui)
{
    carla_debug("lv2ui_show(%p)", ui);
    return uiPtr->lv2ui_show();
}

static int lv2ui_hide(LV2UI_Handle ui)
{
    carla_debug("lv2ui_hide(%p)", ui);
    return uiPtr->lv2ui_hide();
}

static const void* lv2ui_extension_data(const char* uri)
{
    carla_stdout("lv2ui_extension_data(\"%s\")", uri);

    static const LV2UI_Idle_Interface uiidle = { lv2ui_idle };
    static const LV2UI_Show_Interface uishow = { lv2ui_show, lv2ui_hide };
    static const LV2_Programs_UI_Interface uiprograms = { lv2ui_select_program };

    if (std::strcmp(uri, LV2_UI__idleInterface) == 0)
        return &uiidle;
    if (std::strcmp(uri, LV2_UI__showInterface) == 0)
        return &uishow;
    if (std::strcmp(uri, LV2_PROGRAMS__UIInterface) == 0)
        return &uiprograms;

    return nullptr;
}

#undef uiPtr

// -----------------------------------------------------------------------
// Startup code

CARLA_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    carla_debug("lv2_descriptor(%i)", index);

    PluginListManager& plm(PluginListManager::getInstance());

    if (index >= plm.descs.count())
    {
        carla_debug("lv2_descriptor(%i) - out of bounds", index);
        return nullptr;
    }
    if (index < plm.lv2Descs.count())
    {
        carla_debug("lv2_descriptor(%i) - found previously allocated", index);
        return plm.lv2Descs.getAt(index, nullptr);
    }

    const NativePluginDescriptor* const pluginDesc(plm.descs.getAt(index, nullptr));
    CARLA_SAFE_ASSERT_RETURN(pluginDesc != nullptr, nullptr);

    CarlaString tmpURI;
    tmpURI  = "http://kxstudio.sf.net/carla/plugins/";
    tmpURI += pluginDesc->label;

    carla_debug("lv2_descriptor(%i) - not found, allocating new with uri \"%s\"", index, (const char*)tmpURI);

    const LV2_Descriptor lv2DescTmp = {
    /* URI            */ carla_strdup(tmpURI),
    /* instantiate    */ lv2_instantiate,
    /* connect_port   */ lv2_connect_port,
    /* activate       */ lv2_activate,
    /* run            */ lv2_run,
    /* deactivate     */ lv2_deactivate,
    /* cleanup        */ lv2_cleanup,
    /* extension_data */ lv2_extension_data
    };

    LV2_Descriptor* lv2Desc;

    try {
        lv2Desc = new LV2_Descriptor;
    } CARLA_SAFE_EXCEPTION_RETURN("new LV2_Descriptor", nullptr);

    std::memcpy(lv2Desc, &lv2DescTmp, sizeof(LV2_Descriptor));

    plm.lv2Descs.append(lv2Desc);
    return lv2Desc;
}

CARLA_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    carla_debug("lv2ui_descriptor(%i)", index);

#ifdef CARLA_OS_LINUX
    static const LV2UI_Descriptor lv2UiEmbedDesc = {
    /* URI            */ "http://kxstudio.sf.net/carla/ui-embed",
    /* instantiate    */ lv2ui_instantiate_embed,
    /* cleanup        */ lv2ui_cleanup,
    /* port_event     */ lv2ui_port_event,
    /* extension_data */ lv2ui_extension_data
    };

    if (index == 0)
        return &lv2UiEmbedDesc;
    else
        --index;
#endif

    static const LV2UI_Descriptor lv2UiExtDesc = {
    /* URI            */ "http://kxstudio.sf.net/carla/ui-ext",
    /* instantiate    */ lv2ui_instantiate_external,
    /* cleanup        */ lv2ui_cleanup,
    /* port_event     */ lv2ui_port_event,
    /* extension_data */ lv2ui_extension_data
    };

    return (index == 0) ? &lv2UiExtDesc : nullptr;
}

// -----------------------------------------------------------------------
