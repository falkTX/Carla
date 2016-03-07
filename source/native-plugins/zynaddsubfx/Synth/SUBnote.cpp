/*
  ZynAddSubFX - a software synthesizer

  SUBnote.cpp - The "subtractive" synthesizer
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <iostream>
#include "../globals.h"
#include "SUBnote.h"
#include "Envelope.h"
#include "ModFilter.h"
#include "../Params/Controller.h"
#include "../Params/SUBnoteParameters.h"
#include "../Params/FilterParams.h"
#include "../Misc/Time.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"

SUBnote::SUBnote(const SUBnoteParameters *parameters, SynthParams &spars)
    :SynthNote(spars), pars(*parameters),
    AmpEnvelope(nullptr),
    FreqEnvelope(nullptr),
    BandWidthEnvelope(nullptr),
    GlobalFilter(nullptr),
    GlobalFilterEnvelope(nullptr),
    NoteEnabled(true),
    lfilter(nullptr), rfilter(nullptr)
{
    setup(spars.frequency, spars.velocity, spars.portamento, spars.note);
}

float SUBnote::setupFilters(int *pos, bool automation)
{
    //how much the amplitude is normalised (because the harmonics)
    float reduceamp = 0.0f;

    for(int n = 0; n < numharmonics; ++n) {
        float freq =  basefreq * pars.POvertoneFreqMult[pos[n]];
        overtone_freq[n] = freq;
        overtone_rolloff[n] = computerolloff(freq);

        //the bandwidth is not absolute(Hz); it is relative to frequency
        float bw =
            powf(10, (pars.Pbandwidth - 127.0f) / 127.0f * 4) * numstages;

        //Bandwidth Scale
        bw *= powf(1000 / freq, (pars.Pbwscale - 64.0f) / 64.0f * 3.0f);

        //Relative BandWidth
        bw *= powf(100, (pars.Phrelbw[pos[n]] - 64.0f) / 64.0f);

        if(bw > 25.0f)
            bw = 25.0f;

        //try to keep same amplitude on all freqs and bw. (empirically)
        float gain = sqrt(1500.0f / (bw * freq));

        float hmagnew = 1.0f - pars.Phmag[pos[n]] / 127.0f;
        float hgain;

        switch(pars.Phmagtype) {
            case 1:
                hgain = expf(hmagnew * logf(0.01f));
                break;
            case 2:
                hgain = expf(hmagnew * logf(0.001f));
                break;
            case 3:
                hgain = expf(hmagnew * logf(0.0001f));
                break;
            case 4:
                hgain = expf(hmagnew * logf(0.00001f));
                break;
            default:
                hgain = 1.0f - hmagnew;
        }
        gain      *= hgain;
        reduceamp += hgain;

        for(int nph = 0; nph < numstages; ++nph) {
            float amp = 1.0f;
            if(nph == 0)
                amp = gain;
            initfilter(lfilter[nph + n * numstages], freq + OffsetHz, bw,
                       amp, hgain, automation);
            if(stereo)
                initfilter(rfilter[nph + n * numstages], freq + OffsetHz, bw,
                           amp, hgain, automation);
        }
    }

    if(reduceamp < 0.001f)
        reduceamp = 1.0f;

    return reduceamp;
}

void SUBnote::setup(float freq,
                    float velocity,
                    int portamento_,
                    int midinote,
                    bool legato)
{
    this->velocity = velocity;
    portamento  = portamento_;
    NoteEnabled = ON;
    volume      = powf(0.1f, 3.0f * (1.0f - pars.PVolume / 96.0f)); //-60 dB .. 0 dB
    volume     *= VelF(velocity, pars.PAmpVelocityScaleFunction);
    if(pars.PPanning != 0)
        panning = pars.PPanning / 127.0f;
    else
        panning = RND;

    if(!legato) { //normal note
        numstages = pars.Pnumstages;
        stereo    = pars.Pstereo;
        start     = pars.Pstart;
        firsttick = 1;
    }


    if(pars.Pfixedfreq == 0)
        basefreq = freq;
    else {
        basefreq = 440.0f;
        int fixedfreqET = pars.PfixedfreqET;
        if(fixedfreqET) { //if the frequency varies according the keyboard note
            float tmp = (midinote - 69.0f) / 12.0f
                * (powf(2.0f, (fixedfreqET - 1) / 63.0f) - 1.0f);
            if(fixedfreqET <= 64)
                basefreq *= powf(2.0f, tmp);
            else
                basefreq *= powf(3.0f, tmp);
        }
    }
    int BendAdj = pars.PBendAdjust - 64;
    if (BendAdj % 24 == 0)
        BendAdjust = BendAdj / 24;
    else
        BendAdjust = BendAdj / 24.0f;
    float offset_val = (pars.POffsetHz - 64)/64.0f;
    OffsetHz = 15.0f*(offset_val * sqrtf(fabsf(offset_val)));
    float detune = getdetune(pars.PDetuneType,
                             pars.PCoarseDetune,
                             pars.PDetune);
    basefreq *= powf(2.0f, detune / 1200.0f); //detune
//    basefreq*=ctl.pitchwheel.relfreq;//pitch wheel

    int pos[MAX_SUB_HARMONICS];
    int harmonics = 0;

    //select only harmonics that desire to compute
    for(int n = 0; n < MAX_SUB_HARMONICS; ++n) {
        if(pars.Phmag[n] == 0)
            continue;
        pos[harmonics++] = n;
    }
    if(!legato) //normal note
        firstnumharmonics = numharmonics = harmonics;
    else {
        if(harmonics > firstnumharmonics)
            numharmonics = firstnumharmonics;
        else
            numharmonics = harmonics;
    }


    if(numharmonics == 0) {
        NoteEnabled = false;
        return;
    }


    if(!legato) { //normal note
        lfilter = memory.valloc<bpfilter>(numstages * numharmonics);
        if(stereo)
            rfilter = memory.valloc<bpfilter>(numstages * numharmonics);
    }

    //how much the amplitude is normalised (because the harmonics)
    float reduceamp = setupFilters(pos, false);
    oldreduceamp    = reduceamp;

    volume /= reduceamp;

    oldpitchwheel = 0;
    oldbandwidth  = 64;
    if(!legato) { //normal note
        if(pars.Pfixedfreq == 0)
            initparameters(basefreq);
        else
            initparameters(basefreq / 440.0f * freq);
    }
    else {
        if(pars.Pfixedfreq == 0)
            freq = basefreq;
        else
            freq *= basefreq / 440.0f;

        if(GlobalFilter)
            GlobalFilter->updateNoteFreq(basefreq);
    }

    oldamplitude = newamplitude;
}

SynthNote *SUBnote::cloneLegato(void)
{
    SynthParams sp{memory, ctl, synth, time, legato.param.freq, velocity,
                   portamento, legato.param.midinote, true};
    return memory.alloc<SUBnote>(&pars, sp);
}

void SUBnote::legatonote(LegatoParams pars)
{
    // Manage legato stuff
    if(legato.update(pars))
        return;

    try {
        setup(pars.frequency, pars.velocity, pars.portamento, pars.midinote,
              true);
    } catch (std::bad_alloc &ba) {
        std::cerr << "failed to set legato note parameter in SUBnote: " << ba.what() << std::endl;
    }
}

SUBnote::~SUBnote()
{
    if(NoteEnabled)
        KillNote();
}

/*
 * Kill the note
 */
