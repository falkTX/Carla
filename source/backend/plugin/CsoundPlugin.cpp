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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaPluginInternal.hpp"

#define WANT_CSOUND 1
#ifdef WANT_CSOUND

//#include "CarlaVstUtils.hpp"

CARLA_BACKEND_START_NAMESPACE

#if 0
}
#endif

class CsoundPlugin : public CarlaPlugin
{
public:
    CsoundPlugin(CarlaEngine* const engine, const unsigned short id)
        : CarlaPlugin(engine, id)
    {
        carla_debug("CsoundPlugin::CsoundPlugin(%p, %i)", engine, id);
    }

    ~CsoundPlugin() override
    {
        carla_debug("CsoundPlugin::~CsoundPlugin()");

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_CSOUND;
    }

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        carla_debug("CsoundPlugin::reload() - start");

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        carla_debug("CsoundPlugin::reload() - end");
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames) override
    {
    }

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CsoundPlugin)
};

CARLA_BACKEND_END_NAMESPACE

#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newCSOUND(const Initializer& init)
{
    carla_debug("CarlaPlugin::newCsound(%p, \"%s\", \"%s\", \"%s\")", init.engine, init.filename, init.name, init.label);

#ifdef WANT_CSOUND
    CsoundPlugin* const plugin(new CsoundPlugin(init.engine, init.id));

    //if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && ! plugin->canRunInRack())
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

CARLA_BACKEND_END_NAMESPACE
