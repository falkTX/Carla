/*
 * Carla Plugin Bridge
 * Copyright (C) 2011-2018 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBackendUtils.hpp"
#include "CarlaBase64Utils.hpp"
#include "CarlaBridgeUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaPipeUtils.hpp"
#include "CarlaShmUtils.hpp"
#include "CarlaThread.hpp"

#include "jackbridge/JackBridge.hpp"

#include <ctime>

#include "water/files/File.h"
#include "water/misc/Time.h"
#include "water/threads/ChildProcess.h"

// ---------------------------------------------------------------------------------------------------------------------

using water::ChildProcess;
using water::File;
using water::String;
using water::StringArray;
using water::Time;

CARLA_BACKEND_START_NAMESPACE

// ---------------------------------------------------------------------------------------------------------------------
// Fallback data

static const ExternalMidiNote kExternalMidiNoteFallback = { -1, 0, 0 };

// ---------------------------------------------------------------------------------------------------------------------

static String findWinePrefix(const String filename, const int recursionLimit = 10)
{
    if (recursionLimit == 0 || filename.length() < 5 || ! filename.contains("/"))
        return "";

    const String path(filename.upToLastOccurrenceOf("/", false, false));

    if (File(path + "/dosdevices").isDirectory())
        return path;

    return findWinePrefix(path, recursionLimit-1);
}

// ---------------------------------------------------------------------------------------------------------------------

struct BridgeParamInfo {
    float value;
    CarlaString name;
    CarlaString symbol;
    CarlaString unit;

    BridgeParamInfo() noexcept
        : value(0.0f),
          name(),
          symbol(),
          unit() {}

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeParamInfo)
};

// ---------------------------------------------------------------------------------------------------------------------

class CarlaPluginBridgeThread : public CarlaThread
{
public:
    CarlaPluginBridgeThread(CarlaEngine* const engine, CarlaPlugin* const plugin) noexcept
        : CarlaThread("CarlaPluginBridgeThread"),
          kEngine(engine),
          kPlugin(plugin),
          fBinary(),
          fLabel(),
          fShmIds(),
#ifndef CARLA_OS_WIN
          fWinePrefix(),
#endif
          fProcess() {}

    void setData(
#ifndef CARLA_OS_WIN
                 const char* const winePrefix,
#endif
                 const char* const binary,
                 const char* const label,
                 const char* const shmIds) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(binary != nullptr && binary[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(shmIds != nullptr && shmIds[0] != '\0',);
        CARLA_SAFE_ASSERT(! isThreadRunning());

#ifndef CARLA_OS_WIN
        fWinePrefix = winePrefix;
#endif
        fBinary = binary;
        fShmIds = shmIds;

        if (label != nullptr)
            fLabel = label;
        if (fLabel.isEmpty())
            fLabel = "\"\"";
    }

    uintptr_t getProcessPID() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fProcess != nullptr, 0);

        return (uintptr_t)fProcess->getPID();
    }

protected:
    void run()
    {
        if (fProcess == nullptr)
        {
            fProcess = new ChildProcess();
        }
        else if (fProcess->isRunning())
        {
            carla_stderr("CarlaPluginBridgeThread::run() - already running");
        }

        char strBuf[STR_MAX+1];
        strBuf[STR_MAX] = '\0';

        const EngineOptions& options(kEngine->getOptions());

        String name(kPlugin->getName());
        String filename(kPlugin->getFilename());

        if (name.isEmpty())
            name = "(none)";

        if (filename.isEmpty())
            filename = "\"\"";

        StringArray arguments;

#ifndef CARLA_OS_WIN
        // start with "wine" if needed
        if (fBinary.endsWithIgnoreCase(".exe"))
        {
            String wineCMD;

            if (options.wine.executable != nullptr && options.wine.executable[0] != '\0')
            {
                wineCMD = options.wine.executable;

                if (fBinary.endsWithIgnoreCase("64.exe")
                    && options.wine.executable[0] == CARLA_OS_SEP
                    && File(wineCMD + "64").existsAsFile())
                    wineCMD += "64";
            }
            else
            {
                wineCMD = "wine";
            }

            arguments.add(wineCMD);
        }
#endif

        // binary
        arguments.add(fBinary);

        // plugin type
        arguments.add(getPluginTypeAsString(kPlugin->getType()));

        // filename
        arguments.add(filename);

        // label
        arguments.add(fLabel);

        // uniqueId
        arguments.add(String(static_cast<water::int64>(kPlugin->getUniqueId())));

        bool started;

        {
            const ScopedEngineEnvironmentLocker _seel(kEngine);

#ifdef CARLA_OS_LINUX
            const ScopedEnvVar sev1("LD_LIBRARY_PATH", nullptr);
            const ScopedEnvVar sev2("LD_PRELOAD", nullptr);
#endif

            carla_setenv("ENGINE_OPTION_FORCE_STEREO",          bool2str(options.forceStereo));
            carla_setenv("ENGINE_OPTION_PREFER_PLUGIN_BRIDGES", bool2str(options.preferPluginBridges));
            carla_setenv("ENGINE_OPTION_PREFER_UI_BRIDGES",     bool2str(options.preferUiBridges));
            carla_setenv("ENGINE_OPTION_UIS_ALWAYS_ON_TOP",     bool2str(options.uisAlwaysOnTop));

            std::snprintf(strBuf, STR_MAX, "%u", options.maxParameters);
            carla_setenv("ENGINE_OPTION_MAX_PARAMETERS", strBuf);

            std::snprintf(strBuf, STR_MAX, "%u", options.uiBridgesTimeout);
            carla_setenv("ENGINE_OPTION_UI_BRIDGES_TIMEOUT",strBuf);

            if (options.pathLADSPA != nullptr)
                carla_setenv("ENGINE_OPTION_PLUGIN_PATH_LADSPA", options.pathLADSPA);
            else
                carla_setenv("ENGINE_OPTION_PLUGIN_PATH_LADSPA", "");

            if (options.pathDSSI != nullptr)
                carla_setenv("ENGINE_OPTION_PLUGIN_PATH_DSSI", options.pathDSSI);
            else
                carla_setenv("ENGINE_OPTION_PLUGIN_PATH_DSSI", "");

            if (options.pathLV2 != nullptr)
                carla_setenv("ENGINE_OPTION_PLUGIN_PATH_LV2", options.pathLV2);
            else
                carla_setenv("ENGINE_OPTION_PLUGIN_PATH_LV2", "");

            if (options.pathVST2 != nullptr)
                carla_setenv("ENGINE_OPTION_PLUGIN_PATH_VST2", options.pathVST2);
            else
                carla_setenv("ENGINE_OPTION_PLUGIN_PATH_VST2", "");

            if (options.pathSF2 != nullptr)
                carla_setenv("ENGINE_OPTION_PLUGIN_PATH_SF2", options.pathSF2);
            else
                carla_setenv("ENGINE_OPTION_PLUGIN_PATH_SF2", "");

            if (options.pathSFZ != nullptr)
                carla_setenv("ENGINE_OPTION_PLUGIN_PATH_SFZ", options.pathSFZ);
            else
                carla_setenv("ENGINE_OPTION_PLUGIN_PATH_SFZ", "");

            if (options.binaryDir != nullptr)
                carla_setenv("ENGINE_OPTION_PATH_BINARIES", options.binaryDir);
            else
                carla_setenv("ENGINE_OPTION_PATH_BINARIES", "");

            if (options.resourceDir != nullptr)
                carla_setenv("ENGINE_OPTION_PATH_RESOURCES", options.resourceDir);
            else
                carla_setenv("ENGINE_OPTION_PATH_RESOURCES", "");

            carla_setenv("ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR", bool2str(options.preventBadBehaviour));

            std::snprintf(strBuf, STR_MAX, P_UINTPTR, options.frontendWinId);
            carla_setenv("ENGINE_OPTION_FRONTEND_WIN_ID", strBuf);

            carla_setenv("ENGINE_BRIDGE_SHM_IDS", fShmIds.toRawUTF8());

#ifndef CARLA_OS_WIN
            if (fWinePrefix.isNotEmpty())
            {
                carla_setenv("WINEDEBUG", "-all");
                carla_setenv("WINEPREFIX", fWinePrefix.toRawUTF8());

                if (options.wine.rtPrio)
                {
                    carla_setenv("STAGING_SHARED_MEMORY", "1");
                    carla_setenv("WINE_RT_POLICY", "FF");

                    std::snprintf(strBuf, STR_MAX, "%i", options.wine.baseRtPrio);
                    carla_setenv("STAGING_RT_PRIORITY_BASE", strBuf);
                    carla_setenv("WINE_RT", strBuf);
                    carla_setenv("WINE_RT_PRIO", strBuf);

                    std::snprintf(strBuf, STR_MAX, "%i", options.wine.serverRtPrio);
                    carla_setenv("STAGING_RT_PRIORITY_SERVER", strBuf);
                    carla_setenv("WINE_SVR_RT", strBuf);

                    carla_stdout("Using WINEPREFIX '%s', with base RT prio %i and server RT prio %i",
                                fWinePrefix.toRawUTF8(), options.wine.baseRtPrio, options.wine.serverRtPrio);
                }
                else
                {
                    carla_unsetenv("STAGING_SHARED_MEMORY");
                    carla_unsetenv("WINE_RT_POLICY");
                    carla_unsetenv("STAGING_RT_PRIORITY_BASE");
                    carla_unsetenv("STAGING_RT_PRIORITY_SERVER");
                    carla_unsetenv("WINE_RT");
                    carla_unsetenv("WINE_RT_PRIO");
                    carla_unsetenv("WINE_SVR_RT");

                    carla_stdout("Using WINEPREFIX '%s', without RT priorities", fWinePrefix.toRawUTF8());
                }
            }
#endif

            carla_stdout("Starting plugin bridge, command is:\n%s \"%s\" \"%s\" \"%s\" " P_INT64,
                         fBinary.toRawUTF8(), getPluginTypeAsString(kPlugin->getType()), filename.toRawUTF8(), fLabel.toRawUTF8(), kPlugin->getUniqueId());

            started = fProcess->start(arguments);
        }

        if (! started)
        {
            carla_stdout("failed!");
            fProcess = nullptr;
            return;
        }

        for (; fProcess->isRunning() && ! shouldThreadExit();)
            carla_sleep(1);

        // we only get here if bridge crashed or thread asked to exit
        if (fProcess->isRunning() && shouldThreadExit())
        {
            fProcess->waitForProcessToFinish(2000);

            if (fProcess->isRunning())
            {
                carla_stdout("CarlaPluginBridgeThread::run() - bridge refused to close, force kill now");
                fProcess->kill();
            }
            else
            {
                carla_stdout("CarlaPluginBridgeThread::run() - bridge auto-closed successfully");
            }
        }
        else
        {
            // forced quit, may have crashed
            if (fProcess->getExitCode() != 0 /*|| fProcess->exitStatus() == QProcess::CrashExit*/)
            {
                carla_stderr("CarlaPluginBridgeThread::run() - bridge crashed");

                CarlaString errorString("Plugin '" + CarlaString(kPlugin->getName()) + "' has crashed!\n"
                                        "Saving now will lose its current settings.\n"
                                        "Please remove this plugin, and not rely on it from this point.");
                kEngine->callback(CarlaBackend::ENGINE_CALLBACK_ERROR, kPlugin->getId(), 0, 0, 0.0f, errorString);
            }
        }

        fProcess = nullptr;
    }

