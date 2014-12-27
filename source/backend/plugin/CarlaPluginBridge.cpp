/*
 * Carla Plugin Bridge
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef BUILD_BRIDGE

#include "CarlaBackendUtils.hpp"
#include "CarlaBase64Utils.hpp"
#include "CarlaBridgeUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaShmUtils.hpp"
#include "CarlaThread.hpp"

#include "jackbridge/JackBridge.hpp"

#include <cerrno>
#include <cmath>
#include <ctime>

// -------------------------------------------------------------------------------------------------------------------

using juce::ChildProcess;
using juce::File;
using juce::ScopedPointer;
using juce::String;
using juce::StringArray;

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------

struct BridgeAudioPool {
    CarlaString filename;
    std::size_t size;
    float* data;
    shm_t shm;

    BridgeAudioPool() noexcept
        : filename(),
          size(0),
          data(nullptr)
#ifdef CARLA_PROPER_CPP11_SUPPORT
        , shm(shm_t_INIT) {}
#else
    {
        shm = shm_t_INIT;
    }
#endif

    ~BridgeAudioPool() noexcept
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    void clear() noexcept
    {
        filename.clear();

        if (! carla_is_shm_valid(shm))
        {
            CARLA_SAFE_ASSERT(data == nullptr);
            return;
        }

        if (data != nullptr)
        {
            carla_shm_unmap(shm, data);
            data = nullptr;
        }

        size = 0;
        carla_shm_close(shm);
        carla_shm_init(shm);
    }

    void resize(const uint32_t bufferSize, const uint32_t audioPortCount, const uint32_t cvPortCount) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm),);

        if (data != nullptr)
            carla_shm_unmap(shm, data);

        size = (audioPortCount+cvPortCount)*bufferSize*sizeof(float);

        if (size == 0)
            size = sizeof(float);

        data = (float*)carla_shm_map(shm, size);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeAudioPool)
};

// -------------------------------------------------------------------------------------------------------------------

struct BridgeRtClientControl : public CarlaRingBufferControl<SmallStackBuffer> {
    CarlaString filename;
    BridgeRtClientData* data;
    shm_t shm;

    BridgeRtClientControl()
        : filename(),
          data(nullptr)
#ifdef CARLA_PROPER_CPP11_SUPPORT
        , shm(shm_t_INIT) {}
#else
    {
        shm = shm_t_INIT;
    }
#endif

    ~BridgeRtClientControl() noexcept override
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    void clear() noexcept
    {
        filename.clear();

        if (! carla_is_shm_valid(shm))
        {
            CARLA_SAFE_ASSERT(data == nullptr);
            return;
        }

        if (data != nullptr)
        {
            carla_shm_unmap(shm, data);
            data = nullptr;
        }

        carla_shm_close(shm);
        carla_shm_init(shm);
    }

    bool mapData() noexcept
    {
        CARLA_SAFE_ASSERT(data == nullptr);

        if (carla_shm_map<BridgeRtClientData>(shm, data))
        {
            carla_zeroStruct(data->sem);
            carla_zeroStruct(data->timeInfo);
            carla_zeroBytes(data->midiOut, kBridgeRtClientDataMidiOutSize);
            setRingBuffer(&data->ringBuffer, true);
            return true;
        }

        return false;
    }

    void unmapData() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        carla_shm_unmap(shm, data);
        data = nullptr;

        setRingBuffer(nullptr, false);
    }

    bool waitForClient(const uint secs) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

        jackbridge_sem_post(&data->sem.server);

        return jackbridge_sem_timedwait(&data->sem.client, secs);
    }

    void writeOpcode(const PluginBridgeRtClientOpcode opcode) noexcept
    {
        writeUInt(static_cast<uint32_t>(opcode));
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeRtClientControl)
};

// -------------------------------------------------------------------------------------------------------------------

struct BridgeNonRtClientControl : public CarlaRingBufferControl<BigStackBuffer> {
    CarlaMutex mutex;
    CarlaString filename;
    BridgeNonRtClientData* data;
    shm_t shm;

    BridgeNonRtClientControl() noexcept
        : mutex(),
          filename(),
          data(nullptr)
#ifdef CARLA_PROPER_CPP11_SUPPORT
        , shm(shm_t_INIT) {}
#else
    {
        shm = shm_t_INIT;
    }
#endif

    ~BridgeNonRtClientControl() noexcept override
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    void clear() noexcept
    {
        filename.clear();

        if (! carla_is_shm_valid(shm))
        {
            CARLA_SAFE_ASSERT(data == nullptr);
            return;
        }

        if (data != nullptr)
        {
            carla_shm_unmap(shm, data);
            data = nullptr;
        }

        carla_shm_close(shm);
        carla_shm_init(shm);
    }

    bool mapData() noexcept
    {
        CARLA_SAFE_ASSERT(data == nullptr);

        if (carla_shm_map<BridgeNonRtClientData>(shm, data))
        {
            setRingBuffer(&data->ringBuffer, true);
            return true;
        }

        return false;
    }

    void unmapData() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        carla_shm_unmap(shm, data);
        data = nullptr;

        setRingBuffer(nullptr, false);
    }

    void writeOpcode(const PluginBridgeNonRtClientOpcode opcode) noexcept
    {
        writeUInt(static_cast<uint32_t>(opcode));
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeNonRtClientControl)
};

// -------------------------------------------------------------------------------------------------------------------

struct BridgeNonRtServerControl : public CarlaRingBufferControl<HugeStackBuffer> {
    CarlaString filename;
    BridgeNonRtServerData* data;
    shm_t shm;

    BridgeNonRtServerControl() noexcept
        : filename(),
          data(nullptr)
#ifdef CARLA_PROPER_CPP11_SUPPORT
        , shm(shm_t_INIT) {}
#else
    {
        shm = shm_t_INIT;
    }
#endif

    ~BridgeNonRtServerControl() noexcept override
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    void clear() noexcept
    {
        filename.clear();

        if (! carla_is_shm_valid(shm))
        {
            CARLA_SAFE_ASSERT(data == nullptr);
            return;
        }

        if (data != nullptr)
        {
            carla_shm_unmap(shm, data);
            data = nullptr;
        }

        carla_shm_close(shm);
        carla_shm_init(shm);
    }

    bool mapData() noexcept
    {
        CARLA_SAFE_ASSERT(data == nullptr);

        if (carla_shm_map<BridgeNonRtServerData>(shm, data))
        {
            setRingBuffer(&data->ringBuffer, true);
            return true;
        }

        return false;
    }

    void unmapData() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        carla_shm_unmap(shm, data);
        data = nullptr;

        setRingBuffer(nullptr, false);
    }

    PluginBridgeNonRtServerOpcode readOpcode() noexcept
    {
        return static_cast<PluginBridgeNonRtServerOpcode>(readUInt());
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeNonRtServerControl)
};

// -------------------------------------------------------------------------------------------------------------------

struct BridgeParamInfo {
    float value;
    CarlaString name;
    CarlaString unit;

    BridgeParamInfo() noexcept
        : value(0.0f),
          name(),
          unit() {}

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeParamInfo)
};

// -------------------------------------------------------------------------------------------------------------------

class CarlaPluginBridgeThread : public CarlaThread
{
public:
    CarlaPluginBridgeThread(CarlaEngine* const engine, CarlaPlugin* const plugin) noexcept
        : CarlaThread("CarlaThreadDSSIUI"),
          kEngine(engine),
          kPlugin(plugin),
          fBinary(),
          fLabel(),
          fShmIds(),
          fPluginType(PLUGIN_NONE),
          fProcess(),
          leakDetector_CarlaPluginBridgeThread() {}

    void setData(const char* const binary, const char* const label, const char* const shmIds, const PluginType ptype) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(binary != nullptr && binary[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(label != nullptr  && label[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(shmIds != nullptr && shmIds[0] != '\0',);
        CARLA_SAFE_ASSERT(! isThreadRunning());

        fBinary = binary;
        fLabel  = binary;
        fShmIds = shmIds;
        fPluginType = ptype;
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
            carla_stderr("CarlaPluginBridgeThread::run() - already running, giving up...");
        }

        String name(kPlugin->getName());
        String filename(kPlugin->getFilename());

        if (name.isEmpty())
            name = "(none)";

        if (filename.isEmpty())
            filename = "\"\"";

        StringArray arguments;

#ifndef CARLA_OS_WIN
        // start with "wine" if needed
        if (fBinary.endsWith(".exe"))
            arguments.add("wine");
#endif

        // binary
        arguments.add(fBinary.buffer());

#if 0
        /* stype    */ arguments.add(fExtra1.buffer());
        /* filename */ arguments.add(filename);
        /* label    */ arguments.add(fLabel.buffer());
        /* uniqueId */ arguments.add(String(static_cast<juce::int64>(fPlugin->getUniqueId())));

        carla_setenv("ENGINE_BRIDGE_SHM_IDS", fShmIds.buffer());
        carla_setenv("WINEDEBUG", "-all");
