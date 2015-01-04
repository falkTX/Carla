/* nekobee DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Sean Bolton and others.
 *
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

#ifndef _XSYNTH_H
#define _XSYNTH_H

/* ==== debugging ==== */

/* XSYNTH_DEBUG bits */
#define XDB_DSSI   1   /* DSSI interface */
#define XDB_AUDIO  2   /* audio output */
#define XDB_NOTE   4   /* note on/off, voice allocation */
#define XDB_DATA   8   /* plugin patchbank handling */
#define GDB_MAIN  16   /* GUI main program flow */
#define GDB_OSC   32   /* GUI OSC handling */
#define GDB_IO    64   /* GUI patch file input/output */
#define GDB_GUI  128   /* GUI GUI callbacks, updating, etc. */

/* If you want debug information, define XSYNTH_DEBUG to the XDB_* bits you're
 * interested in getting debug information about, bitwise-ORed together.
 * Otherwise, leave it undefined. */
// #define XSYNTH_DEBUG (1+8+16+32+64)

//#define XSYNTH_DEBUG GDB_GUI + GDB_OSC

// #define XSYNTH_DEBUG XDB_DSSI
#ifdef XSYNTH_DEBUG

#include <stdio.h>
#define XSYNTH_DEBUG_INIT(x)
#define XDB_MESSAGE(type, fmt...) { if (XSYNTH_DEBUG & type) fprintf(stderr, "nekobee-dssi.so" fmt); }
#define GDB_MESSAGE(type, fmt...) { if (XSYNTH_DEBUG & type) fprintf(stderr, "nekobee_gtk" fmt); }
// -FIX-:
// #include "message_buffer.h"
// #define XSYNTH_DEBUG_INIT(x)  mb_init(x)
// #define XDB_MESSAGE(type, fmt...) { \-
//     if (XSYNTH_DEBUG & type) { \-
//         char _m[256]; \-
//         snprintf(_m, 255, fmt); \-
//         add_message(_m); \-
//     } \-
// }

#else  /* !XSYNTH_DEBUG */

#define XDB_MESSAGE(type, fmt...)
#define GDB_MESSAGE(type, fmt...)
#define XSYNTH_DEBUG_INIT(x)

#endif  /* XSYNTH_DEBUG */

/* ==== end of debugging ==== */

#define XSYNTH_MAX_POLYPHONY     1
#define XSYNTH_DEFAULT_POLYPHONY  1

#endif /* _XSYNTH_H */
