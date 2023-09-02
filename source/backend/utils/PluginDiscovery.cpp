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
#include "CarlaBinaryUtils.hpp"
#include "CarlaJuceUtils.hpp"
#include "CarlaPipeUtils.hpp"
#include "CarlaSha1Utils.hpp"
#include "CarlaTimeUtils.hpp"

#include "water/files/File.h"
#include "water/files/FileInputStream.h"
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
                         const BinaryType btype,
                         const PluginType ptype,
                         const std::vector<water::File>&& binaries,
                         const CarlaPluginDiscoveryCallback discoveryCb,
                         const CarlaPluginCheckCacheCallback checkCacheCb,
                         void* const callbackPtr)
        : fBinaryType(btype),
          fPluginType(ptype),
          fDiscoveryCallback(discoveryCb),
          fCheckCacheCallback(checkCacheCb),
          fCallbackPtr(callbackPtr),
          fPluginsFoundInBinary(false),
          fBinaryIndex(0),
          fBinaryCount(static_cast<uint>(binaries.size())),
          fBinaries(binaries),
          fDiscoveryTool(discoveryTool),
          fLastMessageTime(0),
          fNextLabel(nullptr),
          fNextMaker(nullptr),
          fNextName(nullptr)
    {
        start();
    }

    CarlaPluginDiscovery(const char* const discoveryTool,
                         const BinaryType btype,
                         const PluginType ptype,
                         const CarlaPluginDiscoveryCallback discoveryCb,
                         const CarlaPluginCheckCacheCallback checkCacheCb,
                         void* const callbackPtr)
        : fBinaryType(btype),
          fPluginType(ptype),
          fDiscoveryCallback(discoveryCb),
          fCheckCacheCallback(checkCacheCb),
          fCallbackPtr(callbackPtr),
          fPluginsFoundInBinary(false),
          fBinaryIndex(0),
          fBinaryCount(1),
          fDiscoveryTool(discoveryTool),
          fLastMessageTime(0),
          fNextLabel(nullptr),
          fNextMaker(nullptr),
          fNextName(nullptr)
    {
        start();
    }

    ~CarlaPluginDiscovery()
    {
        stopPipeServer(5000);
        std::free(fNextLabel);
        std::free(fNextMaker);
        std::free(fNextName);
    }

    bool idle()
    {
        if (isPipeRunning())
        {
            idlePipe();

            // automatically skip a plugin if 30s passes without a reply
            const uint32_t timeNow = carla_gettime_ms();

            if (timeNow - fLastMessageTime < 30000)
                return true;

            carla_stdout("Plugin took too long to respond, skipping...");
            stopPipeServer(1000);
        }

        // report binary as having no plugins
        if (fCheckCacheCallback != nullptr && !fPluginsFoundInBinary && !fBinaries.empty())
        {
            const water::File file(fBinaries[fBinaryIndex]);
            const water::String filename(file.getFullPathName());

            makeHash(file, filename);

            if (! fCheckCacheCallback(fCallbackPtr, filename.toRawUTF8(), fNextSha1Sum))
                fDiscoveryCallback(fCallbackPtr, nullptr, fNextSha1Sum);
        }

        if (++fBinaryIndex == fBinaryCount)
            return false;

        start();
        return true;
    }

    void skip()
    {
        if (isPipeRunning())
            stopPipeServer(1000);
    }

