/*
 * Carla LV2 Single Plugin
 * Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>
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

// #define BUILD_BRIDGE

#include "engine/CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaLv2Utils.hpp"
#include "CarlaUtils.h"

// ---------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

class CarlaEngineLV2Single : public CarlaEngine
{
public:
    CarlaEngineLV2Single(const uint32_t bufferSize, const double sampleRate, const char* const bundlePath, const LV2_URID_Map* uridMap)
        : fPlugin(nullptr),
          fIsRunning(false),
          fIsOffline(false)
    {
        // xxxxx
        CarlaString resourceDir(bundlePath);
        resourceDir += CARLA_OS_SEP_STR "resources" CARLA_OS_SEP_STR;

        pData->bufferSize = bufferSize;
        pData->sampleRate = sampleRate;
        pData->initTime(nullptr);

        pData->options.processMode         = ENGINE_PROCESS_MODE_BRIDGE;
        pData->options.transportMode       = ENGINE_TRANSPORT_MODE_PLUGIN;
        pData->options.forceStereo         = false;
        pData->options.preferPluginBridges = false;
        pData->options.preferUiBridges     = false;
        init("LV2-Export");

        if (pData->options.resourceDir != nullptr)
            delete[] pData->options.resourceDir;
        if (pData->options.binaryDir != nullptr)
            delete[] pData->options.binaryDir;

        pData->options.resourceDir = resourceDir.dup();
        pData->options.binaryDir   = carla_strdup(carla_get_library_folder());

        setCallback(_engine_callback, this);

        if (! addPlugin(BINARY_NATIVE, PLUGIN_VST2, "/usr/lib/vst/3BandEQ-vst.so", nullptr, nullptr, 0, nullptr, 0x0))
        {
            carla_stderr2("Failed to init plugin, possible reasons: %s", getLastError());
            return;
        }

        CARLA_SAFE_ASSERT_RETURN(pData->curPluginCount == 1,)

        fPlugin = pData->plugins[0].plugin;
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fPlugin->isEnabled(),);

        fPorts.init(fPlugin);
    }

    ~CarlaEngineLV2Single()
    {
        close();
    }

    bool hasPlugin()
    {
        return fPlugin != nullptr;
    }

    // -----------------------------------------------------------------------------------------------------------------
    // LV2 functions

    void lv2_connect_port(const uint32_t port, void* const dataLocation)
    {
        fPorts.connectPort(port, dataLocation);
    }

    void lv2_activate()
    {
        CARLA_SAFE_ASSERT_RETURN(! fIsRunning,);

        fPlugin->activate();
        fIsRunning = true;
    }

    void lv2_deactivate()
    {
        CARLA_SAFE_ASSERT_RETURN(fIsRunning,);

        fIsRunning = false;
        fPlugin->deactivate();
    }

    void lv2_run(const uint32_t frames)
    {
        fIsOffline = (fPorts.freewheel != nullptr && *fPorts.freewheel >= 0.5f);

        // Check for updated parameters
        float curValue;

        for (uint32_t i=0; i < fPorts.numParams; ++i)
        {
            if (fPorts.paramsOut[i])
                continue;

            CARLA_SAFE_ASSERT_CONTINUE(fPorts.paramsPtr[i] != nullptr)

            curValue = *fPorts.paramsPtr[i];

            if (carla_isEqual(fPorts.paramsLast[i], curValue))
                continue;

            fPorts.paramsLast[i] = curValue;
            fPlugin->setParameterValue(i, curValue, false, false, false);
        }

        if (frames == 0)
           return fPorts.updateOutputs();

        if (fPlugin->tryLock(fIsOffline))
        {
            fPlugin->initBuffers();
            fPlugin->process(fPorts.audioIns, fPorts.audioOuts, nullptr, nullptr, frames);
            fPlugin->unlock();
        }
        else
        {
            for (uint32_t i=0; i<fPorts.numAudioOuts; ++i)
                carla_zeroFloats(fPorts.audioOuts[i], frames);
        }

        fPorts.updateOutputs();
    }

protected:
    // -----------------------------------------------------------------------------------------------------------------
    // CarlaEngine virtual calls

    bool init(const char* const clientName) override
    {
        carla_stdout("CarlaEngineNative::init(\"%s\")", clientName);

        if (! pData->init(clientName))
        {
            close();
            setLastError("Failed to init internal data");
            return false;
        }

        return true;
    }

    bool close() override
    {
        fIsRunning = false;
        CarlaEngine::close();
        return true;
    }

    bool isRunning() const noexcept override
    {
        return fIsRunning;
    }

    bool isOffline() const noexcept override
    {
        return fIsOffline;
    }

    bool usesConstantBufferSize() const noexcept override
    {
        return true;
    }

    EngineType getType() const noexcept override
    {
        return kEngineTypePlugin;
    }

    const char* getCurrentDriverName() const noexcept override
    {
        return "LV2 Plugin";
    }

    void callback(const EngineCallbackOpcode action, const uint pluginId, const int value1, const int value2, const float value3, const char* const valueStr) noexcept override
    {
        CarlaEngine::callback(action, pluginId, value1, value2, value3, valueStr);

        //if (action == ENGINE_CALLBACK_IDLE && ! pData->aboutToClose)
        //    pHost->dispatcher(pHost->handle, NATIVE_HOST_OPCODE_HOST_IDLE, 0, 0, nullptr, 0.0f);
    }

    void engineCallback(const EngineCallbackOpcode action, const uint pluginId, const int value1, const int value2, const float value3, const char* const valueStr)
    {
        carla_stdout("engineCallback(%i:%s, %u, %i, %i, %f, %s)", action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valueStr);
    }

    // -----------------------------------------------------------------------------------------------------------------

private:
    CarlaPlugin* fPlugin;
    bool fIsRunning;

    // Lv2 host data
    bool fIsOffline;

    struct Ports {
        uint32_t numAudioIns;
        uint32_t numAudioOuts;
        uint32_t numParams;
        const float** audioIns;
        /* */ float** audioOuts;
        float*   freewheel;
        float*   paramsLast;
        float**  paramsPtr;
        bool*    paramsOut;
        const CarlaPlugin* plugin;

        Ports()
            : numAudioIns(0),
              numAudioOuts(0),
              numParams(0),
              audioIns(nullptr),
              audioOuts(nullptr),
              freewheel(nullptr),
              paramsLast(nullptr),
              paramsPtr(nullptr),
              paramsOut(nullptr),
              plugin(nullptr) {}

        ~Ports()
        {
            if (audioIns != nullptr)
            {
                delete[] audioIns;
                audioIns = nullptr;
            }

            if (audioOuts != nullptr)
            {
                delete[] audioOuts;
                audioOuts = nullptr;
            }

            if (paramsLast != nullptr)
            {
                delete[] paramsLast;
                paramsLast = nullptr;
            }

            if (paramsPtr != nullptr)
            {
                delete[] paramsPtr;
                paramsPtr = nullptr;
            }

            if (paramsOut != nullptr)
            {
                delete[] paramsOut;
                paramsOut = nullptr;
            }
        }

        void init(const CarlaPlugin* const pl)
        {
            plugin = pl;

            if ((numAudioIns = plugin->getAudioInCount()) > 0)
            {
                audioIns = new const float*[numAudioIns];

                for (uint32_t i=0; i < numAudioIns; ++i)
                    audioIns[i] = nullptr;
            }

            if ((numAudioOuts = plugin->getAudioOutCount()) > 0)
            {
                audioOuts = new float*[numAudioOuts];

                for (uint32_t i=0; i < numAudioOuts; ++i)
                    audioOuts[i] = nullptr;
            }

            if ((numParams = plugin->getParameterCount()) > 0)
            {
                paramsLast = new float[numParams];
                paramsPtr  = new float*[numParams];
                paramsOut  = new bool[numParams];

                for (uint32_t i=0; i < numParams; ++i)
                {
                    paramsLast[i] = plugin->getParameterValue(i);
                    paramsPtr [i] = nullptr;
                    paramsOut [i] = plugin->isParameterOutput(i);
                }
            }
        }

        void connectPort(const uint32_t port, void* const dataLocation) noexcept
        {
            uint32_t index = 0;

            for (uint32_t i=0; i < numAudioIns; ++i)
            {
                if (port == index++)
                {
                    audioIns[i] = (float*)dataLocation;
                    return;
                }
            }

            for (uint32_t i=0; i < numAudioOuts; ++i)
            {
                if (port == index++)
                {
                    audioOuts[i] = (float*)dataLocation;
                    return;
                }
            }

            if (port == index++)
            {
                freewheel = (float*)dataLocation;
                return;
            }

            for (uint32_t i=0; i < numParams; ++i)
            {
                if (port == index++)
                {
                    paramsPtr[i] = (float*)dataLocation;
                    return;
                }
            }
        }

        void updateOutputs() noexcept
        {
            for (uint32_t i=0; i < numParams; ++i)
            {
                if (! paramsOut[i])
                    continue;

                paramsLast[i] = plugin->getParameterValue(i);

                if (paramsPtr[i] != nullptr)
                    *paramsPtr[i] = paramsLast[i];
            }
        }

        CARLA_DECLARE_NON_COPY_STRUCT(Ports);

    } fPorts;

    #define handlePtr ((CarlaEngineLV2Single*)handle)

    static void _engine_callback(void* handle, EngineCallbackOpcode action, uint pluginId, int value1, int value2, float value3, const char* valueStr)
    {
        handlePtr->engineCallback(action, pluginId, value1, value2, value3, valueStr);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineLV2Single)
};

