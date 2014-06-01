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
// Cleanup

static struct JuceCleanup {
    JuceCleanup() {}
    ~JuceCleanup()
    {
        if (gRetNames != nullptr)
        {
            delete[] gRetNames;
            gRetNames = nullptr;
        }

        gJuceDeviceTypes.clear(true);
    }
} sJuceCleanup;

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
            fDevice = nullptr;
            setLastError(error.toUTF8());
            return false;
        }

        pData->bufferSize = static_cast<uint32_t>(fDevice->getCurrentBufferSizeSamples());
        pData->sampleRate = fDevice->getCurrentSampleRate();

        pData->audio.inCount  = static_cast<uint32_t>(inputChannels.countNumberOfSetBits());
        pData->audio.outCount = static_cast<uint32_t>(outputChannels.countNumberOfSetBits());

        CARLA_SAFE_ASSERT(pData->audio.outCount > 0);

        pData->audio.create(pData->bufferSize);

        fDevice->start(this);

        CarlaEngine::init(clientName);
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
        // const String& deviceName(fDevice->getName());
        return true;
    }

    // -------------------------------------------------------------------

protected:
    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels, float** outputChannelData, int numOutputChannels, int numSamples) override
    {
        // assert juce buffers
        CARLA_SAFE_ASSERT_RETURN(numInputChannels == static_cast<int>(pData->audio.inCount),);
        CARLA_SAFE_ASSERT_RETURN(numOutputChannels == static_cast<int>(pData->audio.outCount),);
        CARLA_SAFE_ASSERT_RETURN(outputChannelData != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(numSamples == static_cast<int>(pData->bufferSize),);

        if (numOutputChannels == 0 || ! pData->audio.isReady)
            return runPendingRtEvents();

        // initialize input events
        carla_zeroStruct<EngineEvent>(pData->events.in, kMaxEngineEventInternalCount);

        // TODO - get events from juce

        if (pData->graph.isRack)
        {
            pData->processRackFull(const_cast<float**>(inputChannelData), static_cast<uint32_t>(numInputChannels),
                                   outputChannelData, static_cast<uint32_t>(numOutputChannels),
                                   static_cast<uint32_t>(numSamples), false);
        }
        else
        {
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

uint CarlaEngine::getJuceApiCount()
{
    return 0; // TODO

    initJuceDevices();

    return static_cast<uint>(gJuceDeviceTypes.size());
}

const char* CarlaEngine::getJuceApiName(const uint index)
{
    initJuceDevices();

    if (static_cast<int>(index) >= gJuceDeviceTypes.size())
        return nullptr;

    AudioIODeviceType* const deviceType(gJuceDeviceTypes[static_cast<int>(index)]);

    if (deviceType == nullptr)
        return nullptr;

    return deviceType->getTypeName().toRawUTF8();
}

const char* const* CarlaEngine::getJuceApiDeviceNames(const uint index)
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

const EngineDriverDeviceInfo* CarlaEngine::getJuceDeviceInfo(const uint index, const char* const deviceName)
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
