/*
 * DISTRHO 3BandSplitter Plugin, based on 3BandSplitter by Michael Gruhn
 * Copyright (C) 2007 Michael Gruhn <michael-gruhn@web.de>
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the license see the LGPL.txt file
 */

#ifndef __DISTRHO_PLUGIN_3BANDSPLITTER_HPP__
#define __DISTRHO_PLUGIN_3BANDSPLITTER_HPP__

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

class DistrhoPlugin3BandSplitter : public Plugin
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

    DistrhoPlugin3BandSplitter();
    ~DistrhoPlugin3BandSplitter();

    // ---------------------------------------------

protected:
    // Information
    const char* d_label() const
    {
        return "3BandSplitter";
    }

    const char* d_maker() const
    {
        return "DISTRHO";
    }

    const char* d_license() const
    {
        return "LGPL";
    }

    uint32_t d_version() const
    {
        return 0x1000;
    }

    long d_uniqueId() const
    {
        return d_cconst('D', '3', 'E', 'S');
    }

    // Init
    void d_initParameter(uint32_t index, Parameter& parameter);
    void d_initProgramName(uint32_t index, d_string& programName);

    // Internal data
    float d_parameterValue(uint32_t index);
    void  d_setParameterValue(uint32_t index, float value);
    void  d_setProgram(uint32_t index);

    // Process
    void d_activate();
    void d_deactivate();
    void d_run(float** inputs, float** outputs, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents);

    // ---------------------------------------------

private:
    float fLow, fMid, fHigh, fMaster, fLowMidFreq, fMidHighFreq;

    float lowVol, midVol, highVol, outVol;
    float freqLP, freqHP;

    float xLP, a0LP, b1LP;
    float xHP, a0HP, b1HP;

    float out1LP, out2LP, out1HP, out2HP;
    float tmp1LP, tmp2LP, tmp1HP, tmp2HP;
};

END_NAMESPACE_DISTRHO

#endif  // __DISTRHO_PLUGIN_3BANDSPLITTER_HPP__
