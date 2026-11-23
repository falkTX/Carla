/*
 * Carla Plugin Host
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngineOsc.hpp"

#ifdef HAVE_LIBLO

#include "CarlaEngine.hpp"

#ifdef CARLA_OS_LINUX
# include <sys/socket.h>
#endif

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

CarlaEngineOsc::CarlaEngineOsc(CarlaEngine* const engine) noexcept
    : fEngine(engine),
      fControlDataTCP(),
      fControlDataUDP(),
      fControlEnabled(false),
      fName(),
      fServerPathTCP(),
      fServerPathUDP(),
      fServerTCP(nullptr),
      fServerUDP(nullptr)
{
    CARLA_SAFE_ASSERT(engine != nullptr);
    carla_debug("CarlaEngineOsc::CarlaEngineOsc(%p)", engine);
}

CarlaEngineOsc::~CarlaEngineOsc() noexcept
{
    CARLA_SAFE_ASSERT(fName.isEmpty());
    CARLA_SAFE_ASSERT(fServerPathTCP.isEmpty());
    CARLA_SAFE_ASSERT(fServerPathUDP.isEmpty());
    CARLA_SAFE_ASSERT(fServerTCP == nullptr);
    CARLA_SAFE_ASSERT(fServerUDP == nullptr);
    carla_debug("CarlaEngineOsc::~CarlaEngineOsc()");
}

// -----------------------------------------------------------------------

static lo_server createLocalOscServer(int port, const int protocol, lo_err_handler errorHandler) noexcept
{
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        if (port != 0 && (port < 1024 || port > 32767))
            break;

        char portString[6];
        std::snprintf(portString, sizeof(portString), "%d", port);
        if (lo_server server = lo_server_new_with_proto(port == 0 ? nullptr : portString, protocol, errorHandler))
        {
#ifdef SO_BINDTODEVICE
            // Older kernels may require privileges. Dispatch also checks the peer address.
            setsockopt(lo_server_get_socket_fd(server), SOL_SOCKET, SO_BINDTODEVICE, "lo", sizeof("lo"));
#endif
            return server;
        }
        if (port != 0)
            ++port;
    }
    return nullptr;
}

void CarlaEngineOsc::init(const char* const name, const bool enableControl, int tcpPort, int udpPort) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fName.isEmpty(),);
    CARLA_SAFE_ASSERT_RETURN(fServerPathTCP.isEmpty(),);
    CARLA_SAFE_ASSERT_RETURN(fServerPathUDP.isEmpty(),);
    CARLA_SAFE_ASSERT_RETURN(fServerTCP == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(fServerUDP == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);
    carla_debug("CarlaEngineOsc::init(\"%s\")", name);

    fName = name;
    fName.toBasic();
    fControlEnabled = enableControl;

    if (enableControl && fEngine->getType() != kEngineTypePlugin)
    {
        const char* const tcpPortEnv = std::getenv("CARLA_OSC_TCP_PORT");
        const char* const udpPortEnv = std::getenv("CARLA_OSC_UDP_PORT");

        if (tcpPortEnv != nullptr)
            tcpPort = std::atoi(tcpPortEnv);
        if (udpPortEnv != nullptr)
            udpPort = std::atoi(udpPortEnv);
    }

    if (enableControl && (fServerTCP = createLocalOscServer(tcpPort, LO_TCP, osc_error_handler_TCP)) != nullptr)
    {
        char url[32];
        std::snprintf(url, sizeof(url), "osc.tcp://127.0.0.1:%u/", static_cast<uint16_t>(lo_server_get_port(fServerTCP)));
        fServerPathTCP = url;
        fServerPathTCP += fName;

        lo_server_add_method(fServerTCP, nullptr, nullptr, osc_message_handler_TCP, this);
        carla_debug("OSC TCP server running and listening at %s", fServerPathTCP.buffer());
    }

    // DSSI UIs still need a local UDP endpoint when OSC control is disabled.
    if ((fServerUDP = createLocalOscServer(!enableControl && udpPort >= 0 ? 0 : udpPort,
                                          LO_UDP, osc_error_handler_UDP)) != nullptr)
    {
        char url[32];
        std::snprintf(url, sizeof(url), "osc.udp://127.0.0.1:%u/", static_cast<uint16_t>(lo_server_get_port(fServerUDP)));
        fServerPathUDP = url;
        fServerPathUDP += fName;

        lo_server_add_method(fServerUDP, nullptr, nullptr, osc_message_handler_UDP, this);
        carla_debug("OSC UDP server running and listening at %s", fServerPathUDP.buffer());
    }

    // ----------------------------------------------------------------------------------------------------------------

    CARLA_SAFE_ASSERT(fName.isNotEmpty());
}

void CarlaEngineOsc::idle() const noexcept
{
    if (fServerTCP != nullptr)
    {
        for (;;)
        {
            try {
                if (lo_server_recv_noblock(fServerTCP, 0) == 0)
                    break;
            } CARLA_SAFE_EXCEPTION_CONTINUE("OSC idle TCP")
        }
    }

    if (fServerUDP != nullptr)
    {
        for (;;)
        {
            try {
                if (lo_server_recv_noblock(fServerUDP, 0) == 0)
                    break;
            } CARLA_SAFE_EXCEPTION_CONTINUE("OSC idle UDP")
        }
    }
}

void CarlaEngineOsc::close() noexcept
{
    carla_debug("CarlaEngineOsc::close()");

    if (fControlDataTCP.target != nullptr)
        sendExit();

    fName.clear();

    if (fServerTCP != nullptr)
    {
        lo_server_del_method(fServerTCP, nullptr, nullptr);
        lo_server_free(fServerTCP);
        fServerTCP = nullptr;
    }

    if (fServerUDP != nullptr)
    {
        lo_server_del_method(fServerUDP, nullptr, nullptr);
        lo_server_free(fServerUDP);
        fServerUDP = nullptr;
    }

    fServerPathTCP.clear();
    fServerPathUDP.clear();

    fControlDataTCP.clear();
    fControlDataUDP.clear();
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // HAVE_LIBLO
