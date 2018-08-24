/*
 * Carla REST API Server
 * Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>
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

/* NOTE
 * Even though Carla is GPL, restbed if AGPL.
 * As such, the resulting binary will be AGPL.
 * Take this into consideration before deploying it to any servers.
 */

#include "CarlaHost.h"
#include "CarlaUtils.h"
#include "CarlaUtils.hpp"

#include <cstring>
#include <restbed>

// #include <memory>
// #include <cstdlib>

// using namespace std;
using namespace restbed;

#if 0
void post_method_handler(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const string body = "Hello, " + request->get_path_parameter("id");
    session->close(OK, body, { { "Content-Length", ::to_string( body.size( ) ) } } );
}
#endif

CARLA_BACKEND_START_NAMESPACE

enum {
    kStrBufSize = 1023,
    kJsonBufSize = 4095,
    kSizeBufSize = 31,
};

// static buffer to returned content
static char strBuf[kStrBufSize+1];
static char jsonBuf[kJsonBufSize+1];

// static buffer to returned content size
static char sizeBuf[kSizeBufSize+1];

void prepare_size()
{
    const std::size_t size = std::strlen(strBuf);
    std::snprintf(sizeBuf, kSizeBufSize, P_SIZE, size);
    sizeBuf[kSizeBufSize] = '\0';
}

void prepare_buffer_uint(const uint value)
{
    std::snprintf(strBuf, kStrBufSize, "%u", value);
    strBuf[kStrBufSize] = '\0';
}

void prepare_buffer_string(const char* const string)
{
    std::strncpy(strBuf, string, kStrBufSize);
    strBuf[kStrBufSize] = '\0';
}

void prepare_buffer_string_array(const char* const* const array)
{
    size_t bytesRead = 0;
    strBuf[0] = '\0';

    for (int i=0; array[i] != nullptr && bytesRead < kStrBufSize; ++i)
    {
        const std::size_t size = std::strlen(array[i]);

        if (bytesRead + size > kStrBufSize)
            break;

        std::strncpy(strBuf+bytesRead, array[i], kStrBufSize);
        bytesRead += size;
        strBuf[bytesRead] = '\n';
        bytesRead += 1;
    }

    strBuf[bytesRead] = '\0';
}

char* prepare_buffer_json_start()
{
    std::strcpy(jsonBuf, "{");
    return jsonBuf + 1;
}

char* prepare_buffer_json_add_uint(char* bufPtr, const char* const key, const uint value)
{
    prepare_buffer_uint(value);

    if (bufPtr != jsonBuf+1)
    {
        *bufPtr = ',';
        bufPtr += 1;
    }

    *bufPtr++ = '"';

    std::strcpy(bufPtr, key);
    bufPtr += std::strlen(key);

    *bufPtr++ = '"';
    *bufPtr++ = ':';

    std::strcpy(bufPtr, strBuf);
    bufPtr += std::strlen(strBuf);

    return bufPtr;
}

void prepare_buffer_json_end(char* const bufPtr)
{
    std::strcpy(bufPtr, "}");

    const std::size_t size = std::strlen(jsonBuf);
    std::snprintf(sizeBuf, kSizeBufSize, P_SIZE, size);
    sizeBuf[kSizeBufSize] = '\0';
}

void handle_carla_get_complete_license_text(const std::shared_ptr<Session> session)
{
    prepare_buffer_string(carla_get_complete_license_text());
    prepare_size();
    session->close(OK, strBuf, { { "Content-Length", sizeBuf } } );
}

void handle_carla_get_supported_file_extensions(const std::shared_ptr<Session> session)
{
    prepare_buffer_string_array(carla_get_supported_file_extensions());
    prepare_size();
    session->close(OK, strBuf, { { "Content-Length", sizeBuf } } );
}

void handle_carla_get_supported_features(const std::shared_ptr<Session> session)
{
    prepare_buffer_string_array(carla_get_supported_features());
    prepare_size();
    session->close(OK, strBuf, { { "Content-Length", sizeBuf } } );
}

void handle_carla_get_cached_plugin_count(const std::shared_ptr<Session> session)
{
    PluginType ptype = PLUGIN_LV2;
    const char* pluginPath = nullptr;

    prepare_buffer_uint(carla_get_cached_plugin_count(ptype, pluginPath));
    prepare_size();
    session->close(OK, strBuf, { { "Content-Length", sizeBuf } } );
}

void handle_carla_get_cached_plugin_info(const std::shared_ptr<Session> session)
{
    PluginType ptype = PLUGIN_LV2;
    const uint index = 0;

    // REMOVE THIS
    carla_get_cached_plugin_count(ptype, nullptr);

    const CarlaCachedPluginInfo* const info = carla_get_cached_plugin_info(ptype, index);

    char* bufPtr;
    bufPtr = prepare_buffer_json_start();
    bufPtr = prepare_buffer_json_add_uint(bufPtr, "category", info->category);
    bufPtr = prepare_buffer_json_add_uint(bufPtr, "hints", info->hints);
    bufPtr = prepare_buffer_json_add_uint(bufPtr, "audioIns", info->audioIns);
    bufPtr = prepare_buffer_json_add_uint(bufPtr, "audioOuts", info->audioOuts);
    bufPtr = prepare_buffer_json_add_uint(bufPtr, "midiIns", info->midiIns);
    bufPtr = prepare_buffer_json_add_uint(bufPtr, "midiOuts", info->midiOuts);
    bufPtr = prepare_buffer_json_add_uint(bufPtr, "parameterIns", info->parameterIns);
    bufPtr = prepare_buffer_json_add_uint(bufPtr, "parameterOuts", info->parameterOuts);

//     const char* name;
//     const char* label;
//     const char* maker;
//     const char* copyright;

    prepare_buffer_json_end(bufPtr);

    session->close(OK, jsonBuf, { { "Content-Length", sizeBuf } } );
}

// CARLA_EXPORT const CarlaCachedPluginInfo* carla_get_cached_plugin_info(PluginType ptype, uint index);

CARLA_BACKEND_END_NAMESPACE

void make_resource(Service& service,
                   const char* const path,
                   const std::function<void (const std::shared_ptr<Session>)>& callback)
{
    std::shared_ptr<Resource> resource = std::make_shared<Resource>();
    resource->set_path(path);
    resource->set_method_handler("GET", callback);
    service.publish(resource);
}

int main(int, const char**)
{
    CARLA_BACKEND_USE_NAMESPACE;

    Service service;
    make_resource(service, "/get_complete_license_text",     handle_carla_get_complete_license_text);
    make_resource(service, "/get_supported_file_extensions", handle_carla_get_supported_file_extensions);
    make_resource(service, "/get_supported_features",        handle_carla_get_supported_features);
    make_resource(service, "/get_cached_plugin_count",       handle_carla_get_cached_plugin_count);
    make_resource(service, "/get_cached_plugin_info",        handle_carla_get_cached_plugin_info);

    std::shared_ptr<Settings> settings = std::make_shared<Settings>();
    settings->set_port(2228);
    settings->set_default_header("Connection", "close");

    carla_stdout("Carla REST-API Server started");
    service.start(settings);
    return 0;
}
