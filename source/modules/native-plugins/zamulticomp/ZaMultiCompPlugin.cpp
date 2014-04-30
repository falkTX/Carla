/*
 * ZaMultiComp mono multiband compressor
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

#include "ZaMultiCompPlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

ZaMultiCompPlugin::ZaMultiCompPlugin()
    : Plugin(paramCount, 1, 0) // 1 program, 0 states
{
    // set default values
    d_setProgram(0);

    // reset
    d_deactivate();
}

ZaMultiCompPlugin::~ZaMultiCompPlugin()
{
}

// -----------------------------------------------------------------------
// Init

void ZaMultiCompPlugin::d_initParameter(uint32_t index, Parameter& parameter)
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
    case paramThresh:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Threshold";
        parameter.symbol     = "thr";
        parameter.unit       = "dB";
        parameter.ranges.def = -16.0f;
        parameter.ranges.min = -80.0f;
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
        parameter.hints      = PARAMETER_IS_OUTPUT;
        parameter.name       = "Gain Reduction 1";
        parameter.symbol     = "gr1";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramGainR2:
        parameter.hints      = PARAMETER_IS_OUTPUT;
        parameter.name       = "Gain Reduction 2";
        parameter.symbol     = "gr2";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramGainR3:
        parameter.hints      = PARAMETER_IS_OUTPUT;
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
        parameter.ranges.def = 250.0f;
        parameter.ranges.min = 20.0f;
        parameter.ranges.max = 1400.0f;
        break;
    case paramXover2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_LOGARITHMIC;
        parameter.name       = "Crossover freq 2";
        parameter.symbol     = "xover2";
        parameter.unit       = "Hz";
        parameter.ranges.def = 1400.0f;
        parameter.ranges.min = 1400.0f;
        parameter.ranges.max = 14000.0f;
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
    case paramOutputLevel:
        parameter.hints      = PARAMETER_IS_OUTPUT;
        parameter.name       = "Output Level";
        parameter.symbol     = "outlevel";
        parameter.unit       = "dB";
        parameter.ranges.def = -45.0f;
        parameter.ranges.min = -45.0f;
        parameter.ranges.max = 20.0f;
        break;
    }
}

void ZaMultiCompPlugin::d_initProgramName(uint32_t index, d_string& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

// -----------------------------------------------------------------------
// Internal data

float ZaMultiCompPlugin::d_getParameterValue(uint32_t index) const
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
    case paramThresh:
        return thresdb;
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
    case paramOutputLevel:
        return outlevel;
        break;
    default:
        return 0.0f;
    }
}

void ZaMultiCompPlugin::d_setParameterValue(uint32_t index, float value)
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
    case paramThresh:
        thresdb = value;
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
    case paramOutputLevel:
        outlevel = value;
        break;
    }
}

void ZaMultiCompPlugin::d_setProgram(uint32_t index)
{
    if (index != 0)
        return;

    /* Default parameter values */
    attack = 10.0f;
    release = 80.0f;
    knee = 0.0f;
    ratio = 4.0f;
    thresdb = -16.0f;
    makeup[0] = 0.0f;
    makeup[1] = 0.0f;
    makeup[2] = 0.0f;
    gainr[0] = 0.0f;
    gainr[1] = 0.0f;
    gainr[2] = 0.0f;
    xover1 = 250.0f;
    xover2 = 1400.0f;
    toggle[0] = 0.0f;
    toggle[1] = 0.0f;
    toggle[2] = 0.0f;
    listen[0] = 0.0f;
    listen[1] = 0.0f;
    listen[2] = 0.0f;
    globalgain = 0.0f;
    outlevel = -45.0f;

    /* Default variable values */

    /* reset filter values */
    d_activate();
}

// -----------------------------------------------------------------------
// Process

