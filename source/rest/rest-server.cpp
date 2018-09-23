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

#include <map>
#include <restbed>
#include <system_error>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

using namespace std;
using namespace restbed;
using namespace std::chrono;

// std::vector<std::shared_ptr<Session>> gSessions;

CarlaStringList gSessionMessages;
CarlaMutex gSessionMessagesMutex;

std::map< string, shared_ptr< WebSocket > > sockets = { };

// -------------------------------------------------------------------------------------------------------------------

void send_server_side_message(const char* const message)
{
    const CarlaMutexLocker cml(gSessionMessagesMutex);

    gSessionMessages.append(message);
}

// -------------------------------------------------------------------------------------------------------------------

static void event_stream_handler(void)
{
    static bool firstInit = true;

    if (firstInit)
    {
        firstInit = false;
        carla_stdout("Carla REST-API Server started");
    }

    const bool running = carla_is_engine_running();

    if (running)
        carla_engine_idle();

    CarlaStringList messages;

    {
        const CarlaMutexLocker cml(gSessionMessagesMutex);

        if (gSessionMessages.count() > 0)
            gSessionMessages.moveTo(messages);
    }

    for (auto message : messages)
    {
        for (auto entry : sockets)
        {
            auto socket = entry.second;

            if (socket->is_open())
                socket->send(message);
        }
    }

    if (running)
    {
        if (const uint count = carla_get_current_plugin_count())
        {
            char msgBuf[1024];
            float* peaks;

            for (uint i=0; i<count; ++i)
            {
                peaks = carla_get_peak_values(i);
                CARLA_SAFE_ASSERT_BREAK(peaks != nullptr);

                std::snprintf(msgBuf, 1023, "Peaks: %u %f %f %f %f", i, peaks[0], peaks[1], peaks[2], peaks[3]);
                msgBuf[1023] = '\0';

                for (auto entry : sockets)
                {
                    auto socket = entry.second;

                    if (socket->is_open())
                        socket->send(msgBuf);
                }
            }
        }
    }

    for (auto entry : sockets)
    {
        auto socket = entry.second;

        if (socket->is_open())
            socket->send("Keep-Alive");
    }
}


// -------------------------------------------------------------------------------------------------------------------

string base64_encode( const unsigned char* input, int length )
{
    BIO* bmem, *b64;
    BUF_MEM* bptr;

    b64 = BIO_new( BIO_f_base64( ) );
    bmem = BIO_new( BIO_s_mem( ) );
    b64 = BIO_push( b64, bmem );
    BIO_write( b64, input, length );
    ( void ) BIO_flush( b64 );
    BIO_get_mem_ptr( b64, &bptr );

    char* buff = ( char* )malloc( bptr->length );
    memcpy( buff, bptr->data, bptr->length - 1 );
    buff[ bptr->length - 1 ] = 0;

    BIO_free_all( b64 );

    return buff;
}

multimap< string, string > build_websocket_handshake_response_headers( const shared_ptr< const Request >& request )
{
    auto key = request->get_header( "Sec-WebSocket-Key" );
    key.append( "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" );

    Byte hash[ SHA_DIGEST_LENGTH ];
    SHA1( reinterpret_cast< const unsigned char* >( key.data( ) ), key.length( ), hash );

    multimap< string, string > headers;
    headers.insert( make_pair( "Upgrade", "websocket" ) );
    headers.insert( make_pair( "Connection", "Upgrade" ) );
    headers.insert( make_pair( "Sec-WebSocket-Accept", base64_encode( hash, SHA_DIGEST_LENGTH ) ) );

    return headers;
}

void close_handler( const shared_ptr< WebSocket > socket )
{
    carla_stdout("CLOSE %i", __LINE__);

    if ( socket->is_open( ) )
    {
        auto response = make_shared< WebSocketMessage >( WebSocketMessage::CONNECTION_CLOSE_FRAME, Bytes( { 10, 00 } ) );
        socket->send( response );
    }
    carla_stdout("CLOSE %i", __LINE__);

    const auto key = socket->get_key( );
    sockets.erase( key );

    fprintf( stderr, "Closed connection to %s.\n", key.data( ) );
}

