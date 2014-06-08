/*
 * Carla Plugin Host
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef HAVE_JUCE
# error This file should not be compiled if Juce is disabled
#endif

#include "CarlaEngineInternal.hpp"
#include "CarlaBackendUtils.hpp"

#include "RtLinkedList.hpp"

#include "juce_audio_devices.h"

using namespace juce;

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------------------------------------------------------
// Global static data

static const char** gRetNames = nullptr;
static OwnedArray<AudioIODeviceType> gJuceDeviceTypes;

struct JuceCleanup : public DeletedAtShutdown {
    JuceCleanup() noexcept {}
    ~JuceCleanup()
    {
        if (gRetNames != nullptr)
        {
            for (int i=0; gRetNames[i] != nullptr; ++i)
                delete[] gRetNames[i];

            delete[] gRetNames;
            gRetNames = nullptr;
        }

        gJuceDeviceTypes.clear(true);
    }
};

// -------------------------------------------------------------------------------------------------------------------
// Cleanup

static void initJuceDevicesIfNeeded()
{
    static AudioDeviceManager sDeviceManager;
    static bool needsInit = true;

    if (! needsInit)
        return;

    needsInit = false;
    new JuceCleanup();

    sDeviceManager.createAudioDeviceTypes(gJuceDeviceTypes);

    // remove JACK from device list
    for (int i=0, count=gJuceDeviceTypes.size(); i < count; ++i)
    {
        if (gJuceDeviceTypes[i]->getTypeName() != "JACK")
            continue;

        gJuceDeviceTypes.remove(i, true);
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Juce Engine

class CarlaEngineJuce : public CarlaEngine,
                        public AudioIODeviceCallback,
                        public MidiInputCallback
{
public:
    CarlaEngineJuce(AudioIODeviceType* const devType)
        : CarlaEngine(),
          AudioIODeviceCallback(),
          fDeviceType(devType)
    {
        carla_debug("CarlaEngineJuce::CarlaEngineJuce(%p)", devType);

        // just to make sure
        pData->options.transportMode = ENGINE_TRANSPORT_MODE_INTERNAL;
    }

    ~CarlaEngineJuce() override
    {
        carla_debug("CarlaEngineJuce::~CarlaEngineJuce()");
    }

    // -------------------------------------

    bool init(const char* const clientName) override
    {
        CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
        carla_debug("CarlaEngineJuce::init(\"%s\")", clientName);

        if (pData->options.processMode != ENGINE_PROCESS_MODE_CONTINUOUS_RACK && pData->options.processMode != ENGINE_PROCESS_MODE_PATCHBAY)
        {
            setLastError("Invalid process mode");
            return false;
        }

        String deviceName;

        if (pData->options.audioDevice != nullptr && pData->options.audioDevice[0] != '\0')
        {
            deviceName = pData->options.audioDevice;
        }
        else
        {
            const int   defaultIndex(fDeviceType->getDefaultDeviceIndex(false));
            StringArray deviceNames(fDeviceType->getDeviceNames());

            if (defaultIndex >= 0 && defaultIndex < deviceNames.size())
                deviceName = deviceNames[defaultIndex];
        }

        if (deviceName.isEmpty())
        {
            setLastError("Audio device has not been selected yet and a default one is not available");
            return false;
        }

        fDevice = fDeviceType->createDevice(deviceName, deviceName);

        if (fDevice == nullptr)
        {
            setLastError("Failed to create device");
            return false;
        }

        StringArray inputNames(fDevice->getInputChannelNames());
        StringArray outputNames(fDevice->getOutputChannelNames());

        BigInteger inputChannels;
        inputChannels.setRange(0, inputNames.size(), true);

        BigInteger outputChannels;
        outputChannels.setRange(0, outputNames.size(), true);

        String error = fDevice->open(inputChannels, outputChannels, pData->options.audioSampleRate, static_cast<int>(pData->options.audioBufferSize));

        if (error.isNotEmpty())
        {
            setLastError(error.toUTF8());
            fDevice = nullptr;
            return false;
        }

        pData->bufferSize = static_cast<uint32_t>(fDevice->getCurrentBufferSizeSamples());
        pData->sampleRate = fDevice->getCurrentSampleRate();

        if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        {
            pData->audio.inCount  = 2;
            pData->audio.outCount = 2;
        }
        else
        {
            pData->audio.inCount  = 0;
            pData->audio.outCount = 0;
        }

        pData->audio.create(pData->bufferSize);

        fDevice->start(this);

        CarlaEngine::init(clientName);
        pData->audio.isReady = true;

        patchbayRefresh();

        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEngineJuce::close()");

        pData->audio.isReady = false;

        bool hasError = !CarlaEngine::close();

        if (fDevice != nullptr)
        {
            if (fDevice->isPlaying())
                fDevice->stop();

            if (fDevice->isOpen())
                fDevice->close();

            fDevice = nullptr;
        }

        for (LinkedList<MidiInPort>::Itenerator it = fMidiIns.begin(); it.valid(); it.next())
        {
            MidiInPort& inPort(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(inPort.port != nullptr);

            inPort.port->stop();
            delete inPort.port;
        }

        fMidiIns.clear();
        fMidiInEvents.clear();

        fMidiOutMutex.lock();

        for (LinkedList<MidiOutPort>::Itenerator it = fMidiOuts.begin(); it.valid(); it.next())
        {
            MidiOutPort& outPort(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(outPort.port != nullptr);

            outPort.port->stopBackgroundThread();
            delete outPort.port;
        }

        fMidiOuts.clear();
        fMidiOutMutex.unlock();

        return !hasError;
    }

    bool isRunning() const noexcept override
    {
        return fDevice != nullptr && fDevice->isPlaying();
    }

    bool isOffline() const noexcept override
    {
        return false;
    }

    EngineType getType() const noexcept override
    {
        return kEngineTypeJuce;
    }

    const char* getCurrentDriverName() const noexcept override
    {
        return fDeviceType->getTypeName().toRawUTF8();
    }

    // -------------------------------------------------------------------
    // Patchbay

    bool patchbayRefresh() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->audio.isReady, false);

        //fUsedMidiPorts.clear();

        if (pData->graph.isRack)
            patchbayRefreshRack();
        else
            patchbayRefreshPatchbay();

        return true;
    }

    void patchbayRefreshRack()
    {
        RackGraph* const rack(pData->graph.rack);

        rack->connections.clear();

        char strBuf[STR_MAX+1];
        strBuf[STR_MAX] = '\0';

        // Main
        {
            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_GRAPH_GROUP_CARLA, PATCHBAY_ICON_CARLA, -1, 0.0f, getName());

            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_IN1,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in1");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_IN2,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in2");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_OUT1, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out1");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_OUT2, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out2");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_MIDI_IN,    PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT,  0.0f, "midi-in");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_MIDI_OUT,   PATCHBAY_PORT_TYPE_MIDI,                         0.0f, "midi-out");
        }

        String deviceName(fDevice->getName());

        if (deviceName.isNotEmpty())
            deviceName = deviceName.dropLastCharacters(deviceName.fromFirstOccurrenceOf(", ", true, false).length());

        // Audio In
        {
            StringArray inputNames(fDevice->getInputChannelNames());

            if (deviceName.isNotEmpty())
                std::snprintf(strBuf, STR_MAX, "Capture (%s)", deviceName.toRawUTF8());
            else
                std::strncpy(strBuf, "Capture", STR_MAX);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_GRAPH_GROUP_AUDIO_IN, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

            for (int i=0, count=inputNames.size(); i<count; ++i)
                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_AUDIO_IN, static_cast<int>(i), PATCHBAY_PORT_TYPE_AUDIO, 0.0f, inputNames[i].toRawUTF8());
        }

        // Audio Out
        {
            StringArray outputNames(fDevice->getOutputChannelNames());

            if (deviceName.isNotEmpty())
                std::snprintf(strBuf, STR_MAX, "Playback (%s)", deviceName.toRawUTF8());
            else
                std::strncpy(strBuf, "Playback", STR_MAX);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_GRAPH_GROUP_AUDIO_OUT, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

            for (int i=0, count=outputNames.size(); i<count; ++i)
                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_AUDIO_OUT, static_cast<int>(i), PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, outputNames[i].toRawUTF8());
        }

        // MIDI In
        {
            StringArray midiIns(MidiInput::getDevices());

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_GRAPH_GROUP_MIDI_IN, PATCHBAY_ICON_HARDWARE, -1, 0.0f, "Readable MIDI ports");

            for (int i=0, count=midiIns.size(); i<count; ++i)
            {
                String portName(midiIns[i]);

                std::snprintf(strBuf, STR_MAX, "Readable MIDI ports:%s", portName.toRawUTF8());

                PortNameToId portNameToId;
                portNameToId.setData(RACK_GRAPH_GROUP_MIDI_IN, static_cast<uint>(i), portName.toRawUTF8(), strBuf);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, portNameToId.group, static_cast<int>(portNameToId.port), PATCHBAY_PORT_TYPE_MIDI, 0.0f, portNameToId.name);

                rack->midi.ins.append(portNameToId);
            }
        }

        // MIDI Out
        {
            StringArray midiOuts(MidiOutput::getDevices());

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_GRAPH_GROUP_MIDI_OUT, PATCHBAY_ICON_HARDWARE, -1, 0.0f, "Writable MIDI ports");

            for (int i=0, count=midiOuts.size(); i<count; ++i)
            {
                String portName(midiOuts[i]);

                std::snprintf(strBuf, STR_MAX, "Writable MIDI ports:%s", portName.toRawUTF8());

                PortNameToId portNameToId;
                portNameToId.setData(RACK_GRAPH_GROUP_MIDI_OUT, static_cast<uint>(i), portName.toRawUTF8(), strBuf);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, portNameToId.group, static_cast<int>(portNameToId.port), PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT, 0.0f, portNameToId.name);

                rack->midi.outs.append(portNameToId);
            }
        }

        // Connections
        rack->audio.mutex.lock();

        for (LinkedList<uint>::Itenerator it = rack->audio.connectedIn1.begin(); it.valid(); it.next())
        {
            const uint& portId(it.getValue());
            //CARLA_SAFE_ASSERT_CONTINUE(portId < fAudioInCount);

            ConnectionToId connectionToId;
            connectionToId.setData(++(rack->connections.lastId), RACK_GRAPH_GROUP_AUDIO_IN, portId, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_IN1);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

            rack->connections.list.append(connectionToId);
        }

        for (LinkedList<uint>::Itenerator it = rack->audio.connectedIn2.begin(); it.valid(); it.next())
        {
            const uint& portId(it.getValue());
            //CARLA_SAFE_ASSERT_CONTINUE(portId < fAudioInCount);

            ConnectionToId connectionToId;
            connectionToId.setData(++(rack->connections.lastId), RACK_GRAPH_GROUP_AUDIO_IN, portId, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_IN2);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

            rack->connections.list.append(connectionToId);
        }

        for (LinkedList<uint>::Itenerator it = rack->audio.connectedOut1.begin(); it.valid(); it.next())
        {
            const uint& portId(it.getValue());
            //CARLA_SAFE_ASSERT_CONTINUE(portId < fAudioOutCount);

            ConnectionToId connectionToId;
            connectionToId.setData(++(rack->connections.lastId), RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_OUT1, RACK_GRAPH_GROUP_AUDIO_OUT, portId);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

            rack->connections.list.append(connectionToId);
        }

        for (LinkedList<uint>::Itenerator it = rack->audio.connectedOut2.begin(); it.valid(); it.next())
        {
            const uint& portId(it.getValue());
            //CARLA_SAFE_ASSERT_CONTINUE(portId < fAudioOutCount);

            ConnectionToId connectionToId;
            connectionToId.setData(++(rack->connections.lastId), RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_OUT2, RACK_GRAPH_GROUP_AUDIO_OUT, portId);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

            rack->connections.list.append(connectionToId);
        }

        rack->audio.mutex.unlock();

        for (LinkedList<MidiInPort>::Itenerator it=fMidiIns.begin(); it.valid(); it.next())
        {
            const MidiInPort& inPort(it.getValue());

            const uint portId(rack->midi.getPortId(true, inPort.name));
            CARLA_SAFE_ASSERT_CONTINUE(portId < rack->midi.ins.count());

            ConnectionToId connectionToId;
            connectionToId.setData(++(rack->connections.lastId), RACK_GRAPH_GROUP_MIDI_IN, portId, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_MIDI_IN);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

            rack->connections.list.append(connectionToId);
        }

        fMidiOutMutex.lock();

        for (LinkedList<MidiOutPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
        {
            const MidiOutPort& outPort(it.getValue());

            const uint portId(rack->midi.getPortId(false, outPort.name));
            CARLA_SAFE_ASSERT_CONTINUE(portId < rack->midi.outs.count());

            ConnectionToId connectionToId;
            connectionToId.setData(++(rack->connections.lastId), RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_MIDI_OUT, RACK_GRAPH_GROUP_MIDI_OUT, portId);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

            rack->connections.list.append(connectionToId);
        }

        fMidiOutMutex.unlock();
    }

    void patchbayRefreshPatchbay() noexcept
    {
    }

    // -------------------------------------------------------------------

protected:
    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples) override
    {
        // assert juce buffers
        CARLA_SAFE_ASSERT_RETURN(numInputChannels >= 0,        runPendingRtEvents());
        CARLA_SAFE_ASSERT_RETURN(numOutputChannels > 0,        runPendingRtEvents());
        CARLA_SAFE_ASSERT_RETURN(outputChannelData != nullptr, runPendingRtEvents());
        CARLA_SAFE_ASSERT_RETURN(numSamples == static_cast<int>(pData->bufferSize), runPendingRtEvents());

        if (! pData->audio.isReady)
            return runPendingRtEvents();

        const uint32_t nframes(static_cast<uint32_t>(numSamples));

        // initialize juce output
        for (int i=0; i < numOutputChannels; ++i)
            FloatVectorOperations::clear(outputChannelData[i], numSamples);

        // initialize input events
        carla_zeroStruct<EngineEvent>(pData->events.in,  kMaxEngineEventInternalCount);
        carla_zeroStruct<EngineEvent>(pData->events.out, kMaxEngineEventInternalCount);

        if (fMidiInEvents.mutex.tryLock())
        {
            uint32_t engineEventIndex = 0;
            fMidiInEvents.splice();

            for (LinkedList<RtMidiEvent>::Itenerator it = fMidiInEvents.data.begin(); it.valid(); it.next())
            {
                const RtMidiEvent& midiEvent(it.getValue());
                EngineEvent&       engineEvent(pData->events.in[engineEventIndex++]);

                if (midiEvent.time < pData->timeInfo.frame)
                {
                    engineEvent.time = 0;
                }
                else if (midiEvent.time >= pData->timeInfo.frame + nframes)
                {
                    carla_stderr("MIDI Event in the future!, %i vs %i", engineEvent.time, pData->timeInfo.frame);
                    engineEvent.time = static_cast<uint32_t>(pData->timeInfo.frame) + nframes - 1;
                }
                else
                    engineEvent.time = static_cast<uint32_t>(midiEvent.time - pData->timeInfo.frame);

                engineEvent.fillFromMidiData(midiEvent.size, midiEvent.data);

                if (engineEventIndex >= kMaxEngineEventInternalCount)
                    break;
            }

            fMidiInEvents.data.clear();
            fMidiInEvents.mutex.unlock();
        }

        if (pData->graph.isRack)
        {
            pData->processRackFull(inputChannelData, static_cast<uint32_t>(numInputChannels),
                                   outputChannelData, static_cast<uint32_t>(numOutputChannels),
                                   static_cast<uint32_t>(numSamples), false);
        }
        else
        {
        }

        fMidiOutMutex.lock();

        if (fMidiOuts.count() > 0)
        {
            uint8_t        size    = 0;
            uint8_t        data[3] = { 0, 0, 0 };
            const uint8_t* dataPtr = data;

            for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
            {
                const EngineEvent& engineEvent(pData->events.out[i]);

                if (engineEvent.type == kEngineEventTypeNull)
                    break;

                else if (engineEvent.type == kEngineEventTypeControl)
                {
                    const EngineControlEvent& ctrlEvent(engineEvent.ctrl);
                    ctrlEvent.convertToMidiData(engineEvent.channel, size, data);
                    dataPtr = data;
                }
                else if (engineEvent.type == kEngineEventTypeMidi)
                {
                    const EngineMidiEvent& midiEvent(engineEvent.midi);

                    size = midiEvent.size;

                    if (size > EngineMidiEvent::kDataSize && midiEvent.dataExt != nullptr)
                        dataPtr = midiEvent.dataExt;
                    else
                        dataPtr = midiEvent.data;
                }
                else
                {
                    continue;
                }

                if (size > 0)
                {
                    MidiMessage message(static_cast<const void*>(dataPtr), static_cast<int>(size), static_cast<double>(engineEvent.time)/nframes);

                    for (LinkedList<MidiOutPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
                    {
                        MidiOutPort& outPort(it.getValue());
                        CARLA_SAFE_ASSERT_CONTINUE(outPort.port != nullptr);

                        outPort.port->sendMessageNow(message);
                    }
                }
            }
        }

        fMidiOutMutex.unlock();

        runPendingRtEvents();
        return;
    }

    void audioDeviceAboutToStart(AudioIODevice* /*device*/) override
    {
    }

    void audioDeviceStopped() override
    {
    }

    void audioDeviceError(const String& errorMessage) override
    {
        callback(ENGINE_CALLBACK_ERROR, 0, 0, 0, 0.0f, errorMessage.toRawUTF8());
    }

    // -------------------------------------------------------------------

    void handleIncomingMidiMessage(MidiInput* /*source*/, const MidiMessage& message) override
    {
        if (! pData->audio.isReady)
            return;

        const int messageSize(message.getRawDataSize());

        if (messageSize <= 0 || messageSize > EngineMidiEvent::kDataSize)
            return;

        const uint8_t* const messageData(message.getRawData());

        RtMidiEvent midiEvent;
        midiEvent.time = 0; // TODO

        midiEvent.size = static_cast<uint8_t>(messageSize);

        int i=0;
        for (; i < messageSize; ++i)
            midiEvent.data[i] = messageData[i];
        for (; i < EngineMidiEvent::kDataSize; ++i)
            midiEvent.data[i] = 0;

        fMidiInEvents.append(midiEvent);
    }

    // -------------------------------------------------------------------

    bool connectRackMidiInPort(const char* const portName) override
    {
        CARLA_SAFE_ASSERT_RETURN(portName != nullptr && portName[0] != '\0', false);
        carla_debug("CarlaEngineJuce::connectRackMidiInPort(\"%s\")", portName);

        RackGraph* const rack(pData->graph.rack);
        CARLA_SAFE_ASSERT_RETURN(rack->midi.ins.count() > 0, false);

        StringArray midiIns(MidiInput::getDevices());

        if (! midiIns.contains(portName))
            return false;

        MidiInput* const juceMidiIn(MidiInput::openDevice(midiIns.indexOf(portName), this));
        juceMidiIn->start();

        MidiInPort midiPort;
        midiPort.port = juceMidiIn;

        std::strncpy(midiPort.name, portName, STR_MAX);
        midiPort.name[STR_MAX] = '\0';

        fMidiIns.append(midiPort);
        return true;
    }

    bool connectRackMidiOutPort(const char* const portName) override
    {
        CARLA_SAFE_ASSERT_RETURN(portName != nullptr && portName[0] != '\0', false);
        carla_debug("CarlaEngineJuce::connectRackMidiOutPort(\"%s\")", portName);

        RackGraph* const rack(pData->graph.rack);
        CARLA_SAFE_ASSERT_RETURN(rack->midi.ins.count() > 0, false);

        StringArray midiOuts(MidiOutput::getDevices());

        if (! midiOuts.contains(portName))
            return false;

        MidiOutput* const juceMidiOut(MidiOutput::openDevice(midiOuts.indexOf(portName)));
        juceMidiOut->startBackgroundThread();

        MidiOutPort midiPort;
        midiPort.port = juceMidiOut;

        std::strncpy(midiPort.name, portName, STR_MAX);
        midiPort.name[STR_MAX] = '\0';

        const CarlaMutexLocker cml(fMidiOutMutex);

        fMidiOuts.append(midiPort);
        return true;
    }

    bool disconnectRackMidiInPort(const char* const portName) override
    {
        CARLA_SAFE_ASSERT_RETURN(portName != nullptr && portName[0] != '\0', false);
        carla_debug("CarlaEngineRtAudio::disconnectRackMidiInPort(\"%s\")", portName);

        RackGraph* const rack(pData->graph.rack);
        CARLA_SAFE_ASSERT_RETURN(rack->midi.ins.count() > 0, false);

        for (LinkedList<MidiInPort>::Itenerator it=fMidiIns.begin(); it.valid(); it.next())
        {
            MidiInPort& inPort(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(inPort.port != nullptr);

            if (std::strcmp(inPort.name, portName) != 0)
                continue;

            inPort.port->stop();
            delete inPort.port;

            fMidiIns.remove(it);
            return true;
        }

        return false;
    }

    bool disconnectRackMidiOutPort(const char* const portName) override
    {
        CARLA_SAFE_ASSERT_RETURN(portName != nullptr && portName[0] != '\0', false);
        carla_debug("CarlaEngineRtAudio::disconnectRackMidiOutPort(\"%s\")", portName);

        RackGraph* const rack(pData->graph.rack);
        CARLA_SAFE_ASSERT_RETURN(rack->midi.outs.count() > 0, false);

        const CarlaMutexLocker cml(fMidiOutMutex);

        for (LinkedList<MidiOutPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
        {
            MidiOutPort& outPort(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(outPort.port != nullptr);

            if (std::strcmp(outPort.name, portName) != 0)
                continue;

            outPort.port->stopBackgroundThread();
            delete outPort.port;

            fMidiOuts.remove(it);
            return true;
        }

        return false;
    }

    // -------------------------------------

private:
    ScopedPointer<AudioIODevice> fDevice;
    AudioIODeviceType* const     fDeviceType;

    struct MidiInPort {
        MidiInput* port;
        char name[STR_MAX+1];
    };

    struct MidiOutPort {
        MidiOutput* port;
        char name[STR_MAX+1];
    };

    struct RtMidiEvent {
        uint64_t time; // needs to compare to internal time
        uint8_t  size;
        uint8_t  data[EngineMidiEvent::kDataSize];
    };

    struct RtMidiEvents {
        CarlaMutex mutex;
        RtLinkedList<RtMidiEvent>::Pool dataPool;
        RtLinkedList<RtMidiEvent> data;
        RtLinkedList<RtMidiEvent> dataPending;

        RtMidiEvents()
            : dataPool(512, 512),
              data(dataPool),
              dataPending(dataPool) {}

        ~RtMidiEvents()
        {
            clear();
        }

        void append(const RtMidiEvent& event)
        {
            mutex.lock();
            dataPending.append(event);
            mutex.unlock();
        }

        void clear()
        {
            mutex.lock();
            data.clear();
            dataPending.clear();
            mutex.unlock();
        }

        void splice()
        {
            dataPending.spliceAppendTo(data);
        }
    };

    LinkedList<MidiInPort> fMidiIns;
    RtMidiEvents           fMidiInEvents;

    LinkedList<MidiOutPort> fMidiOuts;
    CarlaMutex              fMidiOutMutex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJuce)
};

// -----------------------------------------

CarlaEngine* CarlaEngine::newJuce(const AudioApi api)
{
    initJuceDevicesIfNeeded();

    String juceApi;

    switch (api)
    {
    case AUDIO_API_NULL:
    case AUDIO_API_OSS:
    case AUDIO_API_PULSE:
        break;
    case AUDIO_API_JACK:
        juceApi = "JACK";
        break;
    case AUDIO_API_ALSA:
        juceApi = "ALSA";
        break;
    case AUDIO_API_CORE:
        juceApi = "CoreAudio";
        break;
    case AUDIO_API_ASIO:
        juceApi = "ASIO";
        break;
    case AUDIO_API_DS:
        juceApi = "DirectSound";
        break;
    }

    if (juceApi.isEmpty())
        return nullptr;

    AudioIODeviceType* deviceType = nullptr;

    for (int i=0, count=gJuceDeviceTypes.size(); i < count; ++i)
    {
        deviceType = gJuceDeviceTypes[i];

        if (deviceType == nullptr || deviceType->getTypeName() == juceApi)
            break;
    }

    if (deviceType == nullptr)
        return nullptr;

    deviceType->scanForDevices();

    return new CarlaEngineJuce(deviceType);
}

uint CarlaEngine::getJuceApiCount()
{
    initJuceDevicesIfNeeded();

    return static_cast<uint>(gJuceDeviceTypes.size());
}

const char* CarlaEngine::getJuceApiName(const uint uindex)
{
    initJuceDevicesIfNeeded();

    const int index(static_cast<int>(uindex));

    CARLA_SAFE_ASSERT_RETURN(index < gJuceDeviceTypes.size(), nullptr);

    AudioIODeviceType* const deviceType(gJuceDeviceTypes[index]);
    CARLA_SAFE_ASSERT_RETURN(deviceType != nullptr, nullptr);

    return deviceType->getTypeName().toRawUTF8();
}

const char* const* CarlaEngine::getJuceApiDeviceNames(const uint uindex)
{
    initJuceDevicesIfNeeded();

    const int index(static_cast<int>(uindex));

    CARLA_SAFE_ASSERT_RETURN(index < gJuceDeviceTypes.size(), nullptr);

    AudioIODeviceType* const deviceType(gJuceDeviceTypes[index]);
    CARLA_SAFE_ASSERT_RETURN(deviceType != nullptr, nullptr);

    deviceType->scanForDevices();

    StringArray deviceNames(deviceType->getDeviceNames());
    const int   deviceNameCount(deviceNames.size());

    if (deviceNameCount <= 0)
        return nullptr;

    if (gRetNames != nullptr)
    {
        for (int i=0; gRetNames[i] != nullptr; ++i)
            delete[] gRetNames[i];
        delete[] gRetNames;
    }

    gRetNames = new const char*[deviceNameCount+1];

    for (int i=0; i < deviceNameCount; ++i)
        gRetNames[i] = carla_strdup(deviceNames[i].toRawUTF8());

    gRetNames[deviceNameCount] = nullptr;

    return gRetNames;
}

const EngineDriverDeviceInfo* CarlaEngine::getJuceDeviceInfo(const uint uindex, const char* const deviceName)
{
    initJuceDevicesIfNeeded();

    const int index(static_cast<int>(uindex));

    CARLA_SAFE_ASSERT_RETURN(index < gJuceDeviceTypes.size(), nullptr);

    AudioIODeviceType* const deviceType(gJuceDeviceTypes[index]);
    CARLA_SAFE_ASSERT_RETURN(deviceType != nullptr, nullptr);

    deviceType->scanForDevices();

    ScopedPointer<AudioIODevice> device(deviceType->createDevice(deviceName, deviceName));

    if (device == nullptr)
        return nullptr;

    static EngineDriverDeviceInfo devInfo = { 0x0, nullptr, nullptr };
    static uint32_t dummyBufferSizes[11]  = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 0 };
    static double   dummySampleRates[14]  = { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0, 0.0 };

    // reset
    devInfo.hints = ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE | ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE;

    // cleanup
    if (devInfo.bufferSizes != nullptr && devInfo.bufferSizes != dummyBufferSizes)
    {
        delete[] devInfo.bufferSizes;
        devInfo.bufferSizes = nullptr;
    }

    if (devInfo.sampleRates != nullptr && devInfo.sampleRates != dummySampleRates)
    {
        delete[] devInfo.sampleRates;
        devInfo.sampleRates = nullptr;
    }

    if (device->hasControlPanel())
        devInfo.hints |= ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL;

    Array<int> juceBufferSizes = device->getAvailableBufferSizes();
    if (int bufferSizesCount = juceBufferSizes.size())
    {
        uint32_t* const bufferSizes(new uint32_t[bufferSizesCount+1]);

        for (int i=0; i < bufferSizesCount; ++i)
            bufferSizes[i] = static_cast<uint32_t>(juceBufferSizes[i]);
        bufferSizes[bufferSizesCount] = 0;

        devInfo.bufferSizes = bufferSizes;
    }
    else
    {
        devInfo.bufferSizes = dummyBufferSizes;
    }

    Array<double> juceSampleRates = device->getAvailableSampleRates();
    if (int sampleRatesCount = juceSampleRates.size())
    {
        double* const sampleRates(new double[sampleRatesCount+1]);

        for (int i=0; i < sampleRatesCount; ++i)
            sampleRates[i] = juceSampleRates[i];
        sampleRates[sampleRatesCount] = 0.0;

        devInfo.sampleRates = sampleRates;
    }
    else
    {
        devInfo.sampleRates = dummySampleRates;
    }

    return &devInfo;
}

// -----------------------------------------

CARLA_BACKEND_END_NAMESPACE
