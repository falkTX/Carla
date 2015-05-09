/*
  ZynAddSubFX - a software synthesizer

  FilterParams.cpp - Parameters for filter
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

#include "FilterParams.h"
#include "../Misc/Util.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <rtosc/rtosc.h>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>

using namespace rtosc;

// g++ 4.8 needs this variable saved separately, otherwise it segfaults
constexpr int sizeof_pvowels = sizeof(FilterParams::Pvowels);

#define rObject FilterParams::Pvowels_t::formants_t
static const rtosc::Ports subsubports = {
    rParamZyn(freq, "Formant frequency"),
    rParamZyn(amp,  "Strength of formant"),
    rParamZyn(q,    "Quality Factor"),
};
#undef rObject

static const rtosc::Ports subports = {
    {"Pformants#" STRINGIFY(FF_MAX_FORMANTS) "/", NULL, &subsubports,
        [](const char *msg, RtData &d) {
            const char *mm = msg;
            while(*mm && !isdigit(*mm)) ++mm;
            unsigned idx = atoi(mm);

            SNIP;
            FilterParams::Pvowels_t *obj = (FilterParams::Pvowels_t *) d.obj;
            d.obj = (void*) &obj->formants[idx];
            subsubports.dispatch(msg, d);
        }},
};

#define rObject FilterParams
#undef  rChangeCb
#define rChangeCb obj->changed = true;
const rtosc::Ports FilterParams::ports = {
    rSelf(FilterParams),
    rPaste,
    rArrayPaste,
    rParamZyn(Pcategory,   "Class of filter"),
    rParamZyn(Ptype,       "Filter Type"),
    rParamZyn(Pfreq,        "Center Freq"),
    rParamZyn(Pq,           "Quality Factor (resonance/bandwidth)"),
    rParamZyn(Pstages,      "Filter Stages + 1"),
    rParamZyn(Pfreqtrack,   "Frequency Tracking amount"),
    rParamZyn(Pgain,        "Output Gain"),
    rParamZyn(Pnumformants, "Number of formants to be used"),
    rParamZyn(Pformantslowness, "Rate that formants change"),
    rParamZyn(Pvowelclearness, "Cost for mixing vowels"),
    rParamZyn(Pcenterfreq,     "Center Freq (formant)"),
    rParamZyn(Poctavesfreq,    "Number of octaves for formant"),

    //TODO check if FF_MAX_SEQUENCE is acutally expanded or not
    rParamZyn(Psequencesize, rMap(max, FF_MAX_SEQUENCE), "Length of vowel sequence"),
    rParamZyn(Psequencestretch, "How modulators stretch the sequence"),
    rToggle(Psequencereversed, "If the modulator input is inverted"),

    //{"Psequence#" FF_MAX_SEQUENCE "/nvowel", "", NULL, [](){}},

    //UI reader
    {"Pvowels:", rDoc("Get Formant Vowels"), NULL,
        [](const char *, RtData &d) {
            FilterParams *obj = (FilterParams *) d.obj;
            d.reply(d.loc, "b", sizeof_pvowels, obj->Pvowels);
        }},

    {"Pvowels#" STRINGIFY(FF_MAX_VOWELS) "/", NULL, &subports,
        [](const char *msg, RtData &d) {
            const char *mm = msg; \
            while(*mm && !isdigit(*mm)) ++mm; \
            unsigned idx = atoi(mm);

            SNIP;
            FilterParams *obj = (FilterParams *) d.obj;
            d.obj = (void*)&obj->Pvowels[idx];
            subports.dispatch(msg, d);

            if(rtosc_narguments(msg))
                rChangeCb;
        }},
    {"centerfreq:", NULL, NULL,
        [](const char *, RtData &d) {
            FilterParams *obj = (FilterParams *) d.obj;
            d.reply(d.loc, "f", obj->getcenterfreq());
        }},
    {"octavesfreq:", NULL, NULL,
        [](const char *, RtData &d) {
            FilterParams *obj = (FilterParams *) d.obj;
            d.reply(d.loc, "f", obj->getoctavesfreq());
        }},
    //    "", NULL, [](){}},"/freq"
    //{"Pvowels#" FF_MAX_VOWELS "/formants#" FF_MAX_FORMANTS "/amp",
    //    "", NULL, [](){}},
    //{"Pvowels#" FF_MAX_VOWELS "/formants#" FF_MAX_FORMANTS "/q",
    //    "", NULL, [](){}},
};
#undef rChangeCb
#define rChangeCb



FilterParams::FilterParams()
    :FilterParams(0,64,64)
{
}
FilterParams::FilterParams(unsigned char Ptype_,
                           unsigned char Pfreq_,
                           unsigned char Pq_)
{
    setpresettype("Pfilter");
    Dtype = Ptype_;
    Dfreq = Pfreq_;
    Dq    = Pq_;

    changed = false;
    defaults();
}

FilterParams::~FilterParams()
{}


void FilterParams::defaults()
{
    Ptype = Dtype;
    Pfreq = Dfreq;
    Pq    = Dq;

    Pstages    = 0;
    Pfreqtrack = 64;
    Pgain      = 64;
    Pcategory  = 0;

    Pnumformants     = 3;
    Pformantslowness = 64;
    for(int j = 0; j < FF_MAX_VOWELS; ++j)
        defaults(j);
    ;

    Psequencesize = 3;
    for(int i = 0; i < FF_MAX_SEQUENCE; ++i)
        Psequence[i].nvowel = i % FF_MAX_VOWELS;

    Psequencestretch  = 40;
    Psequencereversed = 0;
    Pcenterfreq     = 64; //1 kHz
    Poctavesfreq    = 64;
    Pvowelclearness = 64;
}

void FilterParams::defaults(int n)
{
    int j = n;
    for(int i = 0; i < FF_MAX_FORMANTS; ++i) {
        Pvowels[j].formants[i].freq = (int)(RND * 127.0f); //some random freqs
        Pvowels[j].formants[i].q    = 64;
        Pvowels[j].formants[i].amp  = 127;
    }
}


/*
 * Get the parameters from other FilterParams
 */

