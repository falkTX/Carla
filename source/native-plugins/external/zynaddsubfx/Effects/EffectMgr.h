/*
  ZynAddSubFX - a software synthesizer

  EffectMgr.h - Effect manager, an interface betwen the program and effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef EFFECTMGR_H
#define EFFECTMGR_H

#include <pthread.h>

#include "../Params/FilterParams.h"
#include "../Params/Presets.h"

namespace zyncarla {

class Effect;
class FilterParams;
class XMLwrapper;
class Allocator;

/** Effect manager, an interface between the program and effects */
class EffectMgr:public Presets
{
    public:
        EffectMgr(Allocator &alloc, const SYNTH_T &synth, const bool insertion_,
              const AbsTime *time_ = nullptr);
        ~EffectMgr();

        void paste(EffectMgr &e);
        void add2XML(XMLwrapper& xml);
        void defaults(void) REALTIME;
        void getfromXML(XMLwrapper& xml);

        void out(float *smpsl, float *smpsr) REALTIME;

        void setdryonly(bool value);

        /**get the output(to speakers) volume of the systemeffect*/
        float sysefxgetvolume(void);

        void init(void) REALTIME;
        void kill(void) REALTIME;
        void cleanup(void) REALTIME;

        void changeeffectrt(int nefx_, bool avoidSmash=false) REALTIME;
        void changeeffect(int nefx_) NONREALTIME;
        int geteffect(void);
        void changepreset(unsigned char npreset) NONREALTIME;
        void changepresetrt(unsigned char npreset, bool avoidSmash=false) REALTIME;
        unsigned char getpreset(void);
        void seteffectpar(int npar, unsigned char value) NONREALTIME;
        void seteffectparrt(int npar, unsigned char value) REALTIME;
        unsigned char geteffectpar(int npar);
        unsigned char geteffectparrt(int npar) REALTIME;

        const bool insertion;
        float     *efxoutl, *efxoutr;

        // used by UI
        float getEQfreqresponse(float freq);

        FilterParams *filterpars;

        static const rtosc::Ports &ports;
        int     nefx;
        Effect *efx;
        const AbsTime *time;
    private:

        //Parameters Prior to initialization
        char preset;

        /**
         * When loading an effect from XML the child effect cannot be loaded
         * directly as it would require access to the realtime memory pool,
         * which cannot be done outside of the realtime thread.
         * Therefore, parameters are loaded to this array which can then be used
         * to construct the full effect (via init()) once the object is in the
         * realtime context.
         *
         * Additionally this structure is used in the case of pasting effects as
         * pasted effect object are *not* fully initialized when they're put on
         * the middleware -> backend ringbuffer, but settings has the values
         * loaded from the XML serialization.
         * The settings values can be pasted once they're on the realtime thread
         * and then they can be applied.
         *
         * The requirement that the realtime memory pool is used to create the
         * effect is in place as it is possible to change the effect type in the
         * realtime thread and thus the new effect would draw from the realtime
         * memory pool and the old object would be expected to be freed to the
         * realtime memory pool.
         *
         * See also: PresetExtractor.cpp
         */
        char settings[128];

        bool dryonly;
        Allocator &memory;
        const SYNTH_T &synth;
};

}

#endif
