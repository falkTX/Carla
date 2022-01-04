/*
 * Carla VST3 Plugin
 * Copyright (C) 2014-2021 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"
#include "AppConfig.h"

#if defined(USING_JUCE) && JUCE_PLUGINHOST_VST3
# define USE_JUCE_FOR_VST3
#endif

#include "CarlaVst3Utils.hpp"

#include "CarlaPluginUI.hpp"

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

class CarlaPluginVST3 : public CarlaPlugin,
                        private CarlaPluginUI::Callback
{
public:
    CarlaPluginVST3(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fUI()
    {
        carla_debug("CarlaPluginVST3::CarlaPluginVST3(%p, %i)", engine, id);
    }

    ~CarlaPluginVST3() override
    {
        carla_debug("CarlaPluginVST3::~CarlaPluginVST3()");

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_VST3;
    }

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    // -------------------------------------------------------------------
    // Set ui stuff

    // -------------------------------------------------------------------
    // Plugin state

    // -------------------------------------------------------------------
    // Plugin processing

    // -------------------------------------------------------------------
    // Plugin buffers

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    // nothing

    // -------------------------------------------------------------------

protected:
    void handlePluginUIClosed() override
    {
        // CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_debug("CarlaPluginVST3::handlePluginUIClosed()");

        showCustomUI(false);
        pData->engine->callback(true, true,
                                ENGINE_CALLBACK_UI_STATE_CHANGED,
                                pData->id,
                                0,
                                0, 0, 0.0f, nullptr);
    }

    void handlePluginUIResized(const uint width, const uint height) override
    {
        // CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_debug("CarlaPluginVST3::handlePluginUIResized(%u, %u)", width, height);

        return; // unused
        (void)width; (void)height;
    }

private:
    struct UI {
        bool isEmbed;
        bool isOpen;
        bool isVisible;
        CarlaPluginUI* window;

        UI() noexcept
            : isEmbed(false),
              isOpen(false),
              isVisible(false),
              window(nullptr) {}

        ~UI()
        {
            CARLA_ASSERT(isEmbed || ! isVisible);

            if (window != nullptr)
            {
                delete window;
                window = nullptr;
            }
        }

        CARLA_DECLARE_NON_COPY_STRUCT(UI);
    } fUI;
};

// --------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newVST3(const Initializer& init)
{
    carla_debug("CarlaPlugin::newVST3({%p, \"%s\", \"%s\", " P_INT64 "})",
                init.engine, init.filename, init.name, init.uniqueId);

#ifdef USE_JUCE_FOR_VST3
    if (std::getenv("CARLA_DO_NOT_USE_JUCE_FOR_VST3") == nullptr)
        return newJuce(init, "VST3");
#endif

    init.engine->setLastError("VST3 support not available");
    return nullptr;

    /*
    std::shared_ptr<CarlaPluginVST2> plugin(new CarlaPluginVST2(init.engine, init.id));

    if (! plugin->init(plugin, init.filename, init.name, init.uniqueId, init.options))
        return nullptr;

    return plugin;
    */
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
