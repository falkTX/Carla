/*
 * ZamComp mono compressor
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

#include "ZamCompPlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

ZamCompPlugin::ZamCompPlugin()
    : Plugin(paramCount, 1, 0) // 1 program, 0 states
{
    // set default values
    d_setProgram(0);
}

// -----------------------------------------------------------------------
// Init

void ZamCompPlugin::d_initParameter(uint32_t index, Parameter& parameter)
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
        parameter.ranges.max = 200.0f;
        break;
    case paramRelease:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Release";
        parameter.symbol     = "rel";
        parameter.unit       = "ms";
        parameter.ranges.def = 80.0f;
        parameter.ranges.min = 50.0f;
        parameter.ranges.max = 500.0f;
        break;
    case paramKnee:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Knee";
        parameter.symbol     = "kn";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 8.0f;
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
        parameter.ranges.min = -60.0f;
        parameter.ranges.max = 0.0f;
        break;
    case paramMakeup:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Makeup";
        parameter.symbol     = "mak";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -30.0f;
        parameter.ranges.max = 30.0f;
        break;
    case paramGainR:
        parameter.hints      = PARAMETER_IS_OUTPUT;
        parameter.name       = "Gain Reduction";
        parameter.symbol     = "gr";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 40.0f;
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

void ZamCompPlugin::d_initProgramName(uint32_t index, d_string& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

// -----------------------------------------------------------------------
// Internal data

float ZamCompPlugin::d_getParameterValue(uint32_t index) const
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
    case paramGainR:
        return gainr;
        break;
    case paramOutputLevel:
        return outlevel;
        break;
    default:
        return 0.0f;
    }
}

void ZamCompPlugin::d_setParameterValue(uint32_t index, float value)
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
    case paramGainR:
        gainr = value;
        break;
    case paramOutputLevel:
        outlevel = value;
        break;
    }
}

void ZamCompPlugin::d_setProgram(uint32_t index)
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

    /* reset filter values */
    d_activate();
}

// -----------------------------------------------------------------------
// Process

void ZamCompPlugin::d_activate()
{
    gainr = 0.0f;
    outlevel = -45.f;
    old_yl = old_y1 = 0.0f;
}

void ZamCompPlugin::d_run(const float** inputs, float** outputs, uint32_t frames)
{
	float srate = d_getSampleRate();
        float width=(knee-0.99f)*6.f;
        float cdb=0.f;
        float attack_coeff = exp(-1000.f/(attack * srate));
        float release_coeff = exp(-1000.f/(release * srate));

        float gain = 1.f;
        float xg, xl, yg, yl, y1;
        uint32_t i;
	float max = 0.f;

        for (i = 0; i < frames; i++) {
                yg=0.f;
                xg = (inputs[0][i]==0.f) ? -160.f : to_dB(fabs(inputs[0][i]));
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
                old_y1 = sanitize_denormal(old_y1);
                old_yl = sanitize_denormal(old_yl);

                y1 = fmaxf(xl, release_coeff * old_y1+(1.f-release_coeff)*xl);
                yl = attack_coeff * old_yl+(1.f-attack_coeff)*y1;
                y1 = sanitize_denormal(y1);
                yl = sanitize_denormal(yl);

		cdb = -yl;
                gain = from_dB(cdb);

                gainr = yl;

                outputs[0][i] = inputs[0][i];
                outputs[0][i] *= gain * from_dB(makeup);

		max = (fabsf(outputs[0][i]) > max) ? fabsf(outputs[0][i]) : sanitize_denormal(max);

                old_yl = yl;
                old_y1 = y1;
        }
	outlevel = (max == 0.f) ? -45.f : to_dB(max);
    }

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new ZamCompPlugin();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
