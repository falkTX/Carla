/*
  ZynAddSubFX - a software synthesizer

  ModFilter.cpp - Modulated Filter
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "ModFilter.h"
#include "Envelope.h"
#include "LFO.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Params/FilterParams.h"
#include "../DSP/Filter.h"
#include "../DSP/SVFilter.h"
#include "../DSP/AnalogFilter.h"
#include "../DSP/FormantFilter.h"
#include <cassert>

ModFilter::ModFilter(const FilterParams &pars_,
                     const SYNTH_T      &synth_,
                     const AbsTime      &time_,
                           Allocator    &alloc_,
                           bool         stereo,
                           float        notefreq)
    :pars(pars_), synth(synth_), time(time_), alloc(alloc_),
    baseQ(pars.getq()), baseFreq(pars.getfreq()),
    noteFreq(notefreq),
    left(nullptr), 
    right(nullptr),
    env(nullptr),
    lfo(nullptr)  
{
    tracking = pars.getfreqtracking(notefreq);
    baseQ    = pars.getq();
    baseFreq = pars.getfreq();

    left = Filter::generate(alloc, &pars,
            synth.samplerate, synth.buffersize);

    if(stereo)
        right = Filter::generate(alloc, &pars,
                synth.samplerate, synth.buffersize);
}

ModFilter::~ModFilter(void)
{
    alloc.dealloc(left);
    alloc.dealloc(right);
}
        
void ModFilter::addMod(LFO &lfo_)
{
    lfo = &lfo_;
}

void ModFilter::addMod(Envelope &env_)
{
    env = &env_;
}

//Recompute Filter Parameters
void ModFilter::update(float relfreq, float relq)
{
    if(pars.last_update_timestamp == time.time()) {
        paramUpdate(left);
        if(right)
            paramUpdate(right);

        baseFreq = pars.getfreq();
        baseQ    = pars.getq();
        tracking = pars.getfreqtracking(noteFreq);
    }

    //Controller Free Center Frequency
    const float Fc = baseFreq
                   + sense
                   + (env ? env->envout() : 0)
                   + (lfo ? lfo->lfoout() : 0);

    const float Fc_mod = Fc + relfreq + tracking;

    //Convert into Hz
    const float Fc_Hz = Filter::getrealfreq(Fc_mod);

    const float q = baseQ * relq;

    left->setfreq_and_q(Fc_Hz, q);
    if(right)
        right->setfreq_and_q(Fc_Hz, q);
}

void ModFilter::updateNoteFreq(float noteFreq_)
{
    noteFreq = noteFreq_;
    tracking = pars.getfreqtracking(noteFreq);
}

void ModFilter::updateSense(float velocity, uint8_t scale,
        uint8_t func)
{
    const float velScale = scale / 127.0f;
    sense = velScale * 6.0f * (VelF(velocity, func) - 1);
}
        
void ModFilter::filter(float *l, float *r)
{
    if(left && l)
        left->filterout(l);
    if(right && r)
        right->filterout(r);
}

static int current_category(Filter *f)
{
    if(dynamic_cast<AnalogFilter*>(f))
        return 0;
    else if(dynamic_cast<FormantFilter*>(f))
        return 1;
    else if(dynamic_cast<SVFilter*>(f))
        return 2;

    assert(false);
    return -1;
}

void ModFilter::paramUpdate(Filter *&f)
{
    //Common parameters
    baseQ    = pars.getq();
    baseFreq = pars.getfreq();
    
    if(current_category(f) != pars.Pcategory) {
        alloc.dealloc(f);
        f = Filter::generate(alloc, &pars,
                synth.samplerate, synth.buffersize);
        return;
    }

    if(auto *sv = dynamic_cast<SVFilter*>(f))
        svParamUpdate(*sv);
    else if(auto *an = dynamic_cast<AnalogFilter*>(f))
        anParamUpdate(*an);
}

void ModFilter::svParamUpdate(SVFilter &sv)
{
    sv.settype(pars.Ptype);
    sv.setstages(pars.Pstages);
}

void ModFilter::anParamUpdate(AnalogFilter &an)
{
    an.settype(pars.Ptype);
    an.setstages(pars.Pstages);
    an.setgain(pars.getgain());
}