void ZaMultiCompPlugin::d_activate()
{
        int i;
        for (i = 0; i < MAX_COMP; i++) {
                old_yl[i]=old_y1[i]=0.f;
        }

        for (i = 0; i < MAX_FILT; i++) {
                a0[i] = a1[i] = a2[i] = 0.f;
                b1[i] = b2[i] = 0.f;
                w1[i] = w2[i] = 0.f;
                z1[i] = z2[i] = 0.f;
        }
}

void ZaMultiCompPlugin::d_deactivate()
{
    // all values to zero
}

float ZaMultiCompPlugin::run_filter(int i, float in)
{
        in = sanitize_denormal(in);
	w1[i] = sanitize_denormal(w1[i]);
	w2[i] = sanitize_denormal(w2[i]);
	z1[i] = sanitize_denormal(z1[i]);
	z2[i] = sanitize_denormal(z2[i]);

        float out = in * a0[i] + w1[i] * a1[i] + w2[i] * a2[i]
			- z1[i] * b1[i] - z2[i] * b2[i];
	out = sanitize_denormal(out);
        w2[i] = w1[i];
        z2[i] = z1[i];
	w1[i] = in;
        z1[i] = out;
        return out;
}

void ZaMultiCompPlugin::set_lp_coeffs(float fc, float q, float sr, int i, float gain=1.0)
{
        float omega=(float)(2.f*M_PI*fc/sr);
        float sn=sin(omega);
        float cs=cos(omega);
        float alpha=(float)(sn/(2.f*q));
        float inv=(float)(1.0/(1.0+alpha));

        a2[i] = a0[i] =  (float)(gain*inv*(1.f - cs)*0.5f);
        a1[i] = a0[i] + a0[i];
        b1[i] = (float)(-2.f*cs*inv);
        b2[i] = (float)((1.f - alpha)*inv);
}

void ZaMultiCompPlugin::set_hp_coeffs(float fc, float q, float sr, int i, float gain=1.0) 
{       
        float omega=(float)(2.f*M_PI*fc/sr);
        float sn=sin(omega);
        float cs=cos(omega);
        float alpha=(float)(sn/(2.f*q));
        float inv=(float)(1.f/(1.f+alpha));
        
        a0[i] =  (float)(gain*inv*(1.f + cs)/2.f);
        a1[i] =  -2.f * a0[i];
        a2[i] =  a0[i];
        b1[i] =  (float)(-2.f*cs*inv);
        b2[i] =  (float)((1.f - alpha)*inv);
}

float ZaMultiCompPlugin::run_comp(int k, float in)
{
	float srate = d_getSampleRate();
        float width=(knee-0.99f)*6.f;
        float attack_coeff = exp(-1000.f/(attack * srate));
        float release_coeff = exp(-1000.f/(release * srate));

        float cdb=0.f;
        float gain = 1.f;
        float xg, xl, yg, yl, y1;
        float out;

        yg=0.f;
	in = sanitize_denormal(in);
        xg = (in==0.f) ? -160.f : to_dB(fabs(in));
	xg = sanitize_denormal(xg);

        if (2.f*(xg-thresdb)<-width) {
                yg = xg;
        } else if (2.f*fabs(xg-thresdb)<=width) {
                yg = xg + (1.f/ratio-1.f)*(xg-thresdb+width/2.f)*(xg-thresdb+width/2.f)/(2.f*width);
        } else if (2.f*(xg-thresdb)>width) {
                yg = thresdb + (xg-thresdb)/ratio;
        }

        yg = sanitize_denormal(yg);

        xl = xg - yg;
        old_y1[k] = sanitize_denormal(old_y1[k]);
        old_yl[k] = sanitize_denormal(old_yl[k]);

        y1 = fmaxf(xl, release_coeff * old_y1[k]+(1.f-release_coeff)*xl);
        yl = attack_coeff * old_yl[k]+(1.f-attack_coeff)*y1;
        y1 = sanitize_denormal(y1);
        yl = sanitize_denormal(yl);

        cdb = -yl;
        gain = from_dB(cdb);
        gainr[k] = yl;

        out = in;
        out *= gain;

        old_yl[k] = yl;
        old_y1[k] = y1;
        return sanitize_denormal(out);
}

