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

// #include "RtLinkedList.hpp"

#include "juce_audio_devices.h"

using namespace juce;

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------------------------------------------------------

static const char** gRetNames = nullptr;
static OwnedArray<AudioIODeviceType> gJuceDeviceTypes;

static void initJuceDevices()
{
    static AudioDeviceManager manager;

    if (gJuceDeviceTypes.size() == 0)
        manager.createAudioDeviceTypes(gJuceDeviceTypes);
}

// -------------------------------------------------------------------------------------------------------------------
// Juce Engine

class CarlaEngineJuce : public CarlaEngine,
                        public AudioIODeviceCallback
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

        if (gRetNames != nullptr)
        {
            delete[] gRetNames;
            gRetNames = nullptr;
        }

        gJuceDeviceTypes.clear(true);
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

        pData->bufAudio.usePatchbay = (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY);

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
            fDevice = nullptr;
            setLastError(error.toUTF8());
            return false;
        }

        pData->bufferSize = static_cast<uint32_t>(fDevice->getCurrentBufferSizeSamples());
        pData->sampleRate = fDevice->getCurrentSampleRate();

        pData->bufAudio.inCount  = static_cast<uint32_t>(inputChannels.countNumberOfSetBits());
        pData->bufAudio.outCount = static_cast<uint32_t>(outputChannels.countNumberOfSetBits());

        CARLA_ASSERT(pData->bufAudio.outCount > 0);

        pData->bufAudio.create(pData->bufferSize);

        fDevice->start(this);

        CarlaEngine::init(clientName);
        patchbayRefresh();

        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEngineJuce::close()");

        pData->bufAudio.isReady = false;

        bool hasError = !CarlaEngine::close();

        if (fDevice != nullptr)
        {
            if (fDevice->isPlaying())
                fDevice->stop();

            if (fDevice->isOpen())
                fDevice->close();

            fDevice = nullptr;
        }

        pData->bufAudio.clear();

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
        CARLA_SAFE_ASSERT_RETURN(pData->bufAudio.isReady, false);

        pData->bufAudio.initPatchbay();

        if (pData->bufAudio.usePatchbay)
        {
            // not implemented yet
            return false;
        }

        char strBuf[STR_MAX+1];
        strBuf[STR_MAX] = '\0';

        //EngineRackBuffers* const rack(pData->bufAudio.rack);

        // Main
        {
            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_CARLA, PATCHBAY_ICON_CARLA, -1, 0.0f, getName());

            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_PORT_AUDIO_IN1,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in1");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_PORT_AUDIO_IN2,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in2");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_PORT_AUDIO_OUT1, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out1");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_PORT_AUDIO_OUT2, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out2");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_PORT_MIDI_IN,    PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT,  0.0f, "midi-in");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_PORT_MIDI_OUT,   PATCHBAY_PORT_TYPE_MIDI,                         0.0f, "midi-out");
        }

        const String& deviceName(fDevice->getName());

        // Audio In
        {
            if (deviceName.isNotEmpty())
                std::snprintf(strBuf, STR_MAX, "Capture (%s)", deviceName.toRawUTF8());
            else
                std::strncpy(strBuf, "Capture", STR_MAX);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_AUDIO_IN, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

            StringArray inputNames(fDevice->getInputChannelNames());
            CARLA_ASSERT(inputNames.size() == static_cast<int>(pData->bufAudio.inCount));

            for (uint i=0; i < pData->bufAudio.inCount; ++i)
            {
                String inputName(inputNames[static_cast<int>(i)]);

                if (inputName.trim().isNotEmpty())
                    std::snprintf(strBuf, STR_MAX, "%s", inputName.toRawUTF8());
                else
                    std::snprintf(strBuf, STR_MAX, "capture_%i", i+1);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_AUDIO_IN, static_cast<int>(RACK_PATCHBAY_GROUP_AUDIO_IN*1000 + i), PATCHBAY_PORT_TYPE_AUDIO, 0.0f, strBuf);
            }
        }

        // Audio Out
        {
            if (deviceName.isNotEmpty())
                std::snprintf(strBuf, STR_MAX, "Playback (%s)", deviceName.toRawUTF8());
            else
                std::strncpy(strBuf, "Playback", STR_MAX);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_AUDIO_OUT, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

            StringArray outputNames(fDevice->getOutputChannelNames());
            CARLA_ASSERT(outputNames.size() == static_cast<int>(pData->bufAudio.outCount));

            for (uint i=0; i < pData->bufAudio.outCount; ++i)
            {
                String outputName(outputNames[static_cast<int>(i)]);

                if (outputName.trim().isNotEmpty())
                    std::snprintf(strBuf, STR_MAX, "%s", outputName.toRawUTF8());
                else
                    std::snprintf(strBuf, STR_MAX, "playback_%i", i+1);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_AUDIO_OUT, static_cast<int>(RACK_PATCHBAY_GROUP_AUDIO_OUT*1000 + i), PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, strBuf);
            }
        }

