/*
 * Carla External UI
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_EXTERNAL_UI_HPP_INCLUDED
#define CARLA_EXTERNAL_UI_HPP_INCLUDED

#include "CarlaPipeUtils.hpp"
#include "CarlaString.hpp"

// -----------------------------------------------------------------------

class CarlaExternalUI : public CarlaPipeServer
{
public:
    enum UiState {
        UiNone = 0,
        UiHide,
        UiShow,
        UiCrashed
    };

    CarlaExternalUI() noexcept
        : fFilename(),
          fArg1(),
          fArg2(),
          fUiState(UiNone) {}

    ~CarlaExternalUI() /*noexcept*/ override
    {
        CARLA_SAFE_ASSERT_INT(fUiState == UiNone, fUiState);
    }

    UiState getAndResetUiState() noexcept
    {
        const UiState uiState(fUiState);
        fUiState = UiNone;
        return uiState;
    }

    void setData(const char* const filename, const char* const arg1, const char* const arg2) noexcept
    {
        fFilename = filename;
        fArg1     = arg1;
        fArg2     = arg2;
    }

    void setData(const char* const filename, const double sampleRate, const char* const uiTitle) noexcept
    {
        fFilename = filename;
        fArg1     = CarlaString(sampleRate);
        fArg2     = uiTitle;
    }

    bool startPipeServer(const bool show = true) noexcept
    {
        if (CarlaPipeServer::startPipeServer(fFilename, fArg1, fArg2))
        {
            if (show)
                writeShowMessage();
            return true;
        }

        return false;
    }

protected:
    // returns true if msg was handled
    bool msgReceived(const char* const msg) noexcept override
    {
        if (std::strcmp(msg, "exiting") == 0)
        {
            closePipeServer();
            fUiState = UiHide;
            return true;
        }

        return false;
    }

private:
    CarlaString fFilename;
    CarlaString fArg1;
    CarlaString fArg2;
    UiState     fUiState;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaExternalUI)
};

// -----------------------------------------------------------------------

#endif // CARLA_EXTERNAL_UI_HPP_INCLUDED
