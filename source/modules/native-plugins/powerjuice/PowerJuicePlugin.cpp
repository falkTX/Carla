/*
 * Power Juice Plugin
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

#include "PowerJuicePlugin.hpp"
//#include <cstring>
#include <cstdlib>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

PowerJuicePlugin::PowerJuicePlugin()
    : Plugin(paramCount, 1, 0) // 1 program, 0 states
{
    // set default values
    d_setProgram(0);

    // reset
    d_deactivate();
}

PowerJuicePlugin::~PowerJuicePlugin()
{
	free(lookaheadStack.data);
	free(RMSStack.data);
}

// -----------------------------------------------------------------------
// Init

void PowerJuicePlugin::d_initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
    case paramAttack:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Attack";
        parameter.symbol     = "att";
        parameter.unit       = "ms";
        parameter.ranges.def = 20.0f;
        parameter.ranges.min = 0.1f;
        parameter.ranges.max = 1000.0f;
        break;
    case paramRelease:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Release";
        parameter.symbol     = "rel";
        parameter.unit       = "ms";
        parameter.ranges.def = 200.0f;
        parameter.ranges.min = 0.1f;
        parameter.ranges.max = 1000.0f;
        break;
    case paramThreshold:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Threshold";
        parameter.symbol     = "thr";
        parameter.unit       = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -60.0f;
        parameter.ranges.max = 0.0f;
        break;
    case paramRatio:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Ratio";
        parameter.symbol     = "rat";
        parameter.unit       = "";
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 10.0f;
        break;
    case paramMakeup:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Make-Up";
        parameter.symbol     = "mak";
        parameter.unit       = "";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 20.0f;
        break;
    case paramMix:
        parameter.hints      = PARAMETER_IS_AUTOMABLE;
        parameter.name       = "Mix";
        parameter.symbol     = "Mix";
        parameter.unit       = "";
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;

    }
}

void PowerJuicePlugin::d_initProgramName(uint32_t index, d_string& programName)
{
    if (index != 0)
        return;

    programName = "Default";
}

// -----------------------------------------------------------------------
// Internal data

float PowerJuicePlugin::d_getParameterValue(uint32_t index) const
{
    switch (index)
    {
    case paramAttack:
        return attack;
    case paramRelease:
        return release;
    case paramThreshold:
        return threshold;
    case paramRatio:
        return ratio;
    case paramMakeup:
        return makeup;
    case paramMix:
        return mix;
    default:
        return 0.0f;
    }
}

void PowerJuicePlugin::d_setParameterValue(uint32_t index, float value)
{
    switch (index)
    {
    case paramAttack:
        attack = value;
        attackSamples = d_getSampleRate()*(attack/1000.0f);
        break;
    case paramRelease:
        release = value;
        releaseSamples = d_getSampleRate()*(release/1000.0f);
        break;
    case paramThreshold:
        threshold = value;
        break;
    case paramRatio:
        ratio = value;
        break;
    case paramMakeup:
        makeup = value;
        makeupFloat = fromDB(makeup);
        break;
    case paramMix:
        mix = value;
        break;
    }
}

void PowerJuicePlugin::d_setProgram(uint32_t index)
{
    if (index != 0)
        return;

    /* Default parameter values */
    attack = 20.0f;
    release = 200.0f;
    threshold = 0.0f;
    ratio = 1.0f;
    makeup = 0.0f;
    mix = 1.0f;

    makeupFloat = fromDB(makeup);
    attackSamples = d_getSampleRate()*(attack/1000.0f);
    releaseSamples = d_getSampleRate()*(release/1000.0f);

	

    w = 563; //waveform plane size, size of the plane in pixels;
    w2 = 1126; //wavefowm array
    h = 121; //waveform plane height
    x = 27; //waveform plane positions
    y = 53;
    dc = 113; //0DC line y position
	
    /* Default variable values */
    averageCounter = 0;
    inputMax = 0.0f;

    balancer = 1.0f;
    GR = 1.0f;

	newRepaint = false;

    input.start = 0;
    rms.start = 0;
    gainReduction.start = 0;
    RMSStack.start = 0;
    lookaheadStack.start = 0;
    repaintSkip = 0;
    
    
    kFloatRMSStackCount = 400.0f/44100.0f*d_getSampleRate();
    RMSStack.data = (float*) calloc(kFloatRMSStackCount, sizeof(float));
    
    kFloatLookaheadStackCount = 800.0f/44100.0f*d_getSampleRate();
    lookaheadStack.data = (float*) calloc(kFloatLookaheadStackCount, sizeof(float));
    
    refreshSkip= 300.0f/44100.0f*d_getSampleRate();
    
    std::memset(rms.data, 0, sizeof(float)*kFloatStackCount);
    std::memset(gainReduction.data, 0, sizeof(float)*kFloatStackCount);
    std::memset(RMSStack.data, 0, sizeof(float)*kFloatRMSStackCount);
    std::memset(lookaheadStack.data, 0, sizeof(float)*kFloatLookaheadStackCount);
    
	for (int j=0; j < kFloatStackCount; ++j)
		history.rms[j] = h +y;
	for (int j=0; j < kFloatStackCount; ++j) 
		history.gainReduction[j] = h +y;

    d_activate();
    
}

