/*
  ZynAddSubFX - a software synthesizer

  SUBnote.h - The subtractive synthesizer
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef SUB_NOTE_H
#define SUB_NOTE_H

#include "SynthNote.h"
#include "../globals.h"

class SUBnote:public SynthNote
{
    public:
        SUBnote(const SUBnoteParameters *parameters, SynthParams &pars);
        ~SUBnote();

        SynthNote *cloneLegato(void);
        void legatonote(LegatoParams pars);

        int noteout(float *outl, float *outr); //note output,return 0 if the note is finished
        void releasekey();
        bool finished() const;
        void entomb(void);
    private:

        void setup(float freq,
                   float velocity,
                   int portamento_,
                   int midinote,
                   bool legato = false);
        float setupFilters(int *pos, bool automation);
        void computecurrentparameters();
        /*
         * Initialize envelopes and global filter
         * calls computercurrentparameters()
         */
        void initparameters(float freq);
        void KillNote();

        const SUBnoteParameters &pars;

        //parameters
        bool       stereo;
        int       numstages; //number of stages of filters
        int       numharmonics; //number of harmonics (after the too higher hamonics are removed)
        int       firstnumharmonics; //To keep track of the first note's numharmonics value, useful in legato mode.
        int       start; //how the harmonics start
        float     basefreq;
        float     BendAdjust;
        float     OffsetHz;
        float     panning;
        Envelope *AmpEnvelope;
        Envelope *FreqEnvelope;
        Envelope *BandWidthEnvelope;

        ModFilter *GlobalFilter;
        Envelope  *GlobalFilterEnvelope;

        //internal values
        bool   NoteEnabled;
        bool   firsttick, portamento;
        float  volume, oldamplitude, newamplitude;
        float  oldreduceamp;

        struct bpfilter {
            float freq, bw, amp; //filter parameters
            float a1, a2, b0, b2; //filter coefs. b1=0
            float xn1, xn2, yn1, yn2; //filter internal values
        };

        void chanOutput(float *out, bpfilter *bp, int buffer_size);

        void initfilter(bpfilter &filter,
                        float freq,
                        float bw,
                        float amp,
                        float mag,
                        bool automation);
        float computerolloff(float freq);
        void computeallfiltercoefs(bpfilter *filters, float envfreq, float envbw, float gain);
        void computefiltercoefs(bpfilter &filter,
                                float freq,
                                float bw,
                                float gain);
        inline void filter(bpfilter &filter, float *smps);

        bpfilter *lfilter, *rfilter;

        float overtone_rolloff[MAX_SUB_HARMONICS];
        float overtone_freq[MAX_SUB_HARMONICS];

        int   oldpitchwheel, oldbandwidth;
        float globalfiltercenterq;
        float velocity;
};

#endif
