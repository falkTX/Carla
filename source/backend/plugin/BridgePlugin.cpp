/*
 * Carla Bridge Plugin
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaPluginInternal.hpp"

#ifndef BUILD_BRIDGE

#include "CarlaBridgeUtils.hpp"
#include "CarlaShmUtils.hpp"

#include "jackbridge/JackBridge.hpp"

#include <cerrno>
#include <cmath>
#include <ctime>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>

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

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Engine Helpers

extern void registerEnginePlugin(CarlaEngine* const engine, const unsigned int id, CarlaPlugin* const plugin);

// -------------------------------------------------------------------------------------------------------------------

shm_t shm_mkstemp(char* const fileBase)
{
    static const char charSet[] = "abcdefghijklmnopqrstuvwxyz"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "0123456789";

    const int size = (fileBase != nullptr) ? std::strlen(fileBase) : 0;

    shm_t shm;
    carla_shm_init(shm);

    if (size < 6)
    {
        errno = EINVAL;
        return shm;
    }

    if (std::strcmp(fileBase + size - 6, "XXXXXX") != 0)
    {
        errno = EINVAL;
        return shm;
    }

    while (true)
    {
        for (int c = size - 6; c < size; ++c)
        {
            // Note the -1 to avoid the trailing '\0' in charSet.
            fileBase[c] = charSet[std::rand() % (sizeof(charSet) - 1)];
        }

        shm_t shm = carla_shm_create(fileBase);

        if (carla_is_shm_valid(shm) || errno != EEXIST)
            return shm;
    }
}

// -------------------------------------------------------------------------------------------------------------------

struct BridgeParamInfo {
    float value;
    CarlaString name;
    CarlaString unit;

    BridgeParamInfo()
        : value(0.0f) {}

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(BridgeParamInfo)
};

class BridgePlugin : public CarlaPlugin
{
public:
    BridgePlugin(CarlaEngine* const engine, const unsigned int id, const BinaryType btype, const PluginType ptype)
        : CarlaPlugin(engine, id),
          fBinaryType(btype),
          fPluginType(ptype),
          fInitiated(false),
          fInitError(false),
          fSaved(false),
          fParams(nullptr)
    {
        carla_debug("BridgePlugin::BridgePlugin(%p, %i, %s, %s)", engine, id, BinaryType2Str(btype), PluginType2Str(ptype));

        kData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_BRIDGE);

        fHints |= PLUGIN_IS_BRIDGE;
    }

    ~BridgePlugin() override
    {
        carla_debug("BridgePlugin::~BridgePlugin()");

        kData->singleMutex.lock();
        kData->masterMutex.lock();

        if (kData->active)
        {
            deactivate();
            kData->active = false;
        }

        if (kData->osc.thread.isRunning())
        {
            rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeQuit);
            rdwr_commitWrite(&fShmControl.data->ringBuffer);
            waitForServer();
        }

        if (kData->osc.data.target != nullptr)
        {
            osc_send_hide(&kData->osc.data);
            osc_send_quit(&kData->osc.data);
        }

        kData->osc.data.free();

        // Wait a bit first, then force kill
        if (kData->osc.thread.isRunning() && ! kData->osc.thread.wait(kData->engine->getOptions().oscUiTimeout))
        {
            carla_stderr("Failed to properly stop Plugin Bridge thread");
            kData->osc.thread.terminate();
        }

        cleanup();
        clearBuffers();

        //info.chunk.clear();
    }

    // -------------------------------------------------------------------
    // Information (base)

    BinaryType binaryType() const
    {
        return fBinaryType;
    }

    PluginType type() const override
    {
        return fPluginType;
    }

    PluginCategory category() override
    {
        return fInfo.category;
    }

    long uniqueId() const override
    {
        return fInfo.uniqueId;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t midiInCount() const override
    {
        return fInfo.mIns;
    }

    uint32_t midiOutCount() const override
    {
        return fInfo.mOuts;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t chunkData(void** const dataPtr) override
    {
        CARLA_ASSERT(fOptions & PLUGIN_OPTION_USE_CHUNKS);
        CARLA_ASSERT(dataPtr != nullptr);

#if 0
        if (! info.chunk.isEmpty())
        {
            *dataPtr = info.chunk.data();
            return info.chunk.size();
        }
#endif

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int availableOptions() override
    {
        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        options |= PLUGIN_OPTION_USE_CHUNKS;
        options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
        options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
        options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
        options |= PLUGIN_OPTION_SEND_PITCHBEND;
        options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

        return options;
    }

    float getParameterValue(const uint32_t parameterId) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        return fParams[parameterId].value;
    }

    void getLabel(char* const strBuf) override
    {
        std::strncpy(strBuf, (const char*)fInfo.label, STR_MAX);
    }

    void getMaker(char* const strBuf) override
    {
        std::strncpy(strBuf, (const char*)fInfo.maker, STR_MAX);
    }

    void getCopyright(char* const strBuf) override
    {
        std::strncpy(strBuf, (const char*)fInfo.copyright, STR_MAX);
    }

    void getRealName(char* const strBuf) override
    {
        std::strncpy(strBuf, (const char*)fInfo.name, STR_MAX);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        std::strncpy(strBuf, (const char*)fParams[parameterId].name, STR_MAX);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        std::strncpy(strBuf, (const char*)fParams[parameterId].unit, STR_MAX);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    void prepareForSave() override
    {
#if 0
        m_saved = false;
        osc_send_configure(&osc.data, CARLA_BRIDGE_MSG_SAVE_NOW, "");

        for (int i=0; i < 200; ++i)
        {
            if (m_saved)
                break;
            carla_msleep(50);
        }

        if (! m_saved)
            carla_stderr("BridgePlugin::prepareForSave() - Timeout while requesting save state");
        else
            carla_debug("BridgePlugin::prepareForSave() - success!");
#endif
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    // nothing

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        const float fixedValue(kData->param.fixValue(parameterId, value));
        fParams[parameterId].value = fixedValue;

        const bool doLock(sendGui || sendOsc || sendCallback);

        if (doLock)
            kData->singleMutex.lock();

        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeSetParameter);
        rdwr_writeInt(&fShmControl.data->ringBuffer, parameterId);
        rdwr_writeFloat(&fShmControl.data->ringBuffer, value);

        if (doLock)
        {
            rdwr_commitWrite(&fShmControl.data->ringBuffer);
            kData->singleMutex.unlock();
        }

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(kData->prog.count));

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(kData->prog.count))
            return;

        const bool doLock(sendGui || sendOsc || sendCallback);

        if (doLock)
            kData->singleMutex.lock();

        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeSetProgram);
        rdwr_writeInt(&fShmControl.data->ringBuffer, index);

        if (doLock)
        {
            rdwr_commitWrite(&fShmControl.data->ringBuffer);
            kData->singleMutex.unlock();
        }

        CarlaPlugin::setProgram(index, sendGui, sendOsc, sendCallback);
    }

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(kData->midiprog.count));

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(kData->midiprog.count))
            return;

        const bool doLock(sendGui || sendOsc || sendCallback);

        if (doLock)
            kData->singleMutex.lock();

        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeSetMidiProgram);
        rdwr_writeInt(&fShmControl.data->ringBuffer, index);

        if (doLock)
        {
            rdwr_commitWrite(&fShmControl.data->ringBuffer);
            kData->singleMutex.unlock();
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

    void setChunkData(const char* const stringData) override
    {
        CARLA_ASSERT(m_hints & PLUGIN_USES_CHUNKS);
        CARLA_ASSERT(stringData);

        QString filePath;
        filePath  = QDir::tempPath();
        filePath += "/.CarlaChunk_";
        filePath += m_name;

        filePath = QDir::toNativeSeparators(filePath);

        QFile file(filePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            out << stringData;
            file.close();
            osc_send_configure(&osc.data, CARLA_BRIDGE_MSG_SET_CHUNK, filePath.toUtf8().constData());
        }
    }
#endif

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo) override
    {
        if (yesNo)
            osc_send_show(&kData->osc.data);
        else
            osc_send_hide(&kData->osc.data);
    }

    void idleGui() override
    {
        if (! kData->osc.thread.isRunning())
            carla_stderr2("TESTING: Bridge has closed!");

        CarlaPlugin::idleGui();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        carla_debug("BridgePlugin::reload() - start");
        CARLA_ASSERT(kData->engine != nullptr);

        if (kData->engine == nullptr)
            return;

        const ProcessMode processMode(kData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        if (fInfo.aIns > 0)
        {
            kData->audioIn.createNew(fInfo.aIns);
        }

        if (fInfo.aOuts > 0)
        {
            kData->audioOut.createNew(fInfo.aOuts);
            needsCtrlIn = true;
        }

        if (fInfo.mIns > 0)
            needsCtrlIn = true;

        if (fInfo.mOuts > 0)
            needsCtrlOut = true;

        const uint portNameSize(kData->engine->maxPortNameSize());
        CarlaString portName;

        // Audio Ins
        for (uint32_t j=0; j < fInfo.aIns; ++j)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
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

            kData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, true);
            kData->audioIn.ports[j].rindex = j;
        }

        // Audio Outs
        for (uint32_t j=0; j < fInfo.aOuts; ++j)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
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

            kData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
            kData->audioOut.ports[j].rindex = j;
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "event-in";
            portName.truncate(portNameSize);

            kData->event.portIn = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "event-out";
            portName.truncate(portNameSize);

            kData->event.portOut = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        bufferSizeChanged(kData->engine->getBufferSize());
        reloadPrograms(true);

        carla_debug("BridgePlugin::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() override
    {
        // already locked before
        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeSetParameter);
        rdwr_writeInt(&fShmControl.data->ringBuffer, PARAMETER_ACTIVE);
        rdwr_writeFloat(&fShmControl.data->ringBuffer, 1.0f);
        rdwr_commitWrite(&fShmControl.data->ringBuffer);
        waitForServer();
    }

    void deactivate() override
    {
        // already locked before
        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeSetParameter);
        rdwr_writeInt(&fShmControl.data->ringBuffer, PARAMETER_ACTIVE);
        rdwr_writeFloat(&fShmControl.data->ringBuffer, 0.0f);
        rdwr_commitWrite(&fShmControl.data->ringBuffer);
        waitForServer();
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames) override
    {
        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! kData->active)
        {
            // disable any output sound
            for (i=0; i < kData->audioOut.count; ++i)
                carla_zeroFloat(outBuffer[i], frames);

            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (kData->needsReset)
        {
            // TODO

            kData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input

        if (kData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (kData->extNotes.mutex.tryLock())
            {
                while (! kData->extNotes.data.isEmpty())
                {
                    const ExternalMidiNote& note(kData->extNotes.data.getFirst(true));

                    CARLA_ASSERT(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    char data1, data2, data3;
                    data1  = (note.velo > 0) ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
                    data1 += note.channel;
                    data2  = note.note;
                    data3  = note.velo;

                    rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeMidiEvent);
                    rdwr_writeLong(&fShmControl.data->ringBuffer, 0);
                    rdwr_writeInt(&fShmControl.data->ringBuffer, 3);
                    rdwr_writeChar(&fShmControl.data->ringBuffer, data1);
                    rdwr_writeChar(&fShmControl.data->ringBuffer, data2);
                    rdwr_writeChar(&fShmControl.data->ringBuffer, data3);
                }

                kData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;

            uint32_t nEvents = kData->event.portIn->getEventCount();
            uint32_t nextBankId = 0;

            if (kData->midiprog.current >= 0 && kData->midiprog.count > 0)
                nextBankId = kData->midiprog.data[kData->midiprog.current].bank;

            for (i=0; i < nEvents; ++i)
            {
                const EngineEvent& event(kData->event.portIn->getEvent(i));

                // Control change
                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl:
                {
                    const EngineControlEvent& ctrlEvent = event.ctrl;

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter:
                    {
                        // Control backend stuff
                        if (event.channel == kData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (fHints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (fHints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (fHints & PLUGIN_CAN_BALANCE) > 0)
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
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                            }
                        }

                        // Control plugin parameters
                        for (k=0; k < kData->param.count; ++k)
                        {
                            if (kData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (kData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (kData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((kData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            float value;

                            if (kData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5f) ? kData->param.ranges[k].min : kData->param.ranges[k].max;
                            }
                            else
                            {
                                value = kData->param.ranges[i].unnormalizeValue(ctrlEvent.value);

                                if (kData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, value);
                        }

                        if ((fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param <= 0x5F)
                        {
                            rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeMidiEvent);
                            rdwr_writeLong(&fShmControl.data->ringBuffer, event.time);
                            rdwr_writeInt(&fShmControl.data->ringBuffer, 3);
                            rdwr_writeChar(&fShmControl.data->ringBuffer, MIDI_STATUS_CONTROL_CHANGE + event.channel);
                            rdwr_writeChar(&fShmControl.data->ringBuffer, ctrlEvent.param);
                            rdwr_writeChar(&fShmControl.data->ringBuffer, ctrlEvent.value*127.0f);
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == kData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == kData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t nextProgramId(ctrlEvent.param);

                            if (kData->midiprog.count > 0)
                            {
                                for (k=0; k < kData->midiprog.count; ++k)
                                {
                                    if (kData->midiprog.data[k].bank == nextBankId && kData->midiprog.data[k].program == nextProgramId)
                                    {
                                        setMidiProgram(k, false, false, false);
                                        postponeRtEvent(kPluginPostRtEventMidiProgramChange, k, 0, 0.0f);
                                        break;
                                    }
                                }
                            }
                            else
                            {
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            // TODO
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (event.channel == kData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }

                            // TODO
                        }
                        break;
                    }

                    break;
                }

                case kEngineEventTypeMidi:
                {
                    const EngineMidiEvent& midiEvent(event.midi);

                    uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                    uint8_t channel = event.channel;

                    if (MIDI_IS_STATUS_AFTERTOUCH(status) && (fOptions & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (MIDI_IS_STATUS_CONTROL_CHANGE(status) && (fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status) && (fOptions & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status) && (fOptions & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    // Fix bad note-off
                    if (status == MIDI_STATUS_NOTE_ON && midiEvent.data[2] == 0)
                        status -= 0x10;

                    char data[4];
                    data[0] = status + channel;
                    data[1] = midiEvent.data[1];
                    data[2] = midiEvent.data[2];
                    data[3] = midiEvent.data[3];

                    rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeMidiEvent);
                    rdwr_writeLong(&fShmControl.data->ringBuffer, event.time);
                    rdwr_writeInt(&fShmControl.data->ringBuffer, midiEvent.size);

                    for (uint8_t j=0; j < midiEvent.size && j < 4; ++j)
                        rdwr_writeChar(&fShmControl.data->ringBuffer, data[j]);

                    if (status == MIDI_STATUS_NOTE_ON)
                        postponeRtEvent(kPluginPostRtEventNoteOn, channel, midiEvent.data[1], midiEvent.data[2]);
                    else if (status == MIDI_STATUS_NOTE_OFF)
                        postponeRtEvent(kPluginPostRtEventNoteOff, channel, midiEvent.data[1], 0.0f);

                    break;
                }
                }
            }

            kData->postRtEvents.trySplice();

        } // End of Event Input

        processSingle(inBuffer, outBuffer, frames);
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames)
    {
        CARLA_ASSERT(frames > 0);

        if (frames == 0)
            return false;

        if (kData->audioIn.count > 0)
        {
            CARLA_ASSERT(inBuffer != nullptr);
            if (inBuffer == nullptr)
                return false;
        }
        if (kData->audioOut.count > 0)
        {
            CARLA_ASSERT(outBuffer != nullptr);
            if (outBuffer == nullptr)
                return false;
        }

        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (kData->engine->isOffline())
        {
            kData->singleMutex.lock();
        }
        else if (! kData->singleMutex.tryLock())
        {
            for (i=0; i < kData->audioOut.count; ++i)
                carla_zeroFloat(outBuffer[i], frames);

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        for (i=0; i < fInfo.aIns; ++i)
            carla_copyFloat(fShmAudioPool.data + (i * frames), inBuffer[i], frames);

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeProcess);
        rdwr_commitWrite(&fShmControl.data->ringBuffer);

        if (! waitForServer())
        {
            kData->singleMutex.unlock();
            return true;
        }

        for (i=0; i < fInfo.aOuts; ++i)
            carla_copyFloat(outBuffer[i], fShmAudioPool.data + ((i + fInfo.aIns) * frames), frames);

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doVolume  = (fHints & PLUGIN_CAN_VOLUME) != 0 && kData->postProc.volume != 1.0f;
            const bool doDryWet  = (fHints & PLUGIN_CAN_DRYWET) != 0 && kData->postProc.dryWet != 1.0f;
            const bool doBalance = (fHints & PLUGIN_CAN_BALANCE) != 0 && (kData->postProc.balanceLeft != -1.0f || kData->postProc.balanceRight != 1.0f);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < kData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (k=0; k < frames; ++k)
                    {
                        bufValue        = inBuffer[(kData->audioIn.count == 1) ? 0 : i][k];
                        outBuffer[i][k] = (outBuffer[i][k] * kData->postProc.dryWet) + (bufValue * (1.0f - kData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < kData->audioOut.count);
                        carla_copyFloat(oldBufLeft, outBuffer[i], frames);
                    }

                    float balRangeL = (kData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (kData->postProc.balanceRight + 1.0f)/2.0f;

                    for (k=0; k < frames; ++k)
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

                // Volume
                if (doVolume)
                {
                    for (k=0; k < frames; ++k)
                        outBuffer[i][k] *= kData->postProc.volume;
                }
            }

        } // End of Post-processing

        // --------------------------------------------------------------------------------------------------------

        kData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        resizeAudioPool(newBufferSize);

        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeSetBufferSize);
        rdwr_writeInt(&fShmControl.data->ringBuffer, newBufferSize);
        rdwr_commitWrite(&fShmControl.data->ringBuffer);

    }

    void sampleRateChanged(const double newSampleRate) override
    {
        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeSetSampleRate);
        rdwr_writeFloat(&fShmControl.data->ringBuffer, newSampleRate);
        rdwr_commitWrite(&fShmControl.data->ringBuffer);
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() override
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

    int setOscPluginBridgeInfo(const PluginBridgeInfoType type, const int argc, const lo_arg* const* const argv, const char* const types)
    {
        carla_debug("setOscPluginBridgeInfo(%s, %i, %p, \"%s\")", PluginBridgeInfoType2str(type), argc, argv, types);

        switch (type)
        {
        case kPluginBridgeAudioCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iii");

            const int32_t aIns   = argv[0]->i;
            const int32_t aOuts  = argv[1]->i;
            const int32_t aTotal = argv[2]->i;

            CARLA_ASSERT(aIns >= 0);
            CARLA_ASSERT(aOuts >= 0);
            CARLA_ASSERT(aIns + aOuts == aTotal);

            fInfo.aIns  = aIns;
            fInfo.aOuts = aOuts;

            break;

            // unused
            (void)aTotal;
        }

        case kPluginBridgeMidiCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iii");

            const int32_t mIns   = argv[0]->i;
            const int32_t mOuts  = argv[1]->i;
            const int32_t mTotal = argv[2]->i;

            CARLA_ASSERT(mIns >= 0);
            CARLA_ASSERT(mOuts >= 0);
            CARLA_ASSERT(mIns + mOuts == mTotal);

            fInfo.mIns  = mIns;
            fInfo.mOuts = mOuts;

            break;

            // unused
            (void)mTotal;
        }

        case kPluginBridgeParameterCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iii");

            const int32_t pIns   = argv[0]->i;
            const int32_t pOuts  = argv[1]->i;
            const int32_t pTotal = argv[2]->i;

            CARLA_ASSERT(pIns >= 0);
            CARLA_ASSERT(pOuts >= 0);
            CARLA_ASSERT(pIns + pOuts <= pTotal);

            // delete old data
            kData->param.clear();

            if (fParams != nullptr)
            {
                delete[] fParams;
                fParams = nullptr;
            }

            CARLA_SAFE_ASSERT_INT2(pTotal < static_cast<int32_t>(kData->engine->getOptions().maxParameters), pTotal, kData->engine->getOptions().maxParameters);

            const int32_t count(carla_min<int32_t>(pTotal, kData->engine->getOptions().maxParameters, 0));

            if (count > 0)
            {
                kData->param.createNew(count);
                fParams = new BridgeParamInfo[count];
            }

            break;

            // unused
            (void)pIns;
            (void)pOuts;
        }

        case kPluginBridgeProgramCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t count = argv[0]->i;

            CARLA_ASSERT(count >= 0);

            kData->prog.clear();

            if (count > 0)
                kData->prog.createNew(count);

            break;
        }

        case kPluginBridgeMidiProgramCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t count = argv[0]->i;

            CARLA_ASSERT(count >= 0);

            kData->midiprog.clear();

            if (count > 0)
                kData->midiprog.createNew(count);

            break;
        }

        case kPluginBridgePluginInfo:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(7, "iissssh");

            const int32_t category  = argv[0]->i;
            const int32_t hints     = argv[1]->i;
            const char* const name  = (const char*)&argv[2]->s;
            const char* const label = (const char*)&argv[3]->s;
            const char* const maker = (const char*)&argv[4]->s;
            const char* const copyright = (const char*)&argv[5]->s;
            const int64_t uniqueId      = argv[6]->h;

            CARLA_ASSERT(category >= 0);
            CARLA_ASSERT(hints >= 0);
            CARLA_ASSERT(name != nullptr);
            CARLA_ASSERT(label != nullptr);
            CARLA_ASSERT(maker != nullptr);
            CARLA_ASSERT(copyright != nullptr);

            fHints = hints | PLUGIN_IS_BRIDGE;

            fInfo.category = static_cast<PluginCategory>(category);
            fInfo.uniqueId = uniqueId;

            fInfo.name  = name;
            fInfo.label = label;
            fInfo.maker = maker;
            fInfo.copyright = copyright;

            if (fName.isEmpty())
                fName = name;

            break;
        }

        case kPluginBridgeParameterInfo:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iss");

            const int32_t index    = argv[0]->i;
            const char* const name = (const char*)&argv[1]->s;
            const char* const unit = (const char*)&argv[2]->s;

            CARLA_ASSERT_INT2(index >= 0 && index < static_cast<int32_t>(kData->param.count), index, kData->param.count);
            CARLA_ASSERT(name != nullptr);
            CARLA_ASSERT(unit != nullptr);

            if (index >= 0 && static_cast<int32_t>(kData->param.count))
            {
                fParams[index].name = name;
                fParams[index].unit = unit;
            }

            break;
        }

        case kPluginBridgeParameterData:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(6, "iiiiii");

            const int32_t index   = argv[0]->i;
            const int32_t type    = argv[1]->i;
            const int32_t rindex  = argv[2]->i;
            const int32_t hints   = argv[3]->i;
            const int32_t channel = argv[4]->i;
            const int32_t cc      = argv[5]->i;

            CARLA_ASSERT_INT2(index >= 0 && index < static_cast<int32_t>(kData->param.count), index, kData->param.count);
            CARLA_ASSERT(type >= 0);
            CARLA_ASSERT(rindex >= 0);
            CARLA_ASSERT(hints >= 0);
            CARLA_ASSERT(channel >= 0 && channel < 16);
            CARLA_ASSERT(cc >= -1);

            if (index >= 0 && static_cast<int32_t>(kData->param.count))
            {
                kData->param.data[index].type    = static_cast<ParameterType>(type);
                kData->param.data[index].index   = index;
                kData->param.data[index].rindex  = rindex;
                kData->param.data[index].hints   = hints;
                kData->param.data[index].midiChannel = channel;
                kData->param.data[index].midiCC  = cc;
            }

            break;
        }

        case kPluginBridgeParameterRanges:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(7, "iffffff");

            const int32_t index = argv[0]->i;
            const float def     = argv[1]->f;
            const float min     = argv[2]->f;
            const float max     = argv[3]->f;
            const float step    = argv[4]->f;
            const float stepSmall = argv[5]->f;
            const float stepLarge = argv[6]->f;

            CARLA_ASSERT_INT2(index >= 0 && index < static_cast<int32_t>(kData->param.count), index, kData->param.count);
            CARLA_ASSERT(min < max);
            CARLA_ASSERT(def >= min);
            CARLA_ASSERT(def <= max);

            if (index >= 0 && static_cast<int32_t>(kData->param.count))
            {
                kData->param.ranges[index].def  = def;
                kData->param.ranges[index].min  = min;
                kData->param.ranges[index].max  = max;
                kData->param.ranges[index].step = step;
                kData->param.ranges[index].stepSmall = stepSmall;
                kData->param.ranges[index].stepLarge = stepLarge;
            }

            break;
        }

        case kPluginBridgeProgramInfo:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "is");

            const int32_t index    = argv[0]->i;
            const char* const name = (const char*)&argv[1]->s;

            CARLA_ASSERT_INT2(index >= 0 && index < static_cast<int32_t>(kData->prog.count), index, kData->prog.count);
            CARLA_ASSERT(name != nullptr);

            if (index >= 0 && index < static_cast<int32_t>(kData->prog.count))
            {
                if (kData->prog.names[index] != nullptr)
                    delete[] kData->prog.names[index];

                kData->prog.names[index] = carla_strdup(name);
            }

            break;
        }

        case kPluginBridgeMidiProgramInfo:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(4, "iiis");

            const int32_t index    = argv[0]->i;
            const int32_t bank     = argv[1]->i;
            const int32_t program  = argv[2]->i;
            const char* const name = (const char*)&argv[3]->s;

            CARLA_ASSERT_INT2(index < static_cast<int32_t>(kData->midiprog.count), index, kData->midiprog.count);
            CARLA_ASSERT(bank >= 0);
            CARLA_ASSERT(program >= 0 && program < 128);
            CARLA_ASSERT(name != nullptr);

            if (index >= 0 && index < static_cast<int32_t>(kData->midiprog.count))
            {
                if (kData->midiprog.data[index].name != nullptr)
                    delete[] kData->midiprog.data[index].name;

                kData->midiprog.data[index].bank    = bank;
                kData->midiprog.data[index].program = program;
                kData->midiprog.data[index].name    = carla_strdup(name);
            }

            break;
        }

        case kPluginBridgeConfigure:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "ss");

            const char* const key   = (const char*)&argv[0]->s;
            const char* const value = (const char*)&argv[1]->s;

            CARLA_ASSERT(key != nullptr);
            CARLA_ASSERT(value != nullptr);

            if (key == nullptr || value == nullptr)
                break;

            if (std::strcmp(key, CARLA_BRIDGE_MSG_HIDE_GUI) == 0)
                kData->engine->callback(CALLBACK_SHOW_GUI, fId, 0, 0, 0.0f, nullptr);
            else if (std::strcmp(key, CARLA_BRIDGE_MSG_SAVED) == 0)
                fSaved = true;

            break;
        }

        case kPluginBridgeSetParameterValue:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "if");

            const int32_t index = argv[0]->i;
            const float   value = argv[1]->f;

            CARLA_ASSERT_INT2(index >= 0 && index < static_cast<int32_t>(kData->param.count), index, kData->param.count);

            if (index >= 0 && static_cast<int32_t>(kData->param.count))
            {
                const float fixedValue(kData->param.fixValue(index, value));
                fParams[index].value = fixedValue;

                CarlaPlugin::setParameterValue(index, fixedValue, false, true, true);
            }

            break;
        }

        case kPluginBridgeSetDefaultValue:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "if");

            const int32_t index = argv[0]->i;
            const float   value = argv[1]->f;

            CARLA_ASSERT_INT2(index >= 0 && index < static_cast<int32_t>(kData->param.count), index, kData->param.count);

            if (index >= 0 && static_cast<int32_t>(kData->param.count))
                kData->param.ranges[index].def = value;

            break;
        }

        case kPluginBridgeSetProgram:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t index = argv[0]->i;

            CARLA_ASSERT_INT2(index >= 0 && index < static_cast<int32_t>(kData->prog.count), index, kData->prog.count);

            setProgram(index, false, true, true);

            break;
        }

        case kPluginBridgeSetMidiProgram:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t index = argv[0]->i;

            CARLA_ASSERT_INT2(index < static_cast<int32_t>(kData->midiprog.count), index, kData->midiprog.count);

            setMidiProgram(index, false, true, true);

            break;
        }

        case kPluginBridgeSetCustomData:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "sss");

            const char* const type  = (const char*)&argv[0]->s;
            const char* const key   = (const char*)&argv[1]->s;
            const char* const value = (const char*)&argv[2]->s;

            CARLA_ASSERT(type != nullptr);
            CARLA_ASSERT(key != nullptr);
            CARLA_ASSERT(value != nullptr);

            setCustomData(type, key, value, false);

            break;
        }

        case kPluginBridgeSetChunkData:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "s");
#if 0
            const char* const chunkFileChar = (const char*)&argv[0]->s;

            CARLA_ASSERT(chunkFileChar);

            QString chunkFileStr(chunkFileChar);

#ifndef Q_OS_WIN
            // Using Wine, fix temp dir
            if (m_binary == BINARY_WIN32 || m_binary == BINARY_WIN64)
            {
                // Get WINEPREFIX
                QString wineDir;
                if (const char* const WINEPREFIX = getenv("WINEPREFIX"))
                    wineDir = QString(WINEPREFIX);
                else
                    wineDir = QDir::homePath() + "/.wine";

                QStringList chunkFileStrSplit1 = chunkFileStr.split(":/");
                QStringList chunkFileStrSplit2 = chunkFileStrSplit1.at(1).split("\\");

                QString wineDrive = chunkFileStrSplit1.at(0).toLower();
                QString wineTMP   = chunkFileStrSplit2.at(0);
                QString baseName  = chunkFileStrSplit2.at(1);

                chunkFileStr  = wineDir;
                chunkFileStr += "/drive_";
                chunkFileStr += wineDrive;
                chunkFileStr += "/";
                chunkFileStr += wineTMP;
                chunkFileStr += "/";
                chunkFileStr += baseName;
                chunkFileStr  = QDir::toNativeSeparators(chunkFileStr);
            }
#endif

            QFile chunkFile(chunkFileStr);

            if (chunkFile.open(QIODevice::ReadOnly))
            {
                info.chunk = chunkFile.readAll();
                chunkFile.close();
                chunkFile.remove();
            }
#endif
            break;
        }

        case kPluginBridgeUpdateNow:
        {
            fInitiated = true;
            break;
        }

        case kPluginBridgeError:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "s");

            const char* const error = (const char*)&argv[0]->s;

            CARLA_ASSERT(error != nullptr);

            kData->engine->setLastError(error);

            fInitError = true;
            fInitiated = true;
            break;
        }
        }

        return 0;
    }

    // -------------------------------------------------------------------

    const void* getExtraStuff() const override
    {
        return fBridgeBinary.isNotEmpty() ? (const char*)fBridgeBinary : nullptr;
    }

    bool init(const char* const filename, const char* const name, const char* const label, const char* const bridgeBinary)
    {
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(kData->client == nullptr);

        // ---------------------------------------------------------------
        // first checks

        if (kData->engine == nullptr)
        {
            return false;
        }

        if (kData->client != nullptr)
        {
            kData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        // ---------------------------------------------------------------
        // set info

        if (name != nullptr)
            fName = kData->engine->getUniquePluginName(name);

        fFilename = filename;
        fBridgeBinary = bridgeBinary;

        // ---------------------------------------------------------------
        // SHM Audio Pool
        {
            char tmpFileBase[60];

            std::srand(std::time(NULL));
            std::sprintf(tmpFileBase, "/carla-bridge_shm_XXXXXX");

            fShmAudioPool.shm = shm_mkstemp(tmpFileBase);

            if (! carla_is_shm_valid(fShmAudioPool.shm))
            {
                //_cleanup();
                carla_stdout("Failed to open or create shared memory file #1");
                return false;
            }

            fShmAudioPool.filename = tmpFileBase;
        }

        // ---------------------------------------------------------------
        // SHM Control
        {
            char tmpFileBase[60];

            std::sprintf(tmpFileBase, "/carla-bridge_shc_XXXXXX");

            fShmControl.shm = shm_mkstemp(tmpFileBase);

            if (! carla_is_shm_valid(fShmControl.shm))
            {
                //_cleanup();
                carla_stdout("Failed to open or create shared memory file #2");
                return false;
            }

            fShmControl.filename = tmpFileBase;

            if (! carla_shm_map<BridgeShmControl>(fShmControl.shm, fShmControl.data))
            {
                //_cleanup();
                carla_stdout("Failed to mmap shared memory file");
                return false;
            }

            CARLA_ASSERT(fShmControl.data != nullptr);

            std::memset(fShmControl.data, 0, sizeof(BridgeShmControl));
            std::strcpy(fShmControl.data->ringBuffer.buf, "This thing is actually working!!!!");

            if (sem_init(&fShmControl.data->runServer, 1, 0) != 0)
            {
                //_cleanup();
                carla_stdout("Failed to initialize shared memory semaphore #1");
                return false;
            }

            if (sem_init(&fShmControl.data->runClient, 1, 0) != 0)
            {
                //_cleanup();
                carla_stdout("Failed to initialize shared memory semaphore #2");
                return false;
            }
        }

        // initial values
        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeSetBufferSize);
        rdwr_writeInt(&fShmControl.data->ringBuffer, kData->engine->getBufferSize());

        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeSetSampleRate);
        rdwr_writeFloat(&fShmControl.data->ringBuffer, kData->engine->getSampleRate());

        rdwr_commitWrite(&fShmControl.data->ringBuffer);

        // register plugin now so we can receive OSC (and wait for it)
        fHints |= PLUGIN_IS_BRIDGE;
        registerEnginePlugin(kData->engine, fId, this);

        // init OSC
        {
            char shmIdStr[12+1] = { 0 };
            std::strncpy(shmIdStr, &fShmAudioPool.filename[fShmAudioPool.filename.length()-6], 6);
            std::strncat(shmIdStr, &fShmControl.filename[fShmControl.filename.length()-6], 6);

            kData->osc.thread.setOscData(bridgeBinary, label, getPluginTypeAsString(fPluginType), shmIdStr);
            kData->osc.thread.start();
        }

        for (int i=0; i < 200; ++i)
        {
            if (fInitiated || ! kData->osc.thread.isRunning())
                break;
            carla_msleep(50);
        }

        if (fInitError || ! fInitiated)
        {
            // unregister so it gets handled properly
            registerEnginePlugin(kData->engine, fId, nullptr);

            kData->osc.thread.quit();

            if (kData->osc.thread.isRunning())
                kData->osc.thread.terminate();

            if (! fInitError)
                kData->engine->setLastError("Timeout while waiting for a response from plugin-bridge\n(or the plugin crashed on initialization?)");

            return false;
        }

        // ---------------------------------------------------------------
        // register client

        kData->client = kData->engine->addClient(this);

        if (kData->client == nullptr || ! kData->client->isOk())
        {
            kData->engine->setLastError("Failed to register plugin client");
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

    CarlaString fBridgeBinary;

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
    } fShmAudioPool;

    struct BridgeControl {
        CarlaString filename;
        BridgeShmControl* data;
        shm_t shm;

        BridgeControl()
            : data(nullptr)
        {
            carla_shm_init(shm);
        }

    } fShmControl;

    struct Info {
        uint32_t aIns, aOuts;
        uint32_t mIns, mOuts;
        PluginCategory category;
        long uniqueId;
        CarlaString name;
        CarlaString label;
        CarlaString maker;
        CarlaString copyright;
        //QByteArray chunk;

        Info()
            : aIns(0),
              aOuts(0),
              mIns(0),
              mOuts(0),
              category(PLUGIN_CATEGORY_NONE),
              uniqueId(0) {}
    } fInfo;

    BridgeParamInfo* fParams;

    void cleanup()
    {
        if (fShmAudioPool.filename.isNotEmpty())
            fShmAudioPool.filename.clear();

        if (fShmControl.filename.isNotEmpty())
            fShmControl.filename.clear();

        if (fShmAudioPool.data != nullptr)
        {
            carla_shm_unmap(fShmAudioPool.shm, fShmAudioPool.data, fShmAudioPool.size);
            fShmAudioPool.data = nullptr;
        }

        fShmAudioPool.size = 0;

        if (fShmControl.data != nullptr)
        {
            carla_shm_unmap(fShmControl.shm, fShmControl.data, sizeof(BridgeShmControl));
            fShmControl.data = nullptr;
        }

        if (carla_is_shm_valid(fShmAudioPool.shm))
            carla_shm_close(fShmAudioPool.shm);

        if (carla_is_shm_valid(fShmControl.shm))
            carla_shm_close(fShmControl.shm);
    }

    void resizeAudioPool(uint32_t bufferSize)
    {
        if (fShmAudioPool.data != nullptr)
            carla_shm_unmap(fShmAudioPool.shm, fShmAudioPool.data, fShmAudioPool.size);

        fShmAudioPool.size = (fInfo.aIns+fInfo.aOuts)*bufferSize*sizeof(float);

        if (fShmAudioPool.size == 0)
            fShmAudioPool.size = sizeof(float);

        fShmAudioPool.data = (float*)carla_shm_map(fShmAudioPool.shm, fShmAudioPool.size);

        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeSetAudioPool);
        rdwr_writeInt(&fShmControl.data->ringBuffer, fShmAudioPool.size);
        rdwr_commitWrite(&fShmControl.data->ringBuffer);

        waitForServer();
    }

    bool waitForServer()
    {
        sem_post(&fShmControl.data->runServer);

        if (! jackbridge_sem_timedwait(&fShmControl.data->runClient, 5))
        {
            carla_stderr("waitForServer() timeout");
            kData->active = false; // TODO
            return false;
        }

        return true;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BridgePlugin)
};

CARLA_BACKEND_END_NAMESPACE

#endif // ! BUILD_BRIDGE

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newBridge(const Initializer& init, BinaryType btype, PluginType ptype, const char* const bridgeBinary)
{
    carla_debug("CarlaPlugin::newBridge({%p, \"%s\", \"%s\", \"%s\"}, %s, %s, \"%s\")", init.engine, init.filename, init.name, init.label, BinaryType2Str(btype), PluginType2Str(ptype), bridgeBinary);

#ifndef BUILD_BRIDGE
    if (bridgeBinary == nullptr)
    {
        init.engine->setLastError("Bridge not possible, bridge-binary not found");
        return nullptr;
    }

    BridgePlugin* const plugin(new BridgePlugin(init.engine, init.id, btype, ptype));

    if (! plugin->init(init.filename, init.name, init.label, bridgeBinary))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && ! CarlaPluginProtectedData::canRunInRack(plugin))
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
// -------------------------------------------------------------------
// Bridge Helper

int CarlaPluginSetOscBridgeInfo(CarlaPlugin* const plugin, const PluginBridgeInfoType type,
                                const int argc, const lo_arg* const* const argv, const char* const types)
{
    CARLA_ASSERT(plugin != nullptr && (plugin->hints() & PLUGIN_IS_BRIDGE) != 0);
    return ((BridgePlugin*)plugin)->setOscPluginBridgeInfo(type, argc, argv, types);
}

BinaryType CarlaPluginGetBridgeBinaryType(CarlaPlugin* const plugin)
{
    CARLA_ASSERT(plugin != nullptr && (plugin->hints() & PLUGIN_IS_BRIDGE) != 0);
    return ((BridgePlugin*)plugin)->binaryType();
}
#endif

CARLA_BACKEND_END_NAMESPACE
