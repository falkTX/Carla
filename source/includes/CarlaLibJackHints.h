/*
 * Carla JACK API for external applications
 * Copyright (C) 2016-2020 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_LIBJACK_HINTS_H_INCLUDED
#define CARLA_LIBJACK_HINTS_H_INCLUDED

#include "CarlaDefines.h"

#ifdef __cplusplus
extern "C" {
#endif

enum SetupHints {
    // Application Window management
    LIBJACK_FLAG_CONTROL_WINDOW              = 0x01,
    LIBJACK_FLAG_CAPTURE_FIRST_WINDOW        = 0x02,
    // Audio/MIDI Buffers management
    LIBJACK_FLAG_AUDIO_BUFFERS_ADDITION      = 0x10,
    LIBJACK_FLAG_MIDI_OUTPUT_CHANNEL_MIXDOWN = 0x20,
    // Developer options, not saved on disk
    LIBJACK_FLAG_EXTERNAL_START              = 0x40,
};

enum SessionManager {
    LIBJACK_SESSION_MANAGER_NONE   = 0,
    LIBJACK_SESSION_MANAGER_AUTO   = 1,
    LIBJACK_SESSION_MANAGER_JACK   = 2,
    LIBJACK_SESSION_MANAGER_LADISH = 3,
    LIBJACK_SESSION_MANAGER_NSM    = 4,
};

enum InterposerAction {
    LIBJACK_INTERPOSER_ACTION_NONE = 0,
    LIBJACK_INTERPOSER_ACTION_SET_HINTS_AND_CALLBACK,
    LIBJACK_INTERPOSER_ACTION_SET_SESSION_MANAGER,
    LIBJACK_INTERPOSER_ACTION_SHOW_HIDE_GUI,
    LIBJACK_INTERPOSER_ACTION_CLOSE_EVERYTHING,
};

enum InterposerCallbacks {
    LIBJACK_INTERPOSER_CALLBACK_NONE = 0,
    LIBJACK_INTERPOSER_CALLBACK_UI_HIDE,
};

typedef int (*CarlaInterposedCallback)(int, void*);

CARLA_API
int jack_carla_interposed_action(uint action, uint value, void* ptr);

#ifdef __cplusplus
}
#endif

#endif // CARLA_LIBJACK_HINTS_H_INCLUDED
