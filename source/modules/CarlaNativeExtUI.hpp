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
          fExtUiPath(extUiPath),
          leakDetector_NativePluginAndUiClass() {}

protected:
    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (show)
        {
            if (isRunning())
                return;

            CarlaString path(getResourceDir() + fExtUiPath);
            carla_stdout("Trying to start UI using \"%s\"", path.buffer());

            CarlaExternalUI::setData(path, getSampleRate(), getUiName());
            CarlaExternalUI::start();
        }
        else
        {
            CarlaExternalUI::stop(5000);
        }
    }

    void uiIdle() override
    {
        CarlaExternalUI::idle();

        switch (CarlaExternalUI::getAndResetUiState())
        {
        case CarlaExternalUI::UiNone:
        case CarlaExternalUI::UiShow:
            break;
        case CarlaExternalUI::UiCrashed:
            hostUiUnavailable();
            break;
        case CarlaExternalUI::UiHide:
            uiClosed();
            CarlaExternalUI::stop(2000);
            break;
        }
    }

    void uiSetParameterValue(const uint32_t index, const float value) override
    {
        CARLA_SAFE_ASSERT_RETURN(index < getParameterCount(),);

        char tmpBuf[0xff+1];

        const CarlaMutexLocker cml(getLock());

        writeMsg("control\n", 8);
        std::sprintf(tmpBuf, "%i\n", index);
        writeMsg(tmpBuf);
        std::sprintf(tmpBuf, "%f\n", value);
        writeMsg(tmpBuf);
    }

    void uiSetMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program) override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);

        char tmpBuf[0xff+1];

        const CarlaMutexLocker cml(getLock());

        writeMsg("program\n", 8);
        std::sprintf(tmpBuf, "%i\n", channel);
        writeMsg(tmpBuf);
        std::sprintf(tmpBuf, "%i\n", bank);
        writeMsg(tmpBuf);
        std::sprintf(tmpBuf, "%i\n", program);
        writeMsg(tmpBuf);
    }

    void uiSetCustomData(const char* const key, const char* const value) override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

        const CarlaMutexLocker cml(getLock());

        writeMsg("configure\n", 10);
        writeAndFixMsg(key);
        writeAndFixMsg(value);
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
                uiMidiProgramChanged(channel, bank, program);
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

        carla_stderr("msgReceived : %s", msg);
        return false;
    }

private:
    CarlaString fExtUiPath;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativePluginAndUiClass)
};

/**@}*/

// -----------------------------------------------------------------------

#endif // CARLA_NATIVE_EXTERNAL_UI_HPP_INCLUDED
