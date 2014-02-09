/* nekobee DSSI software synthesizer plugin
 */

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#include <math.h>

#include "nekobee.h"
#include "nekobee_synth.h"
#include "nekobee_voice.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define M_2PI_F (2.0f * (float)M_PI)
#define M_PI_F (float)M_PI

#define VCF_FREQ_MAX  (0.825f)    /* original filters only stable to this frequency */

static int   tables_initialized = 0;

float        nekobee_pitch[128];

#define pitch_ref_note 69

#define volume_to_amplitude_scale 128

static float volume_to_amplitude_table[4 + volume_to_amplitude_scale + 2];

static float velocity_to_attenuation[128];

static float qdB_to_amplitude_table[4 + 256 + 0];

void
nekobee_init_tables(void)
{
    int i;
    float pexp;
    float volume, volume_exponent;
    float ol, amp;

    if (tables_initialized)
        return;

    /* MIDI note to pitch */
    for (i = 0; i < 128; ++i) {
        pexp = (float)(i - pitch_ref_note) / 12.0f;
        nekobee_pitch[i] = powf(2.0f, pexp);
    }

    /* volume to amplitude
     *
     * This generates a curve which is:
     *  volume_to_amplitude_table[128 + 4] = 0.25 * 3.16...   ~=  -2dB
     *  volume_to_amplitude_table[64 + 4]  = 0.25 * 1.0       ~= -12dB
     *  volume_to_amplitude_table[32 + 4]  = 0.25 * 0.316...  ~= -22dB
     *  volume_to_amplitude_table[16 + 4]  = 0.25 * 0.1       ~= -32dB
     *   etc.
     */
    volume_exponent = 1.0f / (2.0f * log10f(2.0f));
    for (i = 0; i <= volume_to_amplitude_scale; i++) {
        volume = (float)i / (float)volume_to_amplitude_scale;
        volume_to_amplitude_table[i + 4] = powf(2.0f * volume, volume_exponent) / 4.0f;
    }
    volume_to_amplitude_table[ -1 + 4] = 0.0f;
    volume_to_amplitude_table[129 + 4] = volume_to_amplitude_table[128 + 4];

    /* velocity to attenuation
     *
     * Creates the velocity to attenuation lookup table, for converting
     * velocities [1, 127] to full-velocity-sensitivity attenuation in
     * quarter decibels.  Modeled after my TX-7's velocity response.*/
    velocity_to_attenuation[0] = 253.9999f;
    for (i = 1; i < 127; i++) {
        if (i >= 10) {
            ol = (powf(((float)i / 127.0f), 0.32f) - 1.0f) * 100.0f;
            amp = powf(2.0f, ol / 8.0f);
        } else {
            ol = (powf(((float)10 / 127.0f), 0.32f) - 1.0f) * 100.0f;
            amp = powf(2.0f, ol / 8.0f) * (float)i / 10.0f;
        }
        velocity_to_attenuation[i] = log10f(amp) * -80.0f;
    }
    velocity_to_attenuation[127] = 0.0f;

    /* quarter-decibel attenuation to amplitude */
    qdB_to_amplitude_table[-1 + 4] = 1.0f;
    for (i = 0; i <= 255; i++) {
        qdB_to_amplitude_table[i + 4] = powf(10.0f, (float)i / -80.0f);
    }

    tables_initialized = 1;
}

static inline float
volume(float level)
{
    unsigned char segment;
    float fract;

    level *= (float)volume_to_amplitude_scale;
    segment = lrintf(level - 0.5f);
    fract = level - (float)segment;

    return volume_to_amplitude_table[segment + 4] + fract *
               (volume_to_amplitude_table[segment + 5] -
                volume_to_amplitude_table[segment + 4]);
}

static inline float
qdB_to_amplitude(float qdB)
{
    int i = lrintf(qdB - 0.5f);
    float f = qdB - (float)i;
    return qdB_to_amplitude_table[i + 4] + f *
           (qdB_to_amplitude_table[i + 5] -
            qdB_to_amplitude_table[i + 4]);
}

void blosc_place_step_dd(float *buffer, int index, float phase, float w, float scale){
    float r;
    int i;

    r = MINBLEP_PHASES * phase / w;
    i = lrintf(r - 0.5f);
    r -= (float)i;
    i &= MINBLEP_PHASE_MASK;  /* port changes can cause i to be out-of-range */
    /* This would be better than the above, but more expensive:
     *  while (i < 0) {
     *    i += MINBLEP_PHASES;
     *    index++;
     *  }
     */

    while (i < MINBLEP_PHASES * STEP_DD_PULSE_LENGTH) {
        buffer[index] += scale * (step_dd_table[i].value + r * step_dd_table[i].delta);
        i += MINBLEP_PHASES;
        index++;
    }
}


void vco(unsigned long sample_count, nekobee_voice_t *voice, struct blosc *osc,
                int index, float w)

