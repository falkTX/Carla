// ----------------------------------------------------------------------
//
//  Copyright (C) 2010-2014 Fons Adriaensen <fons@linuxaudio.org>
//  Modified by falkTX on Jan-Apr 2015 for inclusion in Carla
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// ----------------------------------------------------------------------


#ifndef __STYLES_H
#define __STYLES_H

#include <clxclient.h>
#include "button.h"
#include "rotary.h"

namespace AT1 {


enum
{
    C_MAIN_BG, C_MAIN_FG,
    C_TEXT_BG, C_TEXT_FG,
    NXFTCOLORS
};

enum
{
    F_TEXT,
    F_BUTT,
    NXFTFONTS
};


extern void styles_init (X_display *disp, X_resman *xrm);
extern void styles_fini (X_display *disp);

extern XftColor  *XftColors [NXFTCOLORS];
extern XftFont   *XftFonts [NXFTFONTS];

extern X_textln_style tstyle1;
extern X_button_style bstyle1;

extern XImage     *notesect_img;
extern XImage     *ctrlsect_img;
extern ButtonImg   b_midi_img;
extern ButtonImg   b_note_img;
extern RotaryGeom  r_tune_geom;
extern RotaryGeom  r_filt_geom;
extern RotaryGeom  r_bias_geom;
extern RotaryGeom  r_corr_geom;
extern RotaryGeom  r_offs_geom;


}

#endif
