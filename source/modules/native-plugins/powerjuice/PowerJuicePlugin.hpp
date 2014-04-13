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

#ifndef POWERJUICEPLUGIN_HPP_INCLUDED
#define POWERJUICEPLUGIN_HPP_INCLUDED

#include "DistrhoPlugin.hpp"
#include "carla/CarlaShmUtils.hpp"

static const int kFloatStackCount = 1126;

struct FloatStack {
    int32_t start;
    float data[kFloatStackCount];
};

struct SharedMemData {
    float input[kFloatStackCount];
    float output[kFloatStackCount];
    float gainReduction[kFloatStackCount];
};

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class PowerJuicePlugin : public Plugin
{
public:
    enum Parameters
    {
        paramAttack = 0,
        paramRelease,
        paramThreshold,
        paramRatio,
        paramMakeup,
        paramMix,
        paramInput,
        paramOutput,
        paramGainReduction,
        paramCount
    };

    PowerJuicePlugin();
    ~PowerJuicePlugin() override;

protected:
    // -------------------------------------------------------------------
    // Information

    const char* d_getLabel() const noexcept override
    {
        return "PowerJuice";
    }

    const char* d_getMaker() const noexcept override
    {
        return "Andre Sklenar";
    }

    const char* d_getLicense() const noexcept override
    {
        return "GPL v2+";
    }

    uint32_t d_getVersion() const noexcept override
    {
        return 0x1000;
    }

    long d_getUniqueId() const noexcept override
    {
        return d_cconst('P', 'w', 'r', 'J');
    }

    // -------------------------------------------------------------------
    // Init

    void d_initParameter(uint32_t index, Parameter& parameter) override;
    void d_initProgramName(uint32_t index, d_string& programName) override;
    void d_initStateKey(uint32_t, d_string&) override;

    // -------------------------------------------------------------------
    // Internal data

    float d_getParameterValue(uint32_t index) const override;
    void  d_setParameterValue(uint32_t index, float value) override;
    void  d_setProgram(uint32_t index) override;
    void  d_setState(const char* key, const char* value) override;

    // -------------------------------------------------------------------
    // Process

    void d_activate() override;
    void d_deactivate() override;
    void d_run(float** inputs, float** outputs, uint32_t frames) override;

    // -------------------------------------------------------------------

private:
    // params
    float attack, release, threshold, ratio, makeup, mix;

    int averageCounter;
    float inputMin, inputMax;

    // this was unused
    // float averageInputs[150];

    FloatStack input, output, gainReduction;

    shm_t shm;
    SharedMemData* shmData;

    void initShm(const char* shmKey);
    void closeShm();
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // POWERJUICE_HPP_INCLUDED