#endif
    }

private:
    CarlaEngine* const kEngine;
    CarlaPlugin* const kPlugin;

    CarlaString fBinary;
    CarlaString fLabel;
    CarlaString fShmIds;
    PluginType  fPluginType;

    ScopedPointer<ChildProcess> fProcess;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginBridgeThread)
};

// -------------------------------------------------------------------------------------------------------------------

class CarlaPluginBridge : public CarlaPlugin
{
public:
    CarlaPluginBridge(CarlaEngine* const engine, const uint id, const BinaryType btype, const PluginType ptype)
        : CarlaPlugin(engine, id),
          fBinaryType(btype),
          fPluginType(ptype),
          fInitiated(false),
          fInitError(false),
          fSaved(false),
          fNeedsSemDestroy(false),
          fTimedOut(false),
          fLastPongCounter(-1),
          fBridgeBinary(),
          fBridgeThread(engine, this),
          fShmAudioPool(),
          fShmRtClientControl(),
          fShmNonRtClientControl(),
          fShmNonRtServerControl(),
          fInfo(),
          fParams(nullptr),
          leakDetector_CarlaPluginBridge()
    {
        carla_debug("CarlaPluginBridge::CarlaPluginBridge(%p, %i, %s, %s)", engine, id, BinaryType2Str(btype), PluginType2Str(ptype));

        pData->hints |= PLUGIN_IS_BRIDGE;
    }

    ~CarlaPluginBridge() override
    {
        carla_debug("CarlaPluginBridge::~CarlaPluginBridge()");

        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
            pData->transientTryCounter = 0;

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
                fShmRtClientControl.waitForClient(3);
        }

        fBridgeThread.stopThread(3000);

        if (fNeedsSemDestroy)
        {
            jackbridge_sem_destroy(&fShmRtClientControl.data->sem.server);
            jackbridge_sem_destroy(&fShmRtClientControl.data->sem.client);
        }

        fShmAudioPool.clear();
        fShmRtClientControl.clear();
        fShmNonRtClientControl.clear();
        fShmNonRtServerControl.clear();

        clearBuffers();

        fInfo.chunk.clear();
    }

    // -------------------------------------------------------------------
    // Information (base)

    BinaryType getBinaryType() const noexcept
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
        return fInfo.uniqueId;
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
        CARLA_SAFE_ASSERT_RETURN(fInfo.chunk.size() > 0, 0);

        *dataPtr = fInfo.chunk.data();
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
        CARLA_ASSERT(parameterId < pData->param.count);

        std::strncpy(strBuf, fParams[parameterId].name.buffer(), STR_MAX);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_ASSERT(parameterId < pData->param.count);

        std::strncpy(strBuf, fParams[parameterId].unit.buffer(), STR_MAX);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    void prepareForSave() override
    {
        fSaved = false;

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientPrepareForSave);
            fShmNonRtClientControl.commitWrite();
        }

        carla_stdout("CarlaPluginBridge::prepareForSave() - sent, now waiting...");

        for (int i=0; i < 200; ++i)
        {
            if (fSaved)
                break;
            carla_msleep(30);
            pData->engine->callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);
            pData->engine->idle();
        }

        if (! fSaved)
            carla_stderr("CarlaPluginBridge::prepareForSave() - Timeout while requesting save state");
        else
            carla_stdout("CarlaPluginBridge::prepareForSave() - success!");
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
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT

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

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fParams[parameterId].value = fixedValue;

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetParameterValue);
            fShmNonRtClientControl.writeUInt(parameterId);
            fShmNonRtClientControl.writeFloat(value);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setParameterMidiChannel(const uint32_t parameterId, const uint8_t channel, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);

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
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(cc >= -1 && cc < MAX_MIDI_CONTROL,);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetParameterMidiCC);
            fShmNonRtClientControl.writeUInt(parameterId);
            fShmNonRtClientControl.writeShort(cc);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setParameterMidiCC(parameterId, cc, sendOsc, sendCallback);
    }

    void setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->prog.count),);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetProgram);
            fShmNonRtClientControl.writeInt(index);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setProgram(index, sendGui, sendOsc, sendCallback);
    }

    void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetMidiProgram);
            fShmNonRtClientControl.writeInt(index);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

#if 0
    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_ASSERT(type);
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);

        if (sendGui)
        {
            // TODO - if type is chunk|binary, store it in a file and send path instead
            QString cData;
            cData  = type;
            cData += "·";
            cData += key;
            cData += "·";
            cData += value;
            osc_send_configure(&osc.data, CARLA_BRIDGE_MSG_SET_CUSTOM, cData.toUtf8().constData());
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }
#endif

    void setChunkData(const void* const data, const std::size_t dataSize) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(dataSize > 0,);

        CarlaString dataBase64 = CarlaString::asBase64(data, dataSize);
        CARLA_SAFE_ASSERT_RETURN(dataBase64.length() > 0,);

        String filePath(File::getSpecialLocation(File::tempDirectory).getFullPathName());

        filePath += CARLA_OS_SEP_STR;
        filePath += ".CarlaChunk_";
        filePath += fShmAudioPool.filename.buffer() + 18;

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

    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(yesNo ? kPluginBridgeNonRtClientShowUI : kPluginBridgeNonRtClientHideUI);
            fShmNonRtClientControl.commitWrite();
        }

        if (yesNo)
        {
            pData->tryTransient();
        }
        else
        {
            pData->transientTryCounter = 0;
        }
    }

    void idle() override
    {
        if (fBridgeThread.isThreadRunning())
        {
            if (fTimedOut && pData->active)
                setActive(false, true, true);

            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientPing);
            fShmNonRtClientControl.commitWrite();
        }
        else
            carla_stderr2("TESTING: Bridge has closed!");

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

            if (fInfo.aIns > 1)
            {
                portName += "input_";
                portName += CarlaString(j+1);
            }
            else
                portName += "input";
            portName.truncate(portNameSize);

            pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true);
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

            if (fInfo.aOuts > 1)
            {
                portName += "output_";
                portName += CarlaString(j+1);
            }
            else
                portName += "output";
            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
            pData->audioOut.ports[j].rindex = j;
        }

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

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true);
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

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        // extra plugin hints
        pData->extraHints = 0x0;

        if (fInfo.mIns > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (fInfo.mOuts > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_OUT;

        if (fInfo.aIns <= 2 && fInfo.aOuts <= 2 && (fInfo.aIns == fInfo.aOuts || fInfo.aIns == 0 || fInfo.aOuts == 0))
            pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        carla_debug("CarlaPluginBridge::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientActivate);
            fShmNonRtClientControl.commitWrite();
        }

        bool timedOut = true;

        try {
            timedOut = waitForClient(1);
        } catch(...) {}

        if (! timedOut)
            fTimedOut = false;
    }

    void deactivate() noexcept override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientDeactivate);
            fShmNonRtClientControl.commitWrite();
        }

        bool timedOut = true;

        try {
            timedOut = waitForClient(1);
        } catch(...) {}

        if (! timedOut)
            fTimedOut = false;
    }

    void process(const float** const audioIn, float** const audioOut, const float** const cvIn, float** const cvOut, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (fTimedOut || ! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(audioOut[i], static_cast<int>(frames));
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
                FloatVectorOperations::clear(cvOut[i], static_cast<int>(frames));
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
                for (RtLinkedList<ExternalMidiNote>::Itenerator it = pData->extNotes.data.begin(); it.valid(); it.next())
                {
                    const ExternalMidiNote& note(it.getValue());

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

            bool allNotesOffSent = false;

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
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                                break;
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                                break;
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

                                setBalanceLeft(left, false, false);
                                setBalanceRight(right, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                                break;
                            }
                        }

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
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }

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

        processSingle(audioIn, audioOut, cvIn, cvOut, frames);
    }

    bool processSingle(const float** const audioIn, float** const audioOut, const float** const cvIn, float** const cvOut, const uint32_t frames)
    {
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

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(audioOut[i], static_cast<int>(frames));
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
                FloatVectorOperations::clear(cvOut[i], static_cast<int>(frames));
            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        for (uint32_t i=0; i < fInfo.aIns; ++i)
            FloatVectorOperations::copy(fShmAudioPool.data + (i * frames), audioIn[i], static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------
        // TimeInfo

        const EngineTimeInfo& timeInfo(pData->engine->getTimeInfo());
        BridgeTimeInfo& bridgeTimeInfo(fShmRtClientControl.data->timeInfo);

        bridgeTimeInfo.playing = timeInfo.playing;
        bridgeTimeInfo.frame   = timeInfo.frame;
        bridgeTimeInfo.usecs   = timeInfo.usecs;
        bridgeTimeInfo.valid   = timeInfo.valid;

        if (timeInfo.valid & EngineTimeInfo::kValidBBT)
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
            fShmRtClientControl.commitWrite();
        }

        if (! waitForClient(2))
        {
            pData->singleMutex.unlock();
            return true;
        }

        for (uint32_t i=0; i < fInfo.aOuts; ++i)
            FloatVectorOperations::copy(audioOut[i], fShmAudioPool.data + ((i + fInfo.aIns) * frames), static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doVolume  = (pData->hints & PLUGIN_CAN_VOLUME) != 0 && ! carla_compareFloats(pData->postProc.volume, 1.0f);
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && ! carla_compareFloats(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_compareFloats(pData->postProc.balanceLeft, -1.0f) && carla_compareFloats(pData->postProc.balanceRight, 1.0f));

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (uint32_t k=0; k < frames; ++k)
                    {
                        bufValue        = audioIn[(pData->audioIn.count == 1) ? 0 : i][k];
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
                        FloatVectorOperations::copy(oldBufLeft, audioOut[i], static_cast<int>(frames));
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

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        resizeAudioAndCVPool(newBufferSize);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);
            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetBufferSize);
            fShmNonRtClientControl.writeUInt(newBufferSize);
            fShmNonRtClientControl.commitWrite();
        }

        fShmRtClientControl.waitForClient(1);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);
            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetSampleRate);
            fShmNonRtClientControl.writeDouble(newSampleRate);
            fShmNonRtClientControl.commitWrite();
        }

        fShmRtClientControl.waitForClient(1);
    }

    void offlineModeChanged(const bool isOffline) override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);
            fShmNonRtClientControl.writeOpcode(isOffline ? kPluginBridgeNonRtClientSetOffline : kPluginBridgeNonRtClientSetOnline);
            fShmNonRtClientControl.commitWrite();
        }

        fShmRtClientControl.waitForClient(1);
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
        CARLA_SAFE_ASSERT_RETURN(index < pData->midiprog.count,);

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

