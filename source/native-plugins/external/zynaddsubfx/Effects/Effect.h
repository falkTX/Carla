/*
  ZynAddSubFX - a software synthesizer

  Effect.h - this class is inherited by the all effects(Reverb, Echo, ..)
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef EFFECT_H
#define EFFECT_H

#include "../Misc/Util.h"
#include "../globals.h"
#include "../Params/FilterParams.h"
#include "../Misc/Stereo.h"

// bug: the effect parameters can currently be set, but such values
//      will not be saved into XML files
#ifndef rEffPar
#define rEffPar(name, idx, ...) \
  {STRINGIFY(name) "::i",  rProp(parameter) rDefaultDepends(preset) \
   DOC(__VA_ARGS__), NULL, rEffParCb(idx)}
#define rEffParTF(name, idx, ...) \
  {STRINGIFY(name) "::T:F",  rProp(parameter) rDefaultDepends(preset) \
   DOC(__VA_ARGS__), NULL, rEffParTFCb(idx)}
#define rEffParCb(idx) \
    [](const char *msg, rtosc::RtData &d) {\
        rObject &obj = *(rObject*)d.obj; \
        if(rtosc_narguments(msg)) \
            obj.changepar(idx, rtosc_argument(msg, 0).i); \
        else \
            d.reply(d.loc, "i", obj.getpar(idx));}
#define rEffParTFCb(idx) \
    [](const char *msg, rtosc::RtData &d) {\
        rObject &obj = *(rObject*)d.obj; \
        if(rtosc_narguments(msg)) \
            obj.changepar(idx, rtosc_argument(msg, 0).T*127); \
        else \
            d.reply(d.loc, obj.getpar(idx)?"T":"F");}
#endif

#define rEffParCommon(pname, rshort, rdoc, idx, ...) \
{STRINGIFY(pname) "::i", rProp(parameter) rLinear(0,127) \
    rShort(rshort) rDoc(rdoc), \
    0, \
    [](const char *msg, rtosc::RtData &d) \
    { \
        rObject& eff = *(rObject*)d.obj; \
        if(!rtosc_narguments(msg)) \
            d.reply(d.loc, "i", eff.getpar(idx)); \
        else { \
            eff.changepar(0, rtosc_argument(msg, 0).i); \
            d.broadcast(d.loc, "i", eff.getpar(idx)); \
        } \
    }}

#define rEffParVol(...) rEffParCommon(Pvolume, "amt", "amount of effect", 0, \
    __VA_ARGS__)
#define rEffParPan(...) rEffParCommon(Ppanning, "pan", "panning", 1, \
    __VA_ARGS__)


namespace zyncarla {

class FilterParams;
class Allocator;

struct EffectParams
{
    /**
     * Effect Parameter Constructor
     * @param alloc        Realtime Memory Allocator
     * @param insertion_   1 when it is an insertion Effect
     * @param efxoutl_     Effect output buffer Left channel
     * @param efxoutr_     Effect output buffer Right channel
     * @param filterpars_  pointer to FilterParams array
     * @param Ppreset_     chosen preset
     * @return Initialized Effect Parameter object*/
    EffectParams(Allocator &alloc_, bool insertion_, float *efxoutl_, float *efxoutr_,
            unsigned char Ppreset_, unsigned int srate, int bufsize, FilterParams *filterpars_,
            bool filterprotect=false);


    Allocator &alloc;
    bool insertion;
    float *efxoutl;
    float *efxoutr;
    unsigned char Ppreset;
    unsigned int srate;
    int bufsize;
    FilterParams *filterpars;
    bool filterprotect;
};

/**this class is inherited by the all effects(Reverb, Echo, ..)*/
class Effect
{
    public:
        Effect(EffectParams pars);
        virtual ~Effect() {}
        /**
         * Choose a preset
         * @param npreset number of chosen preset*/
        virtual void setpreset(unsigned char npreset) = 0;
        /**Change parameter npar to value
         * @param npar chosen parameter
         * @param value chosen new value*/
        virtual void changepar(int npar, unsigned char value) = 0;
        /**Get the value of parameter npar
         * @param npar chosen parameter
         * @return the value of the parameter in an unsigned char or 0 if it
         * does not exist*/
        virtual unsigned char getpar(int npar) const = 0;
        /**Output result of effect based on the given buffers
         *
         * This method should result in the effect generating its results
         * and placing them into the efxoutl and efxoutr buffers.
         * Every Effect should overide this method.
         *
         * @param smpsl Input buffer for the Left channel
         * @param smpsr Input buffer for the Right channel
         */
        void out(float *const smpsl, float *const smpsr);
        virtual void out(const Stereo<float *> &smp) = 0;
        /**Reset the state of the effect*/
        virtual void cleanup(void) {}
        virtual float getfreqresponse(float freq) { return freq; }

        unsigned char Ppreset;   /**<Currently used preset*/
        float *const  efxoutl; /**<Effect out Left Channel*/
        float *const  efxoutr; /**<Effect out Right Channel*/
        float outvolume; /**<This is the volume of effect and is public because
                          * it is needed in system effects.
                          * The out volume of such effects are always 1.0f, so
                          * this setting tells me how is the volume to the
                          * Master Output only.*/

        float volume;

        FilterParams *filterpars; /**<Parameters for filters used by Effect*/

        //Perform L/R crossover
        static void crossover(float &a, float &b, float crossover);

    protected:
        void setpanning(char Ppanning_);
        void setlrcross(char Plrcross_);

        const bool insertion;
        //panning parameters
        char  Ppanning;
        float pangainL;
        float pangainR;
        char  Plrcross; // L/R mix
        float lrcross;

        //Allocator
        Allocator &memory;

        // current setup
        unsigned int samplerate;
        int buffersize;

        // alias for above terms
        float samplerate_f;
        float halfsamplerate_f;
        float buffersize_f;
        int   bufferbytes;

        inline void alias()
        {
            samplerate_f     = samplerate;
            halfsamplerate_f = samplerate_f / 2.0f;
            buffersize_f     = buffersize;
            bufferbytes      = buffersize * sizeof(float);
        }
};

}

#endif
