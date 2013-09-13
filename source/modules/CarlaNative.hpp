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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_NATIVE_HPP_INCLUDED
#define CARLA_NATIVE_HPP_INCLUDED

#include "CarlaNative.h"
#include "CarlaMIDI.h"
#include "CarlaJuceUtils.hpp"

/*!
 * @defgroup CarlaNativeAPI Carla Native API
 * @{
 */

// -----------------------------------------------------------------------
// Plugin Class

class PluginClass
{
public:
    struct MappedValues {
        // event types
        MappedValue midi;
        MappedValue parameter;
        // plugin opcodes
        MappedValue msgReceived;
        MappedValue bufferSizeChanged;
        MappedValue sampleRateChanged;
        MappedValue offlineChanged;
        // host opcodes
        MappedValue needsIdle;

    } fMap;

    PluginClass(const PluginHostDescriptor* const host)
        : pHost(host)
    {
        std::memset(&fMap, 0, sizeof(MappedValues));
        CARLA_SAFE_ASSERT_RETURN(host != nullptr,);

        fMap.midi      = pHost->map_value(pHost->handle, EVENT_TYPE_MIDI);
        fMap.parameter = pHost->map_value(pHost->handle, EVENT_TYPE_PARAMETER);

        fMap.msgReceived       = pHost->map_value(pHost->handle, PLUGIN_OPCODE_MSG_RECEIVED);
        fMap.bufferSizeChanged = pHost->map_value(pHost->handle, PLUGIN_OPCODE_BUFFER_SIZE_CHANGED);
        fMap.sampleRateChanged = pHost->map_value(pHost->handle, PLUGIN_OPCODE_SAMPLE_RATE_CHANGED);
        fMap.offlineChanged    = pHost->map_value(pHost->handle, PLUGIN_OPCODE_OFFLINE_CHANGED);

        fMap.needsIdle = pHost->map_value(pHost->handle, HOST_OPCODE_NEEDS_IDLE);
    }

    virtual ~PluginClass()
    {
    }

protected:
    // -------------------------------------------------------------------
    // Host calls

    const PluginHostDescriptor* getHostHandle() const noexcept
    {
        return pHost;
    }

    int getPluginVersion() const noexcept
    {
        return pHost->pluginVersion;
    }

    MappedValue mapValue(const char* const valueStr) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr, 0);

        return pHost->map_value(pHost->handle, valueStr);
    }

    const char* unmapValue(const MappedValue value) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(value != 0, nullptr);

        return pHost->unmap_value(pHost->handle, value);
    }

    uint32_t getBufferSize() const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr, 0);

        return pHost->get_buffer_size(pHost->handle);
    }

    double getSampleRate() const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr, 0.0);

        return pHost->get_sample_rate(pHost->handle);
    }

    bool isOffline() const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr, false);

        return pHost->is_offline(pHost->handle);
    }

    const TimeInfo* getTimeInfo() const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr, nullptr);

        return pHost->get_time_info(pHost->handle);
    }

    bool sendUiMessage(const char* const msg)
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(msg != nullptr, false);

        return pHost->send_ui_msg(pHost->handle, msg);
    }

    bool writeEvent(const Event* const event) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(event != nullptr, false);

        return pHost->write_event(pHost->handle, event);
    }

    bool writeMidiEvent(const MidiEvent* const midiEvent) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(midiEvent != nullptr, false);

        return pHost->write_event(pHost->handle, (const Event*)midiEvent);
    }

    bool writeParameterEvent(const ParameterEvent* const paramEvent) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(paramEvent != nullptr, false);

        return pHost->write_event(pHost->handle, (const Event*)paramEvent);
    }

    // -------------------------------------------------------------------
    // Host dispatcher calls

    void hostNeedsIdle() const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, fMap.needsIdle, 0, 0, nullptr, 0.0f);
    }

