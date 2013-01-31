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

#include "carla_engine.hpp"
#include "carla_engine_osc.hpp"
#include "carla_plugin.hpp"
#include "carla_midi.h"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

CarlaEngineOsc::CarlaEngineOsc(CarlaEngine* const engine)
    : kEngine(engine),
      fServerTCP(nullptr),
      fServerUDP(nullptr)
{
    qDebug("CarlaEngineOsc::CarlaEngineOsc(%p)", engine);
    CARLA_ASSERT(engine != nullptr);
}

CarlaEngineOsc::~CarlaEngineOsc()
{
    qDebug("CarlaEngineOsc::~CarlaEngineOsc()");
    CARLA_ASSERT(fName.isEmpty());
    CARLA_ASSERT(fServerPathTCP.isEmpty());
    CARLA_ASSERT(fServerPathUDP.isEmpty());
    CARLA_ASSERT(fServerTCP == nullptr);
    CARLA_ASSERT(fServerUDP == nullptr);
}

// -----------------------------------------------------------------------

void CarlaEngineOsc::init(const char* const name)
{
    qDebug("CarlaEngineOsc::init(\"%s\")", name);
    CARLA_ASSERT(fName.isEmpty());
    CARLA_ASSERT(fServerPathTCP.isEmpty());
    CARLA_ASSERT(fServerPathUDP.isEmpty());
    CARLA_ASSERT(fServerTCP == nullptr);
    CARLA_ASSERT(fServerUDP == nullptr);
    CARLA_ASSERT(name);

    fName = name;
    fName.toBasic();

    fServerTCP = lo_server_new_with_proto(nullptr, LO_TCP, osc_error_handler_TCP);

    if (fServerTCP != nullptr)
    {
        if (char* const tmpServerPathTCP = lo_server_get_url(fServerTCP))
        {
            fServerPathTCP  = tmpServerPathTCP;
            fServerPathTCP += fName;
            free(tmpServerPathTCP);
        }

        lo_server_add_method(fServerTCP, nullptr, nullptr, osc_message_handler, this);
    }

    fServerUDP = lo_server_new_with_proto(nullptr, LO_UDP, osc_error_handler_UDP);

    if (fServerUDP != nullptr)
    {
        if (char* const tmpServerPathUDP = lo_server_get_url(fServerUDP))
        {
            fServerPathUDP  = tmpServerPathUDP;
            fServerPathUDP += fName;
            free(tmpServerPathUDP);
        }

        lo_server_add_method(fServerUDP, nullptr, nullptr, osc_message_handler, this);
    }
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
    qDebug("CarlaEngineOsc::close()");
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
}

// -----------------------------------------------------------------------

bool isDigit(const char c)
{
    return (c >= '0' && c <= '9');
}

