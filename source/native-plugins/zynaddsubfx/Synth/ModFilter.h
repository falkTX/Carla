#pragma once
#include "../globals.h"
#include "../Misc/Time.h"

//Modulated instance of one of the filters in src/DSP/
//Supports stereo modes
class ModFilter
{
    public:
        ModFilter(const FilterParams &pars,
                  const SYNTH_T      &synth,
                  const AbsTime      &time,
                        Allocator    &alloc,
                        bool         stereo,
                        float        notefreq_);
        ~ModFilter(void);

        void addMod(LFO      &lfo);
        void addMod(Envelope &env);

        //normal per tick update
        void update(float relfreq, float relq);

        //updates typically seen in note-init
        void updateNoteFreq(float noteFreq_);
        void updateSense(float velocity,
                uint8_t scale, uint8_t func);

        //filter stereo/mono signal(s) in-place
        void filter(float *l, float *r);
    private:
        void paramUpdate(Filter *&f);
        void svParamUpdate(SVFilter &sv);
        void anParamUpdate(AnalogFilter &an);

        
        const FilterParams &pars;  //Parameters to Pull Updates From
        const SYNTH_T      &synth; //Synthesizer Buffer Parameters
        const AbsTime      &time;  //Time for RT Updates
        Allocator          &alloc; //RT Memory Pool



        float baseQ;    //filter sharpness
        float baseFreq; //base filter frequency
        float noteFreq; //frequency note was initialized to
        float tracking; //shift due to note frequency
        float sense;    //shift due to note velocity


        Filter       *left; //left  channel filter
        Filter       *right;//right channel filter
        Envelope     *env;  //center freq envelope
        LFO          *lfo;  //center freq lfo
};
