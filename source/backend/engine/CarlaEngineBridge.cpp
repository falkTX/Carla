// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef BUILD_BRIDGE
# error This file should not be compiled if not building bridge
#endif

// must be first so we can undef CARLA_SAFE_*_RETURN_ERR* macros
#include "CarlaPluginInternal.hpp"
#undef CARLA_SAFE_ASSERT_RETURN_ERR
#undef CARLA_SAFE_ASSERT_RETURN_ERRN
#undef CARLA_SAFE_EXCEPTION_RETURN_ERR
#undef CARLA_SAFE_EXCEPTION_RETURN_ERRN

#include "CarlaEngineClient.hpp"
#include "CarlaEngineInit.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaBridgeUtils.hpp"
#include "CarlaMIDI.h"

#ifdef __SSE2_MATH__
# include <xmmintrin.h>
#endif

#include "extra/Base64.hpp"
#include "extra/Time.hpp"

#include "water/files/File.h"
#include "water/misc/Time.h"

// must be last
#include "jackbridge/JackBridge.hpp"

using water::File;
using water::MemoryBlock;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Bridge Engine client

struct LatencyChangedCallback {
    virtual ~LatencyChangedCallback() noexcept {}
    virtual void latencyChanged(const uint32_t samples) noexcept = 0;
};

class CarlaEngineBridgeClient : public CarlaEngineClientForSubclassing
{
public:
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CarlaEngineBridgeClient(const CarlaEngine& engine,
                            EngineInternalGraph& egraph,
                            const CarlaPluginPtr plugin,
                            LatencyChangedCallback* const cb)
        : CarlaEngineClientForSubclassing(engine, egraph, plugin),
          fLatencyCallback(cb) {}
#else
    CarlaEngineBridgeClient(const CarlaEngine& engine, LatencyChangedCallback* const cb)
        : CarlaEngineClientForSubclassing(engine),
          fLatencyCallback(cb) {}
#endif

protected:
    void setLatency(const uint32_t samples) noexcept override
    {
        if (getLatency() == samples)
            return;

        fLatencyCallback->latencyChanged(samples);
        CarlaEngineClient::setLatency(samples);
    }

private:
    LatencyChangedCallback* const fLatencyCallback;

    CARLA_DECLARE_NON_COPYABLE(CarlaEngineBridgeClient)
};

// -------------------------------------------------------------------

struct BridgeTextReader {
    char* text;

    BridgeTextReader(BridgeNonRtClientControl& nonRtClientCtrl)
        : text(nullptr)
    {
        const uint32_t size = nonRtClientCtrl.readUInt();
        CARLA_SAFE_ASSERT_RETURN(size != 0,);

        text = new char[size + 1];
        nonRtClientCtrl.readCustomData(text, size);
        text[size] = '\0';
    }

    BridgeTextReader(BridgeNonRtClientControl& nonRtClientCtrl, const uint32_t size)
        : text(nullptr)
    {
        text = new char[size + 1];

        if (size != 0)
            nonRtClientCtrl.readCustomData(text, size);

        text[size] = '\0';
    }

    ~BridgeTextReader() noexcept
    {
        delete[] text;
    }

    CARLA_DECLARE_NON_COPYABLE(BridgeTextReader)
};

// -------------------------------------------------------------------

