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


#include "styles.h"
#include "png2img.h"

#include "CarlaString.hpp"
#include <dlfcn.h>

namespace BLS1 {


XftColor      *XftColors [NXFTCOLORS];
XftFont       *XftFonts [NXFTFONTS];

X_textln_style tstyle1;

XImage    *inputsect;
XImage    *shuffsect;
XImage    *lfshfsect;
RotaryImg  inpbal_img;
RotaryImg  hpfilt_img;
RotaryImg  shfreq_img;
RotaryImg  shgain_img;
RotaryImg  lffreq_img;
RotaryImg  lfgain_img;


static CarlaString getResourceDir()
{
    Dl_info exeInfo;
    dladdr((void*)getResourceDir, &exeInfo);

    CarlaString filename(exeInfo.dli_fname);
    return filename.truncate(filename.rfind("-ui"));
}


int styles_init (X_display *disp, X_resman *xrm)
{
    CarlaString resourceDir(getResourceDir());

    XftColors [C_MAIN_BG] = disp->alloc_xftcolor (0.25f, 0.25f, 0.25f, 1.0f);
    XftColors [C_MAIN_FG] = disp->alloc_xftcolor (1.0f, 1.0f, 1.0f, 1.0f);
    XftColors [C_TEXT_BG] = disp->alloc_xftcolor (1.0f, 1.0f, 0.0f, 1.0f);
    XftColors [C_TEXT_FG] = disp->alloc_xftcolor (0.0f, 0.0f, 0.0f, 1.0f);

    XftFonts [F_TEXT] = disp->alloc_xftfont (xrm->get (".font.text", "luxi:bold::pixelsize=11"));

    tstyle1.font = XftFonts [F_TEXT];
    tstyle1.color.normal.bgnd = XftColors [C_TEXT_BG]->pixel;
    tstyle1.color.normal.text = XftColors [C_TEXT_FG];

    inputsect = png2img (resourceDir+"/inputsect.png", disp, XftColors [C_MAIN_BG]);
    shuffsect = png2img (resourceDir+"/shuffsect.png", disp, XftColors [C_MAIN_BG]);
    lfshfsect = png2img (resourceDir+"/lfshfsect.png", disp, XftColors [C_MAIN_BG]);
    if (!inputsect || !shuffsect || !lfshfsect) return 1;

    inpbal_img._backg = XftColors [C_MAIN_BG];
    inpbal_img._image [0] = inputsect;
    inpbal_img._lncol [0] = 1;
    inpbal_img._x0 = 28;
    inpbal_img._y0 = 17;
    inpbal_img._dx = 25;
    inpbal_img._dy = 25;
    inpbal_img._xref = 12.5;
    inpbal_img._yref = 12.5;
    inpbal_img._rad = 12;

    hpfilt_img._backg = XftColors [C_MAIN_BG];
    hpfilt_img._image [0] = inputsect;
    hpfilt_img._lncol [0] = 0;
    hpfilt_img._x0 = 87;
    hpfilt_img._y0 = 17;
    hpfilt_img._dx = 25;
    hpfilt_img._dy = 25;
    hpfilt_img._xref = 12.5;
    hpfilt_img._yref = 12.5;
    hpfilt_img._rad = 12;

    shgain_img._backg = XftColors [C_MAIN_BG];
    shgain_img._image [0] = shuffsect;
    shgain_img._lncol [0] = 0;
    shgain_img._x0 = 68;
    shgain_img._y0 = 17;
    shgain_img._dx = 25;
    shgain_img._dy = 25;
    shgain_img._xref = 12.5;
    shgain_img._yref = 12.5;
    shgain_img._rad = 12;
 
    shfreq_img._backg = XftColors [C_MAIN_BG];
    shfreq_img._image [0] = shuffsect;
    shfreq_img._lncol [0] = 0;
    shfreq_img._x0 = 127;
    shfreq_img._y0 = 17;
    shfreq_img._dx = 25;
    shfreq_img._dy = 25;
    shfreq_img._xref = 12.5;
    shfreq_img._yref = 12.5;
    shfreq_img._rad = 12;
 
    lffreq_img._backg = XftColors [C_MAIN_BG];
    lffreq_img._image [0] = lfshfsect;
    lffreq_img._lncol [0] = 0;
    lffreq_img._x0 = 14;
    lffreq_img._y0 = 31;
    lffreq_img._dx = 25;
    lffreq_img._dy = 25;
    lffreq_img._xref = 12.5;
    lffreq_img._yref = 12.5;
    lffreq_img._rad = 12;

    lfgain_img._backg = XftColors [C_MAIN_BG];
    lfgain_img._image [0] = lfshfsect;
    lfgain_img._lncol [0] = 1;
    lfgain_img._x0 = 63;
    lfgain_img._y0 = 17;
    lfgain_img._dx = 25;
    lfgain_img._dy = 25;
    lfgain_img._xref = 12.5;
    lfgain_img._yref = 12.5;
    lfgain_img._rad = 12;

    return 0;
}


void styles_fini (X_display *disp)
{
}


}
