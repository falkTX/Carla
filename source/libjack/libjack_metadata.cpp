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
const char* JACK_METADATA_PRETTY_NAME;
const char* JACK_METADATA_PRETTY_NAME = "http://jackaudio.org/metadata/pretty-name";

// extern const char* JACK_METADATA_HARDWARE;
// extern const char* JACK_METADATA_CONNECTED;
// extern const char* JACK_METADATA_PORT_GROUP;
// extern const char* JACK_METADATA_ICON_SMALL;
// extern const char* JACK_METADATA_ICON_LARGE;

// --------------------------------------------------------------------------------------------------------------------

// int
// jack_set_property(jack_client_t*,
//                   jack_uuid_t subject,
//                   const char* key,
//                   const char* value,
//                   const char* type);

// int
// jack_get_property(jack_uuid_t subject,
//                   const char* key,
//                   char**      value,
//                   char**      type);

// void
// jack_free_description (jack_description_t* desc, int free_description_itself);

// int
// jack_get_properties (jack_uuid_t         subject,
//                      jack_description_t* desc);

// int
// jack_get_all_properties (jack_description_t** descs);

// int jack_remove_property (jack_client_t* client, jack_uuid_t subject, const char* key);

// int jack_remove_properties (jack_client_t* client, jack_uuid_t subject);

// int jack_remove_all_properties (jack_client_t* client);

// int jack_set_property_change_callback (jack_client_t*             client,
//                                        JackPropertyChangeCallback callback,
//                                        void*                      arg);

// --------------------------------------------------------------------------------------------------------------------
