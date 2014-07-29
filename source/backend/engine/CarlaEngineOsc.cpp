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

#include "CarlaEngine.hpp"
#include "CarlaEngineOsc.hpp"
#include "CarlaPlugin.hpp"

#ifndef BUILD_BRIDGE
# include "CarlaBridgeUtils.hpp"
#endif

#include "CarlaMIDI.h"

#include <cctype>

CARLA_BACKEND_START_NAMESPACE

#ifndef BUILD_BRIDGE
// -------------------------------------------------------------------
// Bridge Helper, defined in BridgePlugin.cpp

extern int CarlaPluginSetOscBridgeInfo(CarlaPlugin* const plugin, const PluginBridgeOscInfoType type,
                                       const int argc, const lo_arg* const* const argv, const char* const types);
#endif

// -----------------------------------------------------------------------

CarlaEngineOsc::CarlaEngineOsc(CarlaEngine* const engine) noexcept
    : fEngine(engine),
#ifndef BUILD_BRIDGE
      fControlData(),
#endif
      fName(),
      fServerPathTCP(),
      fServerPathUDP(),
      fServerTCP(nullptr),
      fServerUDP(nullptr),
      leakDetector()
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

void CarlaEngineOsc::init(const char* const name) noexcept
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

    fServerTCP = lo_server_new_with_proto(nullptr, LO_TCP, osc_error_handler_TCP);

    if (fServerTCP != nullptr)
    {
        if (char* const tmpServerPathTCP = lo_server_get_url(fServerTCP))
        {
            fServerPathTCP  = tmpServerPathTCP;
            fServerPathTCP += fName;
            std::free(tmpServerPathTCP);
        }

        lo_server_add_method(fServerTCP, nullptr, nullptr, osc_message_handler_TCP, this);
    }

    fServerUDP = lo_server_new_with_proto(nullptr, LO_UDP, osc_error_handler_UDP);

    if (fServerUDP != nullptr)
    {
        if (char* const tmpServerPathUDP = lo_server_get_url(fServerUDP))
        {
            fServerPathUDP  = tmpServerPathUDP;
            fServerPathUDP += fName;
            std::free(tmpServerPathUDP);
        }

        lo_server_add_method(fServerUDP, nullptr, nullptr, osc_message_handler_UDP, this);
    }

    CARLA_SAFE_ASSERT(fName.isNotEmpty());
    CARLA_SAFE_ASSERT(fServerPathTCP.isNotEmpty());
    CARLA_SAFE_ASSERT(fServerPathUDP.isNotEmpty());
    CARLA_SAFE_ASSERT(fServerTCP != nullptr);
    CARLA_SAFE_ASSERT(fServerUDP != nullptr);
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
    CARLA_SAFE_ASSERT(fName.isNotEmpty());
    CARLA_SAFE_ASSERT(fServerPathTCP.isNotEmpty());
    CARLA_SAFE_ASSERT(fServerPathUDP.isNotEmpty());
    CARLA_SAFE_ASSERT(fServerTCP != nullptr);
    CARLA_SAFE_ASSERT(fServerUDP != nullptr);
    carla_debug("CarlaEngineOsc::close()");

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

#ifndef BUILD_BRIDGE
    fControlData.clear();
#endif
}

// -----------------------------------------------------------------------

