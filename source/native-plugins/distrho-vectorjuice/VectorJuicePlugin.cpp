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

#include "VectorJuicePlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

VectorJuicePlugin::VectorJuicePlugin()
    : Plugin(paramCount, 1, 0) // 1 program, 0 states
{
    // set default values
    loadProgram(0);
}

// -----------------------------------------------------------------------
// Init

void VectorJuicePlugin::initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
    case paramX:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "X";
        parameter.symbol     = "x";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;

    case paramY:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "Y";
        parameter.symbol     = "y";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;

    case paramOrbitSizeX:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "Orbit Size X";
        parameter.symbol     = "sizex";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;

    case paramOrbitSizeY:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "Orbit Size Y";
        parameter.symbol     = "sizey";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;

    case paramOrbitSpeedX:
        parameter.hints      = kParameterIsAutomable|kParameterIsInteger;
        parameter.name       = "Orbit Speed X";
        parameter.symbol     = "speedx";
        parameter.ranges.def = 4.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 128.0f;
        break;

    case paramOrbitSpeedY:
        parameter.hints      = kParameterIsAutomable|kParameterIsInteger;
        parameter.name       = "Orbit Speed Y";
        parameter.symbol     = "speedy";
        parameter.ranges.def = 4.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 128.0f;
        break;

    case paramSubOrbitSize:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "SubOrbit Size";
        parameter.symbol     = "subsize";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;

    case paramSubOrbitSpeed:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "SubOrbit Speed";
        parameter.symbol     = "subspeed";
        parameter.ranges.def = 32.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 128.0f;
        break;

    case paramSubOrbitSmooth:
        parameter.hints      = kParameterIsAutomable;
        parameter.name       = "SubOrbit Wave";
        parameter.symbol     = "subwave";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;

    case paramOrbitWaveX:
        parameter.hints      = kParameterIsAutomable|kParameterIsInteger;
        parameter.name       = "Orbit Wave X";
        parameter.symbol     = "wavex";
        parameter.ranges.def = 3.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 4.0f;
        break;

    case paramOrbitWaveY:
        parameter.hints      = kParameterIsAutomable|kParameterIsInteger;
        parameter.name       = "Orbit Wave Y";
        parameter.symbol     = "wavey";
        parameter.ranges.def = 3.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 4.0f;
        break;

    case paramOrbitPhaseX:
        parameter.hints      = kParameterIsAutomable|kParameterIsInteger;
        parameter.name       = "Orbit Phase X";
        parameter.symbol     = "phasex";
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 4.0f;
        break;

    case paramOrbitPhaseY:
        parameter.hints      = kParameterIsAutomable|kParameterIsInteger;
        parameter.name       = "Orbit Phase Y";
        parameter.symbol     = "phasey";
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 4.0f;
        break;

    case paramOrbitOutX:
        parameter.hints      = kParameterIsOutput;
        parameter.name       = "Orbit X";
        parameter.symbol     = "orx";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;

    case paramOrbitOutY:
        parameter.hints      = kParameterIsOutput;
        parameter.name       = "Orbit Y";
        parameter.symbol     = "ory";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;

    case paramSubOrbitOutX:
        parameter.hints      = kParameterIsOutput;
        parameter.name       = "SubOrbit X";
        parameter.symbol     = "sorx";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;

    case paramSubOrbitOutY:
        parameter.hints      = kParameterIsOutput;
        parameter.name       = "SubOrbit Y";
        parameter.symbol     = "sory";
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    }
}

