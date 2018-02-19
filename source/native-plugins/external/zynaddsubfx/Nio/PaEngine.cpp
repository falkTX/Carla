/*
  ZynAddSubFX - a software synthesizer

  PaEngine.cpp - Audio output for PortAudio
  Copyright (C) 2002 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "PaEngine.h"
#include <iostream>
using namespace std;

namespace zyncarla {

PaEngine::PaEngine(const SYNTH_T &synth)
    :AudioOut(synth), stream(NULL)
{
    name = "PA";
}


PaEngine::~PaEngine()
{
    Stop();
}

bool PaEngine::Start()
{
    if(getAudioEn())
        return true;
    Pa_Initialize();

    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice();
    if(outputParameters.device == paNoDevice) {
        cerr << "Error: No default output device." << endl;
        Pa_Terminate();
        return false;
    }
    outputParameters.channelCount     = 2; /* stereo output */
    outputParameters.sampleFormat     = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency =
        Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;


    Pa_OpenStream(&stream,
                  NULL,
                  &outputParameters,
                  synth.samplerate,
                  synth.buffersize,
                  0,
                  PAprocess,
                  (void *) this);
    Pa_StartStream(stream);
    return true;
}

void PaEngine::setAudioEn(bool nval)
{
    if(nval)
        Start();
    else
        Stop();
}

bool PaEngine::getAudioEn() const
{
    return stream;
}

int PaEngine::PAprocess(const void *inputBuffer,
                        void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *outTime,
                        PaStreamCallbackFlags flags,
                        void *userData)
{
    (void) inputBuffer;
    (void) outTime;
    (void) flags;
    return static_cast<PaEngine *>(userData)->process((float *) outputBuffer,
                                                      framesPerBuffer);
}

int PaEngine::process(float *out, unsigned long framesPerBuffer)
{
    const Stereo<float *> smp = getNext();
    for(unsigned i = 0; i < framesPerBuffer; ++i) {
        *out++ = smp.l[i];
        *out++ = smp.r[i];
    }

    return 0;
}

void PaEngine::Stop()
{
    if(!getAudioEn())
        return;
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    stream = NULL;
    Pa_Terminate();
}

}
