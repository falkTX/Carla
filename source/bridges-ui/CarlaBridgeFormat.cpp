/*
 * Carla Bridge UI
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

#include "CarlaBridgeFormat.hpp"
#include "CarlaBridgeToolkit.hpp"

#include "CarlaBase64Utils.hpp"
#include "CarlaProcessUtils.hpp"

#include "CarlaMIDI.h"

// needed for atom-util
#ifndef nullptr
# undef NULL
# define NULL nullptr
#endif

#include "lv2/atom-util.h"

CARLA_BRIDGE_UI_START_NAMESPACE

// ---------------------------------------------------------------------

CarlaBridgeFormat::CarlaBridgeFormat() noexcept
    : CarlaPipeClient(),
      fQuitReceived(false),
      fGotOptions(false),
      fLastMsgTimer(-1),
      fToolkit(nullptr),
      fLib(nullptr),
      fLibFilename(),
      fBase64ReservedChunk()
{
    carla_debug("CarlaBridgeFormat::CarlaBridgeFormat()");

    try {
        fToolkit = CarlaBridgeToolkit::createNew(this);
    } CARLA_SAFE_EXCEPTION_RETURN("CarlaBridgeToolkit::createNew",);
}

CarlaBridgeFormat::~CarlaBridgeFormat() /*noexcept*/
{
    carla_debug("CarlaBridgeFormat::~CarlaBridgeFormat()");

    if (isPipeRunning() && ! fQuitReceived)
        writeExitingMessageAndWait();

    if (fLib != nullptr)
    {
        lib_close(fLib);
        fLib = nullptr;
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

bool CarlaBridgeFormat::libOpen(const char* const filename) noexcept
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

void* CarlaBridgeFormat::libSymbol(const char* const symbol) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fLib != nullptr, nullptr);

    return lib_symbol<void*>(fLib, symbol);
}

const char* CarlaBridgeFormat::libError() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fLibFilename.isNotEmpty(), nullptr);

    return lib_error(fLibFilename);
}

// ---------------------------------------------------------------------

bool CarlaBridgeFormat::msgReceived(const char* const msg) noexcept
{
    carla_debug("CarlaBridgeFormat::msgReceived(\"%s\")", msg);

    if (! fGotOptions && std::strcmp(msg, "urid") != 0 && std::strcmp(msg, "uiOptions") != 0)
    {
        carla_stderr2("CarlaBridgeFormat::msgReceived(\"%s\") - invalid message while waiting for options", msg);
        return true;
    }

    if (fLastMsgTimer > 0)
        --fLastMsgTimer;

    if (std::strcmp(msg, "atom") == 0)
    {
        uint32_t index, atomTotalSize, base64Size;
        const char* base64atom;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(index), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(atomTotalSize), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(base64Size), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(base64atom, false, base64Size), true);

        carla_getChunkFromBase64String_impl(fBase64ReservedChunk, base64atom);
        CARLA_SAFE_ASSERT_UINT2_RETURN(fBase64ReservedChunk.size() >= sizeof(LV2_Atom),
                                       fBase64ReservedChunk.size(), sizeof(LV2_Atom), true);

#ifdef CARLA_PROPER_CPP11_SUPPORT
        const LV2_Atom* const atom((const LV2_Atom*)fBase64ReservedChunk.data());
#else
        const LV2_Atom* const atom((const LV2_Atom*)&fBase64ReservedChunk.front());
#endif
        const uint32_t atomTotalSizeCheck(lv2_atom_total_size(atom));

        CARLA_SAFE_ASSERT_UINT2_RETURN(atomTotalSizeCheck == atomTotalSize, atomTotalSizeCheck, atomTotalSize, true);
        CARLA_SAFE_ASSERT_UINT2_RETURN(atomTotalSizeCheck == fBase64ReservedChunk.size(),
                                       atomTotalSizeCheck, fBase64ReservedChunk.size(), true);

        dspAtomReceived(index, atom);
        return true;
    }

    if (std::strcmp(msg, "urid") == 0)
    {
        uint32_t urid, size;
        const char* uri;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(urid), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(size), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(uri, false, size), true);

        if (urid != 0)
            dspURIDReceived(urid, uri);

        return true;
    }

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

    if (std::strcmp(msg, "midiprogram") == 0)
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

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(key, true), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(value, false), true);

        dspStateChanged(key, value);

        delete[] key;
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
        return true;
    }

    if (std::strcmp(msg, "uiOptions") == 0)
    {
        BridgeFormatOptions opts;
        uint64_t transientWindowId;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsDouble(opts.sampleRate), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(opts.bgColor), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(opts.fgColor), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(opts.uiScale), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(opts.useTheme), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(opts.useThemeColors), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(opts.windowTitle, true), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsULong(transientWindowId), true);
        opts.transientWindowId = transientWindowId;

        fGotOptions = true;
        uiOptionsChanged(opts);

        delete[] opts.windowTitle;
        return true;
    }

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
        delete fToolkit;
        fToolkit = nullptr;
        return true;
    }

    if (std::strcmp(msg, "uiTitle") == 0)
    {
        const char* title;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(title, false), true);

        fToolkit->setTitle(title);
        return true;
    }

    carla_stderr("CarlaBridgeFormat::msgReceived : %s", msg);
    return false;
}

// ---------------------------------------------------------------------

bool CarlaBridgeFormat::init(const int argc, const char* argv[])
{
    CARLA_SAFE_ASSERT_RETURN(fToolkit != nullptr, false);

    if (argc == 7)
    {
        if (! initPipeClient(argv))
            return false;

        fLastMsgTimer = 0;

        // wait for ui options
        for (; ++fLastMsgTimer < 50 && ! fGotOptions;)
        {
            idlePipe(true);
            carla_msleep(20);
        }

        if (! fGotOptions)
        {
            carla_stderr2("CarlaBridgeFormat::init() - did not get options on time, quitting...");
            writeExitingMessageAndWait();
            closePipeClient();
            return false;
        }
    }

    if (! fToolkit->init(argc, argv))
    {
        if (argc == 7)
            closePipeClient();
        return false;
    }

    return true;
}

void CarlaBridgeFormat::exec(const bool showUI)
{
    CARLA_SAFE_ASSERT_RETURN(fToolkit != nullptr,);

    carla_terminateProcessOnParentExit(true);
    fToolkit->exec(showUI);
}

// ---------------------------------------------------------------------

CARLA_BRIDGE_UI_END_NAMESPACE

#include "CarlaPipeUtils.cpp"

// ---------------------------------------------------------------------
