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


#include <string.h>
#include <math.h>
#include "hp3filt.h"

namespace BLS1 {


HP3filt::HP3filt (void) :
    _touch0 (0),
    _touch1 (0),
    _state (BYPASS),
    _f0 (0),
    _f1 (0),
    _c1 (0),
    _c2 (0),
    _c3 (0),
    _g (0),
    _d (0)
{
    setfsamp (0.0f);
}


HP3filt::~HP3filt (void)
{
}


void HP3filt::setfsamp (float fsamp)
{
    _fsamp = fsamp;
    reset ();
}


void HP3filt::reset (void)
{
    memset (_z1, 0, sizeof (float) * MAXCH); 
    memset (_z2, 0, sizeof (float) * MAXCH); 
    memset (_z3, 0, sizeof (float) * MAXCH); 
}


void HP3filt::prepare (int nsamp)
{
    float f;

    f = _f0; 
    if (_touch1 != _touch0)
    {
	if (_f1 == f)
	{
	    _touch1 = _touch0;
	    if (_state == FADING)
	    {
		_state = (_d > 0) ? STATIC : BYPASS;
	    }
	}
	else if (_f1 == 0)
	{
	    _f1 = f;
   	    _a = 0.0f;
   	    _d = 1.0f / nsamp;
            calcpar1 (0, _f1);
	    reset ();
	    _state = FADING;
        }
        else if (f == 0)
	{
	    _f1 = f;
	    _a = 1.0f;
	    _d = -1.0f / nsamp;
	    _state = FADING;
	}
        else
	{
            if (f > 1.25f * _f1) _f1 *= 1.25f;
            if (f < 0.80f * _f1) _f1 *= 0.80f;
            else _f1 = f;
            calcpar1 (0, _f1);
	}
    }
}


float HP3filt::response (float f)
{
    // Compute gain at frequency f from _c1 _c2, _c3, _g.
    // This is left as an exercise for the reader.
    return 0;
}


void HP3filt::calcpar1 (int nsamp, float f)
{
    float a, b, t;

    _g = 1;
    a = (float)(M_PI) * f / _fsamp;
    b = a * a;
    t = 1 + a + b;
    _g /= t;   
    _c1 = 2 * a + 4 * b;
    _c2 = 4 * b / _c1;
    _c1 /= t;
    t = 1 + a;
    _g /= t;
    _c3 = 2 * a / t;
}


void HP3filt::process1 (int nsamp, int nchan, float *data[])
{
    int    i, j;
    float  a, d, x, y, z1, z2, z3;
    float  *p;

    a = _a;
    d = _d;
    for (i = 0; i < nchan; i++)
    {
	p = data [i];
	z1 = _z1 [i];
	z2 = _z2 [i];
	z3 = _z3 [i];
	if (_state == FADING)
	{
	    a = _a;
	    for (j = 0; j < nsamp; j++)
	    {
		x = *p;
		y = x - z1 - z2 + 1e-20f;
		z2 += _c2 * z1;
		z1 += _c1 * y;
		y -= z3 - 1e-20f;
		z3 += _c3 * y;
		a += d;
		*p++ = a * (_g * y) + (1 - a) * x;
	    }
	}
	else
	{
	    for (j = 0; j < nsamp; j++)
	    {
		x = *p;
		y = x - z1 - z2 + 1e-20f;
		z2 += _c2 * z1;
		z1 += _c1 * y;
		y -= z3 - 1e-20f;
		z3 += _c3 * y;
		*p++ = _g * y;
	    }
	}
        _z1 [i] = z1;
        _z2 [i] = z2;
        _z3 [i] = z3;
    }
    if (_state == FADING) _a = a;
}


}
