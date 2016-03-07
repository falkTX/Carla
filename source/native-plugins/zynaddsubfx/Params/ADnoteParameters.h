/*
  ZynAddSubFX - a software synthesizer

  ADnoteParameters.h - Parameters for ADnote (ADsynth)
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef AD_NOTE_PARAMETERS_H
#define AD_NOTE_PARAMETERS_H

#include "../globals.h"
#include "PresetsArray.h"

enum FMTYPE {
    NONE, MORPH, RING_MOD, PHASE_MOD, FREQ_MOD, PW_MOD
};

/*****************************************************************/
/*                    GLOBAL PARAMETERS                          */
/*****************************************************************/

struct ADnoteGlobalParam {
    ADnoteGlobalParam(const AbsTime* time_ = nullptr);
    ~ADnoteGlobalParam();
    void defaults();
    void add2XML(XMLwrapper& xml);
    void getfromXML(XMLwrapper& xml);
    void paste(ADnoteGlobalParam &a);
    /* The instrument type  - MONO/STEREO
    If the mode is MONO, the panning of voices are not used
    Stereo=1, Mono=0. */

    unsigned char PStereo;


    /******************************************
    *     FREQUENCY GLOBAL PARAMETERS        *
    ******************************************/
    unsigned short int PDetune; //fine detune
    unsigned short int PCoarseDetune; //coarse detune+octave
    unsigned char      PDetuneType; //detune type

    unsigned char PBandwidth;      //how much the relative fine detunes of the voices are changed

    EnvelopeParams *FreqEnvelope;    //Frequency Envelope

    LFOParams *FreqLfo; //Frequency LFO

    /********************************************
    *     AMPLITUDE GLOBAL PARAMETERS          *
    ********************************************/

    /* Panning -  0 - random
              1 - left
             64 - center
            127 - right */
    unsigned char PPanning;

    unsigned char PVolume;

    unsigned char PAmpVelocityScaleFunction;

    EnvelopeParams *AmpEnvelope;

    LFOParams *AmpLfo;

    /* Adjustment factor for anti-pop fadein */
    unsigned char Fadein_adjustment;

    unsigned char PPunchStrength, PPunchTime, PPunchStretch,
                  PPunchVelocitySensing;

    /******************************************
    *        FILTER GLOBAL PARAMETERS        *
    ******************************************/
    FilterParams *GlobalFilter;

    // filter velocity sensing
    unsigned char PFilterVelocityScale;

    // filter velocity sensing
    unsigned char PFilterVelocityScaleFunction;

    EnvelopeParams *FilterEnvelope;

    LFOParams *FilterLfo;

    // RESONANCE
    Resonance *Reson;

    //how the randomness is applied to the harmonics on more voices using the same oscillator
    unsigned char Hrandgrouping;

    const AbsTime *time;
    int64_t last_update_timestamp;

    static const rtosc::Ports &ports;
};



/***********************************************************/
/*                    VOICE PARAMETERS                     */
/***********************************************************/
struct ADnoteVoiceParam {
    ADnoteVoiceParam() : time(nullptr), last_update_timestamp(0) { };
    void getfromXML(XMLwrapper& xml, unsigned nvoice);
    void add2XML(XMLwrapper& xml, bool fmoscilused);
    void paste(ADnoteVoiceParam &p);
    void defaults(void);
    void enable(const SYNTH_T &synth, FFTwrapper *fft, Resonance *Reson,
                const AbsTime *time);
    void kill(void);
    float getUnisonFrequencySpreadCents(void) const;
    /** If the voice is enabled */
    unsigned char Enabled;

    /** How many subvoices are used in this voice */
    unsigned char Unison_size;

    /** How subvoices are spread */
    unsigned char Unison_frequency_spread;

    /** How much phase randomization */
    unsigned char Unison_phase_randomness;

    /** Stereo spread of the subvoices*/
    unsigned char Unison_stereo_spread;

    /** Vibratto of the subvoices (which makes the unison more "natural")*/
    unsigned char Unison_vibratto;

    /** Medium speed of the vibratto of the subvoices*/
    unsigned char Unison_vibratto_speed;

    /** Unison invert phase */
    unsigned char Unison_invert_phase; //0=none,1=random,2=50%,3=33%,4=25%

    /** Type of the voice (0=Sound,1=Noise)*/
    unsigned char Type;

    /** Voice Delay */
    unsigned char PDelay;

    /** If the resonance is enabled for this voice */
    unsigned char Presonance;

    // What external oscil should I use, -1 for internal OscilSmp&FMSmp
    short int Pextoscil, PextFMoscil;
    // it is not allowed that the externoscil,externFMoscil => current voice

    // oscillator phases
    unsigned char Poscilphase, PFMoscilphase;

    // filter bypass
    unsigned char Pfilterbypass;

    /** Voice oscillator */
    OscilGen *OscilSmp;

    /**********************************
    *     FREQUENCY PARAMETERS        *
    **********************************/

