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

#ifndef BUILD_BRIDGE
# error This file should not be compiled if not building bridge
#endif

#include "engine/CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaLv2Utils.hpp"
#include "CarlaUtils.h"

#include "water/files/File.h"

// ---------------------------------------------------------------------------------------------------------------------
// -Weffc++ compat ext widget

extern "C" {

typedef struct _LV2_External_UI_Widget_Compat {
  void (*run )(struct _LV2_External_UI_Widget_Compat*);
  void (*show)(struct _LV2_External_UI_Widget_Compat*);
  void (*hide)(struct _LV2_External_UI_Widget_Compat*);

  _LV2_External_UI_Widget_Compat() noexcept
      : run(nullptr), show(nullptr), hide(nullptr) {}

} LV2_External_UI_Widget_Compat;

}

// ---------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Weffc++"
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Weffc++"
#endif
class CarlaEngineLV2Single : public CarlaEngine,
                             public LV2_External_UI_Widget_Compat
{
#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif
public:
    CarlaEngineLV2Single(const uint32_t bufferSize, const double sampleRate, const char* const bundlePath, const LV2_URID_Map* uridMap)
        : fPlugin(nullptr),
          fIsActive(false),
          fIsOffline(false),
          fPorts(),
          fUI()
    {
        run  = extui_run;
        show = extui_show;
        hide = extui_hide;

        CARLA_SAFE_ASSERT_RETURN(pData->curPluginCount == 0,)
        CARLA_SAFE_ASSERT_RETURN(pData->plugins[0].plugin == nullptr,);

        // xxxxx
        CarlaString binaryDir(bundlePath);
        binaryDir += CARLA_OS_SEP_STR "bin" CARLA_OS_SEP_STR;

        CarlaString resourceDir(bundlePath);
        resourceDir += CARLA_OS_SEP_STR "res" CARLA_OS_SEP_STR;

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

        pData->options.binaryDir   = binaryDir.dup();
        pData->options.resourceDir = resourceDir.dup();

        setCallback(_engine_callback, this);

        using water::File;
        const File pluginFile(File::getSpecialLocation(File::currentExecutableFile).withFileExtension("xml"));

        if (! loadProject(pluginFile.getFullPathName().toRawUTF8()))
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
        if (fPlugin != nullptr && fIsActive)
            fPlugin->setActive(false, false, false);

        close();
    }

    bool hasPlugin() noexcept
    {
        return fPlugin != nullptr;
    }

    // -----------------------------------------------------------------------------------------------------------------
    // LV2 functions

    void lv2_connect_port(const uint32_t port, void* const dataLocation) noexcept
    {
        fPorts.connectPort(port, dataLocation);
    }

    void lv2_activate() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(! fIsActive,);

        fPlugin->setActive(true, false, false);
        fIsActive = true;
    }

    void lv2_deactivate() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fIsActive,);

        fIsActive = false;
        fPlugin->setActive(false, false, false);
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

    // -----------------------------------------------------------------------------------------------------------------

    void lv2ui_instantiate(LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
                           LV2UI_Widget* widget, const LV2_Feature* const* features)
    {
        fUI.writeFunction = writeFunction;
        fUI.controller = controller;

        const LV2_URID_Map* uridMap = nullptr;

        // -------------------------------------------------------------------------------------------------------------
        // see if the host supports external-ui, get uridMap

        for (int i=0; features[i] != nullptr; ++i)
        {
            if (std::strcmp(features[i]->URI, LV2_EXTERNAL_UI__Host) == 0 ||
                std::strcmp(features[i]->URI, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0)
            {
                fUI.host = (const LV2_External_UI_Host*)features[i]->data;
            }
            else if (std::strcmp(features[i]->URI, LV2_URID__map) == 0)
            {
                uridMap = (const LV2_URID_Map*)features[i]->data;
            }
        }

        if (fUI.host != nullptr)
        {
            fUI.name = fUI.host->plugin_human_id;
            *widget  = (LV2_External_UI_Widget*)this;
            return;
        }

        // -------------------------------------------------------------------------------------------------------------
        // no external-ui support, use showInterface

        for (int i=0; features[i] != nullptr; ++i)
        {
            if (std::strcmp(features[i]->URI, LV2_OPTIONS__options) == 0)
            {
                const LV2_Options_Option* const options((const LV2_Options_Option*)features[i]->data);

                for (int j=0; options[j].key != 0; ++j)
                {
                    if (options[j].key == uridMap->map(uridMap->handle, LV2_UI__windowTitle))
                    {
                        fUI.name = (const char*)options[j].value;
                        break;
                    }
                }
                break;
            }
        }

        if (fUI.name.isEmpty())
            fUI.name = fPlugin->getName();

        *widget = nullptr;
    }

    void lv2ui_port_event(uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer) const
    {
        if (format != 0 || bufferSize != sizeof(float) || buffer == nullptr)
            return;
        if (portIndex >= fPorts.indexOffset || ! fUI.visible)
            return;

        const float value(*(const float*)buffer);
        fPlugin->uiParameterChange(portIndex-fPorts.indexOffset, value);
    }

    void lv2ui_cleanup()
    {
        if (fUI.visible)
            handleUiHide();

        fUI.writeFunction = nullptr;
        fUI.controller = nullptr;
        fUI.host = nullptr;
    }

    // -----------------------------------------------------------------------------------------------------------------

    int lv2ui_idle() const
    {
        if (! fUI.visible)
            return 1;

        handleUiRun();
        return 0;
    }

    int lv2ui_show()
    {
        handleUiShow();
        return 0;
    }

    int lv2ui_hide()
    {
        handleUiHide();
        return 0;
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

    bool isRunning() const noexcept override
    {
        return fIsActive;
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

    void engineCallback(const EngineCallbackOpcode action, const uint pluginId, const int value1, const int value2, const float value3, const char* const valueStr)
    {
        switch (action)
        {
        case ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED:
            CARLA_SAFE_ASSERT_RETURN(value1 >= 0,);
            if (fUI.writeFunction != nullptr && fUI.controller != nullptr && fUI.visible)
                fUI.writeFunction(fUI.controller,
                                  static_cast<uint32_t>(value1)+fPorts.indexOffset,
                                  sizeof(float), 0, &value3);
            break;

        case ENGINE_CALLBACK_UI_STATE_CHANGED:
            fUI.visible = value1 == 1;
            if (fUI.host != nullptr)
                fUI.host->ui_closed(fUI.controller);
            break;

        case ENGINE_CALLBACK_IDLE:
            break;

        default:
            carla_stdout("engineCallback(%i:%s, %u, %i, %i, %f, %s)", action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valueStr);
            break;
        }
    }

    // -----------------------------------------------------------------------------------------------------------------

    void handleUiRun() const
    {
        try {
            fPlugin->uiIdle();
        } CARLA_SAFE_EXCEPTION("fPlugin->uiIdle()")
    }

    void handleUiShow()
    {
        fPlugin->showCustomUI(true);
        fUI.visible = true;
    }

    void handleUiHide()
    {
        fUI.visible = false;
        fPlugin->showCustomUI(false);
    }

    // -----------------------------------------------------------------------------------------------------------------

private:
    CarlaPlugin* fPlugin;

    // Lv2 host data
    bool fIsActive;
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
        uint32_t indexOffset;
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
              indexOffset(1),
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

            indexOffset = numAudioIns + numAudioOuts + 1;
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

    struct UI {
        LV2UI_Write_Function writeFunction;
        LV2UI_Controller controller;
        const LV2_External_UI_Host* host;
        CarlaString name;
        bool visible;

        UI()
          : writeFunction(nullptr),
            controller(nullptr),
            host(nullptr),
            name(),
            visible(false) {}
        CARLA_DECLARE_NON_COPY_STRUCT(UI)
    } fUI;

    // -------------------------------------------------------------------

    #define handlePtr ((CarlaEngineLV2Single*)handle)

    static void extui_run(LV2_External_UI_Widget_Compat* handle)
    {
        handlePtr->handleUiRun();
    }

    static void extui_show(LV2_External_UI_Widget_Compat* handle)
    {
        handlePtr->handleUiShow();
    }

    static void extui_hide(LV2_External_UI_Widget_Compat* handle)
    {
        handlePtr->handleUiHide();
    }

    static void _engine_callback(void* handle, EngineCallbackOpcode action, uint pluginId, int value1, int value2, float value3, const char* valueStr)
    {
        handlePtr->engineCallback(action, pluginId, value1, value2, value3, valueStr);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineLV2Single)
};

CARLA_BACKEND_END_NAMESPACE

CARLA_BACKEND_USE_NAMESPACE

// ---------------------------------------------------------------------------------------------------------------------
// LV2 DSP functions

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

    CarlaEngineLV2Single* const instance(new CarlaEngineLV2Single(bufferSize, sampleRate, bundlePath, uridMap));

    if (instance->hasPlugin())
        return (LV2_Handle)instance;

    delete instance;
    return nullptr;
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
// LV2 UI functions

static LV2UI_Handle lv2ui_instantiate(const LV2UI_Descriptor*, const char*, const char*,
                                      LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
                                      LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    carla_debug("lv2ui_instantiate(..., %p, %p, %p)", writeFunction, controller, widget, features);

    CarlaEngineLV2Single* engine = nullptr;

    for (int i=0; features[i] != nullptr; ++i)
    {
        if (std::strcmp(features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0)
        {
            engine = (CarlaEngineLV2Single*)features[i]->data;
            break;
        }
    }

    if (engine == nullptr)
    {
        carla_stderr("Host doesn't support instance-access, cannot show UI");
        return nullptr;
    }

    engine->lv2ui_instantiate(writeFunction, controller, widget, features);

    return (LV2UI_Handle)engine;
}

#define uiPtr ((CarlaEngineLV2Single*)ui)

static void lv2ui_port_event(LV2UI_Handle ui, uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
{
    carla_debug("lv2ui_port_event(%p, %i, %i, %i, %p)", ui, portIndex, bufferSize, format, buffer);
    uiPtr->lv2ui_port_event(portIndex, bufferSize, format, buffer);
}

static void lv2ui_cleanup(LV2UI_Handle ui)
{
    carla_debug("lv2ui_cleanup(%p)", ui);
    uiPtr->lv2ui_cleanup();
}

static int lv2ui_idle(LV2UI_Handle ui)
{
    return uiPtr->lv2ui_idle();
}

static int lv2ui_show(LV2UI_Handle ui)
{
    carla_debug("lv2ui_show(%p)", ui);
    return uiPtr->lv2ui_show();
}

static int lv2ui_hide(LV2UI_Handle ui)
{
    carla_debug("lv2ui_hide(%p)", ui);
    return uiPtr->lv2ui_hide();
}

static const void* lv2ui_extension_data(const char* uri)
{
    carla_stdout("lv2ui_extension_data(\"%s\")", uri);

    static const LV2UI_Idle_Interface uiidle = { lv2ui_idle };
    static const LV2UI_Show_Interface uishow = { lv2ui_show, lv2ui_hide };

    if (std::strcmp(uri, LV2_UI__idleInterface) == 0)
        return &uiidle;
    if (std::strcmp(uri, LV2_UI__showInterface) == 0)
        return &uishow;

    return nullptr;
}

#undef uiPtr

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
        using namespace water;
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

    static CarlaString ret;

    if (ret.isEmpty())
    {
        using namespace water;
        const File file(File::getSpecialLocation(File::currentExecutableFile).getSiblingFile("ext-ui"));
        ret = String("file://" + file.getFullPathName()).toRawUTF8();
    }

    static const LV2UI_Descriptor lv2UiExtDesc = {
    /* URI            */ ret.buffer(),
    /* instantiate    */ lv2ui_instantiate,
    /* cleanup        */ lv2ui_cleanup,
    /* port_event     */ lv2ui_port_event,
    /* extension_data */ lv2ui_extension_data
    };

    return (index == 0) ? &lv2UiExtDesc : nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
