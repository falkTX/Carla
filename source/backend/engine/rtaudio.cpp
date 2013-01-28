/*
 * Carla RtAudio Engine
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifdef WANT_RTAUDIO

#include "carla_engine_internal.hpp"
#include "carla_backend_utils.hpp"
#include "carla_midi.h"

#include "RtAudio.h"
#include "RtMidi.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------------------------------------------------------
// RtAudio Engine client

class CarlaEngineRtAudioClient : public CarlaEngineClient
{
public:
    CarlaEngineRtAudioClient(const EngineType engineType, const ProcessMode processMode)
        : CarlaEngineClient(engineType, processMode)
    {
        qDebug("CarlaEngineRtAudioClient::CarlaEngineRtAudioClient(%s, %s)", EngineType2Str(engineType), ProcessMode2Str(processMode));
    }

    ~CarlaEngineRtAudioClient()
    {
        qDebug("CarlaEngineRtAudioClient::~CarlaEngineRtAudioClient()");
    }

    const CarlaEnginePort* addPort(const EnginePortType portType, const char* const name, const bool isInput)
    {
        qDebug("CarlaEngineRtAudioClient::addPort(%s, \"%s\", %s)", EnginePortType2Str(portType), name, bool2str(isInput));

        switch (portType)
        {
        case kEnginePortTypeNull:
            break;
        case kEnginePortTypeAudio:
            return new CarlaEngineAudioPort(isInput, kProcessMode);
        case kEnginePortTypeEvent:
            return new CarlaEngineEventPort(isInput, kProcessMode);
        }

        qCritical("CarlaEngineRtAudioClient::addPort(%s, \"%s\", %s) - invalid type", EnginePortType2Str(portType), name, bool2str(isInput));
        return nullptr;
    }

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineRtAudioClient)
};

// -------------------------------------------------------------------------------------------------------------------
// RtAudio Engine

class CarlaEngineRtAudio : public CarlaEngine
{
public:
    CarlaEngineRtAudio(RtAudio::Api api)
        : CarlaEngine(),
          audio(api)
    {
        qDebug("CarlaEngineRtAudio::CarlaEngineRtAudio(%i)", api);

        evIn  = nullptr;
        evOut = nullptr;

        m_audioInterleaved = false;
        m_inBuf1  = nullptr;
        m_inBuf2  = nullptr;
        m_outBuf1 = nullptr;
        m_outBuf2 = nullptr;

        // just to make sure
        fOptions.forceStereo = true;
        fOptions.processMode = PROCESS_MODE_CONTINUOUS_RACK;
    }

    ~CarlaEngineRtAudio()
    {
        qDebug("CarlaEngineRtAudio::~CarlaEngineRtAudio()");
    }

    // -------------------------------------

    bool init(const char* const clientName)
    {
        qDebug("CarlaEngineRtAudio::init(\"%s\")", clientName);

        if (audio.getDeviceCount() < 1)
        {
            setLastError("No audio devices available for this driver");
            return false;
        }

        fBufferSize = fOptions.preferredBufferSize;
        fSampleRate = fOptions.preferredSampleRate;

        RtAudio::StreamParameters iParams, oParams;
        iParams.deviceId = audio.getDefaultInputDevice();
        oParams.deviceId = audio.getDefaultOutputDevice();
        iParams.nChannels = 2;
        oParams.nChannels = 2;

        RtAudio::StreamOptions rtOptions;
        rtOptions.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_HOG_DEVICE | RTAUDIO_SCHEDULE_REALTIME;
        rtOptions.numberOfBuffers = 2;
        rtOptions.streamName = clientName;
        rtOptions.priority = 85;

        if (audio.getCurrentApi() != RtAudio::LINUX_PULSE)
        {
            rtOptions.flags |= RTAUDIO_NONINTERLEAVED;
            m_audioInterleaved = false;

            if (audio.getCurrentApi() == RtAudio::LINUX_ALSA)
                rtOptions.flags |= RTAUDIO_ALSA_USE_DEFAULT;
        }
        else
            m_audioInterleaved = true;

        try {
            audio.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, fSampleRate, &fBufferSize, carla_rtaudio_process_callback, this, &rtOptions);
        }
        catch (RtError& e)
        {
            setLastError(e.what());
            return false;
        }

        try {
            audio.startStream();
        }
        catch (RtError& e)
        {
            setLastError(e.what());
            return false;
        }

        fSampleRate = audio.getStreamSampleRate();

        m_inBuf1  = new float[fBufferSize];
        m_inBuf2  = new float[fBufferSize];
        m_outBuf1 = new float[fBufferSize];
        m_outBuf2 = new float[fBufferSize];

        //midiIn = new MidiInAlsa(clientName, 512);
        //midiIn->openVirtualPort("control-in");
        //midiIn->openVirtualPort("midi-in");

        //midiOut = new MidiOutAlsa(clientName);
        //midiOut->openVirtualPort("control-out");
        //midiOut->openVirtualPort("midi-out");

        fName = clientName;
        fName.toBasic();

        CarlaEngine::init(fName);
        return true;
    }

    bool close()
    {
        qDebug("CarlaEngineRtAudio::close()");
        CarlaEngine::close();

        if (audio.isStreamRunning())
            audio.stopStream();

        if (audio.isStreamOpen())
            audio.closeStream();

#if 0
        if (midiIn)
        {
            midiIn->cancelCallback();
            midiIn->closePort();
            delete midiIn;
            midiIn = nullptr;
        }

        if (midiOut)
        {
            midiOut->closePort();
            delete midiOut;
            midiOut = nullptr;
        }
#endif

        if (m_inBuf1)
        {
            delete[] m_inBuf1;
            m_inBuf1 = nullptr;
        }

        if (m_inBuf2)
        {
            delete[] m_inBuf2;
            m_inBuf2 = nullptr;
        }

        if (m_outBuf1)
        {
            delete[] m_outBuf1;
            m_outBuf1 = nullptr;
        }

        if (m_outBuf2)
        {
            delete[] m_outBuf2;
            m_outBuf2 = nullptr;
        }

        return true;
    }

    bool isRunning() const
    {
        return audio.isStreamRunning();
    }

    bool isOffline() const
    {
        return false;
    }

    EngineType type() const
    {
        return kEngineTypeRtAudio;
    }

    CarlaEngineClient* addClient(CarlaPlugin* const)
    {
        return new CarlaEngineRtAudioClient(kEngineTypeRtAudio, fOptions.processMode);
    }

    // -------------------------------------

protected:
    void handleProcessCallback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status)
    {
        if (maxPluginNumber() == 0)
            return;

        // get buffers from RtAudio
        float* insPtr  = (float*)inputBuffer;
        float* outsPtr = (float*)outputBuffer;

        // assert buffers
        CARLA_ASSERT(insPtr);
        CARLA_ASSERT(outsPtr);

        // initialize audio input
        if (m_audioInterleaved)
        {
            for (unsigned int i=0; i < nframes*2; i++)
            {
                if (i % 2)
                    m_inBuf2[i/2] = insPtr[i];
                else
                    m_inBuf1[i/2] = insPtr[i];
            }
        }
        else
        {
            for (unsigned int i=0; i < nframes; i++)
                m_inBuf1[i] = insPtr[i];

            for (unsigned int i=0, j=nframes; i < nframes; i++, j++)
                m_inBuf2[i] = insPtr[j];
        }

        // create audio buffers
        float* inBuf[2]  = { m_inBuf1, m_inBuf2 };
        float* outBuf[2] = { m_outBuf1, m_outBuf2 };

        // initialize events input
        //memset(rackEventsIn, 0, sizeof(EngineEvent)*MAX_EVENTS);
        {
            // TODO
        }

        processRack(inBuf, outBuf, nframes);

        // output audio
        if (m_audioInterleaved)
        {
            for (unsigned int i=0; i < nframes*2; i++)
            {
                if (i % 2)
                    outsPtr[i] = m_outBuf2[i/2];
                else
                    outsPtr[i] = m_outBuf1[i/2];
            }
        }
        else
        {
            for (unsigned int i=0; i < nframes; i++)
                outsPtr[i] = m_outBuf1[i];

            for (unsigned int i=0, j=nframes; i < nframes; i++, j++)
                outsPtr[j] = m_outBuf2[i];
        }

        // output control
        {
            // TODO
        }

        // output midi
        {
            // TODO
        }

        Q_UNUSED(streamTime);
        Q_UNUSED(status);
    }

    // -------------------------------------

private:
    RtAudio audio;
    ScopedPointer<MidiInApi>  evIn;
    ScopedPointer<MidiOutApi> evOut;

    bool   m_audioInterleaved;
    float* m_inBuf1;
    float* m_inBuf2;
    float* m_outBuf1;
    float* m_outBuf2;

    static int carla_rtaudio_process_callback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status, void* userData)
    {
        CarlaEngineRtAudio* const _this_ = (CarlaEngineRtAudio*)userData;
        _this_->handleProcessCallback(outputBuffer, inputBuffer, nframes, streamTime, status);
        return 0;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineRtAudio)
};

// -----------------------------------------

static std::vector<RtAudio::Api> rtApis;

static void initRtApis()
{
    static bool initiated = false;

    if (! initiated)
    {
        initiated = true;

        RtAudio::getCompiledApi(rtApis);
    }
}

CarlaEngine* CarlaEngine::newRtAudio(RtAudioApi api)
{
    RtAudio::Api rtApi = RtAudio::UNSPECIFIED;

    switch (api)
    {
    case RTAUDIO_DUMMY:
        rtApi = RtAudio::RTAUDIO_DUMMY;
        break;
    case RTAUDIO_LINUX_ALSA:
        rtApi = RtAudio::LINUX_ALSA;
        break;
    case RTAUDIO_LINUX_PULSE:
        rtApi = RtAudio::LINUX_PULSE;
        break;
    case RTAUDIO_LINUX_OSS:
        rtApi = RtAudio::LINUX_OSS;
        break;
    case RTAUDIO_UNIX_JACK:
        rtApi = RtAudio::UNIX_JACK;
        break;
    case RTAUDIO_MACOSX_CORE:
        rtApi = RtAudio::MACOSX_CORE;
        break;
    case RTAUDIO_WINDOWS_ASIO:
        rtApi = RtAudio::WINDOWS_ASIO;
        break;
    case RTAUDIO_WINDOWS_DS:
        rtApi = RtAudio::WINDOWS_DS;
        break;
    }

    return new CarlaEngineRtAudio(rtApi);
}

unsigned int CarlaEngine::getRtAudioApiCount()
{
    initRtApis();

    return rtApis.size();
}

const char* CarlaEngine::getRtAudioApiName(unsigned int index)
{
    initRtApis();

    if (index < rtApis.size())
    {
        const RtAudio::Api& api(rtApis[index]);

        switch (api)
        {
        case RtAudio::UNSPECIFIED:
            return "Unspecified";
        case RtAudio::LINUX_ALSA:
            return "ALSA";
        case RtAudio::LINUX_PULSE:
            return "PulseAudio";
        case RtAudio::LINUX_OSS:
            return "OSS";
        case RtAudio::UNIX_JACK:
            return "JACK (RtAudio)";
        case RtAudio::MACOSX_CORE:
            return "CoreAudio";
        case RtAudio::WINDOWS_ASIO:
            return "ASIO";
        case RtAudio::WINDOWS_DS:
            return "DirectSound";
        case RtAudio::RTAUDIO_DUMMY:
            return "Dummy";
        }
    }

    return nullptr;
}

// -----------------------------------------

CARLA_BACKEND_END_NAMESPACE

#ifdef QTCREATOR_TEST
int main()
{
    return 0;
}

#endif

#endif // CARLA_ENGINE_RTAUDIO
