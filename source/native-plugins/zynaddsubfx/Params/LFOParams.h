/*
  ZynAddSubFX - a software synthesizer

  LFOParams.h - Parameters for LFO
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef LFO_PARAMS_H
#define LFO_PARAMS_H

#include <Misc/Time.h>
#include <rtosc/ports.h>
#include "Presets.h"

#define LFO_SINE      0
#define LFO_TRIANGLE  1
#define LFO_SQUARE    2
#define LFO_RAMPUP    3
#define LFO_RAMPDOWN  4
#define LFO_EXP_DOWN1 5
#define LFO_EXP_DOWN2 6
#define LFO_RANDOM    7

namespace zyncarla {

class XMLwrapper;

class LFOParams:public Presets
{
    public:
        LFOParams(const AbsTime* time_ = nullptr);
        LFOParams(char Pfreq_,
                  char Pintensity_,
                  char Pstartphase_,
                  char PLFOtype_,
                  char Prandomness_,
                  char Pdelay_,
                  char Pcontinous,
                  char fel_,
                  const AbsTime* time_ = nullptr);
        ~LFOParams();

        void add2XML(XMLwrapper& xml);
        void defaults();
        /**Loads the LFO from the xml*/
        void getfromXML(XMLwrapper& xml);
        void paste(LFOParams &);

        /*  MIDI Parameters*/
        float Pfreq;      /**<frequency*/
        unsigned char Pintensity; /**<intensity*/
        unsigned char Pstartphase; /**<start phase (0=random)*/
        unsigned char PLFOtype; /**<LFO type (sin,triangle,square,ramp,...)*/
        unsigned char Prandomness; /**<randomness (0=off)*/
        unsigned char Pfreqrand; /**<frequency randomness (0=off)*/
        unsigned char Pdelay; /**<delay (0=off)*/
        unsigned char Pcontinous; /**<1 if LFO is continous*/
        unsigned char Pstretch; /**<how the LFO is "stretched" according the note frequency (64=no stretch)*/

        int fel; //what kind is the LFO (0 - frequency, 1 - amplitude, 2 - filter)

        const AbsTime *time;
        int64_t last_update_timestamp;

        static const rtosc::Ports &ports;
    private:
        /* Default parameters */
        unsigned char Dfreq;
        unsigned char Dintensity;
        unsigned char Dstartphase;
        unsigned char DLFOtype;
        unsigned char Drandomness;
        unsigned char Ddelay;
        unsigned char Dcontinous;
};

}

#endif
