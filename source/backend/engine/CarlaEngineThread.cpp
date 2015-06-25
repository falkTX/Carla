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
      kEngine(engine)
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
#ifndef BUILD_BRIDGE
    CARLA_SAFE_ASSERT(kEngine->isRunning());
#endif
    carla_debug("CarlaEngineThread::run()");

#ifdef HAVE_LIBLO
    const bool isPlugin(kEngine->getType() == kEngineTypePlugin);
#endif
    float value;

#ifdef BUILD_BRIDGE
    for (; /*kEngine->isRunning() &&*/ ! shouldThreadExit();)
#else
    for (; kEngine->isRunning() && ! shouldThreadExit();)
#endif
    {
#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
        const bool oscRegisted = kEngine->isOscControlRegistered();
#else
        const bool oscRegisted = false;
#endif

#ifdef HAVE_LIBLO
        if (isPlugin)
            kEngine->idleOsc();
#endif

        for (uint i=0, count = kEngine->getCurrentPluginCount(); i < count; ++i)
        {
            CarlaPlugin* const plugin(kEngine->getPluginUnchecked(i));

            CARLA_SAFE_ASSERT_CONTINUE(plugin != nullptr && plugin->isEnabled());
            CARLA_SAFE_ASSERT_UINT2(i == plugin->getId(), i, plugin->getId());

            const uint hints(plugin->getHints());
            const bool updateUI((hints & PLUGIN_HAS_CUSTOM_UI) != 0 && (hints & PLUGIN_NEEDS_UI_MAIN_THREAD) == 0);

            // -----------------------------------------------------------
            // DSP Idle

            try {
                plugin->idle();
            } CARLA_SAFE_EXCEPTION("idle()")

            // -----------------------------------------------------------
            // Post-poned events

            if (oscRegisted || updateUI)
            {
                // -------------------------------------------------------
                // Update parameter outputs

                for (uint32_t j=0, pcount=plugin->getParameterCount(); j < pcount; ++j)
                {
                    if (! plugin->isParameterOutput(j))
                        continue;

                    value = plugin->getParameterValue(j);

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
                    // Update OSC engine client
                    if (oscRegisted)
                        kEngine->oscSend_control_set_parameter_value(i, static_cast<int32_t>(j), value);
#endif
                    // Update UI
                    if (updateUI)
                        plugin->uiParameterChange(j, value);
                }

                if (updateUI)
                {
                    try {
                        plugin->uiIdle();
                    } CARLA_SAFE_EXCEPTION("uiIdle()")
                }
            }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
            // -----------------------------------------------------------
            // Update OSC control client peaks

            if (oscRegisted)
                kEngine->oscSend_control_set_peaks(i);
#endif
        }

        carla_msleep(25);
    }
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
