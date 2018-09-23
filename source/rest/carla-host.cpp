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
#include "CarlaBackendUtils.hpp"

// -------------------------------------------------------------------------------------------------------------------

static bool gEngineRunning = false;

// -------------------------------------------------------------------------------------------------------------------

static void EngineCallback(void* ptr, EngineCallbackOpcode action, uint pluginId, int value1, int value2, float value3, const char* valueStr)
{
    carla_debug("EngineCallback(%p, %u:%s, %u, %i, %i, %f, %s)",
                ptr, (uint)action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valueStr);

    char msgBuf[1024];
    std::snprintf(msgBuf, 1023, "Carla: %u %u %i %i %f %s", action, pluginId, value1, value2, value3, valueStr);
    msgBuf[1023] = '\0';

    switch (action)
    {
    case ENGINE_CALLBACK_ENGINE_STARTED:
        gEngineRunning = true;
        break;
    case ENGINE_CALLBACK_ENGINE_STOPPED:
    case ENGINE_CALLBACK_QUIT:
        gEngineRunning = false;
        break;
    default:
        break;
    }

    return send_server_side_message(msgBuf);

    // maybe unused
    (void)ptr;
}

static const char* FileCallback(void* ptr, FileCallbackOpcode action, bool isDir, const char* title, const char* filter)
{
    carla_stdout("FileCallback(%p, %u:%s, %s, %s, %s)",
                 ptr, (uint)action, FileCallbackOpcode(action), bool2str(isDir), title, filter);

    char msgBuf[1024];
    std::snprintf(msgBuf, 1023, "fc %u %i \"%s\" \"%s\"", action, isDir, title, filter);
    msgBuf[1023] = '\0';

    send_server_side_message(msgBuf);

    // FIXME, need to wait for response!
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_get_engine_driver_count(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_uint(carla_get_engine_driver_count());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_engine_driver_name(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int index = std::atoi(request->get_query_parameter("index").c_str());
    CARLA_SAFE_ASSERT_RETURN(index >= 0 /*&& index < INT_MAX*/,)

    const char* const buf = str_buf_string(carla_get_engine_driver_name(index));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_engine_driver_device_names(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int index = std::atoi(request->get_query_parameter("index").c_str());
    CARLA_SAFE_ASSERT_RETURN(index >= 0 /*&& index < INT_MAX*/,)

    const char* const buf = str_buf_string_array(carla_get_engine_driver_device_names(index));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_engine_driver_device_info(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int index = std::atoi(request->get_query_parameter("index").c_str());
    CARLA_SAFE_ASSERT_RETURN(index >= 0 /*&& index < INT_MAX*/,)

    const std::string name = request->get_query_parameter("name");

    const EngineDriverDeviceInfo* const info = carla_get_engine_driver_device_info(index, name.c_str());

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_uint(jsonBuf, "hints", info->hints);
    jsonBuf = json_buf_add_uint_array(jsonBuf, "bufferSizes", info->bufferSizes);
    jsonBuf = json_buf_add_float_array(jsonBuf, "sampleRates", info->sampleRates);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_engine_init(const std::shared_ptr<Session> session)
{
    // setup callbacks
    carla_set_engine_callback(EngineCallback, nullptr);
    carla_set_file_callback(FileCallback, nullptr);

    // handle request now
    const std::shared_ptr<const Request> request = session->get_request();

    const std::string driverName = request->get_query_parameter("driverName");
    const std::string clientName = request->get_query_parameter("clientName");

    const char* const buf = str_buf_bool(carla_engine_init(driverName.c_str(), clientName.c_str()));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_engine_close(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_bool(carla_engine_close());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_is_engine_running(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_bool(carla_is_engine_running());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_set_engine_about_to_close(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_bool(carla_set_engine_about_to_close());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_set_engine_option(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int option = std::atol(request->get_query_parameter("option").c_str());
    CARLA_SAFE_ASSERT_RETURN(option >= ENGINE_OPTION_DEBUG && option < ENGINE_OPTION_DEBUG_CONSOLE_OUTPUT,)

    const int value = std::atol(request->get_query_parameter("value").c_str());
    CARLA_SAFE_ASSERT_RETURN(value >= 0,)

    const std::string valueStr = request->get_query_parameter("valueStr");

    carla_set_engine_option(static_cast<EngineOption>(option), value, valueStr.c_str());
    session->close(OK);
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_load_file(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const std::string filename = request->get_query_parameter("filename");

    const char* const buf = str_buf_bool(carla_load_file(filename.c_str()));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_load_project(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const std::string filename = request->get_query_parameter("filename");

    const char* const buf = str_buf_bool(carla_load_project(filename.c_str()));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_save_project(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const std::string filename = request->get_query_parameter("filename");

    const char* const buf = str_buf_bool(carla_save_project(filename.c_str()));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_patchbay_connect(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int groupIdA = std::atoi(request->get_query_parameter("groupIdA").c_str());
    CARLA_SAFE_ASSERT_RETURN(groupIdA >= 0,)

    const int portIdA = std::atoi(request->get_query_parameter("portIdA").c_str());
    CARLA_SAFE_ASSERT_RETURN(portIdA >= 0,)

    const int groupIdB = std::atoi(request->get_query_parameter("groupIdB").c_str());
    CARLA_SAFE_ASSERT_RETURN(groupIdB >= 0,)

    const int portIdB = std::atoi(request->get_query_parameter("portIdB").c_str());
    CARLA_SAFE_ASSERT_RETURN(portIdB >= 0,)

    const char* const buf = str_buf_bool(carla_patchbay_connect(groupIdA, portIdA, groupIdB, portIdB));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_patchbay_disconnect(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int connectionId = std::atoi(request->get_query_parameter("connectionId").c_str());
    CARLA_SAFE_ASSERT_RETURN(connectionId >= 0,)

    const char* const buf = str_buf_bool(carla_patchbay_disconnect(connectionId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_patchbay_refresh(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int external = std::atoi(request->get_query_parameter("external").c_str());
    CARLA_SAFE_ASSERT_RETURN(external == 0 || external == 1,)

    const char* const buf = str_buf_bool(carla_patchbay_refresh(external));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_transport_play(const std::shared_ptr<Session> session)
{
    carla_transport_play();
    session->close(OK);
}

void handle_carla_transport_pause(const std::shared_ptr<Session> session)
{
    carla_transport_pause();
    session->close(OK);
}

void handle_carla_transport_bpm(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const double bpm = std::atof(request->get_query_parameter("bpm").c_str());
    CARLA_SAFE_ASSERT_RETURN(bpm > 0.0, session->close(OK)) // FIXME

    carla_transport_bpm(bpm);
    session->close(OK);
}

void handle_carla_transport_relocate(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const long int frame = std::atol(request->get_query_parameter("frame").c_str());
    CARLA_SAFE_ASSERT_RETURN(frame >= 0,)

    carla_transport_relocate(frame);
    session->close(OK);
}

void handle_carla_get_current_transport_frame(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_uint64(carla_get_current_transport_frame());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_transport_info(const std::shared_ptr<Session> session)
{
    const CarlaTransportInfo* const info = carla_get_transport_info();

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_bool(jsonBuf, "playing", info->playing);
    jsonBuf = json_buf_add_uint64(jsonBuf, "frame", info->frame);
    jsonBuf = json_buf_add_int(jsonBuf, "bar", info->bar);
    jsonBuf = json_buf_add_int(jsonBuf, "beat", info->beat);
    jsonBuf = json_buf_add_int(jsonBuf, "tick", info->tick);
    jsonBuf = json_buf_add_float(jsonBuf, "bpm", info->bpm);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_get_current_plugin_count(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_uint(carla_get_current_plugin_count());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_max_plugin_number(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_uint(carla_get_max_plugin_number());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_add_plugin(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const long int btype = std::atoi(request->get_query_parameter("btype").c_str());
    CARLA_SAFE_ASSERT_RETURN(btype >= BINARY_NONE && btype <= BINARY_OTHER,)

    const long int ptype = std::atoi(request->get_query_parameter("ptype").c_str());
    CARLA_SAFE_ASSERT_RETURN(ptype >= PLUGIN_NONE && ptype <= PLUGIN_JACK,)

    const std::string filename = request->get_query_parameter("filename");
    const std::string name = request->get_query_parameter("name");
    const std::string label = request->get_query_parameter("label");

    const long int uniqueId = std::atol(request->get_query_parameter("uniqueId").c_str());
    CARLA_SAFE_ASSERT_RETURN(uniqueId >= 0,)

    const int options = std::atoi(request->get_query_parameter("options").c_str());
    CARLA_SAFE_ASSERT_RETURN(options >= 0,)

    const char* const buf = str_buf_bool(carla_add_plugin(static_cast<BinaryType>(btype),
                                         static_cast<PluginType>(ptype),
                                         filename.c_str(),
                                         name.c_str(),
                                         label.c_str(),
                                         uniqueId,
                                         nullptr,
                                         options));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_remove_plugin(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const char* const buf = str_buf_bool(carla_remove_plugin(pluginId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_remove_all_plugins(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_bool(carla_remove_all_plugins());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_rename_plugin(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const std::string newName = request->get_query_parameter("newName");

    const char* const buf = carla_rename_plugin(pluginId, newName.c_str());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_clone_plugin(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const char* const buf = str_buf_bool(carla_clone_plugin(pluginId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_replace_plugin(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const char* const buf = str_buf_bool(carla_replace_plugin(pluginId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_switch_plugins(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginIdA = std::atol(request->get_query_parameter("pluginIdA").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginIdA >= 0,)

    const int pluginIdB = std::atol(request->get_query_parameter("pluginIdB").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginIdB >= 0,)

    const char* const buf = str_buf_bool(carla_switch_plugins(pluginIdA, pluginIdB));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_load_plugin_state(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const std::string filename = request->get_query_parameter("filename");

    const char* const buf = str_buf_bool(carla_load_plugin_state(pluginId, filename.c_str()));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_save_plugin_state(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const std::string filename = request->get_query_parameter("filename");

    const char* const buf = str_buf_bool(carla_save_plugin_state(pluginId, filename.c_str()));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_export_plugin_lv2(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const std::string lv2path = request->get_query_parameter("lv2path");

    const char* const buf = str_buf_bool(carla_export_plugin_lv2(pluginId, lv2path.c_str()));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_get_plugin_info(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const CarlaPluginInfo* const info = carla_get_plugin_info(pluginId);

    // running remotely, so we cannot show custom UI or inline display
    const uint hints = info->hints & ~(PLUGIN_HAS_CUSTOM_UI|PLUGIN_HAS_INLINE_DISPLAY);

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_uint(jsonBuf, "type", info->type);
    jsonBuf = json_buf_add_uint(jsonBuf, "category", info->category);
    jsonBuf = json_buf_add_uint(jsonBuf, "hints", hints);
    jsonBuf = json_buf_add_uint(jsonBuf, "optionsAvailable", info->optionsAvailable);
    jsonBuf = json_buf_add_uint(jsonBuf, "optionsEnabled", info->optionsEnabled);
    jsonBuf = json_buf_add_string(jsonBuf, "filename", info->filename);
    jsonBuf = json_buf_add_string(jsonBuf, "name", info->name);
    jsonBuf = json_buf_add_string(jsonBuf, "label", info->label);
    jsonBuf = json_buf_add_string(jsonBuf, "maker", info->maker);
    jsonBuf = json_buf_add_string(jsonBuf, "copyright", info->copyright);
    jsonBuf = json_buf_add_string(jsonBuf, "iconName", info->iconName);
    jsonBuf = json_buf_add_int64(jsonBuf, "uniqueId", info->uniqueId);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_audio_port_count_info(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const CarlaPortCountInfo* const info = carla_get_audio_port_count_info(pluginId);

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_uint(jsonBuf, "ins", info->ins);
    jsonBuf = json_buf_add_uint(jsonBuf, "outs", info->outs);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_midi_port_count_info(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const CarlaPortCountInfo* const info = carla_get_midi_port_count_info(pluginId);

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_uint(jsonBuf, "ins", info->ins);
    jsonBuf = json_buf_add_uint(jsonBuf, "outs", info->outs);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_parameter_count_info(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const CarlaPortCountInfo* const info = carla_get_parameter_count_info(pluginId);

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_uint(jsonBuf, "ins", info->ins);
    jsonBuf = json_buf_add_uint(jsonBuf, "outs", info->outs);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_parameter_info(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int parameterId = std::atoi(request->get_query_parameter("parameterId").c_str());
    CARLA_SAFE_ASSERT_RETURN(parameterId >= 0,)

    const CarlaParameterInfo* const info = carla_get_parameter_info(pluginId, parameterId);

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_string(jsonBuf, "name", info->name);
    jsonBuf = json_buf_add_string(jsonBuf, "symbol", info->symbol);
    jsonBuf = json_buf_add_string(jsonBuf, "unit", info->unit);
    jsonBuf = json_buf_add_uint(jsonBuf, "scalePointCount", info->scalePointCount);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_parameter_scalepoint_info(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int parameterId = std::atoi(request->get_query_parameter("parameterId").c_str());
    CARLA_SAFE_ASSERT_RETURN(parameterId >= 0,)

    const int scalePointId = std::atoi(request->get_query_parameter("scalePointId").c_str());
    CARLA_SAFE_ASSERT_RETURN(scalePointId >= 0,)

    const CarlaScalePointInfo* const info = carla_get_parameter_scalepoint_info(pluginId, parameterId, scalePointId);

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_float(jsonBuf, "value", info->value);
    jsonBuf = json_buf_add_string(jsonBuf, "label", info->label);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_parameter_data(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int parameterId = std::atoi(request->get_query_parameter("parameterId").c_str());
    CARLA_SAFE_ASSERT_RETURN(parameterId >= 0,)

    const ParameterData* const info = carla_get_parameter_data(pluginId, parameterId);

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_uint(jsonBuf, "type", info->type);
    jsonBuf = json_buf_add_uint(jsonBuf, "hints", info->hints);
    jsonBuf = json_buf_add_int(jsonBuf, "index", info->index);
    jsonBuf = json_buf_add_int(jsonBuf, "rindex", info->rindex);
    jsonBuf = json_buf_add_int(jsonBuf, "midiCC", info->midiCC);
    jsonBuf = json_buf_add_uint(jsonBuf, "midiChannel", info->midiChannel);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_parameter_ranges(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int parameterId = std::atoi(request->get_query_parameter("parameterId").c_str());
    CARLA_SAFE_ASSERT_RETURN(parameterId >= 0,)

    const ParameterRanges* const info = carla_get_parameter_ranges(pluginId, parameterId);

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_float(jsonBuf, "def", info->def);
    jsonBuf = json_buf_add_float(jsonBuf, "min", info->min);
    jsonBuf = json_buf_add_float(jsonBuf, "max", info->max);
    jsonBuf = json_buf_add_float(jsonBuf, "step", info->step);
    jsonBuf = json_buf_add_float(jsonBuf, "stepSmall", info->stepSmall);
    jsonBuf = json_buf_add_float(jsonBuf, "stepLarge", info->stepLarge);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_midi_program_data(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int midiProgramId = std::atoi(request->get_query_parameter("midiProgramId").c_str());
    CARLA_SAFE_ASSERT_RETURN(midiProgramId >= 0,)

    const MidiProgramData* const info = carla_get_midi_program_data(pluginId, midiProgramId);

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_uint(jsonBuf, "bank", info->bank);
    jsonBuf = json_buf_add_uint(jsonBuf, "program", info->program);
    jsonBuf = json_buf_add_string(jsonBuf, "name", info->name);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_custom_data(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int customDataId = std::atoi(request->get_query_parameter("customDataId").c_str());
    CARLA_SAFE_ASSERT_RETURN(customDataId >= 0,)

    const CustomData* const info = carla_get_custom_data(pluginId, customDataId);

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_string(jsonBuf, "type", info->type);
    jsonBuf = json_buf_add_string(jsonBuf, "key", info->key);
    jsonBuf = json_buf_add_string(jsonBuf, "value", info->value);

    const char* const buf = json_buf_end(jsonBuf);
    puts(buf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_custom_data_value(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const std::string type = request->get_query_parameter("type");
    const std::string key = request->get_query_parameter("key");

    const char* const buf = carla_get_custom_data_value(pluginId, type.c_str(), key.c_str());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_chunk_data(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const char* const buf = carla_get_chunk_data(pluginId);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_get_parameter_count(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const char* const buf = str_buf_uint(carla_get_parameter_count(pluginId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_program_count(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const char* const buf = str_buf_uint(carla_get_program_count(pluginId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_midi_program_count(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const char* const buf = str_buf_uint(carla_get_midi_program_count(pluginId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_custom_data_count(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const char* const buf = str_buf_uint(carla_get_custom_data_count(pluginId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_parameter_text(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int parameterId = std::atoi(request->get_query_parameter("parameterId").c_str());
    CARLA_SAFE_ASSERT_RETURN(parameterId >= 0,)

    const char* const buf = carla_get_parameter_text(pluginId, parameterId);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_program_name(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int programId = std::atoi(request->get_query_parameter("programId").c_str());
    CARLA_SAFE_ASSERT_RETURN(programId >= 0,)

    const char* const buf = carla_get_program_name(pluginId, programId);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_midi_program_name(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int midiProgramId = std::atoi(request->get_query_parameter("midiProgramId").c_str());
    CARLA_SAFE_ASSERT_RETURN(midiProgramId >= 0,)

    const char* const buf = carla_get_midi_program_name(pluginId, midiProgramId);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_real_plugin_name(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const char* const buf = carla_get_real_plugin_name(pluginId);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_current_program_index(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const char* const buf = str_buf_uint(carla_get_current_program_index(pluginId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_current_midi_program_index(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const char* const buf = str_buf_uint(carla_get_current_midi_program_index(pluginId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_default_parameter_value(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int parameterId = std::atoi(request->get_query_parameter("parameterId").c_str());
    CARLA_SAFE_ASSERT_RETURN(parameterId >= 0,)

    const char* const buf = str_buf_float(carla_get_default_parameter_value(pluginId, parameterId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_current_parameter_value(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int parameterId = std::atoi(request->get_query_parameter("parameterId").c_str());
    CARLA_SAFE_ASSERT_RETURN(parameterId >= 0,)

    const char* const buf = str_buf_float(carla_get_current_parameter_value(pluginId, parameterId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_internal_parameter_value(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int parameterId = std::atoi(request->get_query_parameter("parameterId").c_str());
    CARLA_SAFE_ASSERT_RETURN(parameterId > PARAMETER_MAX,);

    const char* const buf = str_buf_float(carla_get_internal_parameter_value(pluginId, parameterId));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_input_peak_value(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int isLeft = std::atoi(request->get_query_parameter("isLeft").c_str());
    CARLA_SAFE_ASSERT_RETURN(isLeft == 0 || isLeft == 1,)

    const char* const buf = str_buf_float(carla_get_input_peak_value(pluginId, isLeft));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_output_peak_value(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int isLeft = std::atoi(request->get_query_parameter("isLeft").c_str());
    CARLA_SAFE_ASSERT_RETURN(isLeft == 0 || isLeft == 1,)

    const char* const buf = str_buf_float(carla_get_output_peak_value(pluginId, isLeft));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_set_active(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int onOff = std::atoi(request->get_query_parameter("onOff").c_str());
    CARLA_SAFE_ASSERT_RETURN(onOff == 0 || onOff == 1,)

    carla_set_active(pluginId, onOff);
    session->close(OK);
}

void handle_carla_set_drywet(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const double value = std::atof(request->get_query_parameter("value").c_str());
    CARLA_SAFE_ASSERT_RETURN(value >= 0.0 && value <= 1.0,)

    carla_set_drywet(pluginId, value);
    session->close(OK);
}

void handle_carla_set_volume(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const double value = std::atof(request->get_query_parameter("value").c_str());
    CARLA_SAFE_ASSERT_RETURN(value >= 0.0 && value <= 1.27,)

    carla_set_volume(pluginId, value);
    session->close(OK);
}

void handle_carla_set_balance_left(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const double value = std::atof(request->get_query_parameter("value").c_str());
    CARLA_SAFE_ASSERT_RETURN(value >= -1.0 && value <= 1.0,)

    carla_set_balance_left(pluginId, value);
    session->close(OK);
}

void handle_carla_set_balance_right(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const double value = std::atof(request->get_query_parameter("value").c_str());
    CARLA_SAFE_ASSERT_RETURN(value >= -1.0 && value <= 1.0,)

    carla_set_balance_right(pluginId, value);
    session->close(OK);
}

void handle_carla_set_panning(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const double value = std::atof(request->get_query_parameter("value").c_str());
    CARLA_SAFE_ASSERT_RETURN(value >= -1.0 && value <= 1.0,)

    carla_set_panning(pluginId, value);
    session->close(OK);
}

void handle_carla_set_ctrl_channel(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int channel = std::atoi(request->get_query_parameter("channel").c_str());
    CARLA_SAFE_ASSERT_RETURN(channel < 16,)

    carla_set_ctrl_channel(pluginId, channel);
    session->close(OK);
}

void handle_carla_set_option(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int option = std::atoi(request->get_query_parameter("option").c_str());
    CARLA_SAFE_ASSERT_RETURN(option >= 0,)

    const int yesNo = std::atoi(request->get_query_parameter("yesNo").c_str());
    CARLA_SAFE_ASSERT_RETURN(yesNo == 0 || yesNo == 1,)

    carla_set_option(pluginId, option, yesNo);
    session->close(OK);
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_set_parameter_value(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int parameterId = std::atoi(request->get_query_parameter("parameterId").c_str());
    CARLA_SAFE_ASSERT_RETURN(parameterId >= 0,)

    const double value = std::atof(request->get_query_parameter("value").c_str());

    carla_set_parameter_value(pluginId, parameterId, value);
    session->close(OK);
}

void handle_carla_set_parameter_midi_channel(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int parameterId = std::atoi(request->get_query_parameter("parameterId").c_str());
    CARLA_SAFE_ASSERT_RETURN(parameterId >= 0,)

    const int channel = std::atoi(request->get_query_parameter("channel").c_str());
    CARLA_SAFE_ASSERT_RETURN(channel >= 0 && channel < 16,)

    carla_set_parameter_midi_channel(pluginId, parameterId, channel);
    session->close(OK);
}

void handle_carla_set_parameter_midi_cc(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int parameterId = std::atoi(request->get_query_parameter("parameterId").c_str());
    CARLA_SAFE_ASSERT_RETURN(parameterId >= 0,)

    const int cc = std::atoi(request->get_query_parameter("channel").c_str());
    CARLA_SAFE_ASSERT_RETURN(cc >= -1 && cc < INT16_MAX,);

    carla_set_parameter_midi_cc(pluginId, parameterId, cc);
    session->close(OK);
}

void handle_carla_set_program(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int programId = std::atoi(request->get_query_parameter("programId").c_str());
    CARLA_SAFE_ASSERT_RETURN(programId >= 0,)

    carla_set_program(pluginId, programId);
    session->close(OK);
}

void handle_carla_set_midi_program(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int midiProgramId = std::atoi(request->get_query_parameter("midiProgramId").c_str());
    CARLA_SAFE_ASSERT_RETURN(midiProgramId >= 0,)

    carla_set_midi_program(pluginId, midiProgramId);
    session->close(OK);
}

void handle_carla_set_custom_data(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const std::string type = request->get_query_parameter("type");
    const std::string key = request->get_query_parameter("key");
    const std::string value = request->get_query_parameter("value");

    carla_set_custom_data(pluginId, type.c_str(), key.c_str(), value.c_str());
    session->close(OK);
}

void handle_carla_set_chunk_data(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const std::string chunkData = request->get_query_parameter("chunkData");

    carla_set_chunk_data(pluginId, chunkData.c_str());
    session->close(OK);
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_prepare_for_save(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    carla_prepare_for_save(pluginId);
    session->close(OK);
}

void handle_carla_reset_parameters(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    carla_reset_parameters(pluginId);
    session->close(OK);
}

void handle_carla_randomize_parameters(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    carla_randomize_parameters(pluginId);
    session->close(OK);
}

void handle_carla_send_midi_note(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int pluginId = std::atoi(request->get_query_parameter("pluginId").c_str());
    CARLA_SAFE_ASSERT_RETURN(pluginId >= 0,)

    const int channel = std::atoi(request->get_query_parameter("channel").c_str());
    CARLA_SAFE_ASSERT_RETURN(channel >= 0 && channel < 16,)

    const int note = std::atoi(request->get_query_parameter("note").c_str());
    CARLA_SAFE_ASSERT_RETURN(note >= 0 && note < 128,)

    const int velocity = std::atoi(request->get_query_parameter("velocity").c_str());
    CARLA_SAFE_ASSERT_RETURN(velocity >= 0 && velocity < 128,)

    carla_send_midi_note(pluginId, channel, note, velocity);
    session->close(OK);
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_get_buffer_size(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_uint(carla_get_buffer_size());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_sample_rate(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_float(carla_get_sample_rate());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_last_error(const std::shared_ptr<Session> session)
{
    const char* const buf = carla_get_last_error();
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_host_osc_url_tcp(const std::shared_ptr<Session> session)
{
    const char* const buf = carla_get_host_osc_url_tcp();
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_host_osc_url_udp(const std::shared_ptr<Session> session)
{
    const char* const buf = carla_get_host_osc_url_udp();
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------
