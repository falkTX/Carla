/*
 * Carla JACK API for external applications
 * Copyright (C) 2016-2017 Filipe Coelho <falktx@falktx.com>
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

CARLA_EXPORT
jack_nframes_t jack_midi_get_event_count(void* buf)
{
    JackMidiPortBuffer* const jmidibuf((JackMidiPortBuffer*)buf);
    CARLA_SAFE_ASSERT_RETURN(jmidibuf != nullptr, 0);
    CARLA_SAFE_ASSERT_RETURN(jmidibuf->isInput, 0);

    return jmidibuf->count;
}

CARLA_EXPORT
int jack_midi_event_get(jack_midi_event_t* ev, void* buf, uint32_t index)
{
    JackMidiPortBuffer* const jmidibuf((JackMidiPortBuffer*)buf);
    CARLA_SAFE_ASSERT_RETURN(jmidibuf != nullptr, EFAULT);
    CARLA_SAFE_ASSERT_RETURN(jmidibuf->isInput, EFAULT);

    if (index >= jmidibuf->count)
        return ENODATA;

    std::memcpy(ev, &jmidibuf->events[index], sizeof(jack_midi_event_t));
    return 0;
}

CARLA_EXPORT
void jack_midi_clear_buffer(void* buf)
{
    JackMidiPortBuffer* const jmidibuf((JackMidiPortBuffer*)buf);
    CARLA_SAFE_ASSERT_RETURN(jmidibuf != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(! jmidibuf->isInput,);

    jmidibuf->count = 0;
    jmidibuf->bufferPoolPos = 0;
    std::memset(jmidibuf->bufferPool, 0, JackMidiPortBuffer::kBufferPoolSize);
}

CARLA_EXPORT
void jack_midi_reset_buffer(void* buf)
{
    jack_midi_clear_buffer(buf);
}

CARLA_EXPORT
size_t jack_midi_max_event_size(void*)
{
    return JackMidiPortBuffer::kMaxEventSize;
}

CARLA_EXPORT
jack_midi_data_t* jack_midi_event_reserve(void* buf, jack_nframes_t frame, size_t size)
{
    JackMidiPortBuffer* const jmidibuf((JackMidiPortBuffer*)buf);
    CARLA_SAFE_ASSERT_RETURN(jmidibuf != nullptr, nullptr);
    CARLA_SAFE_ASSERT_RETURN(! jmidibuf->isInput, nullptr);
    CARLA_SAFE_ASSERT_RETURN(size > 0 && size < JackMidiPortBuffer::kMaxEventSize, nullptr);

    if (jmidibuf->count >= JackMidiPortBuffer::kMaxEventCount)
        return nullptr;
    if (jmidibuf->bufferPoolPos + size >= JackMidiPortBuffer::kBufferPoolSize)
        return nullptr;

    jack_midi_data_t* const jmdata = jmidibuf->bufferPool + jmidibuf->bufferPoolPos;
    jmidibuf->bufferPoolPos += size;

    jmidibuf->events[jmidibuf->count++] = { frame, size, jmdata };
    std::memset(jmdata, 0, size);
    return jmdata;
}

CARLA_EXPORT
int jack_midi_event_write(void* buf, jack_nframes_t frame, const jack_midi_data_t* data, size_t size)
{
    JackMidiPortBuffer* const jmidibuf((JackMidiPortBuffer*)buf);
    CARLA_SAFE_ASSERT_RETURN(jmidibuf != nullptr, EFAULT);
    CARLA_SAFE_ASSERT_RETURN(! jmidibuf->isInput, EFAULT);
    CARLA_SAFE_ASSERT_RETURN(size > 0 && size < JackMidiPortBuffer::kMaxEventSize, ENOBUFS);

    if (jmidibuf->count >= JackMidiPortBuffer::kMaxEventCount)
        return ENOBUFS;
    if (jmidibuf->bufferPoolPos + size >= JackMidiPortBuffer::kBufferPoolSize)
        return ENOBUFS;

    jack_midi_data_t* const jmdata = jmidibuf->bufferPool + jmidibuf->bufferPoolPos;
    jmidibuf->bufferPoolPos += size;

    jmidibuf->events[jmidibuf->count++] = { frame, size, jmdata };
    std::memcpy(jmdata, data, size);
    return 0;
}

CARLA_EXPORT
uint32_t jack_midi_get_lost_event_count(void*)
{
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
