/*
    Copyright (C) 2009-2010 Fons Adriaensen <fons@linuxaudio.org>
    Modified by falkTX on Jan-Apr 2015 for inclusion in Carla
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include <math.h>
#include "tmeter.h"

namespace AT1 {


XImage  *Tmeter::_scale = 0;
XImage  *Tmeter::_imag0 = 0;
XImage  *Tmeter::_imag1 = 0;


Tmeter::Tmeter (X_window *parent, int xpos, int ypos) :
    X_window (parent, xpos, ypos, XS + 2 * XM, YS + 2 * YM, 0),
    _k0 (86),
    _k1 (86)
{
    if (!_imag0 || !_imag1 || !_scale) return;
    x_add_events (ExposureMask);
}


Tmeter::~Tmeter (void)
{
}


void Tmeter::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case Expose:
	expose ((XExposeEvent *) E);
	break;  
    }
}


void Tmeter::expose (XExposeEvent *E)
{
    if (E->count) return;
    XSetFunction (dpy (), dgc (), GXcopy);
    XPutImage (dpy (), win (), dgc (), _imag0, 0, 0, XM, YM, XS, Y1); 
    XPutImage (dpy (), win (), dgc (), _imag1, _k0 - 2, 0, XM + _k0 - 2, YM, 5 + _k1 - _k0, Y1); 
    XPutImage (dpy (), win (), dgc (), _scale, 0, 0, XM, YM + Y1, XS, Y2); 
}


void Tmeter::update (float v0, float v1)
{
    int k0, k1;

    k0 = (int)(floorf (86.0f + 80.0f * v0 + 0.5f));
    k1 = (int)(floorf (86.0f + 80.0f * v1 + 0.5f));
    if (k0 < 4) k0 = 4;
    if (k0 > 168) k0 = 168;
    if (k1 < 4) k1 = 4;
    if (k1 > 168) k1 = 168;
    XSetFunction (dpy (), dgc (), GXcopy);
    XPutImage (dpy (), win (), dgc (), _imag0, _k0 - 2, 0, XM + _k0 - 2, YM, 5 + _k1 - _k0, Y1); 
    _k0 = k0;
    _k1 = k1;
    XPutImage (dpy (), win (), dgc (), _imag1, _k0 - 2, 0, XM + _k0 - 2, YM, 5 + _k1 - _k0, Y1); 
}


}