#if 0
    int setOscPluginBridgeInfo(const PluginBridgeOscInfoType infoType, const int argc, const lo_arg* const* const argv, const char* const types)
    {
#ifdef DEBUG
        if (infoType != kPluginBridgeOscPong) {
            carla_debug("CarlaPluginBridge::setOscPluginBridgeInfo(%s, %i, %p, \"%s\")", PluginBridgeOscInfoType2str(infoType), argc, argv, types);
        }
#endif

        switch (infoType)
        {
        case kPluginBridgeOscNull:
            break;

        case kPluginBridgeOscPong:
            if (fLastPongCounter > 0)
                fLastPongCounter = 0;
            break;

        case kPluginBridgeOscPluginInfo1: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(5, "iiiih");

            const int32_t category = argv[0]->i;
            const int32_t hints    = argv[1]->i;
            const int32_t optionAv = argv[2]->i;
            const int32_t optionEn = argv[3]->i;
            const int64_t uniqueId = argv[4]->h;

            CARLA_SAFE_ASSERT_BREAK(category >= 0);
            CARLA_SAFE_ASSERT_BREAK(hints >= 0);
            CARLA_SAFE_ASSERT_BREAK(optionAv >= 0);
            CARLA_SAFE_ASSERT_BREAK(optionEn >= 0);

            pData->hints  = static_cast<uint>(hints);
            pData->hints |= PLUGIN_IS_BRIDGE;

            pData->options = static_cast<uint>(optionEn);

            fInfo.category = static_cast<PluginCategory>(category);
            fInfo.uniqueId = uniqueId;
            fInfo.optionsAvailable = static_cast<uint>(optionAv);
            break;
        }

        case kPluginBridgeOscPluginInfo2: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(4, "ssss");

            const char* const realName  = (const char*)&argv[0]->s;
            const char* const label     = (const char*)&argv[1]->s;
            const char* const maker     = (const char*)&argv[2]->s;
            const char* const copyright = (const char*)&argv[3]->s;

            CARLA_SAFE_ASSERT_BREAK(realName != nullptr);
            CARLA_SAFE_ASSERT_BREAK(label != nullptr);
            CARLA_SAFE_ASSERT_BREAK(maker != nullptr);
            CARLA_SAFE_ASSERT_BREAK(copyright != nullptr);

            fInfo.name  = realName;
            fInfo.label = label;
            fInfo.maker = maker;
            fInfo.copyright = copyright;

            if (pData->name == nullptr)
                pData->name = pData->engine->getUniquePluginName(realName);
            break;
        }

        case kPluginBridgeOscAudioCount: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "ii");

            const int32_t ins  = argv[0]->i;
            const int32_t outs = argv[1]->i;

            CARLA_SAFE_ASSERT_BREAK(ins >= 0);
            CARLA_SAFE_ASSERT_BREAK(outs >= 0);

            fInfo.aIns  = static_cast<uint32_t>(ins);
            fInfo.aOuts = static_cast<uint32_t>(outs);
            break;
        }

        case kPluginBridgeOscMidiCount: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "ii");

            const int32_t ins  = argv[0]->i;
            const int32_t outs = argv[1]->i;

            CARLA_SAFE_ASSERT_BREAK(ins >= 0);
            CARLA_SAFE_ASSERT_BREAK(outs >= 0);

            fInfo.mIns  = static_cast<uint32_t>(ins);
            fInfo.mOuts = static_cast<uint32_t>(outs);
            break;
        }

        case kPluginBridgeOscParameterCount: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "ii");

            const int32_t ins  = argv[0]->i;
            const int32_t outs = argv[1]->i;

            CARLA_SAFE_ASSERT_BREAK(ins >= 0);
            CARLA_SAFE_ASSERT_BREAK(outs >= 0);

            // delete old data
            pData->param.clear();

            if (fParams != nullptr)
            {
                delete[] fParams;
                fParams = nullptr;
            }

            if (int32_t count = ins+outs)
            {
                const int32_t maxParams(static_cast<int32_t>(pData->engine->getOptions().maxParameters));

                if (count > maxParams)
                {
                    // this is expected right now, to be handled better later
                    //carla_safe_assert_int2("count <= pData->engine->getOptions().maxParameters", __FILE__, __LINE__, count, maxParams);
                    count = maxParams;
                }

                const uint32_t ucount(static_cast<uint32_t>(count));

                pData->param.createNew(ucount, false);
                fParams = new BridgeParamInfo[ucount];
            }
            break;
        }

        case kPluginBridgeOscProgramCount: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t count = argv[0]->i;

            CARLA_SAFE_ASSERT_BREAK(count >= 0);

            pData->prog.clear();

            if (count > 0)
                pData->prog.createNew(static_cast<uint32_t>(count));

            break;
        }

        case kPluginBridgeOscMidiProgramCount: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t count = argv[0]->i;

            CARLA_SAFE_ASSERT_BREAK(count >= 0);

            pData->midiprog.clear();

            if (count > 0)
                pData->midiprog.createNew(static_cast<uint32_t>(count));
            break;
        }

        case kPluginBridgeOscParameterData1: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(5, "iiiii");

            const int32_t index    = argv[0]->i;
            const int32_t rindex   = argv[1]->i;
            const int32_t type     = argv[2]->i;
            const int32_t hints    = argv[3]->i;
            const int32_t midiCC   = argv[4]->i;

            CARLA_SAFE_ASSERT_BREAK(index >= 0);
            CARLA_SAFE_ASSERT_BREAK(rindex >= 0);
            CARLA_SAFE_ASSERT_BREAK(type >= 0);
            CARLA_SAFE_ASSERT_BREAK(hints >= 0);
            CARLA_SAFE_ASSERT_BREAK(midiCC >= -1 && midiCC < MAX_MIDI_CONTROL);
            CARLA_SAFE_ASSERT_INT2(index < static_cast<int32_t>(pData->param.count), index, pData->param.count);

            if (index < static_cast<int32_t>(pData->param.count))
            {
                pData->param.data[index].type   = static_cast<ParameterType>(type);
                pData->param.data[index].index  = index;
                pData->param.data[index].rindex = rindex;
                pData->param.data[index].hints  = static_cast<uint>(hints);
                pData->param.data[index].midiCC = static_cast<int16_t>(midiCC);
            }
            break;
        }

        case kPluginBridgeOscParameterData2: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iss");

            const int32_t index    = argv[0]->i;
            const char* const name = (const char*)&argv[1]->s;
            const char* const unit = (const char*)&argv[2]->s;

            CARLA_SAFE_ASSERT_BREAK(index >= 0);
            CARLA_SAFE_ASSERT_BREAK(name != nullptr);
            CARLA_SAFE_ASSERT_BREAK(unit != nullptr);
            CARLA_SAFE_ASSERT_INT2(index < static_cast<int32_t>(pData->param.count), index, pData->param.count);

            if (index < static_cast<int32_t>(pData->param.count))
            {
                fParams[index].name = name;
                fParams[index].unit = unit;
            }
            break;
        }

        case kPluginBridgeOscParameterRanges1: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(4, "ifff");

            const int32_t index = argv[0]->i;
            const float def     = argv[1]->f;
            const float min     = argv[2]->f;
            const float max     = argv[3]->f;

            CARLA_SAFE_ASSERT_BREAK(index >= 0);
            CARLA_SAFE_ASSERT_BREAK(min < max);
            CARLA_SAFE_ASSERT_BREAK(def >= min);
            CARLA_SAFE_ASSERT_BREAK(def <= max);
            CARLA_SAFE_ASSERT_INT2(index < static_cast<int32_t>(pData->param.count), index, pData->param.count);

            if (index < static_cast<int32_t>(pData->param.count))
            {
                pData->param.ranges[index].def = def;
                pData->param.ranges[index].min = min;
                pData->param.ranges[index].max = max;
            }
            break;
        }

        case kPluginBridgeOscParameterRanges2: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(4, "ifff");

            const int32_t index   = argv[0]->i;
            const float step      = argv[1]->f;
            const float stepSmall = argv[2]->f;
            const float stepLarge = argv[3]->f;

            CARLA_SAFE_ASSERT_BREAK(index >= 0);
            CARLA_SAFE_ASSERT_INT2(index < static_cast<int32_t>(pData->param.count), index, pData->param.count);

            if (index < static_cast<int32_t>(pData->param.count))
            {
                pData->param.ranges[index].step      = step;
                pData->param.ranges[index].stepSmall = stepSmall;
                pData->param.ranges[index].stepLarge = stepLarge;
            }
            break;
        }

        case kPluginBridgeOscParameterValue: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "if");

            const int32_t index = argv[0]->i;
            const float   value = argv[1]->f;

            CARLA_SAFE_ASSERT_BREAK(index >= 0);
            CARLA_SAFE_ASSERT_INT2(index < static_cast<int32_t>(pData->param.count), index, pData->param.count);

            if (index < static_cast<int32_t>(pData->param.count))
            {
                const uint32_t uindex(static_cast<uint32_t>(index));
                const float fixedValue(pData->param.getFixedValue(uindex, value));
                fParams[uindex].value = fixedValue;

                CarlaPlugin::setParameterValue(uindex, fixedValue, false, true, true);
            }
            break;
        }

        case kPluginBridgeOscDefaultValue: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "if");

            const int32_t index = argv[0]->i;
            const float   value = argv[1]->f;

            CARLA_SAFE_ASSERT_BREAK(index >= 0);
            CARLA_SAFE_ASSERT_INT2(index < static_cast<int32_t>(pData->param.count), index, pData->param.count);

            if (index < static_cast<int32_t>(pData->param.count))
                pData->param.ranges[index].def = value;
            break;
        }

        case kPluginBridgeOscCurrentProgram: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t index = argv[0]->i;

            CARLA_SAFE_ASSERT_BREAK(index >= -1);
            CARLA_SAFE_ASSERT_INT2(index < static_cast<int32_t>(pData->prog.count), index, pData->prog.count);

            CarlaPlugin::setProgram(index, false, true, true);
            break;
        }

        case kPluginBridgeOscCurrentMidiProgram: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t index = argv[0]->i;

            CARLA_SAFE_ASSERT_BREAK(index >= -1);
            CARLA_SAFE_ASSERT_INT2(index < static_cast<int32_t>(pData->midiprog.count), index, pData->midiprog.count);

            CarlaPlugin::setMidiProgram(index, false, true, true);
            break;
        }

        case kPluginBridgeOscProgramName: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "is");

            const int32_t index    = argv[0]->i;
            const char* const name = (const char*)&argv[1]->s;

            CARLA_SAFE_ASSERT_BREAK(index >= 0);
            CARLA_SAFE_ASSERT_BREAK(name != nullptr);
            CARLA_SAFE_ASSERT_INT2(index < static_cast<int32_t>(pData->prog.count), index, pData->prog.count);

            if (index < static_cast<int32_t>(pData->prog.count))
            {
                if (pData->prog.names[index] != nullptr)
                    delete[] pData->prog.names[index];
                pData->prog.names[index] = carla_strdup(name);
            }
            break;
        }

        case kPluginBridgeOscMidiProgramData: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(4, "iiis");

            const int32_t index    = argv[0]->i;
            const int32_t bank     = argv[1]->i;
            const int32_t program  = argv[2]->i;
            const char* const name = (const char*)&argv[3]->s;

            CARLA_SAFE_ASSERT_BREAK(index >= 0);
            CARLA_SAFE_ASSERT_BREAK(bank >= 0);
            CARLA_SAFE_ASSERT_BREAK(program >= 0);
            CARLA_SAFE_ASSERT_BREAK(name != nullptr);
            CARLA_SAFE_ASSERT_INT2(index < static_cast<int32_t>(pData->midiprog.count), index, pData->midiprog.count);

            if (index < static_cast<int32_t>(pData->midiprog.count))
            {
                if (pData->midiprog.data[index].name != nullptr)
                    delete[] pData->midiprog.data[index].name;
                pData->midiprog.data[index].bank    = static_cast<uint32_t>(bank);
                pData->midiprog.data[index].program = static_cast<uint32_t>(program);
                pData->midiprog.data[index].name    = carla_strdup(name);
            }
            break;
        }

        case kPluginBridgeOscConfigure: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "ss");

            const char* const key   = (const char*)&argv[0]->s;
            const char* const value = (const char*)&argv[1]->s;

            CARLA_SAFE_ASSERT_BREAK(key != nullptr);
            CARLA_SAFE_ASSERT_BREAK(value != nullptr);

            if (std::strcmp(key, CARLA_BRIDGE_MSG_HIDE_GUI) == 0)
                pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
            else if (std::strcmp(key, CARLA_BRIDGE_MSG_SAVED) == 0)
                fSaved = true;
            break;
        }

        case kPluginBridgeOscSetCustomData: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "sss");

            const char* const type  = (const char*)&argv[0]->s;
            const char* const key   = (const char*)&argv[1]->s;
            const char* const value = (const char*)&argv[2]->s;

            CARLA_SAFE_ASSERT_BREAK(type != nullptr);
            CARLA_SAFE_ASSERT_BREAK(key != nullptr);
            CARLA_SAFE_ASSERT_BREAK(value != nullptr);

            CarlaPlugin::setCustomData(type, key, value, false);
            break;
        }

        case kPluginBridgeOscSetChunkDataFile: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "s");

            const char* const chunkFilePath = (const char*)&argv[0]->s;

            CARLA_SAFE_ASSERT_BREAK(chunkFilePath != nullptr);

            String realChunkFilePath(chunkFilePath);
            carla_stdout("chunk save path BEFORE => %s", realChunkFilePath.toRawUTF8());