int CarlaEngineOsc::handleMessage(const bool isTCP, const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg)
{
    CARLA_SAFE_ASSERT_RETURN(fName.isNotEmpty(), 1);
    CARLA_SAFE_ASSERT_RETURN(path != nullptr && path[0] != '\0', 1);
#ifdef DEBUG
    if (std::strstr(path, "/bridge_pong") == nullptr) {
        carla_debug("CarlaEngineOsc::handleMessage(%s, \"%s\", %i, %p, \"%s\", %p)", bool2str(isTCP), path, argc, argv, types, msg);
    }
#endif

    //if (isTCP)
    {
        CARLA_SAFE_ASSERT_RETURN(fServerPathTCP.isNotEmpty(), 1);
        CARLA_SAFE_ASSERT_RETURN(fServerTCP != nullptr, 1);
    }
    //else
    {
        CARLA_SAFE_ASSERT_RETURN(fServerPathUDP.isNotEmpty(), 1);
        CARLA_SAFE_ASSERT_RETURN(fServerUDP != nullptr, 1);
    }

#ifndef BUILD_BRIDGE
    // Initial path check
    if (std::strcmp(path, "/register") == 0)
    {
        const lo_address source(lo_message_get_source(msg));
        return handleMsgRegister(isTCP, argc, argv, types, source);
    }
    if (std::strcmp(path, "/unregister") == 0)
    {
        return handleMsgUnregister();
    }
#endif

    const size_t nameSize(fName.length());

    // Check if message is for this client
    if (std::strlen(path) <= nameSize || std::strncmp(path+1, fName, nameSize) != 0)
    {
        carla_stderr("CarlaEngineOsc::handleMessage() - message not for this client -> '%s' != '/%s/'", path, fName.buffer());
        return 1;
    }

    // Get plugin id from path, "/carla/23/method" -> 23
    uint pluginId = 0;
    size_t offset;

    if (std::isdigit(path[nameSize+2]))
    {
        if (std::isdigit(path[nameSize+3]))
        {
            if (std::isdigit(path[nameSize+5]))
            {
                carla_stderr2("CarlaEngineOsc::handleMessage() - invalid plugin id, over 999? (value: \"%s\")", path+(nameSize+1));
                return 1;
            }
            else if (std::isdigit(path[nameSize+4]))
            {
                // 3 digits, /xyz/method
                offset    = 6;
                pluginId += uint(path[nameSize+2]-'0')*100;
                pluginId += uint(path[nameSize+3]-'0')*10;
                pluginId += uint(path[nameSize+4]-'0');
            }
            else
            {
                // 2 digits, /xy/method
                offset    = 5;
                pluginId += uint(path[nameSize+2]-'0')*10;
                pluginId += uint(path[nameSize+3]-'0');
            }
        }
        else
        {
            // single digit, /x/method
            offset    = 4;
            pluginId += uint(path[nameSize+2]-'0');
        }
    }
    else
    {
        carla_stderr("CarlaEngineOsc::handleMessage() - invalid message '%s'", path);
        return 1;
    }

    if (pluginId > fEngine->getCurrentPluginCount())
    {
        carla_stderr("CarlaEngineOsc::handleMessage() - failed to get plugin, wrong id '%i'", pluginId);
        return 0;
    }

    // Get plugin
    CarlaPlugin* const plugin(fEngine->getPluginUnchecked(pluginId));

    if (plugin == nullptr || plugin->getId() != pluginId)
    {
        carla_stderr("CarlaEngineOsc::handleMessage() - invalid plugin id '%i', probably has been removed (path: '%s')", pluginId, path);
        return 0;
    }

    // Get method from path, "/Carla/i/method" -> "method"
    char method[32+1];
    method[32] = '\0';
    std::strncpy(method, path + (nameSize + offset), 32);

    if (method[0] == '\0')
    {
        carla_stderr("CarlaEngineOsc::handleMessage(%s, \"%s\", ...) - received message without method", bool2str(isTCP), path);
        return 0;
    }

    // Common OSC methods (all bridges)
    if (std::strcmp(method, "update") == 0)
    {
        const lo_address source(lo_message_get_source(msg));
        return handleMsgUpdate(plugin, argc, argv, types, source);
    }
    if (std::strcmp(method, "exiting") == 0)
        return handleMsgExiting(plugin);

#ifndef BUILD_BRIDGE
    // Common OSC methods (DSSI and bridge UIs)
    if (std::strcmp(method, "configure") == 0)
        return handleMsgConfigure(plugin, argc, argv, types);
    if (std::strcmp(method, "control") == 0)
        return handleMsgControl(plugin, argc, argv, types);
    if (std::strcmp(method, "program") == 0)
        return handleMsgProgram(plugin, argc, argv, types);
    if (std::strcmp(method, "midi") == 0)
        return handleMsgMidi(plugin, argc, argv, types);

    // Internal methods
    if (std::strcmp(method, "set_active") == 0)
        return handleMsgSetActive(plugin, argc, argv, types);
    if (std::strcmp(method, "set_drywet") == 0)
        return handleMsgSetDryWet(plugin, argc, argv, types);
    if (std::strcmp(method, "set_volume") == 0)
        return handleMsgSetVolume(plugin, argc, argv, types);
    if (std::strcmp(method, "set_balance_left") == 0)
        return handleMsgSetBalanceLeft(plugin, argc, argv, types);
    if (std::strcmp(method, "set_balance_right") == 0)
        return handleMsgSetBalanceRight(plugin, argc, argv, types);
    if (std::strcmp(method, "set_panning") == 0)
        return handleMsgSetPanning(plugin, argc, argv, types);
    if (std::strcmp(method, "set_parameter_value") == 0)
        return handleMsgSetParameterValue(plugin, argc, argv, types);
    if (std::strcmp(method, "set_parameter_midi_cc") == 0)
        return handleMsgSetParameterMidiCC(plugin, argc, argv, types);
    if (std::strcmp(method, "set_parameter_midi_channel") == 0)
        return handleMsgSetParameterMidiChannel(plugin, argc, argv, types);
    if (std::strcmp(method, "set_program") == 0)
        return handleMsgSetProgram(plugin, argc, argv, types);
    if (std::strcmp(method, "set_midi_program") == 0)
        return handleMsgSetMidiProgram(plugin, argc, argv, types);
    if (std::strcmp(method, "note_on") == 0)
        return handleMsgNoteOn(plugin, argc, argv, types);
    if (std::strcmp(method, "note_off") == 0)
        return handleMsgNoteOff(plugin, argc, argv, types);

    // Plugin Bridges
    if ((plugin->getHints() & PLUGIN_IS_BRIDGE) != 0 && std::strlen(method) >= 11 && std::strncmp(method, "bridge_", 7) == 0)
    {
        const char* const bmethod(method+7);

        if (std::strcmp(bmethod, "pong") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscPong, argc, argv, types);
        if (std::strcmp(bmethod, "plugin_info1") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscPluginInfo1, argc, argv, types);
        if (std::strcmp(bmethod, "plugin_info2") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscPluginInfo2, argc, argv, types);
        if (std::strcmp(bmethod, "audio_count") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscAudioCount, argc, argv, types);
        if (std::strcmp(bmethod, "midi_count") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscMidiCount, argc, argv, types);
        if (std::strcmp(bmethod, "parameter_count") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscParameterCount, argc, argv, types);
        if (std::strcmp(bmethod, "program_count") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscProgramCount, argc, argv, types);
        if (std::strcmp(bmethod, "midi_program_count") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscMidiProgramCount, argc, argv, types);
        if (std::strcmp(bmethod, "parameter_data1") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscParameterData1, argc, argv, types);
        if (std::strcmp(bmethod, "parameter_data2") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscParameterData2, argc, argv, types);
        if (std::strcmp(bmethod, "parameter_ranges1") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscParameterRanges1, argc, argv, types);
        if (std::strcmp(bmethod, "parameter_ranges2") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscParameterRanges2, argc, argv, types);
        if (std::strcmp(bmethod, "parameter_value") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscParameterValue, argc, argv, types);
        if (std::strcmp(bmethod, "default_value") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscDefaultValue, argc, argv, types);
        if (std::strcmp(bmethod, "current_program") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscCurrentProgram, argc, argv, types);
        if (std::strcmp(bmethod, "current_midi_program") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscCurrentMidiProgram, argc, argv, types);
        if (std::strcmp(bmethod, "program_name") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscProgramName, argc, argv, types);
        if (std::strcmp(bmethod, "midi_program_data") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscMidiProgramData, argc, argv, types);
        if (std::strcmp(bmethod, "configure") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscConfigure, argc, argv, types);
        if (std::strcmp(bmethod, "set_custom_data") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscSetCustomData, argc, argv, types);
        if (std::strcmp(bmethod, "set_chunk_data_file") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscSetChunkDataFile, argc, argv, types);
        if (std::strcmp(bmethod, "latency") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscLatency, argc, argv, types);
        if (std::strcmp(bmethod, "ready") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscReady, argc, argv, types);
        if (std::strcmp(bmethod, "error") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeOscError, argc, argv, types);
    }

    // Plugin-specific methods
    if (std::strcmp(method, "lv2_atom_transfer") == 0)
        return handleMsgLv2AtomTransfer(plugin, argc, argv, types);
    if (std::strcmp(method, "lv2_urid_map") == 0)
        return handleMsgLv2UridMap(plugin, argc, argv, types);
#endif

    carla_stderr("CarlaEngineOsc::handleMessage() - unsupported OSC method '%s'", method);
    return 1;
}

