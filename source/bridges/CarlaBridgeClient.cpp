/*
 * Carla Bridge Client
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaBridgeClient.hpp"

#ifdef BUILD_BRIDGE_UI
# include "CarlaBridgeToolkit.hpp"
# include "CarlaLibUtils.hpp"
#endif

CARLA_BRIDGE_START_NAMESPACE

// ---------------------------------------------------------------------

CarlaBridgeClient::CarlaBridgeClient(const char* const uiTitle)
    : kOsc(this),
#ifdef BUILD_BRIDGE_UI
      kUiToolkit(CarlaBridgeToolkit::createNew(this, uiTitle)),
      fUiFilename(nullptr),
      fUiLib(nullptr),
      fUiQuit(false),
#endif
      fOscData(nullptr)
{
    carla_debug("CarlaBridgeClient::CarlaBridgeClient(\"%s\")", uiTitle);
}

CarlaBridgeClient::~CarlaBridgeClient()
{
    carla_debug("CarlaBridgeClient::~CarlaBridgeClient()");

#ifdef BUILD_BRIDGE_UI
    if (fUiFilename != nullptr)
        delete[] fUiFilename;

    delete kUiToolkit;
#endif
}

#ifdef BUILD_BRIDGE_UI
// ---------------------------------------------------------------------
// ui initialization

bool CarlaBridgeClient::uiInit(const char* const, const char* const)
{
    carla_debug("CarlaBridgeClient::uiInit()");

    // Test for single init
    {
        static bool initiated = false;
        CARLA_ASSERT(! initiated);
        initiated = true;
    }

    fUiQuit = false;
    kUiToolkit->init();

    return false;
}

void CarlaBridgeClient::uiClose()
{
    carla_debug("CarlaBridgeClient::uiClose()");

    if (! fUiQuit)
    {
        fUiQuit = true;

        if (isOscControlRegistered())
            sendOscExiting();
    }

    kUiToolkit->quit();
}
#endif

// ---------------------------------------------------------------------
// osc stuff

void CarlaBridgeClient::oscInit(const char* const url)
{
    carla_debug("CarlaBridgeClient::oscInit(\"%s\")", url);

    kOsc.init(url);
    fOscData = kOsc.getControlData();
}

bool CarlaBridgeClient::oscIdle()
{
    kOsc.idle();

#ifdef BUILD_BRIDGE_UI
    return ! fUiQuit;
#else
    return true;
#endif
}

void CarlaBridgeClient::oscClose()
{
    carla_debug("CarlaBridgeClient::oscClose()");
    CARLA_ASSERT(fOscData != nullptr);

    kOsc.close();
    fOscData = nullptr;
}

bool CarlaBridgeClient::isOscControlRegistered() const
{
    return kOsc.isControlRegistered();
}

void CarlaBridgeClient::sendOscUpdate()
{
    carla_debug("CarlaBridgeClient::sendOscUpdate()");
    CARLA_ASSERT(fOscData != nullptr);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_update(fOscData, kOsc.getServerPath());
}

#ifdef BUILD_BRIDGE_PLUGIN
void CarlaBridgeClient::sendOscBridgeUpdate()
{
    carla_debug("CarlaBridgeClient::sendOscBridgeUpdate()");
    CARLA_ASSERT(fOscData != nullptr);
    CARLA_ASSERT(fOscData->target != nullptr && fOscData->path != nullptr);

    if (fOscData != nullptr && fOscData->target != nullptr && fOscData->path != nullptr)
        osc_send_bridge_update(fOscData, fOscData->path);
}

void CarlaBridgeClient::sendOscBridgeError(const char* const error)
{
    carla_debug("CarlaBridgeClient::sendOscBridgeError(\"%s\")", error);
    CARLA_ASSERT(fOscData != nullptr);
    CARLA_ASSERT(error != nullptr);

    if (fOscData != nullptr && fOscData->target != nullptr && error != nullptr)
        osc_send_bridge_error(fOscData, error);
}
#endif

#ifdef BUILD_BRIDGE_UI
// ---------------------------------------------------------------------
// toolkit

void CarlaBridgeClient::toolkitShow()
{
    carla_debug("CarlaBridgeClient::toolkitShow()");

    kUiToolkit->show();
}

void CarlaBridgeClient::toolkitHide()
{
    carla_debug("CarlaBridgeClient::toolkitHide()");

    kUiToolkit->hide();
}

void CarlaBridgeClient::toolkitResize(const int width, const int height)
{
    carla_debug("CarlaBridgeClient::toolkitResize(%i, %i)", width, height);

    kUiToolkit->resize(width, height);
}

void CarlaBridgeClient::toolkitExec(const bool showGui)
{
    carla_debug("CarlaBridgeClient::toolkitExec(%s)", bool2str(showGui));

    kUiToolkit->exec(showGui);
}

void CarlaBridgeClient::toolkitQuit()
{
    carla_debug("CarlaBridgeClient::toolkitQuit()");

    fUiQuit = true;
    kUiToolkit->quit();
}
#endif

// ---------------------------------------------------------------------

void CarlaBridgeClient::sendOscConfigure(const char* const key, const char* const value)
{
    carla_debug("CarlaBridgeClient::sendOscConfigure(\"%s\", \"%s\")", key, value);
    CARLA_ASSERT(fOscData != nullptr);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_configure(fOscData, key, value);
}

void CarlaBridgeClient::sendOscControl(const int32_t index, const float value)
{
    carla_debug("CarlaBridgeClient::sendOscControl(%i, %f)", index, value);
    CARLA_ASSERT(fOscData != nullptr);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_control(fOscData, index, value);
}

void CarlaBridgeClient::sendOscProgram(const int32_t index)
{
    carla_debug("CarlaBridgeClient::sendOscProgram(%i)", index);
    CARLA_ASSERT(fOscData != nullptr);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_program(fOscData, index);
}

#ifdef BUILD_BRIDGE_PLUGIN
void CarlaBridgeClient::sendOscMidiProgram(const int32_t index)
{
    carla_debug("CarlaBridgeClient::sendOscMidiProgram(%i)", index);
    CARLA_ASSERT(fOscData != nullptr);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_midi_program(fOscData, index);
}
#endif

void CarlaBridgeClient::sendOscMidi(const uint8_t midiBuf[4])
{
    carla_debug("CarlaBridgeClient::sendOscMidi(%p)", midiBuf);
    CARLA_ASSERT(fOscData != nullptr);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_midi(fOscData, midiBuf);
}

void CarlaBridgeClient::sendOscExiting()
{
    carla_debug("CarlaBridgeClient::sendOscExiting()");
    CARLA_ASSERT(fOscData != nullptr);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_exiting(fOscData);
}

#ifdef BRIDGE_LV2
void CarlaBridgeClient::sendOscLv2TransferAtom(const int32_t portIndex, const char* const typeStr, const char* const atomBuf)
{
    carla_debug("CarlaBridgeClient::sendOscLv2TransferAtom(%i, \"%s\", \"%s\")", portIndex, typeStr, atomBuf);
    CARLA_ASSERT(fOscData != nullptr);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_lv2_transfer_atom(fOscData, portIndex, typeStr, atomBuf);
}

void CarlaBridgeClient::sendOscLv2TransferEvent(const int32_t portIndex, const char* const typeStr, const char* const atomBuf)
{
    carla_debug("CarlaBridgeClient::sendOscLv2TransferEvent(%i, \"%s\", \"%s\")", portIndex, typeStr, atomBuf);
    CARLA_ASSERT(fOscData != nullptr);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_lv2_transfer_event(fOscData, portIndex, typeStr, atomBuf);
}
#endif

// ---------------------------------------------------------------------

#ifdef BUILD_BRIDGE_UI
void* CarlaBridgeClient::getContainerId()
{
    return kUiToolkit->getContainerId();
}

bool CarlaBridgeClient::uiLibOpen(const char* const filename)
{
    CARLA_ASSERT(fUiLib == nullptr);
    CARLA_ASSERT(fUiFilename != nullptr);

    if (fUiFilename != nullptr)
        delete[] fUiFilename;

    fUiLib = lib_open(filename);
    fUiFilename = carla_strdup(filename ? filename : "");

    return (fUiLib != nullptr);
}

bool CarlaBridgeClient::uiLibClose()
{
    CARLA_ASSERT(fUiLib != nullptr);

    if (fUiLib != nullptr)
    {
        const bool closed = lib_close(fUiLib);
        fUiLib = nullptr;
        return closed;
    }

    return false;
}

void* CarlaBridgeClient::uiLibSymbol(const char* const symbol)
{
    CARLA_ASSERT(fUiLib != nullptr);

    if (fUiLib != nullptr)
        return lib_symbol(fUiLib, symbol);

    return nullptr;
}

const char* CarlaBridgeClient::uiLibError()
{
    return lib_error(fUiFilename);
}
#endif

CARLA_BRIDGE_END_NAMESPACE
