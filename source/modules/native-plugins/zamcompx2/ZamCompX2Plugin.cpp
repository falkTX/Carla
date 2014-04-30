/*
 * ZamCompX2 Stereo compressor
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

#include "ZamCompX2Plugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

ZamCompX2Plugin::ZamCompX2Plugin()
    : Plugin(paramCount, 1, 0) // 1 program, 0 states
{
    // set default values
    d_setProgram(0);

    // reset
    d_deactivate();
}

ZamCompX2Plugin::~ZamCompX2Plugin()
{
}

// -----------------------------------------------------------------------
// Init

void ZamCompX2Plugin::d_initParameter(uint32_t index, Parameter& parameter)
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
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -80.0f;
        parameter.ranges.max = 0.0f;
        break;
    case paramMakeup:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Makeup";
        parameter.symbol     = "mak";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 30.0f;
        break;
    case paramGainRed:
        parameter.hints      = PARAMETER_IS_OUTPUT;
        parameter.name       = "Gain Reduction";
        parameter.symbol     = "gr";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramStereo:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_INTEGER;
        parameter.name       = "Stereolink";
        parameter.symbol     = "stereo";
        parameter.unit       = " ";
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 2.0f;
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

void ZamCompX2Plugin::d_initProgramName(uint32_t index, d_string& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

// -----------------------------------------------------------------------
// Internal data

float ZamCompX2Plugin::d_getParameterValue(uint32_t index) const
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
    case paramMakeup:
        return makeup;
        break;
    case paramGainRed:
        return gainred;
        break;
    case paramStereo:
        return stereolink;
        break;
    case paramOutputLevel:
        return outlevel;
        break;
    default:
        return 0.0f;
    }
}

void ZamCompX2Plugin::d_setParameterValue(uint32_t index, float value)
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
    case paramMakeup:
        makeup = value;
        break;
    case paramGainRed:
        gainred = value;
        break;
    case paramStereo:
        stereolink = value;
        break;
    case paramOutputLevel:
        outlevel = value;
        break;
    }
}

void ZamCompX2Plugin::d_setProgram(uint32_t index)
{
    if (index != 0)
        return;

    /* Default parameter values */
    attack = 10.0f;
    release = 80.0f;
    knee = 0.0f;
    ratio = 4.0f;
    thresdb = 0.0f;
    makeup = 0.0f;
    gainred = 0.0f;
    stereolink = 1.0f;
    outlevel = -45.0f;

    /* Default variable values */

    /* reset filter values */
    d_activate();
}

// -----------------------------------------------------------------------
// Process

void ZamCompX2Plugin::d_activate()
{
	oldL_yl = oldL_y1 = oldR_yl = oldR_y1 = 0.f;
}

void ZamCompX2Plugin::d_deactivate()
{
    // all values to zero
}

void ZamCompX2Plugin::d_run(float** inputs, float** outputs, uint32_t frames)
{
	float srate = d_getSampleRate();
        float width=(knee-0.99f)*6.f;
        float cdb=0.f;
        float attack_coeff = exp(-1000.f/(attack * srate));
        float release_coeff = exp(-1000.f/(release * srate));
	int stereo = (stereolink > 1.f) ? STEREOLINK_MAX : (stereolink > 0.f) ? STEREOLINK_AVERAGE : STEREOLINK_UNCOUPLED;

	float max = 0.f;
        float Lgain = 1.f;
        float Rgain = 1.f;
        float Lxg, Lxl, Lyg, Lyl, Ly1;
        float Rxg, Rxl, Ryg, Ryl, Ry1;
        uint32_t i;

        for (i = 0; i < frames; i++) {
                Lyg = Ryg = 0.f;
                Lxg = (inputs[0][i]==0.f) ? -160.f : to_dB(fabs(inputs[0][i]));
                Rxg = (inputs[1][i]==0.f) ? -160.f : to_dB(fabs(inputs[1][i]));
                Lxg = sanitize_denormal(Lxg);
                Rxg = sanitize_denormal(Rxg);


                if (2.f*(Lxg-thresdb)<-width) {
                        Lyg = Lxg;
                } else if (2.f*fabs(Lxg-thresdb)<=width) {
                        Lyg = Lxg + (1.f/ratio-1.f)*(Lxg-thresdb+width/2.f)*(Lxg-thresdb+width/2.f)/(2.f*width);
                } else if (2.f*(Lxg-thresdb)>width) {
                        Lyg = thresdb + (Lxg-thresdb)/ratio;
                }

                Lyg = sanitize_denormal(Lyg);

                if (2.f*(Rxg-thresdb)<-width) {
                        Ryg = Rxg;
                } else if (2.f*fabs(Rxg-thresdb)<=width) {
                        Ryg = Rxg + (1.f/ratio-1.f)*(Rxg-thresdb+width/2.f)*(Rxg-thresdb+width/2.f)/(2.f*width);
                } else if (2.f*(Rxg-thresdb)>width) {
                        Ryg = thresdb + (Rxg-thresdb)/ratio;
                }

                Ryg = sanitize_denormal(Ryg);

                if (stereo == STEREOLINK_UNCOUPLED) {
                        Lxl = Lxg - Lyg;
                        Rxl = Rxg - Ryg;
                } else if (stereo == STEREOLINK_MAX) {
                        Lxl = Rxl = fmaxf(Lxg - Lyg, Rxg - Ryg);
                } else {
                        Lxl = Rxl = (Lxg - Lyg + Rxg - Ryg) / 2.f;
                }

                oldL_y1 = sanitize_denormal(oldL_y1);
                oldR_y1 = sanitize_denormal(oldR_y1);
                oldL_yl = sanitize_denormal(oldL_yl);
                oldR_yl = sanitize_denormal(oldR_yl);
                Ly1 = fmaxf(Lxl, release_coeff * oldL_y1+(1.f-release_coeff)*Lxl);
                Lyl = attack_coeff * oldL_yl+(1.f-attack_coeff)*Ly1;
                Ly1 = sanitize_denormal(Ly1);
                Lyl = sanitize_denormal(Lyl);

                cdb = -Lyl;
                Lgain = from_dB(cdb);

                gainred = Lyl;

                Ry1 = fmaxf(Rxl, release_coeff * oldR_y1+(1.f-release_coeff)*Rxl);
                Ryl = attack_coeff * oldR_yl+(1.f-attack_coeff)*Ry1;
                Ry1 = sanitize_denormal(Ry1);
                Ryl = sanitize_denormal(Ryl);

                cdb = -Ryl;
                Rgain = from_dB(cdb);

                outputs[0][i] = inputs[0][i];
                outputs[0][i] *= Lgain * from_dB(makeup);
                outputs[1][i] = inputs[1][i];
                outputs[1][i] *= Rgain * from_dB(makeup);
		
		max = (fabsf(outputs[0][i]) > max) ? fabsf(outputs[0][i]) : sanitize_denormal(max);
		max = (fabsf(outputs[1][i]) > max) ? fabsf(outputs[1][i]) : sanitize_denormal(max);

                oldL_yl = Lyl;
                oldR_yl = Ryl;
                oldL_y1 = Ly1;
                oldR_y1 = Ry1;
        }
	outlevel = (max == 0.f) ? -45.f : to_dB(max);
    }

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new ZamCompX2Plugin();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
