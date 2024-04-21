// SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#ifdef CARLA_OS_MAC
# include "CarlaBackendUtils.hpp"
# include "CarlaPluginUI.hpp"
# include "CarlaMacUtils.hpp"
# include <AudioToolbox/AudioUnit.h>
# include <Foundation/Foundation.h>
#endif

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

#ifdef CARLA_OS_MAC
typedef AudioComponentPlugInInterface* (*FactoryFn)(const AudioComponentDescription*);
typedef OSStatus (*InitializeFn)(void*);
typedef OSStatus (*UninitializeFn)(void*);
typedef OSStatus (*GetPropertyInfoFn)(void*, AudioUnitPropertyID, AudioUnitScope, AudioUnitElement, UInt32*, Boolean*);
typedef OSStatus (*GetPropertyFn)(void*, AudioUnitPropertyID, AudioUnitScope, AudioUnitElement, void*, UInt32*);
typedef OSStatus (*MIDIEventFn)(void*, UInt32, UInt32, UInt32, UInt32);

static constexpr FourCharCode getFourCharCodeFromString(const char str[4])
{
    return (str[0] << 24) + (str[1] << 16) + (str[2] << 8) + str[3];
}

class CarlaPluginAU : public CarlaPlugin,
                      private CarlaPluginUI::Callback
{
public:
    CarlaPluginAU(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fInterface(nullptr)
    {
        carla_stdout("CarlaPluginAU::CarlaPluginAU(%p, %i)", engine, id);
    }

    ~CarlaPluginAU() override
    {
        carla_stdout("CarlaPluginAU::~CarlaPluginAU()");

        // close UI
        showCustomUI(false);

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fInterface != nullptr)
        {
            fInterface->Close(fInterface);
            fInterface = nullptr;
        }

        // if (fLastChunk != nullptr)
        // {
        //     std::free(fLastChunk);
        //     fLastChunk = nullptr;
        // }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_AU;
    }

    PluginCategory getCategory() const noexcept override
    {
        // TODO
        return PLUGIN_CATEGORY_NONE;
    }

    uint32_t getLatencyInFrames() const noexcept override
    {
        // TODO
        return 0;
    }

    // -------------------------------------------------------------------
    // Information (count)

    // -------------------------------------------------------------------
    // Information (current data)

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    bool getLabel(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fLabel.buffer(), STR_MAX);
        return true;
    }

    bool getMaker(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fMaker.buffer(), STR_MAX);
        return true;
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fName.buffer(), STR_MAX);
        return true;
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fInterface != nullptr,);
        carla_debug("CarlaPluginAU::reload() - start");

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("CarlaPluginAU::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInterface != nullptr,);

        // TODO
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInterface != nullptr,);

        // TODO
    }

    void process(const float* const* const audioIn,
                 float** const audioOut,
                 const float* const* const cvIn,
                 float** const,
                 const uint32_t frames) override
    {
        // TODO
    }

    // -------------------------------------------------------------------

protected:
    void handlePluginUIClosed() override
    {
        carla_stdout("CarlaPluginCLAP::handlePluginUIClosed()");

        // TODO
    }

    void handlePluginUIResized(const uint width, const uint height) override
    {
        // TODO
    }

    // -------------------------------------------------------------------

