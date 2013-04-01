/*
 * Carla Native Plugins
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNative.hpp"

#include "CarlaEngine.hpp"
#include "CarlaEngineInternal.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

class CarlaEngineNative : public CarlaEngine,
                          public PluginDescriptorClass
{
public:
    CarlaEngineNative(const HostDescriptor* const host)
        : CarlaEngine(),
          PluginDescriptorClass(host)
    {
        carla_debug("CarlaEngineNative::CarlaEngineNative()");

        // set-up engine
        fOptions.processMode   = PROCESS_MODE_CONTINUOUS_RACK;
        fOptions.transportMode = TRANSPORT_MODE_INTERNAL;
        fOptions.forceStereo   = true;
        fOptions.preferPluginBridges = false;
        fOptions.preferUiBridges = false;
        init("Carla");
    }

    ~CarlaEngineNative()
    {
        carla_debug("CarlaEngineNative::~CarlaEngineNative()");

        setAboutToClose();
        removeAllPlugins();
        close();
    }

    // -------------------------------------
    // CarlaEngine virtual calls

    bool init(const char* const clientName)
    {
        carla_debug("CarlaEngineNative::init(\"%s\")", clientName);

        fBufferSize = CarlaEngine::getBufferSize();
        fSampleRate = CarlaEngine::getSampleRate();

        CarlaEngine::init(clientName);
        return true;
    }

    bool close()
    {
        carla_debug("CarlaEngineNative::close()");
        CarlaEngine::close();

        return true;
    }

    bool isRunning() const
    {
        return true;
    }

    bool isOffline() const
    {
        return false;
    }

    EngineType type() const
    {
        return kEngineTypePlugin;
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount()
    {
        if (kData->curPluginCount == 0 || kData->plugins == nullptr)
            return 0;

        CarlaPlugin* const plugin(kData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->enabled())
            return 0;

        return kData->plugins[0].plugin->parameterCount();
    }

    const Parameter* getParameterInfo(const uint32_t index)
    {
        if (index >= getParameterCount())
            return nullptr;

        CarlaPlugin* const plugin(kData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->enabled())
            return nullptr;

        static ::Parameter param;
        static char strBufName[STR_MAX+1];
        static char strBufUnit[STR_MAX+1];

        const ParameterData& paramData(plugin->parameterData(index));
        const ParameterRanges& paramRanges(plugin->parameterRanges(index));

        plugin->getParameterName(index, strBufName);
        plugin->getParameterUnit(index, strBufUnit);

        int hints = 0x0;

        if (paramData.hints & PARAMETER_IS_BOOLEAN)
            hints |= ::PARAMETER_IS_BOOLEAN;
        if (paramData.hints & PARAMETER_IS_INTEGER)
            hints |= ::PARAMETER_IS_INTEGER;
        if (paramData.hints & PARAMETER_IS_LOGARITHMIC)
            hints |= ::PARAMETER_IS_LOGARITHMIC;
        if (paramData.hints & PARAMETER_IS_AUTOMABLE)
            hints |= ::PARAMETER_IS_AUTOMABLE;
        if (paramData.hints & PARAMETER_USES_SAMPLERATE)
            hints |= ::PARAMETER_USES_SAMPLE_RATE;
        if (paramData.hints & PARAMETER_USES_SCALEPOINTS)
            hints |= ::PARAMETER_USES_SCALEPOINTS;
        if (paramData.hints & PARAMETER_USES_CUSTOM_TEXT)
            hints |= ::PARAMETER_USES_CUSTOM_TEXT;

        if (paramData.type == PARAMETER_INPUT || paramData.type == PARAMETER_OUTPUT)
        {
            if (paramData.hints & PARAMETER_IS_ENABLED)
                hints |= ::PARAMETER_IS_ENABLED;
            if (paramData.type == PARAMETER_OUTPUT)
                hints |= ::PARAMETER_IS_OUTPUT;
        }

        param.hints = (::ParameterHints)hints;
        param.name  = strBufName;
        param.unit  = strBufUnit;
        param.ranges.def = paramRanges.def;
        param.ranges.min = paramRanges.min;
        param.ranges.max = paramRanges.max;
        param.ranges.step = paramRanges.step;
        param.ranges.stepSmall = paramRanges.stepSmall;
        param.ranges.stepLarge = paramRanges.stepLarge;
        param.scalePointCount = 0; // TODO
        param.scalePoints = nullptr;

        return &param;
    }

    float getParameterValue(const uint32_t index)
    {
        if (index >= getParameterCount())
            return 0.0f;

        CarlaPlugin* const plugin(kData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->enabled())
            return 0.0f;

        return plugin->getParameterValue(index);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate()
    {
    }

    void deactivate()
    {
    }

    void process(float**, float**, const uint32_t frames, const uint32_t midiEventCount, const MidiEvent* const midiEvents)
    {
    }

private:

    PluginDescriptorClassEND(CarlaEngineNative)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineNative)
};

// -----------------------------------------------------------------------

static const PluginDescriptor carlaDesc = {
    /* category  */ ::PLUGIN_CATEGORY_OTHER,
    /* hints     */ /*static_cast<::PluginCategory>(*/::PLUGIN_IS_SYNTH/*)*/,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla",
    /* label     */ "carla",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(CarlaEngineNative)
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

// -----------------------------------------------------------------------

void carla_register_native_plugin_carla()
{
    CARLA_BACKEND_USE_NAMESPACE
    carla_register_native_plugin(&carlaDesc);
}

// -----------------------------------------------------------------------
