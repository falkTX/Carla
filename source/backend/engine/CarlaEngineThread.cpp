/*
 * Carla Engine Thread
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngineThread.hpp"

#include "CarlaEngine.hpp"
#include "CarlaPlugin.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

CarlaEngineThread::CarlaEngineThread(CarlaEngine* const engine)
    : Thread("CarlaEngineThread"),
      fEngine(engine)
{
    CARLA_ASSERT(engine != nullptr);
    carla_debug("CarlaEngineThread::CarlaEngineThread(%p)", engine);

    setPriority(5);
}

CarlaEngineThread::~CarlaEngineThread()
{
    carla_debug("CarlaEngineThread::~CarlaEngineThread()");
}

// -----------------------------------------------------------------------

void CarlaEngineThread::run()
{
    CARLA_ASSERT(fEngine->isRunning());
    carla_debug("CarlaEngineThread::run()");

    bool oscRegisted, needsSingleThread;
    unsigned int i, count;
    float value;

    while (fEngine->isRunning() && ! threadShouldExit())
    {
#ifdef BUILD_BRIDGE
        oscRegisted = fEngine->isOscBridgeRegistered();
#else
        oscRegisted = fEngine->isOscControlRegistered();
#endif

        for (i=0, count = fEngine->getCurrentPluginCount(); i < count; ++i)
        {
            CarlaPlugin* const plugin(fEngine->getPluginUnchecked(i));

            if (plugin == nullptr || ! plugin->isEnabled())
                continue;

            CARLA_SAFE_ASSERT_INT2(i == plugin->getId(), i, plugin->getId());

            needsSingleThread = (plugin->getHints() & PLUGIN_NEEDS_SINGLE_THREAD);

            // -----------------------------------------------------------
            // Process postponed events

            if (oscRegisted || ! needsSingleThread)
            {
                if (! needsSingleThread)
                    plugin->postRtEventsRun();

                // -------------------------------------------------------
                // Update parameter outputs

                for (uint32_t j=0; j < plugin->getParameterCount(); ++j)
                {
                    if (! plugin->isParameterOutput(j))
                        continue;

                    value = plugin->getParameterValue(j);

                    // Update UI
                    if (! needsSingleThread)
                        plugin->uiParameterChange(j, value);

                    // Update OSC engine client
                    if (oscRegisted)
                    {
#ifdef BUILD_BRIDGE
                        fEngine->oscSend_bridge_set_parameter_value(j, value);
#else
                        fEngine->oscSend_control_set_parameter_value(i, j, value);
#endif
                    }
                }

#ifndef BUILD_BRIDGE
                // ---------------------------------------------------
                // Update OSC control client peaks

                if (oscRegisted)
                    fEngine->oscSend_control_set_peaks(i);
#endif
            }
        }

        fEngine->idleOsc();
        sleep(oscRegisted ? 30 : 50);
    }
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
