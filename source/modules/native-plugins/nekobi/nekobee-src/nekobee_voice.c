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

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#include <stdlib.h>

#include "nekobee_types.h"
#include "nekobee.h"
#include "nekobee_synth.h"
#include "nekobee_voice.h"

/*
 * nekobee_voice_new
 */
nekobee_voice_t *
nekobee_voice_new()
{
    nekobee_voice_t *voice;

    voice = (nekobee_voice_t *)calloc(sizeof(nekobee_voice_t), 1);
    if (voice) {
        voice->status = XSYNTH_VOICE_OFF;
    }
    return voice;
}

/*
 * nekobee_voice_note_on
 */
void
nekobee_voice_note_on(nekobee_synth_t *synth, nekobee_voice_t *voice,
                     unsigned char key, unsigned char velocity)
{
    int i;

    voice->key      = key;
    voice->velocity = velocity;


   if (!synth->monophonic || !(_ON(voice) || _SUSTAINED(voice))) {

        // brand-new voice, or monophonic voice in release phase; set everything up 
        XDB_MESSAGE(XDB_NOTE, " nekobee_voice_note_on in polyphonic/new section: key %d, mono %d, old status %d\n", key, synth->monophonic, voice->status);

        voice->target_pitch = nekobee_pitch[key];
        

            if (synth->held_keys[0] >= 0) {
                voice->prev_pitch = nekobee_pitch[synth->held_keys[0]];
            } else {
                voice->prev_pitch = voice->target_pitch;
            }
        
        if (!_PLAYING(voice)) {
            voice->lfo_pos = 0.0f;
            voice->vca_eg = 0.0f;
            voice->vcf_eg = 0.0f;
            voice->delay1 = 0.0f;
            voice->delay2 = 0.0f;
            voice->delay3 = 0.0f;
            voice->delay4 = 0.0f;
            voice->c5     = 0.0f;
            voice->osc_index = 0;
            voice->osc1.last_waveform = -1;
            voice->osc1.pos = 0.0f;
			
        }
        voice->vca_eg_phase = 0;
        voice->vcf_eg_phase = 0;
//        nekobee_voice_update_pressure_mod(synth, voice);

    } else {

        /* synth is monophonic, and we're modifying a playing voice */
        XDB_MESSAGE(XDB_NOTE, " nekobee_voice_note_on in monophonic section: old key %d => new key %d\n", synth->held_keys[0], key);

        /* set new pitch */
        voice->target_pitch = nekobee_pitch[key];
        if (synth->glide == XSYNTH_GLIDE_MODE_INITIAL ||
            synth->glide == XSYNTH_GLIDE_MODE_OFF)
            voice->prev_pitch = voice->target_pitch;

        /* if in 'on' or 'both' modes, and key has changed, then re-trigger EGs */
        if ((synth->monophonic == XSYNTH_MONO_MODE_ON ||
             synth->monophonic == XSYNTH_MONO_MODE_BOTH) &&
            (synth->held_keys[0] < 0 || synth->held_keys[0] != key)) {
            voice->vca_eg_phase = 0;
            voice->vcf_eg_phase = 0;
        }

        /* all other variables stay what they are */

    }
    synth->last_noteon_pitch = voice->target_pitch;

    /* add new key to the list of held keys */

    /* check if new key is already in the list; if so, move it to the
     * top of the list, otherwise shift the other keys down and add it
     * to the top of the list. */
    for (i = 0; i < 7; i++) {
        if (synth->held_keys[i] == key)
            break;
    }
    for (; i > 0; i--) {
        synth->held_keys[i] = synth->held_keys[i - 1];
    }
    synth->held_keys[0] = key;

    if (!_PLAYING(voice)) {

        nekobee_voice_start_voice(voice);

    } else if (!_ON(voice)) {  /* must be XSYNTH_VOICE_SUSTAINED or XSYNTH_VOICE_RELEASED */

        voice->status = XSYNTH_VOICE_ON;

    }
}

/*
 * nekobee_voice_set_release_phase
 */
static inline void
nekobee_voice_set_release_phase(nekobee_voice_t *voice)
{
    voice->vca_eg_phase = 2;
    voice->vcf_eg_phase = 2;
}

/*
 * nekobee_voice_remove_held_key
 */
inline void
nekobee_voice_remove_held_key(nekobee_synth_t *synth, unsigned char key)
{
    int i;

    /* check if this key is in list of held keys; if so, remove it and
     * shift the other keys up */
    for (i = 7; i >= 0; i--) {
        if (synth->held_keys[i] == key)
            break;
    }
    if (i >= 0) {
        for (; i < 7; i++) {
            synth->held_keys[i] = synth->held_keys[i + 1];
        }
        synth->held_keys[7] = -1;
    }
}

/*
 * nekobee_voice_note_off
 */
void
nekobee_voice_note_off(nekobee_synth_t *synth, nekobee_voice_t *voice,
                      unsigned char key, unsigned char rvelocity)
{
    unsigned char previous_top_key;

    XDB_MESSAGE(XDB_NOTE, " nekobee_set_note_off: called for voice %p, key %d\n", voice, key);

    /* save release velocity */
    voice->velocity = rvelocity;

    previous_top_key = synth->held_keys[0];

    /* remove this key from list of held keys */
    nekobee_voice_remove_held_key(synth, key);

        if (synth->held_keys[0] >= 0) {

            /* still some keys held */

            if (synth->held_keys[0] != previous_top_key) {

                /* most-recently-played key has changed */
                voice->key = synth->held_keys[0];
                XDB_MESSAGE(XDB_NOTE, " note-off in monophonic section: changing pitch to %d\n", voice->key);
                voice->target_pitch = nekobee_pitch[voice->key];
                if (synth->glide == XSYNTH_GLIDE_MODE_INITIAL ||
                    synth->glide == XSYNTH_GLIDE_MODE_OFF)
                    voice->prev_pitch = voice->target_pitch;

                /* if mono mode is 'both', re-trigger EGs */
                if (synth->monophonic == XSYNTH_MONO_MODE_BOTH && !_RELEASED(voice)) {
                    voice->vca_eg_phase = 0;
                    voice->vcf_eg_phase = 0;
                }

            }

        } else {  /* no keys still held */

            if (XSYNTH_SYNTH_SUSTAINED(synth)) {

                /* no more keys in list, but we're sustained */
                XDB_MESSAGE(XDB_NOTE, " note-off in monophonic section: sustained with no held keys\n");
                if (!_RELEASED(voice))
                    voice->status = XSYNTH_VOICE_SUSTAINED;

            } else {  /* not sustained */

                /* no more keys in list, so turn off note */
                XDB_MESSAGE(XDB_NOTE, " note-off in monophonic section: turning off voice %p\n", voice);
                nekobee_voice_set_release_phase(voice);
                voice->status = XSYNTH_VOICE_RELEASED;

            }
        }

}

/*
 * nekobee_voice_release_note
 */
void
nekobee_voice_release_note(nekobee_synth_t *synth, nekobee_voice_t *voice)
{
    XDB_MESSAGE(XDB_NOTE, " nekobee_voice_release_note: turning off voice %p\n", voice);
    if (_ON(voice)) {
        /* dummy up a release velocity */
        voice->rvelocity = 64;
    }
    nekobee_voice_set_release_phase(voice);
    voice->status = XSYNTH_VOICE_RELEASED;

    return;
    (void)synth;
}
