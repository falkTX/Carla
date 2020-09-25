/*
 * ANSI pedantic test for the Carla Backend & Host API
 * Copyright (C) 2013-2020 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaUtils.h"

#include "CarlaMIDI.h"
#include "CarlaNative.h"
#include "CarlaNativePlugin.h"

#ifdef __cplusplus
# include "CarlaEngine.hpp"
# include "CarlaPlugin.hpp"
# include <cassert>
# include <cstdio>
# ifndef nullptr
#  undef NULL
#  define NULL nullptr
# endif
#else
# include <assert.h>
# include <stdio.h>
#endif

#define BOOLEAN_AS_STRING(cond) ((cond) ? "true" : "false")

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
    CarlaHostHandle handle;

    const char* licenseText;
    const char* const* fileExtensions;

    uint l, count;

    licenseText = carla_get_complete_license_text();
    printf("LICENSE:\n%s\n", licenseText);

    fileExtensions = carla_get_supported_file_extensions();
    printf("FILE EXTENSIONS:\n ");
    for (l=0; fileExtensions[l] != NULL; ++l)
        printf(" %s", fileExtensions[l]);
    printf("\n");

    count = carla_get_engine_driver_count();
    printf("DRIVER COUNT: %i\n", count);

    for (l=0; l < count; ++l)
    {
        const char* driverName;
        const char* const* driverDeviceNames;
        const EngineDriverDeviceInfo* driverDeviceInfo;
        uint m, m2, count2;

        driverName = carla_get_engine_driver_name(l);
        driverDeviceNames = carla_get_engine_driver_device_names(l);
        printf("DRIVER %i/%i: \"%s\" : DEVICES:\n", l+1, count, driverName);

        count2 = 0;
        while (driverDeviceNames[count2] != NULL)
            ++count2;

        for (m = 0; m < count2; ++m)
        {
            driverDeviceInfo = carla_get_engine_driver_device_info(l, driverDeviceNames[m]);
            printf("DRIVER DEVICE %i/%i: \"%s\"\n", m+1, count2, driverDeviceNames[m]);
            printf("  Has control panel: %s\n", BOOLEAN_AS_STRING(driverDeviceInfo->hints & ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL));
            printf("  Can triple buffer: %s\n", BOOLEAN_AS_STRING(driverDeviceInfo->hints & ENGINE_DRIVER_DEVICE_CAN_TRIPLE_BUFFER));
            printf("  Variable buffer size: %s\n", BOOLEAN_AS_STRING(driverDeviceInfo->hints & ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE));
            printf("  Variable sample rate: %s\n", BOOLEAN_AS_STRING(driverDeviceInfo->hints & ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE));

            if (driverDeviceInfo->bufferSizes != NULL && driverDeviceInfo->bufferSizes[0] != 0)
            {
                printf("  Buffer sizes:");

                m2 = 0;
                while (driverDeviceInfo->bufferSizes[m2] != 0)
                    printf(" %i", driverDeviceInfo->bufferSizes[m2++]);

                printf("\n");
            }
            else
            {
                printf("  Buffer sizes: (null)\n");
            }

            if (driverDeviceInfo->sampleRates != NULL && driverDeviceInfo->sampleRates[0] > 0.1)
            {
                printf("  Sample rates:");

                m2 = 0;
                while (driverDeviceInfo->sampleRates[m2] > 0.1)
                    printf(" %.1f", driverDeviceInfo->sampleRates[m2++]);

                printf("\n");
            }
            else
            {
                printf("  Sample rates: (null)\n");
            }
        }
    }

    handle = carla_standalone_host_init();

    carla_set_engine_option(handle, ENGINE_OPTION_PROCESS_MODE, ENGINE_PROCESS_MODE_PATCHBAY, NULL);
    carla_set_engine_option(handle, ENGINE_OPTION_TRANSPORT_MODE, ENGINE_TRANSPORT_MODE_INTERNAL, NULL);

    if (carla_engine_init(handle, "Dummy", "ansi-test"))
    {
#ifdef __cplusplus
        CarlaEngine* const engine(carla_get_engine_from_handle(handle));
        assert(engine != nullptr);
        engine->getLastError();
#endif
        if (carla_add_plugin(handle, BINARY_NATIVE, PLUGIN_INTERNAL, NULL, NULL, "audiofile", 0, NULL, 0x0))
        {
#ifdef __cplusplus
            const std::shared_ptr<CarlaPlugin> plugin(engine->getPlugin(0));
            assert(plugin.get() != nullptr);
            plugin->getId();
#endif
            /* carla_set_custom_data(0, CUSTOM_DATA_TYPE_STRING, "file", "/home/falktx/Music/test.wav"); */
            carla_transport_play(handle);
        }
        else
        {
            printf("%s\n", carla_get_last_error(handle));
        }
#ifdef __cplusplus
        engine->setAboutToClose();
#endif
        carla_engine_close(handle);
    }

#ifdef __cplusplus
    EngineControlEvent e1;
    EngineMidiEvent e2;
    EngineEvent e3;
    e3.fillFromMidiData(0, nullptr, 0);
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
