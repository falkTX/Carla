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

#include "carla_bridge_osc.hpp"
#include "carla_bridge_client.hpp"
#include "carla_midi.h"
#include "carla_utils.hpp"

#include <QtCore/QString>
#include <QtCore/QStringList>

CARLA_BRIDGE_START_NAMESPACE

// -----------------------------------------------------------------------

CarlaBridgeOsc::CarlaBridgeOsc(CarlaBridgeClient* const client_)
    : client(client_)
{
    qDebug("CarlaBridgeOsc::CarlaBridgeOsc(%p)", client);
    CARLA_ASSERT(client);

    m_name = nullptr;
    m_nameSize = 0;

    m_server = nullptr;
    m_serverPath = nullptr;
}

CarlaBridgeOsc::~CarlaBridgeOsc()
{
    qDebug("CarlaBridgeOsc::~CarlaBridgeOsc()");
    CARLA_ASSERT(! m_name);
    CARLA_ASSERT(! m_server);
    CARLA_ASSERT(! m_serverPath);
}

bool CarlaBridgeOsc::init(const char* const url)
{
    qDebug("CarlaBridgeOsc::init(\"%s\")", url);
    CARLA_ASSERT(! m_name);
    CARLA_ASSERT(! m_server);
    CARLA_ASSERT(! m_serverPath);
    CARLA_ASSERT(m_nameSize == 0);
    CARLA_ASSERT(url);

    if (! url)
    {
        qWarning("CarlaBridgeOsc::init(\"%s\") - invalid url", url);
        return false;
    }

#ifdef BUILD_BRIDGE_PLUGIN
    m_name = strdup("carla-bridge-plugin");
#else
    m_name = strdup("carla-bridge-ui");
#endif
    m_nameSize = strlen(m_name);

    char* const host = lo_url_get_hostname(url);
    char* const path = lo_url_get_path(url);
    char* const port = lo_url_get_port(url);

    if (! host)
    {
        qCritical("CarlaBridgeOsc::init(\"%s\") - failed to get url hostname", url);
        return false;
    }

    if (! path)
    {
        free(host);
        qCritical("CarlaBridgeOsc::init(\"%s\") - failed to get url path", url);
        return false;
    }

    if (! port)
    {
        free(host);
        free(path);
        qCritical("CarlaBridgeOsc::init(\"%s\") - failed to get url port", url);
        return false;
    }

    m_controlData.path   = path;
    m_controlData.target = lo_address_new_with_proto(LO_TCP, host, port);

    free(host);
    free(port);

    if (! m_controlData.target)
    {
        qCritical("CarlaBridgeOsc::init(\"%s\") - failed to get new url address for host '%s' and port '%s'", url, host, port);
        return false;
    }

    m_server = lo_server_new_with_proto(nullptr, LO_TCP, osc_error_handler);

    if (! m_server)
    {
        qCritical("CarlaBridgeOsc::init(\"%s\") - failed to create new OSC server", url);
        return false;
    }

    if (char* const serverUrl = lo_server_get_url(m_server))
    {
        m_serverPath = strdup(QString("%1%2").arg(serverUrl).arg(m_name).toUtf8().constData());
        free(serverUrl);
    }
    else
        m_serverPath = strdup(QString("%1carla-bridge").arg(serverUrl).toUtf8().constData());

    lo_server_add_method(m_server, nullptr, nullptr, osc_message_handler, this);

    return true;
}

void CarlaBridgeOsc::idle()
{
    CARLA_ASSERT(m_server);

    if (m_server)
    {
        while (lo_server_recv_noblock(m_server, 0) != 0) {}
    }
}

void CarlaBridgeOsc::close()
{
    qDebug("CarlaBridgeOsc::close()");
    CARLA_ASSERT(m_name);
    CARLA_ASSERT(m_server);
    CARLA_ASSERT(m_serverPath);

    m_nameSize = 0;

    if (m_name)
    {
        free(m_name);
        m_name = nullptr;
    }

    if (m_server)
    {
        lo_server_del_method(m_server, nullptr, nullptr);
        lo_server_free(m_server);
        m_server = nullptr;
    }

    if (m_serverPath)
    {
        free(m_serverPath);
        m_serverPath = nullptr;
    }

    m_controlData.free();
}

// -----------------------------------------------------------------------

