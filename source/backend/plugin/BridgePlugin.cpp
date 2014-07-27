/*
 * Carla Bridge Plugin
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

#include "jackbridge/JackBridge.hpp"

#include <cerrno>
#include <cmath>
#include <ctime>

#define CARLA_BRIDGE_CHECK_OSC_TYPES(/* argc, types, */ argcToCompare, typesToCompare)                                       \
    /* check argument count */                                                                                               \
    if (argc != argcToCompare)                                                                                               \
    {                                                                                                                        \
        carla_stderr("BridgePlugin::%s() - argument count mismatch: %i != %i", __FUNCTION__, argc, argcToCompare);           \
        return 1;                                                                                                            \
    }                                                                                                                        \
    if (argc > 0)                                                                                                            \
    {                                                                                                                        \
        /* check for nullness */                                                                                             \
        if (! (types && typesToCompare))                                                                                     \
        {                                                                                                                    \
            carla_stderr("BridgePlugin::%s() - argument types are null", __FUNCTION__);                                      \
            return 1;                                                                                                        \
        }                                                                                                                    \
        /* check argument types */                                                                                           \
        if (std::strcmp(types, typesToCompare) != 0)                                                                         \
        {                                                                                                                    \
            carla_stderr("BridgePlugin::%s() - argument types mismatch: '%s' != '%s'", __FUNCTION__, types, typesToCompare); \
            return 1;                                                                                                        \
        }                                                                                                                    \
    }

// -------------------------------------------------------------------------------------------------------------------

using juce::File;
using juce::MemoryBlock;
using juce::String;
using juce::StringArray;

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// call carla_shm_create with for a XXXXXX temp filename

static shm_t shm_mkstemp(char* const fileBase)
{
    CARLA_SAFE_ASSERT_RETURN(fileBase != nullptr, gNullCarlaShm);

    const size_t fileBaseLen(std::strlen(fileBase));

    CARLA_SAFE_ASSERT_RETURN(fileBaseLen > 6, gNullCarlaShm);
    CARLA_SAFE_ASSERT_RETURN(std::strcmp(fileBase + fileBaseLen - 6, "XXXXXX") == 0, gNullCarlaShm);

    static const char charSet[] = "abcdefghijklmnopqrstuvwxyz"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "0123456789";
    static const int charSetLen = static_cast<int>(std::strlen(charSet) - 1); // -1 to avoid trailing '\0'

    // try until getting a valid shm or an error occurs
    for (;;)
    {
        for (size_t c = fileBaseLen - 6; c < fileBaseLen; ++c)
            fileBase[c] = charSet[std::rand() % charSetLen];

        const shm_t shm = carla_shm_create(fileBase);

        if (carla_is_shm_valid(shm))
            return shm;
        if (errno != EEXIST)
            return gNullCarlaShm;
    }
}

// -------------------------------------------------------------------------------------------------------------------

struct BridgeAudioPool {
    CarlaString filename;
    float* data;
    size_t size;
    shm_t shm;

    BridgeAudioPool()
        : data(nullptr),
          size(0)
    {
        carla_shm_init(shm);
    }

    ~BridgeAudioPool()
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    void clear()
    {
        filename.clear();

        if (! carla_is_shm_valid(shm))
            return;

        if (data != nullptr)
        {
            carla_shm_unmap(shm, data, size);
            data = nullptr;
        }

        size = 0;
        carla_shm_close(shm);
    }

    void resize(const uint32_t bufferSize, const uint32_t portCount)
    {
        if (data != nullptr)
            carla_shm_unmap(shm, data, size);

        size = portCount*bufferSize*sizeof(float);

        if (size == 0)
            size = sizeof(float);

        data = (float*)carla_shm_map(shm, size);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeAudioPool)
};

// -------------------------------------------------------------------------------------------------------------------

struct BridgeRtControl : public CarlaRingBuffer<StackBuffer> {
    CarlaString filename;
    BridgeRtData* data;
    shm_t shm;

    BridgeRtControl()
        : CarlaRingBuffer<StackBuffer>(),
          data(nullptr)
    {
        carla_shm_init(shm);
    }

