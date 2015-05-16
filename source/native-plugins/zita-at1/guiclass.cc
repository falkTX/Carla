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


#include <math.h>
#include "guiclass.h"

namespace AT1 {


int Pbutt0::handle_press (void)
{
    _state |= 2;
    return PRESS;
}

int Pbutt0::handle_relse (void)
{
    _state &= ~2;
    return NOP;
}


int Pbutt1::handle_press (void)
{
    _state ^= 1;
    return PRESS;
}

int Pbutt1::handle_relse (void)
{
    return NOP;
}



Rlinctl::Rlinctl (X_window    *parent,
                  X_callback  *cbobj,
                  int          cbind,
                  RotaryGeom  *rgeom,
                  int          xp,
                  int          yp,
		  int          cm,
		  int          dd,
                  double       vmin,
	          double       vmax,
	          double       vini) :
RotaryCtl (parent, cbobj, cbind, rgeom, xp, yp),
_cm (cm),
_dd (dd),
_vmin (vmin),
_vmax (vmax),
_form (0)
{
    _count = -1;
    set_value (vini);
}

void Rlinctl::get_string (char *p, int n)
{
    if (_form) snprintf (p, n, _form, _value);
    else *p = 0;
}

void Rlinctl::set_value (double v)
{
    set_count ((int) floor (_cm * (v - _vmin) / (_vmax - _vmin) + 0.5));
    render ();
}

int Rlinctl::handle_button (void)
{
    return PRESS;
}

int Rlinctl::handle_motion (int dx, int dy)
{
    return set_count (_rcount + dx - dy);
}

int Rlinctl::handle_mwheel (int dw)
{
    if (! (_keymod & ShiftMask)) dw *= _dd;
    return set_count (_count + dw);
}

int Rlinctl::set_count (int u)
{
    if (u <   0) u=    0;
    if (u > _cm) u = _cm;
    if (u != _count)
    {
	_count = u;
	_value = _vmin + u * (_vmax - _vmin) / _cm;
	_angle = 270.0 * ((double) u / _cm - 0.5);
        return DELTA;
    }
    return 0;
}



Rlogctl::Rlogctl (X_window    *parent,
                  X_callback  *cbobj,
                  int          cbind,
                  RotaryGeom  *rgeom,
                  int          xp,
                  int          yp,
		  int          cm,
		  int          dd,
                  double       vmin,
	          double       vmax,
	          double       vini) :
RotaryCtl (parent, cbobj, cbind, rgeom, xp, yp),
_cm (cm),
_dd (dd),
_form (0)
{
    _count = -1;
    _vmin = log (vmin);
    _vmax = log (vmax);
    set_value (vini);
}

void Rlogctl::get_string (char *p, int n)
{
    if (_form) snprintf (p, n, _form, _value);
    else *p = 0;
}

void Rlogctl::set_value (double v)
{
    set_count ((int) floor (_cm * (log (v) - _vmin) / (_vmax - _vmin) + 0.5));
    render ();
}

int Rlogctl::handle_button (void)
{
    return PRESS;
}

int Rlogctl::handle_motion (int dx, int dy)
{
    return set_count (_rcount + dx - dy);
}

int Rlogctl::handle_mwheel (int dw)
{
    if (! (_keymod & ShiftMask)) dw *= _dd;
    return set_count (_count + dw);
}

int Rlogctl::set_count (int u)
{
    if (u <   0) u=    0;
    if (u > _cm) u = _cm;
    if (u != _count)
    {
	_count = u;
	_value = exp (_vmin + u * (_vmax - _vmin) / _cm);
	_angle = 270.0 * ((double) u / _cm - 0.5);
        return DELTA;
    }
    return 0;
}


}
