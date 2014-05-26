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

#include "GrooveJuicePlugin.hpp"




START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

GrooveJuicePlugin::GrooveJuicePlugin()
    : Plugin(paramCount, 1, 0) // 1 program, 0 states
{
    // set default values
    d_setProgram(0);

    // reset
    d_deactivate();
}

// -----------------------------------------------------------------------
// Init

void GrooveJuicePlugin::d_initParameter(uint32_t index, Parameter& parameter)
{
	switch (index)
	{
	    case paramX:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "X";
		   parameter.symbol     = "x";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;

	    case paramY:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "Y";
		   parameter.symbol     = "y";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;

	    case paramOrbitSpeedX:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "Orbit Speed X";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 4.0f;
		   parameter.ranges.min = 1.0f;
		   parameter.ranges.max = 128.0f;
		   break;

	    case paramOrbitSpeedY:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "Orbit Speed Y";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 4.0f;
		   parameter.ranges.min = 1.0f;
		   parameter.ranges.max = 128.0f;
		   break;

	    case paramOrbitSizeX:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "Orbit Size X";
		   parameter.symbol     = "osl";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;

	    case paramOrbitSizeY:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "Orbit Size Y";
		   parameter.symbol     = "osr";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;

	    case paramSubOrbitSpeed:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "SubOrbit Speed";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 32.0f;
		   parameter.ranges.min = 1.0f;
		   parameter.ranges.max = 128.0f;
		   break;

	    case paramSubOrbitSize:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "SubOrbit Size";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;

	    case paramSubOrbitSmooth:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "SubOrbit Wave";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;

	    case paramOrbitWaveX:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "Orbit Wave X";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 3.0f;
		   parameter.ranges.min = 1.0f;
		   parameter.ranges.max = 4.0f;
		   break;

	    case paramOrbitPhaseX:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "Orbit Phase X";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.0f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;

	    case paramOrbitPhaseY:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "Orbit Phase Y";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.0f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;

	    case paramOrbitWaveY:
		   parameter.hints      = PARAMETER_IS_AUTOMABLE;
		   parameter.name       = "Orbit Wave Y";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 3.0f;
		   parameter.ranges.min = 1.0f;
		   parameter.ranges.max = 4.0f;
		   break;

	    case paramOrbitOutX:
		   parameter.hints      = PARAMETER_IS_OUTPUT;
		   parameter.name       = "Orbit X";
		   parameter.symbol     = "orx";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;

	    case paramOrbitOutY:
		   parameter.hints      = PARAMETER_IS_OUTPUT;
		   parameter.name       = "Orbit Y";
		   parameter.symbol     = "ory";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;

	    case paramSubOrbitOutX:
		   parameter.hints      = PARAMETER_IS_OUTPUT;
		   parameter.name       = "SubOrbit X";
		   parameter.symbol     = "sorx";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;

	    case paramSubOrbitOutY:
		   parameter.hints      = PARAMETER_IS_OUTPUT;
		   parameter.name       = "SubOrbit Y";
		   parameter.symbol     = "sory";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;
   
		case paramW1Out:
		   parameter.hints      = PARAMETER_IS_OUTPUT;
		   parameter.name       = "";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;
  
		case paramW2Out:
		   parameter.hints      = PARAMETER_IS_OUTPUT;
		   parameter.name       = "";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;
		
		case paramMOut:
		   parameter.hints      = PARAMETER_IS_OUTPUT;
		   parameter.name       = "";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;
		  
		case paramCOut:
		   parameter.hints      = PARAMETER_IS_OUTPUT;
		   parameter.name       = "";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;
		 
		case paramROut:
		   parameter.hints      = PARAMETER_IS_OUTPUT;
		   parameter.name       = "";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;
		   
		case paramSOut:
		   parameter.hints      = PARAMETER_IS_OUTPUT;
		   parameter.name       = "";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;
		   
		case paramReOut:
		   parameter.hints      = PARAMETER_IS_OUTPUT;
		   parameter.name       = "";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;
		   
		case paramShOut:
		   parameter.hints      = PARAMETER_IS_OUTPUT;
		   parameter.name       = "";
		   parameter.symbol     = "";
		   parameter.unit       = "";
		   parameter.ranges.def = 0.5f;
		   parameter.ranges.min = 0.0f;
		   parameter.ranges.max = 1.0f;
		   break;
    }
    
    if (index>=17 && index<17+64) {
		//int x = (index-17)%8;
		//int y = (x+1)/(index-17)
		
		parameter.hints      = PARAMETER_IS_AUTOMABLE;
		parameter.name       = "synth";
		parameter.symbol     = "";
		parameter.unit       = "";
		parameter.ranges.def = 0.5f;
		parameter.ranges.min = 0.0f;
		parameter.ranges.max = 1.0f;
    }
}

