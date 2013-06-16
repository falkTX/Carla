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
# include "CarlaLibUtils.hpp"
#endif

CARLA_BRIDGE_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// ---------------------------------------------------------------------

CarlaBridgeClient::CarlaBridgeClient(const char* const uiTitle)
    : kOsc(this),
#ifdef BUILD_BRIDGE_UI
      fUI(CarlaBridgeToolkit::createNew(this, uiTitle)),
#endif
      fOscData(nullptr)
{
    CARLA_ASSERT(uiTitle != nullptr);
    carla_debug("CarlaBridgeClient::CarlaBridgeClient(\"%s\")", uiTitle);
}

CarlaBridgeClient::~CarlaBridgeClient()
{
    carla_debug("CarlaBridgeClient::~CarlaBridgeClient()");
}

#ifdef BUILD_BRIDGE_UI
// ---------------------------------------------------------------------
// ui initialization

bool CarlaBridgeClient::uiInit(const char* const, const char* const)
{
    carla_debug("CarlaBridgeClient::uiInit()");

    fUI.init();

    return true;
}

void CarlaBridgeClient::uiClose()
{
    carla_debug("CarlaBridgeClient::uiClose()");

    if (isOscControlRegistered() && ! fUI.quit)
        sendOscExiting();

    fUI.close();
}

// ---------------------------------------------------------------------
// ui toolkit

void CarlaBridgeClient::toolkitShow()
{
    carla_debug("CarlaBridgeClient::toolkitShow()");

    fUI.toolkit->show();
}

void CarlaBridgeClient::toolkitHide()
{
    carla_debug("CarlaBridgeClient::toolkitHide()");

    fUI.toolkit->hide();
}

void CarlaBridgeClient::toolkitResize(const int width, const int height)
{
    carla_debug("CarlaBridgeClient::toolkitResize(%i, %i)", width, height);

    fUI.toolkit->resize(width, height);
}

void CarlaBridgeClient::toolkitExec(const bool showGui)
{
    carla_debug("CarlaBridgeClient::toolkitExec(%s)", bool2str(showGui));

    fUI.toolkit->exec(showGui);
}

void CarlaBridgeClient::toolkitQuit()
{
    carla_debug("CarlaBridgeClient::toolkitQuit()");

    fUI.close();
}
#endif

// ---------------------------------------------------------------------
// osc stuff

void CarlaBridgeClient::oscInit(const char* const url)
{
    CARLA_ASSERT(fOscData == nullptr);
    carla_debug("CarlaBridgeClient::oscInit(\"%s\")", url);

    kOsc.init(url);
    fOscData = kOsc.getControlData();
}

bool CarlaBridgeClient::oscIdle()
{
    kOsc.idle();

#ifdef BUILD_BRIDGE_UI
    return ! fUI.quit;
#else
    return true;
#endif
}

void CarlaBridgeClient::oscClose()
{
    CARLA_ASSERT(fOscData != nullptr);
    carla_debug("CarlaBridgeClient::oscClose()");

    fOscData = nullptr;
    kOsc.close();
}

bool CarlaBridgeClient::isOscControlRegistered() const
{
    return kOsc.isControlRegistered();
}

void CarlaBridgeClient::sendOscUpdate()
{
    CARLA_ASSERT(fOscData != nullptr);
    carla_debug("CarlaBridgeClient::sendOscUpdate()");

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_update(fOscData, kOsc.getServerPath());
}

#ifdef BUILD_BRIDGE_PLUGIN
void CarlaBridgeClient::sendOscBridgeUpdate()
{
    CARLA_ASSERT(fOscData != nullptr);
    CARLA_ASSERT(fOscData->target != nullptr && fOscData->path != nullptr);
    carla_debug("CarlaBridgeClient::sendOscBridgeUpdate()");

    if (fOscData != nullptr && fOscData->target != nullptr && fOscData->path != nullptr)
        osc_send_bridge_update(fOscData, fOscData->path);
}

void CarlaBridgeClient::sendOscBridgeError(const char* const error)
{
    CARLA_ASSERT(fOscData != nullptr);
    CARLA_ASSERT(error != nullptr);
    carla_debug("CarlaBridgeClient::sendOscBridgeError(\"%s\")", error);

    if (fOscData != nullptr && fOscData->target != nullptr && error != nullptr)
        osc_send_bridge_error(fOscData, error);
}
#endif