void FilterParams::getfromFilterParams(FilterParams *pars)
{
    defaults();

    if(pars == NULL)
        return;

    Ptype = pars->Ptype;
    Pfreq = pars->Pfreq;
    Pq    = pars->Pq;

    Pstages    = pars->Pstages;
    Pfreqtrack = pars->Pfreqtrack;
    Pgain      = pars->Pgain;
    Pcategory  = pars->Pcategory;

    Pnumformants     = pars->Pnumformants;
    Pformantslowness = pars->Pformantslowness;
    for(int j = 0; j < FF_MAX_VOWELS; ++j)
        for(int i = 0; i < FF_MAX_FORMANTS; ++i) {
            Pvowels[j].formants[i].freq = pars->Pvowels[j].formants[i].freq;
            Pvowels[j].formants[i].q    = pars->Pvowels[j].formants[i].q;
            Pvowels[j].formants[i].amp  = pars->Pvowels[j].formants[i].amp;
        }

    Psequencesize = pars->Psequencesize;
    for(int i = 0; i < FF_MAX_SEQUENCE; ++i)
        Psequence[i].nvowel = pars->Psequence[i].nvowel;

    Psequencestretch  = pars->Psequencestretch;
    Psequencereversed = pars->Psequencereversed;
    Pcenterfreq     = pars->Pcenterfreq;
    Poctavesfreq    = pars->Poctavesfreq;
    Pvowelclearness = pars->Pvowelclearness;
}


/*
 * Parameter control
 */
float FilterParams::getfreq()
{
    return (Pfreq / 64.0f - 1.0f) * 5.0f;
}

float FilterParams::getq()
{
    return expf(powf((float) Pq / 127.0f, 2) * logf(1000.0f)) - 0.9f;
}
float FilterParams::getfreqtracking(float notefreq)
{
    return logf(notefreq / 440.0f) * (Pfreqtrack - 64.0f) / (64.0f * LOG_2);
}

float FilterParams::getgain()
{
    return (Pgain / 64.0f - 1.0f) * 30.0f; //-30..30dB
}

/*
 * Get the center frequency of the formant's graph
 */
float FilterParams::getcenterfreq()
{
    return 10000.0f * powf(10, -(1.0f - Pcenterfreq / 127.0f) * 2.0f);
}

/*
 * Get the number of octave that the formant functions applies to
 */
float FilterParams::getoctavesfreq()
{
    return 0.25f + 10.0f * Poctavesfreq / 127.0f;
}

/*
 * Get the frequency from x, where x is [0..1]
 */
float FilterParams::getfreqx(float x)
{
    if(x > 1.0f)
        x = 1.0f;
    float octf = powf(2.0f, getoctavesfreq());
    return getcenterfreq() / sqrt(octf) * powf(octf, x);
}

/*
 * Get the x coordinate from frequency (used by the UI)
 */
float FilterParams::getfreqpos(float freq)
{
    return (logf(freq) - logf(getfreqx(0.0f))) / logf(2.0f) / getoctavesfreq();
}

/*
 * Transforms a parameter to the real value
 */
float FilterParams::getformantfreq(unsigned char freq)
{
    float result = getfreqx(freq / 127.0f);
    return result;
}

float FilterParams::getformantamp(unsigned char amp)
{
    float result = powf(0.1f, (1.0f - amp / 127.0f) * 4.0f);
    return result;
}

float FilterParams::getformantq(unsigned char q)
{
    //temp
    float result = powf(25.0f, (q - 32.0f) / 64.0f);
    return result;
}



void FilterParams::add2XMLsection(XMLwrapper *xml, int n)
{
    int nvowel = n;
    for(int nformant = 0; nformant < FF_MAX_FORMANTS; ++nformant) {
        xml->beginbranch("FORMANT", nformant);
        xml->addpar("freq", Pvowels[nvowel].formants[nformant].freq);
        xml->addpar("amp", Pvowels[nvowel].formants[nformant].amp);
        xml->addpar("q", Pvowels[nvowel].formants[nformant].q);
        xml->endbranch();
    }
}