void GrooveJuicePlugin::d_initProgramName(uint32_t index, d_string& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

// -----------------------------------------------------------------------
// Internal data


float GrooveJuicePlugin::d_getParameterValue(uint32_t index) const
{
	if (index<17 || index>=17+64) {
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
			case paramSubOrbitSpeed:
				return subOrbitSpeed;
			case paramSubOrbitSize:
				return subOrbitSize;
			case paramOrbitOutX:
				return orbitX;
			case paramOrbitOutY:
				return orbitY;
			case paramSubOrbitOutX:
				return subOrbitX;
			case paramSubOrbitOutY:
				return subOrbitY;
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
			case paramW1Out:
				return synthSound[0];
			case paramW2Out:
				return synthSound[1];
			case paramMOut:
				return synthSound[2];
			case paramCOut:
				return synthSound[3];
			case paramROut:
				return synthSound[4];
			case paramSOut:
				return synthSound[5];
			case paramReOut:
				return synthSound[6];
			case paramShOut:
				return synthSound[7];
	    default:
		   return 0.0f;
	    }
    
    } else {
		int num = (index-17); //synth params begin on #17
		int x = num%8; //synth param
		//synth page
		int y = (num-(num%8))/8;
		return synthData[x][y];
    }
}

void GrooveJuicePlugin::d_setParameterValue(uint32_t index, float value)
{

	if (index<17) {
	    switch (index)
	    {
	    case paramX:
		   x = value;
		   break;
	    case paramY:
		   y = value;
		   break;
	    case paramOrbitSpeedX:
		   orbitSpeedX = value;
		   resetPhase();
		   break;
	    case paramOrbitSpeedY:
		   orbitSpeedY = value;
		   resetPhase();
		   break;
	    case paramOrbitSizeX:
		   orbitSizeX = value;
		   break;
	    case paramOrbitSizeY:
		   orbitSizeY = value;
		   break;
	    case paramSubOrbitSpeed:
		   subOrbitSpeed = value;
		   resetPhase();
		   break;
	    case paramSubOrbitSize:
		   subOrbitSize = value;
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
		   resetPhase();
		   break;
	    case paramOrbitPhaseY:
		   orbitPhaseY = value;
		   resetPhase();
		   break;
	    }
    
    } else  {
		
		int num = (index-17); //synth params begin on #17
		int x = num%8; //synth param
		//synth page
		int y = (num-(num%8))/8;
		synthData[x][y] = value;
    }
    
}

void GrooveJuicePlugin::d_setProgram(uint32_t index)
{
    if (index != 0)
        return;

    /* Default parameter values */
    x = 0.5f;
    y = 0.5f;
    orbitSpeedX = 4.0f;
    orbitSpeedY = 4.0f;
    orbitSizeX = 0.5f;
    orbitSizeY = 0.5f;
    subOrbitSize = 0.5f;
    subOrbitSpeed = 32.0f;
    orbitWaveX = 3.0f;
    orbitWaveY = 3.0f;
    subOrbitSmooth = 0.5f;
    orbitPhaseX = 0.0f;
    orbitPhaseY = 0.0f;

    /* Default variable values */
    orbitX=orbitY=orbitTX=orbitTY=0.5;
    subOrbitX=subOrbitY=subOrbitTX=subOrbitTY=0;
    interpolationDivider=200;
    bar=tickX=tickY=percentageX=percentageY=tickOffsetX=0;
    tickOffsetY=sinePosX=sinePosY=tick=percentage=tickOffset=sinePos=0;
    waveBlend=0;

	synthL.setSampleRate(d_getSampleRate());
	synthR.setSampleRate(d_getSampleRate());

    //parameter smoothing
    for (int i=0; i<2; i++) {
        sA[i] = 0.99f;
        sB[i] = 1.f - sA[i];
        sZ[i] = 0;
    }
    
	for (int x=0; x<8; x++)
		for (int y=0; y<8; y++)
			synthData[x][y] = 0.5;

    /* reset filter values */
    d_activate();
}

// -----------------------------------------------------------------------
// Process

