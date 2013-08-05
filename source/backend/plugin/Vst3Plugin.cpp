/*
 * Carla VST Plugin
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaPluginInternal.hpp"

#ifdef WANT_VST3

//#include "CarlaVstUtils.hpp"

CARLA_BACKEND_START_NAMESPACE

class Vst3Plugin : public CarlaPlugin
{
public:
    Vst3Plugin(CarlaEngine* const engine, const unsigned short id)
        : CarlaPlugin(engine, id)
    {
        carla_debug("Vst3Plugin::Vst3Plugin(%p, %i)", engine, id);
    }

    ~Vst3Plugin() override
    {
        carla_debug("Vst3Plugin::~Vst3Plugin()");

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType type() const override
    {
        return PLUGIN_VST3;
    }

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Vst3Plugin)
};

CARLA_BACKEND_END_NAMESPACE

#else // WANT_VST3
// no point on warning when the plugin doesn't even work yet
// # warning Building without VST3 support
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
CarlaPlugin* CarlaPlugin::newVST3(const Initializer& init)
{
    carla_debug("CarlaPlugin::newVST3(%p, \"%s\", \"%s\", \"%s\")", init.engine, init.filename, init.name, init.label);

#ifdef WANT_VST3
    Vst3Plugin* const plugin = new Vst3Plugin(init.engine, init.id);

    //if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && ! CarlaPluginProtectedData::canRunInRack(plugin))
    {
        init.engine->setLastError("Carla's rack mode can only work with Stereo VST3 plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("VST3 support not available");
    return nullptr;
#endif
}
#endif

CARLA_BACKEND_END_NAMESPACE