private:
    CarlaEngine* const kEngine;
    CarlaPlugin* const kPlugin;

    String fBinary;
    String fLabel;
    String fShmIds;
#ifndef CARLA_OS_WIN
    String fWinePrefix;
#endif

    ScopedPointer<ChildProcess> fProcess;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginBridgeThread)
};

// ---------------------------------------------------------------------------------------------------------------------

class CarlaPluginBridge : public CarlaPlugin
{
public:
    CarlaPluginBridge(CarlaEngine* const engine, const uint id, const BinaryType btype, const PluginType ptype)
        : CarlaPlugin(engine, id),
          fBinaryType(btype),
          fPluginType(ptype),
          fInitiated(false),
          fInitError(false),
          fSaved(true),
          fTimedOut(false),
          fTimedError(false),
          fProcWaitTime(0),
          fLastPongTime(-1),
          fBridgeBinary(),
          fBridgeThread(engine, this),
          fShmAudioPool(),
          fShmRtClientControl(),
          fShmNonRtClientControl(),
          fShmNonRtServerControl(),
#ifndef CARLA_OS_WIN
          fWinePrefix(),
#endif
          fReceivingParamText(),
          fInfo(),
          fUniqueId(0),
          fLatency(0),
          fParams(nullptr)
    {
        carla_debug("CarlaPluginBridge::CarlaPluginBridge(%p, %i, %s, %s)", engine, id, BinaryType2Str(btype), PluginType2Str(ptype));

        pData->hints |= PLUGIN_IS_BRIDGE;
    }

    ~CarlaPluginBridge() override
    {
        carla_debug("CarlaPluginBridge::~CarlaPluginBridge()");

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
            pData->transientTryCounter = 0;
#endif

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fBridgeThread.isThreadRunning())
        {
            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientQuit);
            fShmNonRtClientControl.commitWrite();

            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientQuit);
            fShmRtClientControl.commitWrite();

            if (! fTimedOut)
                waitForClient("stopping", 3000);
        }

        fBridgeThread.stopThread(3000);

        fShmNonRtServerControl.clear();
        fShmNonRtClientControl.clear();
        fShmRtClientControl.clear();
        fShmAudioPool.clear();

        clearBuffers();

        fInfo.chunk.clear();
    }

    // -------------------------------------------------------------------
    // Information (base)

    BinaryType getBinaryType() const noexcept override
    {
        return fBinaryType;
    }

    PluginType getType() const noexcept override
    {
        return fPluginType;
    }

    PluginCategory getCategory() const noexcept override
    {
        return fInfo.category;
    }

    int64_t getUniqueId() const noexcept override
    {
        return fUniqueId;
    }

    uint32_t getLatencyInFrames() const noexcept override
    {
        return fLatency;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getMidiInCount() const noexcept override
    {
        return fInfo.mIns;
    }

    uint32_t getMidiOutCount() const noexcept override
    {
        return fInfo.mOuts;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    std::size_t getChunkData(void** const dataPtr) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS, 0);
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);

        waitForSaved();

        CARLA_SAFE_ASSERT_RETURN(fInfo.chunk.size() > 0, 0);

#ifdef CARLA_PROPER_CPP11_SUPPORT
        *dataPtr = fInfo.chunk.data();
#else
        *dataPtr = &fInfo.chunk.front();
