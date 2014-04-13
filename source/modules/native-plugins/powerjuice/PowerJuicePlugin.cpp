/*
 * Power Juice Plugin
 * Copyright (C) 2014 Andre Sklenar <andre.sklenar@gmail.com>, www.juicelab.cz
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

#include "PowerJuicePlugin.hpp"
//#include <cstring>
#include <cstdlib>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

PowerJuicePlugin::PowerJuicePlugin()
    : Plugin(paramCount, 1, 0) // 1 program, 0 states
{
    // set default values
    d_setProgram(0);

    // init shm vars
    carla_shm_init(shm);
    shmData = nullptr;

    // reset
    d_deactivate();
}

PowerJuicePlugin::~PowerJuicePlugin()
{
    closeShm();
}

// -----------------------------------------------------------------------
// Init

void PowerJuicePlugin::d_initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
    case paramAttack:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Attack";
        parameter.symbol     = "att";
        parameter.unit       = "ms";
        parameter.ranges.def = 20.0f;
        parameter.ranges.min = 0.1f;
        parameter.ranges.max = 1000.0f;
        break;
    case paramRelease:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Release";
        parameter.symbol     = "rel";
        parameter.unit       = "ms";
        parameter.ranges.def = 200.0f;
        parameter.ranges.min = 0.1f;
        parameter.ranges.max = 1000.0f;
        break;
    case paramThreshold:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Threshold";
        parameter.symbol     = "thr";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -60.0f;
        parameter.ranges.max = 0.0f;
        break;
    case paramRatio:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Ratio";
        parameter.symbol     = "rat";
        parameter.unit       = "";
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 10.0f;
        break;
    case paramMakeup:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Make-Up";
        parameter.symbol     = "mak";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramMix:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Mix";
        parameter.symbol     = "Mix";
        parameter.unit       = "";
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;

    }
}

void PowerJuicePlugin::d_initProgramName(uint32_t index, d_string& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

void PowerJuicePlugin::d_initStateKey(uint32_t /*index*/, d_string& /*key*/)
{
/*
    if (index != 0)
        return;

    key = "shmKey";
*/
}

// -----------------------------------------------------------------------
// Internal data

float PowerJuicePlugin::d_getParameterValue(uint32_t index) const
{
    switch (index)
    {
    case paramAttack:
        return attack;
    case paramRelease:
        return release;
    case paramThreshold:
        return threshold;
    case paramRatio:
        return ratio;
    case paramMakeup:
        return makeup;
    case paramMix:
        return mix;
    default:
        return 0.0f;
    }
}

void PowerJuicePlugin::d_setParameterValue(uint32_t index, float value)
{
    switch (index)
    {
    case paramAttack:
        attack = value;
        break;
    case paramRelease:
        release = value;
        break;
    case paramThreshold:
        threshold = value;
        break;
    case paramRatio:
        ratio = value;
        break;
    case paramMakeup:
        makeup = value;
        break;
    case paramMix:
        mix = value;
        break;
    }
}

void PowerJuicePlugin::d_setProgram(uint32_t index)
{
    if (index != 0)
        return;

    /* Default parameter values */
    attack = 20.0f;
    release = 200.0f;
    threshold = 0.0f;
    ratio = 1.0f;
    makeup = 0.0f;
    mix = 1.0f;

    /* Default variable values */
    averageCounter = 0;
    inputMin = 0.0f;
    inputMax = 0.0f;

    input.start = 0;
    output.start = 0;
    gainReduction.start = 0;
    std::memset(input.data, 0, sizeof(float)*kFloatStackCount);
    std::memset(output.data, 0, sizeof(float)*kFloatStackCount);
    std::memset(gainReduction.data, 0, sizeof(float)*kFloatStackCount);

    d_activate();
}

void PowerJuicePlugin::d_setState(const char* key, const char* value)
{
    if (std::strcmp(key, "shmKey") != 0)
        return;

    if (value[0] == '\0')
    {
        carla_stdout("Shm closed");
        return closeShm();
    }

    carla_stdout("Got shmKey => %s", value);
    initShm(value);
}

// -----------------------------------------------------------------------
// Process

void PowerJuicePlugin::d_activate()
{
}

void PowerJuicePlugin::d_deactivate()
{
    // all values to zero
}

void PowerJuicePlugin::d_run(float** inputs, float** /*outputs*/, uint32_t frames)
{
    float* in = inputs[0];
    //float* out = outputs[0];

    for (uint32_t i=0; i < frames; i++) {
        //for every sample
        //printf("av");
        //averageInputs[averageCounter] = in[i];
        if (in[i]<inputMin) {
            inputMin = in[i];
        }
        if (in[i]>inputMax) {
            inputMax = in[i];
        }
        if (++averageCounter == 300) {
            //output waveform parameter
            input.data[input.start++] = inputMin;
            input.data[input.start++] = inputMax;

            if (input.start == kFloatStackCount)
                input.start = 0;

            if (shmData != nullptr)
            {
                for (int j=0; j < kFloatStackCount; ++j)
                    shmData->input[j] = input.data[(input.start+j) % kFloatStackCount];
            }

            averageCounter = 0;
            inputMin = 0.0f;
            inputMax = 0.0f;
        }
    }
}

void PowerJuicePlugin::initShm(const char* shmKey)
{
    shm = carla_shm_attach(shmKey);

    if (! carla_is_shm_valid(shm))
    {
        carla_stderr2("Failed to created shared memory!");
        return;
    }

    if (! carla_shm_map<SharedMemData>(shm, shmData))
    {
        carla_stderr2("Failed to map shared memory!");
        return;
    }
}

void PowerJuicePlugin::closeShm()
{
    if (! carla_is_shm_valid(shm))
        return;

    if (shmData != nullptr)
    {
        carla_shm_unmap<SharedMemData>(shm, shmData);
        shmData = nullptr;
    }

    carla_shm_close(shm);
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new PowerJuicePlugin();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
