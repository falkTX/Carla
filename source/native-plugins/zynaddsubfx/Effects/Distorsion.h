/*
  ZynAddSubFX - a software synthesizer

  Distorsion.h - Distorsion Effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef DISTORSION_H
#define DISTORSION_H

#include "Effect.h"

/**Distortion Effect*/
class Distorsion:public Effect
{
    public:
        Distorsion(EffectParams pars);
        ~Distorsion();
        void out(const Stereo<float *> &smp);
        void setpreset(unsigned char npreset);
        void changepar(int npar, unsigned char value);
        unsigned char getpar(int npar) const;
        void cleanup(void);
        void applyfilters(float *efxoutl, float *efxoutr);

    private:
        //Parameters
        unsigned char Pvolume;       //Volume or E/R
        unsigned char Pdrive;        //the input amplification
        unsigned char Plevel;        //the output amplification
        unsigned char Ptype;         //Distorsion type
        unsigned char Pnegate;       //if the input is negated
        unsigned char Plpf;          //lowpass filter
        unsigned char Phpf;          //highpass filter
        unsigned char Pstereo;       //0=mono, 1=stereo
        unsigned char Pprefiltering; //if you want to do the filtering before the distorsion

        void setvolume(unsigned char _Pvolume);
        void setlpf(unsigned char _Plpf);
        void sethpf(unsigned char _Phpf);

        //Real Parameters
        class AnalogFilter * lpfl, *lpfr, *hpfl, *hpfr;
};

#endif
