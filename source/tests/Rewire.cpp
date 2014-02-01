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

#include "rewire/ReWireAPI.h"
#include "rewire/ReWirePanelAPI.h"

// -----------------------------------------------------------------------

// remove this
namespace ReWire {
extern "C" {
typedef const char* TReWireFileSpec;
}
}

// static ReWire::TReWireFileSpec kDeviceSpec = "/home/falktx/.wine32/drive_c/Program Files/Image-Line/FL Studio 11/System/Plugin/ReWire/FLReWire.dll";
//static ReWire::TReWireFileSpec kDeviceSpec = "C:\\Program Files\\Waves\\ReWire\\WavesReWireDevice.dll";

static const ReWire::TReWireFileSpec kDeviceSpec = "C:\\Program Files\\AudioGL\\AudioGL.dll";
static const char* const             kDeviceName = "AudioGL";
static const char* const             kSinature   = "Ahem";

// -----------------------------------------------------------------------
// try to figure this out...

using namespace ReWire;

// RWM2OpenImp

typedef ReWireError (*TRWM2OpenProc)(void); //long reWireLibVersion
typedef ReWireError (*TRWM2CloseProc)(void);

static const char kRWM2OpenProcName[] = "RWM2OpenImp";
static const char kRWM2CloseProcName[] = "RWM2CloseImp";

static TRWM2OpenProc gRWM2OpenProc = NULL;
static TRWM2CloseProc gRWM2CloseProc = NULL;

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
            carla_stderr2("rewire open failed %i", result);
        }

        // get func pointers
        gRWM2OpenProc = (TRWM2OpenProc)ReWireFindReWireSharedLibraryFunction(kRWM2OpenProcName);
        gRWM2CloseProc = (TRWM2CloseProc)ReWireFindReWireSharedLibraryFunction(kRWM2CloseProcName);

        CARLA_SAFE_ASSERT_RETURN(gRWM2OpenProc != nullptr, true);
        CARLA_SAFE_ASSERT_RETURN(gRWM2CloseProc != nullptr, true);

        return fIsOpen;
    }

    bool start()
    {
        ReWire::ReWireError result = (gRWM2OpenProc)();

        if (result == ReWire::kReWireError_NoError)
        {
            fIsLoaded = true;
            carla_stderr("rewire load app ok");
        }
        else
        {
            carla_stderr2("rewire load app failed %i", result);
        }

        return true;
    }

#if 0
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

        result = ReWire::RWPLoadDevice(kDeviceName);

        if (result == ReWire::kReWireError_NoError)
        {
            fIsLoaded = true;
            carla_stderr("rewire load device ok");
        }
        else
        {
            carla_stderr2("rewire load device failed %i", result);
        }

        result = ReWire::RWPComConnect(kSinature, &fHandle);

        if (result == ReWire::kReWireError_NoError)
        {
            carla_stderr("rewire connect ok | %i", fHandle);
        }
        else
        {
            carla_stderr2("rewire connect failed %i | %i", result, fHandle);
        }

        return false;
    }
#endif

    void close()
    {
        if (! fIsOpen)
        {
            CARLA_SAFE_ASSERT(! fIsLoaded);
            CARLA_SAFE_ASSERT(fHandle == 0);
            return;
        }

        if (fIsLoaded)
        {
            const ReWire::ReWireError result((gRWM2CloseProc)());
            fIsLoaded = false;

            if (result == ReWire::kReWireError_NoError)
                carla_stderr("rewire unload app ok");
            else
                carla_stderr2("rewire unload app failed %i", result);
        }

#if 0
        if (fHandle != 0)
        {
            const ReWire::ReWireError result(ReWire::RWPComDisconnect(fHandle));
            fHandle = 0;

            if (result == ReWire::kReWireError_NoError)
                carla_stderr("rewire disconnect ok");
            else
                carla_stderr2("rewire disconnect failed %i", result);
        }

        if (fIsLoaded)
        {
            const ReWire::ReWireError result(ReWire::RWPUnloadDevice(kDeviceName));
            fIsLoaded = false;

            if (result == ReWire::kReWireError_NoError)
                carla_stderr("rewire unload device ok");
            else
                carla_stderr2("rewire unload device failed %i", result);
        }

        if (fIsRegistered)
        {
            const ReWire::ReWireError result(ReWire::RWPUnregisterDevice(kDeviceSpec));
            fIsRegistered = false;

            if (result == ReWire::kReWireError_NoError)
                carla_stderr("rewire unregister ok");
            else
                carla_stderr2("rewire unregister failed %i", result);
        }
#endif

        const ReWire::ReWireError result(ReWire::RWPClose());

        if (result == ReWire::kReWireError_NoError)
            carla_stderr("rewire close ok");
        else
            carla_stderr2("rewire close failed %i", result);

        fIsOpen = false;
    }

private:
    bool fIsOpen;
    bool fIsRegistered;
    bool fIsLoaded;
    ReWire::TRWPPortHandle fHandle;
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
