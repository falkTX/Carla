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
    static bool firstInit = true;

    if (firstInit)
    {
        firstInit = false;
        carla_stdout("Carla REST-API Server started");
    }

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
        // std::puts(message);

        for (auto session : gSessions)
            session->yield(OK, message);
    }

    if (const uint count = carla_get_current_plugin_count())
    {
        char msgBuf[1024];
        float* peaks;

        for (uint i=0; i<count; ++i)
        {
            peaks = carla_get_peak_values(i);
            CARLA_SAFE_ASSERT_BREAK(peaks != nullptr);

            std::snprintf(msgBuf, 1023, "Peaks: %u %f %f %f %f\n", i, peaks[0], peaks[1], peaks[2], peaks[3]);
            msgBuf[1023] = '\0';

            for (auto session : gSessions)
                session->yield(OK, msgBuf);
        }
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
    make_resource(service, "/get_engine_driver_name", handle_carla_get_engine_driver_name);
    make_resource(service, "/get_engine_driver_device_names", handle_carla_get_engine_driver_device_names);
    make_resource(service, "/get_engine_driver_device_info", handle_carla_get_engine_driver_device_info);

    make_resource(service, "/engine_init", handle_carla_engine_init);
    make_resource(service, "/engine_close", handle_carla_engine_close);
    make_resource(service, "/is_engine_running", handle_carla_is_engine_running);
    make_resource(service, "/set_engine_about_to_close", handle_carla_set_engine_about_to_close);

    make_resource(service, "/set_engine_option", handle_carla_set_engine_option);
    make_resource(service, "/load_file", handle_carla_load_file);
    make_resource(service, "/load_project", handle_carla_load_project);
    make_resource(service, "/save_project", handle_carla_save_project);

    make_resource(service, "/patchbay_connect", handle_carla_patchbay_connect);
    make_resource(service, "/patchbay_disconnect", handle_carla_patchbay_disconnect);
    make_resource(service, "/patchbay_refresh", handle_carla_patchbay_refresh);

    make_resource(service, "/transport_play", handle_carla_transport_play);
    make_resource(service, "/transport_pause", handle_carla_transport_pause);
    make_resource(service, "/transport_bpm", handle_carla_transport_bpm);
    make_resource(service, "/transport_relocate", handle_carla_transport_relocate);
    make_resource(service, "/get_current_transport_frame", handle_carla_get_current_transport_frame);
    make_resource(service, "/get_transport_info", handle_carla_get_transport_info);

    make_resource(service, "/get_current_plugin_count", handle_carla_get_current_plugin_count);
    make_resource(service, "/get_max_plugin_number", handle_carla_get_max_plugin_number);
    make_resource(service, "/add_plugin", handle_carla_add_plugin);
    make_resource(service, "/remove_plugin", handle_carla_remove_plugin);
    make_resource(service, "/remove_all_plugins", handle_carla_remove_all_plugins);

    make_resource(service, "/rename_plugin", handle_carla_rename_plugin);
    make_resource(service, "/clone_plugin", handle_carla_clone_plugin);
    make_resource(service, "/replace_plugin", handle_carla_replace_plugin);
    make_resource(service, "/switch_plugins", handle_carla_switch_plugins);

    make_resource(service, "/load_plugin_state", handle_carla_load_plugin_state);
    make_resource(service, "/save_plugin_state", handle_carla_save_plugin_state);
    make_resource(service, "/export_plugin_lv2", handle_carla_export_plugin_lv2);

    make_resource(service, "/get_plugin_info", handle_carla_get_plugin_info);
    make_resource(service, "/get_audio_port_count_info", handle_carla_get_audio_port_count_info);
    make_resource(service, "/get_midi_port_count_info", handle_carla_get_midi_port_count_info);
    make_resource(service, "/get_parameter_count_info", handle_carla_get_parameter_count_info);
    make_resource(service, "/get_parameter_info", handle_carla_get_parameter_info);
    make_resource(service, "/get_parameter_scalepoint_info", handle_carla_get_parameter_scalepoint_info);

    make_resource(service, "/get_parameter_data", handle_carla_get_parameter_data);
    make_resource(service, "/get_parameter_ranges", handle_carla_get_parameter_ranges);
    make_resource(service, "/get_midi_program_data", handle_carla_get_midi_program_data);
    make_resource(service, "/get_custom_data", handle_carla_get_custom_data);
    make_resource(service, "/get_custom_data_value", handle_carla_get_custom_data_value);
    make_resource(service, "/get_chunk_data", handle_carla_get_chunk_data);

    make_resource(service, "/get_parameter_count", handle_carla_get_parameter_count);
    make_resource(service, "/get_program_count", handle_carla_get_program_count);
    make_resource(service, "/get_midi_program_count", handle_carla_get_midi_program_count);
    make_resource(service, "/get_custom_data_count", handle_carla_get_custom_data_count);

    make_resource(service, "/get_parameter_text", handle_carla_get_parameter_text);
    make_resource(service, "/get_program_name", handle_carla_get_program_name);
    make_resource(service, "/get_midi_program_name", handle_carla_get_midi_program_name);
    make_resource(service, "/get_real_plugin_name", handle_carla_get_real_plugin_name);

    make_resource(service, "/get_current_program_index", handle_carla_get_current_program_index);
    make_resource(service, "/get_current_midi_program_index", handle_carla_get_current_midi_program_index);

    make_resource(service, "/get_default_parameter_value", handle_carla_get_default_parameter_value);
    make_resource(service, "/get_current_parameter_value", handle_carla_get_current_parameter_value);
    make_resource(service, "/get_internal_parameter_value", handle_carla_get_internal_parameter_value);
    make_resource(service, "/get_input_peak_value", handle_carla_get_input_peak_value);
    make_resource(service, "/get_output_peak_value", handle_carla_get_output_peak_value);

    make_resource(service, "/set_active", handle_carla_set_active);
    make_resource(service, "/set_drywet", handle_carla_set_drywet);
    make_resource(service, "/set_volume", handle_carla_set_volume);
    make_resource(service, "/set_balance_left", handle_carla_set_balance_left);
    make_resource(service, "/set_balance_right", handle_carla_set_balance_right);
    make_resource(service, "/set_panning", handle_carla_set_panning);
    make_resource(service, "/set_ctrl_channel", handle_carla_set_ctrl_channel);
    make_resource(service, "/set_option", handle_carla_set_option);

    make_resource(service, "/set_parameter_value", handle_carla_set_parameter_value);
    make_resource(service, "/set_parameter_midi_channel", handle_carla_set_parameter_midi_channel);
    make_resource(service, "/set_parameter_midi_cc", handle_carla_set_parameter_midi_cc);
    make_resource(service, "/set_program", handle_carla_set_program);
    make_resource(service, "/set_midi_program", handle_carla_set_midi_program);
    make_resource(service, "/set_custom_data", handle_carla_set_custom_data);
    make_resource(service, "/set_chunk_data", handle_carla_set_chunk_data);

    make_resource(service, "/prepare_for_save", handle_carla_prepare_for_save);
    make_resource(service, "/reset_parameters", handle_carla_reset_parameters);
    make_resource(service, "/randomize_parameters", handle_carla_randomize_parameters);
    make_resource(service, "/send_midi_note", handle_carla_send_midi_note);

    make_resource(service, "/get_buffer_size", handle_carla_get_buffer_size);
    make_resource(service, "/get_sample_rate", handle_carla_get_sample_rate);
    make_resource(service, "/get_last_error", handle_carla_get_last_error);
    make_resource(service, "/get_host_osc_url_tcp", handle_carla_get_host_osc_url_tcp);
    make_resource(service, "/get_host_osc_url_udp", handle_carla_get_host_osc_url_udp);

    // carla-utils
    make_resource(service, "/get_complete_license_text", handle_carla_get_complete_license_text);
    make_resource(service, "/get_supported_file_extensions", handle_carla_get_supported_file_extensions);
    make_resource(service, "/get_supported_features", handle_carla_get_supported_features);
    make_resource(service, "/get_cached_plugin_count", handle_carla_get_cached_plugin_count);
    make_resource(service, "/get_cached_plugin_info", handle_carla_get_cached_plugin_info);

    // schedule events
    service.schedule(engine_idle_handler); // FIXME, crashes on fast times, but we need ~30Hz for OSC..
    service.schedule(event_stream_handler, std::chrono::milliseconds(500));

    std::shared_ptr<Settings> settings = std::make_shared<Settings>();
    settings->set_port(2228);
    settings->set_default_header("Connection", "close");

    service.start(settings);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------
