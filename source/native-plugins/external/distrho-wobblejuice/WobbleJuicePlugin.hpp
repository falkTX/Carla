/*
 * Wobble Juice Plugin
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

#ifndef WOBBLEJUICEPLUGIN_HPP_INCLUDED
#define WOBBLEJUICEPLUGIN_HPP_INCLUDED

#include "DistrhoPlugin.hpp"
#include "moogVCF.hxx"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class WobbleJuicePlugin : public Plugin
{
public:
    enum Parameters
    {
        paramDivision = 0,
        paramReso,
        paramRange,
        paramPhase,
        paramWave,
        paramDrive,
        paramCount
    };

    float getSinePhase(float x) {
        return ((-std::cos(x)+1)/2);
    }
    float getSawPhase(float x) {
        return (-((2/M_PI * std::atan(1/std::tan(x/2)))-1)/2);
    }
    float getRevSawPhase(float x) {
        return (((2/M_PI * std::atan(1/std::tan(x/2)))+1)/2);
    }
    float getSquarePhase(float x) {
        return (std::round((std::sin(x)+1)/2));
    }

    //saw, sqr, sin, revSaw
    float getBlendedPhase(float x, float wave)
    {
        //wave = 2;
        if (wave>=1 && wave<2) {
            /* saw vs sqr */
            waveBlend = wave-1;
            return (getSawPhase(x)*(1-waveBlend) + getSquarePhase(x)*waveBlend);
        } else if (wave>=2 && wave<3) {
            /* sqr vs sin */
            waveBlend = wave-2;
            return (getSquarePhase(x)*(1-waveBlend) + getSinePhase(x)*waveBlend);
        } else if (wave>=3 && wave<=4) {
            /* sin vs revSaw */
            waveBlend = wave-3;
            return (getSinePhase(x)*(1-waveBlend) + getRevSawPhase(x)*waveBlend);
        } else {
            return 0.0f;
        }
    }

    WobbleJuicePlugin();

protected:
    // -------------------------------------------------------------------
    // Information

    const char* getLabel() const noexcept override
    {
        return "WobbleJuice";
    }

    const char* getMaker() const noexcept override
    {
        return "Andre Sklenar";
    }

    const char* getLicense() const noexcept override
    {
        return "GPL v2+";
    }

    uint32_t getVersion() const noexcept override
    {
        return 0x1000;
    }

    int64_t getUniqueId() const noexcept override
    {
        return d_cconst('W', 'b', 'l', 'J');
    }

    // -------------------------------------------------------------------
    // Init

    void initParameter(uint32_t index, Parameter& parameter) override;
    void initProgramName(uint32_t index, String& programName) override;

    // -------------------------------------------------------------------
    // Internal data

    float getParameterValue(uint32_t index) const override;
    void  setParameterValue(uint32_t index, float value) override;
    void  loadProgram(uint32_t index) override;

    // -------------------------------------------------------------------
    // Process

    void activate() override;
    void run(const float** inputs, float** outputs, uint32_t frames) override;

    // -------------------------------------------------------------------

private:
    MoogVCF filterL, filterR;
    float division, reso, range, phase, wave, drive; //parameters
    float bar, tick, tickOffset, percentage, phaseOffset, currentPhaseL, currentPhaseR, posL, posR, cutoffL, cutoffR;
    double sinePos;
    float waveType, waveBlend;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WobbleJuicePlugin)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // WOBBLEJUICE_HPP_INCLUDED