void SUBnote::KillNote()
{
    if(NoteEnabled) {
        memory.devalloc(numstages * numharmonics, lfilter);
        if(stereo)
            memory.devalloc(numstages * numharmonics, rfilter);
        memory.dealloc(AmpEnvelope);
        memory.dealloc(FreqEnvelope);
        memory.dealloc(BandWidthEnvelope);
        memory.dealloc(GlobalFilter);
        memory.dealloc(GlobalFilterEnvelope);
        NoteEnabled = false;
    }
}


/*
 * Compute the filters coefficients
 */
void SUBnote::computefiltercoefs(bpfilter &filter,
                                 float freq,
                                 float bw,
                                 float gain)
{
    if(freq > synth.samplerate_f / 2.0f - 200.0f)
        freq = synth.samplerate_f / 2.0f - 200.0f;


    float omega = 2.0f * PI * freq / synth.samplerate_f;
    float sn    = sinf(omega);
    float cs    = cosf(omega);
    float alpha = sn * sinh(LOG_2 / 2.0f * bw * omega / sn);

    if(alpha > 1)
        alpha = 1;
    if(alpha > bw)
        alpha = bw;

    filter.b0 = alpha / (1.0f + alpha) * filter.amp * gain;
    filter.b2 = -alpha / (1.0f + alpha) * filter.amp * gain;
    filter.a1 = -2.0f * cs / (1.0f + alpha);
    filter.a2 = (1.0f - alpha) / (1.0f + alpha);
}