// -----------------------------------------------------------------------

int CarlaEngineOsc::handleMsgUpdate(CARLA_ENGINE_OSC_HANDLE_ARGS2, const lo_address source)
{
    carla_debug("CarlaEngineOsc::handleMsgUpdate()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "s");

    const char* const url = (const char*)&argv[0]->s;

    plugin->updateOscData(source, url);
    return 0;
}

int CarlaEngineOsc::handleMsgExiting(CARLA_ENGINE_OSC_HANDLE_ARGS1)
{
    carla_debug("CarlaEngineOsc::handleMsgExiting()");

    // TODO - check for non-UIs (dssi-vst) and set to -1 instead
    fEngine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, plugin->getId(), 0, 0, 0.0f, nullptr);

    plugin->showCustomUI(false);
    return 0;
}

// -----------------------------------------------------------------------

#ifndef BUILD_BRIDGE
int CarlaEngineOsc::handleMsgRegister(const bool isTCP, const int argc, const lo_arg* const* const argv, const char* const types, const lo_address source)
{
    carla_debug("CarlaEngineOsc::handleMsgRegister()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "s");

    if (fControlData.path != nullptr)
    {
        carla_stderr("CarlaEngineOsc::handleMsgRegister() - OSC backend already registered to %s", fControlData.path);
        return 1;
    }

    const char* const url = &argv[0]->s;

    carla_debug("CarlaEngineOsc::handleMsgRegister() - OSC backend registered to %s", url);

    {
        const char* host = lo_address_get_hostname(source);
        const char* port = lo_address_get_port(source);
        fControlData.source = lo_address_new_with_proto(isTCP ? LO_TCP : LO_UDP, host, port);
    }

    {
        char* host = lo_url_get_hostname(url);
        char* port = lo_url_get_port(url);
        fControlData.path   = carla_strdup_free(lo_url_get_path(url));
        fControlData.target = lo_address_new_with_proto(isTCP ? LO_TCP : LO_UDP, host, port);

        std::free(host);
        std::free(port);
    }

    for (uint i=0, count=fEngine->getCurrentPluginCount(); i < count; ++i)
    {
        CarlaPlugin* const plugin(fEngine->getPluginUnchecked(i));

        if (plugin != nullptr && plugin->isEnabled())
            plugin->registerToOscClient();
    }

    return 0;
}

int CarlaEngineOsc::handleMsgUnregister()
{
    carla_debug("CarlaEngineOsc::handleMsgUnregister()");

    if (fControlData.path == nullptr)
    {
        carla_stderr("CarlaEngineOsc::handleMsgUnregister() - OSC backend is not registered yet");
        return 1;
    }

    fControlData.clear();
    return 0;
}

// -----------------------------------------------------------------------

int CarlaEngineOsc::handleMsgConfigure(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgConfigure()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ss");

    const char* const key   = (const char*)&argv[0]->s;
    const char* const value = (const char*)&argv[1]->s;

    plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, key, value, false);
    return 0;
}

