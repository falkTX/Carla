/*
 * Carla Native Plugin API (C++)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef __CARLA_NATIVE_HPP__
#define __CARLA_NATIVE_HPP__

#include "CarlaNative.h"
#include "CarlaMIDI.h"
#include "CarlaJuceUtils.hpp"

/*!
 * @defgroup CarlaNativeAPI Carla Native API
 * @{
 */

class PluginDescriptorClass
{
public:
    PluginDescriptorClass(const HostDescriptor* const host)
        : kHost(host)
    {
        CARLA_ASSERT(host != nullptr);
    }

    virtual ~PluginDescriptorClass()
    {
    }

protected:
    // -------------------------------------------------------------------
    // Host calls

    const HostDescriptor* hostHandle() const
    {
        return kHost;
    }

    const char* hostUiName() const
    {
        CARLA_ASSERT(kHost != nullptr);

        if (kHost != nullptr)
            return kHost->ui_name;

        return nullptr;
    }

    uint32_t getBufferSize()
    {
        CARLA_ASSERT(kHost != nullptr);

        if (kHost != nullptr)
            return kHost->get_buffer_size(kHost->handle);

        return 0;
    }

    double getSampleRate()
    {
        CARLA_ASSERT(kHost != nullptr);

        if (kHost != nullptr)
            return kHost->get_sample_rate(kHost->handle);

        return 0.0;
    }

    const TimeInfo* getTimeInfo()
    {
        CARLA_ASSERT(kHost != nullptr);

        if (kHost != nullptr)
            return kHost->get_time_info(kHost->handle);

        return nullptr;
    }

    void writeMidiEvent(const MidiEvent* const event)
    {
        CARLA_ASSERT(kHost != nullptr);

        if (kHost != nullptr)
            kHost->write_midi_event(kHost->handle, event);
    }

    void uiParameterChanged(const uint32_t index, const float value)
    {
        CARLA_ASSERT(kHost != nullptr);

        if (kHost != nullptr)
            kHost->ui_parameter_changed(kHost->handle, index, value);
    }

    void uiMidiProgramChanged(const uint8_t channel, const uint32_t bank, const uint32_t program)
    {
        CARLA_ASSERT(kHost != nullptr);

        if (kHost != nullptr)
            kHost->ui_midi_program_changed(kHost->handle, channel, bank, program);
    }

    void uiCustomDataChanged(const char* const key, const char* const value)
    {
        CARLA_ASSERT(kHost != nullptr);

        if (kHost != nullptr)
            kHost->ui_custom_data_changed(kHost->handle, key, value);
    }

    void uiClosed()
    {
        CARLA_ASSERT(kHost != nullptr);

        if (kHost != nullptr)
            kHost->ui_closed(kHost->handle);
    }

    const char* uiOpenFile(const bool isDir, const char* const title, const char* const filter)
    {
        CARLA_ASSERT(kHost != nullptr);

        if (kHost != nullptr)
            return kHost->ui_open_file(kHost->handle, isDir, title, filter);

        return nullptr;
    }

    const char* uiSaveFile(const bool isDir, const char* const title, const char* const filter)
    {
        CARLA_ASSERT(kHost != nullptr);

        if (kHost != nullptr)
            return kHost->ui_save_file(kHost->handle, isDir, title, filter);

        return nullptr;
    }

    intptr_t hostDispatcher(const HostDispatcherOpcode opcode, const int32_t index, const intptr_t value, void* const ptr)
    {
        CARLA_ASSERT(kHost != nullptr);

        if (kHost != nullptr)
            return kHost->dispatcher(kHost->handle, opcode, index, value, ptr);

        return nullptr;
    }

    // -------------------------------------------------------------------
    // Plugin parameter calls

    virtual uint32_t getParameterCount()
    {
        return 0;
    }

    virtual const Parameter* getParameterInfo(const uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());
        return nullptr;

