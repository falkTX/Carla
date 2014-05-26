/*
 * ZaMultiCompX2 Stereo multiband compressor
 * Copyright (C) 2014  Damien Zammit <damien@zamaudio.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "ZaMultiCompX2Plugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

ZaMultiCompX2Plugin::ZaMultiCompX2Plugin()
    : Plugin(paramCount, 1, 1) // 1 program, 1 state
{
    // set default values
    d_setProgram(0);
}

// -----------------------------------------------------------------------
// Init

void ZaMultiCompX2Plugin::d_initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
    case paramAttack:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Attack";
        parameter.symbol     = "att";
        parameter.unit       = "ms";
        parameter.ranges.def = 10.0f;
        parameter.ranges.min = 0.1f;
        parameter.ranges.max = 100.0f;
        break;
    case paramRelease:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Release";
        parameter.symbol     = "rel";
        parameter.unit       = "ms";
        parameter.ranges.def = 80.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 500.0f;
        break;
    case paramKnee:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Knee";
        parameter.symbol     = "kn";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 9.0f;
        break;
    case paramRatio:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Ratio";
        parameter.symbol     = "rat";
        parameter.unit       = " ";
        parameter.ranges.def = 4.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramThresh1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Threshold 1";
        parameter.symbol     = "thr1";
        parameter.unit       = "dB";
        parameter.ranges.def = -20.0f;
        parameter.ranges.min = -60.0f;
        parameter.ranges.max = 0.0f;
        break;
    case paramThresh2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Threshold 2";
        parameter.symbol     = "thr2";
        parameter.unit       = "dB";
        parameter.ranges.def = -18.0f;
        parameter.ranges.min = -60.0f;
        parameter.ranges.max = 0.0f;
        break;
    case paramThresh3:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Threshold 3";
        parameter.symbol     = "thr3";
        parameter.unit       = "dB";
        parameter.ranges.def = -16.0f;
        parameter.ranges.min = -60.0f;
        parameter.ranges.max = 0.0f;
        break;
    case paramMakeup1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Makeup 1";
        parameter.symbol     = "mak1";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 30.0f;
        break;
    case paramMakeup2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Makeup 2";
        parameter.symbol     = "mak2";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 30.0f;
        break;
    case paramMakeup3:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Makeup 3";
        parameter.symbol     = "mak3";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 30.0f;
        break;
    case paramGainR1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_OUTPUT;
        parameter.name       = "Gain Reduction 1";
        parameter.symbol     = "gr1";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramGainR2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_OUTPUT;
        parameter.name       = "Gain Reduction 2";
        parameter.symbol     = "gr2";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramGainR3:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_OUTPUT;
        parameter.name       = "Gain Reduction 3";
        parameter.symbol     = "gr3";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramXover1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_LOGARITHMIC;
        parameter.name       = "Crossover freq 1";
        parameter.symbol     = "xover1";
        parameter.unit       = "Hz";
        parameter.ranges.def = 500.0f;
        parameter.ranges.min = 20.0f;
        parameter.ranges.max = 20000.0f;
        break;
    case paramXover2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_LOGARITHMIC;
        parameter.name       = "Crossover freq 2";
        parameter.symbol     = "xover2";
        parameter.unit       = "Hz";
        parameter.ranges.def = 3000.0f;
        parameter.ranges.min = 20.0f;
        parameter.ranges.max = 20000.0f;
        break;
    case paramToggle1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_BOOLEAN;
        parameter.name       = "ZamComp 1 ON";
        parameter.symbol     = "toggle1";
        parameter.unit       = " ";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramToggle2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_BOOLEAN;
        parameter.name       = "ZamComp 2 ON";
        parameter.symbol     = "toggle2";
        parameter.unit       = " ";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramToggle3:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_BOOLEAN;
        parameter.name       = "ZamComp 3 ON";
        parameter.symbol     = "toggle3";
        parameter.unit       = " ";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramListen1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_BOOLEAN;
        parameter.name       = "Listen 1";
        parameter.symbol     = "listen1";
        parameter.unit       = " ";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramListen2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_BOOLEAN;
        parameter.name       = "Listen 2";
        parameter.symbol     = "listen2";
        parameter.unit       = " ";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramListen3:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_BOOLEAN;
        parameter.name       = "Listen 3";
        parameter.symbol     = "listen3";
        parameter.unit       = " ";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramGlobalGain:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Master Trim";
        parameter.symbol     = "globalgain";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -12.0f;
        parameter.ranges.max = 12.0f;
        break;
    case paramStereoDet:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_BOOLEAN;
        parameter.name       = "Detection (MAX/avg)";
        parameter.symbol     = "stereodet";
        parameter.unit       = " ";
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramOutputLevelL:
        parameter.hints      = PARAMETER_IS_OUTPUT;
        parameter.name       = "Output Left";
        parameter.symbol     = "outl";
        parameter.unit       = "dB";
        parameter.ranges.def = -45.0f;
        parameter.ranges.min = -45.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramOutputLevelR:
        parameter.hints      = PARAMETER_IS_OUTPUT;
        parameter.name       = "Output Right";
        parameter.symbol     = "outr";
        parameter.unit       = "dB";
        parameter.ranges.def = -45.0f;
        parameter.ranges.min = -45.0f;
        parameter.ranges.max = 20.0f;
        break;
    }
}

void ZaMultiCompX2Plugin::d_initProgramName(uint32_t index, d_string& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

// -----------------------------------------------------------------------
// Internal data

float ZaMultiCompX2Plugin::d_getParameterValue(uint32_t index) const
{
    switch (index)
    {
    case paramAttack:
        return attack;
        break;
    case paramRelease:
        return release;
        break;
    case paramKnee:
        return knee;
        break;
    case paramRatio:
        return ratio;
        break;
    case paramThresh1:
        return thresdb[0];
        break;
    case paramThresh2:
        return thresdb[1];
        break;
    case paramThresh3:
        return thresdb[2];
        break;
    case paramMakeup1:
        return makeup[0];
        break;
    case paramMakeup2:
        return makeup[1];
        break;
    case paramMakeup3:
        return makeup[2];
        break;
    case paramGainR1:
        return gainr[0];
        break;
    case paramGainR2:
        return gainr[1];
        break;
    case paramGainR3:
        return gainr[2];
        break;
    case paramXover1:
        return xover1;
        break;
    case paramXover2:
        return xover2;
        break;
    case paramToggle1:
        return toggle[0];
        break;
    case paramToggle2:
        return toggle[1];
        break;
    case paramToggle3:
        return toggle[2];
        break;
    case paramListen1:
        return listen[0];
        break;
    case paramListen2:
        return listen[1];
        break;
    case paramListen3:
        return listen[2];
        break;
    case paramGlobalGain:
        return globalgain;
        break;
    case paramStereoDet:
        return stereodet;
        break;
    case paramOutputLevelL:
        return outl;
        break;
    case paramOutputLevelR:
        return outr;
        break;
    default:
        return 0.0f;
    }
}

void ZaMultiCompX2Plugin::d_setParameterValue(uint32_t index, float value)
{
    switch (index)
    {
    case paramAttack:
        attack = value;
        break;
    case paramRelease:
        release = value;
        break;
    case paramKnee:
        knee = value;
        break;
    case paramRatio:
        ratio = value;
        break;
    case paramThresh1:
        thresdb[0] = value;
        break;
    case paramThresh2:
        thresdb[1] = value;
        break;
    case paramThresh3:
        thresdb[2] = value;
        break;
    case paramMakeup1:
        makeup[0] = value;
        break;
    case paramMakeup2:
        makeup[1] = value;
        break;
    case paramMakeup3:
        makeup[2] = value;
        break;
    case paramGainR1:
        gainr[0] = value;
        break;
    case paramGainR2:
        gainr[1] = value;
        break;
    case paramGainR3:
        gainr[2] = value;
        break;
    case paramXover1:
        xover1 = value;
        break;
    case paramXover2:
        xover2 = value;
        break;
    case paramToggle1:
        toggle[0] = value;
        if (value == 0.f)
	    gainr[0] = 0.f;
        break;
    case paramToggle2:
        toggle[1] = value;
        if (value == 0.f)
	    gainr[1] = 0.f;
        break;
    case paramToggle3:
        toggle[2] = value;
        if (value == 0.f)
	    gainr[2] = 0.f;
        break;
    case paramListen1:
        listen[0] = value;
        if (value == 0.f)
	    gainr[0] = 0.f;
        break;
    case paramListen2:
        listen[1] = value;
        if (value == 0.f)
	    gainr[1] = 0.f;
        break;
    case paramListen3:
        listen[2] = value;
        if (value == 0.f)
	    gainr[2] = 0.f;
        break;
    case paramGlobalGain:
        globalgain = value;
        break;
    case paramStereoDet:
        stereodet = value;
        break;
    case paramOutputLevelL:
        outl = value;
        break;
    case paramOutputLevelR:
        outr = value;
        break;
    }
}

void ZaMultiCompX2Plugin::d_setProgram(uint32_t index)
{
    if (index != 0)
        return;

    /* Default parameter values */
    attack = 10.0f;
    release = 80.0f;
    knee = 0.0f;
    ratio = 4.0f;
    thresdb[0] = -20.0f;
    thresdb[1] = -18.0f;
    thresdb[2] = -16.0f;
    makeup[0] = 0.0f;
    makeup[1] = 0.0f;
    makeup[2] = 0.0f;
    gainr[0] = 0.0f;
    gainr[1] = 0.0f;
    gainr[2] = 0.0f;
    xover1 = 500.0f;
    xover2 = 3000.0f;
    toggle[0] = 0.0f;
    toggle[1] = 0.0f;
    toggle[2] = 0.0f;
    listen[0] = 0.0f;
    listen[1] = 0.0f;
    listen[2] = 0.0f;
    globalgain = 0.0f;
    stereodet = 1.0f;
    outl = -45.f;
    outr = -45.f;
    maxL = 0.f;
    maxR = 0.f;

    /* Default variable values */

    /* reset filter values */
    d_activate();
}

