/*
  ZynAddSubFX - a software synthesizer

  SVFilter.cpp - Several state-variable filters
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <err.h>
#include "../Misc/Util.h"
#include "SVFilter.h"

SVFilter::SVFilter(unsigned char Ftype, float Ffreq, float Fq,
                   unsigned char Fstages)
    :type(Ftype),
      stages(Fstages),
      freq(Ffreq),
      q(Fq),
      gain(1.0f),
      needsinterpolation(false),
      firsttime(true)
{
    if(stages >= MAX_FILTER_STAGES)
        stages = MAX_FILTER_STAGES;
    outgain = 1.0f;
    cleanup();
    setfreq_and_q(Ffreq, Fq);
}

SVFilter::~SVFilter()
{}

void SVFilter::cleanup()
{
    for(int i = 0; i < MAX_FILTER_STAGES + 1; ++i)
        st[i].low = st[i].high = st[i].band = st[i].notch = 0.0f;
    oldabovenq = false;
    abovenq    = false;
}

void SVFilter::computefiltercoefs(void)
{
    par.f = freq / synth->samplerate_f * 4.0f;
    if(par.f > 0.99999f)
        par.f = 0.99999f;
    par.q      = 1.0f - atanf(sqrtf(q)) * 2.0f / PI;
    par.q      = powf(par.q, 1.0f / (stages + 1));
    par.q_sqrt = sqrtf(par.q);
}


void SVFilter::setfreq(float frequency)
{
    if(frequency < 0.1f)
        frequency = 0.1f;
    float rap = freq / frequency;
    if(rap < 1.0f)
        rap = 1.0f / rap;

    oldabovenq = abovenq;
    abovenq    = frequency > (synth->samplerate_f / 2 - 500.0f);

    bool nyquistthresh = (abovenq ^ oldabovenq);

    //if the frequency is changed fast, it needs interpolation
    if((rap > 3.0f) || nyquistthresh) { //(now, filter and coeficients backup)
        if(!firsttime)
            needsinterpolation = true;
        ipar = par;
    }
    freq = frequency;
    computefiltercoefs();
    firsttime = false;
}

void SVFilter::setfreq_and_q(float frequency, float q_)
{
    q = q_;
    setfreq(frequency);
}

void SVFilter::setq(float q_)
{
    q = q_;
    computefiltercoefs();
}

void SVFilter::settype(int type_)
{
    type = type_;
    computefiltercoefs();
}

void SVFilter::setgain(float dBgain)
{
    gain = dB2rap(dBgain);
    computefiltercoefs();
}

void SVFilter::setstages(int stages_)
{
    if(stages_ >= MAX_FILTER_STAGES)
        stages_ = MAX_FILTER_STAGES - 1;
    stages = stages_;
    cleanup();
    computefiltercoefs();
}

void SVFilter::singlefilterout(float *smp, fstage &x, parameters &par)
{
    float *out = NULL;
    switch(type) {
        case 0:
            out = &x.low;
            break;
        case 1:
            out = &x.high;
            break;
        case 2:
            out = &x.band;
            break;
        case 3:
            out = &x.notch;
            break;
        default:
            errx(1, "Impossible SVFilter type encountered [%d]", type);
    }

    for(int i = 0; i < synth->buffersize; ++i) {
        x.low   = x.low + par.f * x.band;
        x.high  = par.q_sqrt * smp[i] - x.low - par.q * x.band;
        x.band  = par.f * x.high + x.band;
        x.notch = x.high + x.low;
        smp[i]  = *out;
    }
}

void SVFilter::filterout(float *smp)
{
    for(int i = 0; i < stages + 1; ++i)
        singlefilterout(smp, st[i], par);

    if(needsinterpolation) {
        float *ismp = getTmpBuffer();
        memcpy(ismp, smp, synth->bufferbytes);

        for(int i = 0; i < stages + 1; ++i)
            singlefilterout(ismp, st[i], ipar);

        for(int i = 0; i < synth->buffersize; ++i) {
            float x = i / synth->buffersize_f;
            smp[i] = ismp[i] * (1.0f - x) + smp[i] * x;
        }
        returnTmpBuffer(ismp);
        needsinterpolation = false;
    }

    for(int i = 0; i < synth->buffersize; ++i)
        smp[i] *= outgain;
}