#ifndef CARLA_OS_WIN
            // Using Wine, fix temp dir
            if (fBinaryType == BINARY_WIN32 || fBinaryType == BINARY_WIN64)
            {
                // Get WINEPREFIX
                String wineDir;
                if (const char* const WINEPREFIX = getenv("WINEPREFIX"))
                    wineDir = String(WINEPREFIX);
                else
                    wineDir = File::getSpecialLocation(File::userHomeDirectory).getFullPathName() + "/.wine";

                const StringArray driveLetterSplit(StringArray::fromTokens(realChunkFilePath, ":/", ""));

                realChunkFilePath  = wineDir;
                realChunkFilePath += "/drive_";
                realChunkFilePath += driveLetterSplit[0].toLowerCase();
                realChunkFilePath += "/";
                realChunkFilePath += driveLetterSplit[1];

                realChunkFilePath  = realChunkFilePath.replace("\\", "/");
                carla_stdout("chunk save path AFTER => %s", realChunkFilePath.toRawUTF8());
            }
#endif

            File chunkFile(realChunkFilePath);

            if (chunkFile.existsAsFile())
            {
                fInfo.chunk = carla_getChunkFromBase64String(chunkFile.loadFileAsString().toRawUTF8());
                chunkFile.deleteFile();
                carla_stderr("chunk data final");
            }
            break;
        }

        case kPluginBridgeOscLatency:
            // TODO
            break;

        case kPluginBridgeOscReady:
            fInitiated = true;
            break;

        case kPluginBridgeOscError: {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "s");

            const char* const error = (const char*)&argv[0]->s;

            CARLA_ASSERT(error != nullptr);

            pData->engine->setLastError(error);

            fInitError = true;
            fInitiated = true;
            break;
        }
        }

        return 0;
    }
