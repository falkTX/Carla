/*
 * Carla Bridge Client
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

#include "carla_bridge_client.hpp"
#include "carla_bridge_toolkit.hpp"

#ifdef BUILD_BRIDGE_UI
# include "carla_lib_utils.hpp"
#endif

#include <cstdlib>
#include <cstring>

CARLA_BRIDGE_START_NAMESPACE

// ---------------------------------------------------------------------

CarlaBridgeClient::CarlaBridgeClient(const char* const uiTitle)
    : m_osc(this),
      m_toolkit(CarlaBridgeToolkit::createNew(this, uiTitle))
{
    qDebug("CarlaBridgeClient::CarlaBridgeClient(\"%s\")", uiTitle);

    m_oscData = nullptr;

#ifdef BUILD_BRIDGE_UI
    m_uiFilename = nullptr;
    m_uiLib = nullptr;
    m_uiQuit = false;
#endif
}

CarlaBridgeClient::~CarlaBridgeClient()
{
    qDebug("CarlaBridgeClient::~CarlaBridgeClient()");

#ifdef BUILD_BRIDGE_UI
    if (m_uiFilename)
        free(m_uiFilename);
#endif

    delete m_toolkit;
}

#ifdef BUILD_BRIDGE_UI
// ---------------------------------------------------------------------
// ui initialization

bool CarlaBridgeClient::init(const char* const, const char* const)
{
    qDebug("CarlaBridgeClient::init()");

    // Test for single init
    {
        static bool initiated = false;
        CARLA_ASSERT(! initiated);
        initiated = true;
    }

    m_uiQuit = false;

    m_toolkit->init();

    return false;
}

void CarlaBridgeClient::close()
{
    qDebug("CarlaBridgeClient::close()");

    if (! m_uiQuit)
    {
        m_uiQuit = true;

        if (isOscControlRegistered())
            sendOscExiting();
    }

    m_toolkit->quit();
}
#endif

// ---------------------------------------------------------------------
// osc stuff

bool CarlaBridgeClient::oscInit(const char* const url)
{
    qDebug("CarlaBridgeClient::oscInit(\"%s\")", url);

    const bool ret = m_osc.init(url);
    m_oscData = m_osc.getControlData();

    return ret;
}

bool CarlaBridgeClient::oscIdle()
{
    m_osc.idle();

#ifdef BUILD_BRIDGE_UI
    return ! m_uiQuit;
#else
    return true;
#endif
}

void CarlaBridgeClient::oscClose()
{
    qDebug("CarlaBridgeClient::oscClose()");
    CARLA_ASSERT(m_oscData);

    m_osc.close();
    m_oscData = nullptr;
}

bool CarlaBridgeClient::isOscControlRegistered() const
{
    return m_osc.isControlRegistered();
}

void CarlaBridgeClient::sendOscUpdate()
{
    qDebug("CarlaBridgeClient::sendOscUpdate()");
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
        osc_send_update(m_oscData, m_osc.getServerPath());
}

#ifdef BUILD_BRIDGE_PLUGIN
void CarlaBridgeClient::sendOscBridgeUpdate()
{
    qDebug("CarlaBridgeClient::sendOscBridgeUpdate()");
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(m_oscData->target && m_oscData->path);

    if (m_oscData && m_oscData->target && m_oscData->path)
        osc_send_bridge_update(m_oscData, m_oscData->path);
}

void CarlaBridgeClient::sendOscBridgeError(const char* const error)
{
    qDebug("CarlaBridgeClient::sendOscBridgeError(\"%s\")", error);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(error);

    if (m_oscData && m_oscData->target)
        osc_send_bridge_error(m_oscData, error);
}
#endif

// ---------------------------------------------------------------------
// toolkit

void CarlaBridgeClient::toolkitShow()
{
    qDebug("CarlaBridgeClient::toolkitShow()");

    m_toolkit->show();
}

void CarlaBridgeClient::toolkitHide()
{
    qDebug("CarlaBridgeClient::toolkitHide()");

    m_toolkit->hide();
}

void CarlaBridgeClient::toolkitResize(const int width, const int height)
{
    qDebug("CarlaBridgeClient::toolkitResize(%i, %i)", width, height);

    m_toolkit->resize(width, height);
}

void CarlaBridgeClient::toolkitExec(const bool showGui)
{
    qDebug("CarlaBridgeClient::toolkitExec(%s)", bool2str(showGui));

    m_toolkit->exec(showGui);
}

void CarlaBridgeClient::toolkitQuit()
{
    qDebug("CarlaBridgeClient::toolkitQuit()");

#ifdef BUILD_BRIDGE_UI
    m_uiQuit = true;
#endif
    m_toolkit->quit();
}

// ---------------------------------------------------------------------

void CarlaBridgeClient::sendOscConfigure(const char* const key, const char* const value)
{
    qDebug("CarlaBridgeClient::sendOscConfigure(\"%s\", \"%s\")", key, value);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
        osc_send_configure(m_oscData, key, value);
}

void CarlaBridgeClient::sendOscControl(const int32_t index, const float value)
{
    qDebug("CarlaBridgeClient::sendOscControl(%i, %f)", index, value);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
        osc_send_control(m_oscData, index, value);
}

void CarlaBridgeClient::sendOscProgram(const int32_t index)
{
    qDebug("CarlaBridgeClient::sendOscProgram(%i)", index);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
        osc_send_program(m_oscData, index);
}

void CarlaBridgeClient::sendOscMidiProgram(const int32_t index)
{
    qDebug("CarlaBridgeClient::sendOscMidiProgram(%i)", index);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
        osc_send_midi_program(m_oscData, index);
}

void CarlaBridgeClient::sendOscMidi(const uint8_t midiBuf[4])
{
    qDebug("CarlaBridgeClient::sendOscMidi(%p)", midiBuf);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
        osc_send_midi(m_oscData, midiBuf);
}

void CarlaBridgeClient::sendOscExiting()
{
    qDebug("CarlaBridgeClient::sendOscExiting()");
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
        osc_send_exiting(m_oscData);
}

#ifdef BRIDGE_LV2
void CarlaBridgeClient::sendOscLv2TransferAtom(const int32_t portIndex, const char* const typeStr, const char* const atomBuf)
{
    qDebug("CarlaBridgeClient::sendOscLv2TransferAtom(%i, \"%s\", \"%s\")", portIndex, typeStr, atomBuf);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
        osc_send_lv2_transfer_atom(m_oscData, portIndex, typeStr, atomBuf);
}

void CarlaBridgeClient::sendOscLv2TransferEvent(const int32_t portIndex, const char* const typeStr, const char* const atomBuf)
{
    qDebug("CarlaBridgeClient::sendOscLv2TransferEvent(%i, \"%s\", \"%s\")", portIndex, typeStr, atomBuf);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
        osc_send_lv2_transfer_event(m_oscData, portIndex, typeStr, atomBuf);
}
#endif

// ---------------------------------------------------------------------

void* CarlaBridgeClient::getContainerId()
{
    return m_toolkit->getContainerId();
}

#ifdef BUILD_BRIDGE_UI
bool CarlaBridgeClient::uiLibOpen(const char* const filename)
{
    CARLA_ASSERT(! m_uiLib);
    CARLA_ASSERT(filename);

    if (m_uiFilename)
        free(m_uiFilename);

    m_uiLib = lib_open(filename);
    m_uiFilename = strdup(filename ? filename : "");

    return bool(m_uiLib);
}

bool CarlaBridgeClient::uiLibClose()
{
    CARLA_ASSERT(m_uiLib);

    if (m_uiLib)
    {
        const bool closed = lib_close(m_uiLib);
        m_uiLib = nullptr;
        return closed;
    }

    return false;
}

void* CarlaBridgeClient::uiLibSymbol(const char* const symbol)
{
    CARLA_ASSERT(m_uiLib);

    if (m_uiLib)
        return lib_symbol(m_uiLib, symbol);

    return nullptr;
}

const char* CarlaBridgeClient::uiLibError()
{
    return lib_error(m_uiFilename);
}
#endif

CARLA_BRIDGE_END_NAMESPACE
