/*
 * Carla Plugin Host
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "water/misc/Time.h"

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
    carla_debug("CarlaEngineThread::run()");

    const bool kIsPlugin        = kEngine->getType() == kEngineTypePlugin;
    const bool kIsAlwaysRunning = kEngine->getType() == kEngineTypeBridge || kIsPlugin;

    float value;

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
    // int64_t lastPingTime = 0;
    const CarlaEngineOsc& engineOsc(kEngine->pData->osc);
#endif

    // thread must do something...
    CARLA_SAFE_ASSERT_RETURN(kIsAlwaysRunning || kEngine->isRunning(),);

    for (; (kIsAlwaysRunning || kEngine->isRunning()) && ! shouldThreadExit();)
    {
#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
        const bool oscRegistedForUDP = engineOsc.isControlRegisteredForUDP();
#else
        const bool oscRegistedForUDP = false;
#endif

#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (kIsPlugin)
            engineOsc.idle();
#endif

        for (uint i=0, count = kEngine->getCurrentPluginCount(); i < count; ++i)
        {
            const CarlaPluginPtr plugin = kEngine->getPluginUnchecked(i);

            CARLA_SAFE_ASSERT_CONTINUE(plugin.get() != nullptr && plugin->isEnabled());
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

            if (oscRegistedForUDP || updateUI)
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
                    if (oscRegistedForUDP)
                        engineOsc.sendParameterValue(i, j, value);
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

            if (oscRegistedForUDP)
                engineOsc.sendPeaks(i, kEngine->getPeaks(i));
#endif
        }

#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
        if (oscRegistedForUDP)
            engineOsc.sendRuntimeInfo();

        /*
        if (engineOsc.isControlRegisteredForTCP())
        {
            const int64_t timeNow = water::Time::currentTimeMillis();

            if (timeNow - lastPingTime > 1000)
            {
                engineOsc.sendPing();
                lastPingTime = timeNow;
            }
        }
        */
#endif

        carla_msleep(25);
    }

    carla_debug("CarlaEngineThread closed");
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