int CarlaEngineOsc::handleMessage(const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg)
{
#if DEBUG
    qDebug("CarlaEngineOsc::handleMessage(%s, %i, %p, %s, %p)", path, argc, argv, types, msg);
#endif
    CARLA_ASSERT(fName.isNotEmpty());
    CARLA_ASSERT(fServerPathTCP.isNotEmpty() || fServerPathUDP.isNotEmpty());
    CARLA_ASSERT(fServerTCP != nullptr || fServerUDP != nullptr);
    CARLA_ASSERT(path != nullptr);

    if (path == nullptr)
    {
        qCritical("CarlaEngineOsc::handleMessage() - got invalid path");
        return 1;
    }

    if (fName.isEmpty())
    {
        qCritical("CarlaEngineOsc::handleMessage(\"%s\", ...) - received message but client is offline", path);
        return 1;
    }

#ifndef BUILD_BRIDGE
    // Initial path check
    if (strcmp(path, "/register") == 0)
    {
        const lo_address source = lo_message_get_source(msg);
        return handleMsgRegister(argc, argv, types, source);
    }
    if (strcmp(path, "/unregister") == 0)
    {
        return handleMsgUnregister();
    }
#endif

    const size_t nameSize = fName.length();

    // Check if message is for this client
    if (std::strlen(path) <= nameSize || std::strncmp(path+1, (const char*)fName, nameSize) != 0)
    {
        qWarning("CarlaEngineOsc::handleMessage() - message not for this client -> '%s' != '/%s/'", path, (const char*)fName);
        return 1;
    }

    // Get plugin id from message
    // eg, /carla/23/method

    unsigned int pluginId = 0;

    if (isDigit(path[nameSize+2]))
    {
        if (isDigit(path[nameSize+3]))
        {
            if (isDigit(path[nameSize+5]))
            {
                qCritical("CarlaEngineOsc::handleMessage() - invalid plugin id, over 999? (value: \"%s\")", path+nameSize);
                return 1;
            }
            else if (isDigit(path[nameSize+4]))
            {
                // 3 digits, /xyz/method
                pluginId += (path[nameSize+2]-'0')*100;
                pluginId += (path[nameSize+3]-'0')*10;
                pluginId += (path[nameSize+4]-'0');
            }
            else
            {
                // 2 digits, /xy/method
                pluginId += (path[nameSize+2]-'0')*10;
                pluginId += (path[nameSize+3]-'0');
            }
        }
        else
        {
            // single digit, /x/method
            pluginId += path[nameSize+2]-'0';
        }
    }

    if (pluginId > kEngine->currentPluginCount())
    {
        qCritical("CarlaEngineOsc::handleMessage() - failed to get plugin, wrong id '%i'", pluginId);
        return 1;
    }

    // Get plugin
    CarlaPlugin* const plugin = kEngine->getPluginUnchecked(pluginId);

    if (plugin == nullptr || plugin->id() != pluginId)
    {
        qWarning("CarlaEngineOsc::handleMessage() - invalid plugin id '%i', probably has been removed", pluginId);
        return 1;
    }

    // Get method from path, "/Carla/i/method" -> "method"
    const int offset = (pluginId >= 10) ? 5 : 4;
    char  method[32] = { 0 };
    strncpy(method, path + (nameSize + offset), 31);

    if (method[0] == '\0')
    {
        qWarning("CarlaEngineOsc::handleMessage(\"%s\", ...) - received message without method", path);
        return 1;
    }

    // Common OSC methods (DSSI and internal UIs)
    if (strcmp(method, "update") == 0)
    {
        const lo_address source = lo_message_get_source(msg);
        return handleMsgUpdate(plugin, argc, argv, types, source);
    }
    if (strcmp(method, "configure") == 0)
        return handleMsgConfigure(plugin, argc, argv, types);
    if (strcmp(method, "control") == 0)
        return handleMsgControl(plugin, argc, argv, types);
    if (strcmp(method, "program") == 0)
        return handleMsgProgram(plugin, argc, argv, types);
    if (strcmp(method, "midi") == 0)
        return handleMsgMidi(plugin, argc, argv, types);
    if (strcmp(method, "exiting") == 0)
        return handleMsgExiting(plugin);

#ifndef BUILD_BRIDGE
    // Internal methods
    if (strcmp(method, "set_active") == 0)
        return handleMsgSetActive(plugin, argc, argv, types);
    if (strcmp(method, "set_drywet") == 0)
        return handleMsgSetDryWet(plugin, argc, argv, types);
    if (strcmp(method, "set_volume") == 0)
        return handleMsgSetVolume(plugin, argc, argv, types);
    if (strcmp(method, "set_balance_left") == 0)
        return handleMsgSetBalanceLeft(plugin, argc, argv, types);
    if (strcmp(method, "set_balance_right") == 0)
        return handleMsgSetBalanceRight(plugin, argc, argv, types);
    if (strcmp(method, "set_parameter_value") == 0)
        return handleMsgSetParameterValue(plugin, argc, argv, types);
    if (strcmp(method, "set_parameter_midi_cc") == 0)
        return handleMsgSetParameterMidiCC(plugin, argc, argv, types);
    if (strcmp(method, "set_parameter_midi_channel") == 0)
        return handleMsgSetParameterMidiChannel(plugin, argc, argv, types);
    if (strcmp(method, "set_program") == 0)
        return handleMsgSetProgram(plugin, argc, argv, types);
    if (strcmp(method, "set_midi_program") == 0)
        return handleMsgSetMidiProgram(plugin, argc, argv, types);
    if (strcmp(method, "note_on") == 0)
        return handleMsgNoteOn(plugin, argc, argv, types);
    if (strcmp(method, "note_off") == 0)
        return handleMsgNoteOff(plugin, argc, argv, types);

#if 0 // FIXME
    // Plugin Bridges
    if ((plugin->hints() & PLUGIN_IS_BRIDGE) > 0 && strlen(method) > 11 && strncmp(method, "bridge_", 7) == 0)
    {
        if (strcmp(method+7, "set_inpeak") == 0)
            return handleMsgBridgeSetInPeak(plugin, argc, argv, types);
        if (strcmp(method+7, "set_outpeak") == 0)
            return handleMsgBridgeSetOutPeak(plugin, argc, argv, types);
        if (strcmp(method+7, "audio_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeAudioCount, argc, argv, types);
        if (strcmp(method+7, "midi_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeMidiCount, argc, argv, types);
        if (strcmp(method+7, "parameter_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeParameterCount, argc, argv, types);
        if (strcmp(method+7, "program_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeProgramCount, argc, argv, types);
        if (strcmp(method+7, "midi_program_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeMidiProgramCount, argc, argv, types);
        if (strcmp(method+7, "plugin_info") == 0)
            return plugin->setOscBridgeInfo(PluginBridgePluginInfo, argc, argv, types);
        if (strcmp(method+7, "parameter_info") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeParameterInfo, argc, argv, types);
        if (strcmp(method+7, "parameter_data") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeParameterData, argc, argv, types);
        if (strcmp(method+7, "parameter_ranges") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeParameterRanges, argc, argv, types);
        if (strcmp(method+7, "program_info") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeProgramInfo, argc, argv, types);
        if (strcmp(method+7, "midi_program_info") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeMidiProgramInfo, argc, argv, types);
        if (strcmp(method+7, "configure") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeConfigure, argc, argv, types);
        if (strcmp(method+7, "set_parameter_value") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetParameterValue, argc, argv, types);
        if (strcmp(method+7, "set_default_value") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetDefaultValue, argc, argv, types);
        if (strcmp(method+7, "set_program") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetProgram, argc, argv, types);
        if (strcmp(method+7, "set_midi_program") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetMidiProgram, argc, argv, types);
        if (strcmp(method+7, "set_custom_data") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetCustomData, argc, argv, types);
        if (strcmp(method+7, "set_chunk_data") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetChunkData, argc, argv, types);
        if (strcmp(method+7, "update") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeUpdateNow, argc, argv, types);
        if (strcmp(method+7, "error") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeError, argc, argv, types);
    }
#endif
#endif

    // Plugin-specific methods, FIXME
#if 0 //def WANT_LV2
    if (strcmp(method, "lv2_atom_transfer") == 0)
        return handleMsgLv2AtomTransfer(plugin, argc, argv, types);
    if (strcmp(method, "lv2_event_transfer") == 0)
        return handleMsgLv2EventTransfer(plugin, argc, argv, types);
#endif

    qWarning("CarlaEngineOsc::handleMessage() - unsupported OSC method '%s'", method);
    return 1;
}

// -----------------------------------------------------------------------

#ifndef BUILD_BRIDGE
int CarlaEngineOsc::handleMsgRegister(const int argc, const lo_arg* const* const argv, const char* const types, const lo_address source)
{
    qDebug("CarlaEngineOsc::handleMsgRegister()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "s");

    if (fControlData.path != nullptr)
    {
        qWarning("CarlaEngineOsc::handleMsgRegister() - OSC backend already registered to %s", fControlData.path);
        return 1;
    }

    const char* const url = (const char*)&argv[0]->s;
    const char* host;
    const char* port;

    qDebug("CarlaEngineOsc::handleMsgRegister() - OSC backend registered to %s", url);

    host = lo_address_get_hostname(source);
    port = lo_address_get_port(source);
    fControlData.source = lo_address_new_with_proto(LO_TCP, host, port);

    host = lo_url_get_hostname(url);
    port = lo_url_get_port(url);
    fControlData.path   = lo_url_get_path(url);
    fControlData.target = lo_address_new_with_proto(LO_TCP, host, port);

    free((void*)host);
    free((void*)port);

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
    qDebug("CarlaEngineOsc::handleMsgUnregister()");

    if (fControlData.path == nullptr)
    {
        qWarning("CarlaEngineOsc::handleMsgUnregister() - OSC backend is not registered yet");
        return 1;
    }

    fControlData.free();
    return 0;
}
#endif

// -----------------------------------------------------------------------

int CarlaEngineOsc::handleMsgUpdate(CARLA_ENGINE_OSC_HANDLE_ARGS2, const lo_address source)
{
    qDebug("CarlaEngineOsc::handleMsgUpdate()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "s");

    const char* const url = (const char*)&argv[0]->s;

    plugin->updateOscData(source, url);

    return 0;
}

int CarlaEngineOsc::handleMsgConfigure(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgConfigure()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ss");

    const char* const key   = (const char*)&argv[0]->s;
    const char* const value = (const char*)&argv[1]->s;

    // FIXME
    plugin->setCustomData("CUSTOM_DATA_STRING", key, value, false);

    return 0;
}

int CarlaEngineOsc::handleMsgControl(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgControl()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "if");

    const int32_t rindex = argv[0]->i;
    const float   value  = argv[1]->f;

    plugin->setParameterValueByRIndex(rindex, value, false, true, true);

    return 0;
}

int CarlaEngineOsc::handleMsgProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgProgram()");

    if (argc == 2)
    {
        CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ii");

        const uint32_t bank    = argv[0]->i;
        const uint32_t program = argv[1]->i;

        plugin->setMidiProgramById(bank, program, false, true, true, true);

        return 0;
    }
    else
    {
        CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

        const uint32_t program = argv[0]->i;

        if (program < plugin->programCount())
        {
            plugin->setProgram(program, false, true, true, true);
            return 0;
        }

        qCritical("CarlaEngineOsc::handleMsgProgram() - program_id '%i' out of bounds", program);
    }

    return 1;
}

int CarlaEngineOsc::handleMsgMidi(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgMidi()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "m");

    if (plugin->midiInCount() > 0)
    {
#if 0
        const uint8_t* const data = argv[0]->m;
        uint8_t status  = data[1];
        uint8_t channel = status & 0x0F;

        // Fix bad note-off
        if (MIDI_IS_STATUS_NOTE_ON(status) && data[3] == 0)
            status -= 0x10;

        if (MIDI_IS_STATUS_NOTE_OFF(status))
        {
            uint8_t note = data[2];
            plugin->sendMidiSingleNote(channel, note, 0, false, true, true);
        }
        else if (MIDI_IS_STATUS_NOTE_ON(status))
        {
            uint8_t note = data[2];
            uint8_t velo = data[3];
            plugin->sendMidiSingleNote(channel, note, velo, false, true, true);
        }
#endif

        return 0;
    }

    qWarning("CarlaEngineOsc::handleMsgMidi() - recived midi when plugin has no midi inputs");
    return 1;
}

int CarlaEngineOsc::handleMsgExiting(CARLA_ENGINE_OSC_HANDLE_ARGS1)
{
    qDebug("CarlaEngineOsc::handleMsgExiting()");

    // TODO - check for non-UIs (dssi-vst) and set to -1 instead
    kEngine->callback(CALLBACK_SHOW_GUI, plugin->id(), 0, 0, 0.0f, nullptr);

    plugin->freeOscData();

    return 0;
}

// -----------------------------------------------------------------------

#ifndef BUILD_BRIDGE
int CarlaEngineOsc::handleMsgSetActive(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgSetActive()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

#if 0
    const bool active = (bool)argv[0]->i;
    plugin->setActive(active, false, true);
#endif

    return 0;
}

int CarlaEngineOsc::handleMsgSetDryWet(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgSetDryWet()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

#if 0
    const float value = argv[0]->f;
    plugin->setDryWet(value, false, true);
#endif

    return 0;
}

int CarlaEngineOsc::handleMsgSetVolume(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgSetVolume()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

#if 0
    const float value = argv[0]->f;
    plugin->setVolume(value, false, true);
#endif

    return 0;
}

int CarlaEngineOsc::handleMsgSetBalanceLeft(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgSetBalanceLeft()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

#if 0
    const float value = argv[0]->f;
    plugin->setBalanceLeft(value, false, true);
#endif

    return 0;
}

int CarlaEngineOsc::handleMsgSetBalanceRight(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgSetBalanceRight()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

#if 0
    const float value = argv[0]->f;
    plugin->setBalanceRight(value, false, true);
#endif

    return 0;
}

int CarlaEngineOsc::handleMsgSetParameterValue(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgSetParameterValue()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "if");

#if 0
    const int32_t index = argv[0]->i;
    const float   value = argv[1]->f;
    plugin->setParameterValue(index, value, true, false, true);
#endif

    return 0;
}

int CarlaEngineOsc::handleMsgSetParameterMidiCC(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgSetParameterMidiCC()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ii");

#if 0
    const int32_t index = argv[0]->i;
    const int32_t cc    = argv[1]->i;
    plugin->setParameterMidiCC(index, cc, false, true);
#endif

    return 0;
}

int CarlaEngineOsc::handleMsgSetParameterMidiChannel(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgSetParameterMidiChannel()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ii");

#if 0
    const int32_t index   = argv[0]->i;
    const int32_t channel = argv[1]->i;
    plugin->setParameterMidiChannel(index, channel, false, true);
#endif

    return 0;
}

int CarlaEngineOsc::handleMsgSetProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgSetProgram()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

#if 0
    const int32_t index = argv[0]->i;
    plugin->setProgram(index, true, false, true, true);

    // parameters might have changed, send all param values back
    if (m_controlData.target && index >= 0)
    {
        for (uint32_t i=0; i < plugin->parameterCount(); i++)
            engine->osc_send_control_set_parameter_value(plugin->id(), i, plugin->getParameterValue(i));
    }
#endif

    return 0;
}

int CarlaEngineOsc::handleMsgSetMidiProgram(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgSetMidiProgram()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

#if 0
    const int32_t index = argv[0]->i;
    plugin->setMidiProgram(index, true, false, true, true);

    // parameters might have changed, send all param values back
    if (m_controlData.target && index >= 0)
    {
        for (uint32_t i=0; i < plugin->parameterCount(); i++)
            engine->osc_send_control_set_parameter_value(plugin->id(), i, plugin->getParameterValue(i));
    }
#endif

    return 0;
}

int CarlaEngineOsc::handleMsgNoteOn(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgNoteOn()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(3, "iii");

#if 0
    const int32_t channel = argv[0]->i;
    const int32_t note    = argv[1]->i;
    const int32_t velo    = argv[2]->i;
    plugin->sendMidiSingleNote(channel, note, velo, true, false, true);
#endif

    return 0;
}

int CarlaEngineOsc::handleMsgNoteOff(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaEngineOsc::handleMsgNoteOff()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ii");

#if 0
    const int32_t channel = argv[0]->i;
    const int32_t note    = argv[1]->i;
    plugin->sendMidiSingleNote(channel, note, 0, true, false, true);
#endif

    return 0;
}

#if 0
int CarlaEngineOsc::handleMsgBridgeSetInPeak(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "id");

    const int32_t index = argv[0]->i;
    const double  value = argv[1]->d;
    engine->setInputPeak(plugin->id(), index-1, value);

    return 0;
}

int CarlaEngineOsc::handleMsgBridgeSetOutPeak(CARLA_ENGINE_OSC_HANDLE_ARGS2)
{
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "id");

    const int32_t index = argv[0]->i;
    const double  value = argv[1]->d;
    engine->setOutputPeak(plugin->id(), index-1, value);

    return 0;
}
#endif
#endif

CARLA_BACKEND_END_NAMESPACE
