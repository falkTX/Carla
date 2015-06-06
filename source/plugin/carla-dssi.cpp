/*
 * Carla Native Plugins
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#define CARLA_NATIVE_PLUGIN_DSSI
#include "carla-native-base.cpp"

#include "juce_audio_basics.h"
#include "juce_gui_basics.h"

#include "CarlaString.hpp"

#include "dssi/dssi.h"
#include "ladspa/ladspa.h"

using namespace juce;

// -----------------------------------------------------------------------
// Juce Message Thread

class JuceMessageThread : public Thread
{
public:
    JuceMessageThread()
      : Thread("JuceMessageThread"),
        fInitialised(false)
    {
        startThread(7);

        while (! fInitialised)
            sleep(1);
    }

    ~JuceMessageThread()
    {
        signalThreadShouldExit();
        JUCEApplication::quit();
        waitForThreadToExit(5000);
        clearSingletonInstance();
    }

    void run()
    {
        initialiseJuce_GUI();
        fInitialised = true;

        MessageManager::getInstance()->setCurrentThreadAsMessageThread();

        while ((! threadShouldExit()) && MessageManager::getInstance()->runDispatchLoopUntil(250))
        {}
    }

    juce_DeclareSingleton(JuceMessageThread, false);

private:
    bool fInitialised;
};

juce_ImplementSingleton(JuceMessageThread)

static Array<void*> gActivePlugins;

// -----------------------------------------------------------------------
// LV2 descriptor functions

class NativePlugin : public LV2_External_UI_Widget
{
public:
    static const uint32_t kMaxMidiEvents = 512;

    NativePlugin(const NativePluginDescriptor* const desc, const double sampleRate, const char* const bundlePath, const LV2_Feature* const* features)
        : fHandle(nullptr),
          fDescriptor(desc),
          fMidiEventCount(0),
          fUiWasShown(false),
          fIsProcessing(false),
          fVolume(1.0f),
          fDryWet(1.0f),
          fBufferSize(0),
          fSampleRate(sampleRate),
          fUridMap(nullptr)
    {
        run  = extui_run;
        show = extui_show;
        hide = extui_hide;

        CarlaString resourceDir(bundlePath);
#ifdef CARLA_OS_WIN
        resourceDir += "\\resources\\";
#else
        resourceDir += "/resources/";
#endif

        fHost.handle      = this;
        fHost.resourceDir = resourceDir.dup();
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

        const LV2_Options_Option* options   = nullptr;
        const LV2_URID_Map*       uridMap   = nullptr;
        const LV2_URID_Unmap*     uridUnmap = nullptr;

        for (int i=0; features[i] != nullptr; ++i)
        {
            if (std::strcmp(features[i]->URI, LV2_OPTIONS__options) == 0)
                options = (const LV2_Options_Option*)features[i]->data;
            else if (std::strcmp(features[i]->URI, LV2_URID__map) == 0)
                uridMap = (const LV2_URID_Map*)features[i]->data;
            else if (std::strcmp(features[i]->URI, LV2_URID__unmap) == 0)
                uridUnmap = (const LV2_URID_Unmap*)features[i]->data;
        }

        if (options == nullptr || uridMap == nullptr)
        {
            carla_stderr("Host doesn't provides option or urid-map features");
            return;
        }

        for (int i=0; options[i].key != 0; ++i)
        {
            if (uridUnmap != nullptr)
            {
                carla_debug("Host option %i:\"%s\"", i, uridUnmap->unmap(uridUnmap->handle, options[i].key));
            }

            if (options[i].key == uridMap->map(uridMap->handle, LV2_BUF_SIZE__maxBlockLength))
            {
                if (options[i].type == uridMap->map(uridMap->handle, LV2_ATOM__Int))
                {
                    fBufferSize = *(const int*)options[i].value;

                    if (fBufferSize == 0)
                        carla_stderr("Host provides maxBlockLength but has null value");
                }
                else
                    carla_stderr("Host provides maxBlockLength but has wrong value type");

                break;
            }
        }

        fUridMap = uridMap;

        if (fDescriptor->midiIns > 0)
            fUI.portOffset += desc->midiIns;
        else if (fDescriptor->hints & PLUGIN_USES_TIME)
            fUI.portOffset += 1;

        fUI.portOffset += desc->midiOuts;
        fUI.portOffset += 1; // freewheel
        fUI.portOffset += desc->audioIns;
        fUI.portOffset += desc->audioOuts;
    }

    ~NativePlugin()
    {
        CARLA_ASSERT(fHandle == nullptr);
        CARLA_ASSERT(! fUiWasShown);

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
            carla_stderr("Host is missing bufferSize feature");
            //return false;
            // as testing, continue for now
            fBufferSize = 1024;
        }

        fHandle = fDescriptor->instantiate(&fHost);

        if (fHandle == nullptr)
            return false;

        carla_zeroStructs(fMidiEvents, kMaxMidiEvents*2);
        carla_zeroStruct(fTimeInfo);

        fPorts.init(fDescriptor, fHandle);
        fUris.map(fUridMap);

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

        carla_zeroStruct(fTimeInfo);
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

        if (fUiWasShown)
        {
            CARLA_SAFE_ASSERT_RETURN(gActivePlugins.contains(this),);

            gActivePlugins.removeFirstMatchingValue(this);

            JUCE_AUTORELEASEPOOL
            {
                MessageManagerLock mmLock;

                if (gActivePlugins.size() == 0)
                {
                    JuceMessageThread::deleteInstance();
                    shutdownJuce_GUI();
                }
            }

            fUiWasShown = false;
        }
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

        if (fDescriptor->midiIns > 0 || (fDescriptor->hints & PLUGIN_USES_TIME) != 0)
        {
            fMidiEventCount = 0;
            carla_zeroStructs(fMidiEvents, kMaxMidiEvents*2);

            LV2_ATOM_SEQUENCE_FOREACH(fPorts.eventsIn[0], iter)
            {
                const LV2_Atom_Event* const event((const LV2_Atom_Event*)iter);

                if (event == nullptr)
                    continue;
                if (event->body.size > 4)
                    continue;
                if (event->time.frames >= frames)
                    break;

                if (event->body.type == fUris.midiEvent)
                {
                    if (fMidiEventCount >= kMaxMidiEvents*2)
                        continue;

                    const uint8_t* const data((const uint8_t*)(event + 1));

                    fMidiEvents[fMidiEventCount].port = 0;
                    fMidiEvents[fMidiEventCount].time = (uint32_t)event->time.frames;
                    fMidiEvents[fMidiEventCount].size = (uint8_t)event->body.size;

                    for (uint32_t i=0; i < event->body.size; ++i)
                        fMidiEvents[fMidiEventCount].data[i] = data[i];

                    fMidiEventCount += 1;
                    continue;
                }

                if (event->body.type == fUris.atomBlank)
                {
                    const LV2_Atom_Object* const obj((const LV2_Atom_Object*)&event->body);

                    if (obj->body.otype != fUris.timePos)
                        continue;

                    LV2_Atom* bar = nullptr;
                    LV2_Atom* barBeat = nullptr;
                    LV2_Atom* beatsPerBar = nullptr;
                    LV2_Atom* bpm = nullptr;
                    LV2_Atom* beatUnit = nullptr;
                    LV2_Atom* frame = nullptr;
                    LV2_Atom* speed = nullptr;

                    lv2_atom_object_get(obj,
                                        fUris.timeBar, &bar,
                                        fUris.timeBarBeat, &barBeat,
                                        fUris.timeBeatsPerBar, &beatsPerBar,
                                        fUris.timeBeatsPerMinute, &bpm,
                                        fUris.timeBeatUnit, &beatUnit,
                                        fUris.timeFrame, &frame,
                                        fUris.timeSpeed, &speed,
                                        nullptr);

                    if (bpm != nullptr && bpm->type == fUris.atomFloat)
                    {
                        fTimeInfo.bbt.beatsPerMinute = ((LV2_Atom_Float*)bpm)->body;
                        fTimeInfo.bbt.valid = true;
                    }

                    if (beatsPerBar != nullptr && beatsPerBar->type == fUris.atomFloat)
                    {
                        float beatsPerBarValue = ((LV2_Atom_Float*)beatsPerBar)->body;
                        fTimeInfo.bbt.beatsPerBar = beatsPerBarValue;

                        if (bar != nullptr && bar->type == fUris.atomLong)
                        {
                            //float barValue = ((LV2_Atom_Long*)bar)->body;
                            //curPosInfo.ppqPositionOfLastBarStart = barValue * beatsPerBarValue;

                            if (barBeat != nullptr && barBeat->type == fUris.atomFloat)
                            {
                                //float barBeatValue = ((LV2_Atom_Float*)barBeat)->body;
                                //curPosInfo.ppqPosition = curPosInfo.ppqPositionOfLastBarStart + barBeatValue;
                            }
                        }
                    }

                    if (beatUnit != nullptr && beatUnit->type == fUris.atomFloat)
                        fTimeInfo.bbt.beatType = ((LV2_Atom_Float*)beatUnit)->body;

                    if (frame != nullptr && frame->type == fUris.atomLong)
                        fTimeInfo.frame = ((LV2_Atom_Long*)frame)->body;

                    if (speed != nullptr && speed->type == fUris.atomFloat)
                        fTimeInfo.playing = ((LV2_Atom_Float*)speed)->body == 1.0f;

                    continue;
                }
            }

            for (uint32_t i=1; i < fDescriptor->midiIns; ++i)
            {
                LV2_ATOM_SEQUENCE_FOREACH(fPorts.eventsIn[i], iter)
                {
                    const LV2_Atom_Event* const event((const LV2_Atom_Event*)iter);

                    if (event == nullptr)
                        continue;
                    if (event->body.type != fUris.midiEvent)
                        continue;
                    if (event->body.size > 4)
                        continue;
                    if (event->time.frames >= frames)
                        break;
                    if (fMidiEventCount >= kMaxMidiEvents*2)
                        break;

                    const uint8_t* const data((const uint8_t*)(event + 1));

                    fMidiEvents[fMidiEventCount].port = (uint8_t)i;
                    fMidiEvents[fMidiEventCount].size = (uint8_t)event->body.size;
                    fMidiEvents[fMidiEventCount].time = (uint32_t)event->time.frames;

                    for (uint32_t j=0; j < event->body.size; ++j)
                        fMidiEvents[fMidiEventCount].data[j] = data[j];

                    fMidiEventCount += 1;
                }
            }
        }

        fIsProcessing = true;
        fDescriptor->process(fHandle, fPorts.audioIns, fPorts.audioOuts, frames, fMidiEvents, fMidiEventCount);
        fIsProcessing = false;

        if (fDryWet != 1.0f && fDescriptor->audioIns == fDescriptor->audioOuts)
        {
            for (uint32_t i=0; i < fDescriptor->audioOuts; ++i)
            {
                FloatVectorOperations::multiply(fPorts.audioIns[i], fVolume*(1.0f-fDryWet), frames);
                FloatVectorOperations::multiply(fPorts.audioOuts[i], fVolume*fDryWet, frames);
                FloatVectorOperations::add(fPorts.audioOuts[i], fPorts.audioIns[i], frames);
            }
        }
        else if (fVolume != 1.0f)
        {
            for (uint32_t i=0; i < fDescriptor->audioOuts; ++i)
                FloatVectorOperations::multiply(fPorts.audioOuts[i], fVolume, frames);
        }

        // TODO - midi out

        updateParameterOutputs();
    }

    // -------------------------------------------------------------------

    uint32_t lv2_get_options(LV2_Options_Option* const /*options*/) const
    {
        // currently unused
        return LV2_OPTIONS_SUCCESS;
    }

    uint32_t lv2_set_options(const LV2_Options_Option* const options)
    {
        for (int i=0; options[i].key != 0; ++i)
        {
            if (options[i].key == fUridMap->map(fUridMap->handle, LV2_BUF_SIZE__maxBlockLength))
            {
                if (options[i].type == fUridMap->map(fUridMap->handle, LV2_ATOM__Int))
                {
                    fBufferSize = *(const int*)options[i].value;

                    if (fDescriptor->dispatcher != nullptr)
                        fDescriptor->dispatcher(fHandle, PLUGIN_OPCODE_BUFFER_SIZE_CHANGED, 0, fBufferSize, nullptr, 0.0f);
                }
                else
                    carla_stderr("Host changed maxBlockLength but with wrong value type");
            }
            else if (options[i].key == fUridMap->map(fUridMap->handle, LV2_CORE__sampleRate))
            {
                if (options[i].type == fUridMap->map(fUridMap->handle, LV2_ATOM__Double))
                {
                    fSampleRate = *(const double*)options[i].value;

                    if (fDescriptor->dispatcher != nullptr)
                        fDescriptor->dispatcher(fHandle, PLUGIN_OPCODE_SAMPLE_RATE_CHANGED, 0, 0, nullptr, (float)fSampleRate);
                }
                else
                    carla_stderr("Host changed sampleRate but with wrong value type");
            }
        }

        return LV2_OPTIONS_SUCCESS;
    }

    const LV2_Program_Descriptor* lv2_get_program(const uint32_t index)
    {
        if (fDescriptor->category == PLUGIN_CATEGORY_SYNTH)
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
        if (fDescriptor->category == PLUGIN_CATEGORY_SYNTH)
            return;
        if (fDescriptor->set_midi_program == nullptr)
            return;

        fDescriptor->set_midi_program(fHandle, 0, bank, program);
    }

    LV2_State_Status lv2_save(const LV2_State_Store_Function store, const LV2_State_Handle handle, const uint32_t /*flags*/, const LV2_Feature* const* const /*features*/) const
    {
        if ((fDescriptor->hints & PLUGIN_USES_STATE) == 0 || fDescriptor->get_state == nullptr)
            return LV2_STATE_ERR_NO_FEATURE;

        if (char* const state = fDescriptor->get_state(fHandle))
        {
            store(handle, fUridMap->map(fUridMap->handle, "http://kxstudio.sf.net/ns/carla/chunk"), state, std::strlen(state), fUris.atomString, LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE);
            std::free(state);
            return LV2_STATE_SUCCESS;
        }

        return LV2_STATE_ERR_UNKNOWN;
    }

    LV2_State_Status lv2_restore(const LV2_State_Retrieve_Function retrieve, const LV2_State_Handle handle, uint32_t flags, const LV2_Feature* const* const /*features*/) const
    {
        if ((fDescriptor->hints & PLUGIN_USES_STATE) == 0 || fDescriptor->set_state == nullptr)
            return LV2_STATE_ERR_NO_FEATURE;

        size_t   size = 0;
        uint32_t type = 0;
        const void* data = retrieve(handle, fUridMap->map(fUridMap->handle, "http://kxstudio.sf.net/ns/carla/chunk"), &size, &type, &flags);

        if (size == 0)
            return LV2_STATE_ERR_UNKNOWN;
        if (type == 0)
            return LV2_STATE_ERR_UNKNOWN;
        if (data == nullptr)
            return LV2_STATE_ERR_UNKNOWN;
        if (type != fUris.atomString)
            return LV2_STATE_ERR_BAD_TYPE;

        fDescriptor->set_state(fHandle, (const char*)data);

        return LV2_STATE_SUCCESS;
    }

    // -------------------------------------------------------------------

    bool lv2ui_instantiate(LV2UI_Write_Function writeFunction, LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
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

        if (fUI.host == nullptr)
            return false;

        fUI.writeFunction = writeFunction;
        fUI.controller = controller;
        *widget = this;

        fHost.uiName = fUI.host->plugin_human_id;

        return true;
    }

    void lv2ui_port_event(uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer) const
    {
        if (format != 0 || bufferSize != sizeof(float) || buffer == nullptr)
            return;
        if (portIndex >= fUI.portOffset || ! fUI.isVisible)
            return;
        if (fDescriptor->ui_set_parameter_value == nullptr)
            return;

        const float value(*(const float*)buffer);
        fDescriptor->ui_set_parameter_value(fHandle, portIndex-fUI.portOffset, value);
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

    void lv2ui_select_program(uint32_t bank, uint32_t program) const
    {
        if (fDescriptor->category == PLUGIN_CATEGORY_SYNTH)
            return;
        if (fDescriptor->ui_set_midi_program == nullptr)
            return;

        fDescriptor->ui_set_midi_program(fHandle, 0, bank, program);
    }

    // -------------------------------------------------------------------

protected:
    void handleUiRun()
    {
        if (fDescriptor->ui_idle != nullptr)
            fDescriptor->ui_idle(fHandle);
    }

    void handleUiShow()
    {
        if (fDescriptor->ui_show != nullptr)
        {
            if (fDescriptor->hints & PLUGIN_NEEDS_UI_JUCE)
            {
                JUCE_AUTORELEASEPOOL
                {
                    if (gActivePlugins.size() == 0)
                    {
                        initialiseJuce_GUI();
                        JuceMessageThread::getInstance();
                    }
                }

                fDescriptor->ui_show(fHandle, true);

                fUiWasShown = true;
                gActivePlugins.add(this);
            }
            else
                fDescriptor->ui_show(fHandle, true);
        }

        fUI.isVisible = true;
    }

    void handleUiHide()
    {
        if (fDescriptor->ui_show != nullptr)
            fDescriptor->ui_show(fHandle, false);

        fUI.isVisible = false;
    }

    // -------------------------------------------------------------------

    uint32_t handleGetBufferSize() const
    {
        return fBufferSize;
    }

    double handleGetSampleRate() const
    {
        return fSampleRate;
    }

    bool handleIsOffline() const
    {
        CARLA_SAFE_ASSERT_RETURN(fIsProcessing, false);

        return (fPorts.freewheel != nullptr && *fPorts.freewheel >= 0.5f);
    }

    const NativeTimeInfo* handleGetTimeInfo() const
    {
        CARLA_SAFE_ASSERT_RETURN(fIsProcessing, nullptr);

        return &fTimeInfo;
    }

    bool handleWriteMidiEvent(const NativeMidiEvent* const event)
    {
        CARLA_SAFE_ASSERT_RETURN(fIsProcessing, false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->midiOuts > 0, false);
        CARLA_SAFE_ASSERT_RETURN(event != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(event->data[0] != 0, false);

        // reverse-find first free event, and put it there
        for (uint32_t i=(kMaxMidiEvents*2)-1; i > fMidiEventCount; --i)
        {
            if (fMidiEvents[i].data[0] == 0)
            {
                std::memcpy(&fMidiEvents[i], event, sizeof(NativeMidiEvent));
                return true;
            }
        }

        return false;
    }

    void handleUiParameterChanged(const uint32_t index, const float value) const
    {
        if (fUI.writeFunction != nullptr && fUI.controller != nullptr)
            fUI.writeFunction(fUI.controller, index+fUI.portOffset, sizeof(float), 0, &value);
    }

    void handleUiCustomDataChanged(const char* const /*key*/, const char* const /*value*/) const
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
        case HOST_OPCODE_NULL:
            break;
        case HOST_OPCODE_SET_VOLUME:
            fVolume = opt;
            break;
        case HOST_OPCODE_SET_DRYWET:
            fDryWet = opt;
            break;
        case HOST_OPCODE_SET_BALANCE_LEFT:
        case HOST_OPCODE_SET_BALANCE_RIGHT:
        case HOST_OPCODE_SET_PANNING:
            // nothing
            break;
        case HOST_OPCODE_GET_PARAMETER_MIDI_CC:
        case HOST_OPCODE_SET_PARAMETER_MIDI_CC:
        case HOST_OPCODE_SET_PROCESS_PRECISION:
        case HOST_OPCODE_UPDATE_PARAMETER:
        case HOST_OPCODE_UPDATE_MIDI_PROGRAM:
        case HOST_OPCODE_RELOAD_PARAMETERS:
        case HOST_OPCODE_RELOAD_MIDI_PROGRAMS:
        case HOST_OPCODE_RELOAD_ALL:
            // nothing
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
    NativePluginHandle   fHandle;
    NativeHostDescriptor fHost;
    const NativePluginDescriptor* const fDescriptor;
    LV2_Program_Descriptor              fProgramDesc;

    uint32_t        fMidiEventCount;
    NativeMidiEvent fMidiEvents[kMaxMidiEvents*2];
    NativeTimeInfo  fTimeInfo;

    bool  fUiWasShown;
    bool  fIsProcessing;
    float fVolume;
    float fDryWet;

    // Lv2 host data
    uint32_t fBufferSize;
    double   fSampleRate;

    const LV2_URID_Map* fUridMap;

    struct URIDs {
        LV2_URID atomBlank;
        LV2_URID atomFloat;
        LV2_URID atomLong;
        LV2_URID atomSequence;
        LV2_URID atomString;
        LV2_URID midiEvent;
        LV2_URID timePos;
        LV2_URID timeBar;
        LV2_URID timeBarBeat;
        LV2_URID timeBeatsPerBar;
        LV2_URID timeBeatsPerMinute;
        LV2_URID timeBeatUnit;
        LV2_URID timeFrame;
        LV2_URID timeSpeed;

        URIDs()
            : atomBlank(0),
              atomFloat(0),
              atomLong(0),
              atomSequence(0),
              atomString(0),
              midiEvent(0),
              timePos(0),
              timeBar(0),
              timeBarBeat(0),
              timeBeatsPerBar(0),
              timeBeatsPerMinute(0),
              timeBeatUnit(0),
              timeFrame(0),
              timeSpeed(0) {}

        void map(const LV2_URID_Map* const uridMap)
        {
            atomBlank    = uridMap->map(uridMap->handle, LV2_ATOM__Blank);
            atomFloat    = uridMap->map(uridMap->handle, LV2_ATOM__Float);
            atomLong     = uridMap->map(uridMap->handle, LV2_ATOM__Long);
            atomSequence = uridMap->map(uridMap->handle, LV2_ATOM__Sequence);
            atomString   = uridMap->map(uridMap->handle, LV2_ATOM__String);
            midiEvent    = uridMap->map(uridMap->handle, LV2_MIDI__MidiEvent);
            timePos      = uridMap->map(uridMap->handle, LV2_TIME__Position);
            timeBar      = uridMap->map(uridMap->handle, LV2_TIME__bar);
            timeBarBeat  = uridMap->map(uridMap->handle, LV2_TIME__barBeat);
            timeBeatUnit = uridMap->map(uridMap->handle, LV2_TIME__beatUnit);
            timeFrame    = uridMap->map(uridMap->handle, LV2_TIME__frame);
            timeSpeed    = uridMap->map(uridMap->handle, LV2_TIME__speed);
            timeBeatsPerBar    = uridMap->map(uridMap->handle, LV2_TIME__beatsPerBar);
            timeBeatsPerMinute = uridMap->map(uridMap->handle, LV2_TIME__beatsPerMinute);
        }
    } fUris;

    struct UI {
        const LV2_External_UI_Host* host;
        LV2UI_Write_Function writeFunction;
        LV2UI_Controller controller;
        uint32_t portOffset;
        bool isVisible;

        UI()
            : host(nullptr),
              writeFunction(nullptr),
              controller(nullptr),
              portOffset(0),
              isVisible(false) {}
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

        void init(const NativePluginDescriptor* const desc, NativePluginHandle handle)
        {
            CARLA_SAFE_ASSERT_RETURN(desc != nullptr && handle != nullptr,)

            if (desc->midiIns > 0)
            {
                eventsIn = new LV2_Atom_Sequence*[desc->midiIns];

                for (uint32_t i=0; i < desc->midiIns; ++i)
                    eventsIn[i] = nullptr;
            }
            else if (desc->hints & PLUGIN_USES_TIME)
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

        void connectPort(const NativePluginDescriptor* const desc, const uint32_t port, void* const dataLocation)
        {
            uint32_t index = 0;

            if (desc->midiIns > 0 || (desc->hints & PLUGIN_USES_TIME) != 0)
            {
                if (port == index++)
                {
                    eventsIn[0] = (LV2_Atom_Sequence*)dataLocation;
                    return;
                }
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

    static uint32_t host_get_buffer_size(NativeHostHandle handle)
    {
        return handlePtr->handleGetBufferSize();
    }

    static double host_get_sample_rate(NativeHostHandle handle)
    {
        return handlePtr->handleGetSampleRate();
    }

    static bool host_is_offline(NativeHostHandle handle)
    {
        return handlePtr->handleIsOffline();
    }

    static const NativeTimeInfo* host_get_time_info(NativeHostHandle handle)
    {
        return handlePtr->handleGetTimeInfo();
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

    for (LinkedList<const NativePluginDescriptor*>::Itenerator it = plm.descs.begin(); it.valid(); it.next())
    {
        const NativePluginDescriptor* const& tmpDesc(it.getValue());

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

#undef instancePtr

// -----------------------------------------------------------------------
// LV2 UI descriptor functions

static LV2UI_Handle lv2ui_instantiate(const LV2UI_Descriptor*, const char*, const char*, LV2UI_Write_Function writeFunction,
                                      LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    carla_debug("lv2ui_instantiate(..., %p, %p, %p)", writeFunction, controller, widget, features);

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

    if (! plugin->lv2ui_instantiate(writeFunction, controller, widget, features))
    {
        carla_stderr("Host doesn't support external UI");
        return nullptr;
    }

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

static void lv2ui_select_program(LV2UI_Handle ui, uint32_t bank, uint32_t program)
{
    carla_debug("lv2ui_select_program(%p, %i, %i)", ui, bank, program);
    uiPtr->lv2ui_select_program(bank, program);
}

#undef uiPtr

// -----------------------------------------------------------------------
// Startup code

CARLA_EXPORT
const DSSI_Descriptor* dssi_descriptor(ulong index)
{
    carla_debug("dssi_descriptor(%i)", index);

    PluginListManager& plm(PluginListManager::getInstance());

    if (index >= plm.descs.count())
    {
        carla_debug("dssi_descriptor(%i) - out of bounds", index);
        return nullptr;
    }
    if (index < plm.dssiDescs.count())
    {
        carla_debug("lv2_descriptor(%i) - found previously allocated", index);
        return plm.dssiDescs.getAt(index, nullptr);
    }

    const NativePluginDescriptor* const pluginDesc(plm.descs.getAt(index, nullptr));
    CARLA_SAFE_ASSERT_RETURN(pluginDesc != nullptr, nullptr);

    CarlaString tmpURI;
    tmpURI  = "http://kxstudio.sf.net/carla/plugins/";
    tmpURI += pluginDesc->label;

    carla_debug("lv2_descriptor(%i) - not found, allocating new with uri \"%s\"", index, (const char*)tmpURI);

    const DSSI_Descriptor dssiDescTmp = {
    /* URI            */ carla_strdup(tmpURI),
    /* instantiate    */ lv2_instantiate,
    /* connect_port   */ lv2_connect_port,
    /* activate       */ lv2_activate,
    /* run            */ lv2_run,
    /* deactivate     */ lv2_deactivate,
    /* cleanup        */ lv2_cleanup,
    /* extension_data */ lv2_extension_data
    };

    DSSI_Descriptor* const dssiDesc(new DSSI_Descriptor);
    std::memcpy(dssiDesc, &dssiDescTmp, sizeof(DSSI_Descriptor));

    plm.dssiDescs.append(dssiDesc);

    return dssiDesc;
}

CARLA_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    carla_debug("lv2ui_descriptor(%i)", index);

    static const LV2UI_Descriptor lv2UiDesc = {
    /* URI            */ "http://kxstudio.sf.net/carla/ui",
    /* instantiate    */ lv2ui_instantiate,
    /* cleanup        */ lv2ui_cleanup,
    /* port_event     */ lv2ui_port_event,
    /* extension_data */ lv2ui_extension_data
    };

    return (index == 0) ? &lv2UiDesc : nullptr;
}

// -----------------------------------------------------------------------
