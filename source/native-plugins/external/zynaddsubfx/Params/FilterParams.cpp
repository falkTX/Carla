/*
  ZynAddSubFX - a software synthesizer

  FilterParams.cpp - Parameters for filter
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2017      Mark McCurry
  Author: Nasca Octavian Paul
          Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "FilterParams.h"
#include "../Misc/Util.h"
#include "../Misc/Time.h"
#include "../DSP/AnalogFilter.h"
#include "../DSP/SVFilter.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <rtosc/rtosc.h>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
using namespace rtosc;

namespace zyncarla {

// g++ 4.8 needs this variable saved separately, otherwise it segfaults
constexpr int sizeof_pvowels = sizeof(FilterParams::Pvowels);

#define rObject FilterParams::Pvowels_t::formants_t

static const rtosc::Ports subsubports = {
    rParamZyn(freq, rShort("f.freq"), "Formant frequency"),
    rParamZyn(amp,  rShort("f.str"),  "Strength of formant"),
    rParamZyn(q,    rShort("f.q"),    "The formant's quality factor, also known as resonance bandwidth or Q for short"),
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
#define rChangeCb obj->changed = true; if ( obj->time) {      \
    obj->last_update_timestamp = obj->time->time(); }
const rtosc::Ports FilterParams::ports = {
    rSelf(FilterParams),
    rPaste,
    rArrayPaste,
    rOption(Pcategory,          rShort("class"),
            rOptions(analog, formant, st.var.), "Class of filter"),
    rOption(Ptype,              rShort("type"),
            rOptions(LP1, HP1, LP2, HP2, BP, notch, peak,
                l.shelf, h.shelf), "Filter Type"),
    rParamI(Pstages,            rShort("stages"),
            rLinear(0,5),  "Filter Stages"),
    rParamF(baseq,               rShort("q"),      rUnit(none),  rLog(0.1, 1000),
            "Quality Factor (resonance/bandwidth)"),
    rParamF(basefreq,           rShort("cutoff"),  rUnit(Hz),    rLog(31.25, 32000),
            "Base cutoff frequency"),
    rParamF(freqtracking,       rShort("f.track"), rUnit(%),     rLinear(-100, 100),
            "Frequency Tracking amount"),
    rParamF(gain,               rShort("gain"),    rUnit(dB),    rLinear(-30, 30),
            "Output Gain"),
    rParamI(Pnumformants,       rShort("formants"),
            rLinear(1,12),  "Number of formants to be used"),
    rParamZyn(Pformantslowness, rShort("slew"),
            "Rate that formants change"),
    rParamZyn(Pvowelclearness,  rShort("clarity"),
            "How much each vowel is smudged with the next in sequence. A high clarity will avoid smudging."),
    rParamZyn(Pcenterfreq,      rShort("cutoff"),
            "Center Freq (formant)"),
    rParamZyn(Poctavesfreq,     rShort("octaves"),
            "Number of octaves for formant"),

    rParamI(Psequencesize,    rShort("seq.size"), rLinear(0, FF_MAX_SEQUENCE), "Length of vowel sequence"),
    rParamZyn(Psequencestretch, rShort("seq.str"), "How modulators stretch the sequence"),
    rToggle(Psequencereversed,  rShort("reverse"), "If the modulator input is inverted"),

    {"vowel_seq#" STRINGIFY(FF_MAX_SEQUENCE) "::i", rShort("vowel") rDoc("Vowel number of this sequence position"), NULL,
        [](const char *msg, RtData &d){
            FilterParams *obj = (FilterParams *) d.obj;
            const char *mm = msg;
            while(*mm && !isdigit(*mm)) ++mm;
            unsigned idx = atoi(mm);
            if(rtosc_narguments(msg)) {
                obj->Psequence[idx].nvowel = rtosc_argument(msg, 0).i;
                d.broadcast(d.loc, "i", obj->Psequence[idx].nvowel);
            } else
                d.reply(d.loc, "i", obj->Psequence[idx].nvowel);
        }},
    {"type-svf::i", rProp(parameter) rShort("type")
        rOptions(low, high, band, notch)
            rDoc("Filter Type"), 0, rOptionCb(Ptype)},

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
    {"centerfreq:", rDoc("Get the center frequency of the formant's graph"),
        NULL, [](const char *, RtData &d) {
            FilterParams *obj = (FilterParams *) d.obj;
            d.reply(d.loc, "f", obj->getcenterfreq());
        }},
    {"octavesfreq:",
        rDoc("Get the number of octave that the formant functions applies to"),
        NULL, [](const char *, RtData &d) {
            FilterParams *obj = (FilterParams *) d.obj;
            d.reply(d.loc, "f", obj->getoctavesfreq());
        }},
    {"q_value:",
        rDoc("Q value for UI Response Graphs"),
        NULL, [](const char *, RtData &d) {
            FilterParams *obj = (FilterParams *) d.obj;
            d.reply(d.loc, "f", obj->getq());
        }},
    {"response:",
        rDoc("Get a frequency response"),
        NULL, [](const char *, RtData &d) {
            FilterParams *obj = (FilterParams *) d.obj;
            if(obj->Pcategory == 0) {
                int order = 0;
                float gain = dB2rap(obj->getgain());
                if(obj->Ptype != 6 && obj->Ptype != 7 && obj->Ptype != 8)
                    gain = 1.0;
                auto cf = AnalogFilter::computeCoeff(obj->Ptype,
                        Filter::getrealfreq(obj->getfreq()),
                        obj->getq(), obj->Pstages,
                        gain, 48000, order);
                if(order == 2) {
                    d.reply(d.loc, "fffffff",
                            (float)obj->Pstages,
                            cf.c[0], cf.c[1], cf.c[2],
                            0.0,     cf.d[1], cf.d[2]);
                } else if(order == 1) {
                    d.reply(d.loc, "fffff",
                            (float)obj->Pstages,
                            cf.c[0], cf.c[1],
                            0.0,     cf.d[1]);
                }
            } else if(obj->Pcategory == 2) {
                float gain = dB2rap(obj->getgain());
                auto cf = SVFilter::computeResponse(obj->Ptype,
                        Filter::getrealfreq(obj->getfreq()),
                        obj->getq(), obj->Pstages,
                        gain, 48000);
                d.reply(d.loc, "fffffff",
                        (float)obj->Pstages,
                        cf.b[0], cf.b[1], cf.b[2],
                        0.0,     -cf.a[1], -cf.a[2]);
            }
        }},
    //    "", NULL, [](){}},"/freq"
    //{"Pvowels#" FF_MAX_VOWELS "/formants#" FF_MAX_FORMANTS "/amp",
    //    "", NULL, [](){}},
    //{"Pvowels#" FF_MAX_VOWELS "/formants#" FF_MAX_FORMANTS "/q",
    //    "", NULL, [](){}},
    //
        //struct Pvowels_t {
        //    struct formants_t {
        //        unsigned char freq, amp, q; //frequency,amplitude,Q
        //    } formants[FF_MAX_FORMANTS];
        //} Pvowels[FF_MAX_VOWELS];
    {"vowels:",
        rDoc("Get info for formant graph"),
        NULL, [](const char *, RtData &d) {
            FilterParams *obj = (FilterParams *) d.obj;

            rtosc_arg_t args[2+3*FF_MAX_FORMANTS*FF_MAX_VOWELS];
            char type[2+3*FF_MAX_FORMANTS*FF_MAX_VOWELS + 1] = {0};

            type[0] = 'i';
            type[1] = 'i';

            args[0].i = FF_MAX_VOWELS;
            args[1].i = FF_MAX_FORMANTS;


            for(int i=0; i<FF_MAX_VOWELS; ++i) {
                auto &val = obj->Pvowels[i];
                for(int j=0; j<FF_MAX_FORMANTS; ++j) {
                    auto &f = val.formants[j];
                    //each formant is 3 arguments
                    //each vowel is FF_MAX_FORMANTS * length of formants long
                    auto *a = args + i*FF_MAX_FORMANTS*3 + j*3 + 2;
                    auto *t = type + i*FF_MAX_FORMANTS*3 + j*3 + 2;
                    a[0].f = obj->getformantfreq(f.freq);
                    a[1].f = obj->getformantamp(f.amp);
                    a[2].f = obj->getformantq(f.q);
                    //printf("<%d,%d,%d,%d,%d,%f,%f,%f>\n", i, j, f.freq, f.amp, f.q, a[0].f, a[1].f, a[2].f);
                    t[0] = t[1] = t[2] = 'f';
                }
            }
            d.replyArray(d.loc, type, args);
        }},

    //Old 0..127 parameter mappings
    {"Pfreq::i",  rLinear(0, 127) rShort("cutoff")  rProp(deprecated)  rDoc("Center Freq"), 0,
        [](const char *msg, RtData &d) {
            FilterParams *obj = (FilterParams*)d.obj;
            if(rtosc_narguments(msg)) {
                int Pfreq = rtosc_argument(msg, 0).i;
                obj->basefreq  = (Pfreq / 64.0f - 1.0f) * 5.0f;
                obj->basefreq  = powf(2.0f, obj->basefreq + 9.96578428f);
                rChangeCb;
                d.broadcast(d.loc, "i", Pfreq);
            } else {
                float tmp = obj->basefreq;
                tmp = log2f(tmp) - 9.96578428f;
                tmp = (tmp / 5.0 + 1.0) * 64.0f;
                int Pfreq = roundf(tmp);
                d.reply(d.loc, "i", Pfreq);
            }
        }},
    {"Pfreqtrack::i", rLinear(0, 127) rShort("f.track") rProp(deprecated) rDoc("Frequency Tracking amount"), 0,
        [](const char *msg, RtData &d) {
            FilterParams *obj = (FilterParams*)d.obj;
            if(rtosc_narguments(msg)) {
                int Pfreqtracking = rtosc_argument(msg, 0).i;
                obj->freqtracking = 100 * (Pfreqtracking - 64.0f) / (64.0f);
                rChangeCb;
                d.broadcast(d.loc, "i", Pfreqtracking);
            } else {
                int Pfreqtracking = obj->freqtracking/100.0*64.0 + 64.0;
                d.reply(d.loc, "i", Pfreqtracking);
            }
        }},
    {"Pgain::i", rLinear(0, 127) rShort("gain") rProp(deprecated) rDoc("Output Gain"), 0,
        [](const char *msg, RtData &d) {
            FilterParams *obj = (FilterParams*)d.obj;
            if(rtosc_narguments(msg)) {
                int Pgain = rtosc_argument(msg, 0).i;
                obj->gain   = (Pgain / 64.0f - 1.0f) * 30.0f; //-30..30dB
                rChangeCb;
                d.broadcast(d.loc, "i", Pgain);
            } else {
                int Pgain = roundf((obj->gain/30.0f + 1.0f) * 64.0f);
                d.reply(d.loc, "i", Pgain);
            }
        }},
    {"Pq::i", rLinear(0,127) rShort("q") rProp(deprecated)
        rDoc("Quality Factor (resonance/bandwidth)"), 0,
        [](const char *msg, RtData &d) {
            FilterParams *obj = (FilterParams*)d.obj;
            if(rtosc_narguments(msg)) {
                int Pq = rtosc_argument(msg, 0).i;
                obj->baseq  = expf(powf((float) Pq / 127.0f, 2) * logf(1000.0f)) - 0.9f;
                rChangeCb;
                d.broadcast(d.loc, "i", Pq);
            } else {
                int Pq = roundf(127.0f * sqrtf(logf(0.9f + obj->baseq)/logf(1000.0f)));
                d.reply(d.loc, "i", Pq);
            }
        }},
};
#undef rChangeCb
#define rChangeCb



FilterParams::FilterParams(const AbsTime *time_)
    :FilterParams(0,64,64, time_)
{
}
FilterParams::FilterParams(unsigned char Ptype_,
                           unsigned char Pfreq_,
                           unsigned char Pq_,
                           const AbsTime *time_):
        time(time_), last_update_timestamp(0)
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

    Pstages       = 0;
    basefreq  = (Pfreq / 64.0f - 1.0f) * 5.0f;
    basefreq  = powf(2.0f, basefreq + 9.96578428f);
    baseq     = expf(powf((float) Pq / 127.0f, 2) * logf(1000.0f)) - 0.9f;

    gain = 0.0f;
    freqtracking = 0.0f;

    Pcategory     = 0;

    Pnumformants     = 3;
    Pformantslowness = 64;
    for(int j = 0; j < FF_MAX_VOWELS; ++j)
        defaults(j);


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

    Pstages       = pars->Pstages;
    freqtracking  = pars->freqtracking;
    gain          = pars->gain;
    Pcategory     = pars->Pcategory;

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
float FilterParams::getfreq() const
{
    return log2(basefreq) - log2f(1000.0f);
}

float FilterParams::getq() const
{
    return baseq;
}
float FilterParams::getfreqtracking(float notefreq) const
{
    return log2f(notefreq / 440.0f) * (freqtracking / 100.0);
}

float FilterParams::getgain() const
{
    return gain;
}

/*
 * Get the center frequency of the formant's graph
 */
