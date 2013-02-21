/*
 * Carla RtAudio Engine
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#ifdef WANT_RTAUDIO

#include "CarlaEngineInternal.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaMIDI.h"
#include "RtList.hpp"

#include "RtAudio.h"
#include "RtMidi.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------------------------------------------------------

RtMidi::Api getMatchedAudioMidiAPi(const RtAudio::Api rtApi)
{
    switch (rtApi)
    {
    case RtAudio::UNSPECIFIED:
        return RtMidi::UNSPECIFIED;

    case RtAudio::LINUX_ALSA:
    case RtAudio::LINUX_OSS:
    case RtAudio::LINUX_PULSE:
        return RtMidi::LINUX_ALSA;

    case RtAudio::UNIX_JACK:
        return RtMidi::UNIX_JACK;

    case RtAudio::MACOSX_CORE:
        return RtMidi::MACOSX_CORE;

    case RtAudio::WINDOWS_ASIO:
    case RtAudio::WINDOWS_DS:
        return RtMidi::WINDOWS_MM;

    case RtAudio::RTAUDIO_DUMMY:
        return RtMidi::RTMIDI_DUMMY;
    }

    return RtMidi::UNSPECIFIED;
}

// -------------------------------------------------------------------------------------------------------------------
// RtAudio Engine

class CarlaEngineRtAudio : public CarlaEngine
{
public:
    CarlaEngineRtAudio(const RtAudio::Api api)
        : CarlaEngine(),
          fAudio(api),
          fAudioIsInterleaved(false),
          fAudioIsReady(false),
          fAudioInBuf1(nullptr),
          fAudioInBuf2(nullptr),
          fAudioOutBuf1(nullptr),
          fAudioOutBuf2(nullptr),
          fMidiIn(getMatchedAudioMidiAPi(api), "CarlaIn"),
          fMidiOut(getMatchedAudioMidiAPi(api), "CarlaOut")
    {
        carla_debug("CarlaEngineRtAudio::CarlaEngineRtAudio(%i)", api);

        // just to make sure
        fOptions.forceStereo = true;
        fOptions.processMode = PROCESS_MODE_CONTINUOUS_RACK;
    }

    ~CarlaEngineRtAudio()
    {
        carla_debug("CarlaEngineRtAudio::~CarlaEngineRtAudio()");
        CARLA_ASSERT(fAudioInBuf1 == nullptr);
        CARLA_ASSERT(fAudioInBuf2 == nullptr);
        CARLA_ASSERT(fAudioOutBuf1 == nullptr);
        CARLA_ASSERT(fAudioOutBuf2 == nullptr);
    }

    // -------------------------------------

    bool init(const char* const clientName)
    {
        carla_debug("CarlaEngineRtAudio::init(\"%s\")", clientName);
        CARLA_ASSERT(! fAudioIsReady);
        CARLA_ASSERT(fAudioInBuf1 == nullptr);
        CARLA_ASSERT(fAudioInBuf2 == nullptr);
        CARLA_ASSERT(fAudioOutBuf1 == nullptr);
        CARLA_ASSERT(fAudioOutBuf2 == nullptr);
        CARLA_ASSERT(clientName != nullptr);

        if (fAudio.getDeviceCount() == 0)
        {
            setLastError("No audio devices available for this driver");
            return false;
        }

        fBufferSize = fOptions.preferredBufferSize;

        // Audio
        {
            RtAudio::StreamParameters iParams, oParams;
            iParams.deviceId = fAudio.getDefaultInputDevice();
            oParams.deviceId = fAudio.getDefaultOutputDevice();
            iParams.nChannels = 2;
            oParams.nChannels = 2;

            RtAudio::StreamOptions rtOptions;
            rtOptions.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_HOG_DEVICE | RTAUDIO_SCHEDULE_REALTIME;
            rtOptions.numberOfBuffers = 2;
            rtOptions.streamName = clientName;
            rtOptions.priority = 85;

            if (fAudio.getCurrentApi() != RtAudio::LINUX_PULSE)
            {
                rtOptions.flags |= RTAUDIO_NONINTERLEAVED;
                fAudioIsInterleaved = false;

                if (fAudio.getCurrentApi() == RtAudio::LINUX_ALSA)
                    rtOptions.flags |= RTAUDIO_ALSA_USE_DEFAULT;
            }
            else
                fAudioIsInterleaved = true;

            try {
                fAudio.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, fOptions.preferredSampleRate, &fBufferSize, carla_rtaudio_process_callback, this, &rtOptions);
            }
            catch (RtError& e)
            {
                carla_stderr2("RtAudio::openStream() failed");
                if (e.getType() == RtError::SYSTEM_ERROR)
                    setLastError("Stream cannot be opened with the specified parameters");
                else if (e.getType() == RtError::INVALID_USE)
                    setLastError("Invalid device ID");
                else
                    setLastError("Unknown error");
                return false;
            }

            try {
                fAudio.startStream();
            }
            catch (RtError& e)
            {
                carla_stderr2("RtAudio::startStream() failed");
                setLastError(e.what());
                fAudio.closeStream();
                return false;
            }

            fAudioInBuf1  = new float[fBufferSize];
            fAudioInBuf2  = new float[fBufferSize];
            fAudioOutBuf1 = new float[fBufferSize];
            fAudioOutBuf2 = new float[fBufferSize];

            fSampleRate = fAudio.getStreamSampleRate();
        }

        // MIDI
        {
            fMidiIn.setCallback(carla_rtmidi_callback, this);
            fMidiIn.openVirtualPort("events-in");
            fMidiOut.openVirtualPort("events-out");
        }

        fAudioIsReady = true;

        return CarlaEngine::init(clientName);
    }

    bool close()
    {
        carla_debug("CarlaEngineRtAudio::close()");
        CARLA_ASSERT(fAudioIsReady);
        CARLA_ASSERT(fAudioInBuf1 != nullptr);
        CARLA_ASSERT(fAudioInBuf2 != nullptr);
        CARLA_ASSERT(fAudioOutBuf1 != nullptr);
        CARLA_ASSERT(fAudioOutBuf2 != nullptr);

        CarlaEngine::close();

        fAudioIsReady = false;

        if (fAudio.isStreamRunning())
        {
            try {
                fAudio.stopStream();
            }
            catch (...) {}
        }

        if (fAudio.isStreamOpen())
        {
            try {
                fAudio.closeStream();
            }
            catch (...) {}
        }

        fMidiIn.cancelCallback();
        fMidiIn.closePort();
        fMidiOut.closePort();

        if (fAudioInBuf1 != nullptr)
        {
            delete[] fAudioInBuf1;
            fAudioInBuf1 = nullptr;
        }

        if (fAudioInBuf2 != nullptr)
        {
            delete[] fAudioInBuf2;
            fAudioInBuf2 = nullptr;
        }

        if (fAudioOutBuf1 != nullptr)
        {
            delete[] fAudioOutBuf1;
            fAudioOutBuf1 = nullptr;
        }

        if (fAudioOutBuf2 != nullptr)
        {
            delete[] fAudioOutBuf2;
            fAudioOutBuf2 = nullptr;
        }

        fMidiInEvents.clear();
        fMidiOutEvents.clear();

        return true;
    }

    bool isRunning() const
    {
        return fAudio.isStreamRunning();
    }

    bool isOffline() const
    {
        return false;
    }

    EngineType type() const
    {
        return kEngineTypeRtAudio;
    }

    // -------------------------------------

protected:
    void handleAudioProcessCallback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status)
    {
        // get buffers from RtAudio
        float* insPtr  = (float*)inputBuffer;
        float* outsPtr = (float*)outputBuffer;

        // assert buffers
        CARLA_ASSERT(insPtr != nullptr);
        CARLA_ASSERT(outsPtr != nullptr);

        if (! fAudioIsReady)
        {
            carla_zeroFloat(outsPtr, nframes*2);
            proccessPendingEvents();
            return;
        }

        // initialize audio input
        if (fAudioIsInterleaved)
        {
            for (unsigned int i=0; i < nframes*2; i++)
            {
                if (i % 2 == 0)
                    fAudioInBuf1[i/2] = insPtr[i];
                else
                    fAudioInBuf2[i/2] = insPtr[i];
            }
        }
        else
        {
            std::memcpy(fAudioInBuf1, insPtr, sizeof(float)*nframes);
            std::memcpy(fAudioInBuf2, insPtr+nframes, sizeof(float)*nframes);
        }

        // initialize audio output
        carla_zeroFloat(fAudioOutBuf1, fBufferSize);
        carla_zeroFloat(fAudioOutBuf2, fBufferSize);

        // initialize input events
        carla_zeroMem(kData->rack.in, sizeof(EngineEvent)*RACK_EVENT_COUNT);

        if (fMidiInEvents.mutex.tryLock())
        {
            uint32_t engineEventIndex = 0;
            fMidiInEvents.splice();

            while (! fMidiInEvents.data.isEmpty())
            {
                const RtMidiEvent& midiEvent = fMidiInEvents.data.getLast(true);

                EngineEvent* const engineEvent = &kData->rack.in[engineEventIndex++];
                engineEvent->clear();

                const uint8_t midiStatus  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                const uint8_t midiChannel = MIDI_GET_CHANNEL_FROM_DATA(midiEvent.data);

                engineEvent->channel = midiChannel;

                if (midiEvent.time < fTimeInfo.frame)
                    engineEvent->time = 0;
                else if (midiEvent.time >= fTimeInfo.frame + nframes)
                {
                    engineEvent->time = fTimeInfo.frame + nframes-1;
                    carla_stderr("MIDI Event in the future!, %i vs %i", engineEvent->time, fTimeInfo.frame);
                }
                else
                    engineEvent->time = midiEvent.time - fTimeInfo.frame;

                //carla_stdout("Got midi, time %f vs %i", midiEvent.time, engineEvent->time);

                if (MIDI_IS_STATUS_CONTROL_CHANGE(midiStatus))
                {
                    const uint8_t midiControl = midiEvent.data[1];
                    engineEvent->type         = kEngineEventTypeControl;

                    if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
                    {
                        const uint8_t midiBank  = midiEvent.data[2];

                        engineEvent->ctrl.type  = kEngineControlEventTypeMidiBank;
                        engineEvent->ctrl.param = midiBank;
                        engineEvent->ctrl.value = 0.0;
                    }
                    else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
                    {
                        engineEvent->ctrl.type  = kEngineControlEventTypeAllSoundOff;
                        engineEvent->ctrl.param = 0;
                        engineEvent->ctrl.value = 0.0;
                    }
                    else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
                    {
                        engineEvent->ctrl.type  = kEngineControlEventTypeAllNotesOff;
                        engineEvent->ctrl.param = 0;
                        engineEvent->ctrl.value = 0.0;
                    }
                    else
                    {
                        const uint8_t midiValue = midiEvent.data[2];

                        engineEvent->ctrl.type  = kEngineControlEventTypeParameter;
                        engineEvent->ctrl.param = midiControl;
                        engineEvent->ctrl.value = double(midiValue)/127.0;
                    }
                }
                else if (MIDI_IS_STATUS_PROGRAM_CHANGE(midiStatus))
                {
                    const uint8_t midiProgram = midiEvent.data[1];
                    engineEvent->type         = kEngineEventTypeControl;

                    engineEvent->ctrl.type  = kEngineControlEventTypeMidiProgram;
                    engineEvent->ctrl.param = midiProgram;
                    engineEvent->ctrl.value = 0.0;
                }
                else
                {
                    engineEvent->type = kEngineEventTypeMidi;

                    engineEvent->midi.data[0] = midiStatus;
                    engineEvent->midi.data[1] = midiEvent.data[1];
                    engineEvent->midi.data[2] = midiEvent.data[2];
                    engineEvent->midi.size    = midiEvent.size;
                }

                if (engineEventIndex >= RACK_EVENT_COUNT)
                    break;
            }

            fMidiInEvents.mutex.unlock();
        }

        // create audio buffers
        float* inBuf[2]  = { fAudioInBuf1, fAudioInBuf2 };
        float* outBuf[2] = { fAudioOutBuf1, fAudioOutBuf2 };

        processRack(inBuf, outBuf, nframes);

        // output audio
        if (fAudioIsInterleaved)
        {
            for (unsigned int i=0; i < nframes*2; i++)
            {
                if (i % 2 == 0)
                    outsPtr[i] = fAudioOutBuf1[i/2];
                else
                    outsPtr[i] = fAudioOutBuf2[i/2];
            }
        }
        else
        {
            std::memcpy(outsPtr, fAudioOutBuf1, sizeof(float)*nframes);
            std::memcpy(outsPtr+nframes, fAudioOutBuf2, sizeof(float)*nframes);
        }

        // output events
        {
            // TODO
            //fMidiOut.sendMessage();
        }

        // TESTING
        fTimeInfo.playing  = true;
        fTimeInfo.frame   += nframes;

        proccessPendingEvents();

        return;

        // unused
        (void)streamTime;
        (void)status;
    }

    void handleMidiCallback(double timeStamp, std::vector<unsigned char>* const message)
    {
        const size_t messageSize = message->size();
        static uint32_t lastTime = 0;

        if (messageSize == 0 || messageSize > 3)
            return;

        timeStamp /= 2;

        if (timeStamp > 0.95)
            timeStamp = 0.95;

        RtMidiEvent midiEvent;
        midiEvent.time = fTimeInfo.frame + (timeStamp*(double)fBufferSize);
        carla_stdout("Put midi, frame:%09i/%09i, stamp:%g", fTimeInfo.frame, midiEvent.time, timeStamp);

        if (midiEvent.time < lastTime)
            midiEvent.time = lastTime;
        else
            lastTime = midiEvent.time;

        if (messageSize == 1)
        {
            midiEvent.data[0] = message->at(0);
            midiEvent.data[1] = 0;
            midiEvent.data[2] = 0;
            midiEvent.size    = 1;
        }
        else if (messageSize == 2)
        {
            midiEvent.data[0] = message->at(0);
            midiEvent.data[1] = message->at(1);
            midiEvent.data[2] = 0;
            midiEvent.size    = 2;
        }
        else
        {
            midiEvent.data[0] = message->at(0);
            midiEvent.data[1] = message->at(1);
            midiEvent.data[2] = message->at(2);
            midiEvent.size    = 3;
        }

        fMidiInEvents.append(midiEvent);
    }

    // -------------------------------------

private:
    RtAudio fAudio;
    bool    fAudioIsInterleaved;
    bool    fAudioIsReady;
    float*  fAudioInBuf1;
    float*  fAudioInBuf2;
    float*  fAudioOutBuf1;
    float*  fAudioOutBuf2;

    RtMidiIn  fMidiIn;
    RtMidiOut fMidiOut;

    struct RtMidiEvent {
        uint32_t time;
        unsigned char data[3];
        unsigned char size;
    };

    struct RtMidiEvents {
        CarlaMutex mutex;
        RtList<RtMidiEvent>::Pool dataPool;
        RtList<RtMidiEvent> data;
        RtList<RtMidiEvent> dataPending;

        RtMidiEvents()
            : dataPool(512, 512),
              data(&dataPool),
              dataPending(&dataPool) {}

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
            dataPending.splice(data, true);
        }
    };

    RtMidiEvents fMidiInEvents;
    RtMidiEvents fMidiOutEvents;

    #define handlePtr ((CarlaEngineRtAudio*)userData)

    static int carla_rtaudio_process_callback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status, void* userData)
    {
        handlePtr->handleAudioProcessCallback(outputBuffer, inputBuffer, nframes, streamTime, status);
        return 0;
    }

    static void carla_rtmidi_callback(double timeStamp, std::vector<unsigned char>* message, void* userData)
    {
        handlePtr->handleMidiCallback(timeStamp, message);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineRtAudio)
};

// -----------------------------------------

static std::vector<RtAudio::Api> sRtAudioApis;

static void initRtApis()
{
    static bool initiated = false;

    if (! initiated)
    {
        initiated = true;

        RtAudio::getCompiledApi(sRtAudioApis);
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

size_t CarlaEngine::getRtAudioApiCount()
{
    initRtApis();

    return sRtAudioApis.size();
}

const char* CarlaEngine::getRtAudioApiName(const unsigned int index)
{
    initRtApis();

    if (index < sRtAudioApis.size())
    {
        const RtAudio::Api& api(sRtAudioApis[index]);

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

#endif // CARLA_ENGINE_RTAUDIO