#if 0 // midi implemented yet
        // MIDI In
        {
            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_MIDI_IN, PATCHBAY_ICON_HARDWARE, -1, 0.0f, "Readable MIDI ports");

            for (unsigned int i=0, count=fDummyMidiIn.getPortCount(); i < count; ++i)
            {
                PortNameToId portNameToId;
                portNameToId.portId = RACK_PATCHBAY_GROUP_MIDI_IN*1000 + i;
                std::strncpy(portNameToId.name, fDummyMidiIn.getPortName(i).c_str(), STR_MAX);
                fUsedMidiIns.append(portNameToId);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_MIDI_IN, portNameToId.portId, PATCHBAY_PORT_TYPE_MIDI, 0.0f, portNameToId.name);
            }
        }

        // MIDI Out
        {
            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_MIDI_OUT, PATCHBAY_ICON_HARDWARE, -1, 0.0f, "Writable MIDI ports");

            for (unsigned int i=0, count=fDummyMidiOut.getPortCount(); i < count; ++i)
            {
                PortNameToId portNameToId;
                portNameToId.portId = RACK_PATCHBAY_GROUP_MIDI_OUT*1000 + i;
                std::strncpy(portNameToId.name, fDummyMidiOut.getPortName(i).c_str(), STR_MAX);
                fUsedMidiOuts.append(portNameToId);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, 0, RACK_PATCHBAY_GROUP_MIDI_OUT, portNameToId.portId, PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT, portNameToId.name);
            }
        }
#endif

#if 0
        // Connections
        rack->connectLock.lock();

        for (LinkedList<uint>::Itenerator it = rack->connectedIns[0].begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < pData->bufAudio.inCount);

            ConnectionToId connectionToId;
            connectionToId.id      = rack->lastConnectionId;
            connectionToId.portOut = RACK_PATCHBAY_GROUP_AUDIO_IN*1000 + port;
            connectionToId.portIn  = RACK_PATCHBAY_PORT_AUDIO_IN1;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            rack->usedConnections.append(connectionToId);
            rack->lastConnectionId++;
        }

        for (LinkedList<uint>::Itenerator it = rack->connectedIns[1].begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < pData->bufAudio.inCount);

            ConnectionToId connectionToId;
            connectionToId.id      = rack->lastConnectionId;
            connectionToId.portOut = RACK_PATCHBAY_GROUP_AUDIO_IN*1000 + port;
            connectionToId.portIn  = RACK_PATCHBAY_PORT_AUDIO_IN2;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            rack->usedConnections.append(connectionToId);
            rack->lastConnectionId++;
        }

        for (LinkedList<uint>::Itenerator it = rack->connectedOuts[0].begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < pData->bufAudio.outCount);

            ConnectionToId connectionToId;
            connectionToId.id      = rack->lastConnectionId;
            connectionToId.portOut = RACK_PATCHBAY_PORT_AUDIO_OUT1;
            connectionToId.portIn  = RACK_PATCHBAY_GROUP_AUDIO_OUT*1000 + port;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            rack->usedConnections.append(connectionToId);
            rack->lastConnectionId++;
        }

        for (LinkedList<uint>::Itenerator it = rack->connectedOuts[1].begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < pData->bufAudio.outCount);

            ConnectionToId connectionToId;
            connectionToId.id      = rack->lastConnectionId;
            connectionToId.portOut = RACK_PATCHBAY_PORT_AUDIO_OUT2;
            connectionToId.portIn  = RACK_PATCHBAY_GROUP_AUDIO_OUT*1000 + port;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            rack->usedConnections.append(connectionToId);
            rack->lastConnectionId++;
        }

        pData->bufAudio.rack->connectLock.unlock();

#if 0
        for (LinkedList<MidiPort>::Itenerator it=fMidiIns.begin(); it.valid(); it.next())
        {
            const MidiPort& midiPort(it.getValue());

            ConnectionToId connectionToId;
            connectionToId.id      = rack->lastConnectionId;
            connectionToId.portOut = RACK_PATCHBAY_GROUP_MIDI_IN*1000 + midiPort.portId;
            connectionToId.portIn  = RACK_PATCHBAY_PORT_MIDI_IN;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            rack->usedConnections.append(connectionToId);
            rack->lastConnectionId++;
        }

        for (LinkedList<MidiPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
        {
            const MidiPort& midiPort(it.getValue());

            ConnectionToId connectionToId;
            connectionToId.id      = rack->lastConnectionId;
            connectionToId.portOut = RACK_PATCHBAY_PORT_MIDI_OUT;
            connectionToId.portIn  = RACK_PATCHBAY_GROUP_MIDI_OUT*1000 + midiPort.portId;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            rack->usedConnections.append(connectionToId);
            rack->lastConnectionId++;
        }
#endif
#endif

        return true;
    }

    // -------------------------------------------------------------------

