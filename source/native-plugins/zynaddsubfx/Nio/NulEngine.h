/*
  ZynAddSubFX - a software synthesizer

  NulEngine.h - Dummy In/Out driver
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef NUL_ENGINE_H
#define NUL_ENGINE_H

#include <sys/time.h>
#include <pthread.h>
#include "../globals.h"
#include "AudioOut.h"
#include "MidiIn.h"

class NulEngine:public AudioOut, MidiIn
{
    public:
        NulEngine(const SYNTH_T &synth_);
        ~NulEngine();

        bool Start();
        void Stop();

        void setAudioEn(bool nval);
        bool getAudioEn() const;

        void setMidiEn(bool) {}
        bool getMidiEn() const {return true; }

    protected:
        void *AudioThread();
        static void *_AudioThread(void *arg);

    private:
        struct timeval playing_until;
        pthread_t     *pThread;
};

#endif