float PowerJuicePlugin::getRMSHistory(int n) {
    return history.rms[n];
}

bool PowerJuicePlugin::repaintNeeded() {
	return newRepaint;
}

float PowerJuicePlugin::getGainReductionHistory(int n) {
	if (n == kFloatStackCount-1) {
		newRepaint = false;
		//printf("falsing!\n");
	}
    return history.gainReduction[n];
}

// -----------------------------------------------------------------------
// Process

void PowerJuicePlugin::d_activate()
{
}

void PowerJuicePlugin::d_deactivate()
{
    // all values to zero
}

void PowerJuicePlugin::d_run(const float** inputs, float** outputs, uint32_t frames)
{
    const float* in = inputs[0];
    float* out = outputs[0];
    float sum;
    float data;
    float difference;


    for (uint32_t i=0; i < frames; i++) {

        sum = 0.0f;
        data = 0.0f;
        difference = 0;
        //sanitizeDenormal(in[i]); // FIXME - you cannot modify inputs

        /*   compute last RMS   */

        //store audio samples in an RMS buffer line
        RMSStack.data[RMSStack.start++] = in[i];
	   
        if (RMSStack.start == kFloatRMSStackCount)
                RMSStack.start = 0;
        //compute RMS over last kFloatRMSStackCount samples
        for (int j=0; j < kFloatRMSStackCount; ++j) {
            data = RMSStack.data[(RMSStack.start+j) % kFloatRMSStackCount];
            sum += data * data;
        }
	   
        //root mean SQUARE
          float RMS = sqrt(sum / kFloatRMSStackCount);
        sanitizeDenormal(RMS);
		

        /*   compute gain reduction if needed   */
        float RMSDB = toDB(RMS);
	   
        if (RMSDB>threshold) {
            //attack stage
            float difference = (RMSDB-threshold);
		  
            //sanitizeDenormal(difference);
            targetGR = difference - difference/ratio;
            if (targetGR>difference/(ratio/4.0f)) {
                targetGR = difference - difference/(ratio*1.5f);
                //more power!
            }
		  //
            if (GR<targetGR) {
                //approach targetGR at attackSamples rate
                GR -= (GR-targetGR)/(attackSamples);
            } else {
                //approach targetGR at releaseSamples rate
                GR -= (GR-targetGR)/releaseSamples;
            }
		  
            sanitizeDenormal(GR);
        } else {
            //release stage
            //approach targetGR at releaseSamples rate, targetGR = 0.0f
            GR -= GR/releaseSamples;
        }
		
        //store audio in lookahead buffer
	   
        lookaheadStack.data[lookaheadStack.start++] = in[i];
	   //printf("rms\n");
        if (lookaheadStack.start == kFloatLookaheadStackCount)
                lookaheadStack.start = 0;
		
        if (++averageCounter >= refreshSkip) {
		  
            //add relevant values to the shared memory
            rms.data[rms.start++] = RMSDB;
            gainReduction.data[gainReduction.start++] = GR;
		  

            //rewind stack reading heads if needed
            if (rms.start == kFloatStackCount)
                rms.start = 0;
            if (gainReduction.start == kFloatStackCount)
                gainReduction.start = 0;


            //saving in gfx format, for speed
            //share memory

            for (int j=0; j < kFloatStackCount; ++j)
                history.rms[j] = -toIEC(rms.data[(rms.start+j) % kFloatStackCount])/200*h +h +y;
            for (int j=0; j < kFloatStackCount; ++j) {
                history.gainReduction[j] = -toIEC(-gainReduction.data[(gainReduction.start+j) % kFloatStackCount])/200*h +h +y;
			
		  }
			
			repaintSkip++;
			if (repaintSkip>5) {
				repaintSkip = 0;
				newRepaint = true;
			}
			
            averageCounter = 0;
            inputMax = 0.0f;
        }

        /* compress, mix, done. */
        float compressedSignal = in[i]*fromDB(-GR);
        out[i] = (compressedSignal*makeupFloat*mix)+in[i]*(1-mix);
    }
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new PowerJuicePlugin();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
