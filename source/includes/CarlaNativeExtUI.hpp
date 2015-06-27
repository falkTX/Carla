/*
 * Carla Native Plugin API (C++)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_NATIVE_EXTERNAL_UI_HPP_INCLUDED
#define CARLA_NATIVE_EXTERNAL_UI_HPP_INCLUDED

#include "CarlaNative.hpp"
#include "CarlaExternalUI.hpp"

/*!
 * @defgroup CarlaNativeAPI Carla Native API
 * @{
 */

// -----------------------------------------------------------------------
// Native Plugin and External UI class

class NativePluginAndUiClass : public NativePluginClass,
                               public CarlaExternalUI
{
public:
    NativePluginAndUiClass(const NativeHostDescriptor* const host, const char* const extUiPath)
        : NativePluginClass(host),
          CarlaExternalUI(),
          fExtUiPath(getResourceDir())
    {
        fExtUiPath += CARLA_OS_SEP_STR;
        fExtUiPath += extUiPath;
    }

    const char* getExtUiPath() const noexcept
    {
        return fExtUiPath;
    }

protected:
    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (show)
        {
            if (isPipeRunning())
            {
                const CarlaMutexLocker cml(getPipeLock());
                writeMessage("focus\n", 6);
                flushMessages();
                return;
            }

            carla_stdout("Trying to start UI using \"%s\"", fExtUiPath.buffer());

            CarlaExternalUI::setData(fExtUiPath, getSampleRate(), getUiName());

            if (! CarlaExternalUI::startPipeServer(true))
            {
                uiClosed();
                hostUiUnavailable();
            }
        }
        else
        {
            CarlaExternalUI::stopPipeServer(2000);
        }
    }

    void uiIdle() override
    {
        CarlaExternalUI::idlePipe();

        switch (CarlaExternalUI::getAndResetUiState())
        {
        case CarlaExternalUI::UiNone:
        case CarlaExternalUI::UiShow:
            break;
        case CarlaExternalUI::UiCrashed:
            uiClosed();
            hostUiUnavailable();
            break;
        case CarlaExternalUI::UiHide:
            uiClosed();
            CarlaExternalUI::stopPipeServer(1000);
            break;
        }
    }

    void uiSetParameterValue(const uint32_t index, const float value) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index < getParameterCount(),);

        // TODO writeControlMessage and others

        char tmpBuf[0xff+1];
        tmpBuf[0xff] = '\0';

        const CarlaMutexLocker cml(getPipeLock());
        const ScopedLocale csl;

        writeMessage("control\n", 8);

        {
            std::snprintf(tmpBuf, 0xff, "%i\n", index);
            writeMessage(tmpBuf);

            std::snprintf(tmpBuf, 0xff, "%f\n", value);
            writeMessage(tmpBuf);
        }

        flushMessages();
    }

    void uiSetMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);

        char tmpBuf[0xff+1];
        tmpBuf[0xff] = '\0';

        const CarlaMutexLocker cml(getPipeLock());

        writeMessage("program\n", 8);

        {
            std::snprintf(tmpBuf, 0xff, "%i\n", channel);
            writeMessage(tmpBuf);

            std::snprintf(tmpBuf, 0xff, "%i\n", bank);
            writeMessage(tmpBuf);

            std::snprintf(tmpBuf, 0xff, "%i\n", program);
            writeMessage(tmpBuf);
        }

        flushMessages();
    }

    void uiSetCustomData(const char* const key, const char* const value) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

        const CarlaMutexLocker cml(getPipeLock());

        writeMessage("configure\n", 10);

        writeAndFixMessage(key);
        writeAndFixMessage(value);

        flushMessages();
    }

    void uiNameChanged(const char* const uiName) override
    {
        CARLA_SAFE_ASSERT_RETURN(uiName != nullptr && uiName[0] != '\0',);

        const CarlaMutexLocker cml(getPipeLock());

        writeMessage("uiTitle\n", 8);
        writeAndFixMessage(uiName);
        flushMessages();
    }

    // -------------------------------------------------------------------
    // Pipe Server calls

    bool msgReceived(const char* const msg) noexcept override
    {
        if (CarlaExternalUI::msgReceived(msg))
            return true;

        if (std::strcmp(msg, "control") == 0)
        {
            uint32_t param;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(param), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

            try {
                uiParameterChanged(param, value);
            } CARLA_SAFE_EXCEPTION("uiParameterChanged");

            return true;
        }

        if (std::strcmp(msg, "program") == 0)
        {
            uint32_t channel, bank, program;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(channel), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(bank), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(program), true);
            CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, true);

            try {
                uiMidiProgramChanged(static_cast<uint8_t>(channel), bank, program);
            } CARLA_SAFE_EXCEPTION("uiMidiProgramChanged");

            return true;
        }

        if (std::strcmp(msg, "configure") == 0)
        {
            const char* key;
            const char* value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(key), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(value), true);

            try {
                uiCustomDataChanged(key, value);
            } CARLA_SAFE_EXCEPTION("uiCustomDataChanged");

            delete[] key;
            delete[] value;

            return true;
        }

        return false;
    }

private:
    CarlaString fExtUiPath;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativePluginAndUiClass)
};

/**@}*/

// -----------------------------------------------------------------------

#endif // CARLA_NATIVE_EXTERNAL_UI_HPP_INCLUDED
