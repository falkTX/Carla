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

#include "CarlaMutex.hpp"
#include "CarlaStringList.hpp"

// -------------------------------------------------------------------------------------------------------------------

std::vector<std::shared_ptr<Session>> gSessions;

CarlaStringList gSessionMessages;
CarlaMutex gSessionMessagesMutex;

// -------------------------------------------------------------------------------------------------------------------

void send_server_side_message(const char* const message)
{
    const CarlaMutexLocker cml(gSessionMessagesMutex);

    gSessionMessages.append(message);
}

// -------------------------------------------------------------------------------------------------------------------

static void register_server_side_handler(const std::shared_ptr<Session> session)
{
    const auto headers = std::multimap<std::string, std::string> {
        { "Connection", "keep-alive" },
        { "Cache-Control", "no-cache" },
        { "Content-Type", "text/event-stream" },
        { "Access-Control-Allow-Origin", "*" } //Only required for demo purposes.
    };

    session->yield(OK, headers, [](const std::shared_ptr<Session> rsession) {
        gSessions.push_back(rsession);
    });
}

static void event_stream_handler(void)
{
    gSessions.erase(
            std::remove_if(gSessions.begin(), gSessions.end(),
                                [](const std::shared_ptr<Session> &a) {
                                    return a->is_closed();
                                }),
            gSessions.end());

    CarlaStringList messages;

    {
        const CarlaMutexLocker cml(gSessionMessagesMutex);

        if (gSessionMessages.count() > 0)
            gSessionMessages.moveTo(messages);
    }

    for (auto message : messages)
    {
        std::puts(message);

        for (auto session : gSessions)
            session->yield(OK, message);
    }
}

// -------------------------------------------------------------------------------------------------------------------

static void make_resource(Service& service,
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

    // server-side messages
    {
        std::shared_ptr<Resource> resource = std::make_shared<Resource>();
        resource->set_path("/stream");
        resource->set_method_handler("GET", register_server_side_handler);
        service.publish(resource);
    }

    // carla-host
    make_resource(service, "/get_engine_driver_count", handle_carla_get_engine_driver_count);
    make_resource(service, "/get_engine_driver_name/{index: .*}", handle_carla_get_engine_driver_name);
    make_resource(service, "/get_engine_driver_device_names/{index: .*}", handle_carla_get_engine_driver_device_names);
    make_resource(service, "/get_engine_driver_device_info/{index: .*}/{name: .*}", handle_carla_get_engine_driver_device_info);

    make_resource(service, "/engine_init/{driverName: .*}/{clientName: .*}", handle_carla_engine_init);
    make_resource(service, "/engine_close", handle_carla_engine_close);
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

    // schedule events
    service.schedule(engine_idle_handler, std::chrono::milliseconds(33)); // ~30Hz
    service.schedule(event_stream_handler, std::chrono::milliseconds(500));

    std::shared_ptr<Settings> settings = std::make_shared<Settings>();
    settings->set_port(2228);
    settings->set_default_header("Connection", "close");

    carla_stdout("Carla REST-API Server started");
    service.start(settings);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------