void VectorJuicePlugin::initProgramName(uint32_t index, String& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

// -----------------------------------------------------------------------
// Internal data

float VectorJuicePlugin::getParameterValue(uint32_t index) const
{
    switch (index)
    {
    case paramX:
        return x;
    case paramY:
        return y;
    case paramOrbitSizeX:
        return orbitSizeX;
    case paramOrbitSizeY:
        return orbitSizeY;
    case paramOrbitSpeedX:
        return orbitSpeedX;
    case paramOrbitSpeedY:
        return orbitSpeedY;
    case paramSubOrbitSize:
        return subOrbitSize;
    case paramSubOrbitSpeed:
        return subOrbitSpeed;
    case paramSubOrbitSmooth:
        return subOrbitSmooth;
    case paramOrbitWaveX:
        return orbitWaveX;
    case paramOrbitWaveY:
        return orbitWaveY;
    case paramOrbitPhaseX:
        return orbitPhaseY;
    case paramOrbitPhaseY:
        return orbitPhaseY;
    case paramOrbitOutX:
        return orbitX;
    case paramOrbitOutY:
        return orbitY;
    case paramSubOrbitOutX:
        return subOrbitX;
    case paramSubOrbitOutY:
        return subOrbitY;
    default:
        return 0.0f;
    }
}

void VectorJuicePlugin::setParameterValue(uint32_t index, float value)
{
    bool resetPhase = false;

    switch (index)
    {
    case paramX:
        x = value;
        break;
    case paramY:
        y = value;
        break;
    case paramOrbitSizeX:
        orbitSizeX = value;
        break;
    case paramOrbitSizeY:
        orbitSizeY = value;
        break;
    case paramOrbitSpeedX:
        orbitSpeedX = value;
        resetPhase = true;
        break;
    case paramOrbitSpeedY:
        orbitSpeedY = value;
        resetPhase = true;
        break;
    case paramSubOrbitSize:
        subOrbitSize = value;
        break;
    case paramSubOrbitSpeed:
        subOrbitSpeed = value;
        resetPhase = true;
        break;
    case paramSubOrbitSmooth:
        subOrbitSmooth = value;
        break;
    case paramOrbitWaveX:
        orbitWaveX = value;
        break;
    case paramOrbitWaveY:
        orbitWaveY = value;
        break;
    case paramOrbitPhaseX:
        orbitPhaseX = value;
        resetPhase = true;
        break;
    case paramOrbitPhaseY:
        orbitPhaseY = value;
        resetPhase = true;
        break;
    }

    if (resetPhase)
    {
        sinePosX = 0;
        sinePosY = 0;
        sinePos = 0;
    }
}

void VectorJuicePlugin::loadProgram(uint32_t index)
{
    if (index != 0)
        return;

    /* Default parameter values */
    x = 0.5f;
    y = 0.5f;
    orbitSizeX = 0.5f;
    orbitSizeY = 0.5f;
    orbitSpeedX = 4.0f;
    orbitSpeedY = 4.0f;
    subOrbitSize = 0.5f;
    subOrbitSpeed = 32.0f;
    subOrbitSmooth = 0.5f;
    orbitWaveX = 3.0f;
    orbitWaveY = 3.0f;
    orbitPhaseX = 1.0f;
    orbitPhaseY = 1.0f;

    /* reset filter values */
    activate();
}

// -----------------------------------------------------------------------
// Process

void VectorJuicePlugin::activate()
{
    /* Default variable values */
    orbitX=orbitY=orbitTX=orbitTY=0.5;
    subOrbitX=subOrbitY=subOrbitTX=subOrbitTY=0;
    interpolationDivider=200;
    bar=tickX=tickY=percentageX=percentageY=tickOffsetX=0;
    tickOffsetY=sinePosX=sinePosY=tick=percentage=tickOffset=sinePos=0;
    waveBlend=0;

    //parameter smoothing
    for (int i=0; i<2; i++) {
        sA[i] = 0.99f;
        sB[i] = 1.f - sA[i];
        sZ[i] = 0;
    }
}

void VectorJuicePlugin::run(const float** inputs, float** outputs, uint32_t frames)
{
    float out1, out2, tX, tY;

    for (uint32_t i=0; i<frames; i++) {
        //1.41421 -> 1
        //<0 = 0

        animate();

        tX = subOrbitX;
        tY = subOrbitY;

        out1 = inputs[0][i]*tN(1-std::sqrt((tX*tX)+(tY*tY)));
        out2 = inputs[1][i]*tN(1-std::sqrt((tX*tX)+(tY*tY)));

        out1 += inputs[2][i]*tN(1-std::sqrt(((1-tX)*(1-tX))+(tY*tY)));
        out2 += inputs[3][i]*tN(1-std::sqrt(((1-tX)*(1-tX))+(tY*tY)));

        out1 += inputs[4][i]*tN(1-std::sqrt(((1-tX)*(1-tX))+((1-tY)*(1-tY))));
        out2 += inputs[5][i]*tN(1-std::sqrt(((1-tX)*(1-tX))+((1-tY)*(1-tY))));

        out1 += inputs[6][i]*tN(1-std::sqrt((tX*tX)+((1-tY)*(1-tY))));
        out2 += inputs[7][i]*tN(1-std::sqrt((tX*tX)+((1-tY)*(1-tY))));

        outputs[0][i] = out1;
        outputs[1][i] = out2;
    }
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new VectorJuicePlugin();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