protected:
    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples) override
    {
        // assert juce buffers
        CARLA_SAFE_ASSERT_RETURN(numInputChannels == static_cast<int>(pData->bufAudio.inCount),);
        CARLA_SAFE_ASSERT_RETURN(numOutputChannels == static_cast<int>(pData->bufAudio.outCount),);
        CARLA_SAFE_ASSERT_RETURN(outputChannelData != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(numSamples == static_cast<int>(pData->bufferSize),);

        if (numOutputChannels == 0 || ! pData->bufAudio.isReady)
            return runPendingRtEvents();

        // initialize input events
        carla_zeroStruct<EngineEvent>(pData->bufEvents.in, kMaxEngineEventInternalCount);

        // TODO - get events from juce

        if (pData->bufAudio.usePatchbay)
        {
        }
        else
        {
            pData->processRackFull(const_cast<float**>(inputChannelData), static_cast<uint32_t>(numInputChannels),
                                                       outputChannelData, static_cast<uint32_t>(numOutputChannels),
                                   static_cast<uint32_t>(numSamples), false);
        }

        // output events
        {
            // TODO
            //fMidiOutEvents...
        }

        runPendingRtEvents();
        return;

        // unused
        (void)inputChannelData;
        (void)numInputChannels;
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

    bool connectRackMidiInPort(const int) override
    {
        return false;
    }

    bool connectRackMidiOutPort(const int) override
    {
        return false;
    }

    bool disconnectRackMidiInPort(const int) override
    {
        return false;
    }

    bool disconnectRackMidiOutPort(const int) override
    {
        return false;
    }

    // -------------------------------------

private:
    ScopedPointer<AudioIODevice> fDevice;
    AudioIODeviceType* const     fDeviceType;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJuce)
};

// -----------------------------------------

CarlaEngine* CarlaEngine::newJuce(const AudioApi api)
{
    initJuceDevices();

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

unsigned int CarlaEngine::getJuceApiCount()
{
    initJuceDevices();

    return static_cast<unsigned int>(gJuceDeviceTypes.size());
}

const char* CarlaEngine::getJuceApiName(const unsigned int index)
{
    initJuceDevices();

    if (static_cast<int>(index) >= gJuceDeviceTypes.size())
        return nullptr;

    AudioIODeviceType* const deviceType(gJuceDeviceTypes[static_cast<int>(index)]);

    if (deviceType == nullptr)
        return nullptr;

    return deviceType->getTypeName().toRawUTF8();
}

const char* const* CarlaEngine::getJuceApiDeviceNames(const unsigned int index)
{
    initJuceDevices();

    if (static_cast<int>(index) >= gJuceDeviceTypes.size())
        return nullptr;

    AudioIODeviceType* const deviceType(gJuceDeviceTypes[static_cast<int>(index)]);

    if (deviceType == nullptr)
        return nullptr;

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

const EngineDriverDeviceInfo* CarlaEngine::getJuceDeviceInfo(const unsigned int index, const char* const deviceName)
{
    initJuceDevices();

    if (static_cast<int>(index) >= gJuceDeviceTypes.size())
    {
        carla_stderr("here 001");
        return nullptr;
    }

    AudioIODeviceType* const deviceType(gJuceDeviceTypes[static_cast<int>(index)]);

    if (deviceType == nullptr)
        return nullptr;

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

    if (int bufferSizesCount = device->getNumBufferSizesAvailable())
    {
        uint32_t* const bufferSizes(new uint32_t[bufferSizesCount+1]);

        for (int i=0; i < bufferSizesCount; ++i)
            bufferSizes[i] = static_cast<uint32_t>(device->getBufferSizeSamples(i));
        bufferSizes[bufferSizesCount] = 0;

        devInfo.bufferSizes = bufferSizes;
    }
    else
    {
        devInfo.bufferSizes = dummyBufferSizes;
    }

    if (int sampleRatesCount = device->getNumSampleRates())
    {
        double* const sampleRates(new double[sampleRatesCount+1]);

        for (int i=0; i < sampleRatesCount; ++i)
            sampleRates[i] = device->getSampleRate(i);
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
