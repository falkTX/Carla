/*
  ZynAddSubFX - a software synthesizer

  LFO.cpp - LFO implementation
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

#include "LFO.h"
#include "../Params/LFOParams.h"
#include "../Misc/Util.h"

#include <cstdlib>
#include <cstdio>
#include <cmath>

LFO::LFO(const LFOParams &lfopars, float basefreq, const AbsTime &t)
    :first_half(-1),
    delayTime(t, lfopars.Pdelay / 127.0f * 4.0f), //0..4 sec
    waveShape(lfopars.PLFOtype),
    deterministic(!lfopars.Pfreqrand),
    dt_(t.dt()),
    lfopars_(lfopars), basefreq_(basefreq)
{
    int stretch = lfopars.Pstretch;
    if(stretch == 0)
        stretch = 1;

    //max 2x/octave
    const float lfostretch = powf(basefreq / 440.0f, (stretch - 64.0f) / 63.0f);

    const float lfofreq =
        (powf(2, lfopars.Pfreq * 10.0f) - 1.0f) / 12.0f * lfostretch;
    phaseInc = fabs(lfofreq) * t.dt();

    if(!lfopars.Pcontinous) {
        if(lfopars.Pstartphase == 0)
            phase = RND;
        else
            phase = fmod((lfopars.Pstartphase - 64.0f) / 127.0f + 1.0f, 1.0f);
    }
    else {
        const float tmp = fmod(t.time() * phaseInc, 1.0f);
        phase = fmod((lfopars.Pstartphase - 64.0f) / 127.0f + 1.0f + tmp, 1.0f);
    }

    //Limit the Frequency(or else...)
    if(phaseInc > 0.49999999f)
        phaseInc = 0.499999999f;


    lfornd = limit(lfopars.Prandomness / 127.0f, 0.0f, 1.0f);
    lfofreqrnd = powf(lfopars.Pfreqrand / 127.0f, 2.0f) * 4.0f;

    switch(lfopars.fel) {
        case 1:
            lfointensity = lfopars.Pintensity / 127.0f;
            break;
        case 2:
            lfointensity = lfopars.Pintensity / 127.0f * 4.0f;
            break; //in octave
        default:
            lfointensity = powf(2, lfopars.Pintensity / 127.0f * 11.0f) - 1.0f; //in centi
            phase -= 0.25f; //chance the starting phase
            break;
    }

    amp1     = (1 - lfornd) + lfornd * RND;
    amp2     = (1 - lfornd) + lfornd * RND;
    incrnd   = nextincrnd = 1.0f;
    computeNextFreqRnd();
    computeNextFreqRnd(); //twice because I want incrnd & nextincrnd to be random
}

LFO::~LFO()
{}

float LFO::baseOut(const char waveShape, const float phase)
{
    switch(waveShape) {
        case LFO_TRIANGLE:
            if(phase >= 0.0f && phase < 0.25f)
                return 4.0f * phase;
            else if(phase > 0.25f && phase < 0.75f)
                return 2 - 4 * phase;
            else
                return 4.0f * phase - 4.0f;
            break;
        case LFO_SQUARE:
            if(phase < 0.5f)
                return -1;
            else
                return  1;
            break;
        case LFO_RAMPUP:    return (phase - 0.5f) * 2.0f;
        case LFO_RAMPDOWN:  return (0.5f - phase) * 2.0f;
        case LFO_EXP_DOWN1: return powf(0.05f, phase) * 2.0f - 1.0f;
        case LFO_EXP_DOWN2: return powf(0.001f, phase) * 2.0f - 1.0f;
        case LFO_RANDOM:
            if ((phase < 0.5) != first_half) {
                first_half = phase < 0.5;
                last_random = 2*RND-1;
            }
            return last_random;
        default:            return cosf(phase * 2.0f * PI); //LFO_SINE
    }
}


float LFO::lfoout()
{
    //update internals XXX TODO cleanup
    {
        waveShape = lfopars_.PLFOtype;
        int stretch = lfopars_.Pstretch;
        if(stretch == 0)
            stretch = 1;
        const float lfostretch = powf(basefreq_ / 440.0f, (stretch - 64.0f) / 63.0f);

        float lfofreq =
            (powf(2, lfopars_.Pfreq * 10.0f) - 1.0f) / 12.0f * lfostretch;

        phaseInc = fabs(lfofreq) * dt_;

        switch(lfopars_.fel) {
            case 1:
                lfointensity = lfopars_.Pintensity / 127.0f;
                break;
            case 2:
                lfointensity = lfopars_.Pintensity / 127.0f * 4.0f;
                break; //in octave
            default:
                lfointensity = powf(2, lfopars_.Pintensity / 127.0f * 11.0f) - 1.0f; //in centi
                //x -= 0.25f; //chance the starting phase
                break;
        }
    }

    float out = baseOut(waveShape, phase);

    if(waveShape == LFO_SINE || waveShape == LFO_TRIANGLE)
        out *= lfointensity * (amp1 + phase * (amp2 - amp1));
    else
        out *= lfointensity * amp2;

    if(delayTime.inFuture())
        return out;

    //Start oscillating
    if(deterministic)
        phase += phaseInc;
    else {
        const float tmp = (incrnd * (1.0f - phase) + nextincrnd * phase);
        phase += phaseInc * limit(tmp, 0.0f, 1.0f);
    }
    if(phase >= 1) {
        phase    = fmod(phase, 1.0f);
        amp1 = amp2;
        amp2 = (1 - lfornd) + lfornd * RND;

        computeNextFreqRnd();
    }
    return out;
}

/*
 * LFO out (for amplitude)
 */
float LFO::amplfoout()
{
    return limit(1.0f - lfointensity + lfoout(), -1.0f, 1.0f);
}


void LFO::computeNextFreqRnd()
{
    if(deterministic)
        return;
    incrnd     = nextincrnd;
    nextincrnd = powf(0.5f, lfofreqrnd) + RND * (powf(2.0f, lfofreqrnd) - 1.0f);
}