float FilterParams::getcenterfreq() const
{
    return 10000.0f * powf(10, -(1.0f - Pcenterfreq / 127.0f) * 2.0f);
}

/*
 * Get the number of octave that the formant functions applies to
 */
float FilterParams::getoctavesfreq() const
{
    return 0.25f + 10.0f * Poctavesfreq / 127.0f;
}

/*
 * Get the frequency from x, where x is [0..1]
 */
float FilterParams::getfreqx(float x) const
{
    if(x > 1.0f)
        x = 1.0f;
    float octf = powf(2.0f, getoctavesfreq());
    return getcenterfreq() / sqrt(octf) * powf(octf, x);
}

/*
 * Get the x coordinate from frequency (used by the UI)
 */
float FilterParams::getfreqpos(float freq) const
{
    return (logf(freq) - logf(getfreqx(0.0f))) / logf(2.0f) / getoctavesfreq();
}

/*
 * Transforms a parameter to the real value
 */
float FilterParams::getformantfreq(unsigned char freq) const
{
    return getfreqx(freq / 127.0f);
}

float FilterParams::getformantamp(unsigned char amp) const
{
    return powf(0.1f, (1.0f - amp / 127.0f) * 4.0f);
}

float FilterParams::getformantq(unsigned char q) const
{
    //temp
    return  powf(25.0f, (q - 32.0f) / 64.0f);
}



