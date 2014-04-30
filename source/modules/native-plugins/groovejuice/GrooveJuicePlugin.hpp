/*
 * Vector Juice Plugin
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

#ifndef GROOVEJUICEPLUGIN_HPP_INCLUDED
#define GROOVEJUICEPLUGIN_HPP_INCLUDED

#include "DistrhoPlugin.hpp"
#include <iostream>
#include "Synth.hxx"
#include "moogVCF.hxx"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class GrooveJuicePlugin : public Plugin
{
public:
    enum Parameters
    {
        paramX = 0,
        paramY,
        paramOrbitSizeX,
        paramOrbitSizeY,
        paramOrbitSpeedX,
        paramOrbitSpeedY,
        paramSubOrbitSpeed,
        paramSubOrbitSize,
        paramOrbitWaveX,
        paramOrbitWaveY,
        paramOrbitPhaseX,
        paramOrbitPhaseY,
        paramSubOrbitSmooth,
        paramOrbitOutX,
        paramOrbitOutY,
        paramSubOrbitOutX,
        paramSubOrbitOutY,
	   w11, w21, m1, c1, r1, s1, re1, sh1,
	   w12, w22, m2, c2, r2, s2, re2, sh2,
	   w13, w23, m3, c3, r3, s3, re3, sh3,
	   w14, w24, m4, c4, r4, s4, re4, sh4,
	   w15, w25, m5, c5, r5, s5, re5, sh5,
	   w16, w26, m6, c6, r6, s6, re6, sh6,
	   w17, w27, m7, c7, r7, s7, re7, sh7,
	   w18, w28, m8, c8, r8, s8, re8, sh8,
	   paramW1Out,
	   paramW2Out,
	   paramMOut,
	   paramCOut,
	   paramROut,
	   paramSOut,
	   paramReOut,
	   paramShOut,
        paramCount
    };

    inline float smoothParameter(float in, int axis) {
            sZ[axis] = (in * sB[axis]) + (sZ[axis] * sA[axis]);
            return sZ[axis];
    }

    float getSinePhase(float x) {
        return (-std::sin(x));
    }
    float getSawPhase(float x) {
        return (-(2/M_PI *std::atan(1/std::tan(x/2))));
    }
    float getRevSawPhase(float x) {
        return ((2/M_PI *std::atan(1/std::tan(x/2))));
    }
    float getSquarePhase(float x) {
        return (std::round((std::sin(x)+1)/2)-0.5)*2;
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

    void resetPhase()
    {
        const TimePos& time = d_getTimePos();
        if (!time.playing)
        {
            sinePosX = 0;
            sinePosY = 0;
            sinePos = 0;
        }
    }

    float tN (float x)
    {
        if (x>1) x=1;
	   if (x<0) x=0;
        return x;
    }
    
    float distance(float x, float y, float tX, float tY) {
		float result = pow(tN(1-(sqrt((tX - x)*(tX - x) + (tY - y)*(tY - y)))), 3);
		return result;
    }
    

    void animate()
    {
        //sync orbit with frame, bpm
        const TimePos& time = d_getTimePos();
        bar = ((120.0/(time.bbt.valid ? time.bbt.beatsPerMinute : 120.0))*(d_getSampleRate()));

        int multiplier = 16;//2000*4;
        tickX = bar/(std::round(orbitSpeedX))*multiplier;
        tickY = bar/(std::round(orbitSpeedY))*multiplier;
        tick = bar/(std::round(subOrbitSpeed))*multiplier;

        if (time.playing)
        {
            /* if rolling then sync to timepos */
            tickOffsetX = time.frame-std::floor(time.frame/tickX)*tickX;
            tickOffsetY = time.frame-std::floor(time.frame/tickY)*tickY;
            tickOffset = time.frame-std::floor(time.frame/tick)*tick;
            percentageX = tickOffsetX/tickX;
            percentageY = tickOffsetY/tickY;
            percentage = tickOffset/tick;
            sinePosX = (M_PI*2)*percentageX;
            sinePosY = (M_PI*2)*percentageY;
            sinePos = (M_PI*2)*percentage;
        } else {
            /* else just keep on wobblin' */
            sinePosX += (2*M_PI)/(tickX);
            sinePosY += (2*M_PI)/(tickY);
            sinePos += (M_PI)/(tick);
            if (sinePosX>2*M_PI) {
                sinePosX = 0;
            }
            if (sinePosY>2*M_PI) {
                sinePosY = 0;
            }
            if (sinePos>2*M_PI) {
                sinePos = 0;
            }
        }

        //0..1
        //0..3
        //0, 1, 2, 3
        //* 0.25
        //0, 0.25, 0.5, 0.75

        float tempPhaseX = std::round(orbitPhaseX*3)*0.25;
        float tempPhaseY = std::round(orbitPhaseY*3)*0.25;

        orbitX = x+getBlendedPhase(sinePosX + tempPhaseX*(2*M_PI), std::round(orbitWaveX))*(orbitSizeX/2);
        orbitY = y+getBlendedPhase(sinePosY+M_PI/2 + tempPhaseY*(2*M_PI), std::round(orbitWaveY))*(orbitSizeY/2);
        subOrbitX = smoothParameter(orbitX+getBlendedPhase(sinePos, 3)*(subOrbitSize/3), 0);
        subOrbitY = smoothParameter(orbitY+getBlendedPhase(sinePos+M_PI/2, 3)*(subOrbitSize/3), 1);
        if (orbitX<0) orbitX=0;
        if (orbitX>1) orbitX=1;
        if (orbitY<0) orbitY=0;
        if (orbitY>1) orbitY=1;

        if (subOrbitX<0) subOrbitX=0;
        if (subOrbitX>1) subOrbitX=1;
        if (subOrbitY<0) subOrbitY=0;
        if (subOrbitY>1) subOrbitY=1;
    }

    GrooveJuicePlugin();

protected:
    // -------------------------------------------------------------------
    // Information

    const char* d_getLabel() const noexcept override
    {
        return "GrooveJuice";
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
        return d_cconst('G', 'r', 'v', 'J');
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
    void d_run(float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override;

    // -------------------------------------------------------------------

private:

	CSynth synthL, synthR;
	MoogVCF filterL, filterR;

	float x, y;
	float orbitX, orbitY;
	float orbitTX, orbitTY; //targetX and targetY for interpolation
	float subOrbitX, subOrbitY;
	float subOrbitTX, subOrbitTY;
	float subOrbitSpeed, subOrbitSize, orbitSpeedX, orbitSpeedY;
	float orbitSizeX, orbitSizeY;
	float interpolationDivider;

	float bar, tickX, tickY, percentageX, percentageY, tickOffsetX, tickOffsetY;
	float sinePosX, sinePosY, tick, percentage, tickOffset, sinePos;

	float orbitWaveX, orbitWaveY, subOrbitSmooth, waveBlend;
	float orbitPhaseX, orbitPhaseY;

	//parameter smoothing, for subOrbitX and subOrbitY
	float sA[2], sB[2], sZ[2];

	float synthData[8][8]; //as per gui, [param][page]
	float synthSound[8];
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // GROOVEJUICE_HPP_INCLUDED