protected:
    bool msgReceived(const char* const msg) noexcept
    {
        fLastMessageTime = carla_gettime_ms();

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
            new (&fNextInfo) _CarlaPluginDiscoveryInfo();
            return true;
        }

        if (std::strcmp(msg, "end") == 0)
        {
            const char* _;
            readNextLineAsString(_, false);

            if (fNextInfo.label == nullptr)
                fNextInfo.label = gPluginsDiscoveryNullCharPtr;

            if (fNextInfo.metadata.maker == nullptr)
                fNextInfo.metadata.maker = gPluginsDiscoveryNullCharPtr;

            if (fNextInfo.metadata.name == nullptr)
                fNextInfo.metadata.name = gPluginsDiscoveryNullCharPtr;

            if (fBinaries.empty())
            {
                char* filename = nullptr;

                if (fPluginType == CB::PLUGIN_LV2)
                {
                    do {
                        const char* const slash = std::strchr(fNextLabel, CARLA_OS_SEP);
                        CARLA_SAFE_ASSERT_BREAK(slash != nullptr);
                        filename = strdup(fNextLabel);
                        filename[slash - fNextLabel] = '\0';
                        fNextInfo.filename = filename;
                        fNextInfo.label = slash + 1;
                    } while (false);
                }

                fNextInfo.ptype = fPluginType;
                fDiscoveryCallback(fCallbackPtr, &fNextInfo, nullptr);

                std::free(filename);
            }
            else
            {
                CARLA_SAFE_ASSERT(fNextSha1Sum.isNotEmpty());
                const water::String filename(fBinaries[fBinaryIndex].getFullPathName());
                fNextInfo.filename = filename.toRawUTF8();
                fNextInfo.ptype = fPluginType;
                fPluginsFoundInBinary = true;
                carla_stdout("Found %s from %s", fNextInfo.metadata.name, fNextInfo.filename);
                fDiscoveryCallback(fCallbackPtr, &fNextInfo, fNextSha1Sum);
            }

            std::free(fNextLabel);
            fNextLabel = nullptr;

            std::free(fNextMaker);
            fNextMaker = nullptr;

            std::free(fNextName);
            fNextName = nullptr;

            return true;
        }

        if (std::strcmp(msg, "build") == 0)
        {
            uint8_t btype = 0;
            readNextLineAsByte(btype);
            fNextInfo.btype = static_cast<BinaryType>(btype);
            return true;
        }

        if (std::strcmp(msg, "hints") == 0)
        {
            readNextLineAsUInt(fNextInfo.metadata.hints);
            return true;
        }

        if (std::strcmp(msg, "category") == 0)
        {
            const char* category = nullptr;
            readNextLineAsString(category, false);
            fNextInfo.metadata.category = CB::getPluginCategoryFromString(category);
            return true;
        }

        if (std::strcmp(msg, "name") == 0)
        {
            fNextInfo.metadata.name = fNextName = readNextLineAsString();
            return true;
        }

        if (std::strcmp(msg, "label") == 0)
        {
            fNextInfo.label = fNextLabel = readNextLineAsString();
            return true;
        }

        if (std::strcmp(msg, "maker") == 0)
        {
            fNextInfo.metadata.maker = fNextMaker = readNextLineAsString();
            return true;
        }

        if (std::strcmp(msg, "uniqueId") == 0)
        {
            readNextLineAsULong(fNextInfo.uniqueId);
            return true;
        }

        if (std::strcmp(msg, "audio.ins") == 0)
        {
            readNextLineAsUInt(fNextInfo.io.audioIns);
            return true;
        }

        if (std::strcmp(msg, "audio.outs") == 0)
        {
            readNextLineAsUInt(fNextInfo.io.audioOuts);
            return true;
        }

        if (std::strcmp(msg, "cv.ins") == 0)
        {
            readNextLineAsUInt(fNextInfo.io.cvIns);
            return true;
        }

        if (std::strcmp(msg, "cv.outs") == 0)
        {
            readNextLineAsUInt(fNextInfo.io.cvOuts);
            return true;
        }

        if (std::strcmp(msg, "midi.ins") == 0)
        {
            readNextLineAsUInt(fNextInfo.io.midiIns);
            return true;
        }

        if (std::strcmp(msg, "midi.outs") == 0)
        {
            readNextLineAsUInt(fNextInfo.io.midiOuts);
            return true;
        }

        if (std::strcmp(msg, "parameters.ins") == 0)
        {
            readNextLineAsUInt(fNextInfo.io.parameterIns);
            return true;
        }

        if (std::strcmp(msg, "parameters.outs") == 0)
        {
            readNextLineAsUInt(fNextInfo.io.parameterOuts);
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
    const BinaryType fBinaryType;
    const PluginType fPluginType;
    const CarlaPluginDiscoveryCallback fDiscoveryCallback;
    const CarlaPluginCheckCacheCallback fCheckCacheCallback;
    void* const fCallbackPtr;

    bool fPluginsFoundInBinary;
    uint fBinaryIndex;
    const uint fBinaryCount;
    const std::vector<water::File> fBinaries;
    const CarlaString fDiscoveryTool;

    uint32_t fLastMessageTime;

    CarlaPluginDiscoveryInfo fNextInfo;
    CarlaString fNextSha1Sum;
    char* fNextLabel;
    char* fNextMaker;
    char* fNextName;

    void start()
    {
        fLastMessageTime = carla_gettime_ms();
        fPluginsFoundInBinary = false;
        fNextSha1Sum.clear();

        const char* helperTool;
        switch (fBinaryType)
        {
       #ifndef CARLA_OS_WIN
        case CB::BINARY_WIN32:
            helperTool = "wine";
            break;
        case CB::BINARY_WIN64:
            helperTool = "wine64";
            break;
       #endif
        default:
            helperTool = nullptr;
            break;
        }

        if (fBinaries.empty())
        {
            startPipeServer(helperTool, fDiscoveryTool,
                            getPluginTypeAsString(fPluginType),
                            ":all");
        }
        else
        {
            const water::File file(fBinaries[fBinaryIndex]);
            const water::String filename(file.getFullPathName());

            if (fCheckCacheCallback != nullptr)
            {
                makeHash(file, filename);

                if (fCheckCacheCallback(fCallbackPtr, filename.toRawUTF8(), fNextSha1Sum))
                {
                    fPluginsFoundInBinary = true;
                    carla_debug("Skipping \"%s\", using cache", filename.toRawUTF8());
                    return;
                }
            }

            carla_stdout("Scanning \"%s\"...", filename.toRawUTF8());
            startPipeServer(helperTool, fDiscoveryTool, getPluginTypeAsString(fPluginType), filename.toRawUTF8());
        }
    }

    void makeHash(const water::File& file, const water::String& filename)
    {
        CarlaSha1 sha1;

        /* do we want this? it is not exactly needed and makes discovery slow..
        if (file.existsAsFile() && file.getSize() < 20*1024*1024) // dont bother hashing > 20Mb files
        {
            water::FileInputStream stream(file);

            if (stream.openedOk())
            {
                uint8_t block[8192];
                for (int r; r = stream.read(block, sizeof(block)), r > 0;)
                    sha1.write(block, r);
            }
        }
        */

        sha1.write(filename.toRawUTF8(), filename.length());

        const int64_t mtime = file.getLastModificationTime();
        sha1.write(&mtime, sizeof(mtime));

        fNextSha1Sum = sha1.resultAsString();
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginDiscovery)
};

