/*
 * Carla JACK API for external applications
 * Copyright (C) 2016-2022 Filipe Coelho <falktx@falktx.com>
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

#include "libjack.hpp"

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
jack_nframes_t jack_midi_get_event_count(void* buf)
{
    const JackMidiPortBufferBase* const jbasebuf((const JackMidiPortBufferBase*)buf);
    CARLA_SAFE_ASSERT_RETURN(jbasebuf != nullptr, 0);
    CARLA_SAFE_ASSERT_RETURN(jbasebuf->isInput, 0);

    if (jbasebuf->isDummy)
        return 0;

    const JackMidiPortBufferOnStack* const jmidibuf((const JackMidiPortBufferOnStack*)jbasebuf);

    return jmidibuf->count;
}

CARLA_PLUGIN_EXPORT
int jack_midi_event_get(jack_midi_event_t* ev, void* buf, uint32_t index)
{
    const JackMidiPortBufferBase* const jbasebuf((const JackMidiPortBufferBase*)buf);
    CARLA_SAFE_ASSERT_RETURN(jbasebuf != nullptr, EFAULT);
    CARLA_SAFE_ASSERT_RETURN(jbasebuf->isInput, EFAULT);
    CARLA_SAFE_ASSERT_RETURN(!jbasebuf->isDummy, EFAULT);

    const JackMidiPortBufferOnStack* const jmidibuf((const JackMidiPortBufferOnStack*)jbasebuf);

    if (index >= jmidibuf->count)
        return ENODATA;

    std::memcpy(ev, &jmidibuf->events[index], sizeof(jack_midi_event_t));
    return 0;
}

CARLA_PLUGIN_EXPORT
void jack_midi_clear_buffer(void* buf)
{
    JackMidiPortBufferBase* const jbasebuf((JackMidiPortBufferBase*)buf);
    CARLA_SAFE_ASSERT_RETURN(jbasebuf != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(! jbasebuf->isInput,);

    if (jbasebuf->isDummy)
        return;

    JackMidiPortBufferOnStack* const jmidibuf((JackMidiPortBufferOnStack*)jbasebuf);

    jmidibuf->count = 0;
    jmidibuf->bufferPoolPos = 0;
    std::memset(jmidibuf->bufferPool, 0, JackMidiPortBufferBase::kBufferPoolSize);
}

CARLA_PLUGIN_EXPORT
void jack_midi_reset_buffer(void* buf)
{
    jack_midi_clear_buffer(buf);
}

CARLA_PLUGIN_EXPORT
size_t jack_midi_max_event_size(void*)
{
    return JackMidiPortBufferBase::kMaxEventSize;
}

CARLA_PLUGIN_EXPORT
jack_midi_data_t* jack_midi_event_reserve(void* buf, jack_nframes_t frame, size_t size)
{
    JackMidiPortBufferBase* const jbasebuf((JackMidiPortBufferBase*)buf);
    CARLA_SAFE_ASSERT_RETURN(jbasebuf != nullptr, nullptr);
    CARLA_SAFE_ASSERT_RETURN(! jbasebuf->isInput, nullptr);
    CARLA_SAFE_ASSERT_RETURN(size < JackMidiPortBufferBase::kMaxEventSize, nullptr);

    if (jbasebuf->isDummy)
        return nullptr;

    // broken jack applications, wow...
    if (size == 0)
        return nullptr;

    JackMidiPortBufferOnStack* const jmidibuf((JackMidiPortBufferOnStack*)jbasebuf);

    if (jmidibuf->count >= JackMidiPortBufferBase::kMaxEventCount)
        return nullptr;
    if (jmidibuf->bufferPoolPos + size >= JackMidiPortBufferBase::kBufferPoolSize)
        return nullptr;

    jack_midi_data_t* const jmdata = jmidibuf->bufferPool + jmidibuf->bufferPoolPos;
    jmidibuf->bufferPoolPos += size;

    jmidibuf->events[jmidibuf->count++] = { frame, size, jmdata };
    std::memset(jmdata, 0, size);
    return jmdata;
}

CARLA_PLUGIN_EXPORT
int jack_midi_event_write(void* buf, jack_nframes_t frame, const jack_midi_data_t* data, size_t size)
{
    JackMidiPortBufferBase* const jbasebuf((JackMidiPortBufferBase*)buf);
    CARLA_SAFE_ASSERT_RETURN(jbasebuf != nullptr, EFAULT);
    CARLA_SAFE_ASSERT_RETURN(! jbasebuf->isInput, EINVAL);
    CARLA_SAFE_ASSERT_RETURN(size < JackMidiPortBufferBase::kMaxEventSize, ENOBUFS);

    if (jbasebuf->isDummy)
        return EINVAL;

    // broken jack applications, wow...
    if (size == 0)
        return EINVAL;

    JackMidiPortBufferOnStack* const jmidibuf((JackMidiPortBufferOnStack*)jbasebuf);

    if (jmidibuf->count >= JackMidiPortBufferBase::kMaxEventCount)
        return ENOBUFS;
    if (jmidibuf->bufferPoolPos + size >= JackMidiPortBufferBase::kBufferPoolSize)
        return ENOBUFS;

    jack_midi_data_t* const jmdata = jmidibuf->bufferPool + jmidibuf->bufferPoolPos;
    jmidibuf->bufferPoolPos += size;

    jmidibuf->events[jmidibuf->count++] = { frame, size, jmdata };
    std::memcpy(jmdata, data, size);
    return 0;
}

CARLA_PLUGIN_EXPORT
uint32_t jack_midi_get_lost_event_count(void*)
{
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
