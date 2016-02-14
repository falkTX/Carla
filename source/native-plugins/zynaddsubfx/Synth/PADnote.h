/*
  ZynAddSubFX - a software synthesizer

  PADnote.h - The "pad" synthesizer
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
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#ifndef PAD_NOTE_H
#define PAD_NOTE_H

#include "SynthNote.h"
#include "../globals.h"
#include "Envelope.h"
#include "LFO.h"

/**The "pad" synthesizer*/
class PADnote:public SynthNote
{
    public:
        PADnote(const PADnoteParameters *parameters, SynthParams pars,
                const int &interpolation);
        ~PADnote();

        SynthNote *cloneLegato(void);
        void legatonote(LegatoParams pars);

        int noteout(float *outl, float *outr);
        bool finished() const;
        void entomb(void);

        void releasekey();
    private:
        void setup(float freq, float velocity, int portamento_,
                   int midinote, bool legato = false);
        void fadein(float *smps);
        void computecurrentparameters();
        bool finished_;
        const PADnoteParameters &pars;

        int   poshi_l, poshi_r;
        float poslo;

        float basefreq;
        float BendAdjust;
        float OffsetHz;
        bool  firsttime;

        int nsample, portamento;

        int Compute_Linear(float *outl,
                           float *outr,
                           int freqhi,
                           float freqlo);
        int Compute_Cubic(float *outl,
                          float *outr,
                          int freqhi,
                          float freqlo);


        struct {
            /******************************************
            *     FREQUENCY GLOBAL PARAMETERS        *
            ******************************************/
            float Detune;  //cents

            Envelope *FreqEnvelope;
            LFO      *FreqLfo;

            /********************************************
             *     AMPLITUDE GLOBAL PARAMETERS          *
             ********************************************/
            float Volume;  // [ 0 .. 1 ]

            float Panning;  // [ 0 .. 1 ]

            Envelope *AmpEnvelope;
            LFO      *AmpLfo;

            float Fadein_adjustment;
            struct {
                int   Enabled;
                float initialvalue, dt, t;
            } Punch;

            /******************************************
            *        FILTER GLOBAL PARAMETERS        *
            ******************************************/
            ModFilter *GlobalFilter;
            Envelope  *FilterEnvelope;
            LFO       *FilterLfo;
        } NoteGlobalPar;


        float globaloldamplitude, globalnewamplitude, velocity, realfreq;
        const int& interpolation;
};


#endif
