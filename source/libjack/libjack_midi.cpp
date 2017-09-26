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

// need to include this first
#include "libjack.hpp"

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
jack_nframes_t jack_midi_get_event_count(void*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return 0;
}

CARLA_EXPORT
int jack_midi_event_get(jack_midi_event_t*, void*, uint32_t)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return ENODATA;
}

// void
// jack_midi_clear_buffer(void *port_buffer) JACK_OPTIONAL_WEAK_EXPORT;

// void
// jack_midi_reset_buffer(void *port_buffer) JACK_OPTIONAL_WEAK_EXPORT;

// size_t
// jack_midi_max_event_size(void* port_buffer) JACK_OPTIONAL_WEAK_EXPORT;

// jack_midi_data_t*
// jack_midi_event_reserve(void *port_buffer,
//                         jack_nframes_t  time,
//                         size_t data_size) JACK_OPTIONAL_WEAK_EXPORT;

// int
// jack_midi_event_write(void *port_buffer,
//                       jack_nframes_t time,
//                       const jack_midi_data_t *data,
//                       size_t data_size) JACK_OPTIONAL_WEAK_EXPORT;

// uint32_t jack_midi_get_lost_event_count(void *port_buffer) JACK_OPTIONAL_WEAK_EXPORT;

// --------------------------------------------------------------------------------------------------------------------
