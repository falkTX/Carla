/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_ENGINE_OSC_HPP_INCLUDED
#define CARLA_ENGINE_OSC_HPP_INCLUDED

#include "CarlaBackend.h"
#include "CarlaOscUtils.hpp"
#include "CarlaString.hpp"

#define CARLA_ENGINE_OSC_HANDLE_ARGS1 CarlaPlugin* const plugin
#define CARLA_ENGINE_OSC_HANDLE_ARGS2 CarlaPlugin* const plugin, const int argc, const lo_arg* const* const argv, const char* const types

#define CARLA_ENGINE_OSC_CHECK_OSC_TYPES(/* argc, types, */ argcToCompare, typesToCompare)                                     \
    /* check argument count */                                                                                                 \
    if (argc != argcToCompare)                                                                                                 \
    {                                                                                                                          \
        carla_stderr("CarlaEngineOsc::%s() - argument count mismatch: %i != %i", __FUNCTION__, argc, argcToCompare);           \
        return 1;                                                                                                              \
    }                                                                                                                          \
    if (argc > 0)                                                                                                              \
    {                                                                                                                          \
        /* check for nullness */                                                                                               \
        if (types == nullptr || typesToCompare == nullptr)                                                                     \
        {                                                                                                                      \
            carla_stderr("CarlaEngineOsc::%s() - argument types are null", __FUNCTION__);                                      \
            return 1;                                                                                                          \
        }                                                                                                                      \
        /* check argument types */                                                                                             \
        if (std::strcmp(types, typesToCompare) != 0)                                                                           \
        {                                                                                                                      \
            carla_stderr("CarlaEngineOsc::%s() - argument types mismatch: '%s' != '%s'", __FUNCTION__, types, typesToCompare); \
            return 1;                                                                                                          \
        }                                                                                                                      \
    }

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

class CarlaEngineOsc
{
public:
    CarlaEngineOsc(CarlaEngine* const engine) noexcept;
    ~CarlaEngineOsc() noexcept;

    void init(const char* const name) noexcept;
    void idle() const noexcept;
    void close() noexcept;

    // -------------------------------------------------------------------

    const char* getServerPathTCP() const noexcept
    {
        return fServerPathTCP;
    }

    const char* getServerPathUDP() const noexcept
    {
        return fServerPathUDP;
    }

#ifndef BUILD_BRIDGE
    // -------------------------------------------------------------------

    bool isControlRegistered() const noexcept
    {
        return (fControlData.target != nullptr);
    }

    const CarlaOscData* getControlData() const noexcept
    {
        return &fControlData;
    }
#endif

    // -------------------------------------------------------------------

private:
    CarlaEngine* const fEngine;

#ifndef BUILD_BRIDGE
    CarlaOscData fControlData; // for carla-control
#endif

    CarlaString fName;
    CarlaString fServerPathTCP;
    CarlaString fServerPathUDP;
    lo_server   fServerTCP;
    lo_server   fServerUDP;

    // -------------------------------------------------------------------

    int handleMessage(const bool isTCP, const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg);

    int handleMsgUpdate(CARLA_ENGINE_OSC_HANDLE_ARGS2, const lo_address source);
    int handleMsgExiting(CARLA_ENGINE_OSC_HANDLE_ARGS1);

#ifndef BUILD_BRIDGE
    int handleMsgRegister(const bool isTCP, const int argc, const lo_arg* const* const argv, const char* const types, const lo_address source);
    int handleMsgUnregister();

    int handleMsgConfigure(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgControl(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgMidi(CARLA_ENGINE_OSC_HANDLE_ARGS2);

    int handleMsgSetActive(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetDryWet(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetVolume(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetBalanceLeft(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetBalanceRight(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetPanning(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetParameterValue(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetParameterMidiCC(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetParameterMidiChannel(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetMidiProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgNoteOn(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgNoteOff(CARLA_ENGINE_OSC_HANDLE_ARGS2);

    int handleMsgLv2AtomTransfer(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgLv2UridMap(CARLA_ENGINE_OSC_HANDLE_ARGS2);
#endif

    // -----------------------------------------------------------------------

    static void osc_error_handler_TCP(int num, const char* msg, const char* path)
    {
        carla_stderr("CarlaEngineOsc::osc_error_handler_TCP(%i, \"%s\", \"%s\")", num, msg, path);
    }

    static void osc_error_handler_UDP(int num, const char* msg, const char* path)
    {
        carla_stderr("CarlaEngineOsc::osc_error_handler_UDP(%i, \"%s\", \"%s\")", num, msg, path);
    }

    static int osc_message_handler_TCP(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* userData)
    {
        return ((CarlaEngineOsc*)userData)->handleMessage(true, path, argc, argv, types, msg);
    }

    static int osc_message_handler_UDP(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* userData)
    {
        return ((CarlaEngineOsc*)userData)->handleMessage(false, path, argc, argv, types, msg);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineOsc)
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_OSC_HPP_INCLUDED
