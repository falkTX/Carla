/*
 * ZamEQ2 2 band parametric equaliser
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

#include "ZamEQ2Plugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

ZamEQ2Plugin::ZamEQ2Plugin()
    : Plugin(paramCount, 1, 0) // 1 program, 0 states
{
    // set default values
    d_setProgram(0);

    // reset
    d_deactivate();
}

ZamEQ2Plugin::~ZamEQ2Plugin()
{
}

// -----------------------------------------------------------------------
// Init

void ZamEQ2Plugin::d_initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
    case paramGain1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Boost/Cut 1";
        parameter.symbol     = "boost1";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -50.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramQ1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_LOGARITHMIC;
        parameter.name       = "Bandwidth 1";
        parameter.symbol     = "bw1";
        parameter.unit       = " ";
        parameter.ranges.def = 2.0f;
        parameter.ranges.min = 0.1f;
        parameter.ranges.max = 6.0f;
        break;
    case paramFreq1:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_LOGARITHMIC;
        parameter.name       = "Frequency 1";
        parameter.symbol     = "f1";
        parameter.unit       = "Hz";
        parameter.ranges.def = 500.0f;
        parameter.ranges.min = 20.0f;
        parameter.ranges.max = 14000.0f;
        break;
    case paramGain2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Boost/Cut 2";
        parameter.symbol     = "boost2";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -50.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramQ2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_LOGARITHMIC;
        parameter.name       = "Bandwidth 2";
        parameter.symbol     = "bw2";
        parameter.unit       = " ";
        parameter.ranges.def = 2.0f;
        parameter.ranges.min = 0.1f;
        parameter.ranges.max = 6.0f;
        break;
    case paramFreq2:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_LOGARITHMIC;
        parameter.name       = "Frequency 2";
        parameter.symbol     = "f2";
        parameter.unit       = "Hz";
        parameter.ranges.def = 3000.0f;
        parameter.ranges.min = 20.0f;
        parameter.ranges.max = 14000.0f;
        break;
    case paramGainL:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Boost/Cut L";
        parameter.symbol     = "boostl";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -50.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramFreqL:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_LOGARITHMIC;
        parameter.name       = "Frequency L";
        parameter.symbol     = "fl";
        parameter.unit       = "Hz";
        parameter.ranges.def = 250.0f;
        parameter.ranges.min = 20.0f;
        parameter.ranges.max = 14000.0f;
        break;
    case paramGainH:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Boost/Cut H";
        parameter.symbol     = "boosth";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -50.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramFreqH:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_LOGARITHMIC;
        parameter.name       = "Frequency H";
        parameter.symbol     = "fh";
        parameter.unit       = "Hz";
        parameter.ranges.def = 8000.0f;
        parameter.ranges.min = 20.0f;
        parameter.ranges.max = 14000.0f;
        break;
    case paramMaster:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Master Gain";
        parameter.symbol     = "master";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -12.0f;
        parameter.ranges.max = 12.0f;
        break;
    case paramTogglePeaks:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_BOOLEAN;
        parameter.name       = "Peaks ON";
        parameter.symbol     = "peaks";
        parameter.unit       = " ";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    }
}

void ZamEQ2Plugin::d_initProgramName(uint32_t index, d_string& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

// -----------------------------------------------------------------------
// Internal data

float ZamEQ2Plugin::d_getParameterValue(uint32_t index) const
{
    switch (index)
    {
    case paramGain1:
        return gain1;
        break;
    case paramQ1:
        return q1;
        break;
    case paramFreq1:
        return freq1;
        break;
    case paramGain2:
        return gain2;
        break;
    case paramQ2:
        return q2;
        break;
    case paramFreq2:
        return freq2;
        break;
    case paramGainL:
        return gainl;
        break;
    case paramFreqL:
        return freql;
        break;
    case paramGainH:
        return gainh;
        break;
    case paramFreqH:
        return freqh;
        break;
    case paramMaster:
        return master;
        break;
    case paramTogglePeaks:
        return togglepeaks;
        break;
    default:
        return 0.0f;
    }
}

void ZamEQ2Plugin::d_setParameterValue(uint32_t index, float value)
{
    switch (index)
    {
    case paramGain1:
        gain1 = value;
        break;
    case paramQ1:
        q1 = value;
        break;
    case paramFreq1:
        freq1 = value;
        break;
    case paramGain2:
        gain2 = value;
        break;
    case paramQ2:
        q2 = value;
        break;
    case paramFreq2:
        freq2 = value;
        break;
    case paramGainL:
        gainl = value;
	break;
    case paramFreqL:
        freql = value;
        break;
    case paramGainH:
        gainh = value;
        break;
    case paramFreqH:
        freqh = value;
        break;
    case paramMaster:
        master = value;
        break;
    case paramTogglePeaks:
        togglepeaks = value;
        break;
    }
}

void ZamEQ2Plugin::d_setProgram(uint32_t index)
{
    if (index != 0)
        return;

    /* Default parameter values */
    gain1 = 0.0f;
    q1 = 1.0f;
    freq1 = 500.0f;
    gain2 = 0.0f;
    q2 = 1.0f;
    freq2 = 3000.0f;
    gainl = 0.0f;
    freql = 250.0f;
    gainh = 0.0f;
    freqh = 8000.0f;
    master = 0.f;
    togglepeaks = 0.f;

    /* Default variable values */

    /* reset filter values */
    d_activate();
}

// -----------------------------------------------------------------------
// Process

void ZamEQ2Plugin::d_activate()
{
        int i;
	for (i = 0; i < MAX_FILT; ++i) {
		x1[0][i] = x2[0][i] = 0.f;
		y1[0][i] = y2[0][i] = 0.f;
		b0[0][i] = b1[0][i] = b2[0][i] = 0.f;
		a1[0][i] = a2[0][i] = 0.f;
        }
}