void ZaMultiCompX2Plugin::d_setState(const char* key, const char* value)
{
    resetl = true;
    resetr = true;
}

void ZaMultiCompX2Plugin::d_initStateKey(unsigned int key, d_string& val)
{
}

// -----------------------------------------------------------------------
// Process

void ZaMultiCompX2Plugin::d_activate()
{
        int i,j;
        for (i = 0; i < MAX_COMP; i++)
        	for (j = 0; j < 2; j++)
                	old_yl[j][i]=old_y1[j][i]=0.f;

        for (i = 0; i < MAX_FILT; i++) {
        	for (j = 0; j < 2; j++) {
                	a0[j][i] = a1[j][i] = a2[j][i] = 0.f;
                	b1[j][i] = b2[j][i] = 0.f;
                	w1[j][i] = w2[j][i] = 0.f;
                	z1[j][i] = z2[j][i] = 0.f;
		}
        }
	maxL = maxR = 0.f;
}

float ZaMultiCompX2Plugin::run_filter(int i, int ch, float in)
{
        in = sanitize_denormal(in);
        w1[ch][i] = sanitize_denormal(w1[ch][i]);
        w2[ch][i] = sanitize_denormal(w2[ch][i]);
	z1[ch][i] = sanitize_denormal(z1[ch][i]);
	z2[ch][i] = sanitize_denormal(z2[ch][i]);

        float out = in * a0[ch][i] 	+ w1[ch][i] * a1[ch][i] + w2[ch][i] * a2[ch][i]
					- z1[ch][i] * b1[ch][i] - z2[ch][i] * b2[ch][i];
	out = sanitize_denormal(out);
        w2[ch][i] = w1[ch][i];
        z2[ch][i] = z1[ch][i];
        w1[ch][i] = in;
	z1[ch][i] = out;
        return out;
}

