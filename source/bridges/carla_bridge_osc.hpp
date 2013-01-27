/*
 * Carla Bridge OSC
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_BRIDGE_OSC_HPP
#define CARLA_BRIDGE_OSC_HPP

#include "carla_bridge.hpp"
#include "carla_osc_utils.hpp"

#define CARLA_BRIDGE_OSC_HANDLE_ARGS const int argc, const lo_arg* const* const argv, const char* const types

#define CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(/* argc, types, */ argcToCompare, typesToCompare)                                  \
    /* check argument count */                                                                                              \
    if (argc != argcToCompare)                                                                                              \
    {                                                                                                                       \
        qCritical("CarlaBridgeOsc::%s() - argument count mismatch: %i != %i", __FUNCTION__, argc, argcToCompare);           \
        return 1;                                                                                                           \
    }                                                                                                                       \
    if (argc > 0)                                                                                                           \
    {                                                                                                                       \
        /* check for nullness */                                                                                            \
        if (! (types && typesToCompare))                                                                                    \
        {                                                                                                                   \
            qCritical("CarlaBridgeOsc::%s() - argument types are null", __FUNCTION__);                                      \
            return 1;                                                                                                       \
        }                                                                                                                   \
        /* check argument types */                                                                                          \
        if (strcmp(types, typesToCompare) != 0)                                                                             \
        {                                                                                                                   \
            qCritical("CarlaBridgeOsc::%s() - argument types mismatch: '%s' != '%s'", __FUNCTION__, types, typesToCompare); \
            return 1;                                                                                                       \
        }                                                                                                                   \
    }

CARLA_BRIDGE_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

class CarlaBridgeOsc
{
public:
    CarlaBridgeOsc(CarlaBridgeClient* const client);
    ~CarlaBridgeOsc();

    bool init(const char* const url);
    void idle();
    void close();

    // -------------------------------------------------------------------

    bool isControlRegistered() const
    {
        return bool(m_controlData.target);
    }

    const CarlaOscData* getControlData() const
    {
        return &m_controlData;
    }

    const char* getServerPath() const
    {
        return m_serverPath;
    }

    // -------------------------------------------------------------------

private:
    CarlaBridgeClient* const client;

    char*  m_name;
    size_t m_nameSize;

    lo_server m_server;
    char*     m_serverPath;

    CarlaOscData m_controlData;

    // -------------------------------------------------------------------

    int handleMessage(const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg);

    int handleMsgConfigure(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgControl(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgProgram(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgMidiProgram(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgMidi(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgShow();
    int handleMsgHide();
    int handleMsgQuit();

#ifdef BRIDGE_LV2
    int handleMsgLv2TransferAtom(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgLv2TransferEvent(CARLA_BRIDGE_OSC_HANDLE_ARGS);
#endif

#ifdef BUILD_BRIDGE_PLUGIN
    int handleMsgPluginSaveNow();
    int handleMsgPluginSetChunk(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgPluginSetCustomData(CARLA_BRIDGE_OSC_HANDLE_ARGS);
#endif

    // -------------------------------------------------------------------

    static void osc_error_handler(const int num, const char* const msg, const char* const path)
    {
        qWarning("CarlaBridgeOsc::osc_error_handler(%i, \"%s\", \"%s\")", num, msg, path);
    }

    static int osc_message_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const user_data)
    {
        CARLA_ASSERT(user_data);
        if (CarlaBridgeOsc* const _this_ = (CarlaBridgeOsc*)user_data)
            return _this_->handleMessage(path, argc, argv, types, msg);
        return 1;
    }
};

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_OSC_HPP