void FilterParams::add2XMLsection(XMLwrapper& xml, int n)
{
    int nvowel = n;
    for(int nformant = 0; nformant < FF_MAX_FORMANTS; ++nformant) {
        xml.beginbranch("FORMANT", nformant);
        xml.addpar("freq", Pvowels[nvowel].formants[nformant].freq);
        xml.addpar("amp", Pvowels[nvowel].formants[nformant].amp);
        xml.addpar("q", Pvowels[nvowel].formants[nformant].q);
        xml.endbranch();
    }
}

void FilterParams::add2XML(XMLwrapper& xml)
{
    //filter parameters
    xml.addpar("category", Pcategory);
    xml.addpar("type", Ptype);
    xml.addparreal("basefreq", basefreq);
    xml.addparreal("baseq", baseq);
    xml.addpar("stages", Pstages);
    xml.addparreal("freq_tracking", freqtracking);
    xml.addparreal("gain",       gain);

    //formant filter parameters
    if((Pcategory == 1) || (!xml.minimal)) {
        xml.beginbranch("FORMANT_FILTER");
        xml.addpar("num_formants", Pnumformants);
        xml.addpar("formant_slowness", Pformantslowness);
        xml.addpar("vowel_clearness", Pvowelclearness);
        xml.addpar("center_freq", Pcenterfreq);
        xml.addpar("octaves_freq", Poctavesfreq);
        for(int nvowel = 0; nvowel < FF_MAX_VOWELS; ++nvowel) {
            xml.beginbranch("VOWEL", nvowel);
            add2XMLsection(xml, nvowel);
            xml.endbranch();
        }
        xml.addpar("sequence_size", Psequencesize);
        xml.addpar("sequence_stretch", Psequencestretch);
        xml.addparbool("sequence_reversed", Psequencereversed);
        for(int nseq = 0; nseq < FF_MAX_SEQUENCE; ++nseq) {
            xml.beginbranch("SEQUENCE_POS", nseq);
            xml.addpar("vowel_id", Psequence[nseq].nvowel);
            xml.endbranch();
        }
        xml.endbranch();
    }
}


