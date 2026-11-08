// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CARLA_ENGINE_OSC_HPP_INCLUDED
#define CARLA_ENGINE_OSC_HPP_INCLUDED

#include "CarlaDefines.h"

#ifdef HAVE_LIBLO

#include "CarlaBackend.h"
#include "CarlaPlugin.hpp"
#include "CarlaJuceUtils.hpp"
#include "CarlaOscUtils.hpp"

#include "extra/String.hpp"

#define CARLA_ENGINE_OSC_HANDLE_ARGS const CarlaPluginPtr& plugin, \
  const int argc, const lo_arg* const* const argv, const char* const types

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
    CarlaEngineOsc(CarlaEngine* engine) noexcept;
    ~CarlaEngineOsc() noexcept;

    void init(const char* name, int tcpPort, int udpPort) noexcept;
    void idle() const noexcept;
    void close() noexcept;

    // -------------------------------------------------------------------

    const String& getServerPathTCP() const noexcept
    {
        return fServerPathTCP;
    }

    const String& getServerPathUDP() const noexcept
    {
        return fServerPathUDP;
    }

    // -------------------------------------------------------------------

    bool isControlRegisteredForTCP() const noexcept
    {
        return fControlDataTCP.target != nullptr;
    }

    bool isControlRegisteredForUDP() const noexcept
    {
        return fControlDataUDP.target != nullptr;
    }

    // -------------------------------------------------------------------
    // TCP

    void sendCallback(EngineCallbackOpcode action, uint pluginId,
                      int value1, int value2, int value3,
                      float valuef, const char* valueStr) const noexcept;
    void sendPluginInfo(const CarlaPluginPtr& plugin) const noexcept;
    void sendPluginPortCount(const CarlaPluginPtr& plugin) const noexcept;
    void sendPluginParameterInfo(const CarlaPluginPtr& plugin, uint32_t index) const noexcept;
    void sendPluginDataCount(const CarlaPluginPtr& plugin) const noexcept;
    void sendPluginProgramCount(const CarlaPluginPtr& plugin) const noexcept;
    void sendPluginProgram(const CarlaPluginPtr& plugin, uint32_t index) const noexcept;
    void sendPluginMidiProgram(const CarlaPluginPtr& plugin, uint32_t index) const noexcept;
    void sendPluginCustomData(const CarlaPluginPtr& plugin, uint32_t index) const noexcept;
    void sendPluginInternalParameterValues(const CarlaPluginPtr& plugin) const noexcept;
    void sendPing() const noexcept;
    void sendResponse(int messageId, const char* error) const noexcept;
    void sendExit() const noexcept;

    // -------------------------------------------------------------------
    // UDP

    void sendRuntimeInfo() const noexcept;
    void sendParameterValue(uint pluginId, uint32_t index, float value) const noexcept;
    void sendPeaks(uint pluginId, const float peaks[4]) const noexcept;

    // -------------------------------------------------------------------

private:
    CarlaEngine* const fEngine;

    // for carla-control
    CarlaOscData fControlDataTCP;
    CarlaOscData fControlDataUDP;

    String    fName;
    String    fServerPathTCP;
    String    fServerPathUDP;
    lo_server fServerTCP;
    lo_server fServerUDP;

    // -------------------------------------------------------------------

    int handleMessage(bool isTCP, const char* path,
                      int argc, const lo_arg* const* argv, const char* types, lo_message msg);

    int handleMsgRegister(bool isTCP, int argc, const lo_arg* const* argv, const char* types, lo_address source);
    int handleMsgUnregister(bool isTCP, int argc, const lo_arg* const* argv, const char* types, lo_address source);
    int handleMsgControl(const char* method,
                         int argc, const lo_arg* const* argv, const char* types);

    // Internal methods
    int handleMsgSetActive(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgSetDryWet(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgSetVolume(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgSetBalanceLeft(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgSetBalanceRight(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgSetPanning(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgSetParameterValue(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgSetParameterMappedControlIndex(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgSetParameterMappedRange(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgSetParameterMidiChannel(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgSetProgram(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgSetMidiProgram(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgNoteOn(CARLA_ENGINE_OSC_HANDLE_ARGS);
    int handleMsgNoteOff(CARLA_ENGINE_OSC_HANDLE_ARGS);

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

#endif // HAVE_LIBLO

#endif // CARLA_ENGINE_OSC_HPP_INCLUDED
