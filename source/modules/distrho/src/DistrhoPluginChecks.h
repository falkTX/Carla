/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_PLUGIN_CHECKS_H_INCLUDED
#define DISTRHO_PLUGIN_CHECKS_H_INCLUDED

#include "DistrhoPluginInfo.h"

// -----------------------------------------------------------------------
// Check if all required macros are defined

#ifndef DISTRHO_PLUGIN_NAME
# error DISTRHO_PLUGIN_NAME undefined!
#endif

#ifndef DISTRHO_PLUGIN_NUM_INPUTS
# error DISTRHO_PLUGIN_NUM_INPUTS undefined!
#endif

#ifndef DISTRHO_PLUGIN_NUM_OUTPUTS
# error DISTRHO_PLUGIN_NUM_OUTPUTS undefined!
#endif

#ifndef DISTRHO_PLUGIN_URI
# error DISTRHO_PLUGIN_URI undefined!
#endif

// -----------------------------------------------------------------------
// Define optional macros if not done yet

#ifndef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#ifndef DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# define DISTRHO_PLUGIN_HAS_EXTERNAL_UI 0
#endif

#ifndef DISTRHO_PLUGIN_IS_RT_SAFE
# define DISTRHO_PLUGIN_IS_RT_SAFE 0
#endif

#ifndef DISTRHO_PLUGIN_IS_SYNTH
# define DISTRHO_PLUGIN_IS_SYNTH 0
#endif

#ifndef DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
# define DISTRHO_PLUGIN_WANT_DIRECT_ACCESS 0
#endif

#ifndef DISTRHO_PLUGIN_WANT_LATENCY
# define DISTRHO_PLUGIN_WANT_LATENCY 0
#endif

#ifndef DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
# define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 0
#endif

#ifndef DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
# define DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST 0
#endif

#ifndef DISTRHO_PLUGIN_WANT_PROGRAMS
# define DISTRHO_PLUGIN_WANT_PROGRAMS 0
#endif

#ifndef DISTRHO_PLUGIN_WANT_STATE
# define DISTRHO_PLUGIN_WANT_STATE 0
#endif

#ifndef DISTRHO_PLUGIN_WANT_FULL_STATE
# define DISTRHO_PLUGIN_WANT_FULL_STATE 0
# define DISTRHO_PLUGIN_WANT_FULL_STATE_WAS_NOT_SET
#endif

#ifndef DISTRHO_PLUGIN_WANT_TIMEPOS
# define DISTRHO_PLUGIN_WANT_TIMEPOS 0
#endif

#ifndef DISTRHO_UI_FILE_BROWSER
# if defined(DGL_FILE_BROWSER_DISABLED) || DISTRHO_PLUGIN_HAS_EXTERNAL_UI
#  define DISTRHO_UI_FILE_BROWSER 0
# else
#  define DISTRHO_UI_FILE_BROWSER 1
# endif
#endif

#ifndef DISTRHO_UI_USER_RESIZABLE
# define DISTRHO_UI_USER_RESIZABLE 0
#endif

#ifndef DISTRHO_UI_USE_NANOVG
# define DISTRHO_UI_USE_NANOVG 0
#endif

// -----------------------------------------------------------------------
// Define DISTRHO_PLUGIN_HAS_EMBED_UI if needed

#ifndef DISTRHO_PLUGIN_HAS_EMBED_UI
# if (defined(DGL_CAIRO) && defined(HAVE_CAIRO)) || (defined(DGL_OPENGL) && defined(HAVE_OPENGL))
#  define DISTRHO_PLUGIN_HAS_EMBED_UI 1
# else
#  define DISTRHO_PLUGIN_HAS_EMBED_UI 0
# endif
#endif

// -----------------------------------------------------------------------
// Define DISTRHO_UI_URI if needed

#ifndef DISTRHO_UI_URI
# define DISTRHO_UI_URI DISTRHO_PLUGIN_URI "#DPF_UI"
#endif

// -----------------------------------------------------------------------
// Test if synth has audio outputs

#if DISTRHO_PLUGIN_IS_SYNTH && DISTRHO_PLUGIN_NUM_OUTPUTS == 0
# error Synths need audio output to work!
#endif

// -----------------------------------------------------------------------
// Enable MIDI input if synth, test if midi-input disabled when synth

#ifndef DISTRHO_PLUGIN_WANT_MIDI_INPUT
# define DISTRHO_PLUGIN_WANT_MIDI_INPUT DISTRHO_PLUGIN_IS_SYNTH
#elif DISTRHO_PLUGIN_IS_SYNTH && ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
# error Synths need MIDI input to work!
#endif

// -----------------------------------------------------------------------
// Enable state if plugin wants state files (deprecated)

#ifdef DISTRHO_PLUGIN_WANT_STATEFILES
# warning DISTRHO_PLUGIN_WANT_STATEFILES is deprecated
# undef DISTRHO_PLUGIN_WANT_STATEFILES
# if ! DISTRHO_PLUGIN_WANT_STATE
#  undef DISTRHO_PLUGIN_WANT_STATE
#  define DISTRHO_PLUGIN_WANT_STATE 1
# endif
#endif

// -----------------------------------------------------------------------
// Enable full state if plugin exports presets

#if DISTRHO_PLUGIN_WANT_PROGRAMS && DISTRHO_PLUGIN_WANT_STATE && defined(DISTRHO_PLUGIN_WANT_FULL_STATE_WAS_NOT_SET)
# warning Plugins with programs and state should implement full state API too
# undef DISTRHO_PLUGIN_WANT_FULL_STATE
# define DISTRHO_PLUGIN_WANT_FULL_STATE 1
#endif

// -----------------------------------------------------------------------
// Disable file browser if using external UI

#if DISTRHO_UI_FILE_BROWSER && DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# warning file browser APIs do not work for external UIs
# undef DISTRHO_UI_FILE_BROWSER 0
# define DISTRHO_UI_FILE_BROWSER 0
#endif

// -----------------------------------------------------------------------
// Disable UI if DGL or external UI is not available

#if (defined(DGL_CAIRO) && ! defined(HAVE_CAIRO)) || (defined(DGL_OPENGL) && ! defined(HAVE_OPENGL))
# undef DISTRHO_PLUGIN_HAS_EMBED_UI
# define DISTRHO_PLUGIN_HAS_EMBED_UI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI && ! DISTRHO_PLUGIN_HAS_EMBED_UI && ! DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

// -----------------------------------------------------------------------
// Prevent users from messing about with DPF internals

#ifdef DISTRHO_UI_IS_STANDALONE
# error DISTRHO_UI_IS_STANDALONE must not be defined
#endif

// -----------------------------------------------------------------------

#endif // DISTRHO_PLUGIN_CHECKS_H_INCLUDED