#endif
        return fInfo.chunk.size();
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        return fInfo.optionsAvailable;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        return fParams[parameterId].value;
    }

    void getLabel(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fInfo.label, STR_MAX);
    }

    void getMaker(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fInfo.maker, STR_MAX);
    }

    void getCopyright(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fInfo.copyright, STR_MAX);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fInfo.name, STR_MAX);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, nullStrBuf(strBuf));

        std::strncpy(strBuf, fParams[parameterId].name.buffer(), STR_MAX);
    }

    void getParameterText(const uint32_t parameterId, char* const strBuf) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(! fReceivingParamText.isCurrentlyWaitingData(), nullStrBuf(strBuf));

        const int32_t parameterIdi = static_cast<int32_t>(parameterId);
        fReceivingParamText.setTargetData(parameterIdi, strBuf);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientGetParameterText);
            fShmNonRtClientControl.writeInt(parameterIdi);
            fShmNonRtClientControl.commitWrite();
        }

        if (! waitForParameterText())
            std::snprintf(strBuf, STR_MAX, "%f", fParams[parameterId].value);
    }

    void getParameterSymbol(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, nullStrBuf(strBuf));

        std::strncpy(strBuf, fParams[parameterId].symbol.buffer(), STR_MAX);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, nullStrBuf(strBuf));

        std::strncpy(strBuf, fParams[parameterId].unit.buffer(), STR_MAX);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    void prepareForSave() noexcept override
    {
        fSaved = false;

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientPrepareForSave);
            fShmNonRtClientControl.commitWrite();
        }
    }

    bool waitForParameterText()
    {
        bool success;
        if (fReceivingParamText.wasDataReceived(&success))
            return success;

        const uint32_t timeoutEnd(Time::getMillisecondCounter() + 500); // 500 ms

        for (; Time::getMillisecondCounter() < timeoutEnd && fBridgeThread.isThreadRunning();)
        {
            if (fReceivingParamText.wasDataReceived(&success))
                return success;

            carla_msleep(5);
        }

        carla_stderr("CarlaPluginBridge::waitForParameterText() - Timeout while requesting text");

#if 0
        // we waited and blocked for 5 secs, give host idle time now
        pData->engine->callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

        if (pData->engine->getType() != kEngineTypePlugin)
            pData->engine->idle();
#endif

        return false;
    }

    void waitForSaved()
    {
        if (fSaved)
            return;

        // TODO: only wait 1 minute for NI plugins
        const uint32_t timeoutEnd(Time::getMillisecondCounter() + 60*1000); // 60 secs, 1 minute
        const bool needsEngineIdle(pData->engine->getType() != kEngineTypePlugin);

        for (; Time::getMillisecondCounter() < timeoutEnd && fBridgeThread.isThreadRunning();)
        {
            pData->engine->callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

            if (needsEngineIdle)
                pData->engine->idle();

            if (fSaved)
                break;

            carla_msleep(20);
        }

        if (! fBridgeThread.isThreadRunning())
            return carla_stderr("CarlaPluginBridge::waitForSaved() - Bridge is not running");
        if (! fSaved)
            return carla_stderr("CarlaPluginBridge::waitForSaved() - Timeout while requesting save state");
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setOption(const uint option, const bool yesNo, const bool sendCallback) override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetOption);
            fShmNonRtClientControl.writeUInt(option);
            fShmNonRtClientControl.writeBool(yesNo);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setOption(option, yesNo, sendCallback);
    }

    void setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetCtrlChannel);
            fShmNonRtClientControl.writeShort(channel);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setCtrlChannel(channel, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(sendGui || sendOsc || sendCallback,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fParams[parameterId].value = fixedValue;

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetParameterValue);
            fShmNonRtClientControl.writeUInt(parameterId);
            fShmNonRtClientControl.writeFloat(value);
            fShmNonRtClientControl.commitWrite();
            fShmNonRtClientControl.waitIfDataIsReachingLimit();
        }

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setParameterMidiChannel(const uint32_t parameterId, const uint8_t channel, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetParameterMidiChannel);
            fShmNonRtClientControl.writeUInt(parameterId);
            fShmNonRtClientControl.writeByte(channel);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setParameterMidiChannel(parameterId, channel, sendOsc, sendCallback);
    }

    void setParameterMidiCC(const uint32_t parameterId, const int16_t cc, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(cc >= -1 && cc < MAX_MIDI_CONTROL,);
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetParameterMidiCC);
            fShmNonRtClientControl.writeUInt(parameterId);
            fShmNonRtClientControl.writeShort(cc);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setParameterMidiCC(parameterId, cc, sendOsc, sendCallback);
    }

    void setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool doingInit) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->prog.count),);
        CARLA_SAFE_ASSERT_RETURN(sendGui || sendOsc || sendCallback || doingInit,);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetProgram);
            fShmNonRtClientControl.writeInt(index);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setProgram(index, sendGui, sendOsc, sendCallback, doingInit);
    }

    void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool doingInit) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);
        CARLA_SAFE_ASSERT_RETURN(sendGui || sendOsc || sendCallback || doingInit,);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetMidiProgram);
            fShmNonRtClientControl.writeInt(index);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, doingInit);
    }

    void setMidiProgramRT(const uint32_t uindex) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(uindex < pData->midiprog.count,);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetMidiProgram);
            fShmNonRtClientControl.writeInt(static_cast<int32_t>(uindex));
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setMidiProgramRT(uindex);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

        if (std::strcmp(type, CUSTOM_DATA_TYPE_PROPERTY) == 0)
            return CarlaPlugin::setCustomData(type, key, value, sendGui);

        if (std::strcmp(type, CUSTOM_DATA_TYPE_STRING) == 0 && std::strcmp(key, "__CarlaPingOnOff__") == 0)
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientPingOnOff);
            fShmNonRtClientControl.writeBool(std::strcmp(value, "true") == 0);
            fShmNonRtClientControl.commitWrite();
            return;
        }

        const uint32_t typeLen(static_cast<uint32_t>(std::strlen(type)));
        const uint32_t keyLen(static_cast<uint32_t>(std::strlen(key)));
        const uint32_t valueLen(static_cast<uint32_t>(std::strlen(value)));

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetCustomData);

            fShmNonRtClientControl.writeUInt(typeLen);
            fShmNonRtClientControl.writeCustomData(type, typeLen);

            fShmNonRtClientControl.writeUInt(keyLen);
            fShmNonRtClientControl.writeCustomData(key, keyLen);

            fShmNonRtClientControl.writeUInt(valueLen);

            if (valueLen > 0)
                fShmNonRtClientControl.writeCustomData(value, valueLen);

            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setChunkData(const void* const data, const std::size_t dataSize) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(dataSize > 0,);

        CarlaString dataBase64(CarlaString::asBase64(data, dataSize));
        CARLA_SAFE_ASSERT_RETURN(dataBase64.length() > 0,);

        String filePath(File::getSpecialLocation(File::tempDirectory).getFullPathName());

        filePath += CARLA_OS_SEP_STR ".CarlaChunk_";
        filePath += fShmAudioPool.getFilenameSuffix();

        if (File(filePath).replaceWithText(dataBase64.buffer()))
        {
            const uint32_t ulength(static_cast<uint32_t>(filePath.length()));

            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetChunkDataFile);
            fShmNonRtClientControl.writeUInt(ulength);
            fShmNonRtClientControl.writeCustomData(filePath.toRawUTF8(), ulength);
            fShmNonRtClientControl.commitWrite();
        }

        // save data internally as well
        fInfo.chunk.resize(dataSize);
#ifdef CARLA_PROPER_CPP11_SUPPORT
        std::memcpy(fInfo.chunk.data(), data, dataSize);
#else
        std::memcpy(&fInfo.chunk.front(), data, dataSize);
#endif
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(yesNo ? kPluginBridgeNonRtClientShowUI : kPluginBridgeNonRtClientHideUI);
            fShmNonRtClientControl.commitWrite();
        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (yesNo)
        {
            pData->tryTransient();
        }
        else
        {
            pData->transientTryCounter = 0;
        }