    ~BridgeRtControl()
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    void clear()
    {
        filename.clear();

        if (! carla_is_shm_valid(shm))
            return;

        if (data != nullptr)
        {
            carla_shm_unmap(shm, data, sizeof(BridgeRtData));
            data = nullptr;
        }

        carla_shm_close(shm);
    }

    bool mapData()
    {
        CARLA_ASSERT(data == nullptr);

        if (carla_shm_map<BridgeRtData>(shm, data))
        {
            setRingBuffer(&data->ringBuffer, true);
            return true;
        }

        return false;
    }

    void unmapData()
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        carla_shm_unmap(shm, data, sizeof(BridgeRtData));
        data = nullptr;

        setRingBuffer(nullptr, false);
    }

    bool waitForServer(const int secs)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

        jackbridge_sem_post(&data->sem.server);

        return jackbridge_sem_timedwait(&data->sem.client, secs);
    }

    void writeOpcode(const PluginBridgeRtOpcode opcode) noexcept
    {
        writeInt(static_cast<int32_t>(opcode));
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeRtControl)
};

// -------------------------------------------------------------------------------------------------------------------

struct BridgeNonRtControl : public CarlaRingBuffer<BigStackBuffer> {
    CarlaMutex mutex;
    CarlaString filename;
    BridgeNonRtData* data;
    shm_t shm;

    BridgeNonRtControl()
        : CarlaRingBuffer<BigStackBuffer>(),
          data(nullptr)
    {
        carla_shm_init(shm);
    }

    ~BridgeNonRtControl()
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    void clear()
    {
        filename.clear();

        if (! carla_is_shm_valid(shm))
            return;

        if (data != nullptr)
        {
            carla_shm_unmap(shm, data, sizeof(BridgeNonRtData));
            data = nullptr;
        }

        carla_shm_close(shm);
    }

    bool mapData()
    {
        CARLA_SAFE_ASSERT(data == nullptr);

        if (carla_shm_map<BridgeNonRtData>(shm, data))
        {
            setRingBuffer(&data->ringBuffer, true);
            return true;
        }

        return false;
    }

    void unmapData()
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        carla_shm_unmap(shm, data, sizeof(BridgeNonRtData));
        data = nullptr;

        setRingBuffer(nullptr, false);
    }

    void writeOpcode(const PluginBridgeNonRtOpcode opcode) noexcept
    {
        writeInt(static_cast<int32_t>(opcode));
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeNonRtControl)
};

// -------------------------------------------------------------------------------------------------------------------

struct BridgeParamInfo {
    float value;
    CarlaString name;
    CarlaString unit;

    BridgeParamInfo() noexcept
        : value(0.0f) {}

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeParamInfo)
};

// -------------------------------------------------------------------------------------------------------------------

class BridgePlugin : public CarlaPlugin
{
public:
    BridgePlugin(CarlaEngine* const engine, const uint id, const BinaryType btype, const PluginType ptype)
        : CarlaPlugin(engine, id),
          fBinaryType(btype),
          fPluginType(ptype),
          fInitiated(false),
          fInitError(false),
          fSaved(false),
          fNeedsSemDestroy(false),
          fTimedOut(false),
          fLastPongCounter(-1),
          fParams(nullptr)
    {
        carla_debug("BridgePlugin::BridgePlugin(%p, %i, %s, %s)", engine, id, BinaryType2Str(btype), PluginType2Str(ptype));

        pData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_BRIDGE);

