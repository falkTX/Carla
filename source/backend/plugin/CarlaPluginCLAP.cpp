/*
 * Carla CLAP Plugin
 * Copyright (C) 2022 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBackendUtils.hpp"
#include "CarlaClapUtils.hpp"
#include "CarlaMathUtils.hpp"

#include "CarlaPluginUI.hpp"

#ifdef CARLA_OS_MAC
# include "CarlaMacUtils.hpp"
# import <Foundation/Foundation.h>
#endif

#include "water/files/File.h"

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

struct carla_clap_host : clap_host_t {
    carla_clap_host()
    {
        clap_version = CLAP_VERSION;
        host_data = this;
        name = "Carla";
        vendor = "falkTX";
        url = "https://kx.studio/carla";
        version = CARLA_VERSION_STRING;
        get_extension = carla_get_extension;
        request_restart = carla_request_restart;
        request_process = carla_request_process;
        request_callback = carla_request_callback;
    }

    static const void* carla_get_extension(const clap_host_t*, const char*) { return nullptr; }
    static void carla_request_restart(const clap_host_t*) {}
    static void carla_request_process(const clap_host_t*) {}
    static void carla_request_callback(const clap_host_t*) {}
};

// --------------------------------------------------------------------------------------------------------------------

class CarlaPluginCLAP : public CarlaPlugin,
                        private CarlaPluginUI::Callback
{
public:
    CarlaPluginCLAP(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fPlugin(nullptr),
          fPluginDescriptor(nullptr),
          fPluginEntry(nullptr),
          fHost()
    {
        carla_debug("CarlaPluginCLAP::CarlaPluginCLAP(%p, %i)", engine, id);

    }

    ~CarlaPluginCLAP() override
    {
        carla_debug("CarlaPluginCLAP::~CarlaPluginCLAP()");

        // close UI
//         if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
//         {
//             if (! fUI.isEmbed)
//                 showCustomUI(false);
//
//             if (fUI.isOpen)
//             {
//                 fUI.isOpen = false;
//                 dispatcher(effEditClose);
//             }
//         }

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

//         CARLA_ASSERT(! fIsProcessing);

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fPlugin != nullptr)
        {
            fPlugin->destroy(fPlugin);
            fPlugin = nullptr;
        }

        clearBuffers();

        if (fPluginEntry != nullptr)
        {
            fPluginEntry->deinit();
            fPluginEntry = nullptr;
        }
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_CLAP;
    }

    /*
    PluginCategory getCategory() const noexcept override
    {
    }

    uint32_t getLatencyInFrames() const noexcept override
    {
    }
    */

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (current data)

    /*
    std::size_t getChunkData(void** const dataPtr) noexcept override
    {
    }
    */

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    /*
    uint getOptionsAvailable() const noexcept override
    {
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
    }
    */

    bool getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPluginDescriptor != nullptr, false);

        std::strncpy(strBuf, fPluginDescriptor->id, STR_MAX);
        return true;
    }

    bool getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPluginDescriptor != nullptr, false);

        std::strncpy(strBuf, fPluginDescriptor->vendor, STR_MAX);
        return true;
    }

    bool getCopyright(char* const strBuf) const noexcept override
    {
        return getMaker(strBuf);
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPluginDescriptor != nullptr, false);

        std::strncpy(strBuf, fPluginDescriptor->name, STR_MAX);
        return true;
    }

    /*
    bool getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
    }

    bool getParameterText(const uint32_t parameterId, char* const strBuf) noexcept override
    {
    }

    bool getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
    }

    bool getParameterGroupName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
    }
    */

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    /*
    void setName(const char* const newName) override
    {
    }
    */

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    /*
    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
    }

    void setParameterValueRT(const uint32_t parameterId, const float value, const uint32_t frameOffset, const bool sendCallbackLater) noexcept override
    {
    }

    void setChunkData(const void* const data, const std::size_t dataSize) override
    {
    }

    void setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool doingInit) noexcept override
    {
    }

    void setProgramRT(const uint32_t uindex, const bool sendCallbackLater) noexcept override
    {
    }
    */

    // -------------------------------------------------------------------
    // Set ui stuff

    /*
    void setCustomUITitle(const char* const title) noexcept override
    {
    }

    void showCustomUI(const bool yesNo) override
    {
    }

    void* embedCustomUI(void* const ptr) override
    {
    }
    */

    void idle() override
    {
        CarlaPlugin::idle();
    }

    void uiIdle() override
    {
        CarlaPlugin::uiIdle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        // CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        carla_debug("CarlaPluginCLAP::reload() - start");

        const EngineProcessMode processMode = pData->engine->getProccessMode();

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        const clap_plugin_audio_ports_t* const audioPorts = static_cast<const clap_plugin_audio_ports_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_AUDIO_PORTS));

        const clap_plugin_note_ports_t* const notePorts = static_cast<const clap_plugin_note_ports_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_NOTE_PORTS));

        const clap_plugin_params_t* const params = static_cast<const clap_plugin_params_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_PARAMS));

        carla_debug("CarlaPluginCLAP::reload() - end");
    }

    void reloadPrograms(const bool doInit) override
    {
        carla_debug("CarlaPluginCLAP::reloadPrograms(%s)", bool2str(doInit));
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
    }

    void deactivate() noexcept override
    {
    }

    void process(const float* const*,
                 float** const audioOut,
                 const float* const*,
                 float** const,
                 const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            return;
        }

    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("CarlaPluginCLAP::bufferSizeChanged(%i)", newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("CarlaPluginCLAP::sampleRateChanged(%g)", newSampleRate);
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() noexcept override
    {
        carla_debug("CarlaPluginCLAP::clearBuffers() - start");

        CarlaPlugin::clearBuffers();

        carla_debug("CarlaPluginCLAP::clearBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    // nothing

    // -------------------------------------------------------------------

protected:
    void handlePluginUIClosed() override
    {
        // CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_debug("CarlaPluginCLAP::handlePluginUIClosed()");

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
        carla_debug("CarlaPluginCLAP::handlePluginUIResized(%u, %u)", width, height);
    }

    // -------------------------------------------------------------------

public:
    bool init(const CarlaPluginPtr plugin,
              const char* const filename, const char* const name, const char* const id, const uint options)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (filename == nullptr || filename[0] == '\0')
        {
            pData->engine->setLastError("null filename");
            return false;
        }

        if (id == nullptr || id[0] == '\0')
        {
            pData->engine->setLastError("null label/id");
            return false;
        }

        // ---------------------------------------------------------------

        const clap_plugin_entry_t* entry;

       #ifdef CARLA_OS_MAC
        if (!water::File(filename).existsAsFile())
        {
            if (! fBundleLoader.load(filename))
            {
                pData->engine->setLastError("Failed to load CLAP bundle executable");
                return false;
            }

            entry = fBundleLoader.getSymbol<const clap_plugin_entry_t*>(CFSTR("clap_entry"));
        }
        else
       #endif
        {
            if (! pData->libOpen(filename))
            {
                pData->engine->setLastError(pData->libError(filename));
                return false;
            }

            entry = pData->libSymbol<const clap_plugin_entry_t*>("clap_entry");
        }

        if (entry == nullptr)
        {
            pData->engine->setLastError("Could not find the CLAP entry in the plugin library");
            return false;
        }

        if (entry->init == nullptr || entry->deinit == nullptr || entry->get_factory == nullptr)
        {
            pData->engine->setLastError("CLAP factory entries are null");
            return false;
        }

        if (!clap_version_is_compatible(entry->clap_version))
        {
            pData->engine->setLastError("Incompatible CLAP plugin");
            return false;
        }

        // ---------------------------------------------------------------

        const water::String pluginPath(water::File(filename).getParentDirectory().getFullPathName());

        if (!entry->init(pluginPath.toRawUTF8()))
        {
            pData->engine->setLastError("Plugin entry failed to initialize");
            return false;
        }

        fPluginEntry = entry;

        // ---------------------------------------------------------------

        const clap_plugin_factory_t* const factory = static_cast<const clap_plugin_factory_t*>(
            entry->get_factory(CLAP_PLUGIN_FACTORY_ID));

        if (factory == nullptr
            || factory->get_plugin_count == nullptr
            || factory->get_plugin_descriptor == nullptr
            || factory->create_plugin == nullptr)
        {
            pData->engine->setLastError("Plugin is missing factory methods");
            return false;
        }

        // ---------------------------------------------------------------

        if (const uint32_t count = factory->get_plugin_count(factory))
        {
            for (uint32_t i=0; i<count; ++i)
            {
                const clap_plugin_descriptor_t* const desc = factory->get_plugin_descriptor(factory, i);
                CARLA_SAFE_ASSERT_CONTINUE(desc != nullptr);
                CARLA_SAFE_ASSERT_CONTINUE(desc->id != nullptr);

                if (std::strcmp(desc->id, id) == 0)
                {
                    fPluginDescriptor = desc;
                    break;
                }
            }
        }
        else
        {
            pData->engine->setLastError("Plugin library contains no plugins");
            return false;
        }

        if (fPluginDescriptor == nullptr)
        {
            pData->engine->setLastError("Plugin library does not contain the requested plugin");
            return false;
        }

        // ---------------------------------------------------------------

        fPlugin = factory->create_plugin(factory, &fHost, fPluginDescriptor->id);

        if (fPlugin == nullptr)
        {
            pData->engine->setLastError("Failed to create CLAP plugin instance");
            return false;
        }

        if (!fPlugin->init(fPlugin))
        {
            pData->engine->setLastError("Failed to initialize CLAP plugin instance");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        pData->name = pData->engine->getUniquePluginName(name != nullptr && name[0] != '\0'
                                                         ? name
                                                         : fPluginDescriptor->name);
        pData->filename = carla_strdup(filename);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(plugin);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // set default options

        pData->options = 0x0;

//         if (fEffect->initialDelay > 0 || hasMidiOutput() || isPluginOptionEnabled(options, PLUGIN_OPTION_FIXED_BUFFERS))
//             pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
//
//         if (fEffect->flags & effFlagsProgramChunks)
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_USE_CHUNKS))
//                 pData->options |= PLUGIN_OPTION_USE_CHUNKS;
//
//         if (hasMidiInput())
//         {
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CONTROL_CHANGES))
//                 pData->options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CHANNEL_PRESSURE))
//                 pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH))
//                 pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PITCHBEND))
//                 pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_ALL_SOUND_OFF))
//                 pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PROGRAM_CHANGES))
//                 pData->options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;
//             if (isPluginOptionInverseEnabled(options, PLUGIN_OPTION_SKIP_SENDING_NOTES))
//                 pData->options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
//         }
//
//         if (fEffect->numPrograms > 1 && (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES) == 0)
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_MAP_PROGRAM_CHANGES))
//                 pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        return true;
    }

private:
    const clap_plugin_t* fPlugin;
    const clap_plugin_descriptor_t* fPluginDescriptor;
    const clap_plugin_entry_t* fPluginEntry;
    const carla_clap_host fHost;

   #ifdef CARLA_OS_MAC
    BundleLoader fBundleLoader;
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newCLAP(const Initializer& init)
{
    carla_debug("CarlaPlugin::newCLAP({%p, \"%s\", \"%s\", \"%s\"})",
                init.engine, init.filename, init.name, init.label);

    std::shared_ptr<CarlaPluginCLAP> plugin(new CarlaPluginCLAP(init.engine, init.id));

    if (! plugin->init(plugin, init.filename, init.name, init.label, init.options))
        return nullptr;

    return plugin;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