void ZaMultiCompX2Plugin::set_lp_coeffs(float fc, float q, float sr, int i, int ch, float gain=1.0)
{
        float omega=(float)(2.f*M_PI*fc/sr);
        float sn=sin(omega);
        float cs=cos(omega);
        float alpha=(float)(sn/(2.f*q));
        float inv=(float)(1.0/(1.0+alpha));

        a2[ch][i] = a0[ch][i] =  (float)(gain*inv*(1.f - cs)*0.5f);
        a1[ch][i] = a0[ch][i] + a0[ch][i];
        b1[ch][i] = (float)(-2.f*cs*inv);
        b2[ch][i] = (float)((1.f - alpha)*inv);
}

void ZaMultiCompX2Plugin::set_hp_coeffs(float fc, float q, float sr, int i, int ch, float gain=1.0)
{
        float omega=(float)(2.f*M_PI*fc/sr);
        float sn=sin(omega);
        float cs=cos(omega);
        float alpha=(float)(sn/(2.f*q));
        float inv=(float)(1.f/(1.f+alpha));

        a0[ch][i] =  (float)(gain*inv*(1.f + cs)/2.f);
        a1[ch][i] =  -2.f * a0[ch][i];
        a2[ch][i] =  a0[ch][i];
        b1[ch][i] =  (float)(-2.f*cs*inv);
        b2[ch][i] =  (float)((1.f - alpha)*inv);
}