void ZaMultiCompPlugin::d_run(float** inputs, float** outputs, uint32_t frames)
{
	float srate = d_getSampleRate();
	float max = 0.f;

        int tog1 = (toggle[0] > 0.5f) ? 1 : 0;
        int tog2 = (toggle[1] > 0.5f) ? 1 : 0;
        int tog3 = (toggle[2] > 0.5f) ? 1 : 0;

        int listen1 = (listen[0] > 0.5f) ? 1 : 0;
        int listen2 = (listen[1] > 0.5f) ? 1 : 0;
        int listen3 = (listen[2] > 0.5f) ? 1 : 0;

        set_lp_coeffs(xover1, ONEOVERROOT2, srate, 0);
        set_lp_coeffs(xover1, ONEOVERROOT2, srate, 1);
        set_hp_coeffs(xover1, ONEOVERROOT2, srate, 2);
        set_hp_coeffs(xover1, ONEOVERROOT2, srate, 3);
        set_lp_coeffs(xover2, ONEOVERROOT2, srate, 4);
        set_lp_coeffs(xover2, ONEOVERROOT2, srate, 5);
        set_hp_coeffs(xover2, ONEOVERROOT2, srate, 6);
        set_hp_coeffs(xover2, ONEOVERROOT2, srate, 7);

        for (uint32_t i = 0; i < frames; ++i) {
                float tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, fil1, fil2, fil3, fil4;
		float cmp1, cmp2, cmp3, in;
		int listenmode = 0;
		in = sanitize_denormal(inputs[0][i]);
		in = (fabs(in) < DANGER) ? in : 0.f;
                fil1 = run_filter(0, in);
                tmp1 = run_filter(1, fil1);
		cmp1 = (!tog1 && !listen1) ? 0.f : run_comp(0, tmp1);
                tmp2 = tog1 ? cmp1 * from_dB(makeup[0]) : tmp1;
                fil2 = run_filter(2, in);
                tmp3 = run_filter(3, fil2);
                fil3 = run_filter(4, tmp3);
                tmp4 = run_filter(5, fil3);
		cmp2 = (!tog2 && !listen2) ? 0.f : run_comp(1, tmp4);
                tmp3 = (tog2) ? cmp2 * from_dB(makeup[1]) : tmp4;
                fil4 = run_filter(6, in);
                tmp5 = run_filter(7, fil4);
		cmp3 = (!tog3 && !listen3) ? 0.f : run_comp(2, tmp5);
                tmp6 = (tog3) ? cmp3 * from_dB(makeup[2]) : tmp5;
        	outputs[0][i] = 0.f;
		if (listen1) {
			listenmode = 1;
			outputs[0][i] += cmp1 * tog1*from_dB(makeup[0])
					+ (1.-tog1) * tmp1;
		}
		if (listen2) {
			listenmode = 1;
			outputs[0][i] += cmp2 * tog2*from_dB(makeup[1])
					+ (1.-tog2) * tmp4;
		}
		if (listen3) {
			listenmode = 1;
			outputs[0][i] += cmp3 * tog3*from_dB(makeup[2])
					+ (1.-tog3) * tmp5;
		}
		if (!listenmode) {
			outputs[0][i] = 0.f;
			outputs[0][i] += tmp2;
			outputs[0][i] += tmp3;
			outputs[0][i] += tmp6;
		}
		outputs[0][i] = sanitize_denormal(outputs[0][i]);
                outputs[0][i] *= from_dB(globalgain);

		max = (fabsf(outputs[0][i]) > max) ? fabsf(outputs[0][i]) : sanitize_denormal(max);
        }
	outlevel = (max == 0.f) ? -45.f : to_dB(max);
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new ZaMultiCompPlugin();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
