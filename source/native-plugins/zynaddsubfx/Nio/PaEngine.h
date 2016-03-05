/*
  ZynAddSubFX - a software synthesizer

  PAaudiooutput.h - Audio output for PortAudio
  Copyright (C) 2002 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef PA_ENGINE_H
#define PA_ENGINE_H

#include <portaudio.h>

#include "../globals.h"
#include "AudioOut.h"

class PaEngine:public AudioOut
{
    public:
        PaEngine(const SYNTH_T &synth);
        ~PaEngine();

        bool Start();
        void Stop();

        void setAudioEn(bool nval);
        bool getAudioEn() const;

    protected:
        static int PAprocess(const void *inputBuffer,
                             void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo *outTime,
                             PaStreamCallbackFlags flags,
                             void *userData);
        int process(float *out, unsigned long framesPerBuffer);
    private:
        PaStream *stream;
};


void PAfinish();

#endif
