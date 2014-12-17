/*
 * Carla Bridge UI
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

#include "CarlaBridgeUI.hpp"
#include "CarlaMIDI.h"

#include "CarlaBase64Utils.hpp"

#ifdef BRIDGE_LV2
# undef NULL
# define NULL nullptr
# include "lv2/atom-util.h"
#endif

CARLA_BRIDGE_START_NAMESPACE

// ---------------------------------------------------------------------

CarlaBridgeUI::CarlaBridgeUI() noexcept
    : CarlaPipeClient(),
      fQuitReceived(false),
#ifdef BRIDGE_LV2
      fUridMapComplete(false),
#endif
      fToolkit(nullptr),
      fLib(nullptr),
      fLibFilename(),
      leakDetector_CarlaBridgeUI()
{
    carla_debug("CarlaBridgeUI::CarlaBridgeUI()");

    try {
        fToolkit = CarlaBridgeToolkit::createNew(this);
    } CARLA_SAFE_EXCEPTION_RETURN("CarlaBridgeToolkit::createNew",);
}

CarlaBridgeUI::~CarlaBridgeUI() /*noexcept*/
{
    carla_debug("CarlaBridgeUI::~CarlaBridgeUI()");

    if (fLib != nullptr)
    {
        lib_close(fLib);
        fLib = nullptr;
    }

    if (isPipeRunning() && ! fQuitReceived)
    {
        const CarlaMutexLocker cml(getPipeLock());
        writeMessage("exiting\n", 8);
        flushMessages();
    }

    if (fToolkit != nullptr)
    {
        fToolkit->quit();
        delete fToolkit;
        fToolkit = nullptr;
    }

    closePipeClient();
}

// ---------------------------------------------------------------------

bool CarlaBridgeUI::libOpen(const char* const filename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fLib == nullptr, false);

    fLib = lib_open(filename);

    if (fLib != nullptr)
    {
        fLibFilename = filename;
        return true;
    }

    return false;
}

void* CarlaBridgeUI::libSymbol(const char* const symbol) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fLib != nullptr, nullptr);

    return lib_symbol<void*>(fLib, symbol);
}

const char* CarlaBridgeUI::libError() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fLibFilename.isNotEmpty(), nullptr);

    return lib_error(fLibFilename);
}

// ---------------------------------------------------------------------

bool CarlaBridgeUI::msgReceived(const char* const msg) noexcept
{
#ifdef BRIDGE_LV2
    if (! fUridMapComplete) {
        CARLA_SAFE_ASSERT_RETURN(std::strcmp(msg, "urid") == 0, true);
    }
#endif

    if (std::strcmp(msg, "control") == 0)
    {
        uint32_t index;
        float value;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(index), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

        dspParameterChanged(index, value);
        return true;
    }

    if (std::strcmp(msg, "program") == 0)
    {
        uint32_t index;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(index), true);

        dspProgramChanged(index);
        return true;
    }

    if (std::strcmp(msg, "mprogram") == 0)
    {
        uint32_t bank, program;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(bank), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(program), true);

        dspMidiProgramChanged(bank, program);
        return true;
    }

    if (std::strcmp(msg, "configure") == 0)
    {
        const char* key;
        const char* value;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(key), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(value), true);

        dspStateChanged(key, value);

        delete[] key;
        delete[] value;
        return true;
    }

    if (std::strcmp(msg, "note") == 0)
    {
        bool onOff;
        uint8_t channel, note, velocity;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(onOff), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(channel), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(note), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(velocity), true);

        dspNoteReceived(onOff, channel, note, velocity);
    }

#ifdef BRIDGE_LV2
    if (std::strcmp(msg, "atom") == 0)
    {
        uint32_t index, size;
        const char* base64atom;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(index), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(size), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(base64atom), true);

        std::vector<uint8_t> chunk(carla_getChunkFromBase64String(base64atom));
        delete[] base64atom;
        CARLA_SAFE_ASSERT_RETURN(chunk.size() >= sizeof(LV2_Atom), true);

        const LV2_Atom* const atom((const LV2_Atom*)chunk.data());
        CARLA_SAFE_ASSERT_RETURN(lv2_atom_total_size(atom) == chunk.size(), true);

        dspAtomReceived(index, atom);
        return true;
    }

    if (std::strcmp(msg, "urid") == 0)
    {
        uint32_t urid;
        const char* uri;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(urid), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(uri), true);

        if (urid == 0)
        {
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(uri, "Complete") == 0, true);
            fUridMapComplete = true;
        }
        else
        {
            dspURIDReceived(urid, uri);
        }

        delete[] uri;
        return true;
    }
