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

// needed for atom-util
#ifndef nullptr
# undef NULL
# define NULL nullptr
#endif

#include "lv2/atom-util.h"

CARLA_BRIDGE_START_NAMESPACE

// ---------------------------------------------------------------------

CarlaBridgeUI::CarlaBridgeUI() noexcept
    : CarlaPipeClient(),
      fQuitReceived(false),
      fGotOptions(false),
      fLastMsgTimer(-1),
      fToolkit(nullptr),
      fLib(nullptr),
      fLibFilename()
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
    carla_debug("CarlaBridgeUI::msgReceived(\"%s\")", msg);

    if (! fGotOptions) {
        CARLA_SAFE_ASSERT_RETURN(std::strcmp(msg, "urid") == 0 || std::strcmp(msg, "uiOptions") == 0, true);
    }

    if (fLastMsgTimer > 0)
        --fLastMsgTimer;

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

    if (std::strcmp(msg, "atom") == 0)
    {
        uint32_t index, atomTotalSize;
        const char* base64atom;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(index), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(atomTotalSize), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(base64atom), true);

        std::vector<uint8_t> chunk(carla_getChunkFromBase64String(base64atom));
        delete[] base64atom;
        CARLA_SAFE_ASSERT_RETURN(chunk.size() >= sizeof(LV2_Atom), true);

        const LV2_Atom* const atom((const LV2_Atom*)chunk.data());
        const uint32_t atomTotalSizeCheck(lv2_atom_total_size(atom));

        CARLA_SAFE_ASSERT_RETURN(atomTotalSizeCheck == atomTotalSize, true);
        CARLA_SAFE_ASSERT_RETURN(atomTotalSizeCheck == chunk.size(), true);

        dspAtomReceived(index, atom);
        return true;
    }

    if (std::strcmp(msg, "urid") == 0)
    {
        uint32_t urid;
        const char* uri;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(urid), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(uri), true);

        if (urid != 0)
            dspURIDReceived(urid, uri);

        delete[] uri;
        return true;
    }

    if (std::strcmp(msg, "uiOptions") == 0)
    {
        double sampleRate;
        bool useTheme, useThemeColors;
        const char* windowTitle;
        uint64_t transientWindowId;

        CARLA_SAFE_ASSERT_RETURN(readNextLineAsDouble(sampleRate), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(useTheme), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(useThemeColors), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(windowTitle), true);
        CARLA_SAFE_ASSERT_RETURN(readNextLineAsULong(transientWindowId), true);

        fGotOptions = true;
        uiOptionsChanged(sampleRate, useTheme, useThemeColors, windowTitle, static_cast<uintptr_t>(transientWindowId));

        delete[] windowTitle;
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
            carla_stderr2("CarlaBridgeUI::init() - did not get options on time, quitting...");
            {
                const CarlaMutexLocker cml(getPipeLock());
                writeMessage("exiting\n", 8);
                flushMessages();
            }
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

void CarlaBridgeUI::exec(const bool showUI)
{
    CARLA_SAFE_ASSERT_RETURN(fToolkit != nullptr,);

    fToolkit->exec(showUI);
}

// ---------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

#include "CarlaPipeUtils.cpp"

// ---------------------------------------------------------------------