void error_handler( const shared_ptr< WebSocket > socket, const error_code error )
{
    const auto key = socket->get_key( );
    fprintf( stderr, "WebSocket Errored '%s' for %s.\n", error.message( ).data( ), key.data( ) );
}

void message_handler( const shared_ptr< WebSocket > source, const shared_ptr< WebSocketMessage > message )
{
    const auto opcode = message->get_opcode( );

    if ( opcode == WebSocketMessage::PING_FRAME )
    {
        auto response = make_shared< WebSocketMessage >( WebSocketMessage::PONG_FRAME, message->get_data( ) );
        source->send( response );
    }
    else if ( opcode == WebSocketMessage::PONG_FRAME )
    {
        //Ignore PONG_FRAME.
        //
        //Every time the ping_handler is scheduled to run, it fires off a PING_FRAME to each
        //WebSocket. The client, if behaving correctly, will respond with a PONG_FRAME.
        //
        //On each occasion the underlying TCP socket sees any packet data transfer, whether
        //a PING, PONG, TEXT, or BINARY... frame. It will automatically reset the timeout counter
        //leaving the connection active; see also Settings::set_connection_timeout.
        return;
    }
    else if ( opcode == WebSocketMessage::CONNECTION_CLOSE_FRAME )
    {
        source->close( );
    }
    else if ( opcode == WebSocketMessage::BINARY_FRAME )
    {
        //We don't support binary data.
        auto response = make_shared< WebSocketMessage >( WebSocketMessage::CONNECTION_CLOSE_FRAME, Bytes( { 10, 03 } ) );
        source->send( response );
    }
    else if ( opcode == WebSocketMessage::TEXT_FRAME )
    {
        auto response = make_shared< WebSocketMessage >( *message );
        response->set_mask( 0 );

        for ( auto socket : sockets )
        {
            auto destination = socket.second;
            destination->send( response );
        }

        const auto key = source->get_key( );
        const auto data = String::format( "Received message '%.*s' from %s\n", message->get_data( ).size( ), message->get_data( ).data( ), key.data( ) );
        fprintf( stderr, "%s", data.data( ) );
    }
}

void get_method_handler(const shared_ptr<Session> session)
{
    carla_stdout("HERE %i", __LINE__);
    const auto request = session->get_request();
    const auto connection_header = request->get_header("connection", String::lowercase);
    carla_stdout("HERE %i", __LINE__);

    if ( connection_header.find( "upgrade" ) not_eq string::npos )
    {
        if ( request->get_header( "upgrade", String::lowercase ) == "websocket" )
        {
            const auto headers = build_websocket_handshake_response_headers( request );

            session->upgrade( SWITCHING_PROTOCOLS, headers, [ ]( const shared_ptr< WebSocket > socket )
            {
                if ( socket->is_open( ) )
                {
                    socket->set_close_handler( close_handler );
                    socket->set_error_handler( error_handler );
                    socket->set_message_handler( message_handler );

                    socket->send("Welcome to Corvusoft Chat!");

                    auto key = socket->get_key( );
                    sockets[key] = socket;
                }
                else
                {
                    fprintf( stderr, "WebSocket Negotiation Failed: Client closed connection.\n" );
                }
            } );

            return;
        }
    }

    session->close( BAD_REQUEST );
}

void ping_handler( void )
{
    for ( auto entry : sockets )
    {
        auto key = entry.first;
        auto socket = entry.second;

        if ( socket->is_open( ) )
        {
            socket->send( WebSocketMessage::PING_FRAME );
        }
        else
        {
            socket->close( );
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

    // websocket
    {
        std::shared_ptr<Resource> resource = std::make_shared<Resource>();
        resource->set_path("/ws");
        resource->set_method_handler("GET", get_method_handler);
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
    service.schedule(event_stream_handler, std::chrono::milliseconds(33));
    service.schedule(ping_handler, milliseconds(5000));

    std::shared_ptr<Settings> settings = std::make_shared<Settings>();
    settings->set_port(2228);
    settings->set_default_header("Connection", "close");

    service.start(settings);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------