#endif

    CARLA_SAFE_ASSERT_RETURN(fToolkit != nullptr, true);

    if (std::strcmp(msg, "show") == 0)
    {
        fToolkit->show();
        return true;
    }

    if (std::strcmp(msg, "focus") == 0)
    {
        fToolkit->focus();
        return true;
    }

    if (std::strcmp(msg, "hide") == 0)
    {
        fToolkit->hide();
        return true;
    }

    if (std::strcmp(msg, "quit") == 0)
    {
        fQuitReceived = true;
        fToolkit->quit();
        return true;
    }

    if (std::strcmp(msg, "uiTitle") == 0)
    {
        const char* title;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(title), true);

        fToolkit->setTitle(title);

        delete[] title;
        return true;
    }

    carla_stderr("CarlaBridgeUI::msgReceived : %s", msg);
    return false;
}

// ---------------------------------------------------------------------

bool CarlaBridgeUI::init(const int argc, const char* argv[])
{
    CARLA_SAFE_ASSERT_RETURN(fToolkit != nullptr, false);

    if (! fToolkit->init(argc, argv))
        return false;

    if (argc == 7)
    {
        if (! initPipeClient(argv))
        {
            fToolkit->quit();
            delete fToolkit;
            fToolkit = nullptr;
            return false;
        }

#ifdef BRIDGE_LV2
        // wait for URID map to complete
        for (int i=0; i<20 && ! fUridMapComplete; ++i)
        {
            idlePipe();
            carla_msleep(100);
        }

        if (! fUridMapComplete)
        {
            fToolkit->quit();
            delete fToolkit;
            fToolkit = nullptr;
            closePipeClient();
        }
#endif
    }

    return true;
}

void CarlaBridgeUI::exec(const bool showUI)
{
    CARLA_SAFE_ASSERT_RETURN(fToolkit != nullptr,);

    fToolkit->exec(showUI);
}

// ---------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

#include "CarlaPipeUtils.cpp"

// ---------------------------------------------------------------------

#if 0
// ---------------------------------------------------------------------
// ui initialization

bool CarlaBridgeUI::uiInit(const char*, const char* const, const char* const)
{
    carla_debug("CarlaBridgeUI::uiInit()");

    fUI.init();

    return true;
}

void CarlaBridgeUI::uiClose()
{
    carla_debug("CarlaBridgeUI::uiClose()");

    if (isOscControlRegistered() && ! fUI.quit)
        sendOscExiting();

    fUI.close();
}

// ---------------------------------------------------------------------
// ui toolkit

void CarlaBridgeUI::toolkitShow()
{
    CARLA_SAFE_ASSERT_RETURN(fUI.toolkit != nullptr,);
    carla_debug("CarlaBridgeUI::toolkitShow()");

    fUI.toolkit->show();
}

void CarlaBridgeUI::toolkitHide()
{
    CARLA_SAFE_ASSERT_RETURN(fUI.toolkit != nullptr,);
    carla_debug("CarlaBridgeUI::toolkitHide()");

    fUI.toolkit->hide();
}

void CarlaBridgeUI::toolkitResize(const int width, const int height)
{
    CARLA_SAFE_ASSERT_RETURN(fUI.toolkit != nullptr,);
    carla_debug("CarlaBridgeUI::toolkitResize(%i, %i)", width, height);

    fUI.toolkit->resize(width, height);
}

void CarlaBridgeUI::toolkitExec(const bool showGui)
{
    CARLA_SAFE_ASSERT_RETURN(fUI.toolkit != nullptr,);
    carla_debug("CarlaBridgeUI::toolkitExec(%s)", bool2str(showGui));

    fUI.toolkit->exec(showGui);
}

void CarlaBridgeUI::toolkitQuit()
{
    carla_debug("CarlaBridgeUI::toolkitQuit()");

    fUI.quit = true;
}

// ---------------------------------------------------------------------
// osc stuff

void CarlaBridgeUI::oscInit(const char* const url)
{
    carla_debug("CarlaBridgeUI::oscInit(\"%s\")", url);

    fOsc.init(url);
}

