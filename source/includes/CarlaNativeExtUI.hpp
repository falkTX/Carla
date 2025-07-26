// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CARLA_NATIVE_EXTERNAL_UI_HPP_INCLUDED
#define CARLA_NATIVE_EXTERNAL_UI_HPP_INCLUDED

#include "CarlaNative.hpp"
#include "CarlaExternalUI.hpp"
#include "CarlaMIDI.h"

/*!
 * @defgroup CarlaNativeAPI Carla Native API
 * @{
 */

// -----------------------------------------------------------------------
// Native Plugin and External UI class

class NativePluginAndUiClass : public NativePluginClass
#ifndef CARLA_OS_WASM
                             , public CarlaExternalUI
#endif
{
public:
    NativePluginAndUiClass(const NativeHostDescriptor* const host, const char* const pathToExternalUI)
        : NativePluginClass(host),
#ifndef CARLA_OS_WASM
          CarlaExternalUI(),
#endif
          fExtUiPath(getResourceDir())
    {
        fExtUiPath += CARLA_OS_SEP_STR;
        fExtUiPath += pathToExternalUI;
#ifdef CARLA_OS_WIN
        fExtUiPath += ".exe";
#endif
    }

    const char* getExtUiPath() const noexcept
    {
        return fExtUiPath;
    }

#ifndef CARLA_OS_WASM
protected:
    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (show)
        {
            if (isPipeRunning())
            {
                writeFocusMessage();
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

        writeControlMessage(index, value);
    }

    void uiSetMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);

        writeProgramMessage(channel, bank, program);
    }

    void uiSetCustomData(const char* const key, const char* const value) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

        writeConfigureMessage(key, value);
    }

    void uiNameChanged(const char* const uiName) override
    {
        CARLA_SAFE_ASSERT_RETURN(uiName != nullptr && uiName[0] != '\0',);

        const CarlaMutexLocker cml(getPipeLock());

        if (! writeMessage("uiTitle\n", 8))
            return;
        if (! writeAndFixMessage(uiName))
            return;

        syncMessages();
    }

    bool uiMIDIEvent(const uint8_t size, const uint8_t data[]) override
    {
        if (size != 3)
            return false;

        const uint8_t status = MIDI_GET_STATUS_FROM_DATA(data);

        if (! (MIDI_IS_STATUS_NOTE_ON(status) || MIDI_IS_STATUS_NOTE_OFF(status)))
            return false;

        writeMidiNoteMessage(MIDI_IS_STATUS_NOTE_ON(status),
                             MIDI_GET_CHANNEL_FROM_DATA(data),
                             data[1], data[2]);
        return true;
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
            uint8_t channel;
            uint32_t bank, program;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(channel), true);
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

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(key, true), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(value, false), true);

            try {
                uiCustomDataChanged(key, value);
            } CARLA_SAFE_EXCEPTION("uiCustomDataChanged");

            delete[] key;

            return true;
        }

        return false;
    }
#endif

private:
    String fExtUiPath;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativePluginAndUiClass)
};

/**@}*/

// -----------------------------------------------------------------------

#endif // CARLA_NATIVE_EXTERNAL_UI_HPP_INCLUDED
