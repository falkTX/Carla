/*
 * Carla Native Plugins
 * Copyright (C) 2013-2018 Filipe Coelho <falktx@falktx.com>
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
#include "vestige/vestige.h"

#ifdef __WINE__
#error This file is not meant to be used by wine!
#endif
#ifndef CARLA_OS_WIN
#error This file is only meant to be used by mingw compilers!
#endif
#ifndef CARLA_PLUGIN_SYNTH
#error CARLA_PLUGIN_SYNTH undefined
#endif

typedef const AEffect* (__cdecl *MainCallback)(audioMasterCallback);

static HINSTANCE currentModuleHandle = nullptr;

HINSTANCE getCurrentModuleInstanceHandle() noexcept
{
    if (currentModuleHandle == nullptr)
        currentModuleHandle = GetModuleHandleA(nullptr);

    return currentModuleHandle;
}

CARLA_EXPORT
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD, LPVOID)
{
    currentModuleHandle = hInst;
    return 1;
}

CARLA_EXPORT __cdecl
const AEffect* VSTPluginMain(audioMasterCallback audioMaster)
{
    static MainCallback sCallback = nullptr;

    if (sCallback == nullptr)
    {
        CHAR filename[MAX_PATH + 256];
        filename[0] = 0;
        GetModuleFileName(getCurrentModuleInstanceHandle(), filename, MAX_PATH + 256);
        strcat(filename, ".so");

        carla_stdout("FILENAME: '%s'", filename);
        static const lib_t lib = lib_open(filename);
        if (lib == nullptr)
        {
            carla_stderr2("lib_open failed: %s", lib_error(filename));
            return nullptr;
        }

        sCallback = lib_symbol<MainCallback>(lib, "VSTPluginMain");
    }

    CARLA_SAFE_ASSERT_RETURN(sCallback != nullptr, nullptr);
    return sCallback(audioMaster);
}
