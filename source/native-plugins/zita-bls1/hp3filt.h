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


#ifndef __HP3FILT_H
#define __HP3FILT_H


#include <stdint.h>
#include "global.h"

namespace BLS1 {


class HP3filt
{
public:

    HP3filt (void);
    ~HP3filt (void);
    
    void setfsamp (float fsamp);
    void setparam (float f)
    {
	_f0 = f;
	_touch0++;
    }
    void reset (void);
    void prepare (int nsamp);
    void process (int nsamp, int nchan, float *data[])
    {
	if (_state != BYPASS) process1 (nsamp, nchan, data); 
    }
    float response (float f);

private:

    enum { BYPASS, STATIC, FADING };

    void calcpar1 (int nsamp, float f);
    void process1 (int nsamp, int nchan, float *data[]);

    volatile int16_t  _touch0;
    volatile int16_t  _touch1;
    int               _state;
    float             _fsamp;
    float             _f0, _f1;
    float             _c1, _c2, _c3;
    float             _g, _a, _d;
    float             _z1 [2];
    float             _z2 [2];
    float             _z3 [2];
};


}

#endif