// ---------------------------------------------------------------------

void CarlaBridgeClient::sendOscConfigure(const char* const key, const char* const value)
{
    CARLA_ASSERT(fOscData != nullptr);
    carla_debug("CarlaBridgeClient::sendOscConfigure(\"%s\", \"%s\")", key, value);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_configure(fOscData, key, value);
}

void CarlaBridgeClient::sendOscControl(const int32_t index, const float value)
{
    CARLA_ASSERT(fOscData != nullptr);
    carla_debug("CarlaBridgeClient::sendOscControl(%i, %f)", index, value);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_control(fOscData, index, value);
}

void CarlaBridgeClient::sendOscProgram(const int32_t index)
{
    CARLA_ASSERT(fOscData != nullptr);
    carla_debug("CarlaBridgeClient::sendOscProgram(%i)", index);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_program(fOscData, index);
}

void CarlaBridgeClient::sendOscMidiProgram(const int32_t index)
{
    CARLA_ASSERT(fOscData != nullptr);
    carla_debug("CarlaBridgeClient::sendOscMidiProgram(%i)", index);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_midi_program(fOscData, index);
}

void CarlaBridgeClient::sendOscMidi(const uint8_t midiBuf[4])
{
    CARLA_ASSERT(fOscData != nullptr);
    carla_debug("CarlaBridgeClient::sendOscMidi(%p)", midiBuf);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_midi(fOscData, midiBuf);
}

void CarlaBridgeClient::sendOscExiting()
{
    CARLA_ASSERT(fOscData != nullptr);
    carla_debug("CarlaBridgeClient::sendOscExiting()");

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_exiting(fOscData);
}

#ifdef BRIDGE_LV2
void CarlaBridgeClient::sendOscLv2AtomTransfer(const int32_t portIndex, const char* const atomBuf)
{
    CARLA_ASSERT(fOscData != nullptr);
    carla_debug("CarlaBridgeClient::sendOscLv2TransferAtom(%i, \"%s\")", portIndex, atomBuf);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_lv2_atom_transfer(fOscData, portIndex, atomBuf);
}

void CarlaBridgeClient::sendOscLv2UridMap(const uint32_t urid, const char* const uri)
{
    CARLA_ASSERT(fOscData != nullptr);
    carla_debug("CarlaBridgeClient::sendOscLv2UridMap(%i, \"%s\")", urid, uri);

    if (fOscData != nullptr && fOscData->target != nullptr)
        osc_send_lv2_urid_map(fOscData, urid, uri);
}
#endif

// ---------------------------------------------------------------------

#ifdef BUILD_BRIDGE_UI
void* CarlaBridgeClient::getContainerId()
{
    carla_debug("CarlaBridgeClient::getContainerId()");
    return fUI.toolkit->getContainerId();
}

bool CarlaBridgeClient::uiLibOpen(const char* const filename)
{
    CARLA_ASSERT(fUI.lib == nullptr);
    CARLA_ASSERT(filename != nullptr);
    carla_debug("CarlaBridgeClient::uiLibOpen(\"%s\")", filename);

    fUI.lib      = lib_open(filename);
    fUI.filename = filename;

    return (fUI.lib != nullptr);
}

bool CarlaBridgeClient::uiLibClose()
{
    CARLA_ASSERT(fUI.lib != nullptr);
    carla_debug("CarlaBridgeClient::uiLibClose()");

    if (fUI.lib == nullptr)
        return false;

    const bool closed = lib_close(fUI.lib);
    fUI.lib = nullptr;
    return closed;
}

void* CarlaBridgeClient::uiLibSymbol(const char* const symbol)
{
    CARLA_ASSERT(fUI.lib != nullptr);
    carla_debug("CarlaBridgeClient::uiLibSymbol(\"%s\")", symbol);

    if (fUI.lib == nullptr)
        return nullptr;

    return lib_symbol(fUI.lib, symbol);
}

const char* CarlaBridgeClient::uiLibError()
{
    carla_debug("CarlaBridgeClient::uiLibError()");

    return lib_error(fUI.filename);
}
#endif

CARLA_BRIDGE_END_NAMESPACE