#if 0
    void hostSetVolume(const float value) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_SET_VOLUME, 0, 0, nullptr, value);
    }

    void hostSetDryWet(const float value) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_SET_DRYWET, 0, 0, nullptr, value);
    }

    void hostSetBalanceLeft(const float value) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_SET_BALANCE_LEFT, 0, 0, nullptr, value);
    }

    void hostSetBalanceRight(const float value) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_SET_BALANCE_RIGHT, 0, 0, nullptr, value);
    }

    void hostSetPanning(const float value) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_SET_PANNING, 0, 0, nullptr, value);
    }

    intptr_t hostGetParameterMidiCC(const int32_t index) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr, -1);

        return pHost->dispatcher(pHost->handle, HOST_OPCODE_GET_PARAMETER_MIDI_CC, index, 0, nullptr, 0.0f);
    }

    void hostSetParameterMidiCC(const int32_t index, const intptr_t value) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_SET_PARAMETER_MIDI_CC, index, value, nullptr, 0.0f);
    }

    void hostUpdateParameter(const int32_t index) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_UPDATE_PARAMETER, index, 0, nullptr, 0.0f);
    }

    void hostUpdateAllParameters() const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_UPDATE_PARAMETER, -1, 0, nullptr, 0.0f);
    }

    void hostUpdateMidiProgram(const int32_t index, const intptr_t channel = 0) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_UPDATE_MIDI_PROGRAM, index, channel, nullptr, 0.0f);
    }

    void hostUpdateAllMidiPrograms(const intptr_t channel = 0) const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_UPDATE_MIDI_PROGRAM, -1, channel, nullptr, 0.0f);
    }

    void hostReloadParameters() const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_RELOAD_PARAMETERS, 0, 0, nullptr, 0.0f);
    }

    void hostReloadMidiPrograms() const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_RELOAD_MIDI_PROGRAMS, 0, 0, nullptr, 0.0f);
    }

    void hostReloadAll() const
    {
        CARLA_SAFE_ASSERT_RETURN(pHost != nullptr,);

        pHost->dispatcher(pHost->handle, HOST_OPCODE_RELOAD_ALL, 0, 0, nullptr, 0.0f);
    }
#endif

    // -------------------------------------------------------------------
    // Plugin parameter calls

    virtual uint32_t getParameterCount() const
    {
        return 0;
    }

    virtual const Parameter* getParameterInfo(const uint32_t index) const
    {
        CARLA_SAFE_ASSERT_RETURN(index < getParameterCount(), nullptr);
        return nullptr;
    }

    virtual float getParameterValue(const uint32_t index) const
    {
        CARLA_SAFE_ASSERT_RETURN(index < getParameterCount(), 0.0f);
        return 0.0f;
    }

    virtual const char* getParameterText(const uint32_t index, const float value) const
    {
        CARLA_SAFE_ASSERT_RETURN(index < getParameterCount(), nullptr);
        return nullptr;

        // unused
        (void)value;
    }

    virtual void setParameterValue(const uint32_t index, const float value)
    {
        CARLA_SAFE_ASSERT_RETURN(index < getParameterCount(),);
        return;

        // unused
        (void)value;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    virtual uint32_t getMidiProgramCount() const
    {
        return 0;
    }

    virtual const MidiProgram* getMidiProgramInfo(const uint32_t index) const
    {
        CARLA_SAFE_ASSERT_RETURN(index < getMidiProgramCount(), nullptr);
        return nullptr;
    }

    virtual void setMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program)
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        return;

        // unused
        (void)bank;
        (void)program;
    }

    // -------------------------------------------------------------------
    // Plugin idle

    virtual void idle()
    {
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    virtual char* getState() const
    {
        return nullptr;
    }

    virtual void setState(const char* const data)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    virtual void activate()
    {
    }

    virtual void deactivate()
    {
    }

    virtual void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const Event* const events, const uint32_t eventCount) = 0;

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    virtual void messageReceived(const char* const msg)
    {
        CARLA_SAFE_ASSERT_RETURN(msg != nullptr,);
    }

    virtual void bufferSizeChanged(const uint32_t bufferSize)
    {
        CARLA_SAFE_ASSERT_RETURN(bufferSize > 0,);
    }

    virtual void sampleRateChanged(const double sampleRate)
    {
        CARLA_SAFE_ASSERT_RETURN(sampleRate > 0.0,);
    }

    virtual void offlineModeChanged(const bool isOffline)
    {
        return;

        // unused
        (void)isOffline;
    }

    // -------------------------------------------------------------------