void ZamEQ2Plugin::d_deactivate()
{
    // all values to zero
}


void ZamEQ2Plugin::lowshelf(int i, int ch, float srate, float fc, float g)
{
	float k, v0;

	k = tanf(M_PI * fc / srate);
	v0 = powf(10., g / 20.);

	if (g < 0.f) {
		// LF cut
		float denom = v0 + sqrt(2. * v0)*k + k*k;
		b0[ch][i] = v0 * (1. + sqrt(2.)*k + k*k) / denom; 
		b1[ch][i] = 2. * v0*(k*k - 1.) / denom;
		b2[ch][i] = v0 * (1. - sqrt(2.)*k + k*k) / denom;
		a1[ch][i] = 2. * (k*k - v0) / denom;
		a2[ch][i] = (v0 - sqrt(2. * v0)*k + k*k) / denom;
	} else {
		// LF boost
		float denom = 1. + sqrt(2.)*k + k*k;
		b0[ch][i] = (1. + sqrt(2. * v0)*k + v0*k*k) / denom;
		b1[ch][i] = 2. * (v0*k*k - 1.) / denom;
		b2[ch][i] = (1. - sqrt(2. * v0)*k + v0*k*k) / denom;
		a1[ch][i] = 2. * (k*k - 1.) / denom;
		a2[ch][i] = (1. - sqrt(2.)*k + k*k) / denom;
	}
}

void ZamEQ2Plugin::highshelf(int i, int ch, float srate, float fc, float g)
{
	float k, v0;

	k = tanf(M_PI * fc / srate);
	v0 = powf(10., g / 20.);

	if (g < 0.f) {
		// HF cut
		float denom = 1. + sqrt(2. * v0)*k + v0*k*k;
		b0[ch][i] = v0*(1. + sqrt(2.)*k + k*k) / denom;
		b1[ch][i] = 2. * v0*(k*k - 1.) / denom;
		b2[ch][i] = v0*(1. - sqrt(2.)*k + k*k) / denom;
		a1[ch][i] = 2. * (v0*k*k - 1.) / denom;
		a2[ch][i] = (1. - sqrt(2. * v0)*k + v0*k*k) / denom;
	} else {
		// HF boost
		float denom = 1. + sqrt(2.)*k + k*k;
		b0[ch][i] = (v0 + sqrt(2. * v0)*k + k*k) / denom;
		b1[ch][i] = 2. * (k*k - v0) / denom;
		b2[ch][i] = (v0 - sqrt(2. * v0)*k + k*k) / denom;
		a1[ch][i] = 2. * (k*k - 1.) / denom;
		a2[ch][i] = (1. - sqrt(2.)*k + k*k) / denom;
	}
}

void ZamEQ2Plugin::peq(int i, int ch, float srate, float fc, float g, float bw)
{
	float k, v0, q;

	k = tanf(M_PI * fc / srate);
	v0 = powf(10., g / 20.);
	q = powf(2., 1./bw)/(powf(2., bw) - 1.); //q from octave bw

	if (g < 0.f) {
		// cut
		float denom = 1. + k/(v0*q) + k*k;
		b0[ch][i] = (1. + k/q + k*k) / denom;
		b1[ch][i] = 2. * (k*k - 1.) / denom;
		b2[ch][i] = (1. - k/q + k*k) / denom;
		a1[ch][i] = b1[ch][i];
		a2[ch][i] = (1. - k/(v0*q) + k*k) / denom;
	} else {
		// boost
		float denom = 1. + k/q + k*k;
		b0[ch][i] = (1. + k*v0/q + k*k) / denom;
		b1[ch][i] = 2. * (k*k - 1.) / denom; 
		b2[ch][i] = (1. - k*v0/q + k*k) / denom;
		a1[ch][i] = b1[ch][i];
		a2[ch][i] = (1. - k/q + k*k) / denom;
	}
}

float ZamEQ2Plugin::run_filter(int i, int ch, double in)
{
	double out;
	in = sanitize_denormal(in);
	out = in * b0[ch][i] 	+ x1[ch][i] * b1[ch][i] 
				+ x2[ch][i] * b2[ch][i]
				- y1[ch][i] * a1[ch][i]
				- y2[ch][i] * a2[ch][i] + 1e-20f;
	out = sanitize_denormal(out);
	x2[ch][i] = sanitize_denormal(x1[ch][i]);
	y2[ch][i] = sanitize_denormal(y1[ch][i]);
	x1[ch][i] = in;
	y1[ch][i] = out;

	return (float) out;
}

void ZamEQ2Plugin::d_run(float** inputs, float** outputs, uint32_t frames)
{
	float srate = d_getSampleRate();

	lowshelf(0, 0, srate, freql, gainl);
        peq(1, 0, srate, freq1, gain1, q1);
        peq(2, 0, srate, freq2, gain2, q2);
        highshelf(3, 0, srate, freqh, gainh);

        for (uint32_t i = 0; i < frames; i++) {
                double tmp,tmpl, tmph;
                double in = inputs[0][i];
                in = sanitize_denormal(in);

                //lowshelf
                tmpl = (gainl == 0.f) ? in : run_filter(0, 0, in);

                //highshelf
                tmph = (gainh == 0.f) ? tmpl : run_filter(3, 0, tmpl);

                //parametric1
                tmp = (gain1 == 0.f) ? tmph : run_filter(1, 0, tmph);

		//parametric2
		tmpl = (gain2 == 0.f) ? tmp : run_filter(2, 0, tmp);

                outputs[0][i] = inputs[0][i];
                outputs[0][i] = (float) tmpl;
		outputs[0][i] *= from_dB(master);
	}
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new ZamEQ2Plugin();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
