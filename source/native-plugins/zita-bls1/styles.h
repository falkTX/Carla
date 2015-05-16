// ----------------------------------------------------------------------
//
//  Copyright (C) 2011 Fons Adriaensen <fons@linuxaudio.org>
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
#include "rotary.h"

namespace BLS1 {


enum
{
    C_MAIN_BG, C_MAIN_FG,
    C_TEXT_BG, C_TEXT_FG,
    NXFTCOLORS
};

enum
{
    F_TEXT,
    NXFTFONTS
};


extern int  styles_init (X_display *disp, X_resman *xrm);
extern void styles_fini (X_display *disp);

extern XftColor  *XftColors [NXFTCOLORS];
extern XftFont   *XftFonts [NXFTFONTS];

extern X_textln_style tstyle1;

extern XImage    *inputsect;
extern XImage    *shuffsect;
extern XImage    *lfshfsect;
extern RotaryImg  inpbal_img;
extern RotaryImg  hpfilt_img;
extern RotaryImg  shfreq_img;
extern RotaryImg  shgain_img;
extern RotaryImg  lffreq_img;
extern RotaryImg  lfgain_img;


}

#endif
