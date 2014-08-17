/*
 * Carla Bridge Client
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

#include "CarlaBridgeClient.hpp"
#include "CarlaLibUtils.hpp"

CARLA_BRIDGE_START_NAMESPACE

// ---------------------------------------------------------------------

CarlaBridgeClient::CarlaBridgeClient(const char* const uiTitle)
    : fOsc(this),
      fOscData(fOsc.getControlData()),
      fUI(CarlaBridgeToolkit::createNew(this, uiTitle))
{
    CARLA_ASSERT(uiTitle != nullptr && uiTitle[0] != '\0');
    carla_debug("CarlaBridgeClient::CarlaBridgeClient(\"%s\")", uiTitle);
}

CarlaBridgeClient::~CarlaBridgeClient()
{
    carla_debug("CarlaBridgeClient::~CarlaBridgeClient()");
}

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
    CARLA_SAFE_ASSERT_RETURN(fUI.toolkit != nullptr,);
    carla_debug("CarlaBridgeClient::toolkitShow()");

    fUI.toolkit->show();
}

void CarlaBridgeClient::toolkitHide()
{
    CARLA_SAFE_ASSERT_RETURN(fUI.toolkit != nullptr,);
    carla_debug("CarlaBridgeClient::toolkitHide()");

    fUI.toolkit->hide();
}

void CarlaBridgeClient::toolkitResize(const int width, const int height)
{
    CARLA_SAFE_ASSERT_RETURN(fUI.toolkit != nullptr,);
    carla_debug("CarlaBridgeClient::toolkitResize(%i, %i)", width, height);

    fUI.toolkit->resize(width, height);
}

void CarlaBridgeClient::toolkitExec(const bool showGui)
{
    CARLA_SAFE_ASSERT_RETURN(fUI.toolkit != nullptr,);
    carla_debug("CarlaBridgeClient::toolkitExec(%s)", bool2str(showGui));

    fUI.toolkit->exec(showGui);
}

void CarlaBridgeClient::toolkitQuit()
{
    carla_debug("CarlaBridgeClient::toolkitQuit()");

    fUI.close();
}

// ---------------------------------------------------------------------
// osc stuff

void CarlaBridgeClient::oscInit(const char* const url)
{
    carla_debug("CarlaBridgeClient::oscInit(\"%s\")", url);

    fOsc.init(url);
}

bool CarlaBridgeClient::oscIdle(const bool onlyOnce) const
{
    if (onlyOnce)
        fOsc.idleOnce();
    else
        fOsc.idle();

    return ! fUI.quit;
}

void CarlaBridgeClient::oscClose()
{
    carla_debug("CarlaBridgeClient::oscClose()");

    fOsc.close();
}

bool CarlaBridgeClient::isOscControlRegistered() const noexcept
{
    return fOsc.isControlRegistered();
}

void CarlaBridgeClient::sendOscUpdate() const
{
    carla_debug("CarlaBridgeClient::sendOscUpdate()");

    if (fOscData.target != nullptr)
        osc_send_update(fOscData, fOsc.getServerPath());
}

// ---------------------------------------------------------------------

void CarlaBridgeClient::sendOscConfigure(const char* const key, const char* const value) const
{
    carla_debug("CarlaBridgeClient::sendOscConfigure(\"%s\", \"%s\")", key, value);

    if (fOscData.target != nullptr)
        osc_send_configure(fOscData, key, value);
}

void CarlaBridgeClient::sendOscControl(const int32_t index, const float value) const
{
    carla_debug("CarlaBridgeClient::sendOscControl(%i, %f)", index, value);

    if (fOscData.target != nullptr)
        osc_send_control(fOscData, index, value);
}

void CarlaBridgeClient::sendOscProgram(const uint32_t index) const
{
    carla_debug("CarlaBridgeClient::sendOscProgram(%i)", index);

    if (fOscData.target != nullptr)
        osc_send_program(fOscData, index);
}

void CarlaBridgeClient::sendOscMidiProgram(const uint32_t index) const
{
    carla_debug("CarlaBridgeClient::sendOscMidiProgram(%i)", index);

    if (fOscData.target != nullptr)
        osc_send_midi_program(fOscData, index);
}

void CarlaBridgeClient::sendOscMidi(const uint8_t midiBuf[4]) const
{
    carla_debug("CarlaBridgeClient::sendOscMidi(%p)", midiBuf);

    if (fOscData.target != nullptr)
        osc_send_midi(fOscData, midiBuf);
}

void CarlaBridgeClient::sendOscExiting() const
{
    carla_debug("CarlaBridgeClient::sendOscExiting()");

    if (fOscData.target != nullptr)
        osc_send_exiting(fOscData);
}

#ifdef BRIDGE_LV2
void CarlaBridgeClient::sendOscLv2AtomTransfer(const uint32_t portIndex, const char* const atomBuf) const
{
    carla_debug("CarlaBridgeClient::sendOscLv2TransferAtom(%i, \"%s\")", portIndex, atomBuf);

    if (fOscData.target != nullptr)
        osc_send_lv2_atom_transfer(fOscData, portIndex, atomBuf);
}

void CarlaBridgeClient::sendOscLv2UridMap(const uint32_t urid, const char* const uri) const
{
    carla_debug("CarlaBridgeClient::sendOscLv2UridMap(%i, \"%s\")", urid, uri);

    if (fOscData.target != nullptr)
        osc_send_lv2_urid_map(fOscData, urid, uri);
}
#endif

// ---------------------------------------------------------------------

void* CarlaBridgeClient::getContainerId()
{
    carla_debug("CarlaBridgeClient::getContainerId()");
    return fUI.toolkit->getContainerId();
}

void* CarlaBridgeClient::getContainerId2()
{
    carla_debug("CarlaBridgeClient::getContainerId2()");
    return fUI.toolkit->getContainerId2();
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
    CARLA_SAFE_ASSERT_RETURN(fUI.lib != nullptr, false);
    carla_debug("CarlaBridgeClient::uiLibClose()");

    const bool closed = lib_close(fUI.lib);
    fUI.lib = nullptr;
    return closed;
}

void* CarlaBridgeClient::uiLibSymbol(const char* const symbol)
{
    CARLA_SAFE_ASSERT_RETURN(fUI.lib != nullptr, nullptr);
    carla_debug("CarlaBridgeClient::uiLibSymbol(\"%s\")", symbol);

    return lib_symbol(fUI.lib, symbol);
}

const char* CarlaBridgeClient::uiLibError()
{
    carla_debug("CarlaBridgeClient::uiLibError()");

    return lib_error(fUI.filename);
}

// ---------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE
