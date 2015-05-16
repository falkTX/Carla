// ----------------------------------------------------------------------
//
//  Copyright (C) 2010 Fons Adriaensen <fons@linuxaudio.org>
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
#include "pareq.h"

namespace REV1 {


Pareq::Pareq (void) :
    _touch0 (0),
    _touch1 (0),
    _state (BYPASS),
    _g0 (1),
    _g1 (1),
    _f0 (1e3f),
    _f1 (1e3f)
{
    setfsamp (0.0f);
}


Pareq::~Pareq (void)
{
}


void Pareq::setfsamp (float fsamp)
{
    _fsamp = fsamp;
    reset ();
}


void Pareq::reset (void)
{
    memset (_z1, 0, sizeof (float) * MAXCH); 
    memset (_z2, 0, sizeof (float) * MAXCH); 
}


void Pareq::prepare (int nsamp)
{
    bool  upd = false;
    float g, f;

    if (_touch1 != _touch0)
    {
	g = _g0;
	f = _f0;
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
	if (upd) 
	{
	    if ((_state == BYPASS) && (_g1 == 1))
	    {
		calcpar1 (0, _g1, _f1);
	    }
	    else
	    {
		_state = SMOOTH;
		calcpar1 (nsamp, _g1, _f1);
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


void Pareq::calcpar1 (int nsamp, float g, float f)
{
    float b, c1, c2, gg;

    f *= float (M_PI) / _fsamp;
    b = 2 * f / sqrtf (g);         
    gg = 0.5f * (g - 1);
    c1 = -cosf (2 * f);
    c2 = (1 - b) / (1 + b);
    if (nsamp)
    {
	_dc1 = (c1 - _c1) / nsamp + 1e-30f;
	_dc2 = (c2 - _c2) / nsamp + 1e-30f;
	_dgg = (gg - _gg) / nsamp + 1e-30f;
    }
    else
    {
	_c1 = c1;
	_c2 = c2;
	_gg = gg;
    }
}


void Pareq::process1 (int nsamp, int nchan, float *data[])
{
    int   i, j;
    float c1, c2, gg;
    float x, y, z1, z2;
    float *p;

    c1 = _c1;
    c2 = _c2;
    gg = _gg;
    if (_state == SMOOTH)
    {
	for (i = 0; i < nchan; i++)
	{
	    p = data [i];
            z1 = _z1 [i];
            z2 = _z2 [i];
            c1 = _c1;
            c2 = _c2;
            gg = _gg;
            for (j = 0; j < nsamp; j++)
            {
                c1 += _dc1;
                c2 += _dc2;
                gg += _dgg;
	        x = *p;
	        y = x - c2 * z2;
		*p++ = x - gg * (z2 + c2 * y - x);
	        y -= c1 * z1;
	        z2 = z1 + c1 * y;
	        z1 = y + 1e-20f;
	    }
            _z1 [i] = z1;
            _z2 [i] = z2;
	}
        _c1 = c1;
        _c2 = c2;
        _gg = gg;
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
	        y = x - c2 * z2;
		*p++ = x - gg * (z2 + c2 * y - x);
	        y -= c1 * z1;
	        z2 = z1 + c1 * y;
	        z1 = y + 1e-20f;
	    }
            _z1 [i] = z1;
            _z2 [i] = z2;
	}
    }
}


}
