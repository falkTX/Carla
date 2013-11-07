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

#include "CarlaEngineInternal.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaMIDI.h"
#include "RtList.hpp"

#include "juce_audio_basics.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------------------------------------------------------
// Juce Engine

static const char** gRetNames = nullptr;

class CarlaEngineJuce : public CarlaEngine/*,
                        public juce::AudioIODeviceCallback*/
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineJuce)
};

// -----------------------------------------

CarlaEngine* CarlaEngine::newJuce(const AudioApi /*api*/)
{
    return new CarlaEngineJuce();
}

size_t CarlaEngine::getJuceApiCount()
{
    return 0;
}

const char* CarlaEngine::getJuceApiName(const unsigned int /*index*/)
{
    return nullptr;
}

const char** CarlaEngine::getJuceApiDeviceNames(const unsigned int /*index*/)
{
#if 0
    juce::ScopedPointer<juce::AudioIODeviceType> deviceType;

    switch(index)
    {
    case 0:
        deviceType = juce::AudioIODeviceType::createAudioIODeviceType_JACK();
        break;
    default:
        //setLastError("");
        return nullptr;
    }

    if (deviceType == nullptr)
    {
        //setLastError("");
        return nullptr;
    }

    deviceType->scanForDevices();

    const juce::StringArray devNames(deviceType->getDeviceNames());
    const int devNameCount(devNames.size());

    if (devNameCount <= 0)
        return nullptr;

    gRetNames = new const char*[devNameCount+1];

    for (unsigned int i=0; i < devNameCount; ++i)
        gRetNames[i] = carla_strdup(devNames[i].toRawUTF8());

    gRetNames[devNameCount] = nullptr;
#endif

    return gRetNames;
}

// -----------------------------------------

CARLA_BACKEND_END_NAMESPACE
