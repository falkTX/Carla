/*
  ZynAddSubFX - a software synthesizer

  JackMultiEngine.h - Channeled Audio output JACK
  Copyright (C) 2012-2012 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef JACK_MULTI_ENGINE
#define JACK_MULTI_ENGINE

#include "AudioOut.h"

namespace zyncarla {

class JackMultiEngine:public AudioOut
{
    public:
        JackMultiEngine(const SYNTH_T &synth);
        ~JackMultiEngine(void);

        void setAudioEn(bool nval);
        bool getAudioEn() const;

        bool Start(void);
        void Stop(void);

    private:
        static int _processCallback(unsigned nframes, void *arg);
        int processAudio(unsigned nframes);

        struct jack_multi *impl;
};

}

#endif
