/*
  ZynAddSubFX - a software synthesizer

  EQ.cpp - EQ effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include "EQ.h"
#include "../DSP/AnalogFilter.h"
#include "../Misc/Allocator.h"

using rtosc::RtData;
#define rObject EQ
#define rBegin [](const char *msg, RtData &d) {\
    rObject *obj = (rObject*)d.obj;
#define rEQ(offset) \
    int nfilt = atoi(msg-2); \
    int id    = 10+nfilt*5+offset; \
    if(rtosc_narguments(msg)) \
        obj->changepar(id, rtosc_argument(msg,0).i);\
    else \
        d.reply(d.loc, "i", obj->getpar(id))

#define rEnd }

static rtosc::Ports filterports {
    {"Ptype::i", rProp(parameter) rOptions(Off, LP1, HP1, LP2,
            HP2, BP, notch, peak, l.shelf, h.shelf)
        rShort("type") rDoc("Filter Type"), 0,
        rBegin;
        rEQ(0);
        rEnd},
    {"Pfreq::i", rProp(parameter) rMap(min, 0) rMap(max, 127)
        rShort("freq"), 0,
        rBegin;
        rEQ(1);
        rEnd},
    {"Pgain::i", rProp(parameter) rMap(min, 0) rMap(max, 127)
        rShort("gain"), 0,
        rBegin;
        rEQ(2);
        rEnd},
    {"Pq::i",    rProp(parameter) rMap(min, 0) rMap(max, 127)
        rShort("q") rDoc("Resonance/Bandwidth"), 0,
        rBegin;
        rEQ(3);
        rEnd},
    {"Pstages::i", rProp(parameter) rMap(min, 0) rMap(max, 4)
        rShort("stages") rDoc("Additional filter stages"), 0,
        rBegin;
        rEQ(4);
        rEnd},
};

rtosc::Ports EQ::ports = {
    {"filter#8/", 0, &filterports,
        rBegin;
        (void)obj;
        SNIP;
        filterports.dispatch(msg, d);
        rEnd},
    {"coeff:", rProp(internal) rDoc("Get equalizer Coefficients"), NULL,
        [](const char *, rtosc::RtData &d)
        {
            EQ *eq = (EQ*)d.obj;
            float a[MAX_EQ_BANDS*MAX_FILTER_STAGES*3];
            float b[MAX_EQ_BANDS*MAX_FILTER_STAGES*3];
            memset(a, 0, sizeof(a));
            memset(b, 0, sizeof(b));
            eq->getFilter(a,b);

            char        type[MAX_EQ_BANDS*MAX_FILTER_STAGES*3*2+1] = {0};
            rtosc_arg_t  val[MAX_EQ_BANDS*MAX_FILTER_STAGES*3*2] = {0};
            for(int i=0; i<MAX_EQ_BANDS*MAX_FILTER_STAGES*3; ++i) {
                int stride = MAX_EQ_BANDS*MAX_FILTER_STAGES*3;
                type[i]  = type[i+stride] = 'f';
                val[i].f = b[i];
                val[i+stride].f = a[i];
            }
            d.replyArray(d.loc, type, val);
        }},
};

#undef rObject
#undef rBegin
#undef rEnd

EQ::EQ(EffectParams pars)
    :Effect(pars)
{
    for(int i = 0; i < MAX_EQ_BANDS; ++i) {
        filter[i].Ptype   = 0;
        filter[i].Pfreq   = 64;
        filter[i].Pgain   = 64;
        filter[i].Pq      = 64;
        filter[i].Pstages = 0;
        filter[i].l = memory.alloc<AnalogFilter>(6, 1000.0f, 1.0f, 0, pars.srate, pars.bufsize);
        filter[i].r = memory.alloc<AnalogFilter>(6, 1000.0f, 1.0f, 0, pars.srate, pars.bufsize);
    }
    //default values
    Pvolume = 50;

    setpreset(Ppreset);
    cleanup();
}

EQ::~EQ()
{
       for(int i = 0; i < MAX_EQ_BANDS; ++i) {
           memory.dealloc(filter[i].l);
           memory.dealloc(filter[i].r);
       }
}

// Cleanup the effect
void EQ::cleanup(void)
{
    for(int i = 0; i < MAX_EQ_BANDS; ++i) {
        filter[i].l->cleanup();
        filter[i].r->cleanup();
    }
}

//Effect output
void EQ::out(const Stereo<float *> &smp)
{
    for(int i = 0; i < buffersize; ++i) {
        efxoutl[i] = smp.l[i] * volume;
        efxoutr[i] = smp.r[i] * volume;
    }

    for(int i = 0; i < MAX_EQ_BANDS; ++i) {
        if(filter[i].Ptype == 0)
            continue;
        filter[i].l->filterout(efxoutl);
        filter[i].r->filterout(efxoutr);
    }
}


//Parameter control
void EQ::setvolume(unsigned char _Pvolume)
{
    Pvolume   = _Pvolume;
    outvolume = powf(0.005f, (1.0f - Pvolume / 127.0f)) * 10.0f;
    volume    = (!insertion) ? 1.0f : outvolume;
}


void EQ::setpreset(unsigned char npreset)
{
    const int     PRESET_SIZE = 1;
    const int     NUM_PRESETS = 2;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        {67}, //EQ 1
        {67}  //EQ 2
    };

    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n < PRESET_SIZE; ++n)
        changepar(n, presets[npreset][n]);
    Ppreset = npreset;
}


void EQ::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
    }
    if(npar < 10)
        return;

    int nb = (npar - 10) / 5; //number of the band (filter)
    if(nb >= MAX_EQ_BANDS)
        return;
    int bp = npar % 5; //band paramenter

    float tmp;
    switch(bp) {
        case 0:
            filter[nb].Ptype = value;
            if(value > 9)
                filter[nb].Ptype = 0;  //has to be changed if more filters will be added
            if(filter[nb].Ptype != 0) {
                filter[nb].l->settype(value - 1);
                filter[nb].r->settype(value - 1);
            }
            break;
        case 1:
            filter[nb].Pfreq = value;
            tmp = 600.0f * powf(30.0f, (value - 64.0f) / 64.0f);
            filter[nb].l->setfreq(tmp);
            filter[nb].r->setfreq(tmp);
            break;
        case 2:
            filter[nb].Pgain = value;
            tmp = 30.0f * (value - 64.0f) / 64.0f;
            filter[nb].l->setgain(tmp);
            filter[nb].r->setgain(tmp);
            break;
        case 3:
            filter[nb].Pq = value;
            tmp = powf(30.0f, (value - 64.0f) / 64.0f);
            filter[nb].l->setq(tmp);
            filter[nb].r->setq(tmp);
            break;
        case 4:
            filter[nb].Pstages = value;
            if(value >= MAX_FILTER_STAGES)
                filter[nb].Pstages = MAX_FILTER_STAGES - 1;
            filter[nb].l->setstages(value);
            filter[nb].r->setstages(value);
            break;
    }
}

unsigned char EQ::getpar(int npar) const
{
    switch(npar) {
        case 0:
            return Pvolume;
            break;
    }

    if(npar < 10)
        return 0;

    int nb = (npar - 10) / 5; //number of the band (filter)
    if(nb >= MAX_EQ_BANDS)
        return 0;
    int bp = npar % 5; //band paramenter
    switch(bp) {
        case 0:
            return filter[nb].Ptype;
            break;
        case 1:
            return filter[nb].Pfreq;
            break;
        case 2:
            return filter[nb].Pgain;
            break;
        case 3:
            return filter[nb].Pq;
            break;
        case 4:
            return filter[nb].Pstages;
            break;
        default: return 0; //in case of bogus parameter number
    }
}


float EQ::getfreqresponse(float freq)
{
    float resp = 1.0f;
    for(int i = 0; i < MAX_EQ_BANDS; ++i) {
        if(filter[i].Ptype == 0)
            continue;
        resp *= filter[i].l->H(freq);
    }
    return rap2dB(resp * outvolume);
}

//Not exactly the most efficient manner to derive the total taps, but it should
//be fast enough in practice
void EQ::getFilter(float *a, float *b) const
{
    a[0] = 1;
    b[0] = 1;
    off_t off=0;
    for(int i = 0; i < MAX_EQ_BANDS; ++i) {
        auto &F = filter[i];
        if(F.Ptype == 0)
            continue;
        const double Fb[3] = {F.l->coeff.c[0], F.l->coeff.c[1], F.l->coeff.c[2]};
        const double Fa[3] = {1.0f, -F.l->coeff.d[1], -F.l->coeff.d[2]};

        for(int j=0; j<F.Pstages+1; ++j) {
            for(int k=0; k<3; ++k) {
                a[off] = Fa[k];
                b[off] = Fb[k];
                off++;
            }
        }
    }
}
