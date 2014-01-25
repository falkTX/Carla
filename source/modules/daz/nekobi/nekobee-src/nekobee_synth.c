/* nekobee DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Sean Bolton and others.
 *
 * Portions of this file may have come from Steve Brookes'
 * nekobee, copyright (C) 1999 S. J. Brookes.
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
 * Portions of this file may have come from Chris Cannam and Steve
 * Harris's public domain DSSI example code.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#include "nekobee.h"
#include "nekobee_synth.h"
#include "nekobee_voice.h"

/*
 * nekobee_synth_all_voices_off
 *
 * stop processing all notes immediately
 */
void
nekobee_synth_all_voices_off(nekobee_synth_t *synth)
{
    int i;
    nekobee_voice_t *voice;

    for (i = 0; i < synth->voices; i++) {
        //voice = synth->voice[i];
        voice = synth->voice;
        if (_PLAYING(voice)) {
            nekobee_voice_off(voice);
        }
    }
    for (i = 0; i < 8; i++) synth->held_keys[i] = -1;
}

/*
 * nekobee_synth_note_off
 *
 * handle a note off message
 */
void
nekobee_synth_note_off(nekobee_synth_t *synth, unsigned char key, unsigned char rvelocity)
{
    int i, count = 0;
    nekobee_voice_t *voice;

    for (i = 0; i < synth->voices; i++) {
        voice = synth->voice;
        if (_PLAYING(voice)) {
            XDB_MESSAGE(XDB_NOTE, " nekobee_synth_note_off: key %d rvel %d voice %d note id %d\n", key, rvelocity, i, voice->note_id);
            nekobee_voice_note_off(synth, voice, key, 64);
            count++;
        }
    }

    if (!count)
        nekobee_voice_remove_held_key(synth, key);

    return;
    (void)rvelocity;
}

/*
 * nekobee_synth_all_notes_off
 *
 * put all notes into the released state
 */
void
nekobee_synth_all_notes_off(nekobee_synth_t* synth)
{
    int i;
    nekobee_voice_t *voice;

    /* reset the sustain controller */
    synth->cc[MIDI_CTL_SUSTAIN] = 0;
    for (i = 0; i < synth->voices; i++) {
        //voice = synth->voice[i];
        voice = synth->voice;
        if (_ON(voice) || _SUSTAINED(voice)) {
            nekobee_voice_release_note(synth, voice);
        }
    }
}

/*
 * nekobee_synth_note_on
 */
void
nekobee_synth_note_on(nekobee_synth_t *synth, unsigned char key, unsigned char velocity)
{
    nekobee_voice_t* voice;

    voice = synth->voice;
    if (_PLAYING(synth->voice)) {
        XDB_MESSAGE(XDB_NOTE, " nekobee_synth_note_on: retriggering mono voice on new key %d\n", key);
    }

    voice->note_id = synth->note_id++;

    nekobee_voice_note_on(synth, voice, key, velocity);
}

/*
 * nekobee_synth_update_volume
 */
void
nekobee_synth_update_volume(nekobee_synth_t* synth)
{
    synth->cc_volume = (float)(synth->cc[MIDI_CTL_MSB_MAIN_VOLUME] * 128 +
                               synth->cc[MIDI_CTL_LSB_MAIN_VOLUME]) / 16256.0f;
    if (synth->cc_volume > 1.0f)
        synth->cc_volume = 1.0f;
    /* don't need to check if any playing voices need updating, because it's global */
}

/*
 * nekobee_synth_control_change
 */
void
nekobee_synth_control_change(nekobee_synth_t *synth, unsigned int param, signed int value)
{
    synth->cc[param] = value;

    switch (param) {

      case MIDI_CTL_MSB_MAIN_VOLUME:
      case MIDI_CTL_LSB_MAIN_VOLUME:
        nekobee_synth_update_volume(synth);
        break;

      case MIDI_CTL_ALL_SOUNDS_OFF:
        nekobee_synth_all_voices_off(synth);
        break;

      case MIDI_CTL_RESET_CONTROLLERS:
        nekobee_synth_init_controls(synth);
        break;

      case MIDI_CTL_ALL_NOTES_OFF:
        nekobee_synth_all_notes_off(synth);
        break;

      /* what others should we respond to? */

      /* these we ignore (let the host handle):
       *  BANK_SELECT_MSB
       *  BANK_SELECT_LSB
       *  DATA_ENTRY_MSB
       *  NRPN_MSB
       *  NRPN_LSB
       *  RPN_MSB
       *  RPN_LSB
       * -FIX- no! we need RPN (0, 0) Pitch Bend Sensitivity!
       */
    }
}

/*
 * nekobee_synth_init_controls
 */
void
nekobee_synth_init_controls(nekobee_synth_t *synth)
{
    int i;

    for (i = 0; i < 128; i++) {
        synth->cc[i] = 0;
    }

    synth->cc[7] = 127;                  /* full volume */
    nekobee_synth_update_volume(synth);
}

/*
 * nekobee_synth_render_voices
 */
void
nekobee_synth_render_voices(nekobee_synth_t *synth, float *out, unsigned long sample_count,
                        int do_control_update)
{
    unsigned long i;
    float res, wow;

    /* clear the buffer */
    for (i = 0; i < sample_count; i++)
        out[i] = 0.0f;

    // we can do anything that must be updated all the time here
    // this is called even when a voice isn't playing

    // approximate a log scale
    res = 1-synth->resonance;
    wow = res*res;
    wow = wow/10.0f;

    // as the resonance is increased, "wow" slows down the accent attack
    if ((synth->voice->velocity>90) && (synth->vcf_accent < synth->voice->vcf_eg)) {
        synth->vcf_accent=(0.985-wow)*synth->vcf_accent+(0.015+wow)*synth->voice->vcf_eg;
    } else {
        synth->vcf_accent=(0.985-wow)*synth->vcf_accent;	// or just decay
    }

    if (synth->voice->velocity>90) {
        synth->vca_accent=0.95*synth->vca_accent+0.05; // ramp up accent on with a time constant
    } else {
        synth->vca_accent=0.95*synth->vca_accent;       // accent off with time constant
    }
#if defined(XSYNTH_DEBUG) && (XSYNTH_DEBUG & XDB_AUDIO)
     out[0] += 0.10f; /* add a 'buzz' to output so there's something audible even when quiescent */
#endif /* defined(XSYNTH_DEBUG) && (XSYNTH_DEBUG & XDB_AUDIO) */
    if (_PLAYING(synth->voice)) {
        nekobee_voice_render(synth, synth->voice, out, sample_count, do_control_update);
    }
}
