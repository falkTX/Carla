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


#ifndef __LFSHELF2_H
#define __LFSHELF2_H


#include <stdint.h>
#include "global.h"

namespace BLS1 {


class LFshelf2
{
public:

    LFshelf2 (void);
    ~LFshelf2 (void);
    
    void setfsamp (float fsamp);
    void setparam (float g, float f, float s)
    {
	_f0 = f;
	_g0 = g;
	_s0 = s;
	_touch0++;
    }
    void reset (void);
    void bypass (bool s)
    {
	if (s != _bypass)
	{
	    _bypass = s;
	    _touch0++;
	}
    }
    void prepare (int nsamp);
    void process (int nsamp, int nchan, float *data[])
    {
	if (_state != BYPASS) process1 (nsamp, nchan, data); 
    }
    float response (float f);

private:

    enum { BYPASS, STATIC, SMOOTH };

    void calcpar1 (int nsamp, float g, float f, float s);
    void process1 (int nsamp, int nchan, float *data[]);

    volatile int16_t  _touch0;
    volatile int16_t  _touch1;
    bool              _bypass;
    int               _state;
    float             _fsamp;

    float             _g0, _g1;
    float             _f0, _f1;
    float             _s0, _s1;

    float             _a0, _a1, _a2;
    float             _b1, _b2;
    float             _da0, _da1, _da2;
    float             _db1, _db2;

    float             _z1 [2];
    float             _z2 [2];
};


}

#endif