    /** If the base frequency is fixed to 440 Hz*/
    unsigned char Pfixedfreq;

    /* Equal temperate (this is used only if the Pfixedfreq is enabled)
       If this parameter is 0, the frequency is fixed (to 440 Hz);
       if this parameter is 64, 1 MIDI halftone -> 1 frequency halftone */
    unsigned char PfixedfreqET;

    /** Fine detune */
    unsigned short int PDetune;

    /** Coarse detune + octave */
    unsigned short int PCoarseDetune;

    /** Detune type */
    unsigned char PDetuneType;

    /** Pitch bend adjustment */
    unsigned char PBendAdjust;

    /** Pitch offset Hz */
    unsigned char POffsetHz;

    /* Frequency Envelope */
    unsigned char   PFreqEnvelopeEnabled;
    EnvelopeParams *FreqEnvelope;

    /* Frequency LFO */
    unsigned char PFreqLfoEnabled;
    LFOParams    *FreqLfo;


    /***************************
    *   AMPLITUDE PARAMETERS   *
    ***************************/

    /* Panning       0 - random
             1 - left
                64 - center
               127 - right
       The Panning is ignored if the instrument is mono */
    unsigned char PPanning;

    /* Voice Volume */
    unsigned char PVolume;

    /* If the Volume negative */
    unsigned char PVolumeminus;

    /* Velocity sensing */
    unsigned char PAmpVelocityScaleFunction;

    /* Amplitude Envelope */
    unsigned char   PAmpEnvelopeEnabled;
    EnvelopeParams *AmpEnvelope;

    /* Amplitude LFO */
    unsigned char PAmpLfoEnabled;
    LFOParams    *AmpLfo;



    /*************************
    *   FILTER PARAMETERS    *
    *************************/

    /* Voice Filter */
    unsigned char PFilterEnabled;
    FilterParams *VoiceFilter;

    /* Filter Envelope */
    unsigned char   PFilterEnvelopeEnabled;
    EnvelopeParams *FilterEnvelope;

    /* Filter LFO */
    unsigned char PFilterLfoEnabled;
    LFOParams    *FilterLfo;

    // filter velocity sensing
    unsigned char PFilterVelocityScale;

    // filter velocity sensing
    unsigned char PFilterVelocityScaleFunction;

    /****************************
    *   MODULLATOR PARAMETERS   *
    ****************************/

    /* Modullator Parameters (0=off,1=Morph,2=RM,3=PM,4=FM.. */
    unsigned char PFMEnabled;

    /* Voice that I use as modullator instead of FMSmp.
       It is -1 if I use FMSmp(default).
       It maynot be equal or bigger than current voice */
    short int PFMVoice;

    /* Modullator oscillator */
    OscilGen *FMSmp;

    /* Modullator Volume */
    unsigned char PFMVolume;

    /* Modullator damping at higher frequencies */
    unsigned char PFMVolumeDamp;

    /* Modullator Velocity Sensing */
    unsigned char PFMVelocityScaleFunction;

    /* Fine Detune of the Modullator*/
    unsigned short int PFMDetune;

    /* Coarse Detune of the Modullator */
    unsigned short int PFMCoarseDetune;

    /* The detune type */
    unsigned char PFMDetuneType;

    /* FM base freq fixed at 440Hz */
    unsigned char PFMFixedFreq;

    /* Frequency Envelope of the Modullator */
    unsigned char   PFMFreqEnvelopeEnabled;
    EnvelopeParams *FMFreqEnvelope;

    /* Frequency Envelope of the Modullator */
    unsigned char   PFMAmpEnvelopeEnabled;
    EnvelopeParams *FMAmpEnvelope;

    unsigned char *GlobalPDetuneType;

    const AbsTime *time;
    int64_t last_update_timestamp;

    static const rtosc::Ports &ports;
};

class ADnoteParameters:public PresetsArray
{
    public:
        ADnoteParameters(const SYNTH_T &synth, FFTwrapper *fft_,
                         const AbsTime *time_ = nullptr);
        ~ADnoteParameters();

        ADnoteGlobalParam GlobalPar;
        ADnoteVoiceParam  VoicePar[NUM_VOICES];

        void defaults();
        void add2XML(XMLwrapper& xml);
        void getfromXML(XMLwrapper& xml);

        void paste(ADnoteParameters &a);
        void pasteArray(ADnoteParameters &a, int section);


        float getBandwidthDetuneMultiplier() const;
        float getUnisonFrequencySpreadCents(int nvoice) const;
        static const rtosc::Ports &ports;
        void defaults(int n); //n is the nvoice
        void add2XMLsection(XMLwrapper& xml, int n);
        void getfromXMLsection(XMLwrapper& xml, int n);
    private:

        void EnableVoice(const SYNTH_T &synth, int nvoice, const AbsTime* time);
        void KillVoice(int nvoice);
        FFTwrapper *fft;

        const AbsTime *time;
        int64_t last_update_timestamp;
};

#endif