#endif

    // -------------------------------------------------------------------

    const void* getExtraStuff() const noexcept override
    {
        return fBridgeBinary.isNotEmpty() ? fBridgeBinary.buffer() : nullptr;
    }

    bool init(const char* const filename, const char* const name, const char* const label, const char* const bridgeBinary)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        // ---------------------------------------------------------------
        // set info

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);

        pData->filename = carla_strdup(filename);

        if (bridgeBinary != nullptr)
            fBridgeBinary = bridgeBinary;

        std::srand(static_cast<uint>(std::time(nullptr)));

        // ---------------------------------------------------------------
        // SHM Audio Pool

        {
            char tmpFileBase[64];

            std::sprintf(tmpFileBase, "/carla-bridge_shm_ap_XXXXXX");

            fShmAudioPool.shm = carla_shm_create_temp(tmpFileBase);

            if (! carla_is_shm_valid(fShmAudioPool.shm))
            {
                carla_stdout("Failed to open or create shared memory file #1");
                return false;
            }

            fShmAudioPool.filename = tmpFileBase;
        }

        // ---------------------------------------------------------------
        // SHM RT Control

        {
            char tmpFileBase[64];

            std::sprintf(tmpFileBase, "/carla-bridge_shm_rtC_XXXXXX");

            fShmRtClientControl.shm = carla_shm_create_temp(tmpFileBase);

            if (! carla_is_shm_valid(fShmRtClientControl.shm))
            {
                carla_stdout("Failed to open or create shared memory file #2");
                // clear
                carla_shm_close(fShmAudioPool.shm);
                return false;
            }

            fShmRtClientControl.filename = tmpFileBase;

            if (! fShmRtClientControl.mapData())
            {
                carla_stdout("Failed to map shared memory file #2");
                // clear
                carla_shm_close(fShmRtClientControl.shm);
                carla_shm_close(fShmAudioPool.shm);
                return false;
            }

            CARLA_SAFE_ASSERT(fShmRtClientControl.data != nullptr);

            if (! jackbridge_sem_init(&fShmRtClientControl.data->sem.server))
            {
                carla_stdout("Failed to initialize shared memory semaphore #1");
                // clear
                fShmRtClientControl.unmapData();
                carla_shm_close(fShmRtClientControl.shm);
                carla_shm_close(fShmAudioPool.shm);
                return false;
            }

            if (! jackbridge_sem_init(&fShmRtClientControl.data->sem.client))
            {
                carla_stdout("Failed to initialize shared memory semaphore #2");
                // clear
                jackbridge_sem_destroy(&fShmRtClientControl.data->sem.server);
                fShmRtClientControl.unmapData();
                carla_shm_close(fShmRtClientControl.shm);
                carla_shm_close(fShmAudioPool.shm);
                return false;
            }

            fNeedsSemDestroy = true;
        }

        // ---------------------------------------------------------------
        // SHM Non-RT Control
        {
            char tmpFileBase[64];

            std::sprintf(tmpFileBase, "/carla-bridge_shm_nonrtC_XXXXXX");

            fShmNonRtClientControl.shm = carla_shm_create_temp(tmpFileBase);

            if (! carla_is_shm_valid(fShmNonRtClientControl.shm))
            {
                carla_stdout("Failed to open or create shared memory file #3");
                return false;
            }

            fShmNonRtClientControl.filename = tmpFileBase;

            if (! fShmNonRtClientControl.mapData())
            {
                carla_stdout("Failed to map shared memory file #3");
                // clear
                fShmNonRtClientControl.unmapData();
                carla_shm_close(fShmNonRtClientControl.shm);
                carla_shm_close(fShmRtClientControl.shm);
                carla_shm_close(fShmAudioPool.shm);
                return false;
            }
        }

        carla_stdout("Carla Server Info:");
        carla_stdout("  sizeof(BridgeRtClientData):    " P_SIZE, sizeof(BridgeRtClientData));
        carla_stdout("  sizeof(BridgeNonRtClientData): " P_SIZE, sizeof(BridgeNonRtClientData));
        carla_stdout("  sizeof(BridgeNonRtServerData): " P_SIZE, sizeof(BridgeNonRtServerData));

        // initial values
        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientNull);
        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeRtClientData)));
        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeNonRtClientData)));
        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeNonRtServerData)));

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetBufferSize);
        fShmNonRtClientControl.writeUInt(pData->engine->getBufferSize());

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetSampleRate);
        fShmNonRtClientControl.writeDouble(pData->engine->getSampleRate());

        fShmNonRtClientControl.commitWrite();

        // register plugin now so we can receive OSC (and wait for it)
        pData->hints |= PLUGIN_IS_BRIDGE;
        pData->engine->registerEnginePlugin(pData->id, this);

        // init OSC
        {
            char shmIdsStr[6*4+1];
            carla_zeroChar(shmIdsStr, 6*4+1);

            std::strncpy(shmIdsStr, &fShmAudioPool.filename[fShmAudioPool.filename.length()-6], 6);
            std::strncat(shmIdsStr, &fShmRtClientControl.filename[fShmRtClientControl.filename.length()-6], 6);
            std::strncat(shmIdsStr, &fShmNonRtClientControl.filename[fShmNonRtClientControl.filename.length()-6], 6);
            std::strncat(shmIdsStr, &fShmNonRtServerControl.filename[fShmNonRtServerControl.filename.length()-6], 6);

            fBridgeThread.setData(bridgeBinary, label, shmIdsStr, fPluginType);
            fBridgeThread.startThread();
        }

        fInitiated = false;
        fLastPongCounter = 0;

        for (; fLastPongCounter++ < 500;)
        {
            if (fInitiated || ! fBridgeThread.isThreadRunning())
                break;
            carla_msleep(25);
            pData->engine->callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);
            pData->engine->idle();
        }

        fLastPongCounter = -1;

        if (fInitError || ! fInitiated)
        {
            fBridgeThread.stopThread(6000);

            if (! fInitError)
                pData->engine->setLastError("Timeout while waiting for a response from plugin-bridge\n(or the plugin crashed on initialization?)");

            return false;
        }

        // ---------------------------------------------------------------
        // register client

        if (pData->name == nullptr)
        {
            if (name != nullptr && name[0] != '\0')
                pData->name = pData->engine->getUniquePluginName(name);
            else if (label != nullptr && label[0] != '\0')
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
    bool fNeedsSemDestroy;
    bool fTimedOut;

    volatile int32_t fLastPongCounter;

    CarlaString             fBridgeBinary;
    CarlaPluginBridgeThread fBridgeThread;

    BridgeAudioPool          fShmAudioPool;
    BridgeRtClientControl    fShmRtClientControl;
    BridgeNonRtClientControl fShmNonRtClientControl;
    BridgeNonRtServerControl fShmNonRtServerControl;

    struct Info {
        uint32_t aIns, aOuts;
        uint32_t cvIns, cvOuts;
        uint32_t mIns, mOuts;
        PluginCategory category;
        uint optionsAvailable;
        int64_t uniqueId;
        CarlaString name;
        CarlaString label;
        CarlaString maker;
        CarlaString copyright;
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
              uniqueId(0),
              name(),
              label(),
              maker(),
              copyright(),
              chunk() {}
    } fInfo;

    BridgeParamInfo* fParams;

    void resizeAudioAndCVPool(const uint32_t bufferSize)
    {
        fShmAudioPool.resize(bufferSize, fInfo.aIns+fInfo.aOuts, fInfo.cvIns+fInfo.cvOuts);

        fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetAudioPool);
        fShmRtClientControl.writeULong(static_cast<uint64_t>(fShmAudioPool.size));

        fShmRtClientControl.commitWrite();

        waitForClient();
    }

    bool waitForClient(const uint secs = 5)
    {
        CARLA_SAFE_ASSERT_RETURN(! fTimedOut, false);

        if (! fShmRtClientControl.waitForClient(secs))
        {
            carla_stderr("waitForClient() timeout here");
            fTimedOut = true;
            return false;
        }

        return true;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginBridge)
};