void FilterParams::getfromXMLsection(XMLwrapper& xml, int n)
{
    int nvowel = n;
    for(int nformant = 0; nformant < FF_MAX_FORMANTS; ++nformant) {
        if(xml.enterbranch("FORMANT", nformant) == 0)
            continue;
        Pvowels[nvowel].formants[nformant].freq = xml.getpar127(
            "freq",
            Pvowels[nvowel
            ].formants[nformant].freq);
        Pvowels[nvowel].formants[nformant].amp = xml.getpar127(
            "amp",
            Pvowels[nvowel
            ].formants[nformant].amp);
        Pvowels[nvowel].formants[nformant].q =
            xml.getpar127("q", Pvowels[nvowel].formants[nformant].q);
        xml.exitbranch();
    }
}

void FilterParams::getfromXML(XMLwrapper& xml)
{
    const bool upgrade_3_0_2 = (xml.fileversion() < version_type(3,0,2)) && (xml.getparreal("basefreq", -1) < 0);

    //filter parameters
    Pcategory    = xml.getpar127("category", Pcategory);
    Ptype        = xml.getpar127("type", Ptype);
    Pstages      = xml.getpar127("stages", Pstages);
    if(upgrade_3_0_2) {
        int Pfreq = xml.getpar127("freq", 0);
        basefreq  = (Pfreq / 64.0f - 1.0f) * 5.0f;
        basefreq  = powf(2.0f, basefreq + 9.96578428f);
        int Pq    = xml.getpar127("q", 0);
        baseq     = expf(powf((float) Pq / 127.0f, 2) * logf(1000.0f)) - 0.9f;
        int Pgain = xml.getpar127("gain", 0);
        gain      = (Pgain / 64.0f - 1.0f) * 30.0f; //-30..30dB
        int Pfreqtracking = xml.getpar127("freq_track", 0);
        freqtracking = 100 * (Pfreqtracking - 64.0f) / (64.0f);
    } else {
        basefreq     = xml.getparreal("basefreq",   1000);
        baseq        = xml.getparreal("baseq",      10);
        gain         = xml.getparreal("gain",       0);
        freqtracking = xml.getparreal("freq_tracking", 0);
    }

    //formant filter parameters
    if(xml.enterbranch("FORMANT_FILTER")) {
        Pnumformants     = xml.getpar127("num_formants", Pnumformants);
        Pformantslowness = xml.getpar127("formant_slowness", Pformantslowness);
        Pvowelclearness  = xml.getpar127("vowel_clearness", Pvowelclearness);
        Pcenterfreq      = xml.getpar127("center_freq", Pcenterfreq);
        Poctavesfreq     = xml.getpar127("octaves_freq", Poctavesfreq);

        for(int nvowel = 0; nvowel < FF_MAX_VOWELS; ++nvowel) {
            if(xml.enterbranch("VOWEL", nvowel) == 0)
                continue;
            getfromXMLsection(xml, nvowel);
            xml.exitbranch();
        }
        Psequencesize     = xml.getpar127("sequence_size", Psequencesize);
        Psequencestretch  = xml.getpar127("sequence_stretch", Psequencestretch);
        Psequencereversed = xml.getparbool("sequence_reversed",
                                            Psequencereversed);
        for(int nseq = 0; nseq < FF_MAX_SEQUENCE; ++nseq) {
            if(xml.enterbranch("SEQUENCE_POS", nseq) == 0)
                continue;
            Psequence[nseq].nvowel = xml.getpar("vowel_id",
                                                 Psequence[nseq].nvowel,
                                                 0,
                                                 FF_MAX_VOWELS - 1);
            xml.exitbranch();
        }
        xml.exitbranch();
    }
}

