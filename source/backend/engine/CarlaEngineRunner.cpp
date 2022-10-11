/*
 * Carla Plugin Host
 * Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngineRunner.hpp"
#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "water/misc/Time.h"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

CarlaEngineRunner::CarlaEngineRunner(CarlaEngine* const engine) noexcept
    : CarlaRunner("CarlaEngineRunner"),
      kEngine(engine),
      fEngineHasIdleOnMainThread(false),
      fIsAlwaysRunning(false),
      fIsPlugin(false)
{
    CARLA_SAFE_ASSERT(engine != nullptr);
    carla_debug("CarlaEngineRunner::CarlaEngineRunner(%p)", engine);
}

CarlaEngineRunner::~CarlaEngineRunner() noexcept
{
    carla_debug("CarlaEngineRunner::~CarlaEngineRunner()");
}

void CarlaEngineRunner::start()
{
    carla_debug("CarlaEngineRunner::start()");
    if (isRunnerActive())
        stopRunner();

    fEngineHasIdleOnMainThread = kEngine->hasIdleOnMainThread();
    fIsPlugin = kEngine->getType() == kEngineTypePlugin;
    fIsAlwaysRunning = kEngine->getType() == kEngineTypeBridge || fIsPlugin;

    startRunner(25);
}

void CarlaEngineRunner::stop()
{
    carla_debug("CarlaEngineRunner::stop()");
    stopRunner();
}

// -----------------------------------------------------------------------

bool CarlaEngineRunner::run() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(kEngine != nullptr, false);

    float value;

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
    // int64_t lastPingTime = 0;
    const CarlaEngineOsc& engineOsc(kEngine->pData->osc);
#endif

    // runner must do something...
    CARLA_SAFE_ASSERT_RETURN(fIsAlwaysRunning || kEngine->isRunning(), false);

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
    const bool oscRegistedForUDP = engineOsc.isControlRegisteredForUDP();
#else
    const bool oscRegistedForUDP = false;
#endif

#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
    if (fIsPlugin)
        engineOsc.idle();
#endif

    for (uint i=0, count = kEngine->getCurrentPluginCount(); i < count; ++i)
    {
        const CarlaPluginPtr plugin = kEngine->getPluginUnchecked(i);

        CARLA_SAFE_ASSERT_CONTINUE(plugin.get() != nullptr && plugin->isEnabled());
        CARLA_SAFE_ASSERT_UINT2(i == plugin->getId(), i, plugin->getId());

        const uint hints = plugin->getHints();
        const bool useIdle = (hints & PLUGIN_NEEDS_MAIN_THREAD_IDLE) == 0 || !fEngineHasIdleOnMainThread;
        const bool updateUI = (hints & PLUGIN_HAS_CUSTOM_UI) != 0 && (hints & PLUGIN_NEEDS_UI_MAIN_THREAD) == 0;

        // -----------------------------------------------------------
        // DSP Idle

        if (useIdle)
        {
            try {
                plugin->idle();
            } CARLA_SAFE_EXCEPTION("idle()")
        }

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

    return true;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