CARLA_BACKEND_END_NAMESPACE

#endif // ! BUILD_BRIDGE

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newBridge(const Initializer& init, BinaryType btype, PluginType ptype, const char* const bridgeBinary)
{
    carla_debug("CarlaPlugin::newBridge({%p, \"%s\", \"%s\", \"%s\"}, %s, %s, \"%s\")", init.engine, init.filename, init.name, init.label, BinaryType2Str(btype), PluginType2Str(ptype), bridgeBinary);

#ifndef BUILD_BRIDGE
    if (bridgeBinary == nullptr || bridgeBinary[0] == '\0')
    {
        init.engine->setLastError("Bridge not possible, bridge-binary not found");
        return nullptr;
    }

    CarlaPluginBridge* const plugin(new CarlaPluginBridge(init.engine, init.id, btype, ptype));

# if 0
    if (! plugin->init(init.filename, init.name, init.label, bridgeBinary))
    {
        init.engine->registerEnginePlugin(init.id, nullptr);
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    bool canRun = true;
# else
    bool canRun = false;
# endif

    if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (! plugin->canRunInRack())
        {
            init.engine->setLastError("Carla's rack mode can only work with Stereo Bridged plugins, sorry!");
            canRun = false;
        }
        else if (plugin->getCVInCount() > 0 || plugin->getCVInCount() > 0)
        {
            init.engine->setLastError("Carla's rack mode cannot work with plugins that have CV ports, sorry!");
            canRun = false;
        }
    }
    else if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_PATCHBAY && (plugin->getCVInCount() > 0 || plugin->getCVInCount() > 0))
    {
        init.engine->setLastError("CV ports in patchbay mode is still TODO");
        canRun = false;
    }

    if (! canRun)
    {
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("Plugin bridge support not available");
    return nullptr;

    // unused
    (void)bridgeBinary; (void)btype; (void)ptype;
#endif
}

CARLA_BACKEND_END_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
