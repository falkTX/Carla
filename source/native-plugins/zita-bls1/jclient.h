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


#ifndef __JCLIENT_H
#define __JCLIENT_H


#include <inttypes.h>
#include <stdlib.h>
#include <math.h>
#include <clthreads.h>
#include "CarlaNativeJack.h"
#include "hp3filt.h"
#include "shuffler.h"
#include "lfshelf2.h"
#include "global.h"

namespace BLS1 {


class Jclient : public A_thread
{
public:

    Jclient (jack_client_t *jclient);
    ~Jclient (void);

    void set_inpbal (float diff)
    {
	_inpbal0 = diff;
    }

    void set_hpfilt (float freq)
    {
	_hpfilt.setparam (freq);
    }

    void set_loshelf (float gain, float freq)
    {
	_lshelf.setparam (powf (10.0f, 0.05f * gain), freq, 0.4f);
    }

    Shuffler *shuffler (void)
    {
	return (Shuffler *) &_shuffl;
    }

private:

    void  init_jack (void);
    void  close_jack (void);
    void  jack_shutdown (void);
    int   jack_process (int nframes);

    virtual void thr_main (void) {}

    jack_client_t  *_jack_client;
    jack_port_t    *_inpports [2];
    jack_port_t    *_outports [2];
    bool            _active;
    unsigned int    _fsamp;
    int             _psize;
    int             _fragm;
    int             _nsamp;
    float           _inpbal0;
    float           _inpbal1;
    float           _ga, _gb;
    float           _da, _db;
    HP3filt         _hpfilt;
    LFshelf2        _lshelf;
    Shuffler        _shuffl;
  
    static void jack_static_shutdown (void *arg);
    static int  jack_static_process (jack_nframes_t nframes, void *arg);
};


}

#endif
