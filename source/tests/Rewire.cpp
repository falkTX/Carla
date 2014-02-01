/*
 * Carla Tests
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaString.hpp"

#include "rewire/ReWirePanelAPI.h"

// -----------------------------------------------------------------------

// remove this
namespace ReWire {
extern "C" {
typedef const char* TReWireFileSpec;
}
}

ReWire::TReWireFileSpec kDeviceSpec = "/home/falktx/.wine32/drive_c/Program Files/Image-Line/FL Studio 11/System/Plugin/ReWire/FLReWire.dll";

// -----------------------------------------------------------------------

class RewireThing
{
public:
    RewireThing()
        : fIsOpen(false),
          fIsRegistered(false),
          fIsLoaded(false),
          fHandle(0)
    {
        fDevName = "";
    }

    ~RewireThing()
    {
        close();
    }

    bool open()
    {
        CARLA_SAFE_ASSERT_RETURN(! fIsOpen, true);

        const ReWire::ReWireError result(ReWire::RWPOpen());

        if (result == ReWire::kReWireError_NoError)
        {
            fIsOpen = true;
            carla_stderr("rewire open ok");
        }
        else
        {
            carla_stderr2("rewire open failed");
        }

        return fIsOpen;
    }

    bool start()
    {
        char isRunningFlag = 0;
        ReWire::ReWireError result = ReWire::RWPIsReWireMixerAppRunning(&isRunningFlag);

        if (isRunningFlag == 0)
        {
            carla_stderr("rewire mixer is NOT running");

            result = ReWire::RWPRegisterDevice(kDeviceSpec);

            if (result == ReWire::kReWireError_NoError)
            {
                fIsRegistered = true;
                carla_stderr("rewire register ok");
            }
            else if (result == ReWire::kReWireError_AlreadyExists)
            {
                fIsRegistered = true;
                carla_stderr("rewire register already in place");
            }
            else
            {
                carla_stderr2("rewire register failed %i", result);
            }
        }
        else
        {
            carla_stderr("rewire mixer is running");
        }

        return false;
    }

    void close()
    {
        if (! fIsOpen)
        {
            CARLA_SAFE_ASSERT(! fIsLoaded);
            CARLA_SAFE_ASSERT(fHandle == 0);
            return;
        }

        if (fHandle != 0)
        {
            const ReWire::ReWireError result(ReWire::RWPComDisconnect(fHandle));
            fHandle = 0;

            if (result == ReWire::kReWireError_NoError)
                carla_stderr("rewire disconnect ok");
            else
                carla_stderr2("rewire disconnect failed");
        }

        if (fIsLoaded)
        {
            const ReWire::ReWireError result(ReWire::RWPUnloadDevice(fDevName.getBuffer()));
            fIsLoaded = false;

            if (result == ReWire::kReWireError_NoError)
                carla_stderr("rewire unload ok");
            else
                carla_stderr2("rewire unload failed");
        }

        if (fIsRegistered)
        {
            const ReWire::ReWireError result(ReWire::RWPUnregisterDevice(kDeviceSpec));
            fIsRegistered = false;

            if (result == ReWire::kReWireError_NoError)
                carla_stderr("rewire unregister ok");
            else
                carla_stderr2("rewire unregister failed");
        }

        const ReWire::ReWireError result(ReWire::RWPClose());

        if (result == ReWire::kReWireError_NoError)
            carla_stderr("rewire close ok");
        else
            carla_stderr2("rewire close failed");

        fIsOpen = false;
    }

private:
    bool fIsOpen;
    bool fIsRegistered;
    bool fIsLoaded;
    ReWire::TRWPPortHandle fHandle;

    CarlaString fDevName;
};

// -----------------------------------------------------------------------

int main(/*int argc, char* argv[]*/)
{
    RewireThing re;

    if (! re.open())
        return 1;

    re.start();
    re.close();

    return 0;
}

// -----------------------------------------------------------------------
