/*
 * Carla Engine OSC
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaEngineOsc.hpp"

#include "CarlaEngine.hpp"
#include "CarlaPlugin.hpp"
#include "CarlaMIDI.h"

CARLA_BACKEND_START_NAMESPACE

#ifndef BUILD_BRIDGE
// -------------------------------------------------------------------
// Bridge Helper, defined in plugin/CarlaBlugin.cpp

extern int CarlaPluginSetOscBridgeInfo(CarlaPlugin* const plugin, const PluginBridgeInfoType type,
                                       const int argc, const lo_arg* const* const argv, const char* const types);
#endif

// -----------------------------------------------------------------------

CarlaEngineOsc::CarlaEngineOsc(CarlaEngine* const engine)
    : kEngine(engine),
      fServerTCP(nullptr),
      fServerUDP(nullptr)
{
    carla_debug("CarlaEngineOsc::CarlaEngineOsc(%p)", engine);
    CARLA_ASSERT(engine != nullptr);
}

CarlaEngineOsc::~CarlaEngineOsc()
{
    carla_debug("CarlaEngineOsc::~CarlaEngineOsc()");
    CARLA_ASSERT(fName.isEmpty());
    CARLA_ASSERT(fServerPathTCP.isEmpty());
    CARLA_ASSERT(fServerPathUDP.isEmpty());
    CARLA_ASSERT(fServerTCP == nullptr);
    CARLA_ASSERT(fServerUDP == nullptr);
}

// -----------------------------------------------------------------------

void CarlaEngineOsc::init(const char* const name)
{
    carla_debug("CarlaEngineOsc::init(\"%s\")", name);
    CARLA_ASSERT(fName.isEmpty());
    CARLA_ASSERT(fServerPathTCP.isEmpty());
    CARLA_ASSERT(fServerPathUDP.isEmpty());
    CARLA_ASSERT(fServerTCP == nullptr);
    CARLA_ASSERT(fServerUDP == nullptr);
    CARLA_ASSERT(name != nullptr);

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

    CARLA_ASSERT(fName.isNotEmpty());
    CARLA_ASSERT(fServerPathTCP.isNotEmpty());
    CARLA_ASSERT(fServerPathUDP.isNotEmpty());
    CARLA_ASSERT(fServerTCP != nullptr);
    CARLA_ASSERT(fServerUDP != nullptr);
}

void CarlaEngineOsc::idle()
{
    if (fServerTCP != nullptr)
    {
        while (lo_server_recv_noblock(fServerTCP, 0) != 0) {}
    }

    if (fServerUDP != nullptr)
    {
        while (lo_server_recv_noblock(fServerUDP, 0) != 0) {}
    }
}

void CarlaEngineOsc::close()
{
    carla_debug("CarlaEngineOsc::close()");
    CARLA_ASSERT(fName.isNotEmpty());
    CARLA_ASSERT(fServerPathTCP.isNotEmpty());
    CARLA_ASSERT(fServerPathUDP.isNotEmpty());
    CARLA_ASSERT(fServerTCP != nullptr);
    CARLA_ASSERT(fServerUDP != nullptr);

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
    fControlData.free();
#endif

    CARLA_ASSERT(fName.isEmpty());
    CARLA_ASSERT(fServerPathTCP.isEmpty());
    CARLA_ASSERT(fServerPathUDP.isEmpty());
    CARLA_ASSERT(fServerTCP == nullptr);
    CARLA_ASSERT(fServerUDP == nullptr);
}

// -----------------------------------------------------------------------

bool isDigit(const char c)
{
    return (c >= '0' && c <= '9');
}

int CarlaEngineOsc::handleMessage(const bool isTCP, const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg)
{
    carla_debug("CarlaEngineOsc::handleMessage(%s, \"%s\", %i, %p, \"%s\", %p)", bool2str(isTCP), path, argc, argv, types, msg);
    CARLA_ASSERT(fName.isNotEmpty());
    CARLA_ASSERT(fServerPathTCP.isNotEmpty());
    CARLA_ASSERT(fServerPathUDP.isNotEmpty());
    CARLA_ASSERT(fServerTCP != nullptr);
    CARLA_ASSERT(fServerUDP != nullptr);
    CARLA_ASSERT(path != nullptr);

    if (path == nullptr)
    {
        carla_stderr("CarlaEngineOsc::handleMessage() - got invalid path");
        return 1;
    }

    if (fName.isEmpty())
    {
        carla_stderr("CarlaEngineOsc::handleMessage(%s, \"%s\", ...) - received message but client is offline", bool2str(isTCP), path);
        return 1;
    }

#ifndef BUILD_BRIDGE
    // Initial path check
    if (std::strcmp(path, "/register") == 0)
    {
        const lo_address source = lo_message_get_source(msg);
        return handleMsgRegister(isTCP, argc, argv, types, source);
    }
    if (std::strcmp(path, "/unregister") == 0)
    {
        return handleMsgUnregister();
    }
#endif

    const size_t nameSize = fName.length();

    // Check if message is for this client
    if (std::strlen(path) <= nameSize || std::strncmp(path+1, (const char*)fName, nameSize) != 0)
    {
        carla_stderr("CarlaEngineOsc::handleMessage() - message not for this client -> '%s' != '/%s/'", path, (const char*)fName);
        return 1;
    }

    // Get plugin id from message
    // eg, /carla/23/method

    unsigned int pluginId = 0;
    size_t offset;

    if (isDigit(path[nameSize+2]))
    {
        if (isDigit(path[nameSize+3]))
        {
            if (isDigit(path[nameSize+5]))
            {
                carla_stderr2("CarlaEngineOsc::handleMessage() - invalid plugin id, over 999? (value: \"%s\")", path+nameSize);
                return 1;
            }
            else if (isDigit(path[nameSize+4]))
            {
                // 3 digits, /xyz/method
                offset    = 6;
                pluginId += (path[nameSize+2]-'0')*100;
                pluginId += (path[nameSize+3]-'0')*10;
                pluginId += (path[nameSize+4]-'0');
            }
            else
            {
                // 2 digits, /xy/method
                offset    = 5;
                pluginId += (path[nameSize+2]-'0')*10;
                pluginId += (path[nameSize+3]-'0');
            }
        }
        else
        {
            // single digit, /x/method
            offset    = 4;
            pluginId += path[nameSize+2]-'0';
        }
    }

    if (pluginId > kEngine->currentPluginCount())
    {
        carla_stderr("CarlaEngineOsc::handleMessage() - failed to get plugin, wrong id '%i'", pluginId);
        return 1;
    }

    // Get plugin
    CarlaPlugin* const plugin = kEngine->getPluginUnchecked(pluginId);

    if (plugin == nullptr || plugin->id() != pluginId)
    {
        carla_stderr("CarlaEngineOsc::handleMessage() - invalid plugin id '%i', probably has been removed", pluginId);
        return 1;
    }

    // Get method from path, "/Carla/i/method" -> "method"
    char method[32] = { 0 };
    std::strncpy(method, path + (nameSize + offset), 31);

    if (method[0] == '\0')
    {
        carla_stderr("CarlaEngineOsc::handleMessage(%s, \"%s\", ...) - received message without method", bool2str(isTCP), path);
        return 1;
    }

    // Common OSC methods (DSSI and bridge UIs)
    if (std::strcmp(method, "update") == 0)
    {
        const lo_address source = lo_message_get_source(msg);
        return handleMsgUpdate(plugin, argc, argv, types, source);
    }
    if (std::strcmp(method, "configure") == 0)
        return handleMsgConfigure(plugin, argc, argv, types);
    if (std::strcmp(method, "control") == 0)
        return handleMsgControl(plugin, argc, argv, types);
    if (std::strcmp(method, "program") == 0)
        return handleMsgProgram(plugin, argc, argv, types);
    if (std::strcmp(method, "midi") == 0)
        return handleMsgMidi(plugin, argc, argv, types);
    if (std::strcmp(method, "exiting") == 0)
        return handleMsgExiting(plugin);

#ifndef BUILD_BRIDGE
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
    if ((plugin->hints() & PLUGIN_IS_BRIDGE) > 0 && strlen(method) > 11 && strncmp(method, "bridge_", 7) == 0)
    {
        if (std::strcmp(method+7, "set_peaks") == 0)
            return handleMsgBridgeSetPeaks(plugin, argc, argv, types);
        if (std::strcmp(method+7, "audio_count") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeAudioCount, argc, argv, types);
        if (std::strcmp(method+7, "midi_count") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeMidiCount, argc, argv, types);
        if (std::strcmp(method+7, "parameter_count") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeParameterCount, argc, argv, types);
        if (std::strcmp(method+7, "program_count") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeProgramCount, argc, argv, types);
        if (std::strcmp(method+7, "midi_program_count") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeMidiProgramCount, argc, argv, types);
        if (std::strcmp(method+7, "plugin_info") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgePluginInfo, argc, argv, types);
        if (std::strcmp(method+7, "parameter_info") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeParameterInfo, argc, argv, types);
        if (std::strcmp(method+7, "parameter_data") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeParameterData, argc, argv, types);
        if (std::strcmp(method+7, "parameter_ranges") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeParameterRanges, argc, argv, types);
        if (std::strcmp(method+7, "program_info") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeProgramInfo, argc, argv, types);
        if (std::strcmp(method+7, "midi_program_info") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeMidiProgramInfo, argc, argv, types);
        if (std::strcmp(method+7, "configure") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeConfigure, argc, argv, types);
        if (std::strcmp(method+7, "set_parameter_value") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeSetParameterValue, argc, argv, types);
        if (std::strcmp(method+7, "set_default_value") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeSetDefaultValue, argc, argv, types);
        if (std::strcmp(method+7, "set_program") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeSetProgram, argc, argv, types);
        if (std::strcmp(method+7, "set_midi_program") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeSetMidiProgram, argc, argv, types);
        if (std::strcmp(method+7, "set_custom_data") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeSetCustomData, argc, argv, types);
        if (std::strcmp(method+7, "set_chunk_data") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeSetChunkData, argc, argv, types);
        if (std::strcmp(method+7, "update") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeUpdateNow, argc, argv, types);
        if (std::strcmp(method+7, "error") == 0)
            return CarlaPluginSetOscBridgeInfo(plugin, kPluginBridgeError, argc, argv, types);
    }
#endif

    // Plugin-specific methods, FIXME
#if 0 //def WANT_LV2
    if (std::strcmp(method, "lv2_atom_transfer") == 0)
        return handleMsgLv2AtomTransfer(plugin, argc, argv, types);
    if (std::strcmp(method, "lv2_event_transfer") == 0)
        return handleMsgLv2EventTransfer(plugin, argc, argv, types);
#endif

    carla_stderr("CarlaEngineOsc::handleMessage() - unsupported OSC method '%s'", method);
    return 1;
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

    for (unsigned short i=0; i < kEngine->currentPluginCount(); i++)
    {
        CarlaPlugin* const plugin = kEngine->getPluginUnchecked(i);

        if (plugin && plugin->enabled())
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

    fControlData.free();
    return 0;
}
#endif

// -----------------------------------------------------------------------

int CarlaEngineOsc::handleMsgUpdate(CARLA_ENGINE_OSC_HANDLE_ARGS2, const lo_address source)
{
    carla_debug("CarlaEngineOsc::handleMsgUpdate()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "s");

    const char* const url = (const char*)&argv[0]->s;

    plugin->updateOscData(source, url);

    return 0;
}

int CarlaEngineOsc::handleMsgConfigure(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgConfigure()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ss");

    const char* const key   = (const char*)&argv[0]->s;
    const char* const value = (const char*)&argv[1]->s;

    plugin->setCustomData(CUSTOM_DATA_STRING, key, value, false);

    return 0;
}

int CarlaEngineOsc::handleMsgControl(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgControl()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "if");

    const int32_t rindex = argv[0]->i;
    const float   value  = argv[1]->f;

    plugin->setParameterValueByRIndex(rindex, value, false, true, true);

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

        CARLA_SAFE_ASSERT_INT(bank >= 0, bank);
        CARLA_SAFE_ASSERT_INT(program >= 0, program);

        if (bank < 0)
            return 1;
        if (program < 0)
            return 1;

        plugin->setMidiProgramById(static_cast<uint32_t>(bank), static_cast<uint32_t>(program), false, true, true, true);

        return 0;
    }
    else
    {
        CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

        const int32_t program = argv[0]->i;

        CARLA_SAFE_ASSERT_INT(program >= 0, program);

        if (program < 0)
            return 1;

        if (program < static_cast<int32_t>(plugin->programCount()))
        {
            plugin->setProgram(program, false, true, true, true);
            return 0;
        }

        carla_stderr("CarlaEngineOsc::handleMsgProgram() - programId '%i' out of bounds", program);
        return 1;
    }
}

int CarlaEngineOsc::handleMsgMidi(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgMidi()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "m");

    if (plugin->midiInCount() == 0)
    {
        carla_stderr("CarlaEngineOsc::handleMsgMidi() - recived midi when plugin has no midi inputs");
        return 1;
    }

    const uint8_t* const data = argv[0]->m;
    uint8_t status  = data[1];
    uint8_t channel = status & 0x0F;

    // Fix bad note-off
    if (MIDI_IS_STATUS_NOTE_ON(status) && data[3] == 0)
        status -= 0x10;

    if (MIDI_IS_STATUS_NOTE_OFF(status))
    {
        const uint8_t note = data[2];

        CARLA_SAFE_ASSERT_INT(note < MAX_MIDI_NOTE, note);

        if (note >= MAX_MIDI_NOTE)
            return 1;

        plugin->sendMidiSingleNote(channel, note, 0, false, true, true);
    }
    else if (MIDI_IS_STATUS_NOTE_ON(status))
    {
        const uint8_t note = data[2];
        const uint8_t velo = data[3];

        CARLA_SAFE_ASSERT_INT(note < MAX_MIDI_NOTE, note);
        CARLA_SAFE_ASSERT_INT(velo < MAX_MIDI_VALUE, velo);

        if (note >= MAX_MIDI_NOTE)
            return 1;
        if (velo >= MAX_MIDI_VALUE)
            return 1;

        plugin->sendMidiSingleNote(channel, note, velo, false, true, true);
    }

    return 0;
}

int CarlaEngineOsc::handleMsgExiting(CARLA_ENGINE_OSC_HANDLE_ARGS1)
{
    carla_debug("CarlaEngineOsc::handleMsgExiting()");

    // TODO - check for non-UIs (dssi-vst) and set to -1 instead
    kEngine->callback(CALLBACK_SHOW_GUI, plugin->id(), 0, 0, 0.0f, nullptr);

    plugin->freeOscData();

    return 0;
}

// -----------------------------------------------------------------------

#ifndef BUILD_BRIDGE
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

    CARLA_SAFE_ASSERT_INT(index >= 0, index);

    if (index < 0)
        return 1;

    plugin->setParameterValue(static_cast<uint32_t>(index), value, true, false, true);

    return 0;
}

int CarlaEngineOsc::handleMsgSetParameterMidiCC(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetParameterMidiCC()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t index = argv[0]->i;
    const int32_t cc    = argv[1]->i;

    CARLA_SAFE_ASSERT_INT(index >= 0, index);
    CARLA_SAFE_ASSERT_INT(cc >= -1 && cc <= 0x5F, cc);

    if (index < 0)
        return 1;
    if (cc < -1 || cc > 0x5F)
        return 1;

    plugin->setParameterMidiCC(static_cast<uint32_t>(index), static_cast<int16_t>(cc), false, true);

    return 0;
}

int CarlaEngineOsc::handleMsgSetParameterMidiChannel(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetParameterMidiChannel()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t index   = argv[0]->i;
    const int32_t channel = argv[1]->i;

    CARLA_SAFE_ASSERT_INT(index >= 0, index);
    CARLA_SAFE_ASSERT_INT(channel >= 0 && channel < MAX_MIDI_CHANNELS, channel);

    if (index < 0)
        return 1;
    if (channel < 0 || channel >= MAX_MIDI_CHANNELS)
        return 1;

    plugin->setParameterMidiChannel(static_cast<uint32_t>(index), static_cast<uint8_t>(channel), false, true);

    return 0;
}

int CarlaEngineOsc::handleMsgSetProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetProgram()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

    const int32_t index = argv[0]->i;

    plugin->setProgram(index, true, false, true, true);

    return 0;
}

int CarlaEngineOsc::handleMsgSetMidiProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgSetMidiProgram()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

    const int32_t index = argv[0]->i;

    plugin->setMidiProgram(index, true, false, true, true);

    return 0;
}

int CarlaEngineOsc::handleMsgNoteOn(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgNoteOn()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(3, "iii");

    const int32_t channel = argv[0]->i;
    const int32_t note    = argv[1]->i;
    const int32_t velo    = argv[2]->i;

    CARLA_SAFE_ASSERT_INT(channel >= 0 && channel < MAX_MIDI_CHANNELS, channel);
    CARLA_SAFE_ASSERT_INT(note >= 0 && note < MAX_MIDI_NOTE, note);
    CARLA_SAFE_ASSERT_INT(velo >= 0 && velo < MAX_MIDI_VALUE, velo);

    if (channel < 0 || channel >= MAX_MIDI_CHANNELS)
        return 1;
    if (note < 0 || note >= MAX_MIDI_NOTE)
        return 1;
    if (velo < 0 || velo >= MAX_MIDI_VALUE)
        return 1;

    plugin->sendMidiSingleNote(static_cast<uint8_t>(channel), static_cast<uint8_t>(note), static_cast<uint8_t>(velo), true, false, true);

    return 0;
}

int CarlaEngineOsc::handleMsgNoteOff(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    carla_debug("CarlaEngineOsc::handleMsgNoteOff()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t channel = argv[0]->i;
    const int32_t note    = argv[1]->i;

    CARLA_SAFE_ASSERT_INT(channel >= 0 && channel < MAX_MIDI_CHANNELS, channel);
    CARLA_SAFE_ASSERT_INT(note >= 0 && note < MAX_MIDI_NOTE, note);

    if (channel < 0 || channel >= MAX_MIDI_CHANNELS)
        return 1;
    if (note < 0 || note >= MAX_MIDI_NOTE)
        return 1;

    plugin->sendMidiSingleNote(static_cast<uint8_t>(channel), static_cast<uint8_t>(note), 0, true, false, true);

    return 0;
}

// FIXME - remove once IPC audio is implemented
int CarlaEngineOsc::handleMsgBridgeSetPeaks(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(4, "ffff");

    const float in1  = argv[0]->f;
    const float in2  = argv[1]->f;
    const float out1 = argv[2]->f;
    const float out2 = argv[3]->f;

    float const inPeaks[CarlaEngine::MAX_PEAKS] = { in1, in2 };
    float const outPeaks[CarlaEngine::MAX_PEAKS] = { out1, out2 };

    kEngine->setPeaks(plugin->id(), inPeaks, outPeaks);

    return 0;
}
#endif

CARLA_BACKEND_END_NAMESPACE