#endif
    }

    void idle() override
    {
        if (fBridgeThread.isThreadRunning())
        {
            if (fInitiated && fTimedOut && pData->active)
                setActive(false, true, true);

            {
                const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

                fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientPing);
                fShmNonRtClientControl.commitWrite();
            }

            try {
                handleNonRtData();
            } CARLA_SAFE_EXCEPTION("handleNonRtData");
        }
        else if (fInitiated)
        {
            fTimedOut   = true;
            fTimedError = true;
            fInitiated  = false;
            handleProcessStopped();
        }

        CarlaPlugin::idle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        carla_debug("CarlaPluginBridge::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        // cleanup of previous data
        pData->audioIn.clear();
        pData->audioOut.clear();
        pData->cvIn.clear();
        pData->cvOut.clear();
        pData->event.clear();

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        if (fInfo.aIns > 0)
        {
            pData->audioIn.createNew(fInfo.aIns);
        }

        if (fInfo.aOuts > 0)
        {
            pData->audioOut.createNew(fInfo.aOuts);
            needsCtrlIn = true;
        }

        if (fInfo.cvIns > 0)
        {
            pData->cvIn.createNew(fInfo.cvIns);
        }

        if (fInfo.cvOuts > 0)
        {
            pData->cvOut.createNew(fInfo.cvOuts);
        }

        if (fInfo.mIns > 0)
            needsCtrlIn = true;

        if (fInfo.mOuts > 0)
            needsCtrlOut = true;

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        // Audio Ins
        for (uint32_t j=0; j < fInfo.aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (fInfo.aInNames != nullptr && fInfo.aInNames[j] != nullptr)
            {
                portName += fInfo.aInNames[j];
            }
            else if (fInfo.aIns > 1)
            {
                portName += "input_";
                portName += CarlaString(j+1);
            }
            else
                portName += "input";

            portName.truncate(portNameSize);

            pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, j);
            pData->audioIn.ports[j].rindex = j;
        }

        // Audio Outs
        for (uint32_t j=0; j < fInfo.aOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (fInfo.aOutNames != nullptr && fInfo.aOutNames[j] != nullptr)
            {
                portName += fInfo.aOutNames[j];
            }
            else if (fInfo.aOuts > 1)
            {
                portName += "output_";
                portName += CarlaString(j+1);
            }
            else
                portName += "output";

            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, j);
            pData->audioOut.ports[j].rindex = j;
        }

        // TODO - MIDI

        // TODO - CV

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "event-in";
            portName.truncate(portNameSize);

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, 0);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "event-out";
            portName.truncate(portNameSize);

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, 0);
        }

        // extra plugin hints
        pData->extraHints = 0x0;

        if (fInfo.mIns > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (fInfo.mOuts > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_OUT;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        carla_debug("CarlaPluginBridge::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        if (! fBridgeThread.isThreadRunning())
        {
            CARLA_SAFE_ASSERT_RETURN(restartBridgeThread(),);
        }

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientActivate);
            fShmNonRtClientControl.commitWrite();
        }

        fTimedOut = false;

        try {
            waitForClient("activate", 2000);
        } CARLA_SAFE_EXCEPTION("activate - waitForClient");
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(! fTimedError,);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientDeactivate);
            fShmNonRtClientControl.commitWrite();
        }

        fTimedOut = false;

        try {
            waitForClient("deactivate", 2000);
        } CARLA_SAFE_EXCEPTION("deactivate - waitForClient");
    }

    void process(const float** const audioIn, float** const audioOut, const float** const cvIn, float** const cvOut, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (fTimedOut || fTimedError || ! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
                carla_zeroFloats(cvOut[i], frames);
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            // TODO

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                for (RtLinkedList<ExternalMidiNote>::Itenerator it = pData->extNotes.data.begin2(); it.valid(); it.next())
                {
                    const ExternalMidiNote& note(it.getValue(kExternalMidiNoteFallback));
                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    uint8_t data1, data2, data3;
                    data1 = uint8_t((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT));
                    data2 = note.note;
                    data3 = note.velo;

                    fShmRtClientControl.writeOpcode(kPluginBridgeRtClientMidiEvent);
                    fShmRtClientControl.writeUInt(0); // time
                    fShmRtClientControl.writeByte(0); // port
                    fShmRtClientControl.writeByte(3); // size
                    fShmRtClientControl.writeByte(data1);
                    fShmRtClientControl.writeByte(data2);
                    fShmRtClientControl.writeByte(data3);
                    fShmRtClientControl.commitWrite();
                }

                pData->extNotes.data.clear();
                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            bool allNotesOffSent = false;
#endif

            for (uint32_t i=0, numEvents=pData->event.portIn->getEventCount(); i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                // Control change
                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl: {
                    const EngineControlEvent& ctrlEvent = event.ctrl;

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter:
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                value = ctrlEvent.value;
                                setDryWetRT(value);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolumeRT(value);
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
                            {
                                float left, right;
                                value = ctrlEvent.value/0.5f - 1.0f;

                                if (value < 0.0f)
                                {
                                    left  = -1.0f;
                                    right = (value*2.0f)+1.0f;
                                }
                                else if (value > 0.0f)
                                {
                                    left  = (value*2.0f)-1.0f;
                                    right = 1.0f;
                                }
                                else
                                {
                                    left  = -1.0f;
                                    right = 1.0f;
                                }

                                setBalanceLeftRT(left);
                                setBalanceRightRT(right);
                            }
                        }
#endif
                        fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventParameter);
                        fShmRtClientControl.writeUInt(event.time);
                        fShmRtClientControl.writeByte(event.channel);
                        fShmRtClientControl.writeUShort(event.ctrl.param);
                        fShmRtClientControl.writeFloat(event.ctrl.value);
                        fShmRtClientControl.commitWrite();
                        break;

                    case kEngineControlEventTypeMidiBank:
                        if (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
                        {
                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventMidiBank);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(event.channel);
                            fShmRtClientControl.writeUShort(event.ctrl.param);
                            fShmRtClientControl.commitWrite();
                        }
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
                        {
                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventMidiProgram);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(event.channel);
                            fShmRtClientControl.writeUShort(event.ctrl.param);
                            fShmRtClientControl.commitWrite();
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventAllSoundOff);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(event.channel);
                            fShmRtClientControl.commitWrite();
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }
#endif

                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventAllNotesOff);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(event.channel);
                            fShmRtClientControl.commitWrite();
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size == 0 || midiEvent.size >= MAX_MIDI_VALUE)
                        continue;

                    const uint8_t* const midiData(midiEvent.size > EngineMidiEvent::kDataSize ? midiEvent.dataExt : midiEvent.data);

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiData));

                    if (status == MIDI_STATUS_CHANNEL_PRESSURE && (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (status == MIDI_STATUS_CONTROL_CHANGE && (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH && (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (status == MIDI_STATUS_PITCH_WHEEL_CONTROL && (pData->options & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    // Fix bad note-off
                    if (status == MIDI_STATUS_NOTE_ON && midiData[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    fShmRtClientControl.writeOpcode(kPluginBridgeRtClientMidiEvent);
                    fShmRtClientControl.writeUInt(event.time);
                    fShmRtClientControl.writeByte(midiEvent.port);
                    fShmRtClientControl.writeByte(midiEvent.size);

                    fShmRtClientControl.writeByte(uint8_t(midiData[0] | (event.channel & MIDI_CHANNEL_BIT)));

                    for (uint8_t j=1; j < midiEvent.size; ++j)
                        fShmRtClientControl.writeByte(midiData[j]);

                    fShmRtClientControl.commitWrite();

                    if (status == MIDI_STATUS_NOTE_ON)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, event.channel, midiData[1], midiData[2]);
                    else if (status == MIDI_STATUS_NOTE_OFF)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, event.channel, midiData[1], 0.0f);
                } break;
                }
            }

            pData->postRtEvents.trySplice();

        } // End of Event Input

        if (! processSingle(audioIn, audioOut, cvIn, cvOut, frames))
            return;

        // --------------------------------------------------------------------------------------------------------
        // Control and MIDI Output

        if (pData->event.portOut != nullptr)
        {
            float value;

            for (uint32_t k=0; k < pData->param.count; ++k)
            {
                if (pData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;

                if (pData->param.data[k].midiCC > 0)
                {
                    value = pData->param.ranges[k].getNormalizedValue(fParams[k].value);
                    pData->event.portOut->writeControlEvent(0, pData->param.data[k].midiChannel, kEngineControlEventTypeParameter, static_cast<uint16_t>(pData->param.data[k].midiCC), value);
                }
            }

            uint32_t time;
            uint8_t port, size;
            const uint8_t* midiData(fShmRtClientControl.data->midiOut);

            for (std::size_t read=0; read<kBridgeRtClientDataMidiOutSize-kBridgeBaseMidiOutHeaderSize;)
            {
                // get time
                time = *(const uint32_t*)midiData;
                midiData += 4;

                // get port and size
                port = *midiData++;
                size = *midiData++;

                if (size == 0)
                    break;

                // store midi data advancing as needed
                uint8_t data[size];

                for (uint8_t j=0; j<size; ++j)
                    data[j] = *midiData++;

                pData->event.portOut->writeMidiEvent(time, size, data);

                read += kBridgeBaseMidiOutHeaderSize + size;
            }

            // TODO
            (void)port;

        } // End of Control and MIDI Output
    }

    bool processSingle(const float** const audioIn, float** const audioOut,
                       const float** const cvIn, float** const cvOut, const uint32_t frames)
    {
        CARLA_SAFE_ASSERT_RETURN(! fTimedError, false);
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioIn != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioOut != nullptr, false);
        }
        if (pData->cvIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(cvIn != nullptr, false);
        }
        if (pData->cvOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(cvOut != nullptr, false);
        }

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

#ifndef STOAT_TEST_BUILD
        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else
#endif
        if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
                carla_zeroFloats(cvOut[i], frames);
            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        for (uint32_t i=0; i < fInfo.aIns; ++i)
            carla_copyFloats(fShmAudioPool.data + (i * frames), audioIn[i], frames);

        // --------------------------------------------------------------------------------------------------------
        // TimeInfo

        const EngineTimeInfo timeInfo(pData->engine->getTimeInfo());
        BridgeTimeInfo& bridgeTimeInfo(fShmRtClientControl.data->timeInfo);

        bridgeTimeInfo.playing    = timeInfo.playing;
        bridgeTimeInfo.frame      = timeInfo.frame;
        bridgeTimeInfo.usecs      = timeInfo.usecs;
        bridgeTimeInfo.validFlags = timeInfo.bbt.valid ? kPluginBridgeTimeInfoValidBBT : 0x0;

        if (timeInfo.bbt.valid)
        {
            bridgeTimeInfo.bar  = timeInfo.bbt.bar;
            bridgeTimeInfo.beat = timeInfo.bbt.beat;
            bridgeTimeInfo.tick = timeInfo.bbt.tick;

            bridgeTimeInfo.beatsPerBar = timeInfo.bbt.beatsPerBar;
            bridgeTimeInfo.beatType    = timeInfo.bbt.beatType;

            bridgeTimeInfo.ticksPerBeat   = timeInfo.bbt.ticksPerBeat;
            bridgeTimeInfo.beatsPerMinute = timeInfo.bbt.beatsPerMinute;
            bridgeTimeInfo.barStartTick   = timeInfo.bbt.barStartTick;
        }

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientProcess);
            fShmRtClientControl.writeUInt(frames);
            fShmRtClientControl.commitWrite();
        }

        waitForClient("process", fProcWaitTime);

        if (fTimedOut)
        {
            pData->singleMutex.unlock();
            return false;
        }

        for (uint32_t i=0; i < fInfo.aOuts; ++i)
            carla_copyFloats(audioOut[i], fShmAudioPool.data + ((i + fInfo.aIns) * frames), frames);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doVolume  = (pData->hints & PLUGIN_CAN_VOLUME) != 0 && carla_isNotEqual(pData->postProc.volume, 1.0f);
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && carla_isNotEqual(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));
            const bool isMono    = (pData->audioIn.count == 1);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    const uint32_t c = isMono ? 0 : i;

                    for (uint32_t k=0; k < frames; ++k)
                    {
# ifndef BUILD_BRIDGE
                        if (k < pData->latency.frames)
                            bufValue = pData->latency.buffers[c][k];
                        else if (pData->latency.frames < frames)
                            bufValue = audioIn[c][k-pData->latency.frames];
                        else
# endif
                            bufValue = audioIn[c][k];

                        audioOut[i][k] = (audioOut[i][k] * pData->postProc.dryWet) + (bufValue * (1.0f - pData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < pData->audioOut.count);
                        carla_copyFloats(oldBufLeft, audioOut[i], frames);
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (isPair)
                        {
                            // left
                            audioOut[i][k]  = oldBufLeft[k]     * (1.0f - balRangeL);
                            audioOut[i][k] += audioOut[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            audioOut[i][k]  = audioOut[i][k] * balRangeR;
                            audioOut[i][k] += oldBufLeft[k]   * balRangeL;
                        }
                    }
                }

                // Volume (and buffer copy)
                if (doVolume)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        audioOut[i][k] *= pData->postProc.volume;
                }
            }

        } // End of Post-processing

# ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Save latency values for next callback

        if (const uint32_t latframes = pData->latency.frames)
        {
            if (latframes <= frames)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                    carla_copyFloats(pData->latency.buffers[i], audioIn[i]+(frames-latframes), latframes);
            }
            else
            {
                const uint32_t diff = pData->latency.frames-frames;

                for (uint32_t i=0, k; i<pData->audioIn.count; ++i)
                {
                    // push back buffer by 'frames'
                    for (k=0; k < diff; ++k)
                        pData->latency.buffers[i][k] = pData->latency.buffers[i][k+frames];

                    // put current input at the end
                    for (uint32_t j=0; k < latframes; ++j, ++k)
                        pData->latency.buffers[i][k] = audioIn[i][j];
                }
            }
        }
# endif
#endif // BUILD_BRIDGE_ALTERNATIVE_ARCH

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        resizeAudioPool(newBufferSize);

        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetBufferSize);
            fShmRtClientControl.writeUInt(newBufferSize);
            fShmRtClientControl.commitWrite();
        }

        //fProcWaitTime = newBufferSize*1000/pData->engine->getSampleRate();
        fProcWaitTime = 1000;

        waitForClient("buffersize", 1000);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetSampleRate);
            fShmRtClientControl.writeDouble(newSampleRate);
            fShmRtClientControl.commitWrite();
        }

        //fProcWaitTime = pData->engine->getBufferSize()*1000/newSampleRate;
        fProcWaitTime = 1000;

        waitForClient("samplerate", 1000);
    }

    void offlineModeChanged(const bool isOffline) override
    {
        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetOnline);
            fShmRtClientControl.writeBool(isOffline);
            fShmRtClientControl.commitWrite();
        }

        waitForClient("offline", 1000);
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() noexcept override
    {
        if (fParams != nullptr)
        {
            delete[] fParams;
            fParams = nullptr;
        }

        CarlaPlugin::clearBuffers();
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    void uiParameterChange(const uint32_t index, const float value) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index < pData->param.count,);

        const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientUiParameterChange);
        fShmNonRtClientControl.writeUInt(index);
        fShmNonRtClientControl.writeFloat(value);
        fShmNonRtClientControl.commitWrite();
    }

    void uiProgramChange(const uint32_t index) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index < pData->prog.count,);

        const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientUiProgramChange);
        fShmNonRtClientControl.writeUInt(index);
        fShmNonRtClientControl.commitWrite();
    }

    void uiMidiProgramChange(const uint32_t index) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index < pData->midiprog.count,);

        const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientUiMidiProgramChange);
        fShmNonRtClientControl.writeUInt(index);
        fShmNonRtClientControl.commitWrite();
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
        CARLA_SAFE_ASSERT_RETURN(velo > 0 && velo < MAX_MIDI_VALUE,);

        const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientUiNoteOn);
        fShmNonRtClientControl.writeByte(channel);
        fShmNonRtClientControl.writeByte(note);
        fShmNonRtClientControl.writeByte(velo);
        fShmNonRtClientControl.commitWrite();
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);

        const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientUiNoteOff);
        fShmNonRtClientControl.writeByte(channel);
        fShmNonRtClientControl.writeByte(note);
        fShmNonRtClientControl.commitWrite();
    }

    // -------------------------------------------------------------------
    // Internal helper functions

    void restoreLV2State() noexcept override
    {
        const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientRestoreLV2State);
        fShmNonRtClientControl.commitWrite();
    }

    void waitForBridgeSaveSignal() noexcept override
    {
        // VSTs only save chunks, for which we already have a waitForSaved there
        if (fPluginType != PLUGIN_VST2)
            waitForSaved();
    }

    // -------------------------------------------------------------------

    void handleNonRtData()
    {
        for (; fShmNonRtServerControl.isDataAvailableForReading();)
        {
            const PluginBridgeNonRtServerOpcode opcode(fShmNonRtServerControl.readOpcode());
#ifdef DEBUG
            if (opcode != kPluginBridgeNonRtServerPong && opcode != kPluginBridgeNonRtServerParameterValue2) {
                carla_debug("CarlaPluginBridge::handleNonRtData() - got opcode: %s", PluginBridgeNonRtServerOpcode2str(opcode));
            }
#endif
            if (opcode != kPluginBridgeNonRtServerNull && fLastPongTime > 0)
                fLastPongTime = Time::currentTimeMillis();

            switch (opcode)
            {
            case kPluginBridgeNonRtServerNull:
            case kPluginBridgeNonRtServerPong:
                break;

            case kPluginBridgeNonRtServerPluginInfo1: {
                // uint/category, uint/hints, uint/optionsAvailable, uint/optionsEnabled, long/uniqueId
                const uint32_t category = fShmNonRtServerControl.readUInt();
                const uint32_t hints    = fShmNonRtServerControl.readUInt();
                const uint32_t optionAv = fShmNonRtServerControl.readUInt();
                const uint32_t optionEn = fShmNonRtServerControl.readUInt();
                const  int64_t uniqueId = fShmNonRtServerControl.readLong();

                if (fUniqueId != 0) {
                    CARLA_SAFE_ASSERT_INT2(fUniqueId == uniqueId, fUniqueId, uniqueId);
                }

                pData->hints   = hints | PLUGIN_IS_BRIDGE;
                pData->options = optionEn;

                fInfo.category = static_cast<PluginCategory>(category);
                fInfo.optionsAvailable = optionAv;
            }   break;

            case kPluginBridgeNonRtServerPluginInfo2: {
                // uint/size, str[] (realName), uint/size, str[] (label), uint/size, str[] (maker), uint/size, str[] (copyright)

                // realName
                const uint32_t realNameSize(fShmNonRtServerControl.readUInt());
                char realName[realNameSize+1];
                carla_zeroChars(realName, realNameSize+1);
                fShmNonRtServerControl.readCustomData(realName, realNameSize);

                // label
                const uint32_t labelSize(fShmNonRtServerControl.readUInt());
                char label[labelSize+1];
                carla_zeroChars(label, labelSize+1);
                fShmNonRtServerControl.readCustomData(label, labelSize);

                // maker
                const uint32_t makerSize(fShmNonRtServerControl.readUInt());
                char maker[makerSize+1];
                carla_zeroChars(maker, makerSize+1);
                fShmNonRtServerControl.readCustomData(maker, makerSize);

                // copyright
                const uint32_t copyrightSize(fShmNonRtServerControl.readUInt());
                char copyright[copyrightSize+1];
                carla_zeroChars(copyright, copyrightSize+1);
                fShmNonRtServerControl.readCustomData(copyright, copyrightSize);

                fInfo.name  = realName;
                fInfo.label = label;
                fInfo.maker = maker;
                fInfo.copyright = copyright;

                if (pData->name == nullptr)
                    pData->name = pData->engine->getUniquePluginName(realName);
            }   break;

            case kPluginBridgeNonRtServerAudioCount: {
                // uint/ins, uint/outs
                fInfo.clear();

                fInfo.aIns  = fShmNonRtServerControl.readUInt();
                fInfo.aOuts = fShmNonRtServerControl.readUInt();

                if (fInfo.aIns > 0)
                {
                    fInfo.aInNames = new const char*[fInfo.aIns];
                    carla_zeroPointers(fInfo.aInNames, fInfo.aIns);
                }

                if (fInfo.aOuts > 0)
                {
                    fInfo.aOutNames = new const char*[fInfo.aOuts];
                    carla_zeroPointers(fInfo.aOutNames, fInfo.aOuts);
                }

            }   break;

            case kPluginBridgeNonRtServerMidiCount: {
                // uint/ins, uint/outs
                fInfo.mIns  = fShmNonRtServerControl.readUInt();
                fInfo.mOuts = fShmNonRtServerControl.readUInt();
            }   break;

            case kPluginBridgeNonRtServerCvCount: {
                // uint/ins, uint/outs
                fInfo.cvIns  = fShmNonRtServerControl.readUInt();
                fInfo.cvOuts = fShmNonRtServerControl.readUInt();
            }   break;

            case kPluginBridgeNonRtServerParameterCount: {
                // uint/count
                const uint32_t count = fShmNonRtServerControl.readUInt();

                // delete old data
                pData->param.clear();

                if (fParams != nullptr)
                {
                    delete[] fParams;
                    fParams = nullptr;
                }

                if (count > 0)
                {
                    pData->param.createNew(count, false);
                    fParams = new BridgeParamInfo[count];

                    // we might not receive all parameter data, so ensure range max is not 0
                    for (uint32_t i=0; i<count; ++i)
                    {
                        pData->param.ranges[i].def = 0.0f;
                        pData->param.ranges[i].min = 0.0f;
                        pData->param.ranges[i].max = 1.0f;
                        pData->param.ranges[i].step = 0.001f;
                        pData->param.ranges[i].stepSmall = 0.0001f;
                        pData->param.ranges[i].stepLarge = 0.1f;
                    }
                }
            }   break;

            case kPluginBridgeNonRtServerProgramCount: {
                // uint/count
                pData->prog.clear();

                if (const uint32_t count = fShmNonRtServerControl.readUInt())
                    pData->prog.createNew(static_cast<uint32_t>(count));

            }   break;

            case kPluginBridgeNonRtServerMidiProgramCount: {
                // uint/count
                pData->midiprog.clear();

                if (const uint32_t count = fShmNonRtServerControl.readUInt())
                    pData->midiprog.createNew(static_cast<uint32_t>(count));

            }   break;

            case kPluginBridgeNonRtServerPortName: {
                // byte/type, uint/index, uint/size, str[] (name)
                const uint8_t  portType = fShmNonRtServerControl.readByte();
                const uint32_t index    = fShmNonRtServerControl.readUInt();

                // name
                const uint32_t nameSize(fShmNonRtServerControl.readUInt());
                char* const name = new char[nameSize+1];
                carla_zeroChars(name, nameSize+1);
                fShmNonRtServerControl.readCustomData(name, nameSize);

                CARLA_SAFE_ASSERT_BREAK(portType > kPluginBridgePortNull && portType < kPluginBridgePortTypeCount);

                switch (portType)
                {
                case kPluginBridgePortAudioInput:
                    CARLA_SAFE_ASSERT_BREAK(index < fInfo.aIns);
                    fInfo.aInNames[index] = name;
                    break;
                case kPluginBridgePortAudioOutput:
                    CARLA_SAFE_ASSERT_BREAK(index < fInfo.aOuts);
                    fInfo.aOutNames[index] = name;
                    break;
                }

            }   break;

            case kPluginBridgeNonRtServerParameterData1: {
                // uint/index, int/rindex, uint/type, uint/hints, int/cc
                const uint32_t index  = fShmNonRtServerControl.readUInt();
                const  int32_t rindex = fShmNonRtServerControl.readInt();
                const uint32_t type   = fShmNonRtServerControl.readUInt();
                const uint32_t hints  = fShmNonRtServerControl.readUInt();
                const  int16_t midiCC = fShmNonRtServerControl.readShort();

                CARLA_SAFE_ASSERT_BREAK(midiCC >= -1 && midiCC < MAX_MIDI_CONTROL);
                CARLA_SAFE_ASSERT_INT2(index < pData->param.count, index, pData->param.count);

                if (index < pData->param.count)
                {
                    pData->param.data[index].type   = static_cast<ParameterType>(type);
                    pData->param.data[index].index  = static_cast<int32_t>(index);
                    pData->param.data[index].rindex = rindex;
                    pData->param.data[index].hints  = hints;
                    pData->param.data[index].midiCC = midiCC;
                }
            }   break;

            case kPluginBridgeNonRtServerParameterData2: {
                // uint/index, uint/size, str[] (name), uint/size, str[] (unit)
                const uint32_t index = fShmNonRtServerControl.readUInt();

                // name
                const uint32_t nameSize(fShmNonRtServerControl.readUInt());
                char name[nameSize+1];
                carla_zeroChars(name, nameSize+1);
                fShmNonRtServerControl.readCustomData(name, nameSize);

                // symbol
                const uint32_t symbolSize(fShmNonRtServerControl.readUInt());
                char symbol[symbolSize+1];
                carla_zeroChars(symbol, symbolSize+1);
                fShmNonRtServerControl.readCustomData(symbol, symbolSize);

                // unit
                const uint32_t unitSize(fShmNonRtServerControl.readUInt());
                char unit[unitSize+1];
                carla_zeroChars(unit, unitSize+1);
                fShmNonRtServerControl.readCustomData(unit, unitSize);

                CARLA_SAFE_ASSERT_INT2(index < pData->param.count, index, pData->param.count);

                if (index < pData->param.count)
                {
                    fParams[index].name   = name;
                    fParams[index].symbol = symbol;
                    fParams[index].unit   = unit;
                }
            }   break;

            case kPluginBridgeNonRtServerParameterRanges: {
                // uint/index, float/def, float/min, float/max, float/step, float/stepSmall, float/stepLarge
                const uint32_t index = fShmNonRtServerControl.readUInt();
                const float def      = fShmNonRtServerControl.readFloat();
                const float min      = fShmNonRtServerControl.readFloat();
                const float max      = fShmNonRtServerControl.readFloat();
                const float step      = fShmNonRtServerControl.readFloat();
                const float stepSmall = fShmNonRtServerControl.readFloat();
                const float stepLarge = fShmNonRtServerControl.readFloat();

                CARLA_SAFE_ASSERT_BREAK(min < max);
                CARLA_SAFE_ASSERT_BREAK(def >= min);
                CARLA_SAFE_ASSERT_BREAK(def <= max);
                CARLA_SAFE_ASSERT_INT2(index < pData->param.count, index, pData->param.count);

                if (index < pData->param.count)
                {
                    pData->param.ranges[index].def = def;
                    pData->param.ranges[index].min = min;
                    pData->param.ranges[index].max = max;
                    pData->param.ranges[index].step      = step;
                    pData->param.ranges[index].stepSmall = stepSmall;
                    pData->param.ranges[index].stepLarge = stepLarge;
                }
            }   break;

            case kPluginBridgeNonRtServerParameterValue: {
                // uint/index, float/value
                const uint32_t index = fShmNonRtServerControl.readUInt();
                const float    value = fShmNonRtServerControl.readFloat();

                if (index < pData->param.count)
                {
                    const float fixedValue(pData->param.getFixedValue(index, value));
                    fParams[index].value = fixedValue;

                    CarlaPlugin::setParameterValue(index, fixedValue, false, true, true);
                }
            }   break;

            case kPluginBridgeNonRtServerParameterValue2: {
                // uint/index, float/value
                const uint32_t index = fShmNonRtServerControl.readUInt();
                const float    value = fShmNonRtServerControl.readFloat();

                if (index < pData->param.count)
                {
                    const float fixedValue(pData->param.getFixedValue(index, value));
                    fParams[index].value = fixedValue;
                }
            }   break;

            case kPluginBridgeNonRtServerDefaultValue: {
                // uint/index, float/value
                const uint32_t index = fShmNonRtServerControl.readUInt();
                const float    value = fShmNonRtServerControl.readFloat();

                if (index < pData->param.count)
                    pData->param.ranges[index].def = value;
            }   break;

            case kPluginBridgeNonRtServerCurrentProgram: {
                // int/index
                const int32_t index = fShmNonRtServerControl.readInt();

                CARLA_SAFE_ASSERT_BREAK(index >= -1);
                CARLA_SAFE_ASSERT_INT2(index < static_cast<int32_t>(pData->prog.count), index, pData->prog.count);

                CarlaPlugin::setProgram(index, false, true, true);
            }   break;

            case kPluginBridgeNonRtServerCurrentMidiProgram: {
                // int/index
                const int32_t index = fShmNonRtServerControl.readInt();

                CARLA_SAFE_ASSERT_BREAK(index >= -1);
                CARLA_SAFE_ASSERT_INT2(index < static_cast<int32_t>(pData->midiprog.count), index, pData->midiprog.count);

                CarlaPlugin::setMidiProgram(index, false, true, true);
            }   break;

            case kPluginBridgeNonRtServerProgramName: {
                // uint/index, uint/size, str[] (name)
                const uint32_t index = fShmNonRtServerControl.readUInt();

                // name
                const uint32_t nameSize(fShmNonRtServerControl.readUInt());
                char name[nameSize+1];
                carla_zeroChars(name, nameSize+1);
                fShmNonRtServerControl.readCustomData(name, nameSize);

                CARLA_SAFE_ASSERT_INT2(index < pData->prog.count, index, pData->prog.count);

                if (index < pData->prog.count)
                {
                    if (pData->prog.names[index] != nullptr)
                        delete[] pData->prog.names[index];
                    pData->prog.names[index] = carla_strdup(name);
                }
            }   break;

            case kPluginBridgeNonRtServerMidiProgramData: {
                // uint/index, uint/bank, uint/program, uint/size, str[] (name)
                const uint32_t index   = fShmNonRtServerControl.readUInt();
                const uint32_t bank    = fShmNonRtServerControl.readUInt();
                const uint32_t program = fShmNonRtServerControl.readUInt();

                // name
                const uint32_t nameSize(fShmNonRtServerControl.readUInt());
                char name[nameSize+1];
                carla_zeroChars(name, nameSize+1);
                fShmNonRtServerControl.readCustomData(name, nameSize);

                CARLA_SAFE_ASSERT_INT2(index < pData->midiprog.count, index, pData->midiprog.count);

                if (index < pData->midiprog.count)
                {
                    if (pData->midiprog.data[index].name != nullptr)
                        delete[] pData->midiprog.data[index].name;
                    pData->midiprog.data[index].bank    = bank;
                    pData->midiprog.data[index].program = program;
                    pData->midiprog.data[index].name    = carla_strdup(name);
                }
            }   break;

            case kPluginBridgeNonRtServerSetCustomData: {
                // uint/size, str[], uint/size, str[], uint/size, str[]

                // type
                const uint32_t typeSize(fShmNonRtServerControl.readUInt());
                char type[typeSize+1];
                carla_zeroChars(type, typeSize+1);
                fShmNonRtServerControl.readCustomData(type, typeSize);

                // key
                const uint32_t keySize(fShmNonRtServerControl.readUInt());
                char key[keySize+1];
                carla_zeroChars(key, keySize+1);
                fShmNonRtServerControl.readCustomData(key, keySize);

                // value
                const uint32_t valueSize(fShmNonRtServerControl.readUInt());
                char value[valueSize+1];
                carla_zeroChars(value, valueSize+1);

                if (valueSize > 0)
                    fShmNonRtServerControl.readCustomData(value, valueSize);

                CarlaPlugin::setCustomData(type, key, value, false);
            }   break;

            case kPluginBridgeNonRtServerSetChunkDataFile: {
                // uint/size, str[] (filename)

                // chunkFilePath
                const uint32_t chunkFilePathSize(fShmNonRtServerControl.readUInt());
                char chunkFilePath[chunkFilePathSize+1];
                carla_zeroChars(chunkFilePath, chunkFilePathSize+1);
                fShmNonRtServerControl.readCustomData(chunkFilePath, chunkFilePathSize);

                String realChunkFilePath(chunkFilePath);

#ifndef CARLA_OS_WIN
                // Using Wine, fix temp dir
                if (fBinaryType == BINARY_WIN32 || fBinaryType == BINARY_WIN64)
                {
                    const StringArray driveLetterSplit(StringArray::fromTokens(realChunkFilePath, ":/", ""));
                    carla_stdout("chunk save path BEFORE => %s", realChunkFilePath.toRawUTF8());

                    realChunkFilePath  = fWinePrefix;
                    realChunkFilePath += "/drive_";
                    realChunkFilePath += driveLetterSplit[0].toLowerCase();
                    realChunkFilePath += driveLetterSplit[1];

                    realChunkFilePath  = realChunkFilePath.replace("\\", "/");
                    carla_stdout("chunk save path AFTER => %s", realChunkFilePath.toRawUTF8());
                }
#endif

                File chunkFile(realChunkFilePath);
                CARLA_SAFE_ASSERT_BREAK(chunkFile.existsAsFile());

                fInfo.chunk = carla_getChunkFromBase64String(chunkFile.loadFileAsString().toRawUTF8());
                chunkFile.deleteFile();
            }   break;

            case kPluginBridgeNonRtServerSetLatency:
                if (true)
                {
                    // FIXME - latency adjust code on this file is broken
                    fShmNonRtServerControl.readUInt();
                    break;
                }

                // uint
                fLatency = fShmNonRtServerControl.readUInt();
#ifndef BUILD_BRIDGE
                if (! fInitiated)
                    pData->latency.recreateBuffers(std::max(fInfo.aIns, fInfo.aOuts), fLatency);
#endif
                break;

            case kPluginBridgeNonRtServerSetParameterText: {
                const int32_t index = fShmNonRtServerControl.readInt();

                const uint32_t textSize(fShmNonRtServerControl.readUInt());
                char text[textSize+1];
                carla_zeroChars(text, textSize+1);
                fShmNonRtServerControl.readCustomData(text, textSize);

                fReceivingParamText.setReceivedData(index, text, textSize);
            }   break;

            case kPluginBridgeNonRtServerReady:
                fInitiated = true;
                break;

            case kPluginBridgeNonRtServerSaved:
                fSaved = true;
                break;

            case kPluginBridgeNonRtServerUiClosed:
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                pData->transientTryCounter = 0;
#endif
                pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
                break;

            case kPluginBridgeNonRtServerError: {
                // error
                const uint32_t errorSize(fShmNonRtServerControl.readUInt());
                char error[errorSize+1];
                carla_zeroChars(error, errorSize+1);
                fShmNonRtServerControl.readCustomData(error, errorSize);

                if (fInitiated)
                {
                    pData->engine->callback(ENGINE_CALLBACK_ERROR, pData->id, 0, 0, 0.0f, error);

                    // just in case
                    pData->engine->setLastError(error);
                    fInitError = true;
                }
                else
                {
                    pData->engine->setLastError(error);
                    fInitError = true;
                    fInitiated = true;
                }
            }   break;
            }
        }
    }

    // -------------------------------------------------------------------

    uintptr_t getUiBridgeProcessId() const noexcept override
    {
        return fBridgeThread.getProcessPID();
    }

    const void* getExtraStuff() const noexcept override
    {
        return fBridgeBinary.isNotEmpty() ? fBridgeBinary.buffer() : nullptr;
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label, const int64_t uniqueId, const char* const bridgeBinary)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (bridgeBinary == nullptr || bridgeBinary[0] == '\0')
        {
            pData->engine->setLastError("null bridge binary");
            return false;
        }

        // ---------------------------------------------------------------
        // set info

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);

        if (filename != nullptr && filename[0] != '\0')
            pData->filename = carla_strdup(filename);
        else
            pData->filename = carla_strdup("");

        fUniqueId     = uniqueId;
        fBridgeBinary = bridgeBinary;

        std::srand(static_cast<uint>(std::time(nullptr)));

        // ---------------------------------------------------------------
        // init sem/shm

        if (! fShmAudioPool.initializeServer())
        {
            carla_stderr("Failed to initialize shared memory audio pool");
            return false;
        }

        if (! fShmRtClientControl.initializeServer())
        {
            carla_stderr("Failed to initialize RT client control");
            fShmAudioPool.clear();
            return false;
        }

        if (! fShmNonRtClientControl.initializeServer())
        {
            carla_stderr("Failed to initialize Non-RT client control");
            fShmRtClientControl.clear();
            fShmAudioPool.clear();
            return false;
        }

        if (! fShmNonRtServerControl.initializeServer())
        {
            carla_stderr("Failed to initialize Non-RT server control");
            fShmNonRtClientControl.clear();
            fShmRtClientControl.clear();
            fShmAudioPool.clear();
            return false;
        }

