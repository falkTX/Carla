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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaEngineThread.hpp"

#include "CarlaEngine.hpp"
#include "CarlaPlugin.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

CarlaEngineThread::CarlaEngineThread(CarlaEngine* const engine)
    : kEngine(engine),
      fStopNow(true)
{
    carla_debug("CarlaEngineThread::CarlaEngineThread(%p)", engine);
    CARLA_ASSERT(engine);
}

CarlaEngineThread::~CarlaEngineThread()
{
    carla_debug("CarlaEngineThread::~CarlaEngineThread()");
    CARLA_ASSERT(fStopNow);
}

// -----------------------------------------------------------------------

void CarlaEngineThread::startNow()
{
    carla_debug("CarlaEngineThread::startNow()");
    CARLA_ASSERT(fStopNow);

    fStopNow = false;
    start();
}

void CarlaEngineThread::stopNow()
{
    carla_debug("CarlaEngineThread::stopNow()");

    if (fStopNow)
        return;

    fStopNow = true;

    const CarlaMutex::ScopedLocker sl(&fMutex);

    if (isRunning() && ! stop(500))
        terminate();
}

// -----------------------------------------------------------------------

void CarlaEngineThread::run()
{
    carla_debug("CarlaEngineThread::run()");
    CARLA_ASSERT(kEngine->isRunning());

    bool oscRegisted, usesSingleThread;
    unsigned int i, count;
    float value;

    while (kEngine->isRunning() && ! fStopNow)
    {
        const CarlaMutex::ScopedLocker sl(&fMutex);

#ifdef BUILD_BRIDGE
        oscRegisted = kEngine->isOscBridgeRegistered();
#else
        oscRegisted = kEngine->isOscControlRegistered();
#endif

        for (i=0, count = kEngine->currentPluginCount(); i < count; i++)
        {
            CarlaPlugin* const plugin = kEngine->getPluginUnchecked(i);

            if (plugin == nullptr || ! plugin->enabled())
                continue;

            CARLA_SAFE_ASSERT_INT2(i == plugin->id(), i, plugin->id());

            usesSingleThread = (plugin->hints() & PLUGIN_USES_SINGLE_THREAD);

            // -------------------------------------------------------
            // Process postponed events

            if (oscRegisted || ! usesSingleThread)
            {
                if (! usesSingleThread)
                    plugin->postRtEventsRun();

                // ---------------------------------------------------
                // Update parameter outputs

                for (uint32_t j=0; j < plugin->parameterCount(); j++)
                {
                    if (! plugin->parameterIsOutput(j))
                        continue;

                    value = plugin->getParameterValue(j);

                    // Update UI
                    if (! usesSingleThread)
                        plugin->uiParameterChange(j, value);

                    // Update OSC engine client
                    if (oscRegisted)
                    {
#ifdef BUILD_BRIDGE
                        kEngine->osc_send_bridge_set_parameter_value(j, value);
#else
                        kEngine->osc_send_control_set_parameter_value(i, j, value);
#endif
                    }
                }

                // ---------------------------------------------------
                // Update OSC control client peaks

                if (oscRegisted)
                {
#ifdef BUILD_BRIDGE
                    kEngine->osc_send_bridge_set_peaks();
#else
                    kEngine->osc_send_control_set_peaks(i);
#endif
                }
            }
        }

        kEngine->idleOsc();
        carla_msleep(oscRegisted ? 40 : 50);
    }
}

CARLA_BACKEND_END_NAMESPACE
