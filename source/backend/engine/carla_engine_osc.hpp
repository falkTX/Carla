/*
 * Carla Engine OSC
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef CARLA_ENGINE_OSC_HPP
#define CARLA_ENGINE_OSC_HPP

#include "carla_backend.hpp"
#include "carla_osc_utils.hpp"

#define CARLA_ENGINE_OSC_HANDLE_ARGS1 CarlaPlugin* const plugin
#define CARLA_ENGINE_OSC_HANDLE_ARGS2 CarlaPlugin* const plugin, const int argc, const lo_arg* const* const argv, const char* const types

#define CARLA_ENGINE_OSC_CHECK_OSC_TYPES(/* argc, types, */ argcToCompare, typesToCompare)                                  \
    /* check argument count */                                                                                              \
    if (argc != argcToCompare)                                                                                              \
    {                                                                                                                       \
        qCritical("CarlaEngineOsc::%s() - argument count mismatch: %i != %i", __FUNCTION__, argc, argcToCompare);           \
        return 1;                                                                                                           \
    }                                                                                                                       \
    if (argc > 0)                                                                                                           \
    {                                                                                                                       \
        /* check for nullness */                                                                                            \
        if (! (types && typesToCompare))                                                                                    \
        {                                                                                                                   \
            qCritical("CarlaEngineOsc::%s() - argument types are null", __FUNCTION__);                                      \
            return 1;                                                                                                       \
        }                                                                                                                   \
        /* check argument types */                                                                                          \
        if (strcmp(types, typesToCompare) != 0)                                                                             \
        {                                                                                                                   \
            qCritical("CarlaEngineOsc::%s() - argument types mismatch: '%s' != '%s'", __FUNCTION__, types, typesToCompare); \
            return 1;                                                                                                       \
        }                                                                                                                   \
    }

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

class CarlaEngineOsc
{
public:
    CarlaEngineOsc(CarlaEngine* const engine);
    ~CarlaEngineOsc();

    void init(const char* const name);
    void idle();
    void close();

    // -------------------------------------------------------------------

#ifndef BUILD_BRIDGE
    bool isControlRegistered() const
    {
        return bool(m_controlData.target);
    }

    const CarlaOscData* getControlData() const
    {
        return &m_controlData;
    }
#endif

    const char* getServerPathTCP() const
    {
        return (const char*)m_serverPathTCP;
    }

    const char* getServerPathUDP() const
    {
        return (const char*)m_serverPathUDP;
    }

    // -------------------------------------------------------------------

private:
    CarlaEngine* const engine;

    char*  m_name;
    size_t m_nameSize;

    lo_server m_serverTCP;
    lo_server m_serverUDP;
    CarlaString m_serverPathTCP;
    CarlaString m_serverPathUDP;

#ifndef BUILD_BRIDGE
    CarlaOscData m_controlData; // for carla-control
#endif

    // -------------------------------------------------------------------

    int handleMessage(const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg);

#ifndef BUILD_BRIDGE
    int handleMsgRegister(const int argc, const lo_arg* const* const argv, const char* const types, const lo_address source);
    int handleMsgUnregister();
#endif

    int handleMsgUpdate(CARLA_ENGINE_OSC_HANDLE_ARGS2, const lo_address source);
    int handleMsgConfigure(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgControl(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgMidi(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgExiting(CARLA_ENGINE_OSC_HANDLE_ARGS1);

#ifndef BUILD_BRIDGE
    int handleMsgSetActive(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetDryWet(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetVolume(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetBalanceLeft(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetBalanceRight(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetParameterValue(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetParameterMidiCC(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetParameterMidiChannel(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgSetMidiProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgNoteOn(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgNoteOff(CARLA_ENGINE_OSC_HANDLE_ARGS2);

    int handleMsgBridgeSetInPeak(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgBridgeSetOutPeak(CARLA_ENGINE_OSC_HANDLE_ARGS2);
#endif

#ifdef WANT_LV2
    int handleMsgLv2AtomTransfer(CARLA_ENGINE_OSC_HANDLE_ARGS2);
    int handleMsgLv2EventTransfer(CARLA_ENGINE_OSC_HANDLE_ARGS2);
#endif

    // -----------------------------------------------------------------------

    static void osc_error_handlerTCP(const int num, const char* const msg, const char* const path)
    {
        qWarning("CarlaEngineOsc::osc_error_handlerTCP(%i, \"%s\", \"%s\")", num, msg, path);
    }

    static void osc_error_handlerUDP(const int num, const char* const msg, const char* const path)
    {
        qWarning("CarlaEngineOsc::osc_error_handlerUDP(%i, \"%s\", \"%s\")", num, msg, path);
    }

    static int osc_message_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const userData)
    {
        CARLA_ASSERT(userData);
        if (CarlaEngineOsc* const _this_ = (CarlaEngineOsc*)userData)
            return _this_->handleMessage(path, argc, argv, types, msg);
        return 1;
    }
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_OSC_HPP