public:
    bool init(const CarlaPluginPtr plugin,
              const char* const filename,
              const char* const label,
              const char* const name,
              const uint options)
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

        if (label == nullptr)
        {
            pData->engine->setLastError("null label");
            return false;
        }

        // ---------------------------------------------------------------
        // load bundle information

        if (! fBundleLoader.load(filename))
        {
            pData->engine->setLastError("Failed to load AU bundle executable");
            return false;
        }

        const CFTypeRef componentsRef = CFBundleGetValueForInfoDictionaryKey(fBundleLoader.getRef(), CFSTR("AudioComponents"));

        if (componentsRef == nullptr || CFGetTypeID(componentsRef) != CFArrayGetTypeID())
        {
            pData->engine->setLastError("Not an AU component");
            return false;
        }

        // ---------------------------------------------------------------
        // find binary matching requested label

        CFStringRef componentName;
        AudioComponentDescription desc = {};
        FactoryFn factoryFn;

        const CFArrayRef components = static_cast<CFArrayRef>(componentsRef);

        for (uint32_t c = 0, count = CFArrayGetCount(components); c < count; ++c)
        {
            // reset
            desc.componentType = 0;

            const CFTypeRef componentRef = CFArrayGetValueAtIndex(components, c);
            CARLA_SAFE_ASSERT_CONTINUE(componentRef != nullptr);
            CARLA_SAFE_ASSERT_CONTINUE(CFGetTypeID(componentRef) == CFDictionaryGetTypeID());

            const CFDictionaryRef component = static_cast<CFDictionaryRef>(componentRef);

            componentName = nullptr;
            CARLA_SAFE_ASSERT_CONTINUE(CFDictionaryGetValueIfPresent(component, CFSTR("name"), (const void **)&componentName));

            CFStringRef componentFactoryFunction = nullptr;
            CARLA_SAFE_ASSERT_CONTINUE(CFDictionaryGetValueIfPresent(component, CFSTR("factoryFunction"), (const void **)&componentFactoryFunction));

            CFStringRef componentType = nullptr;
            CARLA_SAFE_ASSERT_CONTINUE(CFDictionaryGetValueIfPresent(component, CFSTR("type"), (const void **)&componentType));
            CARLA_SAFE_ASSERT_CONTINUE(CFStringGetLength(componentType) == 4);

            CFStringRef componentSubType = nullptr;
            CARLA_SAFE_ASSERT_CONTINUE(CFDictionaryGetValueIfPresent(component, CFSTR("subtype"), (const void **)&componentSubType));
            CARLA_SAFE_ASSERT_CONTINUE(CFStringGetLength(componentSubType) == 4);

            CFStringRef componentManufacturer = nullptr;
            CARLA_SAFE_ASSERT_CONTINUE(CFDictionaryGetValueIfPresent(component, CFSTR("manufacturer"), (const void **)&componentManufacturer));
            CARLA_SAFE_ASSERT_CONTINUE(CFStringGetLength(componentManufacturer) == 4);

            factoryFn = fBundleLoader.getSymbol<FactoryFn>(componentFactoryFunction);
            CARLA_SAFE_ASSERT_CONTINUE(factoryFn != nullptr);

            char clabel[15] = {};
            CFStringGetCString(componentType, clabel, 5, kCFStringEncodingASCII);
            CFStringGetCString(componentSubType, clabel + 5, 5, kCFStringEncodingASCII);
            CFStringGetCString(componentManufacturer, clabel + 10, 5, kCFStringEncodingASCII);

            desc.componentType = getFourCharCodeFromString(clabel);
            desc.componentSubType = getFourCharCodeFromString(clabel + 5);
            desc.componentManufacturer = getFourCharCodeFromString(clabel + 10);

            CARLA_SAFE_ASSERT_CONTINUE(desc.componentType != 0);
            CARLA_SAFE_ASSERT_CONTINUE(desc.componentSubType != 0);
            CARLA_SAFE_ASSERT_CONTINUE(desc.componentManufacturer != 0);

            clabel[4] = clabel[9] = ',';

            if (label[0] == '\0' || std::strcmp(label, clabel) == 0)
                break;
        }

        if (desc.componentType == 0)
        {
            pData->engine->setLastError("Failed to find request plugin in Component bundle");
            return false;
        }

        // ---------------------------------------------------------------
        // load binary

        fInterface = factoryFn(&desc);
        
        if (fInterface == nullptr)
        {
            pData->engine->setLastError("Component failed to create new interface");
            return false;
        }

        const InitializeFn auInitialize = (InitializeFn)fInterface->Lookup(kAudioUnitInitializeSelect);
        const UninitializeFn auUninitialize = (UninitializeFn)fInterface->Lookup(kAudioUnitUninitializeSelect);
        const GetPropertyInfoFn auGetPropertyInfo = (GetPropertyInfoFn)fInterface->Lookup(kAudioUnitGetPropertyInfoSelect);
        const GetPropertyFn auGetProperty = (GetPropertyFn)fInterface->Lookup(kAudioUnitGetPropertySelect);
        const MIDIEventFn auMIDIEvent = (MIDIEventFn)fInterface->Lookup(kMusicDeviceMIDIEventSelect);

        if (auInitialize == nullptr ||
            auUninitialize == nullptr ||
            auGetPropertyInfo == nullptr ||
            auGetProperty == nullptr)
        {
            pData->engine->setLastError("Component does not provide all necessary functions");
            fInterface = nullptr;
            return false;
        }

        if (fInterface->Open(fInterface, (AudioUnit)(void*)0x1) != noErr)
        {
            pData->engine->setLastError("Component failed to open");
            fInterface->Close(fInterface);
            fInterface = nullptr;
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        const CFIndex componentNameLen = CFStringGetLength(componentName);
        char* const nameBuffer = new char[componentNameLen + 1];

        if (CFStringGetCString(componentName, nameBuffer, componentNameLen + 1, kCFStringEncodingUTF8))
        {
            if (char* const sep = std::strstr(nameBuffer, ": "))
            {
                sep[0] = sep[1] = '\0';
                fName = sep + 2;
                fMaker = nameBuffer;
            }
            else
            {
                fName = nameBuffer;
                fMaker = nameBuffer + componentNameLen;
            }
        }

        fLabel = label;
        pData->name = pData->engine->getUniquePluginName(name != nullptr && name[0] != '\0' ? name : fName.buffer());
        pData->filename = carla_strdup(filename);

        delete[] nameBuffer;

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

        pData->options = PLUGIN_OPTION_FIXED_BUFFERS;

        return true;
    }

private:
    BundleLoader fBundleLoader;
    AudioComponentPlugInInterface* fInterface;
    CarlaString fName;
    CarlaString fLabel;
    CarlaString fMaker;
};
#endif

// -------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newAU(const Initializer& init)
{
    carla_stdout("CarlaPlugin::newAU({%p, \"%s\", \"%s\", \"%s\", " P_INT64 "})",
                init.engine, init.filename, init.label, init.name, init.uniqueId);

   #ifdef CARLA_OS_MAC
    std::shared_ptr<CarlaPluginAU> plugin(new CarlaPluginAU(init.engine, init.id));

    if (! plugin->init(plugin, init.filename, init.label, init.name, init.options))
        return nullptr;

    return plugin;
   #else
    init.engine->setLastError("AU support not available");
    return nullptr;
   #endif
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