void FilterParams::add2XML(XMLwrapper *xml)
{
    //filter parameters
    xml->addpar("category", Pcategory);
    xml->addpar("type", Ptype);
    xml->addpar("freq", Pfreq);
    xml->addpar("q", Pq);
    xml->addpar("stages", Pstages);
    xml->addpar("freq_track", Pfreqtrack);
    xml->addpar("gain", Pgain);

    //formant filter parameters
    if((Pcategory == 1) || (!xml->minimal)) {
        xml->beginbranch("FORMANT_FILTER");
        xml->addpar("num_formants", Pnumformants);
        xml->addpar("formant_slowness", Pformantslowness);
        xml->addpar("vowel_clearness", Pvowelclearness);
        xml->addpar("center_freq", Pcenterfreq);
        xml->addpar("octaves_freq", Poctavesfreq);
        for(int nvowel = 0; nvowel < FF_MAX_VOWELS; ++nvowel) {
            xml->beginbranch("VOWEL", nvowel);
            add2XMLsection(xml, nvowel);
            xml->endbranch();
        }
        xml->addpar("sequence_size", Psequencesize);
        xml->addpar("sequence_stretch", Psequencestretch);
        xml->addparbool("sequence_reversed", Psequencereversed);
        for(int nseq = 0; nseq < FF_MAX_SEQUENCE; ++nseq) {
            xml->beginbranch("SEQUENCE_POS", nseq);
            xml->addpar("vowel_id", Psequence[nseq].nvowel);
            xml->endbranch();
        }
        xml->endbranch();
    }
}


void FilterParams::getfromXMLsection(XMLwrapper *xml, int n)
{
    int nvowel = n;
    for(int nformant = 0; nformant < FF_MAX_FORMANTS; ++nformant) {
        if(xml->enterbranch("FORMANT", nformant) == 0)
            continue;
        Pvowels[nvowel].formants[nformant].freq = xml->getpar127(
            "freq",
            Pvowels[nvowel
            ].formants[nformant].freq);
        Pvowels[nvowel].formants[nformant].amp = xml->getpar127(
            "amp",
            Pvowels[nvowel
            ].formants[nformant].amp);
        Pvowels[nvowel].formants[nformant].q =
            xml->getpar127("q", Pvowels[nvowel].formants[nformant].q);
        xml->exitbranch();
    }
}

void FilterParams::getfromXML(XMLwrapper *xml)
{
    //filter parameters
    Pcategory = xml->getpar127("category", Pcategory);
    Ptype     = xml->getpar127("type", Ptype);
    Pfreq     = xml->getpar127("freq", Pfreq);
    Pq         = xml->getpar127("q", Pq);
    Pstages    = xml->getpar127("stages", Pstages);
    Pfreqtrack = xml->getpar127("freq_track", Pfreqtrack);
    Pgain      = xml->getpar127("gain", Pgain);

    //formant filter parameters
    if(xml->enterbranch("FORMANT_FILTER")) {
        Pnumformants     = xml->getpar127("num_formants", Pnumformants);
        Pformantslowness = xml->getpar127("formant_slowness", Pformantslowness);
        Pvowelclearness  = xml->getpar127("vowel_clearness", Pvowelclearness);
        Pcenterfreq      = xml->getpar127("center_freq", Pcenterfreq);
        Poctavesfreq     = xml->getpar127("octaves_freq", Poctavesfreq);

        for(int nvowel = 0; nvowel < FF_MAX_VOWELS; ++nvowel) {
            if(xml->enterbranch("VOWEL", nvowel) == 0)
                continue;
            getfromXMLsection(xml, nvowel);
            xml->exitbranch();
        }
        Psequencesize     = xml->getpar127("sequence_size", Psequencesize);
        Psequencestretch  = xml->getpar127("sequence_stretch", Psequencestretch);
        Psequencereversed = xml->getparbool("sequence_reversed",
                                            Psequencereversed);
        for(int nseq = 0; nseq < FF_MAX_SEQUENCE; ++nseq) {
            if(xml->enterbranch("SEQUENCE_POS", nseq) == 0)
                continue;
            Psequence[nseq].nvowel = xml->getpar("vowel_id",
                                                 Psequence[nseq].nvowel,
                                                 0,
                                                 FF_MAX_VOWELS - 1);
            xml->exitbranch();
        }
        xml->exitbranch();
    }
}

void FilterParams::paste(FilterParams &x)
{
    //Avoid undefined behavior
    if(&x == this)
        return;
    memcpy((char*)this, (const char*)&x, sizeof(*this));
}

void FilterParams::pasteArray(FilterParams &x, int nvowel)
{
    printf("FilterParameters::pasting-an-array<%d>\n", nvowel);
    for(int nformant = 0; nformant < FF_MAX_FORMANTS; ++nformant) {
        auto &self   = Pvowels[nvowel].formants[nformant];
        auto &update = x.Pvowels[nvowel].formants[nformant];
        self.freq = update.freq;
        self.amp  = update.amp;
        self.q    = update.q;
    }
}
