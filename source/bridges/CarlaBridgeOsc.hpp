/*
 * Carla Bridge OSC
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_BRIDGE_OSC_HPP_INCLUDED
#define CARLA_BRIDGE_OSC_HPP_INCLUDED

#include "CarlaBridge.hpp"
#include "CarlaOscUtils.hpp"
#include "CarlaString.hpp"

#define CARLA_BRIDGE_OSC_HANDLE_ARGS const int argc, const lo_arg* const* const argv, const char* const types

#define CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(/* argc, types, */ argcToCompare, typesToCompare)                                     \
    /* check argument count */                                                                                                 \
    if (argc != argcToCompare)                                                                                                 \
    {                                                                                                                          \
        carla_stderr("CarlaBridgeOsc::%s() - argument count mismatch: %i != %i", __FUNCTION__, argc, argcToCompare);           \
        return 1;                                                                                                              \
    }                                                                                                                          \
    if (argc > 0)                                                                                                              \
    {                                                                                                                          \
        /* check for nullness */                                                                                               \
        if (types == nullptr || typesToCompare == nullptr)                                                                     \
        {                                                                                                                      \
            carla_stderr("CarlaBridgeOsc::%s() - argument types are null", __FUNCTION__);                                      \
            return 1;                                                                                                          \
        }                                                                                                                      \
        /* check argument types */                                                                                             \
        if (std::strcmp(types, typesToCompare) != 0)                                                                           \
        {                                                                                                                      \
            carla_stderr("CarlaBridgeOsc::%s() - argument types mismatch: '%s' != '%s'", __FUNCTION__, types, typesToCompare); \
            return 1;                                                                                                          \
        }                                                                                                                      \
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

    void init(const char* const url);
    void idle() const;
    void close();

    // -------------------------------------------------------------------

    bool isControlRegistered() const noexcept
    {
        return (fControlData.target != nullptr);
    }

    const CarlaOscData& getControlData() const noexcept
    {
        return fControlData;
    }

    const char* getServerPath() const noexcept
    {
        return fServerPath;
    }

    // -------------------------------------------------------------------

private:
    CarlaBridgeClient* const fClient;

    CarlaOscData fControlData;
    CarlaString  fName;
    CarlaString  fServerPath;
    lo_server    fServer;

    // -------------------------------------------------------------------

    int handleMessage(const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg);

    int handleMsgShow();
    int handleMsgHide();
    int handleMsgQuit();

#ifdef BUILD_BRIDGE_UI
    int handleMsgUiConfigure(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgUiControl(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgUiProgram(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgUiMidiProgram(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgUiMidi(CARLA_BRIDGE_OSC_HANDLE_ARGS);

# ifdef BRIDGE_LV2
    int handleMsgLv2UiAtomTransfer(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgLv2UiUridMap(CARLA_BRIDGE_OSC_HANDLE_ARGS);
# endif
#endif

    // -------------------------------------------------------------------

    static void osc_error_handler(int num, const char* msg, const char* path)
    {
        carla_stderr("CarlaBridgeOsc::osc_error_handler(%i, \"%s\", \"%s\")", num, msg, path);
    }

    static int osc_message_handler(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* userData)
    {
        return ((CarlaBridgeOsc*)userData)->handleMessage(path, argc, argv, types, msg);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgeOsc)
};

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_OSC_HPP_INCLUDED