#ifndef CARLA_OS_WIN
        // ---------------------------------------------------------------
        // set wine prefix

        if (fBridgeBinary.contains(".exe", true))
        {
            const EngineOptions& options(pData->engine->getOptions());

            if (options.wine.autoPrefix)
                fWinePrefix = findWinePrefix(pData->filename);

            if (fWinePrefix.isEmpty())
            {
                const char* const envWinePrefix(std::getenv("WINEPREFIX"));

                if (envWinePrefix != nullptr && envWinePrefix[0] != '\0')
                    fWinePrefix = envWinePrefix;
                else if (options.wine.fallbackPrefix != nullptr && options.wine.fallbackPrefix[0] != '\0')
                    fWinePrefix = options.wine.fallbackPrefix;
                else
                    fWinePrefix = File::getSpecialLocation(File::userHomeDirectory).getFullPathName() + "/.wine";
            }
        }
#endif

        // ---------------------------------------------------------------
        // init bridge thread

        {
            char shmIdsStr[6*4+1];
            carla_zeroChars(shmIdsStr, 6*4+1);

            std::strncpy(shmIdsStr+6*0, &fShmAudioPool.filename[fShmAudioPool.filename.length()-6], 6);
            std::strncpy(shmIdsStr+6*1, &fShmRtClientControl.filename[fShmRtClientControl.filename.length()-6], 6);
            std::strncpy(shmIdsStr+6*2, &fShmNonRtClientControl.filename[fShmNonRtClientControl.filename.length()-6], 6);
            std::strncpy(shmIdsStr+6*3, &fShmNonRtServerControl.filename[fShmNonRtServerControl.filename.length()-6], 6);

            fBridgeThread.setData(
#ifndef CARLA_OS_WIN
                                  fWinePrefix.toRawUTF8(),
#endif
                                  bridgeBinary, label, shmIdsStr);
        }

        if (! restartBridgeThread())
            return false;

        // ---------------------------------------------------------------
        // register client

        if (pData->name == nullptr)
        {
            if (label != nullptr && label[0] != '\0')
                pData->name = pData->engine->getUniquePluginName(label);
            else
                pData->name = pData->engine->getUniquePluginName("unknown");
        }

        pData->client = pData->engine->addClient(this);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        return true;
    }

