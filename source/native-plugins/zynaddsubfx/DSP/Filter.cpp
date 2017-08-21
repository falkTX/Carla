/*
  ZynAddSubFX - a software synthesizer

  Filter.cpp - Filters, uses analog,formant,etc. filters
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <cstdio>
#include <cassert>

#include "Filter.h"
#include "AnalogFilter.h"
#include "FormantFilter.h"
#include "SVFilter.h"
#include "../Params/FilterParams.h"
#include "../Misc/Allocator.h"

namespace zyncarla {

Filter::Filter(unsigned int srate, int bufsize)
    : outgain(1.0f),
      samplerate(srate),
      buffersize(bufsize)
{
    alias();
}

Filter *Filter::generate(Allocator &memory, const FilterParams *pars,
        unsigned int srate, int bufsize)
{
    assert(srate != 0);
    assert(bufsize != 0);

    unsigned char Ftype   = pars->Ptype;
    unsigned char Fstages = pars->Pstages;

    Filter *filter;
    switch(pars->Pcategory) {
        case 1:
            filter = memory.alloc<FormantFilter>(pars, &memory, srate, bufsize);
            break;
        case 2:
            filter = memory.alloc<SVFilter>(Ftype, 1000.0f, pars->getq(), Fstages, srate, bufsize);
            filter->outgain = dB2rap(pars->getgain());
            if(filter->outgain > 1.0f)
                filter->outgain = sqrt(filter->outgain);
            break;
        default:
            filter = memory.alloc<AnalogFilter>(Ftype, 1000.0f, pars->getq(), Fstages, srate, bufsize);
            if((Ftype >= 6) && (Ftype <= 8))
                filter->setgain(pars->getgain());
            else
                filter->outgain = dB2rap(pars->getgain());
            break;
    }
    return filter;
}

float Filter::getrealfreq(float freqpitch)
{
    return powf(2.0f, freqpitch + 9.96578428f); //log2(1000)=9.95748f
}

}
