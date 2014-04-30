/*
 * ZaMultiComp Plugin
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

#ifndef ZAMULTICOMPPLUGIN_HPP_INCLUDED
#define ZAMULTICOMPPLUGIN_HPP_INCLUDED

#define MAX_FILT 8
#define MAX_COMP 3
#define ONEOVERROOT2 0.7071068f
#define ROOT2 1.4142135f
#define DANGER 100000.f

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class ZaMultiCompPlugin : public Plugin
{
public:
    enum Parameters
    {
        paramAttack = 0,
        paramRelease,
        paramKnee,
        paramRatio,
        paramThresh,

        paramMakeup1,
        paramMakeup2,
        paramMakeup3,

        paramXover1,
        paramXover2,

	paramGainR1,
	paramGainR2,
	paramGainR3,

	paramToggle1,
	paramToggle2,
	paramToggle3,

	paramListen1,
	paramListen2,
	paramListen3,

	paramGlobalGain,
	paramOutputLevel,
        paramCount
    };

    ZaMultiCompPlugin();
    ~ZaMultiCompPlugin() override;

protected:
    // -------------------------------------------------------------------
    // Information

    const char* d_getLabel() const noexcept override
    {
        return "ZaMultiComp";
    }

    const char* d_getMaker() const noexcept override
    {
        return "Damien Zammit";
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
        return d_cconst('Z', 'M', 'C', 'P');
    }

    // -------------------------------------------------------------------
    // Init

    void d_initParameter(uint32_t index, Parameter& parameter) ;
    void d_initProgramName(uint32_t index, d_string& programName) ;

    // -------------------------------------------------------------------
    // Internal data

    float d_getParameterValue(uint32_t index) const override;
    void  d_setParameterValue(uint32_t index, float value) override;
    void  d_setProgram(uint32_t index) ;

    // -------------------------------------------------------------------
    // Process

	static inline float
	sanitize_denormal(float v) {
	        if(!std::isnormal(v) || !std::isfinite(v))
	                return 0.f;
	        return v;
	}

	static inline float
	from_dB(float gdb) {
	        return (exp(gdb/20.f*log(10.f)));
	}

	static inline float
	to_dB(float g) {
	        return (20.f*log10(g));
	}

    float run_comp(int k, float in);
    float run_filter(int i, float in);
    void set_lp_coeffs(float fc, float q, float sr, int i, float gain);
    void set_hp_coeffs(float fc, float q, float sr, int i, float gain);

    void d_activate() override;
    void d_deactivate() override;
    void d_run(float** inputs, float** outputs, uint32_t frames) override;

    // -------------------------------------------------------------------

private:
    float attack,release,knee,ratio,thresdb,makeup[MAX_COMP],globalgain;
    float gainr[MAX_COMP],toggle[MAX_COMP],xover1,xover2,outlevel,listen[MAX_COMP];
    float old_yl[MAX_COMP], old_y1[MAX_COMP];
    // Crossover filter coefficients
    float a0[MAX_FILT];
    float a1[MAX_FILT];
    float a2[MAX_FILT];
    float b1[MAX_FILT];
    float b2[MAX_FILT];

    //Crossover filter states
    float w1[MAX_FILT];
    float w2[MAX_FILT];
    float z1[MAX_FILT];
    float z2[MAX_FILT];
};


// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // ZAMULTICOMP_HPP_INCLUDED