CARLA_BACKEND_END_NAMESPACE

// ---------------------------------------------------------------------------------------------------------------------
// LV2 plugin descriptor functions

CARLA_BACKEND_USE_NAMESPACE

static LV2_Handle lv2_instantiate(const LV2_Descriptor* lv2Descriptor, double sampleRate, const char* bundlePath, const LV2_Feature* const* features)
{
    carla_stdout("lv2_instantiate(%p, %g, %s, %p)", lv2Descriptor, sampleRate, bundlePath, features);

    const LV2_Options_Option*  options   = nullptr;
    const LV2_URID_Map*        uridMap   = nullptr;
    const LV2_URID_Unmap*      uridUnmap = nullptr;

    for (int i=0; features[i] != nullptr; ++i)
    {
        if (std::strcmp(features[i]->URI, LV2_OPTIONS__options) == 0)
            options = (const LV2_Options_Option*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_URID__map) == 0)
            uridMap = (const LV2_URID_Map*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_URID__unmap) == 0)
            uridUnmap = (const LV2_URID_Unmap*)features[i]->data;
    }

    if (options == nullptr || uridMap == nullptr)
    {
        carla_stderr("Host doesn't provide option or urid-map features");
        return nullptr;
    }

    uint32_t bufferSize = 0;

    for (int i=0; options[i].key != 0; ++i)
    {
        if (uridUnmap != nullptr) {
            carla_stdout("Host option %i:\"%s\"", i, uridUnmap->unmap(uridUnmap->handle, options[i].key));
        }

        if (options[i].key == uridMap->map(uridMap->handle, LV2_BUF_SIZE__nominalBlockLength))
        {
            if (options[i].type == uridMap->map(uridMap->handle, LV2_ATOM__Int))
            {
                const int value(*(const int*)options[i].value);
                CARLA_SAFE_ASSERT_CONTINUE(value > 0);

                bufferSize = static_cast<uint32_t>(value);
            }
            else
            {
                carla_stderr("Host provides nominalBlockLength but has wrong value type");
            }
            break;
        }

        if (options[i].key == uridMap->map(uridMap->handle, LV2_BUF_SIZE__maxBlockLength))
        {
            if (options[i].type == uridMap->map(uridMap->handle, LV2_ATOM__Int))
            {
                const int value(*(const int*)options[i].value);
                CARLA_SAFE_ASSERT_CONTINUE(value > 0);

                bufferSize = static_cast<uint32_t>(value);
            }
            else
            {
                carla_stderr("Host provides maxBlockLength but has wrong value type");
            }
            // no break, continue in case host supports nominalBlockLength
        }
    }

    if (bufferSize == 0)
    {
        carla_stderr("Host doesn't provide bufferSize feature");
        return nullptr;
    }

    return new CarlaEngineLV2Single(bufferSize, sampleRate, bundlePath, uridMap);
}