class CarlaEngineBridge : public CarlaEngine,
                          private CarlaThread,
                          private LatencyChangedCallback
{
public:
    CarlaEngineBridge(const char* const audioPoolBaseName, const char* const rtClientBaseName, const char* const nonRtClientBaseName, const char* const nonRtServerBaseName)
        : CarlaEngine(),
          CarlaThread("CarlaEngineBridge"),
          fShmAudioPool(),
          fShmRtClientControl(),
          fShmNonRtClientControl(),
          fShmNonRtServerControl(),
          fBaseNameAudioPool(audioPoolBaseName),
          fBaseNameRtClientControl(rtClientBaseName),
          fBaseNameNonRtClientControl(nonRtClientBaseName),
          fBaseNameNonRtServerControl(nonRtServerBaseName),
          fClosingDown(false),
          fIsOffline(false),
          fFirstIdle(true),
          fBridgeVersion(0),
          fLastPingTime(UINT32_MAX)
    {
        carla_debug("CarlaEngineBridge::CarlaEngineBridge(\"%s\", \"%s\", \"%s\", \"%s\")", audioPoolBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName);
    }

    ~CarlaEngineBridge() noexcept override
    {
        carla_debug("CarlaEngineBridge::~CarlaEngineBridge()");

        clear();
    }

    // -------------------------------------
    // CarlaEngine virtual calls

    bool init(const char* const clientName) override
    {
        carla_debug("CarlaEngineBridge::init(\"%s\")", clientName);

        if (! pData->init(clientName))
        {
            setLastError("Failed to init internal data");
            return false;
        }

        if (! fShmAudioPool.attachClient(fBaseNameAudioPool))
        {
            pData->close();
            setLastError("Failed to attach to audio pool shared memory");
            return false;
        }

        if (! fShmRtClientControl.attachClient(fBaseNameRtClientControl))
        {
            pData->close();
            clear();
            setLastError("Failed to attach to rt client control shared memory");
            return false;
        }

        if (! fShmRtClientControl.mapData())
        {
            pData->close();
            clear();
            setLastError("Failed to map rt client control shared memory");
            return false;
        }

        if (! fShmNonRtClientControl.attachClient(fBaseNameNonRtClientControl))
        {
            pData->close();
            clear();
            setLastError("Failed to attach to non-rt client control shared memory");
            return false;
        }

        if (! fShmNonRtClientControl.mapData())
        {
            pData->close();
            clear();
            setLastError("Failed to map non-rt control client shared memory");
            return false;
        }

        if (! fShmNonRtServerControl.attachClient(fBaseNameNonRtServerControl))
        {
            pData->close();
            clear();
            setLastError("Failed to attach to non-rt server control shared memory");
            return false;
        }

        if (! fShmNonRtServerControl.mapData())
        {
            pData->close();
            clear();
            setLastError("Failed to map non-rt control server shared memory");
            return false;
        }

        PluginBridgeNonRtClientOpcode opcode;

        opcode = fShmNonRtClientControl.readOpcode();
        CARLA_SAFE_ASSERT_RETURN(opcode == kPluginBridgeNonRtClientVersion, false);

        const uint32_t apiVersion = fShmNonRtClientControl.readUInt();
        CARLA_SAFE_ASSERT_RETURN(apiVersion >= CARLA_PLUGIN_BRIDGE_API_VERSION_MINIMUM, false);

        const uint32_t shmRtClientDataSize = fShmNonRtClientControl.readUInt();
        CARLA_SAFE_ASSERT_INT2(shmRtClientDataSize == sizeof(BridgeRtClientData), shmRtClientDataSize, sizeof(BridgeRtClientData));

        const uint32_t shmNonRtClientDataSize = fShmNonRtClientControl.readUInt();
        CARLA_SAFE_ASSERT_INT2(shmNonRtClientDataSize == sizeof(BridgeNonRtClientData), shmNonRtClientDataSize, sizeof(BridgeNonRtClientData));

        const uint32_t shmNonRtServerDataSize = fShmNonRtClientControl.readUInt();
        CARLA_SAFE_ASSERT_INT2(shmNonRtServerDataSize == sizeof(BridgeNonRtServerData), shmNonRtServerDataSize, sizeof(BridgeNonRtServerData));

        if (shmRtClientDataSize != sizeof(BridgeRtClientData) ||
            shmNonRtClientDataSize != sizeof(BridgeNonRtClientData) ||
            shmNonRtServerDataSize != sizeof(BridgeNonRtServerData))
        {
            pData->close();
            clear();
            setLastError("Shared memory data size mismatch");
            return false;
        }

        opcode = fShmNonRtClientControl.readOpcode();
        CARLA_SAFE_ASSERT_RETURN(opcode == kPluginBridgeNonRtClientInitialSetup, false);

        pData->bufferSize = fShmNonRtClientControl.readUInt();
        pData->sampleRate = fShmNonRtClientControl.readDouble();

        if (pData->bufferSize == 0 || carla_isZero(pData->sampleRate))
        {
            pData->close();
            clear();
            setLastError("Shared memory has invalid data");
            return false;
        }

        pData->initTime(nullptr);

        // tell backend we're live
        {
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

            // kPluginBridgeNonRtServerVersion was added in API 7
            if (apiVersion >= 7)
            {
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerVersion);
                fShmNonRtServerControl.writeUInt(CARLA_PLUGIN_BRIDGE_API_VERSION_CURRENT);
            }
            else
            {
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPong);
            }

            fShmNonRtServerControl.commitWrite();
        }

        startThread(true);
        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEngineBridge::close()");
        fLastPingTime = UINT32_MAX;

        CarlaEngine::close();

        stopThread(5000);
        clear();

        return true;
    }

    bool hasIdleOnMainThread() const noexcept override
    {
        return true;
    }

    bool isRunning() const noexcept override
    {
        if (fClosingDown)
            return false;

        return isThreadRunning() || ! fFirstIdle;
    }

    bool isOffline() const noexcept override
    {
        return fIsOffline;
    }

    EngineType getType() const noexcept override
    {
        return kEngineTypeBridge;
    }

    const char* getCurrentDriverName() const noexcept override
    {
        return "Bridge";
    }

    void touchPluginParameter(const uint id, const uint32_t parameterId, const bool touch) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(id == 0,);

        const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterTouch);
        fShmNonRtServerControl.writeUInt(parameterId);
        fShmNonRtServerControl.writeBool(touch);
        fShmNonRtServerControl.commitWrite();
    }

    CarlaEngineClient* addClient(const CarlaPluginPtr plugin) override
    {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        return new CarlaEngineBridgeClient(*this, pData->graph, plugin, this);
#else
        return new CarlaEngineBridgeClient(*this, this);

        // unused
        (void)plugin;
#endif
    }

    void idle() noexcept override
    {
        const CarlaPluginPtr plugin = pData->plugins[0].plugin;

        if (plugin.get() == nullptr)
        {
            if (const uint32_t length = static_cast<uint32_t>(pData->lastError.length()))
            {
                const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerError);
                fShmNonRtServerControl.writeUInt(length);
                fShmNonRtServerControl.writeCustomData(pData->lastError.buffer(), length);
                fShmNonRtServerControl.commitWrite();
            }

            signalThreadShouldExit();
            callback(true, true, ENGINE_CALLBACK_QUIT, 0, 0, 0, 0, 0.0f, nullptr);
            return;
        }

        const bool wasFirstIdle = fFirstIdle;

        if (wasFirstIdle)
        {
            fFirstIdle = false;
            fLastPingTime = d_gettime_ms();

            char bufStr[STR_MAX+1];
            carla_zeroChars(bufStr, STR_MAX+1);

            uint32_t bufStrSize;

            const CarlaEngineClient* const client(plugin->getEngineClient());
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

            // kPluginBridgeNonRtServerPluginInfo1
            {
                // uint/category, uint/hints, uint/optionsAvailable, uint/optionsEnabled, long/uniqueId
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPluginInfo1);
                fShmNonRtServerControl.writeUInt(plugin->getCategory());
                fShmNonRtServerControl.writeUInt(plugin->getHints());
                fShmNonRtServerControl.writeUInt(plugin->getOptionsAvailable());
                fShmNonRtServerControl.writeUInt(plugin->getOptionsEnabled());
                fShmNonRtServerControl.writeLong(plugin->getUniqueId());
                fShmNonRtServerControl.commitWrite();
            }

            // kPluginBridgeNonRtServerPluginInfo2
            {
                // uint/size, str[] (realName), uint/size, str[] (label), uint/size, str[] (maker), uint/size, str[] (copyright)
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPluginInfo2);

                if (! plugin->getRealName(bufStr))
                    bufStr[0] = '\0';
                bufStrSize = carla_fixedValue(1U, 64U, static_cast<uint32_t>(std::strlen(bufStr)));
                fShmNonRtServerControl.writeUInt(bufStrSize);
                fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                if (! plugin->getLabel(bufStr))
                    bufStr[0] = '\0';
                bufStrSize = carla_fixedValue(1U, 256U, static_cast<uint32_t>(std::strlen(bufStr)));
                fShmNonRtServerControl.writeUInt(bufStrSize);
                fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                if (! plugin->getMaker(bufStr))
                    bufStr[0] = '\0';
                bufStrSize = carla_fixedValue(1U, 64U, static_cast<uint32_t>(std::strlen(bufStr)));
                fShmNonRtServerControl.writeUInt(bufStrSize);
                fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                if (! plugin->getCopyright(bufStr))
                    bufStr[0] = '\0';
                bufStrSize = carla_fixedValue(1U, 64U, static_cast<uint32_t>(std::strlen(bufStr)));
                fShmNonRtServerControl.writeUInt(bufStrSize);
                fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                fShmNonRtServerControl.commitWrite();
            }

            fShmNonRtServerControl.waitIfDataIsReachingLimit();

            // kPluginBridgeNonRtServerAudioCount
            {
                const uint32_t aIns  = plugin->getAudioInCount();
                const uint32_t aOuts = plugin->getAudioOutCount();

                // uint/ins, uint/outs
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerAudioCount);
                fShmNonRtServerControl.writeUInt(aIns);
                fShmNonRtServerControl.writeUInt(aOuts);
                fShmNonRtServerControl.commitWrite();

                // kPluginBridgeNonRtServerPortName
                for (uint32_t i=0; i<aIns; ++i)
                {
                    const char* const portName(client->getAudioPortName(true, i));
                    CARLA_SAFE_ASSERT_CONTINUE(portName != nullptr && portName[0] != '\0');

                    // byte/type, uint/index, uint/size, str[] (name)
                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPortName);
                    fShmNonRtServerControl.writeByte(kPluginBridgePortAudioInput);
                    fShmNonRtServerControl.writeUInt(i);

                    bufStrSize = static_cast<uint32_t>(std::strlen(portName));
                    fShmNonRtServerControl.writeUInt(bufStrSize);
                    fShmNonRtServerControl.writeCustomData(portName, bufStrSize);
                }

                // kPluginBridgeNonRtServerPortName
                for (uint32_t i=0; i<aOuts; ++i)
                {
                    const char* const portName(client->getAudioPortName(false, i));
                    CARLA_SAFE_ASSERT_CONTINUE(portName != nullptr && portName[0] != '\0');

                    // byte/type, uint/index, uint/size, str[] (name)
                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPortName);
                    fShmNonRtServerControl.writeByte(kPluginBridgePortAudioOutput);
                    fShmNonRtServerControl.writeUInt(i);

                    bufStrSize = static_cast<uint32_t>(std::strlen(portName));
                    fShmNonRtServerControl.writeUInt(bufStrSize);
                    fShmNonRtServerControl.writeCustomData(portName, bufStrSize);
                }
            }

            fShmNonRtServerControl.waitIfDataIsReachingLimit();

            // kPluginBridgeNonRtServerMidiCount
            {
                // uint/ins, uint/outs
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerMidiCount);
                fShmNonRtServerControl.writeUInt(plugin->getMidiInCount());
                fShmNonRtServerControl.writeUInt(plugin->getMidiOutCount());
                fShmNonRtServerControl.commitWrite();
            }

            fShmNonRtServerControl.waitIfDataIsReachingLimit();

            // kPluginBridgeNonRtServerCvCount
            {
                // uint/ins, uint/outs
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerCvCount);
                fShmNonRtServerControl.writeUInt(plugin->getCVInCount());
                fShmNonRtServerControl.writeUInt(plugin->getCVOutCount());
                fShmNonRtServerControl.commitWrite();
            }

            fShmNonRtServerControl.waitIfDataIsReachingLimit();

            // kPluginBridgeNonRtServerParameter*
            if (const uint32_t count = plugin->getParameterCount())
            {
                // uint/count
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterCount);
                fShmNonRtServerControl.writeUInt(count);
                fShmNonRtServerControl.commitWrite();

                for (uint32_t i=0; i<count; ++i)
                {
                    const ParameterData& paramData(plugin->getParameterData(i));

                    if (paramData.type != PARAMETER_INPUT && paramData.type != PARAMETER_OUTPUT)
                        continue;
                    if ((paramData.hints & PARAMETER_IS_ENABLED) == 0)
                        continue;

                    // kPluginBridgeNonRtServerParameterData1
                    {
                        // uint/index, int/rindex, uint/type, uint/hints, short/cc
                        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterData1);
                        fShmNonRtServerControl.writeUInt(i);
                        fShmNonRtServerControl.writeInt(paramData.rindex);
                        fShmNonRtServerControl.writeUInt(paramData.type);
                        fShmNonRtServerControl.writeUInt(paramData.hints);
                        fShmNonRtServerControl.writeShort(paramData.mappedControlIndex);
                        fShmNonRtServerControl.commitWrite();
                    }

                    // kPluginBridgeNonRtServerParameterData2
                    {
                        // uint/index, uint/size, str[] (name), uint/size, str[] (symbol), uint/size, str[] (unit)
                        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterData2);
                        fShmNonRtServerControl.writeUInt(i);

                        if (! plugin->getParameterName(i, bufStr))
                            std::snprintf(bufStr, STR_MAX, "Param %u", i+1);
                        bufStrSize = carla_fixedValue(1U, 32U, static_cast<uint32_t>(std::strlen(bufStr)));
                        fShmNonRtServerControl.writeUInt(bufStrSize);
                        fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                        if (! plugin->getParameterSymbol(i, bufStr))
                            bufStr[0] = '\0';
                        bufStrSize = carla_fixedValue(1U, 64U, static_cast<uint32_t>(std::strlen(bufStr)));
                        fShmNonRtServerControl.writeUInt(bufStrSize);
                        fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                        if (! plugin->getParameterUnit(i, bufStr))
                            bufStr[0] = '\0';
                        bufStrSize = carla_fixedValue(1U, 32U, static_cast<uint32_t>(std::strlen(bufStr)));
                        fShmNonRtServerControl.writeUInt(bufStrSize);
                        fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                        fShmNonRtServerControl.commitWrite();
                    }

                    // kPluginBridgeNonRtServerParameterRanges
                    {
                        const ParameterRanges& paramRanges(plugin->getParameterRanges(i));

                        // uint/index, float/def, float/min, float/max, float/step, float/stepSmall, float/stepLarge
                        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterRanges);
                        fShmNonRtServerControl.writeUInt(i);
                        fShmNonRtServerControl.writeFloat(paramRanges.def);
                        fShmNonRtServerControl.writeFloat(paramRanges.min);
                        fShmNonRtServerControl.writeFloat(paramRanges.max);
                        fShmNonRtServerControl.writeFloat(paramRanges.step);
                        fShmNonRtServerControl.writeFloat(paramRanges.stepSmall);
                        fShmNonRtServerControl.writeFloat(paramRanges.stepLarge);
                        fShmNonRtServerControl.commitWrite();
                    }

                    // kPluginBridgeNonRtServerParameterValue2
                    {
                        // uint/index float/value (used for init/output parameters only, don't resend values)
                        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterValue2);
                        fShmNonRtServerControl.writeUInt(i);
                        fShmNonRtServerControl.writeFloat(plugin->getParameterValue(i));
                        fShmNonRtServerControl.commitWrite();
                    }

                    fShmNonRtServerControl.waitIfDataIsReachingLimit();
                }
            }

            // kPluginBridgeNonRtServerProgram*
            if (const uint32_t count = plugin->getProgramCount())
            {
                // uint/count
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerProgramCount);
                fShmNonRtServerControl.writeUInt(count);
                fShmNonRtServerControl.commitWrite();

                for (uint32_t i=0; i < count; ++i)
                {
                    // uint/index, uint/size, str[] (name)
                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerProgramName);
                    fShmNonRtServerControl.writeUInt(i);

                    if (! plugin->getProgramName(i, bufStr))
                        bufStr[0] = '\0';
                    bufStrSize = carla_fixedValue(1U, 32U, static_cast<uint32_t>(std::strlen(bufStr)));
                    fShmNonRtServerControl.writeUInt(bufStrSize);
                    fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                    fShmNonRtServerControl.commitWrite();
                    fShmNonRtServerControl.waitIfDataIsReachingLimit();
                }
            }

            // kPluginBridgeNonRtServerMidiProgram*
            if (const uint32_t count = plugin->getMidiProgramCount())
            {
                // uint/count
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerMidiProgramCount);
                fShmNonRtServerControl.writeUInt(count);
                fShmNonRtServerControl.commitWrite();

                for (uint32_t i=0; i < count; ++i)
                {
                    const MidiProgramData& mpData(plugin->getMidiProgramData(i));
                    CARLA_SAFE_ASSERT_CONTINUE(mpData.name != nullptr);

                    // uint/index, uint/bank, uint/program, uint/size, str[] (name)
                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerMidiProgramData);
                    fShmNonRtServerControl.writeUInt(i);
                    fShmNonRtServerControl.writeUInt(mpData.bank);
                    fShmNonRtServerControl.writeUInt(mpData.program);

                    bufStrSize = carla_fixedValue(1U, 32U, static_cast<uint32_t>(std::strlen(mpData.name)));
                    fShmNonRtServerControl.writeUInt(bufStrSize);
                    fShmNonRtServerControl.writeCustomData(mpData.name, bufStrSize);

                    fShmNonRtServerControl.commitWrite();
                    fShmNonRtServerControl.waitIfDataIsReachingLimit();
                }
            }

            if (const uint32_t latency = plugin->getLatencyInFrames())
            {
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSetLatency);
                fShmNonRtServerControl.writeUInt(latency);
                fShmNonRtServerControl.commitWrite();
            }

            // ready!
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerReady);
            fShmNonRtServerControl.commitWrite();
            fShmNonRtServerControl.waitIfDataIsReachingLimit();

            carla_stdout("Carla Bridge Ready!");
            fLastPingTime = d_gettime_ms();
        }

        // send parameter outputs
        if (const uint32_t count = plugin->getParameterCount())
        {
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

            for (uint32_t i=0; i < count; ++i)
            {
                if (! plugin->isParameterOutput(i))
                    continue;

                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterValue2);
                fShmNonRtServerControl.writeUInt(i);
                fShmNonRtServerControl.writeFloat(plugin->getParameterValue(i));

                // parameter outputs are not that important, we can skip some
                if (! fShmNonRtServerControl.commitWrite())
                    break;
            }
        }

        CarlaEngine::idle();

        try {
            handleNonRtData();
        } CARLA_SAFE_EXCEPTION("handleNonRtData");

        if (fLastPingTime != UINT32_MAX && d_gettime_ms() > fLastPingTime + 30000 && ! wasFirstIdle)
        {
            carla_stderr("Did not receive ping message from server for 30 secs, closing...");
            signalThreadShouldExit();
            callback(true, true, ENGINE_CALLBACK_QUIT, 0, 0, 0, 0, 0.0f, nullptr);
        }
    }

    void callback(const bool sendHost, const bool sendOsc,
                  const EngineCallbackOpcode action, const uint pluginId,
                  const int value1, const int value2, const int value3,
                  const float valuef, const char* const valueStr) noexcept override
    {
        CarlaEngine::callback(sendHost, sendOsc, action, pluginId, value1, value2, value3, valuef, valueStr);

        if (fClosingDown || ! sendHost)
            return;

        switch (action)
        {
        // uint/index float/value
        case ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED: {
            CARLA_SAFE_ASSERT_BREAK(value1 >= 0);
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterValue);
            fShmNonRtServerControl.writeUInt(static_cast<uint>(value1));
            fShmNonRtServerControl.writeFloat(valuef);
            fShmNonRtServerControl.commitWrite();
        }   break;

        // uint/index float/value
        case ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED: {
            CARLA_SAFE_ASSERT_BREAK(value1 >= 0);
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerDefaultValue);
            fShmNonRtServerControl.writeUInt(static_cast<uint>(value1));
            fShmNonRtServerControl.writeFloat(valuef);
            fShmNonRtServerControl.commitWrite();
        }   break;

        // int/index
        case ENGINE_CALLBACK_PROGRAM_CHANGED: {
            CARLA_SAFE_ASSERT_BREAK(value1 >= -1);
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerCurrentProgram);
            fShmNonRtServerControl.writeInt(value1);
            fShmNonRtServerControl.commitWrite();
        }   break;

        // int/index
        case ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED: {
            CARLA_SAFE_ASSERT_BREAK(value1 >= -1);
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerCurrentMidiProgram);
            fShmNonRtServerControl.writeInt(value1);
            fShmNonRtServerControl.commitWrite();
        }   break;

        case ENGINE_CALLBACK_UI_STATE_CHANGED:
            if (value1 != 1)
            {
                const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerUiClosed);
                fShmNonRtServerControl.commitWrite();
            }
            break;

        case ENGINE_CALLBACK_EMBED_UI_RESIZED: {
            CARLA_SAFE_ASSERT_BREAK(value1 > 1);
            CARLA_SAFE_ASSERT_BREAK(value2 > 1);

            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerResizeEmbedUI);
            fShmNonRtServerControl.writeUInt(static_cast<uint>(value1));
            fShmNonRtServerControl.writeUInt(static_cast<uint>(value2));
            fShmNonRtServerControl.commitWrite();
        }   break;

        case ENGINE_CALLBACK_RELOAD_PARAMETERS:
            if (const CarlaPluginPtr plugin = pData->plugins[0].plugin)
            {
                if (const uint32_t count = plugin->getParameterCount())
                {
                    const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                    for (uint32_t i=0; i<count; ++i)
                    {
                        const ParameterData& paramData(plugin->getParameterData(i));

                        if (paramData.type != PARAMETER_INPUT && paramData.type != PARAMETER_OUTPUT)
                            continue;
                        if ((paramData.hints & PARAMETER_IS_ENABLED) == 0)
                            continue;

                        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterValue);
                        fShmNonRtServerControl.writeUInt(i);
                        fShmNonRtServerControl.writeFloat(plugin->getParameterValue(i));
                        fShmNonRtServerControl.commitWrite();

                        fShmNonRtServerControl.waitIfDataIsReachingLimit();
                    }
                }
            }
            break;

        default:
            break;
        }
    }

    // -------------------------------------------------------------------

    void clear() noexcept
    {
        fShmAudioPool.clear();
        fShmRtClientControl.clear();
        fShmNonRtClientControl.clear();
        fShmNonRtServerControl.clear();
    }

    void handleNonRtData()
    {
        const CarlaPluginPtr plugin = pData->plugins[0].plugin;
        CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr,);

        for (; fShmNonRtClientControl.isDataAvailableForReading();)
        {
            const PluginBridgeNonRtClientOpcode opcode = fShmNonRtClientControl.readOpcode();

#ifdef DEBUG
            if (opcode != kPluginBridgeNonRtClientPing) {
                carla_debug("CarlaEngineBridge::handleNonRtData() - got opcode: %i:%s",
                            opcode, PluginBridgeNonRtClientOpcode2str(opcode));
            }
#endif

            if (opcode != kPluginBridgeNonRtClientNull &&
                opcode != kPluginBridgeNonRtClientPingOnOff && fLastPingTime != UINT32_MAX)
                fLastPingTime = d_gettime_ms();

            switch (opcode)
            {
            case kPluginBridgeNonRtClientNull:
                break;

            case kPluginBridgeNonRtClientVersion: {
                fBridgeVersion = fShmNonRtServerControl.readUInt();
                CARLA_SAFE_ASSERT_UINT2(fBridgeVersion >= CARLA_PLUGIN_BRIDGE_API_VERSION_MINIMUM,
                                        fBridgeVersion, CARLA_PLUGIN_BRIDGE_API_VERSION_MINIMUM);
            }   break;

            case kPluginBridgeNonRtClientPing: {
                const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPong);
                fShmNonRtServerControl.commitWrite();
            }   break;

            case kPluginBridgeNonRtClientPingOnOff:
                fLastPingTime = fShmNonRtClientControl.readBool() ? d_gettime_ms() : UINT32_MAX;
                break;

            case kPluginBridgeNonRtClientActivate:
                if (plugin->isEnabled())
                    plugin->setActive(true, false, false);
                break;

            case kPluginBridgeNonRtClientDeactivate:
                if (plugin->isEnabled())
                    plugin->setActive(false, false, false);
                break;

            case kPluginBridgeNonRtClientInitialSetup:
                // should never happen!!
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readDouble();
                break;

            case kPluginBridgeNonRtClientSetParameterValue: {
                const uint32_t index(fShmNonRtClientControl.readUInt());
                const float    value(fShmNonRtClientControl.readFloat());

                if (plugin->isEnabled())
                    plugin->setParameterValue(index, value, false, false, false);
                break;
            }

            case kPluginBridgeNonRtClientSetParameterMidiChannel: {
                const uint32_t index(fShmNonRtClientControl.readUInt());
                const uint8_t  channel(fShmNonRtClientControl.readByte());

                if (plugin->isEnabled())
                    plugin->setParameterMidiChannel(index, channel, false, false);
                break;
            }

            case kPluginBridgeNonRtClientSetParameterMappedControlIndex: {
                const uint32_t index(fShmNonRtClientControl.readUInt());
                const int16_t  ctrl(fShmNonRtClientControl.readShort());

                if (plugin->isEnabled())
                    plugin->setParameterMappedControlIndex(index, ctrl, false, false, true);
                break;
            }

            case kPluginBridgeNonRtClientSetParameterMappedRange: {
                const uint32_t index   = fShmNonRtClientControl.readUInt();
                const float    minimum = fShmNonRtClientControl.readFloat();
                const float    maximum = fShmNonRtClientControl.readFloat();

                if (plugin->isEnabled())
                    plugin->setParameterMappedRange(index, minimum, maximum, false, false);
                break;
            }

            case kPluginBridgeNonRtClientSetProgram: {
                const int32_t index(fShmNonRtClientControl.readInt());

                if (plugin->isEnabled())
                    plugin->setProgram(index, true, false, false);
                break;
            }

            case kPluginBridgeNonRtClientSetMidiProgram: {
                const int32_t index(fShmNonRtClientControl.readInt());

                if (plugin->isEnabled())
                    plugin->setMidiProgram(index, true, false, false);
                break;
            }

            case kPluginBridgeNonRtClientSetCustomData: {
                const uint32_t maxLocalValueLen = fBridgeVersion >= 10 ? 4096 : 16384;

                // type
                const BridgeTextReader type(fShmNonRtClientControl);

                // key
                const BridgeTextReader key(fShmNonRtClientControl);

                // value
                const uint32_t valueSize = fShmNonRtClientControl.readUInt();

                if (valueSize > 0)
                {
                    if (valueSize > maxLocalValueLen)
                    {
                        const BridgeTextReader bigValueFilePathTry(fShmNonRtClientControl);

                        CARLA_SAFE_ASSERT_BREAK(bigValueFilePathTry.text[0] != '\0');
                        if (! plugin->isEnabled()) break;

                        water::String bigValueFilePath(bigValueFilePathTry.text);

#ifdef CARLA_OS_WIN
                        // check if running under Wine
                        if (bigValueFilePath.startsWith("/"))
                            bigValueFilePath = bigValueFilePath.replaceSection(0, 1, "Z:\\").replace("/", "\\");
#endif

                        File bigValueFile(bigValueFilePath.toRawUTF8());
                        CARLA_SAFE_ASSERT_BREAK(bigValueFile.existsAsFile());

                        plugin->setCustomData(type.text, key.text, bigValueFile.loadFileAsString().toRawUTF8(), true);

                        bigValueFile.deleteFile();
                    }
                    else
                    {
                        const BridgeTextReader value(fShmNonRtClientControl, valueSize);

                        if (plugin->isEnabled())
                            plugin->setCustomData(type.text, key.text, value.text, true);
                    }
                }
                else
                {
                    if (plugin->isEnabled())
                        plugin->setCustomData(type.text, key.text, "", true);
                }

                break;
            }

            case kPluginBridgeNonRtClientSetChunkDataFile: {
                const uint32_t size = fShmNonRtClientControl.readUInt();
                CARLA_SAFE_ASSERT_BREAK(size > 0);

                const BridgeTextReader chunkFilePathTry(fShmNonRtClientControl, size);

                CARLA_SAFE_ASSERT_BREAK(chunkFilePathTry.text[0] != '\0');
                if (! plugin->isEnabled()) break;

                water::String chunkFilePath(chunkFilePathTry.text);

#ifdef CARLA_OS_WIN
                // check if running under Wine
                if (chunkFilePath.startsWith("/"))
                    chunkFilePath = chunkFilePath.replaceSection(0, 1, "Z:\\").replace("/", "\\");
#endif

                File chunkFile(chunkFilePath.toRawUTF8());
                CARLA_SAFE_ASSERT_BREAK(chunkFile.existsAsFile());

                water::String chunkDataBase64(chunkFile.loadFileAsString());
                chunkFile.deleteFile();
                CARLA_SAFE_ASSERT_BREAK(chunkDataBase64.isNotEmpty());

                std::vector<uint8_t> chunk;
                d_getChunkFromBase64String_impl(chunk, chunkDataBase64.toRawUTF8());

#ifdef CARLA_PROPER_CPP11_SUPPORT
                plugin->setChunkData(chunk.data(), chunk.size());
#else
                plugin->setChunkData(&chunk.front(), chunk.size());
#endif
                break;
            }

            case kPluginBridgeNonRtClientSetCtrlChannel: {
                const int16_t channel(fShmNonRtClientControl.readShort());
                CARLA_SAFE_ASSERT_BREAK(channel >= -1 && channel < MAX_MIDI_CHANNELS);

                if (plugin->isEnabled())
                    plugin->setCtrlChannel(static_cast<int8_t>(channel), false, false);
                break;
            }

            case kPluginBridgeNonRtClientSetOption: {
                const uint32_t option(fShmNonRtClientControl.readUInt());
                const bool     yesNo(fShmNonRtClientControl.readBool());

                if (plugin->isEnabled())
                    plugin->setOption(option, yesNo, false);
                break;
            }

            case kPluginBridgeNonRtClientSetOptions: {
                const uint32_t options(fShmNonRtClientControl.readUInt());

                plugin->pData->options = options;
                break;
            }

            case kPluginBridgeNonRtClientSetWindowTitle: {
                const BridgeTextReader title(fShmNonRtClientControl);

                plugin->setCustomUITitle(title.text);
                break;
            }

            case kPluginBridgeNonRtClientGetParameterText: {
                const int32_t index = fShmNonRtClientControl.readInt();

                if (index >= 0 && plugin->isEnabled())
                {
                    char bufStr[STR_MAX+1];
                    carla_zeroChars(bufStr, STR_MAX+1);
                    if (! plugin->getParameterText(static_cast<uint32_t>(index), bufStr))
                        bufStr[0] = '\0';

                    const uint32_t bufStrLen = static_cast<uint32_t>(std::strlen(bufStr));

                    const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSetParameterText);

                    fShmNonRtServerControl.writeInt(index);
                    fShmNonRtServerControl.writeUInt(bufStrLen);
                    fShmNonRtServerControl.writeCustomData(bufStr, bufStrLen);
                    fShmNonRtServerControl.commitWrite();

                    fShmNonRtServerControl.waitIfDataIsReachingLimit();
                }
                else
                {
                    const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSetParameterText);
                    fShmNonRtServerControl.writeInt(index);
                    fShmNonRtServerControl.writeUInt(0);
                    fShmNonRtServerControl.commitWrite();
                }

                break;
            }

            case kPluginBridgeNonRtClientPrepareForSave: {
                if (! plugin->isEnabled())
                {
                    const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSaved);
                    fShmNonRtServerControl.commitWrite();
                    return;
                }

                plugin->prepareForSave(false);

                const uint32_t maxLocalValueLen = fBridgeVersion >= 10 ? 4096 : 16384;

                for (uint32_t i=0, count=plugin->getCustomDataCount(); i<count; ++i)
                {
                    const CustomData& cdata(plugin->getCustomData(i));

                    if (std::strcmp(cdata.type, CUSTOM_DATA_TYPE_PROPERTY) == 0)
                        continue;

                    const uint32_t typeLen  = static_cast<uint32_t>(std::strlen(cdata.type));
                    const uint32_t keyLen   = static_cast<uint32_t>(std::strlen(cdata.key));
                    const uint32_t valueLen = static_cast<uint32_t>(std::strlen(cdata.value));

                    {
                        const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                        if (valueLen > maxLocalValueLen)
                            fShmNonRtServerControl.waitIfDataIsReachingLimit();

                        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSetCustomData);

                        fShmNonRtServerControl.writeUInt(typeLen);
                        fShmNonRtServerControl.writeCustomData(cdata.type, typeLen);

                        fShmNonRtServerControl.writeUInt(keyLen);
                        fShmNonRtServerControl.writeCustomData(cdata.key, keyLen);

                        fShmNonRtServerControl.writeUInt(valueLen);

                        if (valueLen > 0)
                        {
                            if (valueLen > maxLocalValueLen)
                            {
                                water::String filePath(File::getSpecialLocation(File::tempDirectory).getFullPathName());

                                filePath += CARLA_OS_SEP_STR ".CarlaCustomData_";
                                filePath += fShmAudioPool.getFilenameSuffix();

                                if (File(filePath.toRawUTF8()).replaceWithText(cdata.value))
                                {
                                    const uint32_t ulength(static_cast<uint32_t>(filePath.length()));

                                    fShmNonRtServerControl.writeUInt(ulength);
                                    fShmNonRtServerControl.writeCustomData(filePath.toRawUTF8(), ulength);
                                }
                                else
                                {
                                    fShmNonRtServerControl.writeUInt(0);
                                }
                            }
                            else
                            {
                                fShmNonRtServerControl.writeCustomData(cdata.value, valueLen);
                            }
                        }

                        fShmNonRtServerControl.commitWrite();
                        fShmNonRtServerControl.waitIfDataIsReachingLimit();
                    }
                }

                if (plugin->getOptionsEnabled() & PLUGIN_OPTION_USE_CHUNKS)
                {
                    void* data = nullptr;
                    if (const std::size_t dataSize = plugin->getChunkData(&data))
                    {
                        CARLA_SAFE_ASSERT_BREAK(data != nullptr);

                        String dataBase64 = String::asBase64(data, dataSize);
                        CARLA_SAFE_ASSERT_BREAK(dataBase64.length() > 0);

                        water::String filePath(File::getSpecialLocation(File::tempDirectory).getFullPathName());

                        filePath += CARLA_OS_SEP_STR ".CarlaChunk_";
                        filePath += fShmAudioPool.getFilenameSuffix();

                        if (File(filePath.toRawUTF8()).replaceWithText(dataBase64.buffer()))
                        {
                            const uint32_t ulength(static_cast<uint32_t>(filePath.length()));

                            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSetChunkDataFile);
                            fShmNonRtServerControl.writeUInt(ulength);
                            fShmNonRtServerControl.writeCustomData(filePath.toRawUTF8(), ulength);
                            fShmNonRtServerControl.commitWrite();
                        }
                    }
                }

                {
                    const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSaved);
                    fShmNonRtServerControl.commitWrite();
                }
                break;
            }

            case kPluginBridgeNonRtClientRestoreLV2State:
                if (plugin->isEnabled())
                    plugin->restoreLV2State(false);
                break;

            case kPluginBridgeNonRtClientShowUI:
                if (plugin->isEnabled())
                    plugin->showCustomUI(true);
                break;

            case kPluginBridgeNonRtClientHideUI:
                if (plugin->isEnabled())
                    plugin->showCustomUI(false);
                break;

            case kPluginBridgeNonRtClientEmbedUI: {
                const uint64_t winId = fShmNonRtClientControl.readULong();
                uint64_t resp = 0;

                if (plugin->isEnabled())
                    resp = reinterpret_cast<uint64_t>(plugin->embedCustomUI(reinterpret_cast<void*>(winId)));

                if (resp == 0)
                    resp = 1;

                const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerRespEmbedUI);
                fShmNonRtServerControl.writeULong(resp);
                fShmNonRtServerControl.commitWrite();
                break;
            }

            case kPluginBridgeNonRtClientUiParameterChange: {
                const uint32_t index = fShmNonRtClientControl.readUInt();
                const float    value = fShmNonRtClientControl.readFloat();

                if (plugin->isEnabled())
                    plugin->uiParameterChange(index, value);
                break;
            }

            case kPluginBridgeNonRtClientUiProgramChange: {
                const uint32_t index(fShmNonRtClientControl.readUInt());

                if (plugin->isEnabled())
                    plugin->uiProgramChange(index);
                break;
            }

            case kPluginBridgeNonRtClientUiMidiProgramChange: {
                const uint32_t index(fShmNonRtClientControl.readUInt());

                if (plugin->isEnabled())
                    plugin->uiMidiProgramChange(index);
                break;
            }

            case kPluginBridgeNonRtClientUiNoteOn: {
                const uint8_t chnl(fShmNonRtClientControl.readByte());
                const uint8_t note(fShmNonRtClientControl.readByte());
                const uint8_t velo(fShmNonRtClientControl.readByte());

                if (plugin->isEnabled())
                    plugin->uiNoteOn(chnl, note, velo);
                break;
            }

            case kPluginBridgeNonRtClientUiNoteOff: {
                const uint8_t chnl(fShmNonRtClientControl.readByte());
                const uint8_t note(fShmNonRtClientControl.readByte());

                if (plugin->isEnabled())
                    plugin->uiNoteOff(chnl, note);
                break;
            }

            case kPluginBridgeNonRtClientQuit:
                fClosingDown = true;
                signalThreadShouldExit();
                callback(true, true, ENGINE_CALLBACK_QUIT, 0, 0, 0, 0, 0.0f, nullptr);
                break;

            case kPluginBridgeNonRtClientReload:
                fFirstIdle = true;
                break;
            }
        }
    }

    // -------------------------------------------------------------------

