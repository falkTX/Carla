/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngine.hpp"
#include "CarlaEngineThread.hpp"
#include "CarlaPlugin.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

CarlaEngineThread::CarlaEngineThread(CarlaEngine* const engine) noexcept
    : CarlaThread("CarlaEngineThread"),
      kEngine(engine),
      leakDetector()
{
    CARLA_SAFE_ASSERT(engine != nullptr);
    carla_debug("CarlaEngineThread::CarlaEngineThread(%p)", engine);
}

CarlaEngineThread::~CarlaEngineThread() noexcept
{
    carla_debug("CarlaEngineThread::~CarlaEngineThread()");
}

// -----------------------------------------------------------------------

void CarlaEngineThread::run() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(kEngine != nullptr,);
    CARLA_SAFE_ASSERT(kEngine->isRunning());
    carla_debug("CarlaEngineThread::run()");

    bool hasUI, oscRegisted, needsSingleThread;
    float value;

    for (; kEngine->isRunning() && ! shouldThreadExit();)
    {
#ifdef BUILD_BRIDGE
        oscRegisted = kEngine->isOscBridgeRegistered();
#else
        oscRegisted = kEngine->isOscControlRegistered();
#endif

        for (uint i=0, count = kEngine->getCurrentPluginCount(); i < count; ++i)
        {
            CarlaPlugin* const plugin(kEngine->getPluginUnchecked(i));

            CARLA_SAFE_ASSERT_CONTINUE(plugin != nullptr && plugin->isEnabled());
            CARLA_SAFE_ASSERT_UINT2(i == plugin->getId(), i, plugin->getId());

            hasUI             = (plugin->getHints() & PLUGIN_HAS_CUSTOM_UI);
            needsSingleThread = (plugin->getHints() & PLUGIN_NEEDS_SINGLE_THREAD);

            // -----------------------------------------------------------
            // Process postponed events

            if (oscRegisted || ! needsSingleThread)
            {
                if (! needsSingleThread)
                {
                    try {
                        plugin->postRtEventsRun();
                    } CARLA_SAFE_EXCEPTION("postRtEventsRun()")
                }

                if (oscRegisted || (hasUI && ! needsSingleThread))
                {
                    // ---------------------------------------------------
                    // Update parameter outputs

                    for (uint32_t j=0, pcount=plugin->getParameterCount(); j < pcount; ++j)
                    {
                        if (! plugin->isParameterOutput(j))
                            continue;

                        value = plugin->getParameterValue(j);

                        // Update OSC engine client
                        if (oscRegisted)
                        {
#ifdef BUILD_BRIDGE
                            kEngine->oscSend_bridge_parameter_value(j, value);
#else
                            kEngine->oscSend_control_set_parameter_value(i, static_cast<int32_t>(j), value);
#endif
                        }

                        // Update UI
                        if (hasUI && ! needsSingleThread)
                            plugin->uiParameterChange(j, value);
                    }
                }

#ifndef BUILD_BRIDGE
                // -------------------------------------------------------
                // Update OSC control client peaks

                if (oscRegisted)
                    kEngine->oscSend_control_set_peaks(i);
#endif
            }
        }

        carla_msleep(25);
    }
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
