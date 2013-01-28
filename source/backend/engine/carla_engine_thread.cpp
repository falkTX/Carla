/*
 * Carla Engine Thread
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "carla_engine.hpp"
#include "carla_engine_thread.hpp"
#include "carla_plugin.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

CarlaEngineThread::CarlaEngineThread(CarlaEngine* const engine, QObject* const parent)
    : QThread(parent),
      fEngine(engine)
{
    qDebug("CarlaEngineThread::CarlaEngineThread(%p, %p)", engine, parent);
    CARLA_ASSERT(engine);

    fStopNow = true;
}

CarlaEngineThread::~CarlaEngineThread()
{
    qDebug("CarlaEngineThread::~CarlaEngineThread()");
    CARLA_ASSERT(fStopNow);
}

// -----------------------------------------------------------------------

void CarlaEngineThread::startNow()
{
    qDebug("CarlaEngineThread::startNow()");
    CARLA_ASSERT(fStopNow);

    fStopNow = false;

    start(QThread::HighPriority);
}

void CarlaEngineThread::stopNow()
{
    qDebug("CarlaEngineThread::stopNow()");

    if (fStopNow)
        return;

    fStopNow = true;

    const CarlaMutex::ScopedLocker sl(&fMutex);

    if (isRunning() && ! wait(200))
    {
        quit();

        if (isRunning() && ! wait(300))
            terminate();
    }
}

// -----------------------------------------------------------------------

void CarlaEngineThread::run()
{
    qDebug("CarlaEngineThread::run()");
    CARLA_ASSERT(fEngine->isRunning());

    bool oscRegisted, usesSingleThread;
    int i, count;
    double value;

    while (fEngine->isRunning() && ! fStopNow)
    {
        const CarlaMutex::ScopedLocker sl(&fMutex);

#ifndef BUILD_BRIDGE
        oscRegisted = fEngine->isOscControlRegistered();
#else
        oscRegisted = fEngine->isOscBridgeRegistered();
#endif

        for (i=0, count = fEngine->currentPluginCount(); i < count; i++)
        {
            CarlaPlugin* const plugin = fEngine->getPluginUnchecked(i);

#if 0
            if (! (plugin && plugin->enabled()))
                continue;

            CARLA_ASSERT(i == plugin->id());

            usesSingleThread = (plugin->hints() & PLUGIN_USES_SINGLE_THREAD);

            // -------------------------------------------------------
            // Process postponed events

            if (! usesSingleThread)
                plugin->postEventsRun();

            if (oscRegisted || ! usesSingleThread)
            {
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
                        fEngine->osc_send_bridge_set_parameter_value(j, value);
#else
                        fEngine->osc_send_control_set_parameter_value(i, j, value);
#endif
                    }
                }

                // ---------------------------------------------------
                // Update OSC control client peaks

                if (oscRegisted)
                {
#ifdef BUILD_BRIDGE
                    fEngine->osc_send_peaks(plugin);
#else
                    fEngine->osc_send_peaks(plugin, i);
#endif
                }
            }
#endif
        }

        fEngine->idleOsc();
        msleep(oscRegisted ? 40 : 50);
    }
}

CARLA_BACKEND_END_NAMESPACE
