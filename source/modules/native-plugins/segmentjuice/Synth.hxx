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
        for (int i=0; i<6; i++) {
			waves[i] = 3.0f;
		}
		for (int i=0; i<6; i++) {
			FMs[i] = 0.5f*64;
		}
		for (int i=0; i<6; i++) {
			pans[i] = 1.0f;
		}
		for (int i=0; i<6; i++) {
			amps[i] = 0.5f;
		}
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
		tFreq=0;
		gl = 0;
		glideDistance = 0;
		
		
		attack=decay=sustain=release=0.0f;
		mAmp = 0.5;
		freqOffset = 0;
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
    float getBlendedPhase(float x, float wave, float polarity)
    {
        //wave = 2;
        if (wave>=1 && wave<2) {
            /* saw vs sqr */
            waveBlend = wave-1;
            return (getSawPhase(x)*(1-waveBlend) + getSquarePhase(x)*waveBlend)*polarity;
        } else if (wave>=2 && wave<3) {
            /* sqr vs sin */
            waveBlend = wave-2;
            return (getSquarePhase(x)*(1-waveBlend) + getSinePhase(x)*waveBlend)*polarity;
        } else if (wave>=3 && wave<=4) {
            /* sin vs revSaw */ 
            waveBlend = wave-3;
            return (getSinePhase(x)*(1-waveBlend) + getRevSawPhase(x)*waveBlend)*polarity;
        } else {
            return 0.0f;
        }
    }

	void setWave(int i, float nWave) {
		waves[i] = nWave;
	}

	void setFM(int i, float nFM) {
		FMs[i] = nFM*64;
	}

	void setPan(int i, float nPan) {
		pans[i] = (nPan+1)/2;
	}

	void setAmp(int i, float nAmp) {
		amps[i] = nAmp;
	}

	void setMAmp(float nMAmp) {
		mAmp = nMAmp;
	}

	void setStereo(float nStereo) {
		stereo = nStereo;
		if (playing) {
			freq = 440.0 * pow(2.0, (notePlaying - 69)/12);
			freq += freq*(stereo/12);
		}
	}

	void setTune(float nTune) {
		freqOffset = nTune;
		if (playing) {
			freq = 440.0 * pow(2.0, (notePlaying - 69)/12);
			freq +=freq*freqOffset;
			freq += freq*(stereo/12);
			tFreq = freq;
			//gl = 1;
			//freq += freq*freqOffset;
			glideDistance = 0;
			cycleSize = sampleRate/(freq/4);
			//glide init
			//gCoeff = 1.0f - expf(-1/glide);
		}
	}

	void setAttack(float nAttack) {
		attack = nAttack*sampleRate*4;
	}

	void setDecay(float nDecay) {
		decay = nDecay*sampleRate;
	}

	void setSustain(float nSustain) {
		sustain = nSustain*4;
	}

	void setRelease(float nRelease) {
		release = nRelease*sampleRate;
	}

	void setGlide(float nGlide) {
		glide = nGlide*sampleRate;
	}

	float getWave(int i) {
		return waves[i];
	}

	float getFM(int i) {
		return FMs[i];
	}

	float getPan(int i) {
		return (pans[i]*2)-1;
	}
	
	float getAmp(int i) {
		return amps[i];
	}

	void stop(int note) {
		if (note==notePlaying) {
			envPhase = 2; //release
		}
	}

	void play(float note) {
		if (!playing || (playing && notePlaying==note)) {
			notePlaying = note;
			freq = 440.0 * pow(2.0, (notePlaying - 69)/12);
			freq += freq*(freqOffset);
			freq += freq*(stereo/12);
			cycleSize = sampleRate/(freq/4);
			tFreq = freq;
			playing = true;
			phaseAccum = 0;
			env = 0.0f;
			envPhase = 0;
		
			//env init
			aCoeff = 1.0f - expf(-1/attack);
			dCoeff = 1.0f - expf(-1/decay);
			rCoeff = 1.0f - expf(-1/release);
		} else {
			std::cout << "not playing" << std::endl; 
			//glide towards the target freq
			notePlaying = note;
			tFreq = 440.0 * pow(2.0, (notePlaying - 69)/12);
			tFreq += tFreq*(freqOffset);
			tFreq += tFreq*(stereo/12);
			glideDistance = tFreq-freq;
			gl = 0;
			if (envPhase == 2) {
				envPhase = 0;

			}

			//glide init
			gCoeff = 1.0f - expf(-1/glide);
		}
		
	}

	float run() {
		float out = 0;
		
		if (playing) {
			if (freq!=tFreq) {
				//gliding yo
				gl+=gCoeff * ((1.0/1.0) - gl);
				freq=(tFreq-glideDistance)+glideDistance*gl;
				cycleSize = sampleRate/(freq/4);
			}

			
			out = getSinePhase((phaseAccum/cycleSize)*(2*M_PI))*amps[0]*0.5;
			out += getSinePhase((phaseAccum/cycleSize)*2*(2*M_PI))*amps[0]*0.5;
			out += getBlendedPhase((phaseAccum/cycleSize)*(getBlendedPhase(phaseAccum/(cycleSize/8), waves[0], 1)*FMs[0])*(2*M_PI), waves[0], 1)*pans[0]*amps[0]*0.5;
			if (phaseAccum<cycleSize*0.5) {			
				out += getBlendedPhase((phaseAccum/cycleSize)*(getBlendedPhase(phaseAccum/(cycleSize*2), waves[1], -1)*FMs[1])*(2*M_PI), waves[1], -1)*pans[1]*amps[1]*0.5*2;
				out += getBlendedPhase((phaseAccum/cycleSize)*(getBlendedPhase(phaseAccum/(cycleSize*2), waves[2], -1)*FMs[2])*(2*M_PI), waves[2], -1)*pans[2]*amps[2]*0.5*2;
			} else if (phaseAccum<cycleSize*0.75) {
				out += getBlendedPhase((phaseAccum/cycleSize)*(getBlendedPhase(phaseAccum/(cycleSize*8), waves[3], 1)*FMs[3])*4*(2*M_PI), waves[3], 1)*pans[3]*amps[3]*0.5*2;
				out += getBlendedPhase((phaseAccum/cycleSize)*(getBlendedPhase(phaseAccum/(cycleSize*8), waves[4], 1)*FMs[4])*8*(2*M_PI), waves[4], 1)*pans[4]*amps[4]*0.5*2;
			} else {// if (phaseAccum<cycleSize*0.875) {
				out += getBlendedPhase((phaseAccum/cycleSize)*(getBlendedPhase(phaseAccum/(cycleSize*8), waves[5], -1)*FMs[5])*16*(2*M_PI), waves[5], -1)*pans[5]*amps[5]*0.5*2;
			}
			phaseAccum++;
			if (phaseAccum>cycleSize) {
				phaseAccum = 0;
			}

			//calculate amplitude envelope
			if (envPhase==0) {
				//attack phase
				env+=aCoeff * ((1.0/0.63) - env);
	
				if (env>1.0) envPhase++;
			} else if (envPhase==1) {
				//decay and sustain phase

				env+=dCoeff * (sustain - env);
			} else {
				//release phase
				env += rCoeff * (1.0-(1.0/0.63) - env);
				if (env<0.0) { playing = false; env = 0.0; }
			}
			//apply amplitude envelope
			out*=env;

			//apply master volume
			out*=mAmp*0.5;
			//apply hard clipping
			if (out>1) out=1;
			if (out<-1) out=-1;
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
    float waves[6];
	float FMs[6];
	float pans[6];
	float amps[6];
	float waveBlend;
	float freq;
	float freqOffset;
	float tFreq; //target frequency for gliding
	float glideDistance; //freq distance
	float stereo;
	float mAmp;
	float phaseAccum;
	float cycleSize;
	float sampleRate;
	float notePlaying;
	float glide, gl;
	bool playing;

	float attack, decay, sustain, release;
	float env;
	float envPhase; //0, 1(d+s), 2
	float aCoeff, dCoeff, rCoeff, gCoeff;
};

#endif // SYNTH_HXX_INCLUDED