int CarlaEngineOsc::handleMsgControl(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgControl()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "if");

    const int32_t rindex = argv[0]->i;
    const float   value  = argv[1]->f;

    plugin->setParameterValueByRealIndex(rindex, value, false, true, true);
    return 0;
}

int CarlaEngineOsc::handleMsgProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgProgram()");

    if (argc == 2)
    {
        CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ii");

        const int32_t bank    = argv[0]->i;
        const int32_t program = argv[1]->i;

        CARLA_SAFE_ASSERT_RETURN(bank >= 0, 0);
        CARLA_SAFE_ASSERT_RETURN(program >= 0, 0);

        plugin->setMidiProgramById(static_cast<uint32_t>(bank), static_cast<uint32_t>(program), false, true, true);
        return 0;
    }
    else
    {
        CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

        const int32_t program = argv[0]->i;

        CARLA_SAFE_ASSERT_RETURN(program >= 0, 0);
        CARLA_SAFE_ASSERT_RETURN(program < static_cast<int32_t>(plugin->getProgramCount()), 0);

        plugin->setProgram(program, false, true, true);
        return 0;
    }
}

int CarlaEngineOsc::handleMsgMidi(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgMidi()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "m");

#ifdef BUILD_BRIDGE
    CARLA_ASSERT(false); // this should never happen
    return 0;

    // unused
    (void)plugin;
    (void)argv;
