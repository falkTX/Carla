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

#include "CarlaLibUtils.hpp"

// -----------------------------------------------------------------------

int main(/*int argc, char* argv[]*/)
{
    static const char* const filename = "C:\\Program Files\\Waves\\ReWire\\WavesReWireDevice.dll";
//     static const char* const filename = "C:\\Program Files\\AudioGL\\AudioGL.dll";

    RewireBridge bridge(filename);

    DevInfo devInfo;
    carla_zeroStruct<DevInfo>(devInfo);
    devInfo.size = 8288;

    (bridge.RWDEFGetDeviceInfo)(&devInfo);

    carla_stdout("Ok, this is the info:");
    carla_stdout("\tVersion:  %i", devInfo.version);
    carla_stdout("\tName:     \"%s\"", devInfo.name);
    carla_stdout("\tChannels: %l", devInfo.channelCount);

    for (long i=0; i < devInfo.channelCount; ++i)
        carla_stdout("\t\t#%i: \"%s\"", i+1, devInfo.channelNames[i]);

    OpenInfo info;
    info.size1 = sizeof(OpenInfo);
    info.size2 = 12;
    info.sampleRate = 44100;
    info.bufferSize = 512;

    (bridge.RWDEFOpenDevice)(&info);

#if 0
    carla_stdout("Starting panel...");
    (bridge.RWDEFLaunchPanelApp)();

    for (int i=0; i<500; ++i)
    //for (; (bridge.RWDEFIsPanelAppLaunched)() != 0;)
    {
        (bridge.RWDEFIdle)();
        carla_msleep(20);
    }

    (bridge.RWDEFQuitPanelApp)();
#endif

    for (; (bridge.RWDEFIsCloseOK)() == 0;)
        carla_msleep(10);

    (bridge.RWDEFCloseDevice)();

    return 0;
}
