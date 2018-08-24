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

#include "common.hpp"

#include "carla-host.cpp"
#include "carla-utils.cpp"

// -------------------------------------------------------------------------------------------------------------------

void make_resource(Service& service,
                   const char* const path,
                   const std::function<void (const std::shared_ptr<Session>)>& callback)
{
    std::shared_ptr<Resource> resource = std::make_shared<Resource>();
    resource->set_path(path);
    resource->set_method_handler("GET", callback);
    service.publish(resource);
}

// -------------------------------------------------------------------------------------------------------------------

int main(int, const char**)
{
    Service service;

    // carla-host
    make_resource(service, "/get_engine_driver_count", handle_carla_get_engine_driver_count);
    make_resource(service, "/get_engine_driver_name/{index: .*}", handle_carla_get_engine_driver_name);
    make_resource(service, "/get_engine_driver_device_names/{index: .*}", handle_carla_get_engine_driver_device_names);
    make_resource(service, "/get_engine_driver_device_info/{index: .*}/{name: .*}", handle_carla_get_engine_driver_device_info);

    make_resource(service, "/engine_init/{driverName: .*}/{clientName: .*}", handle_carla_engine_init);
    make_resource(service, "/engine_close", handle_carla_engine_close);
    make_resource(service, "/engine_idle", handle_carla_engine_idle);
    make_resource(service, "/is_engine_running", handle_carla_is_engine_running);
    make_resource(service, "/set_engine_about_to_close", handle_carla_set_engine_about_to_close);

    // ...
    make_resource(service, "/get_last_error", handle_carla_get_last_error);

    // carla-utils
    make_resource(service, "/get_complete_license_text", handle_carla_get_complete_license_text);
    make_resource(service, "/get_supported_file_extensions", handle_carla_get_supported_file_extensions);
    make_resource(service, "/get_supported_features", handle_carla_get_supported_features);
    make_resource(service, "/get_cached_plugin_count/{ptype: .*}/{pluginPath: .*}", handle_carla_get_cached_plugin_count);
    make_resource(service, "/get_cached_plugin_info/{ptype: .*}/{index: .*}", handle_carla_get_cached_plugin_info);

    std::shared_ptr<Settings> settings = std::make_shared<Settings>();
    settings->set_port(2228);
    settings->set_default_header("Connection", "close");

    carla_stdout("Carla REST-API Server started");
    service.start(settings);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------
