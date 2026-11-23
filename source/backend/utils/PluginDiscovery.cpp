// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaUtils.h"

#include "CarlaBackendUtils.hpp"
#include "CarlaBinaryUtils.hpp"
#include "CarlaJuceUtils.hpp"
#include "CarlaPipeUtils.hpp"
#include "CarlaScopeUtils.hpp"
#include "CarlaSha1Utils.hpp"

#include "extra/Time.hpp"

#include "water/files/File.h"
#include "water/threads/ChildProcess.h"
#include "water/text/StringArray.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <thread>

namespace CB = CARLA_BACKEND_NAMESPACE;

// --------------------------------------------------------------------------------------------------------------------

#ifndef CARLA_OS_WIN
static water::String findWinePrefix(const water::String filename, const int recursionLimit = 10)
{
    if (recursionLimit == 0 || filename.length() < 5 || ! filename.contains("/"))
        return "";

    const water::String path(filename.upToLastOccurrenceOf("/", false, false));

    if (water::File(water::String(path + "/dosdevices").toRawUTF8()).isDirectory())
        return path;

    return findWinePrefix(path, recursionLimit-1);
}
#endif

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

struct CarlaPluginDiscoveryOptions {
   #if !defined(BUILD_BRIDGE_ALTERNATIVE_ARCH) && !defined(CARLA_OS_WIN)
    struct {
        bool autoPrefix;
        String executable;
        String fallbackPrefix;
    } wine;
   #endif

