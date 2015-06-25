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

#ifndef VECTORJUICEPLUGIN_HPP_INCLUDED
#define VECTORJUICEPLUGIN_HPP_INCLUDED

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class VectorJuicePlugin : public Plugin
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
        paramSubOrbitSize,
        paramSubOrbitSpeed,
        paramSubOrbitSmooth,
        paramOrbitWaveX,
        paramOrbitWaveY,
        paramOrbitPhaseX,
        paramOrbitPhaseY,
        paramOrbitOutX,
        paramOrbitOutY,
        paramSubOrbitOutX,
        paramSubOrbitOutY,
        paramCount
    };

    float smoothParameter(float in, int axis) {
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

    float tN(float x)
    {
        if (x>0) return x;
        else return 0;
    }

    void animate()
    {
        //sync orbit with frame, bpm
        const TimePosition& time(getTimePosition());
        bar = ((120.0/(time.bbt.valid ? time.bbt.beatsPerMinute : 120.0))*(getSampleRate()));

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

        float tempPhaseX = std::round(orbitPhaseX)*0.25-0.25;
        float tempPhaseY = std::round(orbitPhaseY)*0.25-0.25;

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

    VectorJuicePlugin();

protected:
    // -------------------------------------------------------------------
    // Information

    const char* getLabel() const noexcept override
    {
        return "VectorJuice";
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
        return d_cconst('V', 'e', 'c', 'J');
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
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // VECTORJUICE_HPP_INCLUDED