// --------------------------------------------------------------------------------------------------------------------

static bool findDirectories(std::vector<water::File>& files, const char* const pluginPath, const char* const wildcard)
{
    CARLA_SAFE_ASSERT_RETURN(pluginPath != nullptr, true);

    if (pluginPath[0] == '\0')
        return true;

    using water::File;
    using water::String;
    using water::StringArray;

    const StringArray splitPaths(StringArray::fromTokens(pluginPath, CARLA_OS_SPLIT_STR, ""));

    if (splitPaths.size() == 0)
        return true;

    for (String *it = splitPaths.begin(), *end = splitPaths.end(); it != end; ++it)
    {
        const File dir(*it);
        std::vector<File> results;

        if (dir.findChildFiles(results, File::findDirectories|File::ignoreHiddenFiles, true, wildcard) > 0)
        {
            files.reserve(files.size() + results.size());
            files.insert(files.end(), results.begin(), results.end());
        }
    }

    return files.empty();
}

static bool findFiles(std::vector<water::File>& files,
                      const BinaryType btype, const char* const pluginPath, const char* const wildcard)
{
    CARLA_SAFE_ASSERT_RETURN(pluginPath != nullptr, true);

    if (pluginPath[0] == '\0')
        return true;

    using water::File;
    using water::String;
    using water::StringArray;

    const StringArray splitPaths(StringArray::fromTokens(pluginPath, CARLA_OS_SPLIT_STR, ""));

    if (splitPaths.size() == 0)
        return true;

    for (String *it = splitPaths.begin(), *end = splitPaths.end(); it != end; ++it)
    {
        const File dir(*it);
        std::vector<File> results;

        if (dir.findChildFiles(results, File::findFiles|File::ignoreHiddenFiles, true, wildcard) > 0)
        {
            files.reserve(files.size() + results.size());

            for (std::vector<File>::const_iterator cit = results.begin(); cit != results.end(); ++cit)
            {
                const File file(*cit);

                if (CB::getBinaryTypeFromFile(file.getFullPathName().toRawUTF8()) == btype)
                    files.push_back(file);
            }
        }
    }

    return files.empty();
}