#else
    if (plugin->getMidiInCount() == 0)
    {
        carla_stderr("CarlaEngineOsc::handleMsgMidi() - received midi when plugin has no midi inputs");
        return 0;
    }

    const uint8_t* const data = argv[0]->m;
    uint8_t status  = data[1];
    uint8_t channel = status & 0x0F;

    // Fix bad note-off
    if (MIDI_IS_STATUS_NOTE_ON(status) && data[3] == 0)
        status = MIDI_STATUS_NOTE_OFF;

    if (MIDI_IS_STATUS_NOTE_OFF(status))
    {
        const uint8_t note = data[2];

        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE, 0);

        plugin->sendMidiSingleNote(channel, note, 0, false, true, true);
    }
    else if (MIDI_IS_STATUS_NOTE_ON(status))
    {
        const uint8_t note = data[2];
        const uint8_t velo = data[3];

        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE, 0);
        CARLA_SAFE_ASSERT_RETURN(velo < MAX_MIDI_VALUE, 0);

        plugin->sendMidiSingleNote(channel, note, velo, false, true, true);
    }

    return 0;
#endif
}

// -----------------------------------------------------------------------

int CarlaEngineOsc::handleMsgSetActive(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetActive()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

    const bool active = (argv[0]->i != 0);

    plugin->setActive(active, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetDryWet(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetDryWet()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;

    plugin->setDryWet(value, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetVolume(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetVolume()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;

    plugin->setVolume(value, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetBalanceLeft(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetBalanceLeft()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;

    plugin->setBalanceLeft(value, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetBalanceRight(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetBalanceRight()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;

    plugin->setBalanceRight(value, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetPanning(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetPanning()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;

    plugin->setPanning(value, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetParameterValue(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetParameterValue()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "if");

    const int32_t index = argv[0]->i;
    const float   value = argv[1]->f;

    CARLA_SAFE_ASSERT_RETURN(index >= 0, 0);

    plugin->setParameterValue(static_cast<uint32_t>(index), value, true, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetParameterMidiCC(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetParameterMidiCC()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t index = argv[0]->i;
    const int32_t cc    = argv[1]->i;

    CARLA_SAFE_ASSERT_RETURN(index >= 0, 0);
    CARLA_SAFE_ASSERT_RETURN(cc >= -1 && cc <= 0x5F, 0);

    plugin->setParameterMidiCC(static_cast<uint32_t>(index), static_cast<int16_t>(cc), false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetParameterMidiChannel(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetParameterMidiChannel()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t index   = argv[0]->i;
    const int32_t channel = argv[1]->i;

    CARLA_SAFE_ASSERT_RETURN(index >= 0, 0);
    CARLA_SAFE_ASSERT_RETURN(channel >= 0 && channel < MAX_MIDI_CHANNELS, 0);

    plugin->setParameterMidiChannel(static_cast<uint32_t>(index), static_cast<uint8_t>(channel), false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetProgram()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

    const int32_t index = argv[0]->i;

    CARLA_SAFE_ASSERT_RETURN(index >= -1, 0);

    plugin->setProgram(index, true, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetMidiProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetMidiProgram()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

    const int32_t index = argv[0]->i;

    CARLA_SAFE_ASSERT_RETURN(index >= -1, 0);

    plugin->setMidiProgram(index, true, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgNoteOn(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgNoteOn()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(3, "iii");

    const int32_t channel = argv[0]->i;
    const int32_t note    = argv[1]->i;
    const int32_t velo    = argv[2]->i;

    CARLA_SAFE_ASSERT_RETURN(channel >= 0 && channel < MAX_MIDI_CHANNELS, 0);
    CARLA_SAFE_ASSERT_RETURN(note >= 0 && note < MAX_MIDI_NOTE, 0);
    CARLA_SAFE_ASSERT_RETURN(velo >= 0 && velo < MAX_MIDI_VALUE, 0);

    plugin->sendMidiSingleNote(static_cast<uint8_t>(channel), static_cast<uint8_t>(note), static_cast<uint8_t>(velo), true, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgNoteOff(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgNoteOff()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t channel = argv[0]->i;
    const int32_t note    = argv[1]->i;

    CARLA_SAFE_ASSERT_RETURN(channel >= 0 && channel < MAX_MIDI_CHANNELS, 0);
    CARLA_SAFE_ASSERT_RETURN(note >= 0 && note < MAX_MIDI_NOTE, 0);

    plugin->sendMidiSingleNote(static_cast<uint8_t>(channel), static_cast<uint8_t>(note), 0, true, false, true);
    return 0;
}
#endif // ! BUILD_BRIDGE

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