void ZaMultiCompX2Plugin::run_comp(int k, float inL, float inR, float *outL, float *outR)
{
	float srate = d_getSampleRate();
        float width=(knee-0.99f)*6.f;
        float attack_coeff = exp(-1000.f/(attack * srate));
        float release_coeff = exp(-1000.f/(release * srate));
	int stereolink = (stereodet > 0.5f) ? STEREOLINK_MAX : STEREOLINK_AVERAGE;

        float cdb=0.f;
        float Lgain = 1.f;
        float Rgain = 1.f;
        float Lxg, Lyg;
        float Rxg, Ryg;
        float Lxl, Lyl, Ly1;
        float Rxl, Ryl, Ry1;

        Lyg = Ryg = 0.f;
	inL = sanitize_denormal(inL);
	inR = sanitize_denormal(inR);
        Lxg = (inL==0.f) ? -160.f : to_dB(fabs(inL));
        Rxg = (inR==0.f) ? -160.f : to_dB(fabs(inR));
        Lxg = sanitize_denormal(Lxg);
        Rxg = sanitize_denormal(Rxg);


        if (2.f*(Lxg-thresdb[k])<-width) {
                Lyg = Lxg;
        } else if (2.f*fabs(Lxg-thresdb[k])<=width) {
                Lyg = Lxg + (1.f/ratio-1.f)*(Lxg-thresdb[k]+width/2.f)*(Lxg-thresdb[k]+width/2.f)/(2.f*width);
        } else if (2.f*(Lxg-thresdb[k])>width) {
                Lyg = thresdb[k] + (Lxg-thresdb[k])/ratio;
        }

        Lyg = sanitize_denormal(Lyg);

        if (2.f*(Rxg-thresdb[k])<-width) {
                Ryg = Rxg;
        } else if (2.f*fabs(Rxg-thresdb[k])<=width) {
                Ryg = Rxg + (1.f/ratio-1.f)*(Rxg-thresdb[k]+width/2.f)*(Rxg-thresdb[k]+width/2.f)/(2.f*width);
        } else if (2.f*(Rxg-thresdb[k])>width) {
                Ryg = thresdb[k] + (Rxg-thresdb[k])/ratio;
        }

        Ryg = sanitize_denormal(Ryg);

        if (stereolink == STEREOLINK_MAX) {
                Lxl = Rxl = fmaxf(Lxg - Lyg, Rxg - Ryg);
        } else {
                Lxl = Rxl = (Lxg - Lyg + Rxg - Ryg) / 2.f;
        }

        old_y1[0][k] = sanitize_denormal(old_y1[0][k]);
        old_y1[1][k] = sanitize_denormal(old_y1[1][k]);
        old_yl[0][k] = sanitize_denormal(old_yl[0][k]);
        old_yl[1][k] = sanitize_denormal(old_yl[1][k]);


        Ly1 = fmaxf(Lxl, release_coeff * old_y1[0][k]+(1.f-release_coeff)*Lxl);
        Lyl = attack_coeff * old_yl[0][k]+(1.f-attack_coeff)*Ly1;
        Ly1 = sanitize_denormal(Ly1);
        Lyl = sanitize_denormal(Lyl);

        cdb = -Lyl;
        Lgain = from_dB(cdb);

        Ry1 = fmaxf(Rxl, release_coeff * old_y1[1][k]+(1.f-release_coeff)*Rxl);
        Ryl = attack_coeff * old_yl[1][k]+(1.f-attack_coeff)*Ry1;
        Ry1 = sanitize_denormal(Ry1);
        Ryl = sanitize_denormal(Ryl);

        cdb = -Ryl;
        Rgain = from_dB(cdb);

        if (stereolink == STEREOLINK_MAX)
		gainr[k] = fmaxf(Lyl, Ryl);
	else
        	gainr[k] = (Lyl + Ryl) / 2.f;

	*outL = inL;
	*outL *= Lgain;
	*outR = inR;
	*outR *= Rgain;

        old_yl[0][k] = Lyl;
        old_yl[1][k] = Ryl;
        old_y1[0][k] = Ly1;
        old_y1[1][k] = Ry1;
}

