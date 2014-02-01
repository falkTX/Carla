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

typedef void (*Fn_RWDEFCloseDevice)();
typedef void (*Fn_RWDEFDriveAudio)(void* ins, void* outs);
typedef void (*Fn_RWDEFGetDeviceInfo)(void* info);
typedef void (*Fn_RWDEFGetDeviceNameAndVersion)(long* version, char* name);
typedef void (*Fn_RWDEFGetEventBusInfo)(ushort index, void* info);
typedef void (*Fn_RWDEFGetEventChannelInfo)(void* v1, void* v2);
typedef void (*Fn_RWDEFGetEventControllerInfo)(void* v1, ushort index, void* v2);
typedef void (*Fn_RWDEFGetEventInfo)(void* info);
typedef void (*Fn_RWDEFGetEventNoteInfo)(void* v1, ushort index, void* v2);
typedef void (*Fn_RWDEFIdle)();
typedef char (*Fn_RWDEFIsCloseOK)();
typedef char (*Fn_RWDEFIsPanelAppLaunched)();
typedef int  (*Fn_RWDEFLaunchPanelApp)();
typedef int  (*Fn_RWDEFOpenDevice)(void* info);
typedef int  (*Fn_RWDEFQuitPanelApp)();
typedef void (*Fn_RWDEFSetAudioInfo)(void* info);

// -----------------------------------------------------------------------------

struct RewireBridge {
    void* lib;

    Fn_RWDEFCloseDevice RWDEFCloseDevice;
    Fn_RWDEFDriveAudio RWDEFDriveAudio;
    Fn_RWDEFGetDeviceInfo RWDEFGetDeviceInfo;
    Fn_RWDEFGetDeviceNameAndVersion RWDEFGetDeviceNameAndVersion;
    Fn_RWDEFGetEventBusInfo RWDEFGetEventBusInfo;
    Fn_RWDEFGetEventChannelInfo RWDEFGetEventChannelInfo;
    Fn_RWDEFGetEventControllerInfo RWDEFGetEventControllerInfo;
    Fn_RWDEFGetEventInfo RWDEFGetEventInfo;
    Fn_RWDEFGetEventNoteInfo RWDEFGetEventNoteInfo;
    Fn_RWDEFIdle RWDEFIdle;
    Fn_RWDEFIsCloseOK RWDEFIsCloseOK;
    Fn_RWDEFIsPanelAppLaunched RWDEFIsPanelAppLaunched;
    Fn_RWDEFLaunchPanelApp RWDEFLaunchPanelApp;
    Fn_RWDEFOpenDevice RWDEFOpenDevice;
    Fn_RWDEFQuitPanelApp RWDEFQuitPanelApp;
    Fn_RWDEFSetAudioInfo RWDEFSetAudioInfo;

    RewireBridge(const char* const filename)
        : lib(nullptr),
          RWDEFCloseDevice(nullptr),
          RWDEFDriveAudio(nullptr),
          RWDEFGetDeviceInfo(nullptr),
          RWDEFGetDeviceNameAndVersion(nullptr),
          RWDEFGetEventBusInfo(nullptr),
          RWDEFGetEventChannelInfo(nullptr),
          RWDEFGetEventControllerInfo(nullptr),
          RWDEFGetEventInfo(nullptr),
          RWDEFGetEventNoteInfo(nullptr),
          RWDEFIdle(nullptr),
          RWDEFIsCloseOK(nullptr),
          RWDEFIsPanelAppLaunched(nullptr),
          RWDEFLaunchPanelApp(nullptr),
          RWDEFOpenDevice(nullptr),
          RWDEFQuitPanelApp(nullptr),
          RWDEFSetAudioInfo(nullptr)
    {
        lib = lib_open(filename);

        if (lib == nullptr)
        {
            fprintf(stderr, "Failed to load DLL, reason:\n%s\n", lib_error(filename));
            return;
        }
        else
        {
            fprintf(stdout, "loaded sucessfully!\n");
        }

        #define JOIN(a, b) a ## b
        #define LIB_SYMBOL(NAME) NAME = (Fn_##NAME)lib_symbol(lib, #NAME);

        LIB_SYMBOL(RWDEFCloseDevice)
        LIB_SYMBOL(RWDEFDriveAudio)
        LIB_SYMBOL(RWDEFGetDeviceInfo)
        LIB_SYMBOL(RWDEFGetDeviceNameAndVersion)
        LIB_SYMBOL(RWDEFGetEventBusInfo)
        LIB_SYMBOL(RWDEFGetEventChannelInfo)
        LIB_SYMBOL(RWDEFGetEventControllerInfo)
        LIB_SYMBOL(RWDEFGetEventInfo)
        LIB_SYMBOL(RWDEFGetEventNoteInfo)
        LIB_SYMBOL(RWDEFIdle)
        LIB_SYMBOL(RWDEFIsCloseOK)
        LIB_SYMBOL(RWDEFIsPanelAppLaunched)
        LIB_SYMBOL(RWDEFLaunchPanelApp)
        LIB_SYMBOL(RWDEFOpenDevice)
        LIB_SYMBOL(RWDEFQuitPanelApp)
        LIB_SYMBOL(RWDEFSetAudioInfo)

        #undef JOIN
        #undef LIB_SYMBOL
    }

    ~RewireBridge()
    {
        if (lib != nullptr)
        {
            lib_close(lib);
            lib = nullptr;
        }
    }
};

// -----------------------------------------------------------------------

int main(/*int argc, char* argv[]*/)
{
    //static const char* const filename = "C:\\Program Files\\Waves\\ReWire\\WavesReWireDevice.dll";
    static const char* const filename = "C:\\Program Files\\AudioGL\\AudioGL.dll";

    RewireBridge bridge(filename);

    struct OpenInfo {
        ulong size1; // ??
        ulong size2; // 12
        long sampleRate;
        long bufferSize;
    };
    OpenInfo info;
    info.size1 = sizeof(OpenInfo);
    info.size2 = 12;
    info.sampleRate = 44100;
    info.bufferSize = 512;

    (bridge.RWDEFOpenDevice)(&info);

    long version;
    char name[256];
    carla_zeroChar(name, 256);
    (bridge.RWDEFGetDeviceNameAndVersion)(&version, name);

    carla_stdout("Ok, this is the info:");
    carla_stdout("\tVersion: %i", version);
    carla_stdout("\tName:    \"%s\"", name);

    carla_stdout("Starting panel...");
    (bridge.RWDEFLaunchPanelApp)();

    for (int i=0; i<500; ++i)
    //for (; (bridge.RWDEFIsPanelAppLaunched)() != 0;)
    {
        (bridge.RWDEFIdle)();
        carla_msleep(20);
    }

    (bridge.RWDEFQuitPanelApp)();
    (bridge.RWDEFCloseDevice)();

    return 0;
}