protected:
    void run() override
    {
#ifdef __SSE2_MATH__
        // Set FTZ and DAZ flags
        _mm_setcsr(_mm_getcsr() | 0x8040);
#endif

        bool quitReceived = false;

        for (; ! shouldThreadExit();)
        {
            const BridgeRtClientControl::WaitHelper helper(fShmRtClientControl);

            if (! helper.ok)
                continue;

            for (; fShmRtClientControl.isDataAvailableForReading();)
            {
                const PluginBridgeRtClientOpcode opcode(fShmRtClientControl.readOpcode());
                const CarlaPluginPtr plugin = pData->plugins[0].plugin;

#ifdef DEBUG
                if (opcode != kPluginBridgeRtClientProcess && opcode != kPluginBridgeRtClientMidiEvent) {
                    carla_debug("CarlaEngineBridgeRtThread::run() - got opcode: %s", PluginBridgeRtClientOpcode2str(opcode));
                }
#endif

                switch (opcode)
                {
                case kPluginBridgeRtClientNull:
                    break;

                case kPluginBridgeRtClientSetAudioPool: {
                    if (fShmAudioPool.data != nullptr)
                    {
                        jackbridge_shm_unmap(fShmAudioPool.shm, fShmAudioPool.data);
                        fShmAudioPool.data = nullptr;
                    }
                    const uint64_t poolSize(fShmRtClientControl.readULong());
                    CARLA_SAFE_ASSERT_BREAK(poolSize > 0);
                    fShmAudioPool.data = (float*)jackbridge_shm_map(fShmAudioPool.shm, static_cast<size_t>(poolSize));
                    break;
                }

                case kPluginBridgeRtClientSetBufferSize: {
                    const uint32_t bufferSize(fShmRtClientControl.readUInt());
                    pData->bufferSize = bufferSize;
                    bufferSizeChanged(bufferSize);
                    break;
                }

                case kPluginBridgeRtClientSetSampleRate: {
                    const double sampleRate(fShmRtClientControl.readDouble());
                    pData->sampleRate = sampleRate;
                    sampleRateChanged(sampleRate);
                    break;
                }

                case kPluginBridgeRtClientSetOnline:
                    fIsOffline = fShmRtClientControl.readBool();
                    offlineModeChanged(fIsOffline);
                    break;

                // NOTE this is never used
                case kPluginBridgeRtClientControlEventParameter: {
                    const uint32_t time(fShmRtClientControl.readUInt());
                    const uint8_t  channel(fShmRtClientControl.readByte());
                    const uint16_t param(fShmRtClientControl.readUShort());
                    const float    value(fShmRtClientControl.readFloat());

                    if (EngineEvent* const event = getNextFreeInputEvent())
                    {
                        event->type                 = kEngineEventTypeControl;
                        event->time                 = time;
                        event->channel              = channel;
                        event->ctrl.type            = kEngineControlEventTypeParameter;
                        event->ctrl.param           = param;
                        event->ctrl.midiValue       = -1;
                        event->ctrl.normalizedValue = value;
                        event->ctrl.handled         = true;
                    }
                    break;
                }

                case kPluginBridgeRtClientControlEventMidiBank: {
                    const uint32_t time(fShmRtClientControl.readUInt());
                    const uint8_t  channel(fShmRtClientControl.readByte());
                    const uint16_t index(fShmRtClientControl.readUShort());

                    if (EngineEvent* const event = getNextFreeInputEvent())
                    {
                        event->type                 = kEngineEventTypeControl;
                        event->time                 = time;
                        event->channel              = channel;
                        event->ctrl.type            = kEngineControlEventTypeMidiBank;
                        event->ctrl.param           = index;
                        event->ctrl.midiValue       = -1;
                        event->ctrl.normalizedValue = 0.0f;
                        event->ctrl.handled         = true;
                    }
                    break;
                }

                case kPluginBridgeRtClientControlEventMidiProgram: {
                    const uint32_t time(fShmRtClientControl.readUInt());
                    const uint8_t  channel(fShmRtClientControl.readByte());
                    const uint16_t index(fShmRtClientControl.readUShort());

                    if (EngineEvent* const event = getNextFreeInputEvent())
                    {
                        event->type                 = kEngineEventTypeControl;
                        event->time                 = time;
                        event->channel              = channel;
                        event->ctrl.type            = kEngineControlEventTypeMidiProgram;
                        event->ctrl.param           = index;
                        event->ctrl.midiValue       = -1;
                        event->ctrl.normalizedValue = 0.0f;
                        event->ctrl.handled         = true;
                    }
                    break;
                }

                case kPluginBridgeRtClientControlEventAllSoundOff: {
                    const uint32_t time(fShmRtClientControl.readUInt());
                    const uint8_t  channel(fShmRtClientControl.readByte());

                    if (EngineEvent* const event = getNextFreeInputEvent())
                    {
                        event->type                 = kEngineEventTypeControl;
                        event->time                 = time;
                        event->channel              = channel;
                        event->ctrl.type            = kEngineControlEventTypeAllSoundOff;
                        event->ctrl.param           = 0;
                        event->ctrl.midiValue       = -1;
                        event->ctrl.normalizedValue = 0.0f;
                        event->ctrl.handled         = true;
                    }
                }   break;

                case kPluginBridgeRtClientControlEventAllNotesOff: {
                    const uint32_t time(fShmRtClientControl.readUInt());
                    const uint8_t  channel(fShmRtClientControl.readByte());

                    if (EngineEvent* const event = getNextFreeInputEvent())
                    {
                        event->type                 = kEngineEventTypeControl;
                        event->time                 = time;
                        event->channel              = channel;
                        event->ctrl.type            = kEngineControlEventTypeAllNotesOff;
                        event->ctrl.param           = 0;
                        event->ctrl.midiValue       = -1;
                        event->ctrl.normalizedValue = 0.0f;
                        event->ctrl.handled         = true;
                    }
                }   break;

                case kPluginBridgeRtClientMidiEvent: {
                    const uint32_t time(fShmRtClientControl.readUInt());
                    const uint8_t  port(fShmRtClientControl.readByte());
                    const uint8_t  size(fShmRtClientControl.readByte());
                    CARLA_SAFE_ASSERT_BREAK(size > 0);

                    // FIXME variable-size stack
                    uint8_t data[4];

                    {
                        uint8_t i=0;
                        for (; i<size && i<4; ++i)
                            data[i] = fShmRtClientControl.readByte();
                        for (; i<size; ++i)
                            fShmRtClientControl.readByte();
                    }

                    if (size > 4)
                        continue;

                    if (EngineEvent* const event = getNextFreeInputEvent())
                    {
                        event->type    = kEngineEventTypeMidi;
                        event->time    = time;
                        event->channel = MIDI_GET_CHANNEL_FROM_DATA(data);

                        event->midi.port = port;
                        event->midi.size = size;

                        if (size > EngineMidiEvent::kDataSize)
                        {
                            event->midi.dataExt = data;
                            std::memset(event->midi.data, 0, sizeof(uint8_t)*EngineMidiEvent::kDataSize);
                        }
                        else
                        {
                            event->midi.data[0] = MIDI_GET_STATUS_FROM_DATA(data);

                            uint8_t i=1;
                            for (; i < size; ++i)
                                event->midi.data[i] = data[i];
                            for (; i < EngineMidiEvent::kDataSize; ++i)
                                event->midi.data[i] = 0;

                            event->midi.dataExt = nullptr;
                        }
                    }
                    break;
                }

                case kPluginBridgeRtClientProcess: {
                    const uint32_t frames(fShmRtClientControl.readUInt());

                    CARLA_SAFE_ASSERT_BREAK(fShmAudioPool.data != nullptr);

                    if (plugin.get() != nullptr && plugin->isEnabled() && plugin->tryLock(fIsOffline))
                    {
                        const BridgeTimeInfo& bridgeTimeInfo(fShmRtClientControl.data->timeInfo);

                        const uint32_t audioInCount = plugin->getAudioInCount();
                        const uint32_t audioOutCount = plugin->getAudioOutCount();
                        const uint32_t cvInCount = plugin->getCVInCount();
                        const uint32_t cvOutCount = plugin->getCVOutCount();

                        const float* audioIn[64];
                        /* */ float* audioOut[64];
                        const float* cvIn[32];
                        /* */ float* cvOut[32];

                        float* fdata = fShmAudioPool.data;

                        for (uint32_t i=0; i < audioInCount; ++i, fdata += pData->bufferSize)
                            audioIn[i] = fdata;
                        for (uint32_t i=0; i < audioOutCount; ++i, fdata += pData->bufferSize)
                            audioOut[i] = fdata;

                        for (uint32_t i=0; i < cvInCount; ++i, fdata += pData->bufferSize)
                            cvIn[i] = fdata;
                        for (uint32_t i=0; i < cvOutCount; ++i, fdata += pData->bufferSize)
                            cvOut[i] = fdata;

                        EngineTimeInfo& timeInfo(pData->timeInfo);

                        timeInfo.playing   = bridgeTimeInfo.playing;
                        timeInfo.frame     = bridgeTimeInfo.frame;
                        timeInfo.usecs     = bridgeTimeInfo.usecs;
                        timeInfo.bbt.valid = (bridgeTimeInfo.validFlags & kPluginBridgeTimeInfoValidBBT) != 0;

                        if (timeInfo.bbt.valid)
                        {
                            timeInfo.bbt.bar  = bridgeTimeInfo.bar;
                            timeInfo.bbt.beat = bridgeTimeInfo.beat;
                            timeInfo.bbt.tick = bridgeTimeInfo.tick;

                            timeInfo.bbt.beatsPerBar = bridgeTimeInfo.beatsPerBar;
                            timeInfo.bbt.beatType    = bridgeTimeInfo.beatType;

                            timeInfo.bbt.ticksPerBeat   = bridgeTimeInfo.ticksPerBeat;
                            timeInfo.bbt.beatsPerMinute = bridgeTimeInfo.beatsPerMinute;
                            timeInfo.bbt.barStartTick   = bridgeTimeInfo.barStartTick;
                        }

                        plugin->initBuffers();
                        plugin->process(audioIn, audioOut, cvIn, cvOut, frames);
                        plugin->unlock();
                    }

                    uint8_t* midiData(fShmRtClientControl.data->midiOut);
                    carla_zeroBytes(midiData, kBridgeBaseMidiOutHeaderSize);
                    std::size_t curMidiDataPos = 0;

                    if (pData->events.in[0].type != kEngineEventTypeNull)
                        carla_zeroStructs(pData->events.in, kMaxEngineEventInternalCount);

                    if (pData->events.out[0].type != kEngineEventTypeNull)
                    {
                        for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
                        {
                            const EngineEvent& event(pData->events.out[i]);

                            if (event.type == kEngineEventTypeNull)
                                break;

                            if (event.type == kEngineEventTypeControl)
                            {
                                uint8_t data[3];
                                const uint8_t size = event.ctrl.convertToMidiData(event.channel, data);
                                CARLA_SAFE_ASSERT_CONTINUE(size > 0 && size <= 3);

                                if (curMidiDataPos + kBridgeBaseMidiOutHeaderSize + size >= kBridgeRtClientDataMidiOutSize)
                                    break;

                                // set time
                                *(uint32_t*)midiData = event.time;
                                midiData = midiData + 4;
                                curMidiDataPos += 4;

                                // set port
                                *midiData++ = 0;
                                ++curMidiDataPos;

                                // set size
                                *midiData++ = size;
                                ++curMidiDataPos;

                                // set data
                                for (uint8_t j=0; j<size; ++j)
                                    *midiData++ = data[j];

                                curMidiDataPos += size;
                            }
                            else if (event.type == kEngineEventTypeMidi)
                            {
                                const EngineMidiEvent& _midiEvent(event.midi);

                                if (curMidiDataPos + kBridgeBaseMidiOutHeaderSize + _midiEvent.size >= kBridgeRtClientDataMidiOutSize)
                                    break;

                                const uint8_t* const _midiData(_midiEvent.dataExt != nullptr ? _midiEvent.dataExt : _midiEvent.data);

                                // set time
                                *(uint32_t*)midiData = event.time;
                                midiData += 4;
                                curMidiDataPos += 4;

                                // set port
                                *midiData++ = _midiEvent.port;
                                ++curMidiDataPos;

                                // set size
                                *midiData++ = _midiEvent.size;
                                ++curMidiDataPos;

                                // set data
                                *midiData++ = uint8_t(_midiData[0] | (event.channel & MIDI_CHANNEL_BIT));

                                for (uint8_t j=1; j<_midiEvent.size; ++j)
                                    *midiData++ = _midiData[j];

                                curMidiDataPos += _midiEvent.size;
                            }
                        }

                        if (curMidiDataPos != 0 &&
                            curMidiDataPos + kBridgeBaseMidiOutHeaderSize < kBridgeRtClientDataMidiOutSize)
                            carla_zeroBytes(midiData, kBridgeBaseMidiOutHeaderSize);

                        carla_zeroStructs(pData->events.out, kMaxEngineEventInternalCount);
                    }

                }   break;

                case kPluginBridgeRtClientQuit: {
                    quitReceived = true;
                    fClosingDown = true;
                    signalThreadShouldExit();
                }   break;
                }
            }
        }

        callback(true, true, ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0, 0.0f, nullptr);

        if (! quitReceived)
        {
            const char* const message("Plugin bridge error, process thread has stopped");
            const std::size_t messageSize(std::strlen(message));

            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerError);
            fShmNonRtServerControl.writeUInt(messageSize);
            fShmNonRtServerControl.writeCustomData(message, messageSize);
            fShmNonRtServerControl.commitWrite();
        }
    }

    // called from process thread above
    EngineEvent* getNextFreeInputEvent() const noexcept
    {
        for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
        {
            EngineEvent* const event(&pData->events.in[i]);

            if (event->type == kEngineEventTypeNull)
                return event;
        }
        return nullptr;
    }

    void latencyChanged(const uint32_t samples) noexcept override
    {
        const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSetLatency);
        fShmNonRtServerControl.writeUInt(samples);
        fShmNonRtServerControl.commitWrite();
    }

    // -------------------------------------------------------------------

private:
    BridgeAudioPool          fShmAudioPool;
    BridgeRtClientControl    fShmRtClientControl;
    BridgeNonRtClientControl fShmNonRtClientControl;
    BridgeNonRtServerControl fShmNonRtServerControl;

    String fBaseNameAudioPool;
    String fBaseNameRtClientControl;
    String fBaseNameNonRtClientControl;
    String fBaseNameNonRtServerControl;

    bool fClosingDown;
    bool fIsOffline;
    bool fFirstIdle;
    uint32_t fBridgeVersion;
    uint32_t fLastPingTime;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineBridge)
};

// -----------------------------------------------------------------------

namespace EngineInit {

CarlaEngine* newBridge(const char* const audioPoolBaseName,
                       const char* const rtClientBaseName,
                       const char* const nonRtClientBaseName,
                       const char* const nonRtServerBaseName)
{
    return new CarlaEngineBridge(audioPoolBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName);
}

}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

// -----------------------------------------------------------------------

#include "CarlaBridgeUtils.cpp"

// -----------------------------------------------------------------------
