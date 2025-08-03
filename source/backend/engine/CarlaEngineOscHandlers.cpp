 /*
 * Carla Plugin Host
 * Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"
#include "CarlaMIDI.h"

#include <cctype>

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

int CarlaEngineOsc::handleMessage(const bool isTCP, const char* const path,
                                  const int argc, const lo_arg* const* const argv, const char* const types,
                                  const lo_message msg)
{
    CARLA_SAFE_ASSERT_RETURN(fName.isNotEmpty(), 1);
    CARLA_SAFE_ASSERT_RETURN(path != nullptr && path[0] != '\0', 1);
    CARLA_SAFE_ASSERT_RETURN(path[0] == '/', 1);
#ifdef DEBUG
    if (std::strncmp(path, "/bridge_pong", 12) != 0) {
        carla_debug("CarlaEngineOsc::handleMessage(%s, \"%s\", %i, %p, \"%s\", %p)",
                    bool2str(isTCP), path, argc, argv, types, msg);
    }
#endif

    if (isTCP)
    {
        CARLA_SAFE_ASSERT_RETURN(fServerPathTCP.isNotEmpty(), 1);
        CARLA_SAFE_ASSERT_RETURN(fServerTCP != nullptr, 1);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(fServerPathUDP.isNotEmpty(), 1);
        CARLA_SAFE_ASSERT_RETURN(fServerUDP != nullptr, 1);
    }

    const lo_address source = lo_message_get_source(msg);

    // Initial path check
    if (std::strcmp(path, "/register") == 0)
        return handleMsgRegister(isTCP, argc, argv, types, source);

    if (std::strcmp(path, "/unregister") == 0)
        return handleMsgUnregister(isTCP, argc, argv, types, source);

    if (std::strncmp(path, "/ctrl/", 6) == 0)
    {
        CARLA_SAFE_ASSERT_RETURN(isTCP, 1);
        return handleMsgControl(path + 6, argc, argv, types);
    }

    // Check if message is for this client
    std::size_t bytesAfterName;
    if (fControlDataTCP.owner != nullptr && std::strcmp(lo_address_get_hostname(source), fControlDataTCP.owner) == 0)
    {
        if (const char* const slash = std::strchr(path+1, '/'))
        {
            bytesAfterName = static_cast<std::size_t>(slash - path);
        }
        else
        {
            carla_stderr("CarlaEngineOsc::handleMessage() - message '%s' is invalid", path);
            return 1;
        }
    }
    else
    {
        bytesAfterName = fName.length();

        if (std::strlen(path) <= bytesAfterName || std::strncmp(path+1, fName, bytesAfterName) != 0)
        {
            carla_stderr("CarlaEngineOsc::handleMessage() - message not for this client -> '%s' != '/%s/'",
                        path, fName.buffer());
            return 1;
        }

        ++bytesAfterName;
    }

    // Get plugin id from path, "/carla/23/method" -> 23
    uint pluginId = 0;
    std::size_t offset;

    if (! std::isdigit(path[bytesAfterName+1]))
    {
        carla_stderr("CarlaEngineOsc::handleMessage() - invalid message '%s'", path);
        return 1;
    }

    if (std::isdigit(path[bytesAfterName+2]))
    {
        if (std::isdigit(path[bytesAfterName+4]))
        {
            carla_stderr2("CarlaEngineOsc::handleMessage() - invalid plugin id, over 999? (value: \"%s\")",
                          path+bytesAfterName);
            return 1;
        }
        else if (std::isdigit(path[bytesAfterName+3]))
        {
            // 3 digits, /xyz/method
            offset    = 5;
            pluginId += uint(path[bytesAfterName+1]-'0')*100;
            pluginId += uint(path[bytesAfterName+2]-'0')*10;
            pluginId += uint(path[bytesAfterName+3]-'0');
        }
        else
        {
            // 2 digits, /xy/method
            offset    = 4;
            pluginId += uint(path[bytesAfterName+1]-'0')*10;
            pluginId += uint(path[bytesAfterName+2]-'0');
        }
    }
    else
    {
        // single digit, /x/method
        offset    = 3;
        pluginId += uint(path[bytesAfterName+1]-'0');
    }

    if (pluginId > fEngine->getCurrentPluginCount())
    {
        carla_stderr("CarlaEngineOsc::handleMessage() - failed to get plugin, wrong id '%i'", pluginId);
        return 0;
    }

    // Get plugin
    const CarlaPluginPtr plugin = fEngine->getPluginUnchecked(pluginId);

    if (plugin == nullptr || plugin->getId() != pluginId)
    {
        carla_stderr("CarlaEngineOsc::handleMessage() - invalid plugin id '%i', probably has been removed (path: '%s')", pluginId, path);
        return 0;
    }

    // Get method from path, "/Carla/i/method" -> "method"
    char method[48];
    std::strncpy(method, path + (bytesAfterName + offset), 47);
    method[47] = '\0';

    if (method[0] == '\0')
    {
        carla_stderr("CarlaEngineOsc::handleMessage(%s, \"%s\", ...) - received message without method", bool2str(isTCP), path);
        return 0;
    }

    // Internal methods
    if (std::strcmp(method, "set_option") == 0)
        return 0; //handleMsgSetOption(plugin, argc, argv, types); // TODO
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
    if (std::strcmp(method, "set_ctrl_channel") == 0)
        return 0; //handleMsgSetControlChannel(plugin, argc, argv, types); // TODO
    if (std::strcmp(method, "set_parameter_value") == 0)
        return handleMsgSetParameterValue(plugin, argc, argv, types);
    if (std::strcmp(method, "set_parameter_mapped_control_index") == 0)
        return handleMsgSetParameterMappedControlIndex(plugin, argc, argv, types);
    if (std::strcmp(method, "set_parameter_mapped_range") == 0)
        return handleMsgSetParameterMappedRange(plugin, argc, argv, types);
    if (std::strcmp(method, "set_parameter_midi_channel") == 0)
        return handleMsgSetParameterMidiChannel(plugin, argc, argv, types);
    if (std::strcmp(method, "set_program") == 0)
        return handleMsgSetProgram(plugin, argc, argv, types);
    if (std::strcmp(method, "set_midi_program") == 0)
        return handleMsgSetMidiProgram(plugin, argc, argv, types);
    if (std::strcmp(method, "set_custom_data") == 0)
        return 0; //handleMsgSetCustomData(plugin, argc, argv, types); // TODO
    if (std::strcmp(method, "set_chunk") == 0)
        return 0; //handleMsgSetChunk(plugin, argc, argv, types); // TODO
    if (std::strcmp(method, "note_on") == 0)
        return handleMsgNoteOn(plugin, argc, argv, types);
    if (std::strcmp(method, "note_off") == 0)
        return handleMsgNoteOff(plugin, argc, argv, types);

    // Send all other methods to plugins, TODO
    plugin->handleOscMessage(method, argc, argv, types, msg);
    return 0;
}

// -----------------------------------------------------------------------

int CarlaEngineOsc::handleMsgRegister(const bool isTCP,
                                      const int argc, const lo_arg* const* const argv, const char* const types,
                                      const lo_address source)
{
    carla_debug("CarlaEngineOsc::handleMsgRegister()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "s");

    const char* const url = &argv[0]->s;

    CarlaOscData& oscData(isTCP ? fControlDataTCP : fControlDataUDP);

    if (oscData.owner != nullptr)
    {
        carla_stderr("OSC backend already registered to %s", oscData.owner);

        char* const path = lo_url_get_path(url);
        const size_t pathlen = std::strlen(path);
        CARLA_SAFE_ASSERT_RETURN(pathlen < 32, 0);

        char targetPath[pathlen+12];
        std::strcpy(targetPath, path);
        std::strcat(targetPath, "/exit-error");

        lo_send_from(source, isTCP ? fServerTCP : fServerUDP, LO_TT_IMMEDIATE,
                     targetPath, "s", "OSC already registered to another client");

        free(path);
    }
    else
    {
        const char* const host  = lo_address_get_hostname(source);
        /* */ char* const port  = lo_url_get_port(url); // NOTE: lo_address_get_port is buggy against TCP
        const lo_address target = lo_address_new_with_proto(isTCP ? LO_TCP : LO_UDP, host, port);

        oscData.owner  = carla_strdup_safe(host);
        oscData.path   = carla_strdup_free(lo_url_get_path(url));
        oscData.target = target;

        char* const targeturl = lo_address_get_url(target);
        carla_stdout("OSC %s backend registered to %s, path: %s, target: %s (host: %s, port: %s)",
                     isTCP ? "TCP" : "UDP", url, oscData.path, targeturl, host, port);
        free(targeturl);
        free(port);

        if (isTCP)
        {
            const EngineOptions& opts(fEngine->getOptions());

            fEngine->callback(false, true,
                              ENGINE_CALLBACK_ENGINE_STARTED,
                              fEngine->getCurrentPluginCount(),
                              opts.processMode,
                              opts.transportMode,
                              static_cast<int>(fEngine->getBufferSize()),
                              static_cast<float>(fEngine->getSampleRate()),
                              fEngine->getCurrentDriverName());

            for (uint i=0, count=fEngine->getCurrentPluginCount(); i < count; ++i)
            {
                const CarlaPluginPtr plugin = fEngine->getPluginUnchecked(i);
                CARLA_SAFE_ASSERT_CONTINUE(plugin != nullptr);

                fEngine->callback(false, true, ENGINE_CALLBACK_PLUGIN_ADDED, i, plugin->getType(),
                                  0, 0, 0.0f,
                                  plugin->getName());
            }

            fEngine->patchbayRefresh(false, true, fEngine->pData->graph.isUsingExternalOSC());
        }
    }

    return 0;
}

