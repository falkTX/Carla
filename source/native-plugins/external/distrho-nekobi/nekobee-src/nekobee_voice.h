/* nekobee DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Sean Bolton and others.
 *
 * Portions of this file may have come from Steve Brookes'
 * nekobee, copyright (C) 1999 S. J. Brookes.
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef _XSYNTH_VOICE_H
#define _XSYNTH_VOICE_H

#include <string.h>

#include "nekobee_types.h"

/* maximum size of a rendering burst */
#define XSYNTH_NUGGET_SIZE      64

/* minBLEP constants */
/* minBLEP table oversampling factor (must be a power of two): */
#define MINBLEP_PHASES          64
/* MINBLEP_PHASES minus one: */
#define MINBLEP_PHASE_MASK      63
/* length in samples of (truncated) step discontinuity delta: */
#define STEP_DD_PULSE_LENGTH    72
/* length in samples of (truncated) slope discontinuity delta: */
#define SLOPE_DD_PULSE_LENGTH   71
/* the longer of the two above: */
#define LONGEST_DD_PULSE_LENGTH STEP_DD_PULSE_LENGTH
/* MINBLEP_BUFFER_LENGTH must be at least XSYNTH_NUGGET_SIZE plus
 * LONGEST_DD_PULSE_LENGTH, and not less than twice LONGEST_DD_PULSE_LENGTH: */
#define MINBLEP_BUFFER_LENGTH  512
/* delay between start of DD pulse and the discontinuity, in samples: */
#define DD_SAMPLE_DELAY          4

struct _nekobee_patch_t
{
    float         tuning;
    unsigned char waveform;
    float         cutoff;
    float         resonance;
    float         envmod;
    float         decay;
    float         accent;
    float         volume;
};

enum nekobee_voice_status
{
    XSYNTH_VOICE_OFF,       /* silent: is not processed by render loop */
    XSYNTH_VOICE_ON,        /* has not received a note off event */
    XSYNTH_VOICE_SUSTAINED, /* has received note off, but sustain controller is on */
    XSYNTH_VOICE_RELEASED   /* had note off, not sustained, in final decay phase of envelopes */
};

struct blosc
{
    int   last_waveform,    /* persistent */
          waveform,         /* comes from LADSPA port each cycle */
          bp_high;          /* persistent */
    float pos,              /* persistent */
          pw;               /* comes from LADSPA port each cycle */
};

/*
 * nekobee_voice_t
 */
struct _nekobee_voice_t
{
    unsigned int  note_id;

    unsigned char status;
    unsigned char key;
    unsigned char velocity;
    unsigned char rvelocity;   /* the note-off velocity */

    /* translated controller values */
    float         pressure;    /* filter resonance multiplier, off = 1.0, full on = 0.0 */

    /* persistent voice state */
    float         prev_pitch,
                  target_pitch,
                  lfo_pos;
    struct blosc  osc1;
    float         vca_eg,
                  vcf_eg,
                  accent_slug,
                  delay1,
                  delay2,
                  delay3,
                  delay4,
                  c5;
    unsigned char vca_eg_phase,
                  vcf_eg_phase;
    int           osc_index;       /* shared index into osc_audio */
    float         osc_audio[MINBLEP_BUFFER_LENGTH];
    float         freqcut_buf[XSYNTH_NUGGET_SIZE];
    float         vca_buf[XSYNTH_NUGGET_SIZE];
};

#define _PLAYING(voice)    ((voice)->status != XSYNTH_VOICE_OFF)
#define _ON(voice)         ((voice)->status == XSYNTH_VOICE_ON)
#define _SUSTAINED(voice)  ((voice)->status == XSYNTH_VOICE_SUSTAINED)
#define _RELEASED(voice)   ((voice)->status == XSYNTH_VOICE_RELEASED)
#define _AVAILABLE(voice)  ((voice)->status == XSYNTH_VOICE_OFF)

extern float nekobee_pitch[128];

typedef struct { float value, delta; } float_value_delta;
extern float_value_delta step_dd_table[];

extern float slope_dd_table[];

/* nekobee_voice.c */
nekobee_voice_t *nekobee_voice_new();
void             nekobee_voice_note_on(nekobee_synth_t *synth,
                                     nekobee_voice_t *voice,
                                     unsigned char key,
                                     unsigned char velocity);
void             nekobee_voice_remove_held_key(nekobee_synth_t *synth,
                                             unsigned char key);
void             nekobee_voice_note_off(nekobee_synth_t *synth,
                                      nekobee_voice_t *voice,
                                      unsigned char key,
                                      unsigned char rvelocity);
void             nekobee_voice_release_note(nekobee_synth_t *synth,
                                          nekobee_voice_t *voice);
void             nekobee_voice_set_ports(nekobee_synth_t *synth,
                                       nekobee_patch_t *patch);
void             nekobee_voice_update_pressure_mod(nekobee_synth_t *synth,
                                                 nekobee_voice_t *voice);

/* nekobee_voice_render.c */
void nekobee_init_tables(void);
void nekobee_voice_render(nekobee_synth_t *synth, nekobee_voice_t *voice,
                         float *out, unsigned long sample_count,
                         int do_control_update);

/* inline functions */

/*
 * nekobee_voice_off
 *
 * Purpose: Turns off a voice immediately, meaning that it is not processed
 * anymore by the render loop.
 */
static inline void
nekobee_voice_off(nekobee_voice_t* voice)
{
    voice->status = XSYNTH_VOICE_OFF;
    /* silence the oscillator buffer for the next use */
    memset(voice->osc_audio, 0, MINBLEP_BUFFER_LENGTH * sizeof(float));
    /* -FIX- decrement active voice count? */
}

/*
 * nekobee_voice_start_voice
 */
static inline void
nekobee_voice_start_voice(nekobee_voice_t *voice)
{
    voice->status = XSYNTH_VOICE_ON;
    /* -FIX- increment active voice count? */
}

#endif /* _XSYNTH_VOICE_H */
