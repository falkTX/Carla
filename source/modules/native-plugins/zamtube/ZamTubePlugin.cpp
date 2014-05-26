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
 *
 * Tone Stacks from David Yeh's faust implementation.
 *
 */

#include "ZamTubePlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

ZamTubePlugin::ZamTubePlugin()
    : Plugin(paramCount, 1, 0), // 1 program, 0 states
		Vi(0.0,10000.0), Ci(0.0000001,48000.0), Ck(0.00001,48000.0), Co(0.00000001,48000.0), Ro(1000000.0), Rg(20000.0), 
		Ri(1000000.0), Rk(1000.0), E(250.0,100000.0), 
		S0(&Ci,&Vi), I0(&S0), P0(&I0,&Ri), S1(&Rg,&P0), 
		I1(&S1), I3(&Ck,&Rk), S2(&Co,&Ro), I4(&S2), P2(&I4,&E) 
{
    // set default values
    d_setProgram(0);
}

// -----------------------------------------------------------------------
// Init

void ZamTubePlugin::d_initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
    case paramTubedrive:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Tube Drive";
        parameter.symbol     = "tubedrive";
        parameter.unit       = " ";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -30.0f;
        parameter.ranges.max = 30.0f;
        break;
    case paramBass:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Bass";
        parameter.symbol     = "bass";
        parameter.unit       = " ";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramMiddle:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Mids";
        parameter.symbol     = "mids";
        parameter.unit       = " ";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramTreble:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Treble";
        parameter.symbol     = "treb";
        parameter.unit       = " ";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramToneStack:
        parameter.hints      = PARAMETER_IS_AUTOMABLE | PARAMETER_IS_INTEGER;
        parameter.name       = "Tone Stack Model";
        parameter.symbol     = "tonestack";
        parameter.unit       = " ";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 24.0f;
        break;
    case paramGain:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Output level";
        parameter.symbol     = "gain";
        parameter.unit       = " ";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -15.0f;
        parameter.ranges.max = 15.0f;
        break;
    }
}

