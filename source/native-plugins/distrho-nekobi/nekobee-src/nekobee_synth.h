/* nekobee DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Sean Bolton and others.
 *
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
 * Portions of this file may have come from alsa-lib, copyright
 * and licensed under the LGPL v2.1.
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

#ifndef _XSYNTH_SYNTH_H
#define _XSYNTH_SYNTH_H

#include <pthread.h>

#include "nekobee.h"
#include "nekobee_types.h"

#define XSYNTH_MONO_MODE_OFF  0
#define XSYNTH_MONO_MODE_ON   1
#define XSYNTH_MONO_MODE_ONCE 2
#define XSYNTH_MONO_MODE_BOTH 3

#define XSYNTH_GLIDE_MODE_LEGATO   0
#define XSYNTH_GLIDE_MODE_INITIAL  1
#define XSYNTH_GLIDE_MODE_ALWAYS   2
#define XSYNTH_GLIDE_MODE_LEFTOVER 3
#define XSYNTH_GLIDE_MODE_OFF      4

/*
 * nekobee_synth_t
 */
struct _nekobee_synth_t {
    /* output */
    unsigned long   sample_rate;
    float           deltat;            /* 1 / sample_rate */
    unsigned long   nugget_remains;

    /* voice tracking and data */
    unsigned int    note_id;           /* incremented for every new note, used for voice-stealing prioritization */
    int             polyphony;         /* requested polyphony, must be <= XSYNTH_MAX_POLYPHONY */
    int             voices;            /* current polyphony, either requested polyphony above or 1 while in monophonic mode */
    int             monophonic;        /* true if operating in monophonic mode */
    int             glide;             /* current glide mode */
    float           last_noteon_pitch; /* glide start pitch for non-legato modes */
    signed char     held_keys[8];      /* for monophonic key tracking, an array of note-ons, most recently received first */
    float           vcf_accent;        /* used to emulate the circuit that sweeps the vcf at full resonance */
    float           vca_accent;        /* used to smooth the accent pulse, removing the click */

    //nekobee_voice_t *voice[XSYNTH_MAX_POLYPHONY];
    nekobee_voice_t *voice;
    pthread_mutex_t voicelist_mutex;
    int             voicelist_mutex_grab_failed;

    /* current non-paramter-mapped controller values */
    unsigned char   key_pressure[128];
    unsigned char   cc[128];                  /* controller values */
    unsigned char   channel_pressure;
    unsigned char   pitch_wheel_sensitivity;  /* in semitones */
    int             pitch_wheel;              /* range is -8192 - 8191 */

    /* translated controller values */
    float           mod_wheel;                /* filter cutoff multiplier, off = 1.0, full on = 0.0 */
    float           pitch_bend;               /* frequency multiplier, product of wheel setting and sensitivity, center = 1.0 */
    float           cc_volume;                /* volume multiplier, 0.0 to 1.0 */

    /* patch parameters */
    float          tuning;
    float          waveform;
    float          cutoff;
    float          resonance;
    float          envmod;
    float          decay;
    float          accent;
    float          volume;
};

void nekobee_synth_all_voices_off(nekobee_synth_t *synth);
void nekobee_synth_note_off(nekobee_synth_t *synth, unsigned char key,
                            unsigned char rvelocity);
void nekobee_synth_all_notes_off(nekobee_synth_t *synth);
void nekobee_synth_note_on(nekobee_synth_t *synth, unsigned char key,
                           unsigned char velocity);
void nekobee_synth_control_change(nekobee_synth_t *synth, unsigned int param,
                                  signed int value);
void nekobee_synth_init_controls(nekobee_synth_t *synth);
void nekobee_synth_render_voices(nekobee_synth_t *synth, float *out,
                                 unsigned long sample_count,
                                 int do_control_update);

/* these come right out of alsa/asoundef.h */
#define MIDI_CTL_MSB_MODWHEEL           0x01    /**< Modulation */
#define MIDI_CTL_MSB_PORTAMENTO_TIME    0x05    /**< Portamento time */
#define MIDI_CTL_MSB_MAIN_VOLUME        0x07    /**< Main volume */
#define MIDI_CTL_MSB_BALANCE            0x08    /**< Balance */
#define MIDI_CTL_LSB_MODWHEEL           0x21    /**< Modulation */
#define MIDI_CTL_LSB_PORTAMENTO_TIME    0x25    /**< Portamento time */
#define MIDI_CTL_LSB_MAIN_VOLUME        0x27    /**< Main volume */
#define MIDI_CTL_LSB_BALANCE            0x28    /**< Balance */
#define MIDI_CTL_SUSTAIN                0x40    /**< Sustain pedal */

// nekobee defines
#define MIDI_CTL_TUNING                 0x4b    // impossible
#define MIDI_CTL_WAVEFORM               0x46    // select waveform
#define MIDI_CTL_CUTOFF                 0x4a    // VCF Cutoff
#define MIDI_CTL_RESONANCE              0x47    // VCF Resonance
#define MIDI_CTL_ENVMOD                 0x01    // cheat and use modwheel
#define MIDI_CTL_DECAY                  0x48    // Decay time (well release really)
#define MIDI_CTL_ACCENT                 0x4c    // impossible

#define MIDI_CTL_ALL_SOUNDS_OFF         0x78    /**< All sounds off */
#define MIDI_CTL_RESET_CONTROLLERS      0x79    /**< Reset Controllers */
#define MIDI_CTL_ALL_NOTES_OFF          0x7b    /**< All notes off */

#define XSYNTH_SYNTH_SUSTAINED(_s)  ((_s)->cc[MIDI_CTL_SUSTAIN] >= 64)

#endif /* _XSYNTH_SYNTH_H */
