/*
 * DISTRHO 3BandEQ Plugin, based on 3BandEQ by Michael Gruhn
 * Copyright (C) 2007 Michael Gruhn <michael-gruhn@web.de>
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LICENSE file.
 */

#ifndef DISTRHO_PLUGIN_3BANDEQ_HPP_INCLUDED
#define DISTRHO_PLUGIN_3BANDEQ_HPP_INCLUDED

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class DistrhoPlugin3BandEQ : public Plugin
{
public:
    enum Parameters
    {
        paramLow = 0,
        paramMid,
        paramHigh,
        paramMaster,
        paramLowMidFreq,
        paramMidHighFreq,
        paramCount
    };

    DistrhoPlugin3BandEQ();

protected:
    // -------------------------------------------------------------------
    // Information

    const char* d_getLabel() const noexcept override
    {
        return "3BandEQ";
    }

    const char* d_getMaker() const noexcept override
    {
        return "DISTRHO";
    }

    const char* d_getLicense() const noexcept override
    {
        return "LGPL";
    }

    uint32_t d_getVersion() const noexcept override
    {
        return 0x1000;
    }

    int64_t d_getUniqueId() const noexcept override
    {
        return d_cconst('D', '3', 'E', 'Q');
    }

    // -------------------------------------------------------------------
    // Init

    void d_initParameter(uint32_t index, Parameter& parameter) override;
    void d_initProgramName(uint32_t index, d_string& programName) override;

    // -------------------------------------------------------------------
    // Internal data

    float d_getParameterValue(uint32_t index) const override;
    void  d_setParameterValue(uint32_t index, float value) override;
    void  d_setProgram(uint32_t index) override;

    // -------------------------------------------------------------------
    // Process

    void d_activate() override;
    void d_deactivate() override;
    void d_run(const float** inputs, float** outputs, uint32_t frames) override;

    // -------------------------------------------------------------------

private:
    float fLow, fMid, fHigh, fMaster, fLowMidFreq, fMidHighFreq;

    float lowVol, midVol, highVol, outVol;
    float freqLP, freqHP;

    float xLP, a0LP, b1LP;
    float xHP, a0HP, b1HP;

    float out1LP, out2LP, out1HP, out2HP;
    float tmp1LP, tmp2LP, tmp1HP, tmp2HP;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistrhoPlugin3BandEQ)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // DISTRHO_PLUGIN_3BANDEQ_HPP_INCLUDED