#define instancePtr ((CarlaEngineLV2Single*)instance)

static void lv2_connect_port(LV2_Handle instance, uint32_t port, void* dataLocation)
{
    instancePtr->lv2_connect_port(port, dataLocation);
}

static void lv2_activate(LV2_Handle instance)
{
    carla_stdout("lv2_activate(%p)", instance);
    instancePtr->lv2_activate();
}

static void lv2_run(LV2_Handle instance, uint32_t sampleCount)
{
    instancePtr->lv2_run(sampleCount);
}

static void lv2_deactivate(LV2_Handle instance)
{
    carla_stdout("lv2_deactivate(%p)", instance);
    instancePtr->lv2_deactivate();
}

static void lv2_cleanup(LV2_Handle instance)
{
    carla_stdout("lv2_cleanup(%p)", instance);
    delete instancePtr;
}

static const void* lv2_extension_data(const char* uri)
{
    carla_stdout("lv2_extension_data(\"%s\")", uri);

    return nullptr;
}

#undef instancePtr

// ---------------------------------------------------------------------------------------------------------------------
// Startup code

CARLA_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    carla_stdout("lv2_descriptor(%i)", index);

    if (index != 0)
        return nullptr;

    static CarlaString ret;

    if (ret.isEmpty())
    {
        using namespace juce;
        const File file(File::getSpecialLocation(File::currentExecutableFile).withFileExtension("ttl"));
        ret = String("file://" + file.getFullPathName()).toRawUTF8();
    }

    static const LV2_Descriptor desc = {
    /* URI            */ ret.buffer(),
    /* instantiate    */ lv2_instantiate,
    /* connect_port   */ lv2_connect_port,
    /* activate       */ lv2_activate,
    /* run            */ lv2_run,
    /* deactivate     */ lv2_deactivate,
    /* cleanup        */ lv2_cleanup,
    /* extension_data */ lv2_extension_data
    };

    return &desc;
}

CARLA_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    carla_stdout("lv2ui_descriptor(%i)", index);

    return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