#define COPY(y) this->y = x.y
void FilterParams::paste(FilterParams &x)
{
    COPY(Pcategory);
    COPY(Ptype);
    COPY(basefreq);
    COPY(Pq);
    COPY(Pstages);
    COPY(freqtracking);
    COPY(gain);

    COPY(Pnumformants);
    COPY(Pformantslowness);
    COPY(Pvowelclearness);
    COPY(Pcenterfreq);
    COPY(Poctavesfreq);

    for(int i=0; i<FF_MAX_VOWELS; ++i) {
        for(int j=0; j<FF_MAX_FORMANTS; ++j) {
            auto &a = this->Pvowels[i].formants[j];
            auto &b = x.Pvowels[i].formants[j];
            a.freq = b.freq;
            a.amp  = b.amp;
            a.q    = b.q;
        }
    }


    COPY(Psequencesize);
    COPY(Psequencestretch);
    COPY(Psequencereversed);
    for(int i=0; i<FF_MAX_SEQUENCE; ++i)
        this->Psequence[i] = x.Psequence[i];

    COPY(changed);

    if ( time ) {
        last_update_timestamp = time->time();
    }
}
#undef COPY

void FilterParams::pasteArray(FilterParams &x, int nvowel)
{
    for(int nformant = 0; nformant < FF_MAX_FORMANTS; ++nformant) {
        auto &self   = Pvowels[nvowel].formants[nformant];
        auto &update = x.Pvowels[nvowel].formants[nformant];
        self.freq = update.freq;
        self.amp  = update.amp;
        self.q    = update.q;
    }

    if ( time ) {
        last_update_timestamp = time->time();
    }
}

}
