/*
  ZynAddSubFX - a software synthesizer

  LFOParams.cpp - Parameters for LFO
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <cstdio>
#include "../globals.h"
#include "../Misc/Util.h"
#include "../Misc/XMLwrapper.h"
#include "../Misc/Time.h"
#include "LFOParams.h"

#include <rtosc/port-sugar.h>
#include <rtosc/ports.h>
using namespace rtosc;


#define rObject LFOParams
#undef rChangeCb
#define rChangeCb if (obj->time) { obj->last_update_timestamp = obj->time->time(); }
static const rtosc::Ports _ports = {
    rSelf(LFOParams),
    rPaste,
    rParamF(Pfreq, rShort("freq"), rLinear(0.0,1.0), "frequency of LFO\n"
            "lfo frequency = (2^(10*Pfreq)-1)/12 * stretch\n"
            "true frequency is [0,85.33] Hz"),
    rParamZyn(Pintensity, rShort("depth"), "Intensity of LFO"),
    rParamZyn(Pstartphase, rShort("start"), rSpecial(random), "Starting Phase"),
    rOption(PLFOtype, rShort("type"), rOptions(sine, triangle, square, ramp-up, ramp-down,
                exponential-down1, exponential-down2), "Shape of LFO"),
    rParamZyn(Prandomness, rShort("a.r."), rSpecial(disable),
            "Amplitude Randomness (calculated uniformly at each cycle)"),
    rParamZyn(Pfreqrand, rShort("f.r."), rSpecial(disable),
            "Frequency Randomness (calculated uniformly at each cycle)"),
    rParamZyn(Pdelay, rShort("delay"), rSpecial(disable), "Delay before LFO start\n"
            "0..4 second delay"),
    rToggle(Pcontinous, rShort("c"), "Enable for global operation"),
    rParamZyn(Pstretch, rShort("str"), rCentered, "Note frequency stretch"),
};
#undef rChangeCb

const rtosc::Ports &LFOParams::ports = _ports;

LFOParams::LFOParams(const AbsTime *time_) : time(time_)
{
    Dfreq       = 64;
    Dintensity  = 0;
    Dstartphase = 0;
    DLFOtype    = 0;
    Drandomness = 0;
    Ddelay      = 0;
    Dcontinous  = 0;
    fel  = 0;

    defaults();
}

LFOParams::LFOParams(char Pfreq_,
                     char Pintensity_,
                     char Pstartphase_,
                     char PLFOtype_,
                     char Prandomness_,
                     char Pdelay_,
                     char Pcontinous_,
                     char fel_,
                     const AbsTime *time_) : time(time_),
                                             last_update_timestamp(0) {
    switch(fel_) {
        case 0:
            setpresettype("Plfofrequency");
            break;
        case 1:
            setpresettype("Plfoamplitude");
            break;
        case 2:
            setpresettype("Plfofilter");
            break;
    }
    Dfreq       = Pfreq_;
    Dintensity  = Pintensity_;
    Dstartphase = Pstartphase_;
    DLFOtype    = PLFOtype_;
    Drandomness = Prandomness_;
    Ddelay      = Pdelay_;
    Dcontinous  = Pcontinous_;
    fel  = fel_;

    defaults();
}

LFOParams::~LFOParams()
{}

void LFOParams::defaults()
{
    Pfreq       = Dfreq / 127.0f;
    Pintensity  = Dintensity;
    Pstartphase = Dstartphase;
    PLFOtype    = DLFOtype;
    Prandomness = Drandomness;
    Pdelay      = Ddelay;
    Pcontinous  = Dcontinous;
    Pfreqrand   = 0;
    Pstretch    = 64;
}


void LFOParams::add2XML(XMLwrapper& xml)
{
    xml.addparreal("freq", Pfreq);
    xml.addpar("intensity", Pintensity);
    xml.addpar("start_phase", Pstartphase);
    xml.addpar("lfo_type", PLFOtype);
    xml.addpar("randomness_amplitude", Prandomness);
    xml.addpar("randomness_frequency", Pfreqrand);
    xml.addpar("delay", Pdelay);
    xml.addpar("stretch", Pstretch);
    xml.addparbool("continous", Pcontinous);
}

void LFOParams::getfromXML(XMLwrapper& xml)
{
    Pfreq       = xml.getparreal("freq", Pfreq, 0.0f, 1.0f);
    Pintensity  = xml.getpar127("intensity", Pintensity);
    Pstartphase = xml.getpar127("start_phase", Pstartphase);
    PLFOtype    = xml.getpar127("lfo_type", PLFOtype);
    Prandomness = xml.getpar127("randomness_amplitude", Prandomness);
    Pfreqrand   = xml.getpar127("randomness_frequency", Pfreqrand);
    Pdelay      = xml.getpar127("delay", Pdelay);
    Pstretch    = xml.getpar127("stretch", Pstretch);
    Pcontinous  = xml.getparbool("continous", Pcontinous);
}

#define COPY(y) this->y=x.y
void LFOParams::paste(LFOParams &x)
{
    COPY(Pfreq);
    COPY(Pintensity);
    COPY(Pstartphase);
    COPY(PLFOtype);
    COPY(Prandomness);
    COPY(Pfreqrand);
    COPY(Pdelay);
    COPY(Pcontinous);
    COPY(Pstretch);

    if ( time ) {
        last_update_timestamp = time->time();
    }
}
#undef COPY
