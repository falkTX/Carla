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

#define JACK_UUID_SIZE 36
#define JACK_UUID_STRING_SIZE (JACK_UUID_SIZE+1) /* includes trailing null */
#define JACK_UUID_EMPTY_INITIALIZER 0

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
const char* JACK_METADATA_PRETTY_NAME;
const char* JACK_METADATA_PRETTY_NAME = "http://jackaudio.org/metadata/pretty-name";

CARLA_EXPORT
const char* JACK_METADATA_HARDWARE;
const char* JACK_METADATA_HARDWARE = "http://jackaudio.org/metadata/hardware";

CARLA_EXPORT
const char* JACK_METADATA_CONNECTED;
const char* JACK_METADATA_CONNECTED = "http://jackaudio.org/metadata/connected";

CARLA_EXPORT
const char* JACK_METADATA_PORT_GROUP;
const char* JACK_METADATA_PORT_GROUP = "http://jackaudio.org/metadata/port-group";

CARLA_EXPORT
const char* JACK_METADATA_ICON_SMALL;
const char* JACK_METADATA_ICON_SMALL = "http://jackaudio.org/metadata/icon-small";

CARLA_EXPORT
const char* JACK_METADATA_ICON_LARGE;
const char* JACK_METADATA_ICON_LARGE = "http://jackaudio.org/metadata/icon-large";

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_set_property(jack_client_t*, jack_uuid_t, const char*, const char*, const char*)
{
    return -1;
}

CARLA_EXPORT
int jack_get_property(jack_uuid_t, const char*, char**, char**)
{
    return -1;
}

CARLA_EXPORT
void jack_free_description(jack_description_t*, int)
{
}

CARLA_EXPORT
int jack_get_properties(jack_uuid_t, jack_description_t*)
{
    return -1;
}

CARLA_EXPORT
int jack_get_all_properties(jack_description_t**)
{
    return -1;
}

CARLA_EXPORT
int jack_remove_property(jack_client_t*, jack_uuid_t, const char*)
{
    return -1;
}

CARLA_EXPORT
int jack_remove_properties(jack_client_t*, jack_uuid_t)
{
    return -1;
}

CARLA_EXPORT
int jack_remove_all_properties(jack_client_t*)
{
    return -1;
}

CARLA_EXPORT
int jack_set_property_change_callback(jack_client_t*, JackPropertyChangeCallback, void*)
{
    return -1;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
jack_uuid_t jack_client_uuid_generate()
{
    return 0;
}

CARLA_EXPORT
jack_uuid_t jack_port_uuid_generate(uint32_t)
{
    return 0;
}

CARLA_EXPORT
uint32_t jack_uuid_to_index(jack_uuid_t)
{
    return 0;
}

CARLA_EXPORT
int jack_uuid_compare(jack_uuid_t, jack_uuid_t)
{
    return 0;
}

CARLA_EXPORT
void jack_uuid_copy(jack_uuid_t*, jack_uuid_t)
{
}

CARLA_EXPORT
void jack_uuid_clear(jack_uuid_t*)
{
}

CARLA_EXPORT
int jack_uuid_parse(const char*, jack_uuid_t*)
{
    return 0;
}

CARLA_EXPORT
void jack_uuid_unparse(jack_uuid_t, char buf[JACK_UUID_STRING_SIZE])
{
}

CARLA_EXPORT
int jack_uuid_empty(jack_uuid_t)
{
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
