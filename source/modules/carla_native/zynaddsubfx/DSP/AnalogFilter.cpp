/*
  ZynAddSubFX - a software synthesizer

  AnalogFilter.cpp - Several analog filters (lowpass, highpass...)
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2010-2010 Mark McCurry
  Author: Nasca Octavian Paul
          Mark McCurry

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

#include <cstring> //memcpy
#include <cmath>

#include "../Misc/Util.h"
#include "AnalogFilter.h"

AnalogFilter::AnalogFilter(unsigned char Ftype,
                           float Ffreq,
                           float Fq,
                           unsigned char Fstages)
    :type(Ftype),
      stages(Fstages),
      freq(Ffreq),
      q(Fq),
      gain(1.0),
      abovenq(false),
      oldabovenq(false)
{
    for(int i = 0; i < 3; ++i)
        coeff.c[i] = coeff.d[i] = oldCoeff.c[i] = oldCoeff.d[i] = 0.0f;
    if(stages >= MAX_FILTER_STAGES)
        stages = MAX_FILTER_STAGES;
    cleanup();
    firsttime = false;
    setfreq_and_q(Ffreq, Fq);
    firsttime  = true;
    coeff.d[0] = 0; //this is not used
    outgain    = 1.0f;
}

AnalogFilter::~AnalogFilter()
{}

void AnalogFilter::cleanup()
{
    for(int i = 0; i < MAX_FILTER_STAGES + 1; ++i) {
        history[i].x1 = 0.0f;
        history[i].x2 = 0.0f;
        history[i].y1 = 0.0f;
        history[i].y2 = 0.0f;
        oldHistory[i] = history[i];
    }
    needsinterpolation = false;
}

void AnalogFilter::computefiltercoefs(void)
{
    float tmp;
    bool  zerocoefs = false; //this is used if the freq is too high

    //do not allow frequencies bigger than samplerate/2
    float freq = this->freq;
    if(freq > (synth->halfsamplerate_f - 500.0f)) {
        freq      = synth->halfsamplerate_f - 500.0f;
        zerocoefs = true;
    }
    if(freq < 0.1f)
        freq = 0.1f;
    //do not allow bogus Q
    if(q < 0.0f)
        q = 0.0f;
    float tmpq, tmpgain;
    if(stages == 0) {
        tmpq    = q;
        tmpgain = gain;
    }
    else {
        tmpq    = (q > 1.0f) ? powf(q, 1.0f / (stages + 1)) : q;
        tmpgain = powf(gain, 1.0f / (stages + 1));
    }

    //Alias Terms
    float *c = coeff.c;
    float *d = coeff.d;

    //General Constants
    const float omega = 2 * PI * freq / synth->samplerate_f;
    const float sn    = sinf(omega), cs = cosf(omega);
    float       alpha, beta;

    //most of theese are implementations of
    //the "Cookbook formulae for audio EQ" by Robert Bristow-Johnson
    //The original location of the Cookbook is:
    //http://www.harmony-central.com/Computer/Programming/Audio-EQ-Cookbook.txt
    switch(type) {
        case 0: //LPF 1 pole
            if(!zerocoefs)
                tmp = expf(-2.0f * PI * freq / synth->samplerate_f);
            else
                tmp = 0.0f;
            c[0]  = 1.0f - tmp;
            c[1]  = 0.0f;
            c[2]  = 0.0f;
            d[1]  = tmp;
            d[2]  = 0.0f;
            order = 1;
            break;
        case 1: //HPF 1 pole
            if(!zerocoefs)
                tmp = expf(-2.0f * PI * freq / synth->samplerate_f);
            else
                tmp = 0.0f;
            c[0]  = (1.0f + tmp) / 2.0f;
            c[1]  = -(1.0f + tmp) / 2.0f;
            c[2]  = 0.0f;
            d[1]  = tmp;
            d[2]  = 0.0f;
            order = 1;
            break;
        case 2: //LPF 2 poles
            if(!zerocoefs) {
                alpha = sn / (2.0f * tmpq);
                tmp   = 1 + alpha;
                c[1]  = (1.0f - cs) / tmp;
                c[0]  = c[2] = c[1] / 2.0f;
                d[1]  = -2.0f * cs / tmp * -1.0f;
                d[2]  = (1.0f - alpha) / tmp * -1.0f;
            }
            else {
                c[0] = 1.0f;
                c[1] = c[2] = d[1] = d[2] = 0.0f;
            }
            order = 2;
            break;
        case 3: //HPF 2 poles
            if(!zerocoefs) {
                alpha = sn / (2.0f * tmpq);
                tmp   = 1 + alpha;
                c[0]  = (1.0f + cs) / 2.0f / tmp;
                c[1]  = -(1.0f + cs) / tmp;
                c[2]  = (1.0f + cs) / 2.0f / tmp;
                d[1]  = -2.0f * cs / tmp * -1.0f;
                d[2]  = (1.0f - alpha) / tmp * -1.0f;
            }
            else
                c[0] = c[1] = c[2] = d[1] = d[2] = 0.0f;
            order = 2;
            break;
        case 4: //BPF 2 poles
            if(!zerocoefs) {
                alpha = sn / (2.0f * tmpq);
                tmp   = 1.0f + alpha;
                c[0]  = alpha / tmp *sqrtf(tmpq + 1.0f);
                c[1]  = 0.0f;
                c[2]  = -alpha / tmp *sqrtf(tmpq + 1.0f);
                d[1]  = -2.0f * cs / tmp * -1.0f;
                d[2]  = (1.0f - alpha) / tmp * -1.0f;
            }
            else
                c[0] = c[1] = c[2] = d[1] = d[2] = 0.0f;
            order = 2;
            break;
        case 5: //NOTCH 2 poles
            if(!zerocoefs) {
                alpha = sn / (2.0f * sqrtf(tmpq));
                tmp   = 1.0f + alpha;
                c[0]  = 1.0f / tmp;
                c[1]  = -2.0f * cs / tmp;
                c[2]  = 1.0f / tmp;
                d[1]  = -2.0f * cs / tmp * -1.0f;
                d[2]  = (1.0f - alpha) / tmp * -1.0f;
            }
            else {
                c[0] = 1.0f;
                c[1] = c[2] = d[1] = d[2] = 0.0f;
            }
            order = 2;
            break;
        case 6: //PEAK (2 poles)
            if(!zerocoefs) {
                tmpq *= 3.0f;
                alpha = sn / (2.0f * tmpq);
                tmp   = 1.0f + alpha / tmpgain;
                c[0]  = (1.0f + alpha * tmpgain) / tmp;
                c[1]  = (-2.0f * cs) / tmp;
                c[2]  = (1.0f - alpha * tmpgain) / tmp;
                d[1]  = -2.0f * cs / tmp * -1.0f;
                d[2]  = (1.0f - alpha / tmpgain) / tmp * -1.0f;
            }
            else {
                c[0] = 1.0f;
                c[1] = c[2] = d[1] = d[2] = 0.0f;
            }
            order = 2;
            break;
        case 7: //Low Shelf - 2 poles
            if(!zerocoefs) {
                tmpq  = sqrtf(tmpq);
                alpha = sn / (2.0f * tmpq);
                beta  = sqrtf(tmpgain) / tmpq;
                tmp   = (tmpgain + 1.0f) + (tmpgain - 1.0f) * cs + beta * sn;

                c[0] = tmpgain
                       * ((tmpgain
                           + 1.0f) - (tmpgain - 1.0f) * cs + beta * sn) / tmp;
                c[1] = 2.0f * tmpgain
                       * ((tmpgain - 1.0f) - (tmpgain + 1.0f) * cs) / tmp;
                c[2] = tmpgain
                       * ((tmpgain
                           + 1.0f) - (tmpgain - 1.0f) * cs - beta * sn) / tmp;
                d[1] = -2.0f * ((tmpgain - 1.0f) + (tmpgain + 1.0f) * cs)
                       / tmp * -1.0f;
                d[2] = ((tmpgain + 1.0f) + (tmpgain - 1.0f) * cs - beta * sn)
                       / tmp * -1.0f;
            }
            else {
                c[0] = tmpgain;
                c[1] = c[2] = d[1] = d[2] = 0.0f;
            }
            order = 2;
            break;
        case 8: //High Shelf - 2 poles
            if(!zerocoefs) {
                tmpq  = sqrtf(tmpq);
                alpha = sn / (2.0f * tmpq);
                beta  = sqrtf(tmpgain) / tmpq;
                tmp   = (tmpgain + 1.0f) - (tmpgain - 1.0f) * cs + beta * sn;

                c[0] = tmpgain
                       * ((tmpgain
                           + 1.0f) + (tmpgain - 1.0f) * cs + beta * sn) / tmp;
                c[1] = -2.0f * tmpgain
                       * ((tmpgain - 1.0f) + (tmpgain + 1.0f) * cs) / tmp;
                c[2] = tmpgain
                       * ((tmpgain
                           + 1.0f) + (tmpgain - 1.0f) * cs - beta * sn) / tmp;
                d[1] = 2.0f * ((tmpgain - 1.0f) - (tmpgain + 1.0f) * cs)
                       / tmp * -1.0f;
                d[2] = ((tmpgain + 1.0f) - (tmpgain - 1.0f) * cs - beta * sn)
                       / tmp * -1.0f;
            }
            else {
                c[0] = 1.0f;
                c[1] = c[2] = d[1] = d[2] = 0.0f;
            }
            order = 2;
            break;
        default: //wrong type
            type = 0;
            computefiltercoefs();
            break;
    }
}


void AnalogFilter::setfreq(float frequency)
{
    if(frequency < 0.1f)
        frequency = 0.1f;
    float rap = freq / frequency;
    if(rap < 1.0f)
        rap = 1.0f / rap;

    oldabovenq = abovenq;
    abovenq    = frequency > (synth->halfsamplerate_f - 500.0f);

    bool nyquistthresh = (abovenq ^ oldabovenq);


    //if the frequency is changed fast, it needs interpolation
    if((rap > 3.0f) || nyquistthresh) { //(now, filter and coeficients backup)
        oldCoeff = coeff;
        for(int i = 0; i < MAX_FILTER_STAGES + 1; ++i)
            oldHistory[i] = history[i];
        if(!firsttime)
            needsinterpolation = true;
    }
    freq = frequency;
    computefiltercoefs();
    firsttime = false;
}

void AnalogFilter::setfreq_and_q(float frequency, float q_)
{
    q = q_;
    setfreq(frequency);
}

void AnalogFilter::setq(float q_)
{
    q = q_;
    computefiltercoefs();
}

void AnalogFilter::settype(int type_)
{
    type = type_;
    computefiltercoefs();
}

void AnalogFilter::setgain(float dBgain)
{
    gain = dB2rap(dBgain);
    computefiltercoefs();
}

void AnalogFilter::setstages(int stages_)
{
    if(stages_ >= MAX_FILTER_STAGES)
        stages_ = MAX_FILTER_STAGES - 1;
    stages = stages_;
    cleanup();
    computefiltercoefs();
}

void AnalogFilter::singlefilterout(float *smp, fstage &hist,
                                   const Coeff &coeff)
{
    if(order == 1)   //First order filter
        for(int i = 0; i < synth->buffersize; ++i) {
            float y0 = smp[i] * coeff.c[0] + hist.x1 * coeff.c[1]
                       + hist.y1 * coeff.d[1];
            hist.y1 = y0;
            hist.x1 = smp[i];
            smp[i]  = y0;
        }
    if(order == 2) //Second order filter
        for(int i = 0; i < synth->buffersize; ++i) {
            float y0 = smp[i] * coeff.c[0] + hist.x1 * coeff.c[1]
                       + hist.x2 * coeff.c[2] + hist.y1 * coeff.d[1]
                       + hist.y2 * coeff.d[2];
            hist.y2 = hist.y1;
            hist.y1 = y0;
            hist.x2 = hist.x1;
            hist.x1 = smp[i];
            smp[i]  = y0;
        }
}

void AnalogFilter::filterout(float *smp)
{
    for(int i = 0; i < stages + 1; ++i)
        singlefilterout(smp, history[i], coeff);

    if(needsinterpolation) {
        //Merge Filter at old coeff with new coeff
        float *ismp = getTmpBuffer();
        memcpy(ismp, smp, synth->bufferbytes);

        for(int i = 0; i < stages + 1; ++i)
            singlefilterout(ismp, oldHistory[i], oldCoeff);

        for(int i = 0; i < synth->buffersize; ++i) {
            float x = (float)i / synth->buffersize_f;
            smp[i] = ismp[i] * (1.0f - x) + smp[i] * x;
        }
        returnTmpBuffer(ismp);
        needsinterpolation = false;
    }

    for(int i = 0; i < synth->buffersize; ++i)
        smp[i] *= outgain;
}

float AnalogFilter::H(float freq)
{
    float fr = freq / synth->samplerate_f * PI * 2.0f;
    float x  = coeff.c[0], y = 0.0f;
    for(int n = 1; n < 3; ++n) {
        x += cosf(n * fr) * coeff.c[n];
        y -= sinf(n * fr) * coeff.c[n];
    }
    float h = x * x + y * y;
    x = 1.0f;
    y = 0.0f;
    for(int n = 1; n < 3; ++n) {
        x -= cosf(n * fr) * coeff.d[n];
        y += sinf(n * fr) * coeff.d[n];
    }
    h = h / (x * x + y * y);
    return powf(h, (stages + 1.0f) / 2.0f);
}
