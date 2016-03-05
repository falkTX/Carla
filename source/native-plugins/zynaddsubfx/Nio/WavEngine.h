/*
  ZynAddSubFX - a software synthesizer

  WavEngine.h - Records sound to a file
  Copyright (C) 2008 Nasca Octavian Paul
  Author: Nasca Octavian Paul
          Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef WAVENGINE_H
#define WAVENGINE_H
#include "AudioOut.h"
#include <string>
#include <pthread.h>
#include "ZynSema.h"
#include "SafeQueue.h"

class WavFile;
class WavEngine:public AudioOut
{
    public:
        WavEngine(const SYNTH_T &synth);
        ~WavEngine();

        bool openAudio();
        bool Start();
        void Stop();

        void setAudioEn(bool /*nval*/) {}
        bool getAudioEn() const {return true; }

        void push(Stereo<float *> smps, size_t len);

        void newFile(WavFile *_file);
        void destroyFile();

    protected:
        void *AudioThread();
        static void *_AudioThread(void *arg);

    private:
        WavFile *file;
        ZynSema  work;
        SafeQueue<float> buffer;

        pthread_t *pThread;
};
#endif