void ZamTubePlugin::d_initProgramName(uint32_t index, d_string& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

// -----------------------------------------------------------------------
// Internal data

float ZamTubePlugin::d_getParameterValue(uint32_t index) const
{
    switch (index)
    {
    case paramTubedrive:
        return tubedrive;
        break;
    case paramBass:
        return bass;
        break;
    case paramMiddle:
        return middle;
        break;
    case paramTreble:
        return treble;
        break;
    case paramToneStack:
        return tonestack;
        break;
    case paramGain:
        return mastergain;
        break;
    default:
        return 0.0f;
    }
}

void ZamTubePlugin::d_setParameterValue(uint32_t index, float value)
{
    switch (index)
    {
    case paramTubedrive:
        tubedrive = value;
        break;
    case paramBass:
        bass = value;
        break;
    case paramMiddle:
        middle = value;
        break;
    case paramTreble:
        treble = value;
        break;
    case paramToneStack:
        tonestack = value;
        break;
    case paramGain:
        mastergain = value;
        break;
    }
}

void ZamTubePlugin::d_setProgram(uint32_t index)
{
    if (index != 0)
        return;

    /* Default parameter values */
    tubedrive = 0.0f;
    bass = 0.5f;
    middle = 0.5f;
    treble = 0.0f;
    tonestack = 0.0f;
    mastergain = 0.0f;

    /* Default variable values */

    /* reset filter values */
    d_activate();
}

// -----------------------------------------------------------------------
// Process

void ZamTubePlugin::d_activate()
{

	T Fs = d_getSampleRate();
	
	// Passive components
	T ci = 0.0000001;	//100nF
	T ck = 0.00001;		//10uF
	T co = 0.00000001;	//10nF
	T ro = 1000000.0;	//1Mohm
	T rp = 100000.0;	//100kohm
	T rg = 20000.0;		//20kohm
	T ri = 1000000.0;	//1Mohm
	T rk = 1000.0;		//1kohm
	e = 250.0;		//250V

	Vi = V(0.0,10000.0);	//1kohm internal resistance
	Ci = C(ci, Fs);
	Ck = C(ck, Fs);
	Co = C(co, Fs);
	Ro = R(ro);
	Rg = R(rg);
	Ri = R(ri);
	Rk = R(rk);
	E = V(e, rp);

	//Official
	//->Gate
	S0 = ser(&Ci, &Vi);
	I0 = inv(&S0);
	P0 = par(&I0, &Ri);
	S1 = ser(&Rg, &P0);
	I1 = inv(&S1);

	//->Cathode
	I3 = par(&Ck, &Rk);

	//->Plate
	S2 = ser(&Co, &Ro);
	I4 = inv(&S2);
	P2 = par(&I4, &E);

	// 12AX7 triode model RSD-1
	v.g1 = 2.242e-3;
	v.mu1 = 103.2;
	v.gamma1 = 1.26;
	v.c1 = 3.4;
	v.gg1 = 6.177e-4;
	v.e1 = 1.314;
	v.cg1 = 9.901;
	v.ig01 = 8.025e-8;

	// 12AX7 triode model EHX-1
	v.g2 = 1.371e-3;
	v.mu2 = 86.9;
	v.gamma2 = 1.349;
	v.c2 = 4.56;
	v.gg2 = 3.263e-4;
	v.e2 = 1.156;
	v.cg2 = 11.99;
	v.ig02 = 3.917e-8;

        fSamplingFreq = Fs;
        fConst0 = float(min(192000, max(1, fSamplingFreq)));
        fConst1 = (2 * fConst0);
        fConst2 = faustpower<2>(fConst1);
        fConst3 = (3 * fConst1);
        for (int i=0; i<4; i++) fRec0[i] = 0;
        for (int i=0; i<4; i++) fRec1[i] = 0;
        for (int i=0; i<4; i++) fRec2[i] = 0;
        for (int i=0; i<4; i++) fRec3[i] = 0;
        for (int i=0; i<4; i++) fRec4[i] = 0;
        for (int i=0; i<4; i++) fRec5[i] = 0;
        for (int i=0; i<4; i++) fRec6[i] = 0;
        for (int i=0; i<4; i++) fRec7[i] = 0;
        for (int i=0; i<4; i++) fRec8[i] = 0;
        for (int i=0; i<4; i++) fRec9[i] = 0;
        for (int i=0; i<4; i++) fRec10[i] = 0;
        for (int i=0; i<4; i++) fRec11[i] = 0;
        for (int i=0; i<4; i++) fRec12[i] = 0;
        for (int i=0; i<4; i++) fRec13[i] = 0;
        fConst4 = (1.0691560000000003e-08f * fConst1);
        fConst5 = (3.2074680000000005e-08f * fConst1);
        for (int i=0; i<4; i++) fRec14[i] = 0;
        fConst6 = (0.044206800000000004f * fConst0);
        for (int i=0; i<4; i++) fRec15[i] = 0;
        for (int i=0; i<4; i++) fRec16[i] = 0;
        for (int i=0; i<4; i++) fRec17[i] = 0;
        for (int i=0; i<4; i++) fRec18[i] = 0;
        for (int i=0; i<4; i++) fRec19[i] = 0;
        for (int i=0; i<4; i++) fRec20[i] = 0;
        for (int i=0; i<4; i++) fRec21[i] = 0;
        for (int i=0; i<4; i++) fRec22[i] = 0;
        for (int i=0; i<4; i++) fRec23[i] = 0;
        for (int i=0; i<4; i++) fRec24[i] = 0;
}

void ZamTubePlugin::d_run(const float** inputs, float** outputs, uint32_t frames)
{
	T tubetone = 0.f;
	
	//12AX7 triode tube mod
	v.g = v.g1 + (v.g2-v.g1)*tubetone;
	v.mu = v.mu1 + (v.mu2-v.mu1)*tubetone;
	v.gamma = v.gamma1 + (v.gamma2-v.gamma1)*tubetone;
	v.c = v.c1 + (v.c2-v.c1)*tubetone;
	v.gg = v.gg1 + (v.gg2-v.gg1)*tubetone;
	v.e = v.e1 + (v.e2-v.e1)*tubetone;
	v.cg = v.cg1 + (v.cg2-v.cg1)*tubetone;
	v.ig0 = v.ig01 + (v.ig02-v.ig01)*tubetone;

	float 	fSlow0 = middle;
	float 	fSlow1 = (1.3784375e-06f * fSlow0);
	float 	fSlow2 = expf((3.4f * (bass - 1)));
	float 	fSlow3 = (8.396625e-06f + ((7.405375e-05f * fSlow2) + (fSlow0 * (((1.3784375000000003e-05f * fSlow2) - 5.7371875e-06f) - fSlow1))));
	float 	fSlow4 = ((1.3062500000000001e-09f * fSlow2) - (1.30625e-10f * fSlow0));
	float 	fSlow5 = (4.468750000000001e-09f * fSlow2);
	float 	fSlow6 = (4.46875e-10f + (fSlow5 + (fSlow0 * (fSlow4 - 3.1625e-10f))));
	float 	fSlow7 = (fConst1 * fSlow6);
	float 	fSlow8 = (0.00055f * fSlow0);
	float 	fSlow9 = (0.0250625f * fSlow2);
	float 	fSlow10 = (fConst1 * (0.01842875f + (fSlow9 + fSlow8)));
	float 	fSlow11 = ((fSlow10 + (fConst2 * (fSlow7 - fSlow3))) - 1);
	float 	fSlow12 = (fConst3 * fSlow6);
	float 	fSlow13 = ((fConst2 * (fSlow3 + fSlow12)) - (3 + fSlow10));
	float 	fSlow14 = ((fSlow10 + (fConst2 * (fSlow3 - fSlow12))) - 3);
	float 	fSlow15 = (0 - (1 + (fSlow10 + (fConst2 * (fSlow3 + fSlow7)))));
	float 	fSlow16 = (1.0f / fSlow15);
	float 	fSlow17 = treble;
	float 	fSlow18 = ((fSlow0 * (1.30625e-10f + fSlow4)) + (fSlow17 * ((4.46875e-10f - (4.46875e-10f * fSlow0)) + fSlow5)));
	float 	fSlow19 = (fConst3 * fSlow18);
	float 	fSlow20 = (2.55375e-07f + (((9.912500000000003e-07f * fSlow17) + (fSlow0 * (1.4128125e-06f - fSlow1))) + (fSlow2 * (2.5537500000000007e-06f + (1.3784375000000003e-05f * fSlow0)))));
	float 	fSlow21 = (6.25e-05f * fSlow17);
	float 	fSlow22 = (fSlow8 + fSlow21);
	float 	fSlow23 = (0.0025062500000000002f + (fSlow9 + fSlow22));
	float 	fSlow24 = (fConst1 * fSlow23);
	float 	fSlow25 = (fSlow24 + (fConst2 * (fSlow20 - fSlow19)));
	float 	fSlow26 = (fConst1 * fSlow18);
	float 	fSlow27 = (fSlow24 + (fConst2 * (fSlow26 - fSlow20)));
	float 	fSlow28 = (fConst1 * (0 - fSlow23));
	float 	fSlow29 = (fSlow28 + (fConst2 * (fSlow20 + fSlow19)));
	float 	fSlow30 = (fSlow28 - (fConst2 * (fSlow20 + fSlow26)));
	float 	fSlow31 = tonestack;
	float 	fSlow32 = ((fSlow31 == 23) / fSlow15);
	float 	fSlow33 = (1.0607618000000002e-05f * fSlow0);
	float 	fSlow34 = (3.1187760000000004e-05f + ((0.00032604000000000004f * fSlow2) + (fSlow0 * (((0.00011284700000000001f * fSlow2) - 1.9801382e-05f) - fSlow33))));
	float 	fSlow35 = ((3.5814000000000013e-09f * fSlow2) - (3.3665160000000007e-10f * fSlow0));
	float 	fSlow36 = (8.100000000000003e-09f * fSlow2);
	float 	fSlow37 = (7.614000000000002e-10f + (fSlow36 + (fSlow0 * (fSlow35 - 4.247484000000001e-10f))));
	float 	fSlow38 = (fConst1 * fSlow37);
	float 	fSlow39 = (0.00188f * fSlow0);
	float 	fSlow40 = (0.060025f * fSlow2);
	float 	fSlow41 = (fConst1 * (0.027267350000000003f + (fSlow40 + fSlow39)));
	float 	fSlow42 = ((fSlow41 + (fConst2 * (fSlow38 - fSlow34))) - 1);
	float 	fSlow43 = (fConst3 * fSlow37);
	float 	fSlow44 = ((fConst2 * (fSlow34 + fSlow43)) - (3 + fSlow41));
	float 	fSlow45 = ((fSlow41 + (fConst2 * (fSlow34 - fSlow43))) - 3);
	float 	fSlow46 = (0 - (1 + (fSlow41 + (fConst2 * (fSlow34 + fSlow38)))));
	float 	fSlow47 = (1.0f / fSlow46);
	float 	fSlow48 = ((fSlow0 * (3.3665160000000007e-10f + fSlow35)) + (fSlow17 * ((7.614000000000002e-10f - (7.614000000000002e-10f * fSlow0)) + fSlow36)));
	float 	fSlow49 = (fConst3 * fSlow48);
	float 	fSlow50 = (1.9176000000000002e-07f + (((5.400000000000001e-07f * fSlow17) + (fSlow0 * (1.0654618000000002e-05f - fSlow33))) + (fSlow2 * (2.0400000000000004e-06f + (0.00011284700000000001f * fSlow0)))));
	float 	fSlow51 = (2.5e-05f * fSlow17);
	float 	fSlow52 = (0.005642350000000001f + (fSlow40 + (fSlow39 + fSlow51)));
	float 	fSlow53 = (fConst1 * fSlow52);
	float 	fSlow54 = (fSlow53 + (fConst2 * (fSlow50 - fSlow49)));
	float 	fSlow55 = (fConst1 * fSlow48);
	float 	fSlow56 = (fSlow53 + (fConst2 * (fSlow55 - fSlow50)));
	float 	fSlow57 = (fConst1 * (0 - fSlow52));
	float 	fSlow58 = (fSlow57 + (fConst2 * (fSlow50 + fSlow49)));
	float 	fSlow59 = (fSlow57 - (fConst2 * (fSlow50 + fSlow55)));
	float 	fSlow60 = ((fSlow31 == 24) / fSlow46);
	float 	fSlow61 = (4.7117500000000004e-07f * fSlow0);
	float 	fSlow62 = (0.00011998125000000002f * fSlow2);
	float 	fSlow63 = (5.718000000000001e-06f + (fSlow62 + (fSlow0 * (((1.1779375000000001e-05f * fSlow2) - 4.199450000000001e-06f) - fSlow61))));
	float 	fSlow64 = ((1.0281250000000001e-09f * fSlow2) - (4.1125e-11f * fSlow0));
	float 	fSlow65 = (7.343750000000001e-09f * fSlow2);
	float 	fSlow66 = (2.9375e-10f + (fSlow65 + (fSlow0 * (fSlow64 - 2.52625e-10f))));
	float 	fSlow67 = (fConst1 * fSlow66);
	float 	fSlow68 = (0.00047000000000000004f * fSlow0);
	float 	fSlow69 = (fConst1 * (0.015765f + (fSlow9 + fSlow68)));
	float 	fSlow70 = ((fSlow69 + (fConst2 * (fSlow67 - fSlow63))) - 1);
	float 	fSlow71 = (fConst3 * fSlow66);
	float 	fSlow72 = ((fConst2 * (fSlow63 + fSlow71)) - (3 + fSlow69));
	float 	fSlow73 = ((fSlow69 + (fConst2 * (fSlow63 - fSlow71))) - 3);
	float 	fSlow74 = (0 - (1 + (fSlow69 + (fConst2 * (fSlow63 + fSlow67)))));
	float 	fSlow75 = (1.0f / fSlow74);
	float 	fSlow76 = ((fSlow0 * (4.1125e-11f + fSlow64)) + (fSlow17 * ((2.9375e-10f - (2.9375e-10f * fSlow0)) + fSlow65)));
	float 	fSlow77 = (fConst3 * fSlow76);
	float 	fSlow78 = (9.187500000000001e-07f * fSlow17);
	float 	fSlow79 = (9.925e-08f + ((fSlow78 + (fSlow0 * (5.0055e-07f - fSlow61))) + (fSlow2 * (2.48125e-06f + (1.1779375000000001e-05f * fSlow0)))));
	float 	fSlow80 = (0.0010025f + (fSlow9 + (fSlow21 + fSlow68)));
	float 	fSlow81 = (fConst1 * fSlow80);
	float 	fSlow82 = (fSlow81 + (fConst2 * (fSlow79 - fSlow77)));
	float 	fSlow83 = (fConst1 * fSlow76);
	float 	fSlow84 = (fSlow81 + (fConst2 * (fSlow83 - fSlow79)));
	float 	fSlow85 = (fConst1 * (0 - fSlow80));
	float 	fSlow86 = (fSlow85 + (fConst2 * (fSlow79 + fSlow77)));
	float 	fSlow87 = (fSlow85 - (fConst2 * (fSlow79 + fSlow83)));
	float 	fSlow88 = ((fSlow31 == 22) / fSlow74);
	float 	fSlow89 = (3.059375000000001e-07f * fSlow0);
	float 	fSlow90 = (1.5468750000000003e-06f + ((1.2718750000000003e-05f * fSlow2) + (fSlow0 * (((3.0593750000000007e-06f * fSlow2) - 8.696875000000003e-07f) - fSlow89))));
	float 	fSlow91 = ((2.646875e-10f * fSlow2) - (2.6468750000000002e-11f * fSlow0));
	float 	fSlow92 = (7.5625e-10f * fSlow2);
	float 	fSlow93 = (7.562500000000001e-11f + (fSlow92 + (fSlow0 * (fSlow91 - 4.915625000000001e-11f))));
	float 	fSlow94 = (fConst1 * fSlow93);
	float 	fSlow95 = (0.005562500000000001f * fSlow2);
	float 	fSlow96 = (fConst1 * (0.005018750000000001f + (fSlow8 + fSlow95)));
	float 	fSlow97 = ((fSlow96 + (fConst2 * (fSlow94 - fSlow90))) - 1);
	float 	fSlow98 = (fConst3 * fSlow93);
	float 	fSlow99 = ((fConst2 * (fSlow90 + fSlow98)) - (3 + fSlow96));
	float 	fSlow100 = ((fSlow96 + (fConst2 * (fSlow90 - fSlow98))) - 3);
	float 	fSlow101 = (0 - (1 + (fSlow96 + (fConst2 * (fSlow90 + fSlow94)))));
	float 	fSlow102 = (1.0f / fSlow101);
	float 	fSlow103 = ((fSlow0 * (2.6468750000000002e-11f + fSlow91)) + (fSlow17 * ((7.562500000000001e-11f - (7.562500000000001e-11f * fSlow0)) + fSlow92)));
	float 	fSlow104 = (fConst3 * fSlow103);
	float 	fSlow105 = (6.1875e-08f + (((2.75e-07f * fSlow17) + (fSlow0 * (3.403125000000001e-07f - fSlow89))) + (fSlow2 * (6.1875e-07f + (3.0593750000000007e-06f * fSlow0)))));
	float 	fSlow106 = (0.00055625f + (fSlow22 + fSlow95));
	float 	fSlow107 = (fConst1 * fSlow106);
	float 	fSlow108 = (fSlow107 + (fConst2 * (fSlow105 - fSlow104)));
	float 	fSlow109 = (fConst1 * fSlow103);
	float 	fSlow110 = (fSlow107 + (fConst2 * (fSlow109 - fSlow105)));
	float 	fSlow111 = (fConst1 * (0 - fSlow106));
	float 	fSlow112 = (fSlow111 + (fConst2 * (fSlow105 + fSlow104)));
	float 	fSlow113 = (fSlow111 - (fConst2 * (fSlow105 + fSlow109)));
	float 	fSlow114 = ((fSlow31 == 21) / fSlow101);
	float 	fSlow115 = (2.2193400000000003e-07f * fSlow0);
	float 	fSlow116 = (2.7073879999999998e-06f + ((4.9553415999999996e-05f * fSlow2) + (fSlow0 * (((4.882548000000001e-06f * fSlow2) - 1.964318e-06f) - fSlow115))));
	float 	fSlow117 = ((3.4212992000000004e-10f * fSlow2) - (1.5551360000000004e-11f * fSlow0));
	float 	fSlow118 = (2.3521432000000003e-09f * fSlow2);
	float 	fSlow119 = (1.0691560000000001e-10f + (fSlow118 + (fSlow0 * (fSlow117 - 9.136424e-11f))));
	float 	fSlow120 = (fConst1 * fSlow119);
	float 	fSlow121 = (0.0103884f * fSlow2);
	float 	fSlow122 = (fConst1 * (0.009920600000000002f + (fSlow68 + fSlow121)));
	float 	fSlow123 = ((fSlow122 + (fConst2 * (fSlow120 - fSlow116))) - 1);
	float 	fSlow124 = (fConst3 * fSlow119);
	float 	fSlow125 = ((fConst2 * (fSlow116 + fSlow124)) - (3 + fSlow122));
	float 	fSlow126 = ((fSlow122 + (fConst2 * (fSlow116 - fSlow124))) - 3);
	float 	fSlow127 = (0 - (1 + (fSlow122 + (fConst2 * (fSlow116 + fSlow120)))));
	float 	fSlow128 = (1.0f / fSlow127);
	float 	fSlow129 = ((fSlow0 * (1.5551360000000004e-11f + fSlow117)) + (fSlow17 * ((1.0691560000000001e-10f - (1.0691560000000001e-10f * fSlow0)) + fSlow118)));
	float 	fSlow130 = (fConst3 * fSlow129);
	float 	fSlow131 = (4.3428e-08f + (((4.5496e-07f * fSlow17) + (fSlow0 * (2.4468200000000005e-07f - fSlow115))) + (fSlow2 * (9.55416e-07f + (4.882548000000001e-06f * fSlow0)))));
	float 	fSlow132 = (0.00047220000000000004f + (fSlow121 + (fSlow68 + (4.84e-05f * fSlow17))));
	float 	fSlow133 = (fConst1 * fSlow132);
	float 	fSlow134 = (fSlow133 + (fConst2 * (fSlow131 - fSlow130)));
	float 	fSlow135 = (fConst1 * fSlow129);
	float 	fSlow136 = (fSlow133 + (fConst2 * (fSlow135 - fSlow131)));
	float 	fSlow137 = (fConst1 * (0 - fSlow132));
	float 	fSlow138 = (fSlow137 + (fConst2 * (fSlow131 + fSlow130)));
	float 	fSlow139 = (fSlow137 - (fConst2 * (fSlow131 + fSlow135)));
	float 	fSlow140 = ((fSlow31 == 20) / fSlow127);
	float 	fSlow141 = (2.3926056000000006e-07f * fSlow0);
	float 	fSlow142 = (1.0875480000000001e-05f * fSlow2);
	float 	fSlow143 = (1.1144196800000003e-06f + ((3.659304000000001e-05f * fSlow2) + (fSlow0 * ((fSlow142 - 4.347578400000001e-07f) - fSlow141))));
	float 	fSlow144 = ((1.4413132800000006e-09f * fSlow2) - (3.1708892160000014e-11f * fSlow0));
	float 	fSlow145 = (3.403100800000001e-09f * fSlow2);
	float 	fSlow146 = (7.486821760000003e-11f + (fSlow145 + (fSlow0 * (fSlow144 - 4.315932544000001e-11f))));
	float 	fSlow147 = (fConst1 * fSlow146);
	float 	fSlow148 = (0.00048400000000000006f * fSlow0);
	float 	fSlow149 = (0.022470000000000004f * fSlow2);
	float 	fSlow150 = (fSlow149 + fSlow148);
	float 	fSlow151 = (fConst1 * (0.00358974f + fSlow150));
	float 	fSlow152 = ((fSlow151 + (fConst2 * (fSlow147 - fSlow143))) - 1);
	float 	fSlow153 = (fConst3 * fSlow146);
	float 	fSlow154 = ((fConst2 * (fSlow143 + fSlow153)) - (3 + fSlow151));
	float 	fSlow155 = ((fSlow151 + (fConst2 * (fSlow143 - fSlow153))) - 3);
	float 	fSlow156 = (0 - (1 + (fSlow151 + (fConst2 * (fSlow143 + fSlow147)))));
	float 	fSlow157 = (1.0f / fSlow156);
	float 	fSlow158 = ((fSlow0 * (3.1708892160000014e-11f + fSlow144)) + (fSlow17 * ((7.486821760000003e-11f - (7.486821760000003e-11f * fSlow0)) + fSlow145)));
	float 	fSlow159 = (fConst3 * fSlow158);
	float 	fSlow160 = (1.0875480000000001e-05f * fSlow0);
	float 	fSlow161 = (fSlow0 * (2.893061600000001e-07f - fSlow141));
	float 	fSlow162 = (8.098288000000002e-08f + (((3.0937280000000007e-07f * fSlow17) + fSlow161) + (fSlow2 * (3.6810400000000007e-06f + fSlow160))));
	float 	fSlow163 = (0.00049434f + (fSlow149 + (fSlow148 + (0.0001034f * fSlow17))));
	float 	fSlow164 = (fConst1 * fSlow163);
	float 	fSlow165 = (fSlow164 + (fConst2 * (fSlow162 - fSlow159)));
	float 	fSlow166 = (fConst1 * fSlow158);
	float 	fSlow167 = (fSlow164 + (fConst2 * (fSlow166 - fSlow162)));
	float 	fSlow168 = (fConst1 * (0 - fSlow163));
	float 	fSlow169 = (fSlow168 + (fConst2 * (fSlow162 + fSlow159)));
	float 	fSlow170 = (fSlow168 - (fConst2 * (fSlow162 + fSlow166)));
	float 	fSlow171 = ((fSlow31 == 19) / fSlow156);
	float 	fSlow172 = (7.790052600000002e-07f * fSlow0);
	float 	fSlow173 = (1.4106061200000003e-06f + ((3.7475640000000014e-05f * fSlow2) + (fSlow0 * (((2.3606220000000006e-05f * fSlow2) - 3.2220474e-07f) - fSlow172))));
	float 	fSlow174 = ((1.5406083e-09f * fSlow2) - (5.08400739e-11f * fSlow0));
	float 	fSlow175 = (1.9775250000000004e-09f * fSlow2);
	float 	fSlow176 = (6.5258325e-11f + (fSlow175 + (fSlow0 * (fSlow174 - 1.4418251099999996e-11f))));
	float 	fSlow177 = (fConst1 * fSlow176);
	float 	fSlow178 = (0.001551f * fSlow0);
	float 	fSlow179 = (0.015220000000000001f * fSlow2);
	float 	fSlow180 = (fConst1 * (0.0037192600000000003f + (fSlow179 + fSlow178)));
	float 	fSlow181 = ((fSlow180 + (fConst2 * (fSlow177 - fSlow173))) - 1);
	float 	fSlow182 = (fConst3 * fSlow176);
	float 	fSlow183 = ((fConst2 * (fSlow173 + fSlow182)) - (3 + fSlow180));
	float 	fSlow184 = ((fSlow180 + (fConst2 * (fSlow173 - fSlow182))) - 3);
	float 	fSlow185 = (0 - (1 + (fSlow180 + (fConst2 * (fSlow173 + fSlow177)))));
	float 	fSlow186 = (1.0f / fSlow185);
	float 	fSlow187 = ((fSlow0 * (5.08400739e-11f + fSlow174)) + (fSlow17 * ((6.5258325e-11f - (6.5258325e-11f * fSlow0)) + fSlow175)));
	float 	fSlow188 = (fConst3 * fSlow187);
	float 	fSlow189 = (5.018112e-08f + (((1.7391e-07f * fSlow17) + (fSlow0 * (8.643102600000002e-07f - fSlow172))) + (fSlow2 * (1.5206400000000001e-06f + (2.3606220000000006e-05f * fSlow0)))));
	float 	fSlow190 = (0.0005022600000000001f + (fSlow179 + (fSlow178 + (5.4999999999999995e-05f * fSlow17))));
	float 	fSlow191 = (fConst1 * fSlow190);
	float 	fSlow192 = (fSlow191 + (fConst2 * (fSlow189 - fSlow188)));
	float 	fSlow193 = (fConst1 * fSlow187);
	float 	fSlow194 = (fSlow191 + (fConst2 * (fSlow193 - fSlow189)));
	float 	fSlow195 = (fConst1 * (0 - fSlow190));
	float 	fSlow196 = (fSlow195 + (fConst2 * (fSlow189 + fSlow188)));
	float 	fSlow197 = (fSlow195 - (fConst2 * (fSlow189 + fSlow193)));
	float 	fSlow198 = ((fSlow31 == 18) / fSlow185);
	float 	fSlow199 = (4.7047000000000006e-07f * fSlow0);
	float 	fSlow200 = (5.107200000000001e-06f + ((0.00011849250000000002f * fSlow2) + (fSlow0 * (((1.1761750000000001e-05f * fSlow2) - 4.217780000000001e-06f) - fSlow199))));
	float 	fSlow201 = ((4.1125e-10f * fSlow2) - (1.645e-11f * fSlow0));
	float 	fSlow202 = (2.9375000000000002e-09f * fSlow2);
	float 	fSlow203 = (1.175e-10f + (fSlow202 + (fSlow0 * (fSlow201 - 1.0105e-10f))));
	float 	fSlow204 = (fConst1 * fSlow203);
	float 	fSlow205 = (0.025025000000000002f * fSlow2);
	float 	fSlow206 = (fConst1 * (0.015726f + (fSlow68 + fSlow205)));
	float 	fSlow207 = ((fSlow206 + (fConst2 * (fSlow204 - fSlow200))) - 1);
	float 	fSlow208 = (fConst3 * fSlow203);
	float 	fSlow209 = ((fConst2 * (fSlow200 + fSlow208)) - (3 + fSlow206));
	float 	fSlow210 = ((fSlow206 + (fConst2 * (fSlow200 - fSlow208))) - 3);
	float 	fSlow211 = (0 - (1 + (fSlow206 + (fConst2 * (fSlow200 + fSlow204)))));
	float 	fSlow212 = (1.0f / fSlow211);
	float 	fSlow213 = ((fSlow0 * (1.645e-11f + fSlow201)) + (fSlow17 * ((1.175e-10f - (1.175e-10f * fSlow0)) + fSlow202)));
	float 	fSlow214 = (fConst3 * fSlow213);
	float 	fSlow215 = (3.9700000000000005e-08f + (((3.675000000000001e-07f * fSlow17) + (fSlow0 * (4.8222e-07f - fSlow199))) + (fSlow2 * (9.925e-07f + (1.1761750000000001e-05f * fSlow0)))));
	float 	fSlow216 = (0.001001f + (fSlow205 + (fSlow51 + fSlow68)));
	float 	fSlow217 = (fConst1 * fSlow216);
	float 	fSlow218 = (fSlow217 + (fConst2 * (fSlow215 - fSlow214)));
	float 	fSlow219 = (fConst1 * fSlow213);
	float 	fSlow220 = (fSlow217 + (fConst2 * (fSlow219 - fSlow215)));
	float 	fSlow221 = (fConst1 * (0 - fSlow216));
	float 	fSlow222 = (fSlow221 + (fConst2 * (fSlow215 + fSlow214)));
	float 	fSlow223 = (fSlow221 - (fConst2 * (fSlow215 + fSlow219)));
	float 	fSlow224 = ((fSlow31 == 17) / fSlow211);
	float 	fSlow225 = (3.0896250000000005e-07f * fSlow0);
	float 	fSlow226 = (6.338090000000001e-07f + ((1.8734760000000003e-05f * fSlow2) + (fSlow0 * (((1.2358500000000002e-05f * fSlow2) - 1.361249999999999e-08f) - fSlow225))));
	float 	fSlow227 = ((1.6037340000000005e-09f * fSlow2) - (4.0093350000000015e-11f * fSlow0));
	float 	fSlow228 = (1.8198400000000004e-09f * fSlow2);
	float 	fSlow229 = (4.5496000000000015e-11f + (fSlow228 + (fSlow0 * (fSlow227 - 5.40265e-12f))));
	float 	fSlow230 = (fConst1 * fSlow229);
	float 	fSlow231 = (fConst1 * (0.00208725f + (fSlow8 + fSlow149)));
	float 	fSlow232 = ((fSlow231 + (fConst2 * (fSlow230 - fSlow226))) - 1);
	float 	fSlow233 = (fConst3 * fSlow229);
	float 	fSlow234 = ((fConst2 * (fSlow226 + fSlow233)) - (3 + fSlow231));
	float 	fSlow235 = ((fSlow231 + (fConst2 * (fSlow226 - fSlow233))) - 3);
	float 	fSlow236 = (0 - (1 + (fSlow231 + (fConst2 * (fSlow226 + fSlow230)))));
	float 	fSlow237 = (1.0f / fSlow236);
	float 	fSlow238 = ((fSlow0 * (4.0093350000000015e-11f + fSlow227)) + (fSlow17 * ((4.5496000000000015e-11f - (4.5496000000000015e-11f * fSlow0)) + fSlow228)));
	float 	fSlow239 = (fConst3 * fSlow238);
	float 	fSlow240 = (8.1169e-08f + (((1.6544000000000003e-07f * fSlow17) + (fSlow0 * (3.735875000000001e-07f - fSlow225))) + (fSlow2 * (3.24676e-06f + (1.2358500000000002e-05f * fSlow0)))));
	float 	fSlow241 = (0.00011750000000000001f * fSlow17);
	float 	fSlow242 = (0.0005617500000000001f + (fSlow149 + (fSlow8 + fSlow241)));
	float 	fSlow243 = (fConst1 * fSlow242);
	float 	fSlow244 = (fSlow243 + (fConst2 * (fSlow240 - fSlow239)));
	float 	fSlow245 = (fConst1 * fSlow238);
	float 	fSlow246 = (fSlow243 + (fConst2 * (fSlow245 - fSlow240)));
	float 	fSlow247 = (fConst1 * (0 - fSlow242));
	float 	fSlow248 = (fSlow247 + (fConst2 * (fSlow240 + fSlow239)));
	float 	fSlow249 = (fSlow247 - (fConst2 * (fSlow240 + fSlow245)));
	float 	fSlow250 = ((fSlow31 == 16) / fSlow236);
	float 	fSlow251 = (2.7256800000000006e-07f * fSlow0);
	float 	fSlow252 = (1.4234760000000002e-06f + ((2.851440000000001e-05f * fSlow2) + (fSlow0 * (((6.8142000000000025e-06f * fSlow2) - 7.876920000000001e-07f) - fSlow251))));
	float 	fSlow253 = ((4.724676000000001e-10f * fSlow2) - (1.8898704000000002e-11f * fSlow0));
	float 	fSlow254 = (1.6641900000000002e-09f * fSlow2);
	float 	fSlow255 = (6.656760000000001e-11f + (fSlow254 + (fSlow0 * (fSlow253 - 4.7668896000000004e-11f))));
	float 	fSlow256 = (fConst1 * fSlow255);
	float 	fSlow257 = (0.0008200000000000001f * fSlow0);
	float 	fSlow258 = (0.00831f * fSlow2);
	float 	fSlow259 = (fConst1 * (0.005107400000000001f + (fSlow258 + fSlow257)));
	float 	fSlow260 = ((fSlow259 + (fConst2 * (fSlow256 - fSlow252))) - 1);
	float 	fSlow261 = (fConst3 * fSlow255);
	float 	fSlow262 = ((fConst2 * (fSlow252 + fSlow261)) - (3 + fSlow259));
	float 	fSlow263 = ((fSlow259 + (fConst2 * (fSlow252 - fSlow261))) - 3);
	float 	fSlow264 = (0 - (1 + (fSlow259 + (fConst2 * (fSlow252 + fSlow256)))));
	float 	fSlow265 = (1.0f / fSlow264);
	float 	fSlow266 = ((fSlow0 * (1.8898704000000002e-11f + fSlow253)) + (fSlow17 * ((6.656760000000001e-11f - (6.656760000000001e-11f * fSlow0)) + fSlow254)));
	float 	fSlow267 = (fConst3 * fSlow266);
	float 	fSlow268 = (3.1116000000000005e-08f + (((2.829e-07f * fSlow17) + (fSlow0 * (3.2176800000000005e-07f - fSlow251))) + (fSlow2 * (7.779000000000002e-07f + (6.8142000000000025e-06f * fSlow0)))));
	float 	fSlow269 = (0.00033240000000000006f + (fSlow258 + (fSlow257 + (6e-05f * fSlow17))));
	float 	fSlow270 = (fConst1 * fSlow269);
	float 	fSlow271 = (fSlow270 + (fConst2 * (fSlow268 - fSlow267)));
	float 	fSlow272 = (fConst1 * fSlow266);
	float 	fSlow273 = (fSlow270 + (fConst2 * (fSlow272 - fSlow268)));
	float 	fSlow274 = (fConst1 * (0 - fSlow269));
	float 	fSlow275 = (fSlow274 + (fConst2 * (fSlow268 + fSlow267)));
	float 	fSlow276 = (fSlow274 - (fConst2 * (fSlow268 + fSlow272)));
	float 	fSlow277 = ((fSlow31 == 15) / fSlow264);
	float 	fSlow278 = (4.0108000000000004e-07f * fSlow0);
	float 	fSlow279 = (5.050300000000001e-06f + ((0.00010263250000000001f * fSlow2) + (fSlow0 * (((1.0027e-05f * fSlow2) - 3.5719200000000006e-06f) - fSlow278))));
	float 	fSlow280 = ((9.45e-10f * fSlow2) - (3.78e-11f * fSlow0));
	float 	fSlow281 = (6.75e-09f * fSlow2);
	float 	fSlow282 = (2.7e-10f + (fSlow281 + (fSlow0 * (fSlow280 - 2.3219999999999998e-10f))));
	float 	fSlow283 = (fConst1 * fSlow282);
	float 	fSlow284 = (0.0004f * fSlow0);
	float 	fSlow285 = (0.025067500000000003f * fSlow2);
	float 	fSlow286 = (fConst1 * (0.0150702f + (fSlow285 + fSlow284)));
	float 	fSlow287 = ((fSlow286 + (fConst2 * (fSlow283 - fSlow279))) - 1);
	float 	fSlow288 = (fConst3 * fSlow282);
	float 	fSlow289 = ((fConst2 * (fSlow279 + fSlow288)) - (3 + fSlow286));
	float 	fSlow290 = ((fSlow286 + (fConst2 * (fSlow279 - fSlow288))) - 3);
	float 	fSlow291 = (0 - (1 + (fSlow286 + (fConst2 * (fSlow279 + fSlow283)))));
	float 	fSlow292 = (1.0f / fSlow291);
	float 	fSlow293 = ((fSlow0 * (3.78e-11f + fSlow280)) + (fSlow17 * ((2.7e-10f - (2.7e-10f * fSlow0)) + fSlow281)));
	float 	fSlow294 = (fConst3 * fSlow293);
	float 	fSlow295 = (1.0530000000000001e-07f + (((9.45e-07f * fSlow17) + (fSlow0 * (4.2808000000000006e-07f - fSlow278))) + (fSlow2 * (2.6324999999999998e-06f + (1.0027e-05f * fSlow0)))));
	float 	fSlow296 = (6.75e-05f * fSlow17);
	float 	fSlow297 = (0.0010027f + (fSlow285 + (fSlow284 + fSlow296)));
	float 	fSlow298 = (fConst1 * fSlow297);
	float 	fSlow299 = (fSlow298 + (fConst2 * (fSlow295 - fSlow294)));
	float 	fSlow300 = (fConst1 * fSlow293);
	float 	fSlow301 = (fSlow298 + (fConst2 * (fSlow300 - fSlow295)));
	float 	fSlow302 = (fConst1 * (0 - fSlow297));
	float 	fSlow303 = (fSlow302 + (fConst2 * (fSlow295 + fSlow294)));
	float 	fSlow304 = (fSlow302 - (fConst2 * (fSlow295 + fSlow300)));
	float 	fSlow305 = ((fSlow31 == 14) / fSlow291);
	float 	fSlow306 = (1.95976e-07f * fSlow0);
	float 	fSlow307 = (9.060568000000001e-07f + ((8.801210000000002e-06f * fSlow2) + (fSlow0 * (((2.4497000000000004e-06f * fSlow2) - 4.3256399999999996e-07f) - fSlow306))));
	float 	fSlow308 = ((2.0778120000000008e-10f * fSlow2) - (1.6622496000000003e-11f * fSlow0));
	float 	fSlow309 = (5.553900000000002e-10f * fSlow2);
	float 	fSlow310 = (4.4431200000000016e-11f + (fSlow309 + (fSlow0 * (fSlow308 - 2.7808704000000013e-11f))));
	float 	fSlow311 = (fConst1 * fSlow310);
	float 	fSlow312 = (0.00044f * fSlow0);
	float 	fSlow313 = (0.0055675f * fSlow2);
	float 	fSlow314 = (fConst1 * (0.0035049f + (fSlow313 + fSlow312)));
	float 	fSlow315 = ((fSlow314 + (fConst2 * (fSlow311 - fSlow307))) - 1);
	float 	fSlow316 = (fConst3 * fSlow310);
	float 	fSlow317 = ((fConst2 * (fSlow307 + fSlow316)) - (3 + fSlow314));
	float 	fSlow318 = ((fSlow314 + (fConst2 * (fSlow307 - fSlow316))) - 3);
	float 	fSlow319 = (0 - (1 + (fSlow314 + (fConst2 * (fSlow307 + fSlow311)))));
	float 	fSlow320 = (1.0f / fSlow319);
	float 	fSlow321 = ((fSlow0 * (1.6622496000000003e-11f + fSlow308)) + (fSlow17 * ((4.4431200000000016e-11f - (4.4431200000000016e-11f * fSlow0)) + fSlow309)));
	float 	fSlow322 = (fConst3 * fSlow321);
	float 	fSlow323 = (4.585680000000001e-08f + (((2.0196000000000004e-07f * fSlow17) + (fSlow0 * (2.2567600000000002e-07f - fSlow306))) + (fSlow2 * (5.732100000000001e-07f + (2.4497000000000004e-06f * fSlow0)))));
	float 	fSlow324 = (0.00044540000000000004f + (fSlow313 + (fSlow296 + fSlow312)));
	float 	fSlow325 = (fConst1 * fSlow324);
	float 	fSlow326 = (fSlow325 + (fConst2 * (fSlow323 - fSlow322)));
	float 	fSlow327 = (fConst1 * fSlow321);
	float 	fSlow328 = (fSlow325 + (fConst2 * (fSlow327 - fSlow323)));
	float 	fSlow329 = (fConst1 * (0 - fSlow324));
	float 	fSlow330 = (fSlow329 + (fConst2 * (fSlow323 + fSlow322)));
	float 	fSlow331 = (fSlow329 - (fConst2 * (fSlow323 + fSlow327)));
	float 	fSlow332 = ((fSlow31 == 13) / fSlow319);
	float 	fSlow333 = (4.9434000000000004e-08f * fSlow0);
	float 	fSlow334 = (7.748796000000001e-07f + ((2.8889960000000004e-05f * fSlow2) + (fSlow0 * (((4.943400000000001e-06f * fSlow2) - 1.2634599999999999e-07f) - fSlow333))));
	float 	fSlow335 = ((1.2443156000000004e-09f * fSlow2) - (1.2443156000000002e-11f * fSlow0));
	float 	fSlow336 = (5.345780000000001e-09f * fSlow2);
	float 	fSlow337 = (5.345780000000001e-11f + (fSlow336 + (fSlow0 * (fSlow335 - 4.101464400000001e-11f))));
	float 	fSlow338 = (fConst1 * fSlow337);
	float 	fSlow339 = (0.00022f * fSlow0);
	float 	fSlow340 = (fConst1 * (0.0025277f + (fSlow149 + fSlow339)));
	float 	fSlow341 = ((fSlow340 + (fConst2 * (fSlow338 - fSlow334))) - 1);
	float 	fSlow342 = (fConst3 * fSlow337);
	float 	fSlow343 = ((fConst2 * (fSlow334 + fSlow342)) - (3 + fSlow340));
	float 	fSlow344 = ((fSlow340 + (fConst2 * (fSlow334 - fSlow342))) - 3);
	float 	fSlow345 = (0 - (1 + (fSlow340 + (fConst2 * (fSlow334 + fSlow338)))));
	float 	fSlow346 = (1.0f / fSlow345);
	float 	fSlow347 = ((fSlow0 * (1.2443156000000002e-11f + fSlow335)) + (fSlow17 * ((5.345780000000001e-11f - (5.345780000000001e-11f * fSlow0)) + fSlow336)));
	float 	fSlow348 = (fConst3 * fSlow347);
	float 	fSlow349 = (6.141960000000001e-08f + (((4.859800000000001e-07f * fSlow17) + (fSlow0 * (1.0113400000000001e-07f - fSlow333))) + (fSlow2 * (6.141960000000001e-06f + (4.943400000000001e-06f * fSlow0)))));
	float 	fSlow350 = (0.00022470000000000001f + (fSlow149 + (fSlow339 + (0.00023500000000000002f * fSlow17))));
	float 	fSlow351 = (fConst1 * fSlow350);
	float 	fSlow352 = (fSlow351 + (fConst2 * (fSlow349 - fSlow348)));
	float 	fSlow353 = (fConst1 * fSlow347);
	float 	fSlow354 = (fSlow351 + (fConst2 * (fSlow353 - fSlow349)));
	float 	fSlow355 = (fConst1 * (0 - fSlow350));
	float 	fSlow356 = (fSlow355 + (fConst2 * (fSlow349 + fSlow348)));
	float 	fSlow357 = (fSlow355 - (fConst2 * (fSlow349 + fSlow353)));
	float 	fSlow358 = ((fSlow31 == 12) / fSlow345);
	float 	fSlow359 = (2.5587500000000006e-07f * fSlow0);
	float 	fSlow360 = (7.717400000000001e-07f + ((2.2033600000000005e-05f * fSlow2) + (fSlow0 * (((1.0235000000000001e-05f * fSlow2) - 1.5537499999999997e-07f) - fSlow359))));
	float 	fSlow361 = ((1.3959000000000001e-09f * fSlow2) - (3.48975e-11f * fSlow0));
	float 	fSlow362 = (2.2090000000000005e-09f * fSlow2);
	float 	fSlow363 = (5.522500000000001e-11f + (fSlow362 + (fSlow0 * (fSlow361 - 2.0327500000000007e-11f))));
	float 	fSlow364 = (fConst1 * fSlow363);
	float 	fSlow365 = (0.0005f * fSlow0);
	float 	fSlow366 = (0.020470000000000002f * fSlow2);
	float 	fSlow367 = (fConst1 * (0.0025092499999999998f + (fSlow366 + fSlow365)));
	float 	fSlow368 = ((fSlow367 + (fConst2 * (fSlow364 - fSlow360))) - 1);
	float 	fSlow369 = (fConst3 * fSlow363);
	float 	fSlow370 = ((fConst2 * (fSlow360 + fSlow369)) - (3 + fSlow367));
	float 	fSlow371 = ((fSlow367 + (fConst2 * (fSlow360 - fSlow369))) - 3);
	float 	fSlow372 = (0 - (1 + (fSlow367 + (fConst2 * (fSlow360 + fSlow364)))));
	float 	fSlow373 = (1.0f / fSlow372);
	float 	fSlow374 = ((fSlow0 * (3.48975e-11f + fSlow361)) + (fSlow17 * ((5.522500000000001e-11f - (5.522500000000001e-11f * fSlow0)) + fSlow362)));
	float 	fSlow375 = (fConst3 * fSlow374);
	float 	fSlow376 = (8.084000000000001e-08f + (((2.2090000000000003e-07f * fSlow17) + (fSlow0 * (3.146250000000001e-07f - fSlow359))) + (fSlow2 * (3.2336000000000007e-06f + (1.0235000000000001e-05f * fSlow0)))));
	float 	fSlow377 = (0.00051175f + (fSlow366 + (fSlow241 + fSlow365)));
	float 	fSlow378 = (fConst1 * fSlow377);
	float 	fSlow379 = (fSlow378 + (fConst2 * (fSlow376 - fSlow375)));
	float 	fSlow380 = (fConst1 * fSlow374);
	float 	fSlow381 = (fSlow378 + (fConst2 * (fSlow380 - fSlow376)));
	float 	fSlow382 = (fConst1 * (0 - fSlow377));
	float 	fSlow383 = (fSlow382 + (fConst2 * (fSlow376 + fSlow375)));
	float 	fSlow384 = (fSlow382 - (fConst2 * (fSlow376 + fSlow380)));
	float 	fSlow385 = ((fSlow31 == 11) / fSlow372);
	float 	fSlow386 = (0.00022854915600000004f * fSlow0);
	float 	fSlow387 = (0.00010871476000000002f + ((0.00010719478000000002f * fSlow2) + (fSlow0 * ((0.00012621831200000002f + (0.00022854915600000004f * fSlow2)) - fSlow386))));
	float 	fSlow388 = ((3.421299200000001e-08f * fSlow2) - (3.421299200000001e-08f * fSlow0));
	float 	fSlow389 = (1.0f + (fSlow2 + (93531720.34763868f * (fSlow0 * (2.3521432000000005e-08f + fSlow388)))));
	float 	fSlow390 = (fConst4 * fSlow389);
	float 	fSlow391 = (fConst1 * (0.036906800000000003f + ((0.022103400000000002f * fSlow2) + (0.01034f * fSlow0))));
	float 	fSlow392 = ((fSlow391 + (fConst2 * (fSlow390 - fSlow387))) - 1);
	float 	fSlow393 = (fConst5 * fSlow389);
	float 	fSlow394 = ((fConst2 * (fSlow387 + fSlow393)) - (3 + fSlow391));
	float 	fSlow395 = ((fSlow391 + (fConst2 * (fSlow387 - fSlow393))) - 3);
	float 	fSlow396 = ((fConst2 * (0 - (fSlow387 + fSlow390))) - (1 + fSlow391));
	float 	fSlow397 = (1.0f / fSlow396);
	float 	fSlow398 = ((fSlow0 * (3.421299200000001e-08f + fSlow388)) + (fSlow17 * ((1.0691560000000003e-08f - (1.0691560000000003e-08f * fSlow0)) + (1.0691560000000003e-08f * fSlow2))));
	float 	fSlow399 = (fConst3 * fSlow398);
	float 	fSlow400 = (3.7947800000000004e-06f + (((1.5199800000000001e-06f * fSlow17) + (fSlow0 * (0.00022961831200000004f - fSlow386))) + (fSlow2 * (3.7947800000000004e-06f + fSlow386))));
	float 	fSlow401 = (1.0f + (fSlow2 + ((0.0046780133373146215f * fSlow17) + (0.4678013337314621f * fSlow0))));
	float 	fSlow402 = (fConst6 * fSlow401);
	float 	fSlow403 = (fSlow402 + (fConst2 * (fSlow400 - fSlow399)));
	float 	fSlow404 = (fConst1 * fSlow398);
	float 	fSlow405 = (fSlow402 + (fConst2 * (fSlow404 - fSlow400)));
	float 	fSlow406 = (fConst1 * (0 - (0.022103400000000002f * fSlow401)));
	float 	fSlow407 = (fSlow406 + (fConst2 * (fSlow400 + fSlow399)));
	float 	fSlow408 = (fSlow406 - (fConst2 * (fSlow400 + fSlow404)));
	float 	fSlow409 = ((fSlow31 == 10) / fSlow396);
	float 	fSlow410 = (4.851e-08f * fSlow0);
	float 	fSlow411 = (7.172000000000001e-07f + ((4.972000000000001e-05f * fSlow2) + (fSlow0 * (((4.8510000000000015e-06f * fSlow2) - 4.2449000000000006e-07f) - fSlow410))));
	float 	fSlow412 = ((2.6620000000000007e-10f * fSlow2) - (2.662e-12f * fSlow0));
	float 	fSlow413 = (2.4200000000000003e-09f * fSlow2);
	float 	fSlow414 = (2.4200000000000004e-11f + (fSlow413 + (fSlow0 * (fSlow412 - 2.1538000000000003e-11f))));
	float 	fSlow415 = (fConst1 * fSlow414);
	float 	fSlow416 = (0.022050000000000004f * fSlow2);
	float 	fSlow417 = (fConst1 * (0.0046705f + (fSlow339 + fSlow416)));
	float 	fSlow418 = ((fSlow417 + (fConst2 * (fSlow415 - fSlow411))) - 1);
	float 	fSlow419 = (fConst3 * fSlow414);
	float 	fSlow420 = ((fConst2 * (fSlow411 + fSlow419)) - (3 + fSlow417));
	float 	fSlow421 = ((fSlow417 + (fConst2 * (fSlow411 - fSlow419))) - 3);
	float 	fSlow422 = (0 - (1 + (fSlow417 + (fConst2 * (fSlow411 + fSlow415)))));
	float 	fSlow423 = (1.0f / fSlow422);
	float 	fSlow424 = ((fSlow0 * (2.662e-12f + fSlow412)) + (fSlow17 * ((2.4200000000000004e-11f - (2.4200000000000004e-11f * fSlow0)) + fSlow413)));
	float 	fSlow425 = (fConst3 * fSlow424);
	float 	fSlow426 = (1.32e-08f + (((2.2000000000000004e-07f * fSlow17) + (fSlow0 * (5.951000000000001e-08f - fSlow410))) + (fSlow2 * (1.32e-06f + (4.8510000000000015e-06f * fSlow0)))));
	float 	fSlow427 = (0.00022050000000000002f + (fSlow416 + (fSlow339 + (5e-05f * fSlow17))));
	float 	fSlow428 = (fConst1 * fSlow427);
	float 	fSlow429 = (fSlow428 + (fConst2 * (fSlow426 - fSlow425)));
	float 	fSlow430 = (fConst1 * fSlow424);
	float 	fSlow431 = (fSlow428 + (fConst2 * (fSlow430 - fSlow426)));
	float 	fSlow432 = (fConst1 * (0 - fSlow427));
	float 	fSlow433 = (fSlow432 + (fConst2 * (fSlow426 + fSlow425)));
	float 	fSlow434 = (fSlow432 - (fConst2 * (fSlow426 + fSlow430)));
	float 	fSlow435 = ((fSlow31 == 9) / fSlow422);
	float 	fSlow436 = (1.38796875e-06f * fSlow0);
	float 	fSlow437 = (3.5279375000000002e-06f + ((3.1989375e-05f * fSlow2) + (fSlow0 * (((1.38796875e-05f * fSlow2) - 1.6311937500000001e-06f) - fSlow436))));
	float 	fSlow438 = ((1.0561781250000004e-09f * fSlow2) - (1.0561781250000003e-10f * fSlow0));
	float 	fSlow439 = (1.9328750000000005e-09f * fSlow2);
	float 	fSlow440 = (1.9328750000000007e-10f + (fSlow439 + (fSlow0 * (fSlow438 - 8.766968750000004e-11f))));
	float 	fSlow441 = (fConst1 * fSlow440);
	float 	fSlow442 = (0.001175f * fSlow0);
	float 	fSlow443 = (0.011812500000000002f * fSlow2);
	float 	fSlow444 = (fConst1 * (0.0065077500000000005f + (fSlow443 + fSlow442)));
	float 	fSlow445 = ((fSlow444 + (fConst2 * (fSlow441 - fSlow437))) - 1);
	float 	fSlow446 = (fConst3 * fSlow440);
	float 	fSlow447 = ((fConst2 * (fSlow437 + fSlow446)) - (3 + fSlow444));
	float 	fSlow448 = ((fSlow444 + (fConst2 * (fSlow437 - fSlow446))) - 3);
	float 	fSlow449 = (0 - (1 + (fSlow444 + (fConst2 * (fSlow437 + fSlow441)))));
	float 	fSlow450 = (1.0f / fSlow449);
	float 	fSlow451 = ((fSlow0 * (1.0561781250000003e-10f + fSlow438)) + (fSlow17 * ((1.9328750000000007e-10f - (1.9328750000000007e-10f * fSlow0)) + fSlow439)));
	float 	fSlow452 = (fConst3 * fSlow451);
	float 	fSlow453 = (1.0633750000000002e-07f + (((3.2900000000000005e-07f * fSlow17) + (fSlow0 * (1.4614062500000001e-06f - fSlow436))) + (fSlow2 * (1.0633750000000002e-06f + (1.38796875e-05f * fSlow0)))));
	float 	fSlow454 = (fSlow21 + fSlow442);
	float 	fSlow455 = (0.00118125f + (fSlow443 + fSlow454));
	float 	fSlow456 = (fConst1 * fSlow455);
	float 	fSlow457 = (fSlow456 + (fConst2 * (fSlow453 - fSlow452)));
	float 	fSlow458 = (fConst1 * fSlow451);
	float 	fSlow459 = (fSlow456 + (fConst2 * (fSlow458 - fSlow453)));
	float 	fSlow460 = (fConst1 * (0 - fSlow455));
	float 	fSlow461 = (fSlow460 + (fConst2 * (fSlow453 + fSlow452)));
	float 	fSlow462 = (fSlow460 - (fConst2 * (fSlow453 + fSlow458)));
	float 	fSlow463 = ((fSlow31 == 8) / fSlow449);
	float 	fSlow464 = (3.0937500000000006e-07f * fSlow0);
	float 	fSlow465 = (1.2375000000000003e-05f * fSlow2);
	float 	fSlow466 = (6.677000000000001e-07f + ((1.9448000000000004e-05f * fSlow2) + (fSlow0 * ((fSlow465 - 2.1175000000000003e-08f) - fSlow464))));
	float 	fSlow467 = ((1.7121500000000001e-09f * fSlow2) - (4.2803750000000003e-11f * fSlow0));
	float 	fSlow468 = (1.9965000000000003e-09f * fSlow2);
	float 	fSlow469 = (4.991250000000001e-11f + (fSlow468 + (fSlow0 * (fSlow467 - 7.108750000000004e-12f))));
	float 	fSlow470 = (fConst1 * fSlow469);
	float 	fSlow471 = (0.022500000000000003f * fSlow2);
	float 	fSlow472 = (fSlow8 + fSlow471);
	float 	fSlow473 = (fConst1 * (0.0021395000000000003f + fSlow472));
	float 	fSlow474 = ((fSlow473 + (fConst2 * (fSlow470 - fSlow466))) - 1);
	float 	fSlow475 = (fConst3 * fSlow469);
	float 	fSlow476 = ((fConst2 * (fSlow466 + fSlow475)) - (3 + fSlow473));
	float 	fSlow477 = ((fSlow473 + (fConst2 * (fSlow466 - fSlow475))) - 3);
	float 	fSlow478 = (0 - (1 + (fSlow473 + (fConst2 * (fSlow466 + fSlow470)))));
	float 	fSlow479 = (1.0f / fSlow478);
	float 	fSlow480 = ((fSlow0 * (4.2803750000000003e-11f + fSlow467)) + (fSlow17 * ((4.991250000000001e-11f - (4.991250000000001e-11f * fSlow0)) + fSlow468)));
	float 	fSlow481 = (fConst3 * fSlow480);
	float 	fSlow482 = (1.2375000000000003e-05f * fSlow0);
	float 	fSlow483 = (fSlow0 * (3.781250000000001e-07f - fSlow464));
	float 	fSlow484 = (8.690000000000002e-08f + (((1.815e-07f * fSlow17) + fSlow483) + (fSlow2 * (3.4760000000000007e-06f + fSlow482))));
	float 	fSlow485 = (0.0005625000000000001f + (fSlow471 + (fSlow8 + (0.000125f * fSlow17))));
	float 	fSlow486 = (fConst1 * fSlow485);
	float 	fSlow487 = (fSlow486 + (fConst2 * (fSlow484 - fSlow481)));
	float 	fSlow488 = (fConst1 * fSlow480);
	float 	fSlow489 = (fSlow486 + (fConst2 * (fSlow488 - fSlow484)));
	float 	fSlow490 = (fConst1 * (0 - fSlow485));
	float 	fSlow491 = (fSlow490 + (fConst2 * (fSlow484 + fSlow481)));
	float 	fSlow492 = (fSlow490 - (fConst2 * (fSlow484 + fSlow488)));
	float 	fSlow493 = ((fSlow31 == 7) / fSlow478);
	float 	fSlow494 = (3.0621250000000006e-07f * fSlow0);
	float 	fSlow495 = (5.442360000000002e-07f + ((1.784904e-05f * fSlow2) + (fSlow0 * (((1.2248500000000003e-05f * fSlow2) - 5.596250000000005e-08f) - fSlow494))));
	float 	fSlow496 = ((9.245610000000004e-10f * fSlow2) - (2.3114025000000008e-11f * fSlow0));
	float 	fSlow497 = (1.0781100000000005e-09f * fSlow2);
	float 	fSlow498 = (2.695275000000001e-11f + (fSlow497 + (fSlow0 * (fSlow496 - 3.8387250000000005e-12f))));
	float 	fSlow499 = (fConst1 * fSlow498);
	float 	fSlow500 = (0.02227f * fSlow2);
	float 	fSlow501 = (fConst1 * (0.00207625f + (fSlow8 + fSlow500)));
	float 	fSlow502 = ((fSlow501 + (fConst2 * (fSlow499 - fSlow495))) - 1);
	float 	fSlow503 = (fConst3 * fSlow498);
	float 	fSlow504 = ((fConst2 * (fSlow495 + fSlow503)) - (3 + fSlow501));
	float 	fSlow505 = ((fSlow501 + (fConst2 * (fSlow495 - fSlow503))) - 3);
	float 	fSlow506 = (0 - (1 + (fSlow501 + (fConst2 * (fSlow495 + fSlow499)))));
	float 	fSlow507 = (1.0f / fSlow506);
	float 	fSlow508 = ((fSlow0 * (2.3114025000000008e-11f + fSlow496)) + (fSlow17 * ((2.695275000000001e-11f - (2.695275000000001e-11f * fSlow0)) + fSlow497)));
	float 	fSlow509 = (fConst3 * fSlow508);
	float 	fSlow510 = (4.6926e-08f + (((9.801000000000002e-08f * fSlow17) + (fSlow0 * (3.433375000000001e-07f - fSlow494))) + (fSlow2 * (1.8770400000000002e-06f + (1.2248500000000003e-05f * fSlow0)))));
	float 	fSlow511 = (0.0005567500000000001f + (fSlow500 + (fSlow8 + fSlow296)));
	float 	fSlow512 = (fConst1 * fSlow511);
	float 	fSlow513 = (fSlow512 + (fConst2 * (fSlow510 - fSlow509)));
	float 	fSlow514 = (fConst1 * fSlow508);
	float 	fSlow515 = (fSlow512 + (fConst2 * (fSlow514 - fSlow510)));
	float 	fSlow516 = (fConst1 * (0 - fSlow511));
	float 	fSlow517 = (fSlow516 + (fConst2 * (fSlow510 + fSlow509)));
	float 	fSlow518 = (fSlow516 - (fConst2 * (fSlow510 + fSlow514)));
	float 	fSlow519 = ((fSlow31 == 6) / fSlow506);
	float 	fSlow520 = (1.08515e-06f + ((3.108600000000001e-05f * fSlow2) + (fSlow0 * ((fSlow465 - 2.99475e-07f) - fSlow464))));
	float 	fSlow521 = ((1.8513000000000002e-09f * fSlow2) - (4.628250000000001e-11f * fSlow0));
	float 	fSlow522 = (3.3880000000000003e-09f * fSlow2);
	float 	fSlow523 = (8.470000000000002e-11f + (fSlow522 + (fSlow0 * (fSlow521 - 3.8417500000000006e-11f))));
	float 	fSlow524 = (fConst1 * fSlow523);
	float 	fSlow525 = (fConst1 * (fSlow472 + 0.0031515000000000002f));
	float 	fSlow526 = ((fSlow525 + (fConst2 * (fSlow524 - fSlow520))) - 1);
	float 	fSlow527 = (fConst3 * fSlow523);
	float 	fSlow528 = ((fConst2 * (fSlow520 + fSlow527)) - (3 + fSlow525));
	float 	fSlow529 = ((fSlow525 + (fConst2 * (fSlow520 - fSlow527))) - 3);
	float 	fSlow530 = (0 - (1 + (fSlow525 + (fConst2 * (fSlow520 + fSlow524)))));
	float 	fSlow531 = (1.0f / fSlow530);
	float 	fSlow532 = ((fSlow0 * (4.628250000000001e-11f + fSlow521)) + (fSlow17 * ((8.470000000000002e-11f - (8.470000000000002e-11f * fSlow0)) + fSlow522)));
	float 	fSlow533 = (fConst3 * fSlow532);
	float 	fSlow534 = (9.955000000000001e-08f + ((fSlow483 + (3.08e-07f * fSlow17)) + (fSlow2 * (fSlow482 + 3.982e-06f))));
	float 	fSlow535 = (fSlow486 + (fConst2 * (fSlow534 - fSlow533)));
	float 	fSlow536 = (fConst1 * fSlow532);
	float 	fSlow537 = (fSlow486 + (fConst2 * (fSlow536 - fSlow534)));
	float 	fSlow538 = (fSlow490 + (fConst2 * (fSlow534 + fSlow533)));
	float 	fSlow539 = (fSlow490 - (fConst2 * (fSlow534 + fSlow536)));
	float 	fSlow540 = ((fSlow31 == 5) / fSlow530);
	float 	fSlow541 = (5.665800800000001e-07f + ((1.892924e-05f * fSlow2) + (fSlow0 * ((fSlow142 - 6.207784000000001e-08f) - fSlow141))));
	float 	fSlow542 = ((1.2661536800000005e-09f * fSlow2) - (2.7855380960000008e-11f * fSlow0));
	float 	fSlow543 = (1.6515048000000004e-09f * fSlow2);
	float 	fSlow544 = (3.6333105600000014e-11f + (fSlow543 + (fSlow0 * (fSlow542 - 8.477724640000006e-12f))));
	float 	fSlow545 = (fConst1 * fSlow544);
	float 	fSlow546 = (fConst1 * (fSlow150 + 0.0020497400000000004f));
	float 	fSlow547 = ((fSlow546 + (fConst2 * (fSlow545 - fSlow541))) - 1);
	float 	fSlow548 = (fConst3 * fSlow544);
	float 	fSlow549 = ((fConst2 * (fSlow541 + fSlow548)) - (3 + fSlow546));
	float 	fSlow550 = ((fSlow546 + (fConst2 * (fSlow541 - fSlow548))) - 3);
	float 	fSlow551 = (0 - (1 + (fSlow546 + (fConst2 * (fSlow541 + fSlow545)))));
	float 	fSlow552 = (1.0f / fSlow551);
	float 	fSlow553 = ((fSlow0 * (2.7855380960000008e-11f + fSlow542)) + (fSlow17 * ((3.6333105600000014e-11f - (3.6333105600000014e-11f * fSlow0)) + fSlow543)));
	float 	fSlow554 = (fConst3 * fSlow553);
	float 	fSlow555 = (6.505928000000001e-08f + ((fSlow161 + (1.5013680000000003e-07f * fSlow17)) + (fSlow2 * (fSlow160 + 2.95724e-06f))));
	float 	fSlow556 = (fSlow164 + (fConst2 * (fSlow555 - fSlow554)));
	float 	fSlow557 = (fConst1 * fSlow553);
	float 	fSlow558 = (fSlow164 + (fConst2 * (fSlow557 - fSlow555)));
	float 	fSlow559 = (fSlow168 + (fConst2 * (fSlow555 + fSlow554)));
	float 	fSlow560 = (fSlow168 - (fConst2 * (fSlow555 + fSlow557)));
	float 	fSlow561 = ((fSlow31 == 4) / fSlow551);
	float 	fSlow562 = (1.0855872000000003e-07f * fSlow0);
	float 	fSlow563 = (3.222390000000001e-06f + (fSlow62 + (fSlow0 * (((5.6541000000000015e-06f * fSlow2) - 2.1333412800000006e-06f) - fSlow562))));
	float 	fSlow564 = (4.935e-10f * fSlow2);
	float 	fSlow565 = (fSlow564 - (9.4752e-12f * fSlow0));
	float 	fSlow566 = (1.41e-10f + (fSlow65 + (fSlow0 * (fSlow565 - 1.315248e-10f))));
	float 	fSlow567 = (fConst1 * fSlow566);
	float 	fSlow568 = (0.0002256f * fSlow0);
	float 	fSlow569 = (fConst1 * (0.015243699999999999f + (fSlow9 + fSlow568)));
	float 	fSlow570 = ((fSlow569 + (fConst2 * (fSlow567 - fSlow563))) - 1);
	float 	fSlow571 = (fConst3 * fSlow566);
	float 	fSlow572 = ((fConst2 * (fSlow563 + fSlow571)) - (3 + fSlow569));
	float 	fSlow573 = ((fSlow569 + (fConst2 * (fSlow563 - fSlow571))) - 3);
	float 	fSlow574 = (0 - (1 + (fSlow569 + (fConst2 * (fSlow563 + fSlow567)))));
	float 	fSlow575 = (1.0f / fSlow574);
	float 	fSlow576 = (1.41e-10f - (1.41e-10f * fSlow0));
	float 	fSlow577 = ((fSlow0 * (9.4752e-12f + fSlow565)) + (fSlow17 * (fSlow65 + fSlow576)));
	float 	fSlow578 = (fConst3 * fSlow577);
	float 	fSlow579 = (4.764000000000001e-08f + ((fSlow78 + (fSlow0 * (1.2265872000000003e-07f - fSlow562))) + (fSlow2 * (2.48125e-06f + (5.6541000000000015e-06f * fSlow0)))));
	float 	fSlow580 = (0.00048120000000000004f + (fSlow9 + (fSlow21 + fSlow568)));
	float 	fSlow581 = (fConst1 * fSlow580);
	float 	fSlow582 = (fSlow581 + (fConst2 * (fSlow579 - fSlow578)));
	float 	fSlow583 = (fConst1 * fSlow577);
	float 	fSlow584 = (fSlow581 + (fConst2 * (fSlow583 - fSlow579)));
	float 	fSlow585 = (fConst1 * (0 - fSlow580));
	float 	fSlow586 = (fSlow585 + (fConst2 * (fSlow579 + fSlow578)));
	float 	fSlow587 = (fSlow585 - (fConst2 * (fSlow579 + fSlow583)));
	float 	fSlow588 = ((fSlow31 == 3) / fSlow574);
	float 	fSlow589 = (4.7056400000000006e-07f * fSlow0);
	float 	fSlow590 = (5.188640000000001e-06f + ((0.00011869100000000002f * fSlow2) + (fSlow0 * (((1.1764100000000001e-05f * fSlow2) - 4.215336e-06f) - fSlow589))));
	float 	fSlow591 = (fSlow564 - (1.974e-11f * fSlow0));
	float 	fSlow592 = (3.525e-09f * fSlow2);
	float 	fSlow593 = (1.41e-10f + (fSlow592 + (fSlow0 * (fSlow591 - 1.2126e-10f))));
	float 	fSlow594 = (fConst1 * fSlow593);
	float 	fSlow595 = (0.02503f * fSlow2);
	float 	fSlow596 = (fConst1 * (0.0157312f + (fSlow68 + fSlow595)));
	float 	fSlow597 = ((fSlow596 + (fConst2 * (fSlow594 - fSlow590))) - 1);
	float 	fSlow598 = (fConst3 * fSlow593);
	float 	fSlow599 = ((fConst2 * (fSlow590 + fSlow598)) - (3 + fSlow596));
	float 	fSlow600 = ((fSlow596 + (fConst2 * (fSlow590 - fSlow598))) - 3);
	float 	fSlow601 = (0 - (1 + (fSlow596 + (fConst2 * (fSlow590 + fSlow594)))));
	float 	fSlow602 = (1.0f / fSlow601);
	float 	fSlow603 = ((fSlow0 * (1.974e-11f + fSlow591)) + (fSlow17 * (fSlow576 + fSlow592)));
	float 	fSlow604 = (fConst3 * fSlow603);
	float 	fSlow605 = (4.764000000000001e-08f + (((4.410000000000001e-07f * fSlow17) + (fSlow0 * (4.846640000000001e-07f - fSlow589))) + (fSlow2 * (1.1910000000000001e-06f + (1.1764100000000001e-05f * fSlow0)))));
	float 	fSlow606 = (0.0010012f + (fSlow595 + (fSlow68 + (3e-05f * fSlow17))));
	float 	fSlow607 = (fConst1 * fSlow606);
	float 	fSlow608 = (fSlow607 + (fConst2 * (fSlow605 - fSlow604)));
	float 	fSlow609 = (fConst1 * fSlow603);
	float 	fSlow610 = (fSlow607 + (fConst2 * (fSlow609 - fSlow605)));
	float 	fSlow611 = (fConst1 * (0 - fSlow606));
	float 	fSlow612 = (fSlow611 + (fConst2 * (fSlow605 + fSlow604)));
	float 	fSlow613 = (fSlow611 - (fConst2 * (fSlow605 + fSlow609)));
	float 	fSlow614 = ((fSlow31 == 2) / fSlow601);
	float 	fSlow615 = (2.9448437500000003e-06f * fSlow0);
	float 	fSlow616 = (1.2916875000000002e-05f + (fSlow62 + (fSlow0 * (((2.9448437500000007e-05f * fSlow2) - 8.731718750000001e-06f) - fSlow615))));
	float 	fSlow617 = ((2.5703125000000004e-09f * fSlow2) - (2.5703125000000003e-10f * fSlow0));
	float 	fSlow618 = (7.343750000000001e-10f + (fSlow65 + (fSlow0 * (fSlow617 - 4.773437500000001e-10f))));
	float 	fSlow619 = (fConst1 * fSlow618);
	float 	fSlow620 = (fConst1 * (0.01726875f + (fSlow9 + fSlow442)));
	float 	fSlow621 = ((fSlow620 + (fConst2 * (fSlow619 - fSlow616))) - 1);
	float 	fSlow622 = (fConst3 * fSlow618);
	float 	fSlow623 = ((fConst2 * (fSlow616 + fSlow622)) - (3 + fSlow620));
	float 	fSlow624 = ((fSlow620 + (fConst2 * (fSlow616 - fSlow622))) - 3);
	float 	fSlow625 = (0 - (1 + (fSlow620 + (fConst2 * (fSlow616 + fSlow619)))));
	float 	fSlow626 = (1.0f / fSlow625);
	float 	fSlow627 = ((fSlow0 * (2.5703125000000003e-10f + fSlow617)) + (fSlow17 * (fSlow65 + (7.343750000000001e-10f - (7.343750000000001e-10f * fSlow0)))));
	float 	fSlow628 = (fConst3 * fSlow627);
	float 	fSlow629 = (2.48125e-07f + ((fSlow78 + (fSlow0 * (3.0182812500000004e-06f - fSlow615))) + (fSlow2 * (2.48125e-06f + (2.9448437500000007e-05f * fSlow0)))));
	float 	fSlow630 = (0.0025062500000000002f + (fSlow9 + fSlow454));
	float 	fSlow631 = (fConst1 * fSlow630);
	float 	fSlow632 = (fSlow631 + (fConst2 * (fSlow629 - fSlow628)));
	float 	fSlow633 = (fConst1 * fSlow627);
	float 	fSlow634 = (fSlow631 + (fConst2 * (fSlow633 - fSlow629)));
	float 	fSlow635 = (fConst1 * (0 - fSlow630));
	float 	fSlow636 = (fSlow635 + (fConst2 * (fSlow629 + fSlow628)));
	float 	fSlow637 = (fSlow635 - (fConst2 * (fSlow629 + fSlow633)));
	float 	fSlow638 = ((fSlow31 == 1) / fSlow625);
	float 	fSlow639 = (2.5312500000000006e-07f * fSlow0);
	float 	fSlow640 = (7.4525e-07f + ((2.4210000000000004e-05f * fSlow2) + (fSlow0 * (((1.0125e-05f * fSlow2) - 2.75625e-07f) - fSlow639))));
	float 	fSlow641 = ((7.650000000000002e-10f * fSlow2) - (1.9125000000000002e-11f * fSlow0));
	float 	fSlow642 = (1.4000000000000001e-09f * fSlow2);
	float 	fSlow643 = (3.500000000000001e-11f + (fSlow642 + (fSlow0 * (fSlow641 - 1.5875000000000007e-11f))));
	float 	fSlow644 = (fConst1 * fSlow643);
	float 	fSlow645 = (0.02025f * fSlow2);
	float 	fSlow646 = (fConst1 * (0.0028087500000000005f + (fSlow365 + fSlow645)));
	float 	fSlow647 = ((fSlow646 + (fConst2 * (fSlow644 - fSlow640))) - 1);
	float 	fSlow648 = (fConst3 * fSlow643);
	float 	fSlow649 = ((fConst2 * (fSlow640 + fSlow648)) - (3 + fSlow646));
	float 	fSlow650 = ((fSlow646 + (fConst2 * (fSlow640 - fSlow648))) - 3);
	float 	fSlow651 = (0 - (1 + (fSlow646 + (fConst2 * (fSlow640 + fSlow644)))));
	float 	fSlow652 = (1.0f / fSlow651);
	float 	fSlow653 = ((fSlow0 * (1.9125000000000002e-11f + fSlow641)) + (fSlow17 * ((3.500000000000001e-11f - (3.500000000000001e-11f * fSlow0)) + fSlow642)));
	float 	fSlow654 = (fConst3 * fSlow653);
	float 	fSlow655 = (4.525e-08f + (((1.4e-07f * fSlow17) + (fSlow0 * (2.8437500000000003e-07f - fSlow639))) + (fSlow2 * (1.8100000000000002e-06f + (1.0125e-05f * fSlow0)))));
	float 	fSlow656 = (0.00050625f + (fSlow645 + (fSlow21 + fSlow365)));
	float 	fSlow657 = (fConst1 * fSlow656);
	float 	fSlow658 = (fSlow657 + (fConst2 * (fSlow655 - fSlow654)));
	float 	fSlow659 = (fConst1 * fSlow653);
	float 	fSlow660 = (fSlow657 + (fConst2 * (fSlow659 - fSlow655)));
	float 	fSlow661 = (fConst1 * (0 - fSlow656));
	float 	fSlow662 = (fSlow661 + (fConst2 * (fSlow655 + fSlow654)));
	float 	fSlow663 = (fSlow661 - (fConst2 * (fSlow655 + fSlow659)));
	float 	fSlow664 = ((fSlow31 == 0) / fSlow651);

	float tubeout = 0.f;
	
	for (uint32_t i = 0; i < frames; ++i) {

		//Step 1: read input sample as voltage for the source
		float in = sanitize_denormal(inputs[0][i]);

		// protect against overflowing circuit
		in = fabs(in) < DANGER ? in : 0.f; 

		Vi.e = in*from_dB(tubedrive);

		//Step 2: propagate waves up to the triode and push values into triode element
		I1.waveUp();
		I3.waveUp();
		P2.waveUp();
		v.G.WD = sanitize_denormal(I1.WU);
		v.K.WD = sanitize_denormal(I3.WU); 
		v.P.WD = sanitize_denormal(P2.WU);
		v.vg = v.G.WD;
		v.vk = v.K.WD;
		v.vp = v.P.WD;
		v.G.PortRes = sanitize_denormal(I1.PortRes);
		v.K.PortRes = sanitize_denormal(I3.PortRes);
		v.P.PortRes = sanitize_denormal(P2.PortRes);

		//Step 3: compute wave reflections inside the triode
		T vg0, vg1, vp0, vp1;

		vg0 = -10.0;
		vg1 = 10.0;
		v.vg = sanitize_denormal(v.zeroffg(vg0,vg1,TOLERANCE));
		//v.vg = v.secantfg(&vg0,&vg1);

		vp0 = e;
		vp1 = 0.0;
		v.vp = sanitize_denormal(v.zeroffp(vp0,vp1,TOLERANCE));
		//v.vp = v.secantfp(&vp0,&vp1);

		v.vk = sanitize_denormal(v.ffk());

		v.G.WU = sanitize_denormal(2.0*v.vg-v.G.WD);
		v.K.WU = sanitize_denormal(2.0*v.vk-v.K.WD);
		v.P.WU = sanitize_denormal(2.0*v.vp-v.P.WD);
		
		outputs[0][i] = in;
		
		tubeout = -Ro.Voltage()/e; //invert signal and rescale
		tubeout = sanitize_denormal(tubeout);

		P2.setWD(v.P.WU); 
		I1.setWD(v.G.WU);
		I3.setWD(v.K.WU);
		
		//Tone Stack sim
		fRec0[0] = ((float)tubeout - (fSlow16 * (((fSlow14 * fRec0[2]) + (fSlow13 * fRec0[1])) + (fSlow11 * fRec0[3]))));
		fRec1[0] = ((float)tubeout - (fSlow47 * (((fSlow45 * fRec1[2]) + (fSlow44 * fRec1[1])) + (fSlow42 * fRec1[3]))));
		fRec2[0] = ((float)tubeout - (fSlow75 * (((fSlow73 * fRec2[2]) + (fSlow72 * fRec2[1])) + (fSlow70 * fRec2[3]))));
		fRec3[0] = ((float)tubeout - (fSlow102 * (((fSlow100 * fRec3[2]) + (fSlow99 * fRec3[1])) + (fSlow97 * fRec3[3]))));
		fRec4[0] = ((float)tubeout - (fSlow128 * (((fSlow126 * fRec4[2]) + (fSlow125 * fRec4[1])) + (fSlow123 * fRec4[3]))));
		fRec5[0] = ((float)tubeout - (fSlow157 * (((fSlow155 * fRec5[2]) + (fSlow154 * fRec5[1])) + (fSlow152 * fRec5[3]))));
		fRec6[0] = ((float)tubeout - (fSlow186 * (((fSlow184 * fRec6[2]) + (fSlow183 * fRec6[1])) + (fSlow181 * fRec6[3]))));
		fRec7[0] = ((float)tubeout - (fSlow212 * (((fSlow210 * fRec7[2]) + (fSlow209 * fRec7[1])) + (fSlow207 * fRec7[3]))));
		fRec8[0] = ((float)tubeout - (fSlow237 * (((fSlow235 * fRec8[2]) + (fSlow234 * fRec8[1])) + (fSlow232 * fRec8[3]))));
		fRec9[0] = ((float)tubeout - (fSlow265 * (((fSlow263 * fRec9[2]) + (fSlow262 * fRec9[1])) + (fSlow260 * fRec9[3]))));
		fRec10[0] = ((float)tubeout - (fSlow292 * (((fSlow290 * fRec10[2]) + (fSlow289 * fRec10[1])) + (fSlow287 * fRec10[3]))));
		fRec11[0] = ((float)tubeout - (fSlow320 * (((fSlow318 * fRec11[2]) + (fSlow317 * fRec11[1])) + (fSlow315 * fRec11[3]))));
		fRec12[0] = ((float)tubeout - (fSlow346 * (((fSlow344 * fRec12[2]) + (fSlow343 * fRec12[1])) + (fSlow341 * fRec12[3]))));
		fRec13[0] = ((float)tubeout - (fSlow373 * (((fSlow371 * fRec13[2]) + (fSlow370 * fRec13[1])) + (fSlow368 * fRec13[3]))));
		fRec14[0] = ((float)tubeout - (fSlow397 * (((fSlow395 * fRec14[2]) + (fSlow394 * fRec14[1])) + (fSlow392 * fRec14[3]))));
		fRec15[0] = ((float)tubeout - (fSlow423 * (((fSlow421 * fRec15[2]) + (fSlow420 * fRec15[1])) + (fSlow418 * fRec15[3]))));
		fRec16[0] = ((float)tubeout - (fSlow450 * (((fSlow448 * fRec16[2]) + (fSlow447 * fRec16[1])) + (fSlow445 * fRec16[3]))));
		fRec17[0] = ((float)tubeout - (fSlow479 * (((fSlow477 * fRec17[2]) + (fSlow476 * fRec17[1])) + (fSlow474 * fRec17[3]))));
		fRec18[0] = ((float)tubeout - (fSlow507 * (((fSlow505 * fRec18[2]) + (fSlow504 * fRec18[1])) + (fSlow502 * fRec18[3]))));
		fRec19[0] = ((float)tubeout - (fSlow531 * (((fSlow529 * fRec19[2]) + (fSlow528 * fRec19[1])) + (fSlow526 * fRec19[3]))));
		fRec20[0] = ((float)tubeout - (fSlow552 * (((fSlow550 * fRec20[2]) + (fSlow549 * fRec20[1])) + (fSlow547 * fRec20[3]))));
		fRec21[0] = ((float)tubeout - (fSlow575 * (((fSlow573 * fRec21[2]) + (fSlow572 * fRec21[1])) + (fSlow570 * fRec21[3]))));
		fRec22[0] = ((float)tubeout - (fSlow602 * (((fSlow600 * fRec22[2]) + (fSlow599 * fRec22[1])) + (fSlow597 * fRec22[3]))));
		fRec23[0] = ((float)tubeout - (fSlow626 * (((fSlow624 * fRec23[2]) + (fSlow623 * fRec23[1])) + (fSlow621 * fRec23[3]))));
		fRec24[0] = ((float)tubeout - (fSlow652 * (((fSlow650 * fRec24[2]) + (fSlow649 * fRec24[1])) + (fSlow647 * fRec24[3]))));
		outputs[0][i] = (FAUSTFLOAT)((fSlow664 * ((fSlow663 * fRec24[0]) + ((fSlow662 * fRec24[1]) + ((fSlow660 * fRec24[3]) + (fSlow658 * fRec24[2]))))) + ((fSlow638 * ((fSlow637 * fRec23[0]) + ((fSlow636 * fRec23[1]) + ((fSlow634 * fRec23[3]) + (fSlow632 * fRec23[2]))))) + ((fSlow614 * ((fSlow613 * fRec22[0]) + ((fSlow612 * fRec22[1]) + ((fSlow610 * fRec22[3]) + (fSlow608 * fRec22[2]))))) + ((fSlow588 * ((fSlow587 * fRec21[0]) + ((fSlow586 * fRec21[1]) + ((fSlow584 * fRec21[3]) + (fSlow582 * fRec21[2]))))) + ((fSlow561 * ((fSlow560 * fRec20[0]) + ((fSlow559 * fRec20[1]) + ((fSlow558 * fRec20[3]) + (fSlow556 * fRec20[2]))))) + ((fSlow540 * ((fSlow539 * fRec19[0]) + ((fSlow538 * fRec19[1]) + ((fSlow537 * fRec19[3]) + (fSlow535 * fRec19[2]))))) + ((fSlow519 * ((fSlow518 * fRec18[0]) + ((fSlow517 * fRec18[1]) + ((fSlow515 * fRec18[3]) + (fSlow513 * fRec18[2]))))) + ((fSlow493 * ((fSlow492 * fRec17[0]) + ((fSlow491 * fRec17[1]) + ((fSlow489 * fRec17[3]) + (fSlow487 * fRec17[2]))))) + ((fSlow463 * ((fSlow462 * fRec16[0]) + ((fSlow461 * fRec16[1]) + ((fSlow459 * fRec16[3]) + (fSlow457 * fRec16[2]))))) + ((fSlow435 * ((fSlow434 * fRec15[0]) + ((fSlow433 * fRec15[1]) + ((fSlow431 * fRec15[3]) + (fSlow429 * fRec15[2]))))) + ((fSlow409 * ((fSlow408 * fRec14[0]) + ((fSlow407 * fRec14[1]) + ((fSlow405 * fRec14[3]) + (fSlow403 * fRec14[2]))))) + ((fSlow385 * ((fSlow384 * fRec13[0]) + ((fSlow383 * fRec13[1]) + ((fSlow381 * fRec13[3]) + (fSlow379 * fRec13[2]))))) + ((fSlow358 * ((fSlow357 * fRec12[0]) + ((fSlow356 * fRec12[1]) + ((fSlow354 * fRec12[3]) + (fSlow352 * fRec12[2]))))) + ((fSlow332 * ((fSlow331 * fRec11[0]) + ((fSlow330 * fRec11[1]) + ((fSlow328 * fRec11[3]) + (fSlow326 * fRec11[2]))))) + ((fSlow305 * ((fSlow304 * fRec10[0]) + ((fSlow303 * fRec10[1]) + ((fSlow301 * fRec10[3]) + (fSlow299 * fRec10[2]))))) + ((fSlow277 * ((fSlow276 * fRec9[0]) + ((fSlow275 * fRec9[1]) + ((fSlow273 * fRec9[3]) + (fSlow271 * fRec9[2]))))) + ((fSlow250 * ((fSlow249 * fRec8[0]) + ((fSlow248 * fRec8[1]) + ((fSlow246 * fRec8[3]) + (fSlow244 * fRec8[2]))))) + ((fSlow224 * ((fSlow223 * fRec7[0]) + ((fSlow222 * fRec7[1]) + ((fSlow220 * fRec7[3]) + (fSlow218 * fRec7[2]))))) + ((fSlow198 * ((fSlow197 * fRec6[0]) + ((fSlow196 * fRec6[1]) + ((fSlow194 * fRec6[3]) + (fSlow192 * fRec6[2]))))) + ((fSlow171 * ((fSlow170 * fRec5[0]) + ((fSlow169 * fRec5[1]) + ((fSlow167 * fRec5[3]) + (fSlow165 * fRec5[2]))))) + ((fSlow140 * ((fSlow139 * fRec4[0]) + ((fSlow138 * fRec4[1]) + ((fSlow136 * fRec4[3]) + (fSlow134 * fRec4[2]))))) + ((fSlow114 * ((fSlow113 * fRec3[0]) + ((fSlow112 * fRec3[1]) + ((fSlow110 * fRec3[3]) + (fSlow108 * fRec3[2]))))) + ((fSlow88 * ((fSlow87 * fRec2[0]) + ((fSlow86 * fRec2[1]) + ((fSlow84 * fRec2[3]) + (fSlow82 * fRec2[2]))))) + ((fSlow60 * ((fSlow59 * fRec1[0]) + ((fSlow58 * fRec1[1]) + ((fSlow56 * fRec1[3]) + (fSlow54 * fRec1[2]))))) + (fSlow32 * ((fSlow30 * fRec0[0]) + ((fSlow29 * fRec0[1]) + ((fSlow27 * fRec0[3]) + (fSlow25 * fRec0[2])))))))))))))))))))))))))))))* from_dB(mastergain);
		outputs[0][i] = sanitize_denormal(outputs[0][i]);

		// post processing
		for (int i=3; i>0; i--) fRec24[i] = sanitize_denormal(fRec24[i-1]);
		for (int i=3; i>0; i--) fRec23[i] = sanitize_denormal(fRec23[i-1]);
		for (int i=3; i>0; i--) fRec22[i] = sanitize_denormal(fRec22[i-1]);
		for (int i=3; i>0; i--) fRec21[i] = sanitize_denormal(fRec21[i-1]);
		for (int i=3; i>0; i--) fRec20[i] = sanitize_denormal(fRec20[i-1]);
		for (int i=3; i>0; i--) fRec19[i] = sanitize_denormal(fRec19[i-1]);
		for (int i=3; i>0; i--) fRec18[i] = sanitize_denormal(fRec18[i-1]);
		for (int i=3; i>0; i--) fRec17[i] = sanitize_denormal(fRec17[i-1]);
		for (int i=3; i>0; i--) fRec16[i] = sanitize_denormal(fRec16[i-1]);
		for (int i=3; i>0; i--) fRec15[i] = sanitize_denormal(fRec15[i-1]);
		for (int i=3; i>0; i--) fRec14[i] = sanitize_denormal(fRec14[i-1]);
		for (int i=3; i>0; i--) fRec13[i] = sanitize_denormal(fRec13[i-1]);
		for (int i=3; i>0; i--) fRec12[i] = sanitize_denormal(fRec12[i-1]);
		for (int i=3; i>0; i--) fRec11[i] = sanitize_denormal(fRec11[i-1]);
		for (int i=3; i>0; i--) fRec10[i] = sanitize_denormal(fRec10[i-1]);
		for (int i=3; i>0; i--) fRec9[i] = sanitize_denormal(fRec9[i-1]);
		for (int i=3; i>0; i--) fRec8[i] = sanitize_denormal(fRec8[i-1]);
		for (int i=3; i>0; i--) fRec7[i] = sanitize_denormal(fRec7[i-1]);
		for (int i=3; i>0; i--) fRec6[i] = sanitize_denormal(fRec6[i-1]);
		for (int i=3; i>0; i--) fRec5[i] = sanitize_denormal(fRec5[i-1]);
		for (int i=3; i>0; i--) fRec4[i] = sanitize_denormal(fRec4[i-1]);
		for (int i=3; i>0; i--) fRec3[i] = sanitize_denormal(fRec3[i-1]);
		for (int i=3; i>0; i--) fRec2[i] = sanitize_denormal(fRec2[i-1]);
		for (int i=3; i>0; i--) fRec1[i] = sanitize_denormal(fRec1[i-1]);
		for (int i=3; i>0; i--) fRec0[i] = sanitize_denormal(fRec0[i-1]);
	}
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new ZamTubePlugin();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
