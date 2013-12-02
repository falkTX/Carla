/*
 * ANSI pedantic test for the Carla Backend API
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBackend.h"
#include "CarlaHost.h"

#include <stdio.h>

int main(int argc, char* argv[])
{
    ParameterData a;
    ParameterRanges b;
    MidiProgramData c;
    CustomData d;
    EngineDriverDeviceInfo e;

    CarlaPluginInfo f;
    CarlaNativePluginInfo g;
    CarlaPortCountInfo h;
    CarlaParameterInfo i;
    CarlaScalePointInfo j;
    CarlaTransportInfo k;

    const char* licenseText;
    const char* fileExtensions;

    licenseText = carla_get_complete_license_text();
    printf("LICENSE:\n%s\n", licenseText);

    fileExtensions = carla_get_supported_file_extensions();
    printf("FILE EXTENSIONS:\n%s\n", fileExtensions);

    return 0;

    /* unused */
    (void)argc;
    (void)argv;
    (void)a; (void)b; (void)c; (void)d; (void)e;
    (void)f; (void)g; (void)h; (void)i; (void)j; (void)k;
}