/*
 * Initialise the filters
 */
void SUBnote::initfilter(bpfilter &filter,
                         float freq,
                         float bw,
                         float amp,
                         float mag,
                         bool automation)
{
    if(!automation) {
        filter.xn1 = 0.0f;
        filter.xn2 = 0.0f;

        if(start == 0) {
            filter.yn1 = 0.0f;
            filter.yn2 = 0.0f;
        }
        else {
            float a = 0.1f * mag; //empirically
            float p = RND * 2.0f * PI;
            if(start == 1)
                a *= RND;
            filter.yn1 = a * cosf(p);
            filter.yn2 = a * cosf(p + freq * 2.0f * PI / synth.samplerate_f);

            //correct the error of computation the start amplitude
            //at very high frequencies
            if(freq > synth.samplerate_f * 0.96f) {
                filter.yn1 = 0.0f;
                filter.yn2 = 0.0f;
            }
        }
    }

    filter.amp  = amp;
    filter.freq = freq;
    filter.bw   = bw;
    computefiltercoefs(filter, freq, bw, 1.0f);
}

/*
 * Do the filtering
 */

inline void SubFilterA(const float coeff[4], float &src, float work[4])
{
    work[3] = src*coeff[0]+work[1]*coeff[1]+work[2]*coeff[2]+work[3]*coeff[3];
    work[1] = src;
    src     = work[3];
}

inline void SubFilterB(const float coeff[4], float &src, float work[4])
{
    work[2] = src*coeff[0]+work[0]*coeff[1]+work[3]*coeff[2]+work[2]*coeff[3];
    work[0] = src;
    src     = work[2];
}

//This dance is designed to minimize unneeded memory operations which can result
//in quite a bit of wasted time
void SUBnote::filter(bpfilter &filter, float *smps)
{
    assert(synth.buffersize % 8 == 0);
    float coeff[4] = {filter.b0, filter.b2,  -filter.a1, -filter.a2};
    float work[4]  = {filter.xn1, filter.xn2, filter.yn1, filter.yn2};

    for(int i = 0; i < synth.buffersize; i += 8) {
        SubFilterA(coeff, smps[i + 0], work);
        SubFilterB(coeff, smps[i + 1], work);
        SubFilterA(coeff, smps[i + 2], work);
        SubFilterB(coeff, smps[i + 3], work);
        SubFilterA(coeff, smps[i + 4], work);
        SubFilterB(coeff, smps[i + 5], work);
        SubFilterA(coeff, smps[i + 6], work);
        SubFilterB(coeff, smps[i + 7], work);
    }
    filter.xn1 = work[0];
    filter.xn2 = work[1];
    filter.yn1 = work[2];
    filter.yn2 = work[3];
}

/*
 * Init Parameters
 */
void SUBnote::initparameters(float freq)
{
    AmpEnvelope = memory.alloc<Envelope>(*pars.AmpEnvelope, freq, synth.dt());

    if(pars.PFreqEnvelopeEnabled)
        FreqEnvelope = memory.alloc<Envelope>(*pars.FreqEnvelope, freq, synth.dt());

    if(pars.PBandWidthEnvelopeEnabled)
        BandWidthEnvelope = memory.alloc<Envelope>(*pars.BandWidthEnvelope, freq, synth.dt());

    if(pars.PGlobalFilterEnabled) {
        GlobalFilterEnvelope = memory.alloc<Envelope>(*pars.GlobalFilterEnvelope, freq, synth.dt());

        GlobalFilter = memory.alloc<ModFilter>(*pars.GlobalFilter, synth, time, memory, stereo, freq);

        GlobalFilter->updateSense(velocity, pars.PGlobalFilterVelocityScale,
                                  pars.PGlobalFilterVelocityScaleFunction);

        GlobalFilter->addMod(*GlobalFilterEnvelope);
    }
    computecurrentparameters();
}

