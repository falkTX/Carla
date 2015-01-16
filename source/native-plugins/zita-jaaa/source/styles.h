// -------------------------------------------------------------------------
//
//  Copyright (C) 2004-2013 Fons Adriaensen <fons@linuxaudio.org>
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
// -------------------------------------------------------------------------


#ifndef __STYLES_H
#define __STYLES_H


#include <clxclient.h>


struct colors
{
    unsigned long   white;
    unsigned long   black;
    unsigned long   main_bg;
    unsigned long   main_ds;
    unsigned long   main_ls;
    unsigned long   text_ed;
    unsigned long   text_ca;
    unsigned long   spect_bg;
    unsigned long   spect_gr;
    unsigned long   spect_mk;
    unsigned long   spect_trA;
    unsigned long   spect_pkA;
    unsigned long   spect_trB;
    unsigned long   spect_pkB;
    unsigned long   spect_trM;
    unsigned long   butt_bg0;
    unsigned long   butt_bg1;
    unsigned long   butt_bg2;
    unsigned long   butt_bgA;
    unsigned long   butt_bgB;
    unsigned long   butt_bgM;
};


struct fonts 
{
};


struct xft_colors
{
    XftColor  *white;
    XftColor  *black;
    XftColor  *main_fg;
    XftColor  *text_fg;
    XftColor  *spect_fg;
    XftColor  *spect_sc;
    XftColor  *spect_an;
    XftColor  *butt_fg0;
    XftColor  *butt_fg1;
    XftColor  *butt_fg2;
    XftColor  *butt_fgA;
    XftColor  *butt_fgB;
    XftColor  *butt_fgM;
};


struct xft_fonts 
{
    XftFont   *about1;
    XftFont   *about2;
    XftFont   *button;
    XftFont   *labels;
    XftFont   *scales;
};



extern struct colors       Colors;
extern struct fonts        Fonts;
extern struct xft_colors   XftColors;
extern struct xft_fonts    XftFonts;
extern X_button_style      Bst0, Bst1, BstA, BstB, BstM;
extern X_textln_style      Tst0, Tst1;


extern void init_styles (X_display *disp, X_resman *xrm);


#define PROGNAME  "Jack/Alsa Audio Analyser"


#endif
