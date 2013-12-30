/*
 * Carla Juce Engine
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaMIDI.h"

// #include "RtList.hpp"

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

class CarlaEngineJuce : public CarlaEngine/*,
                        public AudioIODeviceCallback*/
{
public:
    CarlaEngineJuce()
        : CarlaEngine()
    {
        carla_debug("CarlaEngineJuce::CarlaEngineJuce()");
    }

    ~CarlaEngineJuce() override
    {
        if (gRetNames != nullptr)
        {
            delete[] gRetNames;
            gRetNames = nullptr;
        }
    }

    // -------------------------------------

    bool init(const char* const clientName) override
    {
        carla_debug("CarlaEngineJuce::init(\"%s\")", clientName);

        CarlaEngine::init(clientName);
        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEngineJuce::close()");

        return CarlaEngine::close();
    }

    bool isRunning() const noexcept override
    {
        return false;
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
        return nullptr;
    }

    // -------------------------------------------------------------------

protected:
//     void audioDeviceIOCallback (const float** inputChannelData,
//                                 int numInputChannels,
//                                 float** outputChannelData,
//                                 int numOutputChannels,
//                                 int numSamples)
//     {
//     }
//
//     void audioDeviceAboutToStart (juce::AudioIODevice* device)
//     {
//     }
//
//     void audioDeviceStopped()
//     {
//     }
//
//     void audioDeviceError (const juce::String& errorMessage)
//     {
//     }

    // -------------------------------------

private:
    //juce::AudioIODeviceType* fDeviceType;

    //JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJuce)
};

// -----------------------------------------

CarlaEngine* CarlaEngine::newJuce(const AudioApi /*api*/)
{
    return new CarlaEngineJuce();
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

    AudioIODeviceType* const deviceType(gJuceDeviceTypes[index]);

    if (deviceType == nullptr)
        return nullptr;

    return deviceType->getTypeName().toRawUTF8();
}

const char* const* CarlaEngine::getJuceApiDeviceNames(const unsigned int index)
{
    initJuceDevices();

    if (static_cast<int>(index) >= gJuceDeviceTypes.size())
        return nullptr;

    AudioIODeviceType* const deviceType(gJuceDeviceTypes[index]);

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
        return nullptr;

    AudioIODeviceType* const deviceType(gJuceDeviceTypes[index]);

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
            bufferSizes[i] = device->getBufferSizeSamples(i);
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
