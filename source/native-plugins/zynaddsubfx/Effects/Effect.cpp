/*
  ZynAddSubFX - a software synthesizer

  Effect.cpp - this class is inherited by the all effects(Reverb, Echo, ..)
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2011 Alan Calvert
  Copyright (C) 2015 Mark McCurry
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "Effect.h"
#include "../Params/FilterParams.h"
#include <cmath>

EffectParams::EffectParams(Allocator &alloc_, bool insertion_, float *efxoutl_, float *efxoutr_,
            unsigned char Ppreset_, unsigned int srate_, int bufsize_, FilterParams *filterpars_,
            bool filterprotect_)
    :alloc(alloc_), insertion(insertion_), efxoutl(efxoutl_), efxoutr(efxoutr_),
     Ppreset(Ppreset_), srate(srate_), bufsize(bufsize_), filterpars(filterpars_),
     filterprotect(filterprotect_)
{}
Effect::Effect(EffectParams pars)
    :Ppreset(pars.Ppreset),
      efxoutl(pars.efxoutl),
      efxoutr(pars.efxoutr),
      filterpars(pars.filterpars),
      insertion(pars.insertion),
      memory(pars.alloc),
      samplerate(pars.srate),
      buffersize(pars.bufsize)
{
    alias();
}

void Effect::out(float *const smpsl, float *const smpsr)
{
    out(Stereo<float *>(smpsl, smpsr));
}

void Effect::crossover(float &a, float &b, float crossover)
{
    float tmpa = a;
    float tmpb = b;
    a = tmpa * (1.0f - crossover) + tmpb * crossover;
    b = tmpb * (1.0f - crossover) + tmpa * crossover;
}

void Effect::setpanning(char Ppanning_)
{
    Ppanning = Ppanning_;
    float t = (Ppanning > 0) ? (float)(Ppanning - 1) / 126.0f : 0.0f;
    pangainL = cosf(t * PI / 2.0f);
    pangainR = cosf((1.0f - t) * PI / 2.0f);
}

void Effect::setlrcross(char Plrcross_)
{
    Plrcross = Plrcross_;
    lrcross  = (float)Plrcross / 127.0f;
}