{
	unsigned long sample;
	float pos = osc->pos;
	float pw, gain, halfgain, out;
	pw=0.46f;
	gain=1.0f;
	halfgain=gain*0.5f;
	int   bp_high = osc->bp_high;
	out=(bp_high ? halfgain : -halfgain);

		switch (osc->waveform)
		{
		default:
		case 0: {

			for (sample = 0; sample < sample_count; sample++) {
		pos += w;
        if (bp_high) {
            if (pos >= pw) {
                blosc_place_step_dd(voice->osc_audio, index, pos - pw, w, -gain);
                bp_high = 0;
                out = -halfgain;
            }
            if (pos >= 1.0f) {
                pos -= 1.0f;
                blosc_place_step_dd(voice->osc_audio, index, pos, w, gain);
                bp_high = 1;
                out = halfgain;
            }
			} else {
            if (pos >= 1.0f) {
                pos -= 1.0f;
                blosc_place_step_dd(voice->osc_audio, index, pos, w, gain);
                bp_high = 1;
                out = halfgain;
            }

            if (bp_high && pos >= pw) {
                blosc_place_step_dd(voice->osc_audio, index, pos - pw, w, -gain);
                bp_high = 0;
                out = -halfgain;
            }
        }

        voice->osc_audio[index + DD_SAMPLE_DELAY] += out;

        index++;
    }

    osc->pos = pos;
    osc->bp_high = bp_high;
		break;
	  }
		case 1:   			// sawtooth wave
		{
			  for (sample=0; sample < sample_count; sample++) {
        	  pos += w;
              if (pos >= 1.0f) {
				  pos -= 1.0f;
				 blosc_place_step_dd(voice->osc_audio, index, pos, w, gain);
			  }
              voice->osc_audio[index + DD_SAMPLE_DELAY] += gain * (0.5f - pos);

			  index++;
		  }

		  break;
	  }

	}

        osc->pos=pos;
}

static inline void
vcf_4pole(nekobee_voice_t *voice, unsigned long sample_count,
          float *in, float *out, float *cutoff, float qres, float *amp)
{
    unsigned long sample;
    float freqcut, freqcut2, highpass,
          delay1 = voice->delay1,
          delay2 = voice->delay2,
          delay3 = voice->delay3,
          delay4 = voice->delay4;

    qres = 2.0f - qres * 1.995f;

    for (sample = 0; sample < sample_count; sample++) {

        /* Hal Chamberlin's state variable filter */

        freqcut = cutoff[sample] * 2.0f;
        freqcut2 = cutoff[sample] * 4.0f;


        if (freqcut > VCF_FREQ_MAX) freqcut = VCF_FREQ_MAX;
        if (freqcut2 > VCF_FREQ_MAX) freqcut2 = VCF_FREQ_MAX;

        delay2 = delay2 + freqcut * delay1;             /* delay2/4 = lowpass output */
        highpass = in[sample] - delay2 - qres * delay1;
        delay1 = freqcut * highpass + delay1;           /* delay1/3 = bandpass output */

        delay4 = delay4 + freqcut2 * delay3;
        highpass = delay2 - delay4 - qres * delay3;
        delay3 = freqcut2 * highpass + delay3;

        /* mix filter output into output buffer */
        out[sample] += 0.1*atan(3*delay4 * amp[sample]);
    }

    voice->delay1 = delay1;
    voice->delay2 = delay2;
    voice->delay3 = delay3;
    voice->delay4 = delay4;
    voice->c5 = 0.0f;
}


/*
 * nekobee_voice_render
 *
 * generate the actual sound data for this voice
 */
