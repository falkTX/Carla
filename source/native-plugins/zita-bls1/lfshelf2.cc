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


#include <math.h>
#include <string.h>
#include "lfshelf2.h"

namespace BLS1 {


LFshelf2::LFshelf2 (void) :
    _touch0 (0),
    _touch1 (0),
    _bypass (true),
    _state (BYPASS),
    _g0 (1),
    _g1 (1),
    _f0 (1e2f),
    _f1 (1e2f),
    _s0 (0),
    _s1 (0)
{
    setfsamp (0.0f);
}


LFshelf2::~LFshelf2 (void)
{
}


void LFshelf2::setfsamp (float fsamp)
{
    _fsamp = fsamp;
    reset ();
}


void LFshelf2::reset (void)
{
    memset (_z1, 0, sizeof (float) * MAXCH); 
    memset (_z2, 0, sizeof (float) * MAXCH); 
}


void LFshelf2::prepare (int nsamp)
{
    bool  upd = false;
    float g, f, s;

    if (_touch1 != _touch0)
    {
	g = _bypass ? 1 : _g0;
	f = _f0;
	s = _s0;
        if (g != _g1)
	{
	    upd = true;
 	    if      (g > 2 * _g1) _g1 *= 2;
	    else if (_g1 > 2 * g) _g1 /= 2;
	    else                  _g1 = g;
	}
        if (f != _f1)
	{
	    upd = true;
 	    if      (f > 2 * _f1) _f1 *= 2;
	    else if (_f1 > 2 * f) _f1 /= 2;
	    else                  _f1 = f;
	}
        if (s != _s1)
	{
	    upd = true;
 	    if      (s > _s1 + 0.3f) _s1 += 0.3f;
	    else if (s < _s1 - 0.3f) _s1 -= 0.3f;
	    else                     _s1 = s;
	}
	if (upd) 
	{
	    if ((_state == BYPASS) && (_g1 == 1))
	    {
		calcpar1 (0, _g1, _f1, _s1);
	    }
	    else
	    {
		_state = SMOOTH;
		calcpar1 (nsamp, _g1, _f1, _s1);
	    }
	}
	else
	{
	    _touch1 = _touch0;
            if (fabs (_g1 - 1) < 0.001f)
            {
	        _state = BYPASS;
	        reset ();
	    }
  	    else    
	    {
	        _state = STATIC;
	    }
	}
    }
}


float LFshelf2::response (float f)
{
    // Compute gain at frequency f from _a0, _a1, _a2, _b1, _b2.
    // This is left as an exercise for the reader.
    return 0;
}


void LFshelf2::calcpar1 (int nsamp, float g, float f, float s)
{
    bool  inv;
    float a0, a1, a2, b1, b2;
    float r, w1, w2, c1, c2, c3, c4, d1, d2;

    inv = (g < 1.0f);
    if (inv) g = 1.0f / g;
    w1 = 2 * M_PI * f / _fsamp;
    w2 = w1 * sqrtf (g);
    s *= (g - 1) / g;
    d1 = 1.8f - 0.55f * s / (1 + 2 * w1);
    d2 = 1.8f - 1.50f * s / (1 + 2 * w2);
    if (inv)
    {
	c1 = w1 * w1;
        c2 = d1 * w1;
	c3 = w2 * w2;
        c4 = d2 * w2;
    }
    else
    {
	c1 = w2 * w2;
        c2 = d2 * w2;
	c3 = w1 * w1;
        c4 = d1 * w1;
    }
    r = c3 + 2 * c4 + 4;
    b1 = 4 * (c4 - c3) / r;
    b2 = 4 * c3 / r;
    a0 = (c1 + 2 * c2 + 4) / r - 1;
    a1 = 4 * (c2 - c1) / r - b1;
    a2 = 4 * c1 / r - b2;
    if (nsamp)
    {
	_da0 = (a0 - _a0) / nsamp + 1e-30f;
	_da1 = (a1 - _a1) / nsamp + 1e-30f;
	_da2 = (a2 - _a2) / nsamp + 1e-30f;
	_db1 = (b1 - _b1) / nsamp + 1e-30f;
	_db2 = (b2 - _b2) / nsamp + 1e-30f;
    }
    else
    {
	_a0 = a0;
	_a1 = a1;
	_a2 = a2;
	_b1 = b1;
	_b2 = b2;
    }
}


void LFshelf2::process1 (int nsamp, int nchan, float *data[])
{
    int   i, j;
    float a0, a1, a2, b1, b2;
    float x, y, z1, z2;
    float *p;

    a0 = _a0;
    a1 = _a1;
    a2 = _a2;
    b1 = _b1;
    b2 = _b2;
    if (_state == SMOOTH)
    {
	for (i = 0; i < nchan; i++)
	{
	    p = data [i];
            z1 = _z1 [i];
            z2 = _z2 [i];
            a0 = _a0;
            a1 = _a1;
            a2 = _a2;
            b1 = _b1;
            b2 = _b2;
            for (j = 0; j < nsamp; j++)
            {
                a0 += _da0;
                a1 += _da1;
                a2 += _da2;
                b1 += _db1;
                b2 += _db2;
	        x = *p;
	        y = x - b1 * z1 - b2 * z2 + 1e-10f;
	        *p++ = x + a0 * y + a1 * z1 + a2 * z2;
	        z2 += z1;
	        z1 += y;
	    }
            _z1 [i] = z1;
            _z2 [i] = z2;
	}
        _a0 = a0;
        _a1 = a1;
        _a2 = a2;
        _b1 = b1;
        _b2 = b2;
    }
    else
    {
	for (i = 0; i < nchan; i++)
	{
	    p = data [i];
            z1 = _z1 [i];
            z2 = _z2 [i];
            for (j = 0; j < nsamp; j++)
            {
	        x = *p;
	        y = x - b1 * z1 - b2 * z2 + 1e-10f;
	        *p++ = x + a0 * y + a1 * z1 + a2 * z2;
	        z2 += z1;
	        z1 += y;
	    }
            _z1 [i] = z1;
            _z2 [i] = z2;
	}
    }
}


}
