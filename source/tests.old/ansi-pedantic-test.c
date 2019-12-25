/*
 * ANSI pedantic test for the Carla Backend & Host API
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

#include "CarlaBackend.h"
#include "CarlaHost.h"
#include "CarlaNative.h"
#include "CarlaMIDI.h"

#ifdef __cplusplus
# include "CarlaEngine.hpp"
# include "CarlaPlugin.hpp"
# include <cassert>
# include <cstdio>
# ifdef CARLA_PROPER_CPP11_SUPPORT
#  undef NULL
#  define NULL nullptr
# endif
#else
# include <assert.h>
# include <stdio.h>
#endif

int main(int argc, char* argv[])
{
#ifdef __cplusplus
    CARLA_BACKEND_USE_NAMESPACE
#endif

    ParameterData a;
    ParameterRanges b;
    MidiProgramData c;
    CustomData d;
    EngineDriverDeviceInfo e;

    CarlaPluginInfo f;
    /*CarlaCachedPluginInfo g;*/
    CarlaPortCountInfo h;
    CarlaParameterInfo i;
    CarlaScalePointInfo j;
    CarlaTransportInfo k;

    /*const char* licenseText;
    const char* fileExtensions;*/

    uint l, count;

    /*licenseText = carla_get_complete_license_text();
    printf("LICENSE:\n%s\n", licenseText);

    fileExtensions = carla_get_supported_file_extensions();
    printf("FILE EXTENSIONS:\n%s\n", fileExtensions);*/

    count = carla_get_engine_driver_count();
    printf("DRIVER COUNT: %i\n", count);

    for (l=0; l < count; ++l)
    {
        const char*        driverName;
        const char* const* driverDeviceNames;
        uint m, count2;

        driverName        = carla_get_engine_driver_name(l);
        driverDeviceNames = carla_get_engine_driver_device_names(l);
        printf("DRIVER %i/%i: \"%s\" : DEVICES:\n", l+1, count, driverName);

        count2 = 0;
        while (driverDeviceNames[count2] != NULL)
            ++count2;

        for (m = 0; m < count2; ++m)
        {
            printf("DRIVER DEVICE %i/%i: \"%s\"\n", m+1, count2, driverDeviceNames[m]);
        }
    }

    if (carla_engine_init("JACK", "ansi-test"))
    {
#ifdef __cplusplus
        CarlaEngine* const engine(carla_get_engine());
        assert(engine != nullptr);
        engine->getLastError();
#endif
        if (carla_add_plugin(BINARY_NATIVE, PLUGIN_INTERNAL, NULL, NULL, "audiofile", 0, NULL, 0x0))
        {
#ifdef __cplusplus
            CarlaPlugin* const plugin(engine->getPlugin(0));
            assert(plugin != nullptr);
            plugin->getId();
#endif
            carla_set_custom_data(0, CUSTOM_DATA_TYPE_STRING, "file", "/home/falktx/Music/test.wav");
            carla_transport_play();
        }
        else
        {
            printf("%s\n", carla_get_last_error());
        }
#ifdef __cplusplus
        engine->setAboutToClose();
#endif
        carla_engine_close();
    }

#ifdef __cplusplus
    EngineControlEvent e1;
    EngineMidiEvent e2;
    EngineEvent e3;
    e3.fillFromMidiData(0, nullptr);
    EngineOptions e4;
    EngineTimeInfoBBT e5;
    EngineTimeInfo e6;
#endif

    /* unused C */
    (void)argc;
    (void)argv;
    (void)a; (void)b; (void)c; (void)d; (void)e;
    (void)f; /*(void)g;*/ (void)h; (void)i; (void)j; (void)k;
#ifdef __cplusplus
    (void)e1; (void)e2; (void)e4; (void)e5; (void)e6;
#endif

    return 0;
}