void ZaMultiCompX2Plugin::d_run(const float** inputs, float** outputs, uint32_t frames)
{
	float srate = d_getSampleRate();
	float maxxL = maxL;
	float maxxR = maxR;

        int tog1 = (toggle[0] > 0.5f) ? 1 : 0;
        int tog2 = (toggle[1] > 0.5f) ? 1 : 0;
        int tog3 = (toggle[2] > 0.5f) ? 1 : 0;

        int listen1 = (listen[0] > 0.5f) ? 1 : 0;
        int listen2 = (listen[1] > 0.5f) ? 1 : 0;
        int listen3 = (listen[2] > 0.5f) ? 1 : 0;

        set_lp_coeffs(xover1, ONEOVERROOT2, srate, 0, 0);
        set_lp_coeffs(xover1, ONEOVERROOT2, srate, 1, 0);
        set_hp_coeffs(xover1, ONEOVERROOT2, srate, 2, 0);
        set_hp_coeffs(xover1, ONEOVERROOT2, srate, 3, 0);
        set_lp_coeffs(xover2, ONEOVERROOT2, srate, 4, 0);
        set_lp_coeffs(xover2, ONEOVERROOT2, srate, 5, 0);
        set_hp_coeffs(xover2, ONEOVERROOT2, srate, 6, 0);
        set_hp_coeffs(xover2, ONEOVERROOT2, srate, 7, 0);

        set_lp_coeffs(xover1, ONEOVERROOT2, srate, 0, 1);
        set_lp_coeffs(xover1, ONEOVERROOT2, srate, 1, 1);
        set_hp_coeffs(xover1, ONEOVERROOT2, srate, 2, 1);
        set_hp_coeffs(xover1, ONEOVERROOT2, srate, 3, 1);
        set_lp_coeffs(xover2, ONEOVERROOT2, srate, 4, 1);
        set_lp_coeffs(xover2, ONEOVERROOT2, srate, 5, 1);
        set_hp_coeffs(xover2, ONEOVERROOT2, srate, 6, 1);
        set_hp_coeffs(xover2, ONEOVERROOT2, srate, 7, 1);

        for (uint32_t i = 0; i < frames; ++i) {
                float tmp1[2], tmp2[2], tmp3[2], tmp4[2], tmp5[2], tmp6[2];
		float fil1[2], fil2[2], fil3[2], fil4[2];
		float outL[MAX_COMP] = {0.f};
		float outR[MAX_COMP] = {0.f};
		float inl = sanitize_denormal(inputs[0][i]);
		float inr = sanitize_denormal(inputs[1][i]);
		inl = (fabs(inl) < DANGER) ? inl : 0.f;
		inr = (fabs(inr) < DANGER) ? inr : 0.f;

		int listenmode = 0;

		// Interleaved channel processing
                fil1[0] = run_filter(0, 0, inl);
                fil1[1] = run_filter(0, 1, inr);
                tmp1[0] = run_filter(1, 0, fil1[0]);
                tmp1[1] = run_filter(1, 1, fil1[1]);
		if (tog1)
			run_comp(0, tmp1[0], tmp1[1], &outL[0], &outR[0]);

		tmp2[0] = tog1 ? outL[0] * from_dB(makeup[0]) : tmp1[0];
                tmp2[1] = tog1 ? outR[0] * from_dB(makeup[0]) : tmp1[1];

                fil2[0] = run_filter(2, 0, inl);
                fil2[1] = run_filter(2, 1, inr);
                tmp3[0] = run_filter(3, 0, fil2[0]);
                tmp3[1] = run_filter(3, 1, fil2[1]);
                fil3[0] = run_filter(4, 0, tmp3[0]);
                fil3[1] = run_filter(4, 1, tmp3[1]);
                tmp4[0] = run_filter(5, 0, fil3[0]);
                tmp4[1] = run_filter(5, 1, fil3[1]);
		if (tog2)
			run_comp(1, tmp4[0], tmp4[1], &outL[1], &outR[1]);

                tmp3[0] = tog2 ? outL[1] * from_dB(makeup[1]) : tmp4[0];
                tmp3[1] = tog2 ? outR[1] * from_dB(makeup[1]) : tmp4[1];

                fil4[0] = run_filter(6, 0, inl);
                fil4[1] = run_filter(6, 1, inr);
                tmp5[0] = run_filter(7, 0, fil4[0]);
                tmp5[1] = run_filter(7, 1, fil4[1]);
		if (tog3)
			run_comp(2, tmp5[0], tmp5[1], &outL[2], &outR[2]);

                tmp6[0] = tog3 ? outL[2] * from_dB(makeup[2]) : tmp5[0];
                tmp6[1] = tog3 ? outR[2] * from_dB(makeup[2]) : tmp5[1];


		outputs[0][i] = outputs[1][i] = 0.f;
		if (listen1) {
			listenmode = 1;
			outputs[0][i] += outL[0] * tog1*from_dB(makeup[0])
					+ (1.-tog1) * tmp1[0];
			outputs[1][i] += outR[0] * tog1*from_dB(makeup[0])
					+ (1.-tog1) * tmp1[1];
		}
		if (listen2) {
			listenmode = 1;
			outputs[0][i] += outL[1] * tog2*from_dB(makeup[1])
					+ (1.-tog2) * tmp4[0];
			outputs[1][i] += outR[1] * tog2*from_dB(makeup[1])
					+ (1.-tog2) * tmp4[1];
		}
		if (listen3) {
			listenmode = 1;
			outputs[0][i] += outL[2] * tog3*from_dB(makeup[2])
					+ (1.-tog3) * tmp5[0];
			outputs[1][i] += outR[2] * tog3*from_dB(makeup[2])
					+ (1.-tog3) * tmp5[1];
		}
		if (!listenmode) {
                	outputs[0][i] = tmp2[0] + tmp3[0] + tmp6[0];
                	outputs[1][i] = tmp2[1] + tmp3[1] + tmp6[1];
		}
                outputs[0][i] = sanitize_denormal(outputs[0][i]);
                outputs[1][i] = sanitize_denormal(outputs[1][i]);
                outputs[0][i] *= from_dB(globalgain);
                outputs[1][i] *= from_dB(globalgain);

		if (resetl) {
			maxL = fabsf(outputs[0][i]);
			resetl = false;
		} else {
			maxxL = (fabsf(outputs[0][i]) > maxxL) ? fabsf(outputs[0][i]) : sanitize_denormal(maxxL);
		}
		if (resetr) {
			maxR = fabsf(outputs[1][i]);
			resetr = false;
		} else {
			maxxR = (fabsf(outputs[1][i]) > maxxR) ? fabsf(outputs[1][i]) : sanitize_denormal(maxxR);
		}
        }
	outl = (maxxL <= 0.f) ? -160.f : to_dB(maxxL);
	outr = (maxxR <= 0.f) ? -160.f : to_dB(maxxR);
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new ZaMultiCompX2Plugin();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
