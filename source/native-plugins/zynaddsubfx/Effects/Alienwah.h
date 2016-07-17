/*
  ZynAddSubFX - a software synthesizer

  Alienwah.h - "AlienWah" effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef ALIENWAH_H
#define ALIENWAH_H

#include "Effect.h"
#include "EffectLFO.h"
#include <complex>

#define MAX_ALIENWAH_DELAY 100

/**"AlienWah" Effect*/
class Alienwah:public Effect
{
    public:
        Alienwah(EffectParams pars);
        ~Alienwah();
        void out(const Stereo<float *> &smp);

        void setpreset(unsigned char npreset);
        void changepar(int npar, unsigned char value);
        unsigned char getpar(int npar) const;
        void cleanup(void);

        static rtosc::Ports ports;
    private:
        //Alienwah Parameters
        EffectLFO     lfo;      //lfo-ul Alienwah
        unsigned char Pvolume;
        unsigned char Pdepth;   //the depth of the Alienwah
        unsigned char Pfb;      //feedback
        unsigned char Pdelay;
        unsigned char Pphase;


        //Control Parameters
        void setvolume(unsigned char _Pvolume);
        void setdepth(unsigned char _Pdepth);
        void setfb(unsigned char _Pfb);
        void setdelay(unsigned char _Pdelay);
        void setphase(unsigned char _Pphase);

        //Internal Values
        float fb, depth, phase;
        std::complex<float> *oldl, *oldr;
        std::complex<float>  oldclfol, oldclfor;
        int oldk;
};

#endif
