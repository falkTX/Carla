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
const char* JACK_METADATA_PRETTY_NAME;
const char* JACK_METADATA_PRETTY_NAME = "http://jackaudio.org/metadata/pretty-name";

CARLA_PLUGIN_EXPORT
const char* JACK_METADATA_HARDWARE;
const char* JACK_METADATA_HARDWARE = "http://jackaudio.org/metadata/hardware";

CARLA_PLUGIN_EXPORT
const char* JACK_METADATA_CONNECTED;
const char* JACK_METADATA_CONNECTED = "http://jackaudio.org/metadata/connected";

CARLA_PLUGIN_EXPORT
const char* JACK_METADATA_PORT_GROUP;
const char* JACK_METADATA_PORT_GROUP = "http://jackaudio.org/metadata/port-group";

CARLA_PLUGIN_EXPORT
const char* JACK_METADATA_ICON_SMALL;
const char* JACK_METADATA_ICON_SMALL = "http://jackaudio.org/metadata/icon-small";

CARLA_PLUGIN_EXPORT
const char* JACK_METADATA_ICON_LARGE;
const char* JACK_METADATA_ICON_LARGE = "http://jackaudio.org/metadata/icon-large";

// --------------------------------------------------------------------------------------------------------------------

typedef struct {
    jack_uuid_t uuid;
    std::string key;
} MetadataKey;

typedef struct {
    std::string type;
    std::string value;
} MetadataValue;

typedef std::map<MetadataKey, MetadataValue> Metadata;

static Metadata sMetadata;

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
int jack_set_property(jack_client_t*, jack_uuid_t uuid, const char* key, const char* value, const char* type)
{
    CARLA_SAFE_ASSERT_RETURN(uuid != JACK_UUID_EMPTY_INITIALIZER, -1);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0', -1);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr && value[0] != '\0', -1);

    const MetadataKey mkey = { uuid, key };
    const MetadataValue mvalue = { value, type };
//     sMetadata[mkey] = mvalue;

    return 0;
}

CARLA_PLUGIN_EXPORT
int jack_get_property(jack_uuid_t uuid, const char* key, char** value, char** type)
{
    CARLA_SAFE_ASSERT_RETURN(uuid != JACK_UUID_EMPTY_INITIALIZER, -1);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0', -1);

//     const MetadataKey mkey = { uuid, key };

//     const auto mvalueit = sMetadata.find(mkey);
//
//     if (mvalueit == sMetadata.end())
//         return -1;
//
//     const MetadataValue& mvalue = mvalueit->second;

//     *value = strdup(mvalue.value.c_str());
//
//     if (mvalue.type.size() != 0)
//         *type = strdup(mvalue.type.c_str());
//     else
//         *type = nullptr;

    return -1;

    (void)value;
    (void)type;
}

CARLA_PLUGIN_EXPORT
void jack_free_description(jack_description_t*, int)
{
}

CARLA_PLUGIN_EXPORT
int jack_get_properties(jack_uuid_t, jack_description_t*)
{
    return -1;
}

CARLA_PLUGIN_EXPORT
int jack_get_all_properties(jack_description_t**)
{
    return -1;
}

CARLA_PLUGIN_EXPORT
int jack_remove_property(jack_client_t*, jack_uuid_t uuid, const char* key)
{
//     const MetadataKey mkey = { uuid, key };

//     sMetadata.erase(mkey);
    return -1;

    (void)uuid;
    (void)key;
}

CARLA_PLUGIN_EXPORT
int jack_remove_properties(jack_client_t*, jack_uuid_t uuid)
{
    int count = 0;

//     for (Metadata::const_iterator cit = sMetadata.begin(), cend = sMetadata.end(); cit != cend; ++cit)
    {
//         const MetadataKey& mkey = cit->first;

//         if (mkey.uuid != uuid)
//             continue;

//         ++count;
//         sMetadata.erase(mkey);
    }

    return count;

    (void)uuid;
}

CARLA_PLUGIN_EXPORT
int jack_remove_all_properties(jack_client_t*)
{
//     sMetadata = {};
    return 0;
}

CARLA_PLUGIN_EXPORT
int jack_set_property_change_callback(jack_client_t*, JackPropertyChangeCallback, void*)
{
    return -1;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
jack_uuid_t jack_client_uuid_generate()
{
    static uint32_t uuid_cnt = 0;
    jack_uuid_t uuid = 0x2; /* JackUUIDClient */;
    uuid = (uuid << 32) | ++uuid_cnt;
    return uuid;
}

CARLA_PLUGIN_EXPORT
jack_uuid_t jack_port_uuid_generate(uint32_t port_id)
{
    jack_uuid_t uuid = 0x1; /* JackUUIDPort */
    uuid = (uuid << 32) | (port_id + 1);
    return uuid;
}

CARLA_PLUGIN_EXPORT
uint32_t jack_uuid_to_index(jack_uuid_t u)
{
    return static_cast<uint32_t>(u & 0xffff) - 1;
}

CARLA_PLUGIN_EXPORT
int jack_uuid_compare(jack_uuid_t a, jack_uuid_t b)
{
    if (a == b) {
        return 0;
    }

    if (a < b) {
        return -1;
    }

    return 1;
}

CARLA_PLUGIN_EXPORT
void jack_uuid_copy(jack_uuid_t* dst, jack_uuid_t src)
{
    *dst = src;
}

CARLA_PLUGIN_EXPORT
void jack_uuid_clear(jack_uuid_t* uuid)
{
    *uuid = JACK_UUID_EMPTY_INITIALIZER;
}

CARLA_PLUGIN_EXPORT
int jack_uuid_parse(const char* b, jack_uuid_t* u)
{
    if (std::sscanf(b, P_UINT64, u) != 1)
        return -1;

    if (*u < (0x1LL << 32)) {
        /* has not type bits set - not legal */
        return -1;
    }

    return 0;
}

CARLA_PLUGIN_EXPORT
void jack_uuid_unparse(jack_uuid_t u, char buf[JACK_UUID_STRING_SIZE])
{
    std::snprintf(buf, JACK_UUID_STRING_SIZE, P_UINT64, u);
}

CARLA_PLUGIN_EXPORT
int jack_uuid_empty(jack_uuid_t u)
{
    return u == JACK_UUID_EMPTY_INITIALIZER;
}

// --------------------------------------------------------------------------------------------------------------------