void GrooveJuicePlugin::d_activate()
{
    //sinePos = 0;
}

void GrooveJuicePlugin::d_deactivate()
{
    // all values to zero
}

void GrooveJuicePlugin::d_run(const float**, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount)
{
    float out1, out2, tX, tY;
        
		/*
        out1 = inputs[0][i]*tN(1-std::sqrt((tX*tX)+(tY*tY)));
        out2 = inputs[1][i]*tN(1-std::sqrt((tX*tX)+(tY*tY)));

        out1 += inputs[2][i]*tN(1-std::sqrt(((1-tX)*(1-tX))+(tY*tY)));
        out2 += inputs[3][i]*tN(1-std::sqrt(((1-tX)*(1-tX))+(tY*tY)));

        out1 += inputs[4][i]*tN(1-std::sqrt(((1-tX)*(1-tX))+((1-tY)*(1-tY))));
        out2 += inputs[5][i]*tN(1-std::sqrt(((1-tX)*(1-tX))+((1-tY)*(1-tY))));

        out1 += inputs[6][i]*tN(1-std::sqrt((tX*tX)+((1-tY)*(1-tY))));
        out2 += inputs[7][i]*tN(1-std::sqrt((tX*tX)+((1-tY)*(1-tY))));
	   */
   
   for (uint32_t i = 0; i<frames; i++) {
	animate();	
	tX = subOrbitX;
	tY = subOrbitY;
	//sum values
	float c = 0.41421f; //mid segment
	float d = c/1.41f; //side segments
	float distances[8];
	distances[0] = distance(d, 0, tX, tY);
	distances[1] = distance(c+d, 0, tX, tY);
	distances[2] = distance(1, d, tX, tY);
	distances[3] = distance(1, c+d, tX, tY);
	distances[4] = distance(d, 1, tX, tY);
	distances[5] = distance(c+d, 1, tX, tY);
	distances[6] = distance(0, d, tX, tY);
	distances[7] = distance(0, c+d, tX, tY);
	
	//std::cout << distances[0] << " ";
	//std::cout << tX << std::endl;
	
	for (int x=0; x<8; x++) {
		float targetValue = 0;
		for (int y=0; y<8; y++) {
			targetValue += synthData[x][y]*(distances[y]);
		}
		synthSound[x] = tN(targetValue);
	}
	
	//printf("wave1: %f\n", synthSound[0]);
	
	//std::cout << synthSound[0] << std::endl;
	
	synthL.setWave(0, synthSound[0]);
	synthR.setWave(0, synthSound[0]);
	synthL.setWave(1, synthSound[1]);
	synthR.setWave(1, synthSound[1]);
	synthL.setMix(synthSound[2]);
	synthR.setMix(synthSound[2]);
	//synthL.setCut(synthSound[3]);
	//synthR.setCut(synthSound[3]);
	//synthL.setReso(synthSound[4]);
	//synthR.setReso(synthSound[4]);
	//synthL.setDecay(synthSound[5]);
	//synthR.setDecay(synthSound[5]);
	synthL.setSustain(synthSound[5]);
	synthR.setSustain(synthSound[5]);
	synthL.setRelease(synthSound[6]);
	synthR.setRelease(synthSound[6]);
	synthL.setStereo(-0.5);
	synthR.setStereo(0.5);
	synthL.setShape(synthSound[7]);
	synthR.setShape(synthSound[7]);
	
	float cutoff = std::exp((std::log(16000)-std::log(500))*synthSound[3]+std::log(500));
	
	filterL.recalc(cutoff, synthSound[4]*0.8, d_getSampleRate(), synthSound[7]);
	filterR.recalc(cutoff, synthSound[4]*0.8, d_getSampleRate(), synthSound[7]);
	
	outputs[0][i] = filterL.process(synthL.run());
	outputs[1][i] = filterR.process(synthL.run());

   }
   
   
   //outputs = buffer;
   
   for (uint32_t i = 0; i<midiEventCount; i++) {
        int mType = midiEvents[i].buf[0] & 0xF0;
        //int mChan = midiEvents[i].buf[0] & 0x0F;
        int mNum = midiEvents[i].buf[1];
        if (mType == 0x90) {
            //note on
            synthL.play(mNum);
            synthR.play(mNum);
        } else if (mType == 0x80) {
            //note off
            synthL.stop(mNum);
            synthR.stop(mNum);
        }
    }
   
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new GrooveJuicePlugin();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
