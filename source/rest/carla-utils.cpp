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

#include "common.hpp"

#include "CarlaUtils.h"

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_get_complete_license_text(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_string(carla_get_complete_license_text());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_supported_file_extensions(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_string_array(carla_get_supported_file_extensions());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_supported_features(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_string_array(carla_get_supported_features());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_cached_plugin_count(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int ptype = std::atoi(request->get_query_parameter("ptype").c_str());
    CARLA_SAFE_ASSERT_RETURN(ptype >= PLUGIN_NONE && ptype <= PLUGIN_JACK,)

    const std::string pluginPath = request->get_query_parameter("pluginPath");

    const char* const buf = str_buf_uint(carla_get_cached_plugin_count(static_cast<PluginType>(ptype),
                                                                       pluginPath.c_str()));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_cached_plugin_info(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int ptype = std::atoi(request->get_query_parameter("ptype").c_str());
    CARLA_SAFE_ASSERT_RETURN(ptype >= PLUGIN_NONE && ptype <= PLUGIN_JACK,)

    const int index = std::atoi(request->get_query_parameter("index").c_str());
    CARLA_SAFE_ASSERT_RETURN(index >= 0 /*&& index < INT_MAX*/,)

    const CarlaCachedPluginInfo* const info = carla_get_cached_plugin_info(static_cast<PluginType>(ptype),
                                                                           static_cast<uint>(index));

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_bool(jsonBuf, "valid", info->valid);
    jsonBuf = json_buf_add_uint(jsonBuf, "category", info->category);
    jsonBuf = json_buf_add_uint(jsonBuf, "hints", info->hints);
    jsonBuf = json_buf_add_uint(jsonBuf, "audioIns", info->audioIns);
    jsonBuf = json_buf_add_uint(jsonBuf, "audioOuts", info->audioOuts);
    jsonBuf = json_buf_add_uint(jsonBuf, "midiIns", info->midiIns);
    jsonBuf = json_buf_add_uint(jsonBuf, "midiOuts", info->midiOuts);
    jsonBuf = json_buf_add_uint(jsonBuf, "parameterIns", info->parameterIns);
    jsonBuf = json_buf_add_uint(jsonBuf, "parameterOuts", info->parameterOuts);
    jsonBuf = json_buf_add_string(jsonBuf, "name", info->name);
    jsonBuf = json_buf_add_string(jsonBuf, "label", info->label);
    jsonBuf = json_buf_add_string(jsonBuf, "maker", info->maker);
    jsonBuf = json_buf_add_string(jsonBuf, "copyright", info->copyright);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------