/*
 * Compute how much to reduce amplitude near nyquist or subaudible frequencies.
 */
float SUBnote::computerolloff(float freq)
{
    const float lower_limit = 10.0f;
    const float lower_width = 10.0f;
    const float upper_width = 200.0f;
    float upper_limit = synth.samplerate / 2.0f;

    if (freq > lower_limit + lower_width &&
            freq < upper_limit - upper_width)
        return 1.0f;
    if (freq <= lower_limit || freq >= upper_limit)
        return 0.0f;
    if (freq <= lower_limit + lower_width)
        return (1.0f - cosf(M_PI * (freq - lower_limit) / lower_width)) / 2.0f;
    return (1.0f - cosf(M_PI * (freq - upper_limit) / upper_width)) / 2.0f;
}

/*
 * Compute Parameters of SUBnote for each tick
 */
void SUBnote::computecurrentparameters()
{
    //Recompute parameters for realtime automation
    if(pars.time && pars.last_update_timestamp == pars.time->time()) {
        //A little bit of copy/paste for now

        int pos[MAX_SUB_HARMONICS];
        int harmonics = 0;

        //select only harmonics that desire to compute
        for(int n = 0; n < MAX_SUB_HARMONICS; ++n) {
            if(pars.Phmag[n] == 0)
                continue;
            pos[harmonics++] = n;
        }

        bool delta_harmonics = (harmonics != numharmonics);
        if(delta_harmonics) {
            memory.devalloc(lfilter);
            memory.devalloc(rfilter);

            firstnumharmonics = numharmonics = harmonics;
            lfilter = memory.valloc<bpfilter>(numstages * numharmonics);
            if(stereo)
                rfilter = memory.valloc<bpfilter>(numstages * numharmonics);
        }

        float reduceamp = setupFilters(pos, !delta_harmonics);
        volume = volume*oldreduceamp/reduceamp;
        oldreduceamp = reduceamp;
    }

    if(FreqEnvelope || BandWidthEnvelope
       || (oldpitchwheel != ctl.pitchwheel.data)
       || (oldbandwidth != ctl.bandwidth.data)
       || portamento) {
        float envfreq = 1.0f;
        float envbw   = 1.0f;

        if(FreqEnvelope) {
            envfreq = FreqEnvelope->envout() / 1200;
            envfreq = powf(2.0f, envfreq);
        }

        envfreq *=
            powf(ctl.pitchwheel.relfreq, BendAdjust); //pitch wheel

        //Update frequency while portamento is converging
        if(portamento) {
            envfreq *= ctl.portamento.freqrap;
            if(!ctl.portamento.used) //the portamento has finished
                portamento = false;
        }

        if(BandWidthEnvelope) {
            envbw = BandWidthEnvelope->envout();
            envbw = powf(2, envbw);
        }

        envbw *= ctl.bandwidth.relbw; //bandwidth controller


        //Recompute High Frequency Dampening Terms
        for(int n = 0; n < numharmonics; ++n)
            overtone_rolloff[n] = computerolloff(overtone_freq[n] * envfreq);


        //Recompute Filter Coefficients
        float tmpgain = 1.0f / sqrt(envbw * envfreq);
        computeallfiltercoefs(lfilter, envfreq, envbw, tmpgain);
        if(stereo)
            computeallfiltercoefs(rfilter, envfreq, envbw, tmpgain);


        oldbandwidth  = ctl.bandwidth.data;
        oldpitchwheel = ctl.pitchwheel.data;
    }
    newamplitude = volume * AmpEnvelope->envout_dB() * 2.0f;

    //Filter
    if(GlobalFilter)
        GlobalFilter->update(ctl.filtercutoff.relfreq,
                             ctl.filterq.relq);
}

