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


#ifndef __SHUFFLER_H
#define __SHUFFLER_H


#include <zita-convolver.h>
#include "global.h"

namespace BLS1 {


class Shuffler
{
public:

    Shuffler (void);
    ~Shuffler (void);
    
    void init (int fsamp, int quant);
    void reset (void);
    void prepare (float gain, float freq);
    void process (int nsamp, float *inp [], float *out []);
    
    bool ready (void) { return _touch0 == _touch1; }

private:

    void fini (void);

    float             _fsamp;
    int               _quant;
    int               _minpt;
    int               _iplen;
    int               _delay;
    float            *_fft_time;
    fftwf_complex    *_fft_freq;
    fftwf_plan        _fft_plan;
    int               _del_size;
    int               _del_wind;
    float            *_del_data;
    int               _state;
    int               _count;
    volatile int16_t  _touch0;
    volatile int16_t  _touch1;
    Convproc          _convproc;
};


}

#endif