void
nekobee_voice_render(nekobee_synth_t *synth, nekobee_voice_t *voice,
                    float *out, unsigned long sample_count,
                    int do_control_update)
{
    unsigned long sample;

    /* state variables saved in voice */
    float         lfo_pos      = voice->lfo_pos,
                  vca_eg       = voice->vca_eg,
                  vcf_eg       = voice->vcf_eg;
    unsigned char vca_eg_phase = voice->vca_eg_phase,
                  vcf_eg_phase = voice->vcf_eg_phase;
    int           osc_index    = voice->osc_index;

    /* temporary variables used in calculating voice */
    float fund_pitch;
    float deltat = synth->deltat;
    float freq, cutoff, vcf_amt;
    float vcf_acc_amt;

    /* set up synthesis variables from patch */
    float         omega;
    float         vca_eg_amp = qdB_to_amplitude(velocity_to_attenuation[voice->velocity] * 0);

    float         vca_eg_rate_level[3], vca_eg_one_rate[3];
    float         vcf_eg_amp = qdB_to_amplitude(velocity_to_attenuation[voice->velocity] * 0);

    float         vcf_eg_rate_level[3], vcf_eg_one_rate[3];
    float         qres = synth->resonance;
    float         vol_out = volume(synth->volume);

    float velocity = (voice->velocity);

    float vcf_egdecay = synth->decay;

    fund_pitch = 0.1f*voice->target_pitch  +0.9 * voice->prev_pitch;    /* glide */

    if (do_control_update) {
        voice->prev_pitch = fund_pitch; /* save pitch for next time */
    }

    fund_pitch *= 440.0f;

    omega = synth->tuning * fund_pitch;

    // if we have triggered ACCENT
    // we need a shorter decay
    // we should probably have something like this in the note on code
    // that could trigger an ACCENT light
    if (velocity>90) {
            vcf_egdecay=.0005;
    }

    // VCA - In a real 303, it is set for around 2 seconds
    vca_eg_rate_level[0] = 0.1f * vca_eg_amp;	// instant on attack
    vca_eg_one_rate[0] = 0.9f;					// very fast
    vca_eg_rate_level[1] = 0.0f; 					// sustain is zero
    vca_eg_one_rate[1] = 1.0f - 0.00001f;		// decay time is very slow
    vca_eg_rate_level[2] = 0.0f;				// decays to zero
    vca_eg_one_rate[2] =  0.975f; 					// very fast release

    // VCF - funny things go on with the accent

    vcf_eg_rate_level[0] = 0.1f * vcf_eg_amp;
    vcf_eg_one_rate[0] = 1-0.1f; //0.9f;
    vcf_eg_rate_level[1] = 0.0f; // vcf_egdecay * *(synth->vcf_eg_sustain_level) * vcf_eg_amp;
    vcf_eg_one_rate[1] = 1.0f - vcf_egdecay;
    vcf_eg_rate_level[2] = 0.0f;
    vcf_eg_one_rate[2] = 0.9995f; // 1.0f - *(synth->vcf_eg_release_time);

    vca_eg_amp *= 0.99f;
    vcf_eg_amp *= 0.99f;

    freq = M_PI_F * deltat * fund_pitch * synth->mod_wheel;  /* now (0 to 1) * pi */

    cutoff = 0.008f * synth->cutoff;

    // 303 always has slight VCF mod
    vcf_amt = 0.05f+(synth->envmod*0.75);

    /* copy some things so oscillator functions can see them */
    voice->osc1.waveform = lrintf(synth->waveform);

    // work out how much the accent will affect the filter
    vcf_acc_amt=.333f+ (synth->resonance/1.5f);

    for (sample = 0; sample < sample_count; sample++) {
        vca_eg = vca_eg_rate_level[vca_eg_phase] + vca_eg_one_rate[vca_eg_phase] * vca_eg;
        vcf_eg = vcf_eg_rate_level[vcf_eg_phase] + vcf_eg_one_rate[vcf_eg_phase] * vcf_eg;

        voice->freqcut_buf[sample] = (cutoff + (vcf_amt * vcf_eg/2.0f)  + (synth->vcf_accent * synth->accent*0.5f));

        voice->vca_buf[sample] = vca_eg * vol_out*(1.0f + synth->accent*synth->vca_accent);

        if (!vca_eg_phase && vca_eg > vca_eg_amp) vca_eg_phase = 1;  /* flip from attack to decay */
        if (!vcf_eg_phase && vcf_eg > vcf_eg_amp) vcf_eg_phase = 1;  /* flip from attack to decay */
    }

    // oscillator
    vco(sample_count, voice, &voice->osc1, osc_index, deltat * omega);

    // VCF and VCA
    vcf_4pole(voice, sample_count, voice->osc_audio + osc_index, out, voice->freqcut_buf, qres, voice->vca_buf);

    osc_index += sample_count;

    if (do_control_update) {
        /* do those things should be done only once per control-calculation
         * interval ("nugget"), such as voice check-for-dead, pitch envelope
         * calculations, volume envelope phase transition checks, etc. */
        /* check if we've decayed to nothing, turn off voice if so */
        if (vca_eg_phase == 2 && voice->vca_buf[sample_count - 1] < 6.26e-6f) {
            // sound has completed its release phase (>96dB below volume '5' max)
            XDB_MESSAGE(XDB_NOTE, " nekobee_voice_render check for dead: killing note id %d\n", voice->note_id);
            nekobee_voice_off(voice);
            return; // we're dead now, so return
        }

        /* already saved prev_pitch above */

        /* check oscillator audio buffer index, shift buffer if necessary */
        if (osc_index > MINBLEP_BUFFER_LENGTH - (XSYNTH_NUGGET_SIZE + LONGEST_DD_PULSE_LENGTH)) {
            memcpy(voice->osc_audio, voice->osc_audio + osc_index,
                   LONGEST_DD_PULSE_LENGTH * sizeof (float));
            memset(voice->osc_audio + LONGEST_DD_PULSE_LENGTH, 0,
                   (MINBLEP_BUFFER_LENGTH - LONGEST_DD_PULSE_LENGTH) * sizeof (float));
            osc_index = 0;
        }
    }

    /* save things for next time around */
    voice->lfo_pos      = lfo_pos;
    voice->vca_eg       = vca_eg;
    voice->vca_eg_phase = vca_eg_phase;
    voice->vcf_eg       = vcf_eg;
    voice->vcf_eg_phase = vcf_eg_phase;
    voice->osc_index    = osc_index;

    return;
    (void)freq;
    (void)vcf_acc_amt;
}