int CarlaEngineOsc::handleMsgUnregister(const bool isTCP,
                                        const int argc, const lo_arg* const* const argv, const char* const types,
                                        const lo_address source)
{
    carla_debug("CarlaEngineOsc::handleMsgUnregister()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "s");

    CarlaOscData& oscData(isTCP ? fControlDataTCP : fControlDataUDP);

    if (oscData.owner == nullptr)
    {
        carla_stderr("OSC backend is not registered yet, unregister failed");
        return 0;
    }

    const char* const url  = &argv[0]->s;
    const char* const host = lo_address_get_hostname(source);
    const char* const path = lo_url_get_path(url);

    if (std::strcmp(oscData.owner, host) != 0)
    {
        carla_stderr("OSC backend unregister failed, current owner host %s does not match requested %s",
                     oscData.owner, host);
        return 0;
    }

    if (std::strcmp(oscData.path, path) != 0)
    {
        carla_stderr("OSC backend unregister failed, current owner path %s does not match requested %s",
                     oscData.path, path);
        return 0;
    }

    carla_stdout("OSC client %s unregistered", url);
    oscData.clear();
    return 0;
}

int CarlaEngineOsc::handleMsgControl(const char* const method,
                                     const int argc, const lo_arg* const* const argv, const char* const types)
{
    carla_debug("CarlaEngineOsc::handleMsgControl()");
    CARLA_SAFE_ASSERT_RETURN(method != nullptr && method[0] != '\0', 0);
    CARLA_SAFE_ASSERT_RETURN(types != nullptr, 0);
    CARLA_SAFE_ASSERT_RETURN(types[0] == 'i', 0);

    if (fControlDataTCP.owner == nullptr)
    {
        carla_stderr("OSC backend is not registered yet, control failed");
        return 0;
    }

    const int32_t messageId = argv[0]->i;
    bool ok;

#define CARLA_SAFE_ASSERT_RETURN_OSC_ERR(cond) \
    if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); sendResponse(messageId, #cond); return 0; }

    /**/ if (std::strcmp(method, "clear_engine_xruns") == 0)
    {
        ok = true;
        fEngine->clearXruns();
    }
    else if (std::strcmp(method, "cancel_engine_action") == 0)
    {
        ok = true;
        fEngine->setActionCanceled(true);
    }
    else if (std::strcmp(method, "patchbay_connect") == 0)
    {
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(argc == 6);
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[1] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[2] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[3] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[4] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[5] == 'i');

        const bool external = argv[1]->i != 0;

        const int32_t groupA = argv[2]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(groupA >= 0);

        const int32_t portA = argv[3]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(portA >= 0);

        const int32_t groupB = argv[4]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(groupB >= 0);

        const int32_t portB = argv[5]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(portB >= 0);

        ok = fEngine->patchbayConnect(external,
                                      static_cast<uint32_t>(groupA),
                                      static_cast<uint32_t>(portA),
                                      static_cast<uint32_t>(groupB),
                                      static_cast<uint32_t>(portB));
    }
    else if (std::strcmp(method, "patchbay_disconnect") == 0)
    {
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(argc == 3);
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[1] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[2] == 'i');

        const bool external = argv[1]->i != 0;

        const int32_t connectionId = argv[2]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(connectionId >= 0);

        ok = fEngine->patchbayDisconnect(external, static_cast<uint32_t>(connectionId));
    }
    else if (std::strcmp(method, "patchbay_set_group_pos") == 0)
    {
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(argc == 7);
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[1] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[2] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[3] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[4] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[5] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[6] == 'i');

        const bool external = argv[1]->i != 0;

        const int32_t groupId = argv[2]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(groupId >= 0);

        ok = fEngine->patchbaySetGroupPos(true, false,
                                          external, static_cast<uint32_t>(groupId),
                                          argv[3]->i, argv[4]->i, argv[5]->i, argv[6]->i);
    }
    else if (std::strcmp(method, "patchbay_refresh") == 0)
    {
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(argc == 2);
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[1] == 'i');

        const bool external = argv[1]->i != 0;

        ok = fEngine->patchbayRefresh(false, true, external);
    }
    else if (std::strcmp(method, "transport_play") == 0)
    {
        ok = true;
        fEngine->transportPlay();
    }
    else if (std::strcmp(method, "transport_pause") == 0)
    {
        ok = true;
        fEngine->transportPause();
    }
    else if (std::strcmp(method, "transport_bpm") == 0)
    {
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(argc == 2);
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[1] == 'f');

        const double bpm = argv[1]->f;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(bpm >= 0.0);

        ok = true;
        fEngine->transportBPM(bpm);
    }
    else if (std::strcmp(method, "transport_relocate") == 0)
    {
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(argc == 2);

        uint64_t frame;

        /**/ if (types[1] == 'i')
        {
            const int32_t i = argv[1]->i;
            CARLA_SAFE_ASSERT_RETURN_OSC_ERR(i >= 0);
            frame = static_cast<uint64_t>(i);
        }
        else if (types[1] == 'h')
        {
            const int64_t h = argv[1]->h;
            CARLA_SAFE_ASSERT_RETURN_OSC_ERR(h >= 0);
            frame = static_cast<uint64_t>(h);
        }
        else
        {
            carla_stderr2("Wrong OSC type used for '%s'", method);
            sendResponse(messageId, "Wrong OSC type");
            return 0;
        }

        ok = true;
        fEngine->transportRelocate(frame);
    }
    else if (std::strcmp(method, "add_plugin") == 0)
    {
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(argc == 8);
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[1] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[2] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[3] == 's');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[4] == 's');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[5] == 's');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[7] == 'i');

        int32_t btype = argv[1]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(btype >= 0);

        const int32_t ptype = argv[2]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(ptype >= 0);

        // Force binary type to be native in some cases
        switch (ptype)
        {
        case PLUGIN_INTERNAL:
        case PLUGIN_LV2:
        case PLUGIN_SF2:
        case PLUGIN_SFZ:
        case PLUGIN_JACK:
            btype = BINARY_NATIVE;
            break;
        }

        const char* filename = &argv[3]->s;

        if (filename != nullptr && std::strcmp(filename, "(null)") == 0)
            filename = nullptr;

        const char* name = &argv[4]->s;

        if (name != nullptr && std::strcmp(name, "(null)") == 0)
            name = nullptr;

        const char* const label = &argv[5]->s;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(label != nullptr && label[0] != '\0');

        int64_t uniqueId;

        /**/ if (types[6] == 'i')
        {
            uniqueId = argv[6]->i;
        }
        else if (types[6] == 'h')
        {
            uniqueId = argv[6]->h;
        }
        else
        {
            carla_stderr2("Wrong OSC type used for '%s' uniqueId", method);
            sendResponse(messageId, "Wrong OSC type");
            return 0;
        }

        const int32_t options = argv[7]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(options >= 0);

        ok = fEngine->addPlugin(static_cast<BinaryType>(btype),
                                static_cast<PluginType>(ptype),
                                filename, name, label, uniqueId, nullptr, static_cast<uint32_t>(options));
    }
    else if (std::strcmp(method, "remove_plugin") == 0)
    {
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(argc == 2);
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[1] == 'i');

        const int32_t id = argv[1]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(id >= 0);

        ok = fEngine->removePlugin(static_cast<uint32_t>(id));
    }
    else if (std::strcmp(method, "remove_all_plugins") == 0)
    {
        ok = fEngine->removeAllPlugins();
    }
    else if (std::strcmp(method, "rename_plugin") == 0)
    {
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(argc == 3);
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[1] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[2] == 's');

        const int32_t id = argv[1]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(id >= 0);

        const char* const newName = &argv[2]->s;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(newName != nullptr && newName[0] != '\0');

        ok = fEngine->renamePlugin(static_cast<uint32_t>(id), newName);
    }
    else if (std::strcmp(method, "clone_plugin") == 0)
    {
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(argc == 2);
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[1] == 'i');

        const int32_t id = argv[1]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(id >= 0);

        ok = fEngine->clonePlugin(static_cast<uint32_t>(id));
    }
    else if (std::strcmp(method, "replace_plugin") == 0)
    {
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(argc == 2);
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[1] == 'i');

        const int32_t id = argv[1]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(id >= 0);

        ok = fEngine->replacePlugin(static_cast<uint32_t>(id));
    }
    else if (std::strcmp(method, "switch_plugins") == 0)
    {
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(argc == 3);
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[1] == 'i');
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(types[2] == 'i');

        const int32_t idA = argv[1]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(idA >= 0);

        const int32_t idB = argv[2]->i;
        CARLA_SAFE_ASSERT_RETURN_OSC_ERR(idB >= 0);

        ok = fEngine->switchPlugins(static_cast<uint32_t>(idA), static_cast<uint32_t>(idB));
    }
    else
    {
        carla_stderr2("Unhandled OSC control for '%s'", method);
        sendResponse(messageId, "Unhandled OSC control method");
        return 0;
    }