        pData->hints |= PLUGIN_IS_BRIDGE;
    }

    ~BridgePlugin() override
    {
        carla_debug("BridgePlugin::~BridgePlugin()");

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

        if (pData->osc.thread.isThreadRunning())
        {
            fShmNonRtControl.writeOpcode(kPluginBridgeNonRtQuit);
            fShmNonRtControl.commitWrite();

            if (! fTimedOut)
                fShmRtControl.waitForServer(3);
        }

        pData->osc.data.clear();
        pData->osc.thread.stopThread(3000);

        if (fNeedsSemDestroy)
        {
            jackbridge_sem_destroy(&fShmRtControl.data->sem.server);
            jackbridge_sem_destroy(&fShmRtControl.data->sem.client);
        }

        fShmAudioPool.clear();
        fShmRtControl.clear();
        fShmNonRtControl.clear();

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
            const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

            fShmNonRtControl.writeOpcode(kPluginBridgeNonRtPrepareForSave);
            fShmNonRtControl.commitWrite();
        }

        carla_stdout("BridgePlugin::prepareForSave() - sent, now waiting...");

        for (int i=0; i < 200; ++i)
        {
            if (fSaved)
                break;
            carla_msleep(30);
            pData->engine->callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);
            pData->engine->idle();
        }

        if (! fSaved)
            carla_stderr("BridgePlugin::prepareForSave() - Timeout while requesting save state");
        else
            carla_stdout("BridgePlugin::prepareForSave() - success!");
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setOption(const uint option, const bool yesNo, const bool sendCallback) override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

            fShmNonRtControl.writeOpcode(kPluginBridgeNonRtSetOption);
            fShmNonRtControl.writeInt(static_cast<int32_t>(option));
            fShmNonRtControl.writeBool(yesNo);
            fShmNonRtControl.commitWrite();
        }

        CarlaPlugin::setOption(option, yesNo, sendCallback);
    }

    void setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback) noexcept override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

            fShmNonRtControl.writeOpcode(kPluginBridgeNonRtSetCtrlChannel);
            fShmNonRtControl.writeShort(channel);
            fShmNonRtControl.commitWrite();
        }

        CarlaPlugin::setCtrlChannel(channel, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_ASSERT(parameterId < pData->param.count);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fParams[parameterId].value = fixedValue;

        {
            const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

            fShmNonRtControl.writeOpcode(kPluginBridgeNonRtSetParameterValue);
            fShmNonRtControl.writeInt(static_cast<int32_t>(parameterId));
            fShmNonRtControl.writeFloat(value);
            fShmNonRtControl.commitWrite();
        }

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->prog.count),);

        {
            const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

            fShmNonRtControl.writeOpcode(kPluginBridgeNonRtSetProgram);
            fShmNonRtControl.writeInt(index);
            fShmNonRtControl.commitWrite();
        }

        CarlaPlugin::setProgram(index, sendGui, sendOsc, sendCallback);
    }

    void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);

        {
            const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

            fShmNonRtControl.writeOpcode(kPluginBridgeNonRtSetMidiProgram);
            fShmNonRtControl.writeInt(index);
            fShmNonRtControl.commitWrite();
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

    void setChunkData(const char* const stringData) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(stringData != nullptr,);

        String filePath(File::getSpecialLocation(File::tempDirectory).getFullPathName());

        filePath += OS_SEP_STR;
        filePath += ".CarlaChunk_";
        filePath += fShmAudioPool.filename.buffer() + 18;

        if (File(filePath).replaceWithText(stringData))
        {
            const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

            fShmNonRtControl.writeOpcode(kPluginBridgeNonRtSetChunkDataFile);
            fShmNonRtControl.writeInt(filePath.length());
            fShmNonRtControl.writeCustomData(filePath.toRawUTF8(), static_cast<uint32_t>(filePath.length()));
            fShmNonRtControl.commitWrite();
        }
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

            fShmNonRtControl.writeOpcode(yesNo ? kPluginBridgeNonRtShowUI : kPluginBridgeNonRtHideUI);
            fShmNonRtControl.commitWrite();
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
        if (pData->osc.thread.isThreadRunning())
        {
            const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

            fShmNonRtControl.writeOpcode(kPluginBridgeNonRtPing);
            fShmNonRtControl.commitWrite();
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
        carla_debug("BridgePlugin::reload() - start");

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

        carla_debug("BridgePlugin::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

            fShmNonRtControl.writeOpcode(kPluginBridgeNonRtActivate);
            fShmNonRtControl.commitWrite();
        }

        bool timedOut = true;

        try {
            timedOut = waitForServer();
        } catch(...) {}

        if (! timedOut)
            fTimedOut = false;
    }

    void deactivate() noexcept override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

            fShmNonRtControl.writeOpcode(kPluginBridgeNonRtDeactivate);
            fShmNonRtControl.commitWrite();
        }

        bool timedOut = true;

        try {
            timedOut = waitForServer();
        } catch(...) {}

        if (! timedOut)
            fTimedOut = false;
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (fTimedOut || ! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(outBuffer[i], static_cast<int>(frames));
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
                    data1 = static_cast<uint8_t>((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT));
                    data2 = note.note;
                    data3 = note.velo;

                    fShmRtControl.writeOpcode(kPluginBridgeRtMidiData);
                    fShmRtControl.writeInt(0); // time
                    fShmRtControl.writeInt(3); // size
                    fShmRtControl.writeByte(data1);
                    fShmRtControl.writeByte(data2);
                    fShmRtControl.writeByte(data3);
                    fShmRtControl.commitWrite();
                }

                pData->extNotes.data.clear();
                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;

            for (uint32_t i=0, numEvents = pData->event.portIn->getEventCount(); i < numEvents; ++i)
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
                    {
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

                        // Control plugin parameters
                        uint32_t k;
                        for (k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            float value;

                            if (pData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5f) ? pData->param.ranges[k].min : pData->param.ranges[k].max;
                            }
                            else
                            {
                                value = pData->param.ranges[k].getUnnormalizedValue(ctrlEvent.value);

                                if (pData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            fShmRtControl.writeOpcode(kPluginBridgeRtSetParameter);
                            fShmRtControl.writeInt(static_cast<int32_t>(k));
                            fShmRtControl.writeFloat(value);
                            fShmRtControl.commitWrite();

                            pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, value);
                            break;
                        }

                        // check if event is already handled
                        if (k != pData->param.count)
                            break;

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param <= 0x5F)
                        {
                            fShmRtControl.writeOpcode(kPluginBridgeRtMidiData);
                            fShmRtControl.writeInt(static_cast<int32_t>(event.time));
                            fShmRtControl.writeInt(3);
                            fShmRtControl.writeByte(static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE + event.channel));
                            fShmRtControl.writeByte(static_cast<uint8_t>(ctrlEvent.param));
                            fShmRtControl.writeByte(static_cast<uint8_t>(ctrlEvent.value*127.0f));
                            fShmRtControl.commitWrite();
                        }

                        break;
                    } // case kEngineControlEventTypeParameter

                    case kEngineControlEventTypeMidiBank:
                        if (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
                        {
                            fShmRtControl.writeOpcode(kPluginBridgeRtMidiBank);
                            fShmRtControl.writeInt(static_cast<int32_t>(event.time));
                            fShmRtControl.writeByte(event.channel);
                            fShmRtControl.writeShort(static_cast<int16_t>(ctrlEvent.param));
                            fShmRtControl.commitWrite();
                        }
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
                        {
                            fShmRtControl.writeOpcode(kPluginBridgeRtMidiProgram);
                            fShmRtControl.writeInt(static_cast<int32_t>(event.time));
                            fShmRtControl.writeByte(event.channel);
                            fShmRtControl.writeShort(static_cast<int16_t>(ctrlEvent.param));
                            fShmRtControl.commitWrite();
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            fShmRtControl.writeOpcode(kPluginBridgeRtAllSoundOff);
                            fShmRtControl.writeInt(static_cast<int32_t>(event.time));
                            fShmRtControl.commitWrite();
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

                            fShmRtControl.writeOpcode(kPluginBridgeRtAllNotesOff);
                            fShmRtControl.writeInt(static_cast<int32_t>(event.time));
                            fShmRtControl.commitWrite();
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi:
                {
                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size == 0 || midiEvent.size > 4)
                        continue;

                    uint8_t status  = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiEvent.data));
                    uint8_t channel = event.channel;

                    if (MIDI_IS_STATUS_NOTE_ON(status) && midiEvent.data[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    if (status == MIDI_STATUS_CHANNEL_PRESSURE && (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (status == MIDI_STATUS_CONTROL_CHANGE && (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH && (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (status == MIDI_STATUS_PITCH_WHEEL_CONTROL && (pData->options & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    fShmRtControl.writeOpcode(kPluginBridgeRtMidiData);
                    fShmRtControl.writeInt(static_cast<int32_t>(event.time));
                    fShmRtControl.writeInt(midiEvent.size);

                    fShmRtControl.writeByte(static_cast<uint8_t>(status + channel));

                    for (uint8_t j=1; j < midiEvent.size; ++j)
                        fShmRtControl.writeByte(midiEvent.data[j]);

                    fShmRtControl.commitWrite();

                    if (status == MIDI_STATUS_NOTE_ON)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, channel, midiEvent.data[1], midiEvent.data[2]);
                    else if (status == MIDI_STATUS_NOTE_OFF)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, channel, midiEvent.data[1], 0.0f);

                    break;
                }
                }
            }

            pData->postRtEvents.trySplice();

        } // End of Event Input

        processSingle(inBuffer, outBuffer, frames);
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(inBuffer != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(outBuffer != nullptr, false);
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
                FloatVectorOperations::clear(outBuffer[i], static_cast<int>(frames));
            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        //std::memset(fShmAudioPool.data, 0, fShmAudioPool.size);

        for (uint32_t i=0; i < fInfo.aIns; ++i)
            FloatVectorOperations::copy(fShmAudioPool.data + (i * frames), inBuffer[i], static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------
        // TimeInfo

        const EngineTimeInfo& timeInfo(pData->engine->getTimeInfo());
        BridgeTimeInfo& bridgeTimeInfo(fShmRtControl.data->timeInfo);

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
            fShmRtControl.writeOpcode(kPluginBridgeRtProcess);
            fShmRtControl.commitWrite();
        }

        if (! waitForServer(2))
        {
            pData->singleMutex.unlock();
            return true;
        }

        for (uint32_t i=0; i < fInfo.aOuts; ++i)
            FloatVectorOperations::copy(outBuffer[i], fShmAudioPool.data + ((i + fInfo.aIns) * frames), static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doVolume  = (pData->hints & PLUGIN_CAN_VOLUME) != 0 && pData->postProc.volume != 1.0f;
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && pData->postProc.dryWet != 1.0f;
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && (pData->postProc.balanceLeft != -1.0f || pData->postProc.balanceRight != 1.0f);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (uint32_t k=0; k < frames; ++k)
                    {
                        bufValue        = inBuffer[(pData->audioIn.count == 1) ? 0 : i][k];
                        outBuffer[i][k] = (outBuffer[i][k] * pData->postProc.dryWet) + (bufValue * (1.0f - pData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < pData->audioOut.count);
                        FloatVectorOperations::copy(oldBufLeft, outBuffer[i], static_cast<int>(frames));
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (isPair)
                        {
                            // left
                            outBuffer[i][k]  = oldBufLeft[k]     * (1.0f - balRangeL);
                            outBuffer[i][k] += outBuffer[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            outBuffer[i][k]  = outBuffer[i][k] * balRangeR;
                            outBuffer[i][k] += oldBufLeft[k]   * balRangeL;
                        }
                    }
                }

                // Volume (and buffer copy)
                if (doVolume)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        outBuffer[i][k] *= pData->postProc.volume;
                }
            }

        } // End of Post-processing

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

        resizeAudioPool(newBufferSize);

        fShmNonRtControl.writeOpcode(kPluginBridgeNonRtSetBufferSize);
        fShmNonRtControl.writeInt(static_cast<int32_t>(newBufferSize));
        fShmNonRtControl.commitWrite();
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        const CarlaMutexLocker _cml(fShmNonRtControl.mutex);

        fShmNonRtControl.writeOpcode(kPluginBridgeNonRtSetSampleRate);
        fShmNonRtControl.writeDouble(newSampleRate);
        fShmNonRtControl.commitWrite();
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

    // nothing

    // -------------------------------------------------------------------

    int setOscPluginBridgeInfo(const PluginBridgeOscInfoType infoType, const int argc, const lo_arg* const* const argv, const char* const types)
    {
#ifdef DEBUG
        if (infoType != kPluginBridgeOscPong) {
            carla_debug("BridgePlugin::setOscPluginBridgeInfo(%s, %i, %p, \"%s\")", PluginBridgeOscInfoType2str(infoType), argc, argv, types);
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
                    count = maxParams;
                    carla_safe_assert_int2("count <= pData->engine->getOptions().maxParameters", __FILE__, __LINE__, count, maxParams);
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
            CARLA_SAFE_ASSERT_BREAK(midiCC >= -1 && midiCC < 0x5F);
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

            fShmAudioPool.shm = shm_mkstemp(tmpFileBase);

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

            std::sprintf(tmpFileBase, "/carla-bridge_shm_rt_XXXXXX");

            fShmRtControl.shm = shm_mkstemp(tmpFileBase);

            if (! carla_is_shm_valid(fShmRtControl.shm))
            {
                carla_stdout("Failed to open or create shared memory file #2");
                // clear
                carla_shm_close(fShmAudioPool.shm);
                return false;
            }

            fShmRtControl.filename = tmpFileBase;

            if (! fShmRtControl.mapData())
            {
                carla_stdout("Failed to map shared memory file #2");
                // clear
                carla_shm_close(fShmRtControl.shm);
                carla_shm_close(fShmAudioPool.shm);
                return false;
            }

            CARLA_SAFE_ASSERT(fShmRtControl.data != nullptr);

            if (! jackbridge_sem_init(&fShmRtControl.data->sem.server))
            {
                carla_stdout("Failed to initialize shared memory semaphore #1");
                // clear
                fShmRtControl.unmapData();
                carla_shm_close(fShmRtControl.shm);
                carla_shm_close(fShmAudioPool.shm);
                return false;
            }

            if (! jackbridge_sem_init(&fShmRtControl.data->sem.client))
            {
                carla_stdout("Failed to initialize shared memory semaphore #2");
                // clear
                jackbridge_sem_destroy(&fShmRtControl.data->sem.server);
                fShmRtControl.unmapData();
                carla_shm_close(fShmRtControl.shm);
                carla_shm_close(fShmAudioPool.shm);
                return false;
            }

            fNeedsSemDestroy = true;
        }

        // ---------------------------------------------------------------
        // SHM Non-RT Control
        {
            char tmpFileBase[64];

            std::sprintf(tmpFileBase, "/carla-bridge_shm_nonrt_XXXXXX");

            fShmNonRtControl.shm = shm_mkstemp(tmpFileBase);

            if (! carla_is_shm_valid(fShmNonRtControl.shm))
            {
                carla_stdout("Failed to open or create shared memory file #3");
                return false;
            }

            fShmNonRtControl.filename = tmpFileBase;

            if (! fShmNonRtControl.mapData())
            {
                carla_stdout("Failed to map shared memory file #3");
                // clear
                fShmNonRtControl.unmapData();
                carla_shm_close(fShmNonRtControl.shm);
                carla_shm_close(fShmRtControl.shm);
                carla_shm_close(fShmAudioPool.shm);
                return false;
            }
        }

        carla_stdout("Carla Server Info:");
        carla_stdout("  sizeof(BridgeRtData):    " P_SIZE, sizeof(BridgeRtData));
        carla_stdout("  sizeof(BridgeNonRtData): " P_SIZE, sizeof(BridgeNonRtData));

        // initial values
        fShmNonRtControl.writeOpcode(kPluginBridgeNonRtNull);
        fShmNonRtControl.writeInt(static_cast<int32_t>(sizeof(BridgeRtData)));
        fShmNonRtControl.writeInt(static_cast<int32_t>(sizeof(BridgeNonRtData)));

        fShmNonRtControl.writeOpcode(kPluginBridgeNonRtSetBufferSize);
        fShmNonRtControl.writeInt(static_cast<int32_t>(pData->engine->getBufferSize()));

        fShmNonRtControl.writeOpcode(kPluginBridgeNonRtSetSampleRate);
        fShmNonRtControl.writeDouble(pData->engine->getSampleRate());

        fShmNonRtControl.commitWrite();

        // register plugin now so we can receive OSC (and wait for it)
        pData->hints |= PLUGIN_IS_BRIDGE;
        pData->engine->registerEnginePlugin(pData->id, this);

        // init OSC
        {
            char shmIdStr[18+1] = { 0 };
            std::strncpy(shmIdStr, &fShmAudioPool.filename[fShmAudioPool.filename.length()-6], 6);
            std::strncat(shmIdStr, &fShmRtControl.filename[fShmRtControl.filename.length()-6], 6);
            std::strncat(shmIdStr, &fShmNonRtControl.filename[fShmNonRtControl.filename.length()-6], 6);

            pData->osc.thread.setOscData(bridgeBinary, label, getPluginTypeAsString(fPluginType), shmIdStr);
            pData->osc.thread.startThread();
        }

        fInitiated = false;
        fLastPongCounter = 0;

        for (; fLastPongCounter < 200; ++fLastPongCounter)
        {
            if (fInitiated || ! pData->osc.thread.isThreadRunning())
                break;
            carla_msleep(30);
            pData->engine->callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);
            pData->engine->idle();
        }

        fLastPongCounter = -1;

        if (fInitError || ! fInitiated)
        {
            pData->osc.thread.stopThread(6000);

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

    CarlaString fBridgeBinary;

    BridgeAudioPool    fShmAudioPool;
    BridgeRtControl    fShmRtControl;
    BridgeNonRtControl fShmNonRtControl;

    struct Info {
        uint32_t aIns, aOuts;
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
              mIns(0),
              mOuts(0),
              category(PLUGIN_CATEGORY_NONE),
              optionsAvailable(0),
              uniqueId(0) {}
    } fInfo;

    BridgeParamInfo* fParams;

    void resizeAudioPool(const uint32_t bufferSize)
    {
        fShmAudioPool.resize(bufferSize, fInfo.aIns+fInfo.aOuts);

        fShmRtControl.writeOpcode(kPluginBridgeRtSetAudioPool);
        fShmRtControl.writeLong(static_cast<int64_t>(fShmAudioPool.size));
        fShmRtControl.commitWrite();

        waitForServer();
    }

    bool waitForServer(const int secs = 5)
    {
        CARLA_SAFE_ASSERT_RETURN(! fTimedOut, false);

        if (! fShmRtControl.waitForServer(secs))
        {
            carla_stderr("waitForServer() timeout here");
            fTimedOut = true;
            return false;
        }

        return true;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BridgePlugin)
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

    BridgePlugin* const plugin(new BridgePlugin(init.engine, init.id, btype, ptype));

    if (! plugin->init(init.filename, init.name, init.label, bridgeBinary))
    {
        init.engine->registerEnginePlugin(init.id, nullptr);
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && ! plugin->canRunInRack())
    {
        init.engine->setLastError("Carla's rack mode can only work with Stereo Bridged plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("Plugin bridge support not available");
    return nullptr;

    // unused
    (void)bridgeBinary;
#endif
}

#ifndef BUILD_BRIDGE
// -------------------------------------------------------------------------------------------------------------------
// Bridge Helper

#define bridgePlugin ((BridgePlugin*)plugin)

extern int CarlaPluginSetOscBridgeInfo(CarlaPlugin* const plugin, const PluginBridgeOscInfoType type,
                                       const int argc, const lo_arg* const* const argv, const char* const types);

int CarlaPluginSetOscBridgeInfo(CarlaPlugin* const plugin, const PluginBridgeOscInfoType type,
                                const int argc, const lo_arg* const* const argv, const char* const types)
{
    CARLA_SAFE_ASSERT(plugin != nullptr && (plugin->getHints() & PLUGIN_IS_BRIDGE) != 0);
    return bridgePlugin->setOscPluginBridgeInfo(type, argc, argv, types);
}

#undef bridgePlugin

#endif

CARLA_BACKEND_END_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
