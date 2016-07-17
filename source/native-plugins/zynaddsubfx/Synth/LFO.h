/*
  ZynAddSubFX - a software synthesizer

  LFO.h - LFO implementation
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef LFO_H
#define LFO_H

#include "../globals.h"
#include "../Misc/Time.h"
#include "WatchPoint.h"

/**Class for creating Low Frequency Oscillators*/
class LFO
{
    public:
        /**Constructor
         *
         * @param lfopars pointer to a LFOParams object
         * @param basefreq base frequency of LFO
         */
        LFO(const LFOParams &lfopars, float basefreq, const AbsTime &t, WatchManager *m=0,
                const char *watch_prefix=0);
        ~LFO();

        float lfoout();
        float amplfoout();
    private:
        float baseOut(const char waveShape, const float phase);
        //Phase of Oscillator
        float phase;
        //Phase Increment Per Frame
        float phaseInc;
        //Frequency Randomness
        float incrnd, nextincrnd;
        //Amplitude Randomness
        float amp1, amp2;

        // RND mode
        int first_half;
        float last_random;

        //Intensity of the wave
        float lfointensity;
        //Amount Randomness
        float lfornd, lfofreqrnd;

        //Delay before starting
        RelTime delayTime;

        char  waveShape;

        //If After initialization there are no calls to random number gen.
        bool  deterministic;

        const float     dt_;
        const LFOParams &lfopars_;
        const float basefreq_;

        VecWatchPoint watchOut;

        void computeNextFreqRnd(void);
};

#endif
