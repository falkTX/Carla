/*
 * Segment Synthesizer Design
 * Implemented by Andre Sklenar <andre.sklenar@gmail.com>, www.juicelab.cz .
 *
 * Copyright (C) 2014 Andre Sklenar <andre.sklenar@gmail.com>, www.juicelab.cz
 */

#ifndef SYNTH_HXX_INCLUDED
#define SYNTH_HXX_INCLUDED

#include <cmath>
#include <cstdlib>


class CSynth
{
public:
    CSynth()
    {
		waveBlend = 0;
		freq = 0;
		phaseAccum = 0;
		playing = false;
		cycleSize = 0;
		sampleRate = 0;
		notePlaying = 0;
		env = 0;
		envPhase = 0;
		aCoeff=dCoeff=rCoeff=0;
		stereo = 0;
		shape = 0.5;
		
		
		sustain=release=0.5f;
		attack=0.1f;
		sustain=0.0f;
		mAmp = 0.5;
		freqOffset = 0;
		waves[0] = 0;
		waves[1] = 0;
		srand (static_cast <unsigned> (time(0)));
    }

	float getSinePhase(float x) {
        return -(std::sin(x));
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

	void setWave(int n, float nWave) {
		waves[n] = nWave;
	}


	void setStereo(float nStereo) {
		stereo = nStereo;
		if (playing) {
			freq = 440.0 * pow(2.0, (notePlaying - 69)/12);
			freq += freq*(stereo/12);
		}
	}
	
	float getRandom() {
		return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	}

	void setSustain(float nSustain) {
		sustain = nSustain*4;
	}
	
	void setDecay(float nDecay) {
		decay = nDecay*sampleRate;
	}

	void setRelease(float nRelease) {
		release = nRelease*sampleRate/4;
	}
	
	void setShape(float nShape) {
		shape = nShape;
	}
	
	void setMix(float nMix) {
		mix = nMix;
	}
	
	void setCut(float nCut) {
		cut = nCut;
	}
	
	void setReso(float nReso) {
		reso = nReso;
	}

	float getWave(int i) {
		return waves[i];
	}

	void stop(int note) {
		if (note==notePlaying) {
			envPhase = 2; //release
		}
	}

	void play(float note) {
		notePlaying = note;
		freq = 440.0 * pow(2.0, (notePlaying - 69)/12);
		freq += freq*(freqOffset);
		freq += freq*(stereo/12);
		cycleSize = sampleRate/(freq/4);
		playing = true;
		phaseAccum = getRandom()*cycleSize;
		//phaseAccum = 0;
		env = 0.0f;
		envPhase = 0;
		
	
		//env init
		decay = 1*sampleRate;
		aCoeff = 1.0f - expf(-1/attack*16);
		dCoeff = 1.0f - expf(-1/(decay/8));
		rCoeff = 1.0f - expf(-1/(release));
	}

	float run() {
		float out = 0;
		
		if (playing) {

			out = getSinePhase((phaseAccum/cycleSize)*(2*M_PI))*0.5;
			//out += getSinePhase((phaseAccum/cycleSize)*2*(2*M_PI))*amps[0]*0.5;
			//out = getSinePhase((phaseAccum/cycleSize)*2*M_PI);
			//printf("out: %f\n", (waves[0]*3));
			//printf("out: %f\n", phaseAccum);
			out += getBlendedPhase((phaseAccum/cycleSize)*2*M_PI, (waves[0]*3)+1);
			out += getBlendedPhase((phaseAccum/cycleSize)*4*M_PI, (waves[0]*3)+1)*(sustain/4);
			out += getBlendedPhase((phaseAccum/cycleSize)*8*M_PI, (waves[0]*3)+1)*(sustain/4);
			out += getBlendedPhase((phaseAccum/cycleSize)*16*M_PI, (waves[0]*3)+1)*(sustain/4)/2;
			out += getBlendedPhase((phaseAccum/cycleSize)*6*M_PI, (waves[0]*3)+1)*shape;
			out += getBlendedPhase((phaseAccum/cycleSize)*12*M_PI, (waves[0]*3)+1)*(1-shape)/2;
			out += getBlendedPhase((phaseAccum/cycleSize)*3*M_PI, (waves[0]*3)+1)*(1-mix);
			out += getBlendedPhase((phaseAccum/cycleSize)*4*M_PI, (waves[1]*3)+1);
			out += getBlendedPhase((phaseAccum/cycleSize)*2*M_PI*(getBlendedPhase((phaseAccum/cycleSize)*M_PI, (waves[1]*3)+1)), (waves[1]*3)+1)*mix;
			//printf("out: %f\n", waves[0]*4);
			//std::cout << (waves[0]*3)+1 << std::endl;
			phaseAccum++;
			if (phaseAccum>cycleSize) {
				phaseAccum = 0;
			}

			//calculate amplitude envelope
			if (envPhase==0) {
				
				//attack phase
				env+=aCoeff * ((1.0/0.63) - env);
	
				if (env>1.0) envPhase+=2;
			} else if (envPhase==1) {
				//decay and sustain phase

				env+=dCoeff * (sustain/4 - env);
			} else {
				//release phase
				env += rCoeff * (1.0-(1.0/0.63) - env);
				if (env<0.0) { playing = false; env = 0.0; }
			}
			//apply amplitude envelope
			out*=env;

			//apply master volume
			//out*=mAmp*0.5;
			//apply hard clipping
			//if (out>1) out=1;
			//if (out<-1) out=-1;
			if (out!=0) {
				//printf("out: %f\n", out);
			}
		}
		
		return out;
	}
	
	void setSampleRate(float sr) {
		sampleRate = sr;
		attack = sampleRate/2;
		decay = sampleRate/2;
		release = sampleRate/2;
	}


private:
    /* vcf filter */
    float waves[2];
	float waveBlend;
	float freq;
	float freqOffset;
	float stereo;
	float mAmp;
	float phaseAccum;
	float cycleSize;
	float sampleRate;
	float notePlaying;
	bool playing;
	float mix;
	float reso;
	float cut;
	float shape;

	float attack, decay, sustain, release;
	float env;
	float envPhase; //0, 1(d+s), 2
	float aCoeff, dCoeff, rCoeff, gCoeff;
};

#endif // SYNTH_HXX_INCLUDED