bool CarlaBridgeUI::oscIdle(const bool onlyOnce) const
{
    if (onlyOnce)
        fOsc.idleOnce();
    else
        fOsc.idle();

    return ! fUI.quit;
}

void CarlaBridgeUI::oscClose()
{
    carla_debug("CarlaBridgeUI::oscClose()");

    fOsc.close();
}

bool CarlaBridgeUI::isOscControlRegistered() const noexcept
{
    return fOsc.isControlRegistered();
}

void CarlaBridgeUI::sendOscUpdate() const
{
    carla_debug("CarlaBridgeUI::sendOscUpdate()");

    if (fOscData.target != nullptr)
        osc_send_update(fOscData, fOsc.getServerPath());
}

// ---------------------------------------------------------------------

void CarlaBridgeUI::sendOscConfigure(const char* const key, const char* const value) const
{
    carla_debug("CarlaBridgeUI::sendOscConfigure(\"%s\", \"%s\")", key, value);

    if (fOscData.target != nullptr)
        osc_send_configure(fOscData, key, value);
}

void CarlaBridgeUI::sendOscControl(const int32_t index, const float value) const
{
    carla_debug("CarlaBridgeUI::sendOscControl(%i, %f)", index, value);

    if (fOscData.target != nullptr)
        osc_send_control(fOscData, index, value);
}

void CarlaBridgeUI::sendOscProgram(const uint32_t index) const
{
    carla_debug("CarlaBridgeUI::sendOscProgram(%i)", index);

    if (fOscData.target != nullptr)
        osc_send_program(fOscData, index);
}

void CarlaBridgeUI::sendOscMidiProgram(const uint32_t index) const
{
    carla_debug("CarlaBridgeUI::sendOscMidiProgram(%i)", index);

    if (fOscData.target != nullptr)
        osc_send_midi_program(fOscData, index);
}

void CarlaBridgeUI::sendOscMidi(const uint8_t midiBuf[4]) const
{
    carla_debug("CarlaBridgeUI::sendOscMidi(%p)", midiBuf);

    if (fOscData.target != nullptr)
        osc_send_midi(fOscData, midiBuf);
}

void CarlaBridgeUI::sendOscExiting() const
{
    carla_debug("CarlaBridgeUI::sendOscExiting()");

    if (fOscData.target != nullptr)
        osc_send_exiting(fOscData);
}

#ifdef BRIDGE_LV2
void CarlaBridgeUI::sendOscLv2AtomTransfer(const uint32_t portIndex, const char* const atomBuf) const
{
    carla_debug("CarlaBridgeUI::sendOscLv2TransferAtom(%i, \"%s\")", portIndex, atomBuf);

    if (fOscData.target != nullptr)
        osc_send_lv2_atom_transfer(fOscData, portIndex, atomBuf);
}

void CarlaBridgeUI::sendOscLv2UridMap(const uint32_t urid, const char* const uri) const
{
    carla_debug("CarlaBridgeUI::sendOscLv2UridMap(%i, \"%s\")", urid, uri);

    if (fOscData.target != nullptr)
        osc_send_lv2_urid_map(fOscData, urid, uri);
}
#endif

// ---------------------------------------------------------------------

void* CarlaBridgeUI::getContainerId()
{
    carla_debug("CarlaBridgeUI::getContainerId()");
    return fUI.toolkit->getContainerId();
}

void* CarlaBridgeUI::getContainerId2()
{
    carla_debug("CarlaBridgeUI::getContainerId2()");
    return fUI.toolkit->getContainerId2();
}

bool CarlaBridgeUI::uiLibOpen(const char* const filename)
{
    CARLA_ASSERT(fUI.lib == nullptr);
    CARLA_ASSERT(filename != nullptr);
    carla_debug("CarlaBridgeUI::uiLibOpen(\"%s\")", filename);

    fUI.lib      = lib_open(filename);
    fUI.filename = filename;

    return (fUI.lib != nullptr);
}

bool CarlaBridgeUI::uiLibClose()
{
    CARLA_SAFE_ASSERT_RETURN(fUI.lib != nullptr, false);
    carla_debug("CarlaBridgeUI::uiLibClose()");

    const bool closed = lib_close(fUI.lib);
    fUI.lib = nullptr;
    return closed;
}

const char* CarlaBridgeUI::uiLibError()
{
    carla_debug("CarlaBridgeUI::uiLibError()");

    return lib_error(fUI.filename);
}
#endif