private:
    const BinaryType fBinaryType;
    const PluginType fPluginType;

    bool fInitiated;
    bool fInitError;
    bool fSaved;
    bool fTimedOut;
    bool fTimedError;
    uint fProcWaitTime;

    int64_t fLastPongTime;

    CarlaString             fBridgeBinary;
    CarlaPluginBridgeThread fBridgeThread;

    BridgeAudioPool          fShmAudioPool;
    BridgeRtClientControl    fShmRtClientControl;
    BridgeNonRtClientControl fShmNonRtClientControl;
    BridgeNonRtServerControl fShmNonRtServerControl;

#ifndef CARLA_OS_WIN
    String fWinePrefix;
#endif

    class ReceivingParamText {
    public:
        ReceivingParamText() noexcept
            : dataRecv(false),
              dataOk(false),
              index(-1),
              strBuf(nullptr),
              mutex() {}

        bool isCurrentlyWaitingData() const noexcept
        {
            return index >= 0;
        }

        bool wasDataReceived(bool* const success) const noexcept
        {
            *success = dataOk;
            return dataRecv;
        }

        void setTargetData(const int32_t i, char* const b) noexcept
        {
            const CarlaMutexLocker cml(mutex);

            dataOk   = false;
            dataRecv = false;
            index    = i;
            strBuf   = b;
        }

        void setReceivedData(const int32_t i, const char* const b, const uint blen) noexcept
        {
            ScopedValueSetter<bool> svs(dataRecv, false, true);

            const CarlaMutexLocker cml(mutex);

            // make backup and reset data
            const int32_t indexCopy = index;
            char* const strBufCopy = strBuf;
            index  = -1;
            strBuf = nullptr;

            CARLA_SAFE_ASSERT_RETURN(indexCopy == i,);
            CARLA_SAFE_ASSERT_RETURN(strBufCopy != nullptr,);

            std::strncpy(strBufCopy, b, std::min(blen, STR_MAX-1U));
            dataOk = true;
        }

    private:
        bool dataRecv;
        bool dataOk;
        int32_t index;
        char* strBuf;
        CarlaMutex mutex;

        CARLA_DECLARE_NON_COPY_CLASS(ReceivingParamText)
    } fReceivingParamText;

    struct Info {
        uint32_t aIns, aOuts;
        uint32_t cvIns, cvOuts;
        uint32_t mIns, mOuts;
        PluginCategory category;
        uint optionsAvailable;
        CarlaString name;
        CarlaString label;
        CarlaString maker;
        CarlaString copyright;
        const char** aInNames;
        const char** aOutNames;
        std::vector<uint8_t> chunk;

        Info()
            : aIns(0),
              aOuts(0),
              cvIns(0),
              cvOuts(0),
              mIns(0),
              mOuts(0),
              category(PLUGIN_CATEGORY_NONE),
              optionsAvailable(0),
              name(),
              label(),
              maker(),
              copyright(),
              aInNames(nullptr),
              aOutNames(nullptr),
              chunk() {}

        ~Info()
        {
            clear();
        }

        void clear()
        {
            if (aInNames != nullptr)
            {
                CARLA_SAFE_ASSERT_INT(aIns > 0, aIns);

                for (uint32_t i=0; i<aIns; ++i)
                    delete[] aInNames[i];

                delete[] aInNames;
                aInNames = nullptr;
            }

            if (aOutNames != nullptr)
            {
                CARLA_SAFE_ASSERT_INT(aOuts > 0, aOuts);

                for (uint32_t i=0; i<aOuts; ++i)
                    delete[] aOutNames[i];

                delete[] aOutNames;
                aOutNames = nullptr;
            }

            aIns = aOuts = 0;
        }

        CARLA_DECLARE_NON_COPY_STRUCT(Info)
    } fInfo;

    int64_t  fUniqueId;
    uint32_t fLatency;

    BridgeParamInfo* fParams;

    void handleProcessStopped() noexcept
    {
        const bool wasActive = pData->active;
        pData->active = false;

        if (wasActive)
        {
#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
            if (pData->engine->isOscControlRegistered())
                pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_ACTIVE, 0.0f);
            pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_ACTIVE, 0, 0.0f, nullptr);
