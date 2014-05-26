/*
 * ZamTube triode WDF distortion model
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

#ifndef ZAMTUBEPLUGIN_HPP_INCLUDED
#define ZAMTUBEPLUGIN_HPP_INCLUDED

#include "DistrhoPlugin.hpp"
#include "wdf.h"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

template <int N> inline float faustpower(float x) 		{ return powf(x,N); }
template <int N> inline double faustpower(double x) 	{ return pow(x,N); }
template <int N> inline int faustpower(int x) 			{ return faustpower<N/2>(x) * faustpower<N-N/2>(x); }
template <> 	 inline int faustpower<0>(int)	 		{ return 1; }
template <> 	 inline int faustpower<1>(int x) 		{ return x; }

#define FAUSTFLOAT float
typedef long double quad;

#define TOLERANCE 1e-6
#define DANGER 1000.f

class ZamTubePlugin : public Plugin
{
public:
	Triode v;
	V Vi;
	C Ci;
	C Ck;
	C Co;
	R Ro;
	R Rg;
	R Ri;
	R Rk;
	V E;

	T e;

	//Official
	//->Gate
	ser S0;
	inv I0;
	par P0;
	ser S1;
	inv I1;

	//->Cathode
	par I3;

	//->Plate
	ser S2;
	inv I4;
	par P2;

        float   fConst0;
        float   fConst1;
        float   fConst2;
        float   fConst3;
        float   fRec0[4];
        float   fRec1[4];
        float   fRec2[4];
        float   fRec3[4];
        float   fRec4[4];
        float   fRec5[4];
        float   fRec6[4];
        float   fRec7[4];
        float   fRec8[4];
        float   fRec9[4];
        float   fRec10[4];
        float   fRec11[4];
        float   fRec12[4];
        float   fRec13[4];
        float   fConst4;
        float   fConst5;
        float   fRec14[4];
        float   fConst6;
        float   fRec15[4];
        float   fRec16[4];
        float   fRec17[4];
        float   fRec18[4];
        float   fRec19[4];
        float   fRec20[4];
        float   fRec21[4];
        float   fRec22[4];
        float   fRec23[4];
        float   fRec24[4];

	float fSamplingFreq;
    enum Parameters
    {
        paramTubedrive = 0,
        paramBass,
        paramMiddle,
        paramTreble,
        paramToneStack,
        paramGain,
        paramCount
    };

    ZamTubePlugin();

protected:
    // -------------------------------------------------------------------
    // Information

    const char* d_getLabel() const noexcept override
    {
        return "ZamTube";
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
        return d_cconst('Z', 'T', 'U', 'B');
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

    void d_activate() override;
    void d_run(const float** inputs, float** outputs, uint32_t frames) override;

    // -------------------------------------------------------------------

private:
    float tubedrive,bass,middle,treble,tonestack,mastergain; //parameters
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // ZAMTUBE_HPP_INCLUDED
