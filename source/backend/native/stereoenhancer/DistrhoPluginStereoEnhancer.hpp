/*
 * DISTRHO StereoEnhancer Plugin, based on StereoEnhancer by Michael Gruhn
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

#ifndef DISTRHO_PLUGIN_STEREO_ENHANCER_HPP_INCLUDED
#define DISTRHO_PLUGIN_STEREO_ENHANCER_HPP_INCLUDED

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

class DistrhoPluginStereoEnhancer : public Plugin
{
public:
    enum Parameters
    {
        paramWidthLows = 0,
        paramWidthHighs,
        paramCrossover,
        paramCount
    };

    DistrhoPluginStereoEnhancer();
    ~DistrhoPluginStereoEnhancer();

protected:
    // ---------------------------------------------
    // Information

    const char* d_label() const
    {
        return "StereoEnhancer";
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
        return d_cconst('D', 'S', 't', 'E');
    }

    // ---------------------------------------------
    // Init

    void d_initParameter(uint32_t index, Parameter& parameter);
    void d_initProgramName(uint32_t index, d_string& programName);

    // ---------------------------------------------
    // Internal data

    float d_parameterValue(uint32_t index);
    void  d_setParameterValue(uint32_t index, float value);
    void  d_setProgram(uint32_t index);

    // ---------------------------------------------
    // Process

    void d_activate();
    void d_deactivate();
    void d_run(float** inputs, float** outputs, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents);

    // ---------------------------------------------

private:
    float widthLP, widthCoeffLP;
    float freqHP, freqHPFader;
    float widthHP, widthCoeffHP;

    float xHP, a0HP, b1HP;

    float out1HP, out2HP;
    float tmp1HP, tmp2HP;

    float monoHP, stereoHP;
    float monoLP, stereoLP;
};

END_NAMESPACE_DISTRHO

#endif  // DISTRHO_PLUGIN_STEREO_ENHANCER_HPP_INCLUDED