        // unused
        (void)index;
    }

    virtual float getParameterValue(const uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());
        return 0.0f;

        // unused
        (void)index;
    }

    virtual const char* getParameterText(const uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());
        return nullptr;

        // unused
        (void)index;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    virtual uint32_t getMidiProgramCount()
    {
        return 0;
    }

    virtual const MidiProgram* getMidiProgramInfo(const uint32_t index)
    {
        CARLA_ASSERT(index < getMidiProgramCount());
        return nullptr;

        // unused
        (void)index;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    virtual void setParameterValue(const uint32_t index, const float value)
    {
        CARLA_ASSERT(index < getParameterCount());
        return;

        // unused
        (void)index;
        (void)value;
    }

    virtual void setMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program)
    {
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        return;

        // unused
        (void)channel;
        (void)bank;
        (void)program;
    }

    virtual void setCustomData(const char* const key, const char* const value)
    {
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);
        return;

        // unused
        (void)key;
        (void)value;
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    virtual void activate()
    {
    }

    virtual void deactivate()
    {
    }

    virtual void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t midiEventCount, const MidiEvent* const midiEvents) = 0;

    // -------------------------------------------------------------------
    // Plugin UI calls

    virtual void uiShow(const bool show)
    {
        return;

        // unused
        (void)show;
    }

    virtual void uiIdle()
    {
    }

    virtual void uiSetParameterValue(const uint32_t index, const float value)
    {
        CARLA_ASSERT(index < getParameterCount());
        return;

        // unused
        (void)value;
    }

    virtual void uiSetMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program)
    {
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        return;

        // unused
        (void)channel;
        (void)bank;
        (void)program;
    }

    virtual void uiSetCustomData(const char* const key, const char* const value)
    {
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);
        return;

        // unused
        (void)key;
        (void)value;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    virtual char* getState()
    {
        return nullptr;
    }

    virtual void setState(const char* const data)
    {
        CARLA_ASSERT(data != nullptr);
        return;

        // unused
        (void)data;
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher

    virtual intptr_t pluginDispatcher(const PluginDispatcherOpcode opcode, const int32_t index, const intptr_t value, void* const ptr)
    {
        return 0;

        // unused
        (void)opcode;
        (void)index;
        (void)value;
        (void)ptr;
    }

    // -------------------------------------------------------------------

private:
    const HostDescriptor* const kHost;

    // -------------------------------------------------------------------

#ifndef DOXYGEN
public:
    #define handlePtr ((PluginDescriptorClass*)handle)

    static uint32_t _get_parameter_count(PluginHandle handle)
    {
        return handlePtr->getParameterCount();
    }

    static const Parameter* _get_parameter_info(PluginHandle handle, uint32_t index)
    {
        return handlePtr->getParameterInfo(index);
    }

    static float _get_parameter_value(PluginHandle handle, uint32_t index)
    {
        return handlePtr->getParameterValue(index);
    }

    static const char* _get_parameter_text(PluginHandle handle, uint32_t index)
    {
        return handlePtr->getParameterText(index);
    }

    static uint32_t _get_midi_program_count(PluginHandle handle)
    {
        return handlePtr->getMidiProgramCount();
    }

    static const MidiProgram* _get_midi_program_info(PluginHandle handle, uint32_t index)
    {
        return handlePtr->getMidiProgramInfo(index);
    }

    static void _set_parameter_value(PluginHandle handle, uint32_t index, float value)
    {
        return handlePtr->setParameterValue(index, value);
    }

    static void _set_midi_program(PluginHandle handle, uint8_t channel, uint32_t bank, uint32_t program)
    {
        return handlePtr->setMidiProgram(channel, bank, program);
    }

    static void _set_custom_data(PluginHandle handle, const char* key, const char* value)
    {
        return handlePtr->setCustomData(key, value);
    }

    static void _ui_show(PluginHandle handle, bool show)
    {
        return handlePtr->uiShow(show);
    }

    static void _ui_idle(PluginHandle handle)
    {
        return handlePtr->uiIdle();
    }

    static void _ui_set_parameter_value(PluginHandle handle, uint32_t index, float value)
    {
        return handlePtr->uiSetParameterValue(index, value);
    }

    static void _ui_set_midi_program(PluginHandle handle, uint8_t channel, uint32_t bank, uint32_t program)
    {
        return handlePtr->uiSetMidiProgram(channel, bank, program);
    }

    static void _ui_set_custom_data(PluginHandle handle, const char* key, const char* value)
    {
        return handlePtr->uiSetCustomData(key, value);
    }

    static void _activate(PluginHandle handle)
    {
        handlePtr->activate();
    }

    static void _deactivate(PluginHandle handle)
    {
        handlePtr->deactivate();
    }

    static void _process(PluginHandle handle, float** inBuffer, float** outBuffer, const uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents)
    {
        return handlePtr->process(inBuffer, outBuffer, frames, midiEventCount, midiEvents);
    }

    static char* _get_state(PluginHandle handle)
    {
        return handlePtr->getState();
    }

    static void _set_state(PluginHandle handle, const char* data)
    {
        handlePtr->setState(data);
    }

    static intptr_t _dispatcher(PluginHandle handle, PluginDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr)
    {
        return handlePtr->pluginDispatcher(opcode, index, value, ptr);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginDescriptorClass)
#endif
};

/**@}*/

// -----------------------------------------------------------------------

#define PluginDescriptorClassEND(className)                                         \
public:                                                                             \
    static PluginHandle _instantiate(const PluginDescriptor*, HostDescriptor* host) \
    {                                                                               \
        return new className(host);                                                 \
    }                                                                               \
    static void _cleanup(PluginHandle handle)                                       \
    {                                                                               \
        delete (className*)handle;                                                  \
    }

#define PluginDescriptorFILL(className) \
    className::_instantiate,            \
    className::_cleanup,                \
    className::_get_parameter_count,    \
    className::_get_parameter_info,     \
    className::_get_parameter_value,    \
    className::_get_parameter_text,     \
    className::_get_midi_program_count, \
    className::_get_midi_program_info,  \
    className::_set_parameter_value,    \
    className::_set_midi_program,       \
    className::_set_custom_data,        \
    className::_ui_show,                \
    className::_ui_idle,                \
    className::_ui_set_parameter_value, \
    className::_ui_set_midi_program,    \
    className::_ui_set_custom_data,     \
    className::_activate,               \
    className::_deactivate,             \
    className::_process,                \
    className::_get_state,              \
    className::_set_state,              \
    className::_dispatcher

#endif // __CARLA_NATIVE_HPP__
