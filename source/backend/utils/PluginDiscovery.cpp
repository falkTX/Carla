/*
 * Carla Plugin Host
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaUtils.h"

#include "CarlaBackendUtils.hpp"
#include "CarlaJuceUtils.hpp"
#include "CarlaPipeUtils.hpp"

#include "water/files/File.h"
#include "water/misc/Time.h"
#include "water/threads/ChildProcess.h"
#include "water/text/StringArray.h"

namespace CB = CARLA_BACKEND_NAMESPACE;

// --------------------------------------------------------------------------------------------------------------------

static const char* const gPluginsDiscoveryNullCharPtr = "";

_CarlaPluginDiscoveryMetadata::_CarlaPluginDiscoveryMetadata() noexcept
    : name(gPluginsDiscoveryNullCharPtr),
      maker(gPluginsDiscoveryNullCharPtr),
      category(CB::PLUGIN_CATEGORY_NONE),
      hints(0x0) {}

_CarlaPluginDiscoveryIO::_CarlaPluginDiscoveryIO() noexcept
    : audioIns(0),
      audioOuts(0),
      cvIns(0),
      cvOuts(0),
      midiIns(0),
      midiOuts(0),
      parameterIns(0),
      parameterOuts(0) {}

_CarlaPluginDiscoveryInfo::_CarlaPluginDiscoveryInfo() noexcept
    : btype(CB::BINARY_NONE),
      ptype(CB::PLUGIN_NONE),
      filename(gPluginsDiscoveryNullCharPtr),
      label(gPluginsDiscoveryNullCharPtr),
      uniqueId(0),
      metadata()  {}

// --------------------------------------------------------------------------------------------------------------------

class CarlaPluginDiscovery : private CarlaPipeServer
{
public:
    CarlaPluginDiscovery(const char* const discoveryTool,
                         const PluginType ptype,
                         const std::vector<water::File>&& binaries,
                         const CarlaPluginDiscoveryCallback callback,
                         void* const callbackPtr)
        : fPluginType(ptype),
          fCallback(callback),
          fCallbackPtr(callbackPtr),
          fBinaryIndex(0),
          fBinaryCount(binaries.size()),
          fBinaries(binaries),
          fDiscoveryTool(discoveryTool),
          fLastMessageTime(0),
          nextLabel(nullptr),
          nextMaker(nullptr),
          nextName(nullptr)
    {
        start();
    }

    CarlaPluginDiscovery(const char* const discoveryTool,
                         const PluginType ptype,
                         const CarlaPluginDiscoveryCallback callback,
                         void* const callbackPtr)
        : fPluginType(ptype),
          fCallback(callback),
          fCallbackPtr(callbackPtr),
          fBinaryIndex(0),
          fBinaryCount(1),
          fDiscoveryTool(discoveryTool),
          fLastMessageTime(0),
          nextLabel(nullptr),
          nextMaker(nullptr),
          nextName(nullptr)
    {
        start();
    }

    ~CarlaPluginDiscovery()
    {
        std::free(nextLabel);
        std::free(nextMaker);
        std::free(nextName);
    }

    // closePipeServer()

    bool idle()
    {
        if (isPipeRunning())
        {
            idlePipe();

            // automatically skip a plugin if 30s passes without a reply
            const uint32_t timeNow = water::Time::getMillisecondCounter();

            if (timeNow - fLastMessageTime < 30000)
                return true;

            carla_stdout("Plugin took too long to respond, skipping...");
            stopPipeServer(1000);
        }

        if (++fBinaryIndex == fBinaryCount)
            return false;

        start();
        return true;
    }

protected:
    bool msgReceived(const char* const msg) noexcept
    {
        fLastMessageTime = water::Time::getMillisecondCounter();

        if (std::strcmp(msg, "warning") == 0 || std::strcmp(msg, "error") == 0)
        {
            const char* text = nullptr;
            readNextLineAsString(text, false);
            carla_stdout("discovery: %s", text);
            return true;
        }

        if (std::strcmp(msg, "init") == 0)
        {
            const char* _;
            readNextLineAsString(_, false);
            new (&nextInfo) _CarlaPluginDiscoveryInfo();
            return true;
        }

        if (std::strcmp(msg, "end") == 0)
        {
            const char* _;
            readNextLineAsString(_, false);

            if (nextInfo.label == nullptr)
                nextInfo.label = gPluginsDiscoveryNullCharPtr;

            if (nextInfo.metadata.maker == nullptr)
                nextInfo.metadata.maker = gPluginsDiscoveryNullCharPtr;

            if (nextInfo.metadata.name == nullptr)
                nextInfo.metadata.name = gPluginsDiscoveryNullCharPtr;

            if (fBinaries.empty())
            {
                char* filename = nullptr;

                if (fPluginType == CB::PLUGIN_LV2)
                {
                    do {
                        const char* const slash = std::strchr(nextLabel, CARLA_OS_SEP);
                        CARLA_SAFE_ASSERT_BREAK(slash != nullptr);
                        filename = strdup(nextLabel);
                        filename[slash - nextLabel] = '\0';
                        nextInfo.filename = filename;
                        nextInfo.label = slash + 1;
                    } while (false);
                }

                fCallback(fCallbackPtr, &nextInfo);

                std::free(filename);
            }
            else
            {
                const water::String filename(fBinaries[fBinaryIndex].getFullPathName());
                nextInfo.filename = filename.toRawUTF8();
                carla_stdout("Found %s from %s", nextInfo.metadata.name, nextInfo.filename);
                fCallback(fCallbackPtr, &nextInfo);
            }

            std::free(nextLabel);
            nextLabel = nullptr;

            std::free(nextMaker);
            nextMaker = nullptr;

            std::free(nextName);
            nextName = nullptr;

            return true;
        }

        if (std::strcmp(msg, "build") == 0)
        {
            uint8_t btype = 0;
            readNextLineAsByte(btype);
            nextInfo.btype = static_cast<BinaryType>(btype);
            return true;
        }

        if (std::strcmp(msg, "hints") == 0)
        {
            readNextLineAsUInt(nextInfo.metadata.hints);
            return true;
        }

        if (std::strcmp(msg, "category") == 0)
        {
            const char* category = nullptr;
            readNextLineAsString(category, false);
            nextInfo.metadata.category = CB::getPluginCategoryFromString(category);
            return true;
        }

        if (std::strcmp(msg, "name") == 0)
        {
            nextInfo.metadata.name = nextName = readNextLineAsString();
            return true;
        }

        if (std::strcmp(msg, "label") == 0)
        {
            nextInfo.label = nextLabel = readNextLineAsString();
            return true;
        }

        if (std::strcmp(msg, "maker") == 0)
        {
            nextInfo.metadata.maker = nextMaker = readNextLineAsString();
            return true;
        }

        if (std::strcmp(msg, "uniqueId") == 0)
        {
            readNextLineAsULong(nextInfo.uniqueId);
            return true;
        }

        if (std::strcmp(msg, "audio.ins") == 0)
        {
            readNextLineAsUInt(nextInfo.io.audioIns);
            return true;
        }

        if (std::strcmp(msg, "audio.outs") == 0)
        {
            readNextLineAsUInt(nextInfo.io.audioOuts);
            return true;
        }

        if (std::strcmp(msg, "cv.ins") == 0)
        {
            readNextLineAsUInt(nextInfo.io.cvIns);
            return true;
        }

        if (std::strcmp(msg, "cv.outs") == 0)
        {
            readNextLineAsUInt(nextInfo.io.cvOuts);
            return true;
        }

        if (std::strcmp(msg, "midi.ins") == 0)
        {
            readNextLineAsUInt(nextInfo.io.midiIns);
            return true;
        }

        if (std::strcmp(msg, "midi.outs") == 0)
        {
            readNextLineAsUInt(nextInfo.io.midiOuts);
            return true;
        }

        if (std::strcmp(msg, "parameters.ins") == 0)
        {
            readNextLineAsUInt(nextInfo.io.parameterIns);
            return true;
        }

        if (std::strcmp(msg, "parameters.outs") == 0)
        {
            readNextLineAsUInt(nextInfo.io.parameterOuts);
            return true;
        }

        if (std::strcmp(msg, "exiting") == 0)
        {
            stopPipeServer(1000);
            return true;
        }

        carla_stdout("discovery: unknown message '%s' received", msg);
        return true;
    }

private:
    const PluginType fPluginType;
    const CarlaPluginDiscoveryCallback fCallback;
    void* const fCallbackPtr;

    uint fBinaryIndex;
    const uint fBinaryCount;
    const std::vector<water::File> fBinaries;
    const CarlaString fDiscoveryTool;

    uint32_t fLastMessageTime;

    CarlaPluginDiscoveryInfo nextInfo;
    char* nextLabel;
    char* nextMaker;
    char* nextName;

    void start()
    {
        fLastMessageTime = water::Time::getMillisecondCounter();

        if (fBinaries.empty())
        {
            startPipeServer(fDiscoveryTool,
                            getPluginTypeAsString(fPluginType),
                            ":all");
        }
        else
        {
            carla_stdout("Scanning %s...", fBinaries[fBinaryIndex].getFullPathName().toRawUTF8());
            startPipeServer(fDiscoveryTool,
                            getPluginTypeAsString(fPluginType),
                            fBinaries[fBinaryIndex].getFullPathName().toRawUTF8());
        }
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginDiscovery)
};

// --------------------------------------------------------------------------------------------------------------------

static std::vector<water::File> findDirectories(const char* const pluginPath, const char* const wildcard)
{
    CARLA_SAFE_ASSERT_RETURN(pluginPath != nullptr, {});

    if (pluginPath[0] == '\0')
        return {};

    using water::File;
    using water::String;
    using water::StringArray;

    const StringArray splitPaths(StringArray::fromTokens(pluginPath, CARLA_OS_SPLIT_STR, ""));

    if (splitPaths.size() == 0)
        return {};

    std::vector<water::File> ret;

    for (String *it = splitPaths.begin(), *end = splitPaths.end(); it != end; ++it)
    {
        const File dir(*it);
        std::vector<File> results;

        if (dir.findChildFiles(results, File::findDirectories|File::ignoreHiddenFiles, true, wildcard) > 0)
        {
            ret.reserve(ret.size() + results.size());
            ret.insert(ret.end(), results.begin(), results.end());
        }
    }

    return ret;
}

static std::vector<water::File> findFiles(const char* const pluginPath, const char* const wildcard)
{
    CARLA_SAFE_ASSERT_RETURN(pluginPath != nullptr, {});

    if (pluginPath[0] == '\0')
        return {};

    using water::File;
    using water::String;
    using water::StringArray;

    const StringArray splitPaths(StringArray::fromTokens(pluginPath, CARLA_OS_SPLIT_STR, ""));

    if (splitPaths.size() == 0)
        return {};

    std::vector<water::File> ret;

    for (String *it = splitPaths.begin(), *end = splitPaths.end(); it != end; ++it)
    {
        const File dir(*it);
        std::vector<File> results;

        if (dir.findChildFiles(results, File::findFiles|File::ignoreHiddenFiles, true, wildcard) > 0)
        {
            ret.reserve(ret.size() + results.size());
            ret.insert(ret.end(), results.begin(), results.end());
        }
    }

    return ret;
}

static std::vector<water::File> findVST3s(const char* const pluginPath)
{
    CARLA_SAFE_ASSERT_RETURN(pluginPath != nullptr, {});

    if (pluginPath[0] == '\0')
        return {};

    using water::File;
    using water::String;
    using water::StringArray;

    const StringArray splitPaths(StringArray::fromTokens(pluginPath, CARLA_OS_SPLIT_STR, ""));

    if (splitPaths.size() == 0)
        return {};

    std::vector<water::File> ret;

   #if defined(CARLA_OS_WIN)
    static constexpr const uint flags = File::findDirectories|File::findFiles;
   #else
    static constexpr const uint flags = File::findDirectories;
   #endif

    for (String *it = splitPaths.begin(), *end = splitPaths.end(); it != end; ++it)
    {
        const File dir(*it);
        std::vector<File> results;

        if (dir.findChildFiles(results, flags|File::ignoreHiddenFiles, true, "*.vst3") > 0)
        {
            ret.reserve(ret.size() + results.size());
            ret.insert(ret.end(), results.begin(), results.end());
        }
    }

    return ret;
}

CarlaPluginDiscoveryHandle carla_plugin_discovery_start(const char* const discoveryTool,
                                                        const PluginType ptype,
                                                        const char* const pluginPath,
                                                        const CarlaPluginDiscoveryCallback callback,
                                                        void* const callbackPtr)
{
    CARLA_SAFE_ASSERT_RETURN(discoveryTool != nullptr && discoveryTool[0] != '\0', nullptr);
    CARLA_SAFE_ASSERT_RETURN(callback != nullptr, nullptr);

    bool directories = false;
    const char* wildcard = nullptr;

    switch (ptype)
    {
    case CB::PLUGIN_NONE:
    case CB::PLUGIN_JACK:
    case CB::PLUGIN_TYPE_COUNT:
        return nullptr;

    case CB::PLUGIN_SFZ:
    case CB::PLUGIN_JSFX:
    {
        const CarlaScopedEnvVar csev("CARLA_DISCOVERY_PATH", pluginPath);
        return new CarlaPluginDiscovery(discoveryTool, ptype, callback, callbackPtr);
    }

    case CB::PLUGIN_INTERNAL:
    case CB::PLUGIN_LV2:
    case CB::PLUGIN_AU:
        return new CarlaPluginDiscovery(discoveryTool, ptype, callback, callbackPtr);

    case CB::PLUGIN_LADSPA:
    case CB::PLUGIN_DSSI:
       #if defined(CARLA_OS_MAC)
        wildcard = "*.dylib";
       #elif defined(CARLA_OS_WIN)
        wildcard = "*.dll";
       #else
        wildcard = "*.so";
       #endif
        break;
    case CB::PLUGIN_VST2:
       #if defined(CARLA_OS_MAC)
        directories = true;
        wildcard = "*.vst";
       #elif defined(CARLA_OS_WIN)
        wildcard = "*.dll";
       #else
        wildcard = "*.so";
       #endif
        break;
    case CB::PLUGIN_VST3:
        // handled separately
        break;
    case CB::PLUGIN_CLAP:
        wildcard = "*.clap";
       #ifdef CARLA_OS_MAC
        directories = true;
       #endif
        break;
    case CB::PLUGIN_DLS:
        wildcard = "*.dls";
        break;
    case CB::PLUGIN_GIG:
        wildcard = "*.gig";
        break;
    case CB::PLUGIN_SF2:
        wildcard = "*.sf2";
        break;
    }

    const std::vector<water::File> binaries(ptype == CB::PLUGIN_VST3
                                            ? findVST3s(pluginPath)
                                            : directories
                                            ? findDirectories(pluginPath, wildcard)
                                            : findFiles(pluginPath, wildcard));

    if (binaries.size() == 0)
        return nullptr;

    return new CarlaPluginDiscovery(discoveryTool, ptype, std::move(binaries), callback, callbackPtr);
}

bool carla_plugin_discovery_idle(CarlaPluginDiscoveryHandle handle)
{
    return static_cast<CarlaPluginDiscovery*>(handle)->idle();
}

void carla_plugin_discovery_stop(CarlaPluginDiscoveryHandle handle)
{
    delete static_cast<CarlaPluginDiscovery*>(handle);
}

// --------------------------------------------------------------------------------------------------------------------