void SUBnote::computeallfiltercoefs(bpfilter *filters, float envfreq,
        float envbw, float gain)
{
    for(int n = 0; n < numharmonics; ++n)
        for(int nph = 0; nph < numstages; ++nph)
            computefiltercoefs(filters[nph + n * numstages],
                    filters[nph + n * numstages].freq * envfreq,
                    filters[nph + n * numstages].bw * envbw,
                    nph == 0 ? gain : 1.0);
}

void SUBnote::chanOutput(float *out, bpfilter *bp, int buffer_size)
{
    float tmprnd[buffer_size];
    float tmpsmp[buffer_size];

    //Initialize Random Input
    for(int i = 0; i < buffer_size; ++i)
        tmprnd[i] = RND * 2.0f - 1.0f;

    //For each harmonic apply the filter on the random input stream
    //Sum the filter outputs to obtain the output signal
    for(int n = 0; n < numharmonics; ++n) {
        float rolloff = overtone_rolloff[n];
        memcpy(tmpsmp, tmprnd, synth.bufferbytes);

        for(int nph = 0; nph < numstages; ++nph)
            filter(bp[nph + n * numstages], tmpsmp);

        for(int i = 0; i < synth.buffersize; ++i)
            out[i] += tmpsmp[i] * rolloff;
    }
}

/*
 * Note Output
 */
int SUBnote::noteout(float *outl, float *outr)
{
    memcpy(outl, synth.denormalkillbuf, synth.bufferbytes);
    memcpy(outr, synth.denormalkillbuf, synth.bufferbytes);

    if(!NoteEnabled)
        return 0;

    if(stereo) {
        chanOutput(outl, lfilter, synth.buffersize);
        chanOutput(outr, rfilter, synth.buffersize);

        if(GlobalFilter)
            GlobalFilter->filter(outl, outr);

    } else {
        chanOutput(outl, lfilter, synth.buffersize);

        if(GlobalFilter)
            GlobalFilter->filter(outl, 0);

        memcpy(outr, outl, synth.bufferbytes);
    }

    if(firsttick) {
        int n = 10;
        if(n > synth.buffersize)
            n = synth.buffersize;
        for(int i = 0; i < n; ++i) {
            float ampfadein = 0.5f - 0.5f * cosf(
                (float) i / (float) n * PI);
            outl[i] *= ampfadein;
            outr[i] *= ampfadein;
        }
        firsttick = false;
    }

    if(ABOVE_AMPLITUDE_THRESHOLD(oldamplitude, newamplitude))
        // Amplitude interpolation
        for(int i = 0; i < synth.buffersize; ++i) {
            float tmpvol = INTERPOLATE_AMPLITUDE(oldamplitude,
                                                 newamplitude,
                                                 i,
                                                 synth.buffersize);
            outl[i] *= tmpvol * panning;
            outr[i] *= tmpvol * (1.0f - panning);
        }
    else
        for(int i = 0; i < synth.buffersize; ++i) {
            outl[i] *= newamplitude * panning;
            outr[i] *= newamplitude * (1.0f - panning);
        }

    oldamplitude = newamplitude;
    computecurrentparameters();

    // Apply legato-specific sound signal modifications
    legato.apply(*this, outl, outr);

    // Check if the note needs to be computed more
    if(AmpEnvelope->finished() != 0) {
        for(int i = 0; i < synth.buffersize; ++i) { //fade-out
            float tmp = 1.0f - (float)i / synth.buffersize_f;
            outl[i] *= tmp;
            outr[i] *= tmp;
        }
        KillNote();
    }
    return 1;
}

/*
 * Release Key (Note Off)
 */
void SUBnote::releasekey()
{
    AmpEnvelope->releasekey();
    if(FreqEnvelope)
        FreqEnvelope->releasekey();
    if(BandWidthEnvelope)
        BandWidthEnvelope->releasekey();
    if(GlobalFilterEnvelope)
        GlobalFilterEnvelope->releasekey();
}

/*
 * Check if the note is finished
 */
bool SUBnote::finished() const
{
    return !NoteEnabled;
}

void SUBnote::entomb(void)
{
    AmpEnvelope->forceFinish();
}
