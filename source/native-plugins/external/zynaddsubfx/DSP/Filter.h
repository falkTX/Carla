/*
  ZynAddSubFX - a software synthesizer

  Filter.h - Filters, uses analog,formant,etc. filters
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef FILTER_H
#define FILTER_H

#include "../globals.h"

namespace zyncarla {

class Filter
{
    public:
        static float getrealfreq(float freqpitch);
        static Filter *generate(Allocator &memory, const FilterParams *pars,
                unsigned int srate, int bufsize);

        Filter(unsigned int srate, int bufsize);
        virtual ~Filter() {}
        virtual void filterout(float *smp)    = 0;
        virtual void setfreq(float frequency) = 0;
        virtual void setfreq_and_q(float frequency, float q_) = 0;
        virtual void setq(float q_) = 0;
        virtual void setgain(float dBgain) = 0;

    protected:
        float outgain;

        // current setup
        unsigned int samplerate;
        int buffersize;

        // alias for above terms
        float samplerate_f;
        float halfsamplerate_f;
        float buffersize_f;
        int   bufferbytes;

        inline void alias()
        {
            samplerate_f     = samplerate;
            halfsamplerate_f = samplerate_f / 2.0f;
            buffersize_f     = buffersize;
            bufferbytes      = buffersize * sizeof(float);
        }
};

}

#endif