int CarlaBridgeOsc::handleMessage(const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg)
{
    qDebug("CarlaBridgeOsc::handleMessage(\"%s\", %i, %p, \"%s\", %p)", path, argc, argv, types, msg);
    CARLA_ASSERT(m_name);
    CARLA_ASSERT(m_server);
    CARLA_ASSERT(m_serverPath);
    CARLA_ASSERT(path);

    if (! path)
    {
        qCritical("CarlaBridgeOsc::handleMessage() - got invalid path");
        return 1;
    }

    if (! (m_name && m_server && m_serverPath))
    {
        qCritical("CarlaBridgeOsc::handleMessage(\"%s\", ...) - received message but client is offline", path);
        return 1;
    }

    if (strlen(path) <= m_nameSize || strncmp(path+1, m_name, m_nameSize) != 0)
    {
        qWarning("CarlaBridgeOsc::handleMessage() - message not for this client: '%s' != '/%s/'", path, m_name);
        return 1;
    }

    char method[32] = { 0 };
    strncpy(method, path + (m_nameSize + 2), 31);

    if (method[0] == '\0')
    {
        qWarning("CarlaBridgeOsc::handleMessage(\"%s\", ...) - received message without method", path);
        return 1;
    }

    // Common OSC methods
    if (strcmp(method, "configure") == 0)
        return handleMsgConfigure(argc, argv, types);
    if (strcmp(method, "control") == 0)
        return handleMsgControl(argc, argv, types);
    if (strcmp(method, "program") == 0)
        return handleMsgProgram(argc, argv, types);
    if (strcmp(method, "midi_program") == 0)
        return handleMsgMidiProgram(argc, argv, types);
    if (strcmp(method, "midi") == 0)
        return handleMsgMidi(argc, argv, types);
    if (strcmp(method, "sample-rate") == 0)
        return 0; // unused
    if (strcmp(method, "show") == 0)
        return handleMsgShow();
    if (strcmp(method, "hide") == 0)
        return handleMsgHide();
    if (strcmp(method, "quit") == 0)
        return handleMsgQuit();

#ifdef BRIDGE_LV2
    // LV2 UI methods
    if (strcmp(method, "lv2_atom_transfer") == 0)
        return handleMsgLv2TransferAtom(argc, argv, types);
    if (strcmp(method, "lv2_event_transfer") == 0)
        return handleMsgLv2TransferEvent(argc, argv, types);
#endif

#ifdef BUILD_BRIDGE_PLUGIN
    // Plugin methods
    if (strcmp(method, "plugin_save_now") == 0)
        return handleMsgPluginSaveNow();
    if (strcmp(method, "plugin_set_chunk") == 0)
        return handleMsgPluginSetChunk(argc, argv, types);
    if (strcmp(method, "plugin_set_custom_data") == 0)
        return handleMsgPluginSetCustomData(argc, argv, types);
#endif

#if 0
    // TODO
    else if (strcmp(method, "set_parameter_midi_channel") == 0)
        return osc_set_parameter_midi_channel_handler(argv);
    else if (strcmp(method, "set_parameter_midi_cc") == 0)
        return osc_set_parameter_midi_channel_handler(argv);
#endif

    qWarning("CarlaBridgeOsc::handleMessage(\"%s\", ...) - received unsupported OSC method '%s'", path, method);
    return 1;
}

int CarlaBridgeOsc::handleMsgConfigure(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgConfigure()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "ss");

    if (! client)
        return 1;

    // nothing here for now

    return 0;

    Q_UNUSED(argv);
}

int CarlaBridgeOsc::handleMsgControl(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgControl()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "if");

    if (! client)
        return 1;

    const int32_t index = argv[0]->i;
    const float   value = argv[1]->f;

    client->setParameter(index, value);

    return 0;
}

int CarlaBridgeOsc::handleMsgProgram(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgProgram()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(1, "i");

    if (! client)
        return 1;

    const int32_t index = argv[0]->i;

    client->setProgram(index);

    return 0;
}

#ifdef BUILD_BRIDGE_PLUGIN
int CarlaBridgeOsc::handleMsgMidiProgram(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgMidiProgram()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(1, "i");

    if (! client)
        return 1;

    const int32_t index = argv[0]->i;

    client->setMidiProgram(index);

    return 0;
}
#else
int CarlaBridgeOsc::handleMsgMidiProgram(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgMidiProgram()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "ii");

    if (! client)
        return 1;

    const int32_t bank    = argv[0]->i;
    const int32_t program = argv[1]->i;

    client->setMidiProgram(bank, program);

    return 0;
}
#endif

int CarlaBridgeOsc::handleMsgMidi(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgMidi()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(1, "m");

    if (! client)
        return 1;

    const uint8_t* const mdata = argv[0]->m;
    const uint8_t      data[4] = { mdata[0], mdata[1], mdata[2], mdata[3] };

    uint8_t status  = data[1];
    uint8_t channel = status & 0x0F;

    // Fix bad note-off
    if (MIDI_IS_STATUS_NOTE_ON(status) && data[3] == 0)
        status -= 0x10;

    if (MIDI_IS_STATUS_NOTE_OFF(status))
    {
        const uint8_t note = data[2];

        client->noteOff(channel, note);
    }
    else if (MIDI_IS_STATUS_NOTE_ON(status))
    {
        const uint8_t note = data[2];
        const uint8_t velo = data[3];

        client->noteOn(channel, note, velo);
    }

    return 0;
}

int CarlaBridgeOsc::handleMsgShow()
{
    qDebug("CarlaBridgeOsc::handleMsgShow()");

    if (! client)
        return 1;

    client->toolkitShow();

    return 0;
}

int CarlaBridgeOsc::handleMsgHide()
{
    qDebug("CarlaBridgeOsc::handleMsgHide()");

    if (! client)
        return 1;

    client->toolkitHide();

    return 0;
}

int CarlaBridgeOsc::handleMsgQuit()
{
    qDebug("CarlaBridgeOsc::handleMsgQuit()");

    if (! client)
        return 1;

    client->toolkitQuit();

    return 0;
}

CARLA_BRIDGE_END_NAMESPACE