    static CarlaPluginDiscoveryOptions& getInstance() noexcept
    {
        static CarlaPluginDiscoveryOptions instance;
        return instance;
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CarlaPluginDiscovery
{
    class Worker : public CarlaPipeServer
    {
    public:
        Worker(CarlaPluginDiscovery& owner,
               const uint binaryIndex,
               const String& sha1Sum)
            : fOwner(owner),
              fBinaryIndex(binaryIndex),
              fPluginsFoundInBinary(false),
              fLastMessageTime(d_gettime_ms()),
              fNextSha1Sum(sha1Sum),
              fNextLabel(nullptr),
              fNextMaker(nullptr),
              fNextName(nullptr) {}

        ~Worker() override
        {
            stopPipeServer(5000);
            std::free(fNextLabel);
            std::free(fNextMaker);
            std::free(fNextName);
        }

        uint getBinaryIndex() const noexcept { return fBinaryIndex; }
        bool hasFoundPlugins() const noexcept { return fPluginsFoundInBinary; }
        uint32_t getLastMessageTime() const noexcept { return fLastMessageTime; }
        const String& getSha1Sum() const noexcept { return fNextSha1Sum; }

    protected:
        bool msgReceived(const char* const msg) noexcept override
        {
            fLastMessageTime = d_gettime_ms();

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

                if (fOwner.fBinaries.empty())
                {
                    char* filename = nullptr;

                    if (fOwner.fPluginType == CB::PLUGIN_LV2)
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

                    fNextInfo.ptype = fOwner.fPluginType;
                    fOwner.fDiscoveryCallback(fOwner.fCallbackPtr, &fNextInfo, nullptr);

                    std::free(filename);
                }
                else
                {
                    CARLA_SAFE_ASSERT(fNextSha1Sum.isNotEmpty());
                    const water::String filename(fOwner.fBinaries[fBinaryIndex].getFullPathName());
                    fNextInfo.filename = filename.toRawUTF8();
                    fNextInfo.ptype = fOwner.fPluginType;
                    fPluginsFoundInBinary = true;
                    carla_stdout("Found %s from %s", fNextInfo.metadata.name, fNextInfo.filename);
                    fOwner.fDiscoveryCallback(fOwner.fCallbackPtr, &fNextInfo, fNextSha1Sum);
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
        CarlaPluginDiscovery& fOwner;
        const uint fBinaryIndex;
        bool fPluginsFoundInBinary;
        uint32_t fLastMessageTime;
        CarlaPluginDiscoveryInfo fNextInfo;
        String fNextSha1Sum;
        char* fNextLabel;
        char* fNextMaker;
        char* fNextName;

        CARLA_DECLARE_NON_COPYABLE(Worker)
    };

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
          fPluginPath(nullptr),
          fBinaryIndex(0),
          fBinaryCount(static_cast<uint>(binaries.size())),
          fMaxWorkers(computeMaxWorkers(btype, static_cast<uint>(binaries.size()))),
          fBinaries(binaries),
          fDiscoveryTool(discoveryTool),
          fWorkers()
    {
        startMoreWorkers();
    }

    CarlaPluginDiscovery(const char* const discoveryTool,
                         const BinaryType btype,
                         const PluginType ptype,
                         const CarlaPluginDiscoveryCallback discoveryCb,
                         const CarlaPluginCheckCacheCallback checkCacheCb,
                         void* const callbackPtr,
                         const char* const pluginPath = nullptr)
        : fBinaryType(btype),
          fPluginType(ptype),
          fDiscoveryCallback(discoveryCb),
          fCheckCacheCallback(checkCacheCb),
          fCallbackPtr(callbackPtr),
          fPluginPath(pluginPath != nullptr ? carla_strdup_safe(pluginPath) : nullptr),
          fBinaryIndex(0),
          fBinaryCount(1),
          fMaxWorkers(1),
          fDiscoveryTool(discoveryTool),
          fWorkers()
    {
        startSingle();
    }

    ~CarlaPluginDiscovery()
    {
        fWorkers.clear();
        delete[] fPluginPath;
    }

    bool idle()
    {
        const uint32_t timeNow = d_gettime_ms();

        for (auto it = fWorkers.begin(); it != fWorkers.end();)
        {
            Worker& worker(**it);

            if (worker.isPipeRunning())
            {
                worker.idlePipe();

                if (worker.isPipeRunning())
                {
                    // automatically skip a plugin if 30s passes without a reply
                    if (timeNow - worker.getLastMessageTime() < 30000)
                    {
                        ++it;
                        continue;
                    }

                    carla_stdout("Plugin took too long to respond, skipping...");
                    worker.stopPipeServer(1000);
                }
            }

            // report binary as having no plugins
            if (fCheckCacheCallback != nullptr && !worker.hasFoundPlugins() && !fBinaries.empty())
            {
                const water::File file(fBinaries[worker.getBinaryIndex()]);
                const water::String filename(file.getFullPathName());
                const String sha1Sum(worker.getSha1Sum().isNotEmpty()
                                         ? worker.getSha1Sum()
                                         : makeHash(file, filename));

                if (! fCheckCacheCallback(fCallbackPtr, filename.toRawUTF8(), sha1Sum))
                    fDiscoveryCallback(fCallbackPtr, nullptr, sha1Sum);
            }

            it = fWorkers.erase(it);
        }

        if (!fBinaries.empty())
            startMoreWorkers();

        return !fWorkers.empty() || fBinaryIndex < fBinaryCount;
    }

    void skip()
    {
        for (auto& worker : fWorkers)
        {
            if (worker->isPipeRunning())
            {
                worker->stopPipeServer(1000);
                break;
            }
        }
    }

private:
    const BinaryType fBinaryType;
    const PluginType fPluginType;
    const CarlaPluginDiscoveryCallback fDiscoveryCallback;
    const CarlaPluginCheckCacheCallback fCheckCacheCallback;
    void* const fCallbackPtr;
    const char* fPluginPath;

    uint fBinaryIndex;
    const uint fBinaryCount;
    const uint fMaxWorkers;
    const std::vector<water::File> fBinaries;
    const String fDiscoveryTool;
    std::vector<std::unique_ptr<Worker>> fWorkers;

    static uint computeMaxWorkers(const BinaryType btype, const uint binaryCount) noexcept
    {
        if (binaryCount <= 1)
            return 1;

        if (const char* const envWorkers = std::getenv("CARLA_DISCOVERY_WORKERS"))
        {
            const int parsed = std::atoi(envWorkers);
            if (parsed > 0)
                return std::min(binaryCount, static_cast<uint>(parsed));
        }

        const uint hw = std::max(1u, std::thread::hardware_concurrency());

        if (btype == CB::BINARY_WIN32 || btype == CB::BINARY_WIN64)
            return std::min(binaryCount, std::max(2u, std::min(4u, hw / 2)));

        return std::min(binaryCount, std::max(2u, std::min(8u, hw)));
    }

   #ifndef CARLA_OS_WIN
    water::String resolveHelperTool() const
    {
        using water::File;
        using water::String;

        const CarlaPluginDiscoveryOptions& options(CarlaPluginDiscoveryOptions::getInstance());
        String helperTool;

        switch (fBinaryType)
        {
        case CB::BINARY_WIN32:
            if (options.wine.executable.isNotEmpty())
                helperTool = options.wine.executable.buffer();
            else
                helperTool = "wine";
            break;

        case CB::BINARY_WIN64:
            if (options.wine.executable.isNotEmpty())
            {
                helperTool = options.wine.executable.buffer();

                if (helperTool.isNotEmpty() && helperTool[0] == CARLA_OS_SEP && File(String(helperTool + "64").toRawUTF8()).existsAsFile())
                    helperTool += "64";
            }
            else
            {
                helperTool = "wine";
            }
            break;

        default:
            break;
        }

        return helperTool;
    }

    water::String resolveWinePrefix(const water::File* const file) const
    {
        using water::File;
        using water::String;

        const CarlaPluginDiscoveryOptions& options(CarlaPluginDiscoveryOptions::getInstance());
        String winePrefix;

        if (options.wine.autoPrefix && file != nullptr)
        {
            const String filename(file->getFullPathName());
            winePrefix = findWinePrefix(filename);
        }

        if (winePrefix.isEmpty())
        {
            const char* const envWinePrefix = std::getenv("WINEPREFIX");

            if (envWinePrefix != nullptr && envWinePrefix[0] != '\0')
                winePrefix = envWinePrefix;
            else if (options.wine.fallbackPrefix.isNotEmpty())
                winePrefix = options.wine.fallbackPrefix.buffer();
            else
                winePrefix = File::getSpecialLocation(File::userHomeDirectory).getFullPathName() + "/.wine";
        }

        return winePrefix;
    }
   #endif

    void startSingle()
    {
        fBinaryIndex = fBinaryCount;

        if (fBinaryType == CB::BINARY_NATIVE)
        {
            switch (fPluginType)
            {
            default:
                break;
            case CB::PLUGIN_INTERNAL:
            case CB::PLUGIN_LV2:
            case CB::PLUGIN_JSFX:
            case CB::PLUGIN_SFZ:
                if (const uint count = carla_get_cached_plugin_count(fPluginType, fPluginPath))
                {
                    for (uint i=0; i<count; ++i)
                    {
                        const CarlaCachedPluginInfo* const pinfo = carla_get_cached_plugin_info(fPluginType, i);

                        if (pinfo == nullptr || !pinfo->valid)
                            continue;

                        char* filename = nullptr;
                        CarlaPluginDiscoveryInfo info = {};
                        info.btype = CB::BINARY_NATIVE;
                        info.ptype = fPluginType;
                        info.metadata.name = pinfo->name;
                        info.metadata.maker = pinfo->maker;
                        info.metadata.category = pinfo->category;
                        info.metadata.hints = pinfo->hints;
                        info.io.audioIns = pinfo->audioIns;
                        info.io.audioOuts = pinfo->audioOuts;
                        info.io.cvIns = pinfo->cvIns;
                        info.io.cvOuts = pinfo->cvOuts;
                        info.io.midiIns = pinfo->midiIns;
                        info.io.midiOuts = pinfo->midiOuts;
                        info.io.parameterIns = pinfo->parameterIns;
                        info.io.parameterOuts = pinfo->parameterOuts;

                        if (fPluginType == CB::PLUGIN_LV2)
                        {
                            const char* const slash = std::strchr(pinfo->label, CARLA_OS_SEP);
                            CARLA_SAFE_ASSERT_BREAK(slash != nullptr);
                            filename = strdup(pinfo->label);
                            filename[slash - pinfo->label] = '\0';
                            info.filename = filename;
                            info.label = slash + 1;
                        }
                        else
                        {
                            info.filename = gPluginsDiscoveryNullCharPtr;
                            info.label = pinfo->label;
                        }

                        fDiscoveryCallback(fCallbackPtr, &info, nullptr);

                        std::free(filename);
                    }
                }
                return;
            }
        }

       #ifndef CARLA_OS_WIN
        const water::String helperTool(resolveHelperTool());
        const water::String winePrefix(resolveWinePrefix(nullptr));
        const CarlaScopedEnvVar sev1("WINEDEBUG", "-all");
        const CarlaScopedEnvVar sev2("WINEPREFIX", winePrefix.toRawUTF8());
       #endif
        const CarlaScopedEnvVar sev3("CARLA_DISCOVERY_NO_PROCESSING_CHECKS", "1");

        std::unique_ptr<Worker> worker(new Worker(*this, 0, String()));

       #ifndef CARLA_OS_WIN
        if (helperTool.isNotEmpty())
            worker->startPipeServer(helperTool.toRawUTF8(), fDiscoveryTool, getPluginTypeAsString(fPluginType), ":all", -1, 2000);
        else
       #endif
            worker->startPipeServer(fDiscoveryTool, getPluginTypeAsString(fPluginType), ":all", -1, 2000);

        if (worker->isPipeRunning())
            fWorkers.push_back(std::move(worker));
    }

    void startMoreWorkers()
    {
        using water::File;
        using water::String;

       #ifndef CARLA_OS_WIN
        const String helperTool(resolveHelperTool());
       #endif

        while (fWorkers.size() < fMaxWorkers && fBinaryIndex < fBinaryCount)
        {
            const uint binaryIndex = fBinaryIndex++;
            const File& file(fBinaries[binaryIndex]);
            const String filename(file.getFullPathName());

            ::String sha1Sum;

            if (fCheckCacheCallback != nullptr)
            {
                sha1Sum = makeHash(file, filename);

                if (fCheckCacheCallback(fCallbackPtr, filename.toRawUTF8(), sha1Sum))
                {
                    carla_debug("Skipping \"%s\", using cache", filename.toRawUTF8());
                    continue;
                }
            }

            carla_stdout("Scanning \"%s\"...", filename.toRawUTF8());

           #ifndef CARLA_OS_WIN
            const String winePrefix(resolveWinePrefix(&file));
            const CarlaScopedEnvVar sev1("WINEDEBUG", "-all");
            const CarlaScopedEnvVar sev2("WINEPREFIX", winePrefix.toRawUTF8());
           #endif
            const CarlaScopedEnvVar sev3("CARLA_DISCOVERY_NO_PROCESSING_CHECKS", "1");

            std::unique_ptr<Worker> worker(new Worker(*this, binaryIndex, sha1Sum));

           #ifndef CARLA_OS_WIN
            if (helperTool.isNotEmpty())
                worker->startPipeServer(helperTool.toRawUTF8(), fDiscoveryTool, getPluginTypeAsString(fPluginType), filename.toRawUTF8(), -1, 2000);
            else
           #endif
                worker->startPipeServer(fDiscoveryTool, getPluginTypeAsString(fPluginType), filename.toRawUTF8(), -1, 2000);

            if (worker->isPipeRunning())
            {
                fWorkers.push_back(std::move(worker));
            }
            else if (fCheckCacheCallback != nullptr)
            {
                if (sha1Sum.isEmpty())
                    sha1Sum = makeHash(file, filename);

                if (! fCheckCacheCallback(fCallbackPtr, filename.toRawUTF8(), sha1Sum))
                    fDiscoveryCallback(fCallbackPtr, nullptr, sha1Sum);
            }
        }
    }

    String makeHash(const water::File& file, const water::String& filename) const
    {
        CarlaSha1 sha1;

        sha1.write(filename.toRawUTF8(), filename.length());

        const int64_t mtime = file.getLastModificationTime();
        sha1.write(&mtime, sizeof(mtime));

       #ifdef CARLA_OS_LINUX
        if (fPluginType == CB::PLUGIN_VST3 && file.isDirectory())
        {
            const water::File binary(CB::getVst3BundleBinary(file, fBinaryType));
            const int64_t binaryMtime = binary.getLastModificationTime();
            const int64_t binarySize = binary.getSize();
            sha1.write(&fBinaryType, sizeof(fBinaryType));
            sha1.write(&binaryMtime, sizeof(binaryMtime));
            sha1.write(&binarySize, sizeof(binarySize));
        }
       #endif

        return String(sha1.resultAsString());
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
        const File dir(it->toRawUTF8());
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
        const File dir(it->toRawUTF8());
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
        const File dir(it->toRawUTF8());
        std::vector<File> results;

        if (dir.findChildFiles(results, flags|File::ignoreHiddenFiles, true, "*.vst3") > 0)
        {
            files.reserve(files.size() + results.size());

            for (std::vector<File>::const_iterator cit = results.begin(); cit != results.end(); ++cit)
            {
                const File file(*cit);

               #ifdef CARLA_OS_LINUX
                if (file.isDirectory())
                {
                    if (CB::getVst3BundleBinary(file, btype).existsAsFile())
                        files.push_back(file);
                    continue;
                }

                const File bundle(file.getParentDirectory().getParentDirectory().getParentDirectory());
                if (bundle.hasFileExtension("vst3") && file == CB::getVst3BundleBinary(bundle, btype))
                    continue;
               #endif

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
    carla_debug("carla_plugin_discovery_start(%s, %d:%s, %d:%s, %s, %p, %p, %p)",
                discoveryTool, btype, BinaryType2Str(btype), ptype, PluginType2Str(ptype), pluginPath,
                discoveryCb, checkCacheCb, callbackPtr);

    bool directories = false;
    const char* wildcard = nullptr;

    switch (ptype)
    {
    case CB::PLUGIN_INTERNAL:
    case CB::PLUGIN_LV2:
    case CB::PLUGIN_SFZ:
    case CB::PLUGIN_JSFX:
    case CB::PLUGIN_DLS:
    case CB::PLUGIN_GIG:
    case CB::PLUGIN_SF2:
        CARLA_SAFE_ASSERT_UINT_RETURN(btype == CB::BINARY_NATIVE, btype, nullptr);
        break;
    default:
        break;
    }

    switch (ptype)
    {
    case CB::PLUGIN_NONE:
    case CB::PLUGIN_JACK:
    case CB::PLUGIN_TYPE_COUNT:
        return nullptr;

    case CB::PLUGIN_LV2:
    case CB::PLUGIN_SFZ:
    case CB::PLUGIN_JSFX:
    {
        const CarlaScopedEnvVar csev("CARLA_DISCOVERY_PATH", pluginPath);
        return new CarlaPluginDiscovery(discoveryTool, btype, ptype, discoveryCb, checkCacheCb, callbackPtr, pluginPath);
    }

    case CB::PLUGIN_INTERNAL:
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

    case CB::PLUGIN_AU:
        directories = true;
        wildcard = "*.component";
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

void carla_plugin_discovery_set_option(const EngineOption option, const int value, const char* const valueStr)
{
    switch (option)
    {
   #if !defined(BUILD_BRIDGE_ALTERNATIVE_ARCH) && !defined(CARLA_OS_WIN)
    case CB::ENGINE_OPTION_WINE_EXECUTABLE:
        if (valueStr != nullptr && valueStr[0] != '\0')
            CarlaPluginDiscoveryOptions::getInstance().wine.executable = valueStr;
        else
            CarlaPluginDiscoveryOptions::getInstance().wine.executable.clear();
        break;
    case CB::ENGINE_OPTION_WINE_AUTO_PREFIX:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        CarlaPluginDiscoveryOptions::getInstance().wine.autoPrefix = value != 0;
        break;
    case CB::ENGINE_OPTION_WINE_FALLBACK_PREFIX:
        if (valueStr != nullptr && valueStr[0] != '\0')
            CarlaPluginDiscoveryOptions::getInstance().wine.fallbackPrefix = valueStr;
        else
            CarlaPluginDiscoveryOptions::getInstance().wine.fallbackPrefix.clear();
        break;
   #endif
    default:
        break;
    }
}

// --------------------------------------------------------------------------------------------------------------------
