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

#include "CarlaHost.h"

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_get_engine_driver_count(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_uint(carla_get_engine_driver_count());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_engine_driver_name(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int index = std::atoi(request->get_path_parameter("index").c_str());
    CARLA_SAFE_ASSERT_RETURN(index >= 0 /*&& index < INT_MAX*/,)

    const char* const buf = str_buf_string(carla_get_engine_driver_name(index));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_engine_driver_device_names(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int index = std::atoi(request->get_path_parameter("index").c_str());
    CARLA_SAFE_ASSERT_RETURN(index >= 0 /*&& index < INT_MAX*/,)

    const char* const buf = str_buf_string_array(carla_get_engine_driver_device_names(index));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_engine_driver_device_info(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int index = std::atoi(request->get_path_parameter("index").c_str());
    CARLA_SAFE_ASSERT_RETURN(index >= 0 /*&& index < INT_MAX*/,)

    const std::string name = request->get_path_parameter("name");

    const EngineDriverDeviceInfo* const info = carla_get_engine_driver_device_info(index, name.c_str());

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_uint(jsonBuf, "hints", info->hints);

    // TODO
    //jsonBuf = json_buf_add_uint(jsonBuf, "bufferSizes", info->bufferSizes);
    //jsonBuf = json_buf_add_uint(jsonBuf, "sampleRates", info->sampleRates);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------------------------