#endif
        }

        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
            pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
    }

    void resizeAudioPool(const uint32_t bufferSize)
    {
        fShmAudioPool.resize(bufferSize, fInfo.aIns+fInfo.aOuts, fInfo.cvIns+fInfo.cvOuts);

        fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetAudioPool);
        fShmRtClientControl.writeULong(static_cast<uint64_t>(fShmAudioPool.dataSize));
        fShmRtClientControl.commitWrite();

        waitForClient("resize-pool", 5000);
    }

    void waitForClient(const char* const action, const uint msecs)
    {
        CARLA_SAFE_ASSERT_RETURN(! fTimedOut,);
        CARLA_SAFE_ASSERT_RETURN(! fTimedError,);

        if (fShmRtClientControl.waitForClient(msecs))
            return;

        fTimedOut = true;
        carla_stderr2("waitForClient(%s) timed out", action);
    }

    bool restartBridgeThread()
    {
        fInitiated  = false;
        fInitError  = false;
        fTimedError = false;

        // reset memory
        fShmRtClientControl.data->procFlags = 0;
        carla_zeroStruct(fShmRtClientControl.data->timeInfo);
        carla_zeroBytes(fShmRtClientControl.data->midiOut, kBridgeRtClientDataMidiOutSize);

        fShmRtClientControl.clearData();
        fShmNonRtClientControl.clearData();
        fShmNonRtServerControl.clearData();

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientVersion);
        fShmNonRtClientControl.writeUInt(CARLA_PLUGIN_BRIDGE_API_VERSION);

        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeRtClientData)));
        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeNonRtClientData)));
        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeNonRtServerData)));

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientInitialSetup);
        fShmNonRtClientControl.writeUInt(pData->engine->getBufferSize());
        fShmNonRtClientControl.writeDouble(pData->engine->getSampleRate());

        fShmNonRtClientControl.commitWrite();

        if (fShmAudioPool.dataSize != 0)
        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetAudioPool);
            fShmRtClientControl.writeULong(static_cast<uint64_t>(fShmAudioPool.dataSize));
            fShmRtClientControl.commitWrite();
        }
        else
        {
            // testing dummy message
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientNull);
            fShmRtClientControl.commitWrite();
        }

        fBridgeThread.startThread();

        fLastPongTime = Time::currentTimeMillis();
        CARLA_SAFE_ASSERT(fLastPongTime > 0);

        static bool sFirstInit = true;

        int64_t timeoutEnd = 5000;

        if (sFirstInit)
            timeoutEnd *= 2;
#ifndef CARLA_OS_WIN
         if (fBinaryType == BINARY_WIN32 || fBinaryType == BINARY_WIN64)
            timeoutEnd *= 2;
#endif
        sFirstInit = false;

        const bool needsEngineIdle = pData->engine->getType() != kEngineTypePlugin;

        for (; Time::currentTimeMillis() < fLastPongTime + timeoutEnd && fBridgeThread.isThreadRunning();)
        {
            pData->engine->callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

            if (needsEngineIdle)
                pData->engine->idle();

            idle();

            if (fInitiated)
                break;
            if (pData->engine->isAboutToClose())
                break;

            carla_msleep(20);
        }

        fLastPongTime = -1;

        if (fInitError || ! fInitiated)
        {
            fBridgeThread.stopThread(6000);

            if (! fInitError)
                pData->engine->setLastError("Timeout while waiting for a response from plugin-bridge\n"
                                            "(or the plugin crashed on initialization?)");

            return false;
        }

        if (const size_t dataSize = fInfo.chunk.size())
        {
#ifdef CARLA_PROPER_CPP11_SUPPORT
            void* data = fInfo.chunk.data();
#else
            void* data = &fInfo.chunk.front();
#endif
            CarlaString dataBase64(CarlaString::asBase64(data, dataSize));
            CARLA_SAFE_ASSERT_RETURN(dataBase64.length() > 0, true);

            String filePath(File::getSpecialLocation(File::tempDirectory).getFullPathName());

            filePath += CARLA_OS_SEP_STR ".CarlaChunk_";
            filePath += fShmAudioPool.getFilenameSuffix();

            if (File(filePath).replaceWithText(dataBase64.buffer()))
            {
                const uint32_t ulength(static_cast<uint32_t>(filePath.length()));

                const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

                fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetChunkDataFile);
                fShmNonRtClientControl.writeUInt(ulength);
                fShmNonRtClientControl.writeCustomData(filePath.toRawUTF8(), ulength);
                fShmNonRtClientControl.commitWrite();
            }
        }

        return true;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginBridge)
};

CARLA_BACKEND_END_NAMESPACE

// ---------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newBridge(const Initializer& init, BinaryType btype, PluginType ptype, const char* bridgeBinary)
{
    carla_debug("CarlaPlugin::newBridge({%p, \"%s\", \"%s\", \"%s\"}, %s, %s, \"%s\")", init.engine, init.filename, init.name, init.label, BinaryType2Str(btype), PluginType2Str(ptype), bridgeBinary);

    if (bridgeBinary == nullptr || bridgeBinary[0] == '\0')
    {
        init.engine->setLastError("Bridge not possible, bridge-binary not found");
        return nullptr;
    }

#ifndef CARLA_OS_WIN
    // FIXME: somewhere, somehow, we end up with double slashes, wine doesn't like that.
    if (std::strncmp(bridgeBinary, "//", 2) == 0)
        ++bridgeBinary;
#endif

    CarlaPluginBridge* const plugin(new CarlaPluginBridge(init.engine, init.id, btype, ptype));

    if (! plugin->init(init.filename, init.name, init.label, init.uniqueId, bridgeBinary))
    {
        delete plugin;
        return nullptr;
    }

    return plugin;
}

CARLA_BACKEND_END_NAMESPACE

// ---------------------------------------------------------------------------------------------------------------------

#ifndef BUILD_BRIDGE
# include "CarlaBridgeUtils.cpp"
#endif

// ---------------------------------------------------------------------------------------------------------------------