static bool findVST3s(std::vector<water::File>& files,
                      const BinaryType btype, const char* const pluginPath)
{
    CARLA_SAFE_ASSERT_RETURN(pluginPath != nullptr, true);

    if (pluginPath[0] == '\0')
        return true;

    using water::File;
    using water::String;
    using water::StringArray;

    const StringArray splitPaths(StringArray::fromTokens(pluginPath, CARLA_OS_SPLIT_STR, ""));

    if (splitPaths.size() == 0)
        return true;

    const uint flags = btype == CB::BINARY_WIN32 || btype == CB::BINARY_WIN64
                     ? File::findDirectories|File::findFiles
                     : File::findDirectories;

    for (String *it = splitPaths.begin(), *end = splitPaths.end(); it != end; ++it)
    {
        const File dir(*it);
        std::vector<File> results;

        if (dir.findChildFiles(results, flags|File::ignoreHiddenFiles, true, "*.vst3") > 0)
        {
            files.reserve(files.size() + results.size());

            for (std::vector<File>::const_iterator cit = results.begin(); cit != results.end(); ++cit)
            {
                const File file(*cit);

                if (CB::getBinaryTypeFromFile(file.getFullPathName().toRawUTF8()) == btype)
                    files.push_back(file);
            }
        }
    }

    return files.empty();
}

CarlaPluginDiscoveryHandle carla_plugin_discovery_start(const char* const discoveryTool,
                                                        const BinaryType btype,
                                                        const PluginType ptype,
                                                        const char* const pluginPath,
                                                        const CarlaPluginDiscoveryCallback discoveryCb,
                                                        const CarlaPluginCheckCacheCallback checkCacheCb,
                                                        void* const callbackPtr)
{
    CARLA_SAFE_ASSERT_RETURN(btype != CB::BINARY_NONE, nullptr);
    CARLA_SAFE_ASSERT_RETURN(ptype != CB::PLUGIN_NONE, nullptr);
    CARLA_SAFE_ASSERT_RETURN(discoveryTool != nullptr && discoveryTool[0] != '\0', nullptr);
    CARLA_SAFE_ASSERT_RETURN(discoveryCb != nullptr, nullptr);

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
        return new CarlaPluginDiscovery(discoveryTool, btype, ptype, discoveryCb, checkCacheCb, callbackPtr);
    }

    case CB::PLUGIN_INTERNAL:
    case CB::PLUGIN_LV2:
    case CB::PLUGIN_AU:
        return new CarlaPluginDiscovery(discoveryTool, btype, ptype, discoveryCb, checkCacheCb, callbackPtr);

    case CB::PLUGIN_LADSPA:
    case CB::PLUGIN_DSSI:
       #ifdef CARLA_OS_WIN
        wildcard = "*.dll";
       #else
        if (btype == CB::BINARY_WIN32 || btype == CB::BINARY_WIN64)
        {
            wildcard = "*.dll";
        }
        else
        {
           #ifdef CARLA_OS_MAC
            wildcard = "*.dylib";
           #else
            wildcard = "*.so";
           #endif
        }
       #endif
        break;

    case CB::PLUGIN_VST2:
       #ifdef CARLA_OS_WIN
        wildcard = "*.dll";
       #else
        if (btype == CB::BINARY_WIN32 || btype == CB::BINARY_WIN64)
        {
            wildcard = "*.dll";
        }
        else
        {
           #ifdef CARLA_OS_MAC
            directories = true;
            wildcard = "*.vst";
           #else
            wildcard = "*.so";
           #endif
        }
       #endif
        break;

    case CB::PLUGIN_VST3:
        directories = true;
        wildcard = "*.vst3";
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

    CARLA_SAFE_ASSERT_RETURN(wildcard != nullptr, nullptr);

    std::vector<water::File> files;

    if (ptype == CB::PLUGIN_VST3)
    {
        if (findVST3s(files, btype, pluginPath))
            return nullptr;
    }
    else if (directories)
    {
        if (findDirectories(files, pluginPath, wildcard))
            return nullptr;
    }
    else
    {
        if (findFiles(files, btype, pluginPath, wildcard))
            return nullptr;
    }

    return new CarlaPluginDiscovery(discoveryTool, btype, ptype, std::move(files),
                                    discoveryCb, checkCacheCb, callbackPtr);
}

bool carla_plugin_discovery_idle(const CarlaPluginDiscoveryHandle handle)
{
    return static_cast<CarlaPluginDiscovery*>(handle)->idle();
}

void carla_plugin_discovery_skip(const CarlaPluginDiscoveryHandle handle)
{
    static_cast<CarlaPluginDiscovery*>(handle)->skip();
}

void carla_plugin_discovery_stop(const CarlaPluginDiscoveryHandle handle)
{
    delete static_cast<CarlaPluginDiscovery*>(handle);
}

// --------------------------------------------------------------------------------------------------------------------
