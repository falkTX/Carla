// ----------------------------------------------------------------------
//
//  Copyright (C) 2010-2014 Fons Adriaensen <fons@linuxaudio.org>
//  Modified by falkTX on Jan 2015 for inclusion in Carla
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


#include "styles.h"
#include "tmeter.h"
#include "png2img.h"

#include "CarlaString.hpp"

namespace AT1 {


XftColor      *XftColors [NXFTCOLORS];
XftFont       *XftFonts [NXFTFONTS];

X_textln_style tstyle1;
X_button_style bstyle1;

XImage    *notesect_img;
XImage    *ctrlsect_img;
XImage    *redzita_img;

ButtonImg  b_note_img;
ButtonImg  b_midi_img;

RotaryGeom  r_tune_geom;
RotaryGeom  r_filt_geom;
RotaryGeom  r_bias_geom;
RotaryGeom  r_corr_geom;
RotaryGeom  r_offs_geom;


int styles_init (X_display *disp, X_resman *xrm, const char *resdir)
{
    XftColors [C_MAIN_BG] = disp->alloc_xftcolor (0.25f, 0.25f, 0.25f, 1.0f);
    XftColors [C_MAIN_FG] = disp->alloc_xftcolor (1.0f, 1.0f, 1.0f, 1.0f);
    XftColors [C_TEXT_BG] = disp->alloc_xftcolor (1.0f, 1.0f, 1.0f, 1.0f);
    XftColors [C_TEXT_FG] = disp->alloc_xftcolor (0.1f, 0.1f, 0.1f, 1.0f);

    XftFonts [F_TEXT] = disp->alloc_xftfont (xrm->get (".font.text", "luxi:bold::pixelsize=11"));
    XftFonts [F_BUTT] = disp->alloc_xftfont (xrm->get (".font.butt", "luxi:bold::pixelsize=11"));

    tstyle1.font = XftFonts [F_TEXT];
    tstyle1.color.normal.bgnd = XftColors [C_TEXT_BG]->pixel;
    tstyle1.color.normal.text = XftColors [C_TEXT_FG];

    bstyle1.font =  XftFonts [F_BUTT];
    bstyle1.color.bg[0] = XftColors [C_MAIN_BG]->pixel;
    bstyle1.color.fg[0] = XftColors [C_MAIN_FG];
    bstyle1.type = X_button_style::PLAIN | X_button_style::ALEFT;

    const CarlaString SHARED = CarlaString(resdir)+"/at1";
    notesect_img = png2img (SHARED+"/notesect.png", disp, XftColors [C_MAIN_BG]);
    ctrlsect_img = png2img (SHARED+"/ctrlsect.png", disp, XftColors [C_MAIN_BG]);
    Tmeter::_scale = png2img (SHARED+"/hscale.png",  disp, XftColors [C_MAIN_BG]);
    Tmeter::_imag0 = png2img (SHARED+"/hmeter0.png", disp, XftColors [C_MAIN_BG]);
    Tmeter::_imag1 = png2img (SHARED+"/hmeter1.png", disp, XftColors [C_MAIN_BG]);

    if (   !notesect_img || !ctrlsect_img
	|| !Tmeter::_scale || !Tmeter::_imag0 || !Tmeter::_imag1) 
    {
	fprintf (stderr, "Can't load images from '%s'.\n", SHARED.buffer());
	return 1;
    }

    b_midi_img._backg = XftColors [C_MAIN_BG];
    b_midi_img._ximage = png2img (SHARED+"/midi.png", disp, XftColors [C_MAIN_BG]);
    b_midi_img._x0 = 0;
    b_midi_img._y0 = 0;
    b_midi_img._dx = 40;
    b_midi_img._dy = 24;

    b_note_img._backg = XftColors [C_MAIN_BG];
    b_note_img._ximage = png2img (SHARED+"/note.png", disp, XftColors [C_MAIN_BG]);
    b_note_img._x0 = 0;
    b_note_img._y0 = 0;
    b_note_img._dx = 16;
    b_note_img._dy = 16;

    r_tune_geom._backg = XftColors [C_MAIN_BG];
    r_tune_geom._image [0] = ctrlsect_img;
    r_tune_geom._lncol [0] = 1;
    r_tune_geom._x0 = 26;
    r_tune_geom._y0 = 17;
    r_tune_geom._dx = 23;
    r_tune_geom._dy = 23;
    r_tune_geom._xref = 11.5;
    r_tune_geom._yref = 11.5;
    r_tune_geom._rad = 11;

    r_bias_geom._backg = XftColors [C_MAIN_BG];
    r_bias_geom._image [0] = ctrlsect_img;
    r_bias_geom._lncol [0] = 0;
    r_bias_geom._x0 = 86;
    r_bias_geom._y0 = 17;
    r_bias_geom._dx = 23;
    r_bias_geom._dy = 23;
    r_bias_geom._xref = 11.5;
    r_bias_geom._yref = 11.5;
    r_bias_geom._rad = 11;

    r_filt_geom._backg = XftColors [C_MAIN_BG];
    r_filt_geom._image [0] = ctrlsect_img;
    r_filt_geom._lncol [0] = 0;
    r_filt_geom._x0 = 146;
    r_filt_geom._y0 = 17;
    r_filt_geom._dx = 23;
    r_filt_geom._dy = 23;
    r_filt_geom._xref = 11.5;
    r_filt_geom._yref = 11.5;
    r_filt_geom._rad = 11;

    r_corr_geom._backg = XftColors [C_MAIN_BG];
    r_corr_geom._image [0] = ctrlsect_img;
    r_corr_geom._lncol [0] = 0;
    r_corr_geom._x0 = 206;
    r_corr_geom._y0 = 17;
    r_corr_geom._dx = 23;
    r_corr_geom._dy = 23;
    r_corr_geom._xref = 11.5;
    r_corr_geom._yref = 11.5;
    r_corr_geom._rad = 11;

    r_offs_geom._backg = XftColors [C_MAIN_BG];
    r_offs_geom._image [0] = ctrlsect_img;
    r_offs_geom._lncol [0] = 0;
    r_offs_geom._x0 = 266;
    r_offs_geom._y0 = 17;
    r_offs_geom._dx = 23;
    r_offs_geom._dy = 23;
    r_offs_geom._xref = 11.5;
    r_offs_geom._yref = 11.5;
    r_offs_geom._rad = 11;

    return 0;
}


void styles_fini (X_display *disp)
{
}


}
