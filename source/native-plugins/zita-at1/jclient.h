// -----------------------------------------------------------------------
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
// -----------------------------------------------------------------------


#ifndef __JCLIENT_H
#define __JCLIENT_H


#include "CarlaNativeJack.h"
#include <clthreads.h>
#include "retuner.h"

namespace AT1 {


class Jclient : public A_thread
{
public:

    Jclient (jack_client_t *jclient);
    ~Jclient (void);

    unsigned int fsize (void) const { return _fsize; } 
    unsigned int fsamp (void) const { return _fsamp; } 
    Retuner *retuner (void) { return _retuner; }
    void set_notemask (int m) { _notemask = m; } 
    void set_midichan (int c) { _midichan = c; } 
    void clr_midimask (void);
    int  get_noteset (void) { return _retuner->get_noteset (); }
    int  get_midiset (void) { return _midimask; }

private:

    virtual void thr_main (void) {}

    void init_jack (void);
    void close_jack (void);
    void jack_shutdown (void);
    int  jack_process (int nframes);
    void midi_process (int nframes);

    jack_client_t  *_jack_client;
    jack_port_t    *_ainp_port;
    jack_port_t    *_aout_port;
    jack_port_t    *_midi_port;
    bool            _active;
    unsigned int    _fsamp;
    unsigned int    _fsize;
    Retuner        *_retuner;
    int             _notes [12];
    int             _notemask;
    int             _midimask;
    int             _midichan;

    static void jack_static_shutdown (void *arg);
    static int  jack_static_process (jack_nframes_t nframes, void *arg);
};


}

#endif
