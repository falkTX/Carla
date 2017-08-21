/*
  ZynAddSubFX - a software synthesizer

  EnvelopeParams.h - Parameters for Envelope
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef ENVELOPE_PARAMS_H
#define ENVELOPE_PARAMS_H

#include "../globals.h"
#include "../Misc/XMLwrapper.h"
#include "Presets.h"

namespace zyncarla {

class EnvelopeParams:public Presets
{
    public:
        enum envelope_type_t
        {
            ad_global_amp_env,     // ADSRinit_dB(0, 40, 127, 25);
            ad_global_freq_env,    // ASRinit(64, 50, 64, 60);
            ad_global_filter_env,  // ADSRinit_filter(64, 40, 64, 70, 60, 64)

            ad_voice_amp_env,      // ADSRinit_dB(0, 100, 127, 100);
            ad_voice_freq_env,     // ASRinit(30, 40, 64, 60);
            ad_voice_filter_env,   // ADSRinit_filter(90, 70, 40, 70, 10, 40);
            ad_voice_fm_freq_env,  // ASRinit(20, 90, 40, 80);
            ad_voice_fm_amp_env,   // ADSRinit(80, 90, 127, 100)
            sub_freq_env,          // ASRinit(30, 50, 64, 60);
            sub_bandwidth_env,     // ASRinit_bw(100, 70, 64, 60)
        };

        EnvelopeParams(unsigned char Penvstretch_=64,
                       unsigned char Pforcedrelease_=0,
                       const AbsTime *time_ = nullptr);
        ~EnvelopeParams();
        void paste(const EnvelopeParams &ep);

        void init(envelope_type_t etype);
        void converttofree();

        void add2XML(XMLwrapper& xml);
        void defaults();
        void getfromXML(XMLwrapper& xml);

        float getdt(char i) const;
        static float dt(char val);
        static char inv_dt(float val);

        //! @brief defines where it is used and its default settings
	//! corresponds to envelope_type_t
        int envelope_type;

        /* MIDI Parameters */
        unsigned char Pfreemode; //1 for free mode, 0 otherwise
        unsigned char Penvpoints;
        unsigned char Penvsustain; //127 for disabled
        unsigned char Penvdt[MAX_ENVELOPE_POINTS];
        unsigned char Penvval[MAX_ENVELOPE_POINTS];
        unsigned char Penvstretch; //64=normal stretch (piano-like), 0=no stretch
        unsigned char Pforcedrelease; //0 - OFF, 1 - ON
        unsigned char Plinearenvelope; //if the amplitude envelope is linear

        unsigned char PA_dt, PD_dt, PR_dt,
                      PA_val, PD_val, PS_val, PR_val;



        int Envmode; // 1 for ADSR parameters (linear amplitude)
                     // 2 for ADSR_dB parameters (dB amplitude)
                     // 3 for ASR parameters (frequency LFO)
                     // 4 for ADSR_filter parameters (filter parameters)
                     // 5 for ASR_bw parameters (bandwidth parameters)

        const AbsTime *time;
        int64_t last_update_timestamp;

        static const rtosc::Ports &ports;

        static float env_rap2dB(float rap);
        static float env_dB2rap(float db);

    private:
        void ADSRinit(char A_dt, char D_dt, char S_val, char R_dt);
        void ADSRinit_dB(char A_dt, char D_dt, char S_val, char R_dt);
        void ASRinit(char A_val, char A_dt, char R_val, char R_dt);
        void ADSRinit_filter(char A_val,
                             char A_dt,
                             char D_val,
                             char D_dt,
                             char R_dt,
                             char R_val);
        void ASRinit_bw(char A_val, char A_dt, char R_val, char R_dt);

        void store2defaults();

        /* Default parameters */
        unsigned char Denvstretch;
        unsigned char Dforcedrelease;
        unsigned char Dlinearenvelope;
        unsigned char DA_dt, DD_dt, DR_dt,
                      DA_val, DD_val, DS_val, DR_val;
};

}

#endif