private:
    const PluginHostDescriptor* const pHost;

    // -------------------------------------------------------------------

#ifndef DOXYGEN
public:
    #define handlePtr ((PluginClass*)handle)

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

    static const char* _get_parameter_text(PluginHandle handle, uint32_t index, float value)
    {
        return handlePtr->getParameterText(index, value);
    }

    static void _set_parameter_value(PluginHandle handle, uint32_t index, float value)
    {
        handlePtr->setParameterValue(index, value);
    }

    static uint32_t _get_midi_program_count(PluginHandle handle)
    {
        return handlePtr->getMidiProgramCount();
    }

    static const MidiProgram* _get_midi_program_info(PluginHandle handle, uint32_t index)
    {
        return handlePtr->getMidiProgramInfo(index);
    }

    static void _set_midi_program(PluginHandle handle, uint8_t channel, uint32_t bank, uint32_t program)
    {
        handlePtr->setMidiProgram(channel, bank, program);
    }

    static void _idle(PluginHandle handle)
    {
        handlePtr->idle();
    }

    static char* _get_state(PluginHandle handle)
    {
        return handlePtr->getState();
    }

    static void _set_state(PluginHandle handle, const char* data)
    {
        handlePtr->setState(data);
    }

    static void _activate(PluginHandle handle)
    {
        handlePtr->activate();
    }

    static void _deactivate(PluginHandle handle)
    {
        handlePtr->deactivate();
    }

    static void _process(PluginHandle handle, float** inBuffer, float** outBuffer, const uint32_t frames, const Event* events, uint32_t eventCount)
    {
        handlePtr->process(inBuffer, outBuffer, frames, events, eventCount);
    }

    static intptr_t _dispatcher(PluginHandle handle, MappedValue opcode, int32_t /*index*/, intptr_t value, void* ptr, float opt)
    {
        const MappedValues& map(handlePtr->fMap);

        if (opcode == 0)
        {
        }
        else if (opcode == map.msgReceived)
        {
            CARLA_SAFE_ASSERT_RETURN(ptr != nullptr, 0);
            handlePtr->messageReceived(static_cast<const char*>(ptr));
        }
        else if (opcode == map.bufferSizeChanged)
        {
            CARLA_SAFE_ASSERT_RETURN(value > 0, 0);
            handlePtr->bufferSizeChanged(static_cast<uint32_t>(value));
        }
        else if (opcode == map.sampleRateChanged)
        {
            CARLA_SAFE_ASSERT_RETURN(opt > 0.0f, 0);
            handlePtr->sampleRateChanged(static_cast<double>(opt));
        }
        else if (opcode == map.offlineChanged)
        {
            handlePtr->offlineModeChanged(value != 0);
        }

        return 0;
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginClass)
#endif
};

/**@}*/

// -----------------------------------------------------------------------

#define PluginClassEND(ClassName)                                      \
public:                                                                \
    static PluginHandle _instantiate(const PluginHostDescriptor* host) \
    {                                                                  \
        return new ClassName(host);                                    \
    }                                                                  \
    static void _cleanup(PluginHandle handle)                          \
    {                                                                  \
        delete (ClassName*)handle;                                     \
    }

#define PluginDescriptorFILL(ClassName) \
    ClassName::_instantiate,            \
    ClassName::_cleanup,                \
    ClassName::_get_parameter_count,    \
    ClassName::_get_parameter_info,     \
    ClassName::_get_parameter_value,    \
    ClassName::_get_parameter_text,     \
    ClassName::_set_parameter_value,    \
    ClassName::_get_midi_program_count, \
    ClassName::_get_midi_program_info,  \
    ClassName::_set_midi_program,       \
    ClassName::_idle,                   \
    ClassName::_get_state,              \
    ClassName::_set_state,              \
    ClassName::_activate,               \
    ClassName::_deactivate,             \
    ClassName::_process,                \
    ClassName::_dispatcher

// -----------------------------------------------------------------------

#endif // CARLA_NATIVE_HPP_INCLUDED