#undef CARLA_SAFE_ASSERT_RETURN_OSC_ERR

    sendResponse(messageId, ok ? "" : fEngine->getLastError());
    return 0;
}

// -----------------------------------------------------------------------

int CarlaEngineOsc::handleMsgSetActive(CARLA_ENGINE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaEngineOsc::handleMsgSetActive()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

    const bool active = (argv[0]->i != 0);

    plugin->setActive(active, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetDryWet(CARLA_ENGINE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaEngineOsc::handleMsgSetDryWet()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;

    plugin->setDryWet(value, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetVolume(CARLA_ENGINE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaEngineOsc::handleMsgSetVolume()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;

    plugin->setVolume(value, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetBalanceLeft(CARLA_ENGINE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaEngineOsc::handleMsgSetBalanceLeft()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;

    plugin->setBalanceLeft(value, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetBalanceRight(CARLA_ENGINE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaEngineOsc::handleMsgSetBalanceRight()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;

    plugin->setBalanceRight(value, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetPanning(CARLA_ENGINE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaEngineOsc::handleMsgSetPanning()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;

    plugin->setPanning(value, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetParameterValue(CARLA_ENGINE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaEngineOsc::handleMsgSetParameterValue()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "if");

    const int32_t index = argv[0]->i;
    const float   value = argv[1]->f;

    CARLA_SAFE_ASSERT_RETURN(index >= 0, 0);

    plugin->setParameterValue(static_cast<uint32_t>(index), value, true, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetParameterMappedControlIndex(CARLA_ENGINE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaEngineOsc::handleMsgSetParameterMappedIndex()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t index = argv[0]->i;
    const int32_t ctrl  = argv[1]->i;

    CARLA_SAFE_ASSERT_RETURN(index >= 0, 0);
    CARLA_SAFE_ASSERT_RETURN(ctrl >= CONTROL_INDEX_NONE && ctrl <= CONTROL_INDEX_MAX_ALLOWED, 0);

    plugin->setParameterMappedControlIndex(static_cast<uint32_t>(index), static_cast<int16_t>(ctrl), false, true, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetParameterMappedRange(CARLA_ENGINE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaEngineOsc::handleMsgSetParameterMappedRange()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(2, "iff");

    const int32_t index   = argv[0]->i;
    const float   minimum = argv[1]->f;
    const float   maximum = argv[2]->f;

    CARLA_SAFE_ASSERT_RETURN(index >= 0, 0);

    plugin->setParameterMappedRange(static_cast<uint32_t>(index), minimum, maximum, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetParameterMidiChannel(CARLA_ENGINE_OSC_HANDLE_ARGS)
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

int CarlaEngineOsc::handleMsgSetProgram(CARLA_ENGINE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaEngineOsc::handleMsgSetProgram()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

    const int32_t index = argv[0]->i;

    CARLA_SAFE_ASSERT_RETURN(index >= -1, 0);

    plugin->setProgram(index, true, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgSetMidiProgram(CARLA_ENGINE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaEngineOsc::handleMsgSetMidiProgram()");
    CARLA_ENGINE_OSC_CHECK_OSC_TYPES(1, "i");

    const int32_t index = argv[0]->i;

    CARLA_SAFE_ASSERT_RETURN(index >= -1, 0);

    plugin->setMidiProgram(index, true, false, true);
    return 0;
}

int CarlaEngineOsc::handleMsgNoteOn(CARLA_ENGINE_OSC_HANDLE_ARGS)
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

int CarlaEngineOsc::handleMsgNoteOff(CARLA_ENGINE_OSC_HANDLE_ARGS)
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

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // HAVE_LIBLO
