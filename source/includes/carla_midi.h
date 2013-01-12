/*
 * Carla common MIDI code
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef __CARLA_MIDI_H__
#define __CARLA_MIDI_H__

#define MAX_MIDI_CHANNELS 16

// MIDI Messages List
#define MIDI_STATUS_NOTE_OFF                           0x80 // note (0-127), velocity (0-127)
#define MIDI_STATUS_NOTE_ON                            0x90 // note (0-127), velocity (0-127)
#define MIDI_STATUS_POLYPHONIC_AFTERTOUCH              0xA0 // note (0-127), pressure (0-127)
#define MIDI_STATUS_CONTROL_CHANGE                     0xB0 // see 'Control Change Messages List'
#define MIDI_STATUS_PROGRAM_CHANGE                     0xC0 // program (0-127), none
#define MIDI_STATUS_AFTERTOUCH                         0xD0 // pressure (0-127), none
#define MIDI_STATUS_PITCH_WHEEL_CONTROL                0xE0 // LSB (0-127), MSB (0-127)

#define MIDI_IS_STATUS_NOTE_OFF(status)                (((status) & 0xF0) == MIDI_STATUS_NOTE_OFF)
#define MIDI_IS_STATUS_NOTE_ON(status)                 (((status) & 0xF0) == MIDI_STATUS_NOTE_ON)
#define MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status)   (((status) & 0xF0) == MIDI_STATUS_POLYPHONIC_AFTERTOUCH)
#define MIDI_IS_STATUS_CONTROL_CHANGE(status)          (((status) & 0xF0) == MIDI_STATUS_CONTROL_CHANGE)
#define MIDI_IS_STATUS_PROGRAM_CHANGE(status)          (((status) & 0xF0) == MIDI_STATUS_PROGRAM_CHANGE)
#define MIDI_IS_STATUS_AFTERTOUCH(status)              (((status) & 0xF0) == MIDI_STATUS_AFTERTOUCH)
#define MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status)     (((status) & 0xF0) == MIDI_STATUS_PITCH_WHEEL_CONTROL)

// Control Change Messages List
#define MIDI_CONTROL_BANK_SELECT                       0x00 // 0-127, MSB
#define MIDI_CONTROL_MODULATION_WHEEL                  0x01 // 0-127, MSB
#define MIDI_CONTROL_BREATH_CONTROLLER                 0x02 // 0-127, MSB
#define MIDI_CONTROL_FOOT_CONTROLLER                   0x04 // 0-127, MSB
#define MIDI_CONTROL_PORTAMENTO_TIME                   0x05 // 0-127, MSB
#define MIDI_CONTROL_DATA_ENTRY                        0x06 // 0-127, MSB
#define MIDI_CONTROL_CHANNEL_VOLUME                    0x07 // 0-127, MSB
#define MIDI_CONTROL_BALANCE                           0x08 // 0-127, MSB
#define MIDI_CONTROL_PAN                               0x0A // 0-127, MSB
#define MIDI_CONTROL_EXPRESSION_CONTROLLER             0x0B // 0-127, MSB
#define MIDI_CONTROL_EFFECT_CONTROL_1                  0x0C // 0-127, MSB
#define MIDI_CONTROL_EFFECT_CONTROL_2                  0x0D // 0-127, MSB
#define MIDI_CONTROL_GENERAL_PURPOSE_CONTROLLER_1      0x10 // 0-127, MSB
#define MIDI_CONTROL_GENERAL_PURPOSE_CONTROLLER_2      0x11 // 0-127, MSB
#define MIDI_CONTROL_GENERAL_PURPOSE_CONTROLLER_3      0x12 // 0-127, MSB
#define MIDI_CONTROL_GENERAL_PURPOSE_CONTROLLER_4      0x13 // 0-127, MSB
#define MIDI_CONTROL_BANK_SELECT__LSB                  0x20 // 0-127, LSB
#define MIDI_CONTROL_MODULATION_WHEEL__LSB             0x21 // 0-127, LSB
#define MIDI_CONTROL_BREATH_CONTROLLER__LSB            0x22 // 0-127, LSB
#define MIDI_CONTROL_FOOT_CONTROLLER__LSB              0x24 // 0-127, LSB
#define MIDI_CONTROL_PORTAMENTO_TIME__LSB              0x25 // 0-127, LSB
#define MIDI_CONTROL_DATA_ENTRY__LSB                   0x26 // 0-127, LSB
#define MIDI_CONTROL_CHANNEL_VOLUME__LSB               0x27 // 0-127, LSB
#define MIDI_CONTROL_BALANCE__LSB                      0x28 // 0-127, LSB
#define MIDI_CONTROL_PAN__LSB                          0x2A // 0-127, LSB
#define MIDI_CONTROL_EXPRESSION_CONTROLLER__LSB        0x2B // 0-127, LSB
#define MIDI_CONTROL_EFFECT_CONTROL_1__LSB             0x2C // 0-127, LSB
#define MIDI_CONTROL_EFFECT_CONTROL_2__LSB             0x3D // 0-127, LSB
#define MIDI_CONTROL_GENERAL_PURPOSE_CONTROLLER_1__LSB 0x30 // 0-127, LSB
#define MIDI_CONTROL_GENERAL_PURPOSE_CONTROLLER_2__LSB 0x31 // 0-127, LSB
#define MIDI_CONTROL_GENERAL_PURPOSE_CONTROLLER_3__LSB 0x32 // 0-127, LSB
#define MIDI_CONTROL_GENERAL_PURPOSE_CONTROLLER_4__LSB 0x33 // 0-127, LSB
#define MIDI_CONTROL_DAMPER_PEDAL                      0x40 // <= 63 off, >= 64 off
#define MIDI_CONTROL_PORTAMENTO                        0x41 // <= 63 off, >= 64 off
#define MIDI_CONTROL_SOSTENUDO                         0x42 // <= 63 off, >= 64 off
#define MIDI_CONTROL_SOFT_PEDAL                        0x43 // <= 63 off, >= 64 off
#define MIDI_CONTROL_LEGAL_FOOTSWITCH                  0x44 // <= 63 normal, >= 64 legato
#define MIDI_CONTROL_HOLD_2                            0x45 // <= 63 off, >= 64 off
#define MIDI_CONTROL_SOUND_CONTROLLER_1                0x46 // 0-127, LSB
#define MIDI_CONTROL_SOUND_CONTROLLER_2                0x47 // 0-127, LSB
#define MIDI_CONTROL_SOUND_CONTROLLER_3                0x48 // 0-127, LSB
#define MIDI_CONTROL_SOUND_CONTROLLER_4                0x49 // 0-127, LSB
#define MIDI_CONTROL_SOUND_CONTROLLER_5                0x4A // 0-127, LSB
#define MIDI_CONTROL_SOUND_CONTROLLER_6                0x4B // 0-127, LSB
#define MIDI_CONTROL_SOUND_CONTROLLER_7                0x4C // 0-127, LSB
#define MIDI_CONTROL_SOUND_CONTROLLER_8                0x4D // 0-127, LSB
#define MIDI_CONTROL_SOUND_CONTROLLER_9                0x4E // 0-127, LSB
#define MIDI_CONTROL_SOUND_CONTROLLER_10               0x4F // 0-127, LSB
#define MIDI_CONTROL_GENERAL_PURPOSE_CONTROLLER_5      0x50 // 0-127, LSB
#define MIDI_CONTROL_GENERAL_PURPOSE_CONTROLLER_6      0x51 // 0-127, LSB
#define MIDI_CONTROL_GENERAL_PURPOSE_CONTROLLER_7      0x52 // 0-127, LSB
#define MIDI_CONTROL_GENERAL_PURPOSE_CONTROLLER_8      0x53 // 0-127, LSB
#define MIDI_CONTROL_PORTAMENTO_CONTROL                0x54 // 0-127, LSB
#define MIDI_CONTROL_HIGH_RESOLUTION_VELOCITY_PREFIX   0x58 // 0-127, LSB
#define MIDI_CONTROL_EFFECTS_1_DEPTH                   0x5B // 0-127
#define MIDI_CONTROL_EFFECTS_2_DEPTH                   0x5C // 0-127
#define MIDI_CONTROL_EFFECTS_3_DEPTH                   0x5D // 0-127
#define MIDI_CONTROL_EFFECTS_4_DEPTH                   0x5E // 0-127
#define MIDI_CONTROL_EFFECTS_5_DEPTH                   0x5F // 0-127

#define MIDI_CONTROL_ALL_SOUND_OFF                     0x78 // 0
#define MIDI_CONTROL_RESET_ALL_CONTROLLERS             0x79 // 0
#define MIDI_CONTROL_LOCAL_CONTROL                     0x7A // 0 off, 127 on
#define MIDI_CONTROL_ALL_NOTES_OFF                     0x7B // 0
#define MIDI_CONTROL_OMNI_MODE_OFF                     0x7C // 0 (+ all notes off)
#define MIDI_CONTROL_OMNI_MODE_ON                      0x7D // 0 (+ all notes off)
#define MIDI_CONTROL_MONO_MODE_ON                      0x7E // ...
#define MIDI_CONTROL_POLY_MODE_ON                      0x7F // 0 ( + mono off, + all notes off)

#define MIDI_CONTROL_SOUND_VARIATION                   MIDI_CONTROL_SOUND_CONTROLLER_1
#define MIDI_CONTROL_TIMBRE                            MIDI_CONTROL_SOUND_CONTROLLER_2
#define MIDI_CONTROL_RELEASE_TIME                      MIDI_CONTROL_SOUND_CONTROLLER_3
#define MIDI_CONTROL_ATTACK_TIME                       MIDI_CONTROL_SOUND_CONTROLLER_4
#define MIDI_CONTROL_BRIGHTNESS                        MIDI_CONTROL_SOUND_CONTROLLER_5
#define MIDI_CONTROL_DECAY_TIME                        MIDI_CONTROL_SOUND_CONTROLLER_6
#define MIDI_CONTROL_VIBRATO_RATE                      MIDI_CONTROL_SOUND_CONTROLLER_7
#define MIDI_CONTROL_VIBRATO_DEPTH                     MIDI_CONTROL_SOUND_CONTROLLER_8
#define MIDI_CONTROL_VIBRATO_DELAY                     MIDI_CONTROL_SOUND_CONTROLLER_9
#define MIDI_CONTROL_REVERB_SEND_LEVEL                 MIDI_CONTROL_EFFECTS_1_DEPTH
#define MIDI_CONTROL_TREMOLO_DEPTH                     MIDI_CONTROL_EFFECTS_2_DEPTH
#define MIDI_CONTROL_CHORUS_SEND_LEVEL                 MIDI_CONTROL_EFFECTS_3_DEPTH
#define MIDI_CONTROL_DETUNE_DEPTH                      MIDI_CONTROL_EFFECTS_4_DEPTH
#define MIDI_CONTROL_PHASER_DEPTH                      MIDI_CONTROL_EFFECTS_5_DEPTH

#define MIDI_IS_CONTROL_BANK_SELECT(control)           ((control) == MIDI_CONTROL_BANK_SELECT           || (control) == MIDI_CONTROL_BANK_SELECT__LSB)
#define MIDI_IS_CONTROL_MODULATION_WHEEL(control)      ((control) == MIDI_CONTROL_MODULATION_WHEEL      || (control) == MIDI_CONTROL_MODULATION_WHEEL__LSB)
#define MIDI_IS_CONTROL_BREATH_CONTROLLER(control)     ((control) == MIDI_CONTROL_BREATH_CONTROLLER     || (control) == MIDI_CONTROL_BREATH_CONTROLLER__LSB)
#define MIDI_IS_CONTROL_FOOT_CONTROLLER(control)       ((control) == MIDI_CONTROL_FOOT_CONTROLLER       || (control) == MIDI_CONTROL_FOOT_CONTROLLER__LSB)
#define MIDI_IS_CONTROL_PORTAMENTO_TIME(control)       ((control) == MIDI_CONTROL_PORTAMENTO_TIME       || (control) == MIDI_CONTROL_PORTAMENTO_TIME__LSB)
#define MIDI_IS_CONTROL_DATA_ENTRY(control)            ((control) == MIDI_CONTROL_DATA_ENTRY            || (control) == MIDI_CONTROL_DATA_ENTRY__LSB)
#define MIDI_IS_CONTROL_CHANNEL_VOLUME(control)        ((control) == MIDI_CONTROL_CHANNEL_VOLUME        || (control) == MIDI_CONTROL_CHANNEL_VOLUME__LSB)
#define MIDI_IS_CONTROL_BALANCE(control)               ((control) == MIDI_CONTROL_BALANCE               || (control) == MIDI_CONTROL_BALANCE__LSB)
#define MIDI_IS_CONTROL_PAN(control)                   ((control) == MIDI_CONTROL_PAN                   || (control) == MIDI_CONTROL_PAN__LSB)
#define MIDI_IS_CONTROL_EXPRESSION_CONTROLLER(control) ((control) == MIDI_CONTROL_EXPRESSION_CONTROLLER || (control) == MIDI_CONTROL_EXPRESSION_CONTROLLER__LSB)
#define MIDI_IS_CONTROL_EFFECT_CONTROL_1(control)      ((control) == MIDI_CONTROL_EFFECT_CONTROL_1      || (control) == MIDI_CONTROL_EFFECT_CONTROL_1__LSB)
#define MIDI_IS_CONTROL_EFFECT_CONTROL_2(control)      ((control) == MIDI_CONTROL_EFFECT_CONTROL_2      || (control) == MIDI_CONTROL_EFFECT_CONTROL_2__LSB)

#endif // __CARLA_MIDI_H__
