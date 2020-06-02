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

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

CarlaEngineOsc::CarlaEngineOsc(CarlaEngine* const engine) noexcept
    : fEngine(engine),
      fControlDataTCP(),
      fControlDataUDP(),
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

void CarlaEngineOsc::init(const char* const name, int tcpPort, int udpPort) noexcept
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

    if (fEngine->getType() != kEngineTypePlugin)
    {
        const char* const tcpPortEnv = std::getenv("CARLA_OSC_TCP_PORT");
        const char* const udpPortEnv = std::getenv("CARLA_OSC_UDP_PORT");

        if (tcpPortEnv != nullptr)
            tcpPort = std::atoi(tcpPortEnv);
        if (udpPortEnv != nullptr)
            udpPort = std::atoi(udpPortEnv);
    }

    // port == 0 means to pick a random one
    // port < 0 will get osc disabled

    static const int kRetryAttempts = 5;

    // ----------------------------------------------------------------------------------------------------------------

    if (tcpPort == 0)
    {
        for (int i=0; i < kRetryAttempts && fServerTCP == nullptr; ++i)
            fServerTCP = lo_server_new_with_proto(nullptr, LO_TCP, osc_error_handler_TCP);
    }
    else if (tcpPort >= 1024)
    {
        char strBuf[0xff];

        for (int i=0; i < kRetryAttempts && tcpPort < 32767 && fServerTCP == nullptr; ++i, ++tcpPort)
        {
            std::snprintf(strBuf, 0xff-1, "%d", tcpPort);
            strBuf[0xff-1] = '\0';

            fServerTCP = lo_server_new_with_proto(strBuf, LO_TCP, osc_error_handler_TCP);
        }
    }

    if (fServerTCP != nullptr)
    {
        if (char* const tmpServerPathTCP = lo_server_get_url(fServerTCP))
        {
            fServerPathTCP  = tmpServerPathTCP;
            fServerPathTCP += fName;
            std::free(tmpServerPathTCP);
        }

        lo_server_add_method(fServerTCP, nullptr, nullptr, osc_message_handler_TCP, this);
        carla_debug("OSC TCP server running and listening at %s", fServerPathTCP.buffer());
    }

    // ----------------------------------------------------------------------------------------------------------------

    if (udpPort == 0)
    {
        for (int i=0; i < kRetryAttempts && fServerUDP == nullptr; ++i)
            fServerUDP = lo_server_new_with_proto(nullptr, LO_UDP, osc_error_handler_UDP);
    }
    else if (udpPort >= 1024)
    {
        char strBuf[0xff];

        for (int i=0; i < kRetryAttempts && udpPort < 32768 && fServerUDP == nullptr; ++i, ++udpPort)
        {
            std::snprintf(strBuf, 0xff-1, "%d", udpPort);
            strBuf[0xff-1] = '\0';

            fServerUDP = lo_server_new_with_proto(strBuf, LO_UDP, osc_error_handler_UDP);
        }
    }

    if (fServerUDP != nullptr)
    {
        if (char* const tmpServerPathUDP = lo_server_get_url(fServerUDP))
        {
            fServerPathUDP  = tmpServerPathUDP;
            fServerPathUDP += fName;
            std::free(tmpServerPathUDP);
        }

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
