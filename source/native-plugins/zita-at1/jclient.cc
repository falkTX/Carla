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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jclient.h"
#include "global.h"

namespace AT1 {


Jclient::Jclient (jack_client_t *jclient) :
    A_thread ("jclient"),
    _jack_client (jclient),
    _active (false)
{
    init_jack ();
}


Jclient::~Jclient (void)
{
    if (_jack_client) close_jack ();
}


void Jclient::init_jack (void)
{
    jack_on_shutdown (_jack_client, jack_static_shutdown, (void *) this);
    jack_set_process_callback (_jack_client, jack_static_process, (void *) this);
    jack_activate (_jack_client);

    _fsamp = jack_get_sample_rate (_jack_client);
    _fsize = jack_get_buffer_size (_jack_client);

    _ainp_port = jack_port_register (_jack_client, "in",  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,  0);
    _aout_port = jack_port_register (_jack_client, "out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    _midi_port = jack_port_register (_jack_client, "pitch", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	
    _retuner = new Retuner (_fsamp);
    _notemask = 0xFFF;
    _midichan = -1;
    clr_midimask ();

    _active = true;
}


void Jclient::close_jack ()
{
    jack_deactivate (_jack_client);
    jack_client_close (_jack_client);
    delete _retuner;
}


void Jclient::jack_static_shutdown (void *arg)
{
    ((Jclient *) arg)->jack_shutdown ();    
}


int Jclient::jack_static_process (jack_nframes_t nframes, void *arg)
{
    return ((Jclient *) arg)->jack_process (nframes);
}


void Jclient::jack_shutdown (void)
{
    send_event (EV_EXIT, 1);
}


void Jclient::clr_midimask (void)
{
    int i;

    for (i = 0; i < 12; i++) _notes [i] = 0;
    _midimask = 0; 
}


void Jclient::midi_process (int nframes)
{
    int                i, b, n, t, v;
    void               *p;
    jack_midi_event_t  E;

    p = jack_port_get_buffer (_midi_port, nframes);
    i = 0;
    while (jack_midi_event_get (&E, p, i) == 0 && E.size == 3)
    {
	t = E.buffer [0];
        n = E.buffer [1];
	v = E.buffer [2];
	if ((_midichan < 0) || ((t & 0x0F) == _midichan))
	{
	    const int n12 = n % 12;
  	    switch (t & 0xF0)
	    {
	    case 0x80:
	    case 0x90:
	        if (v && (t & 0x10))_notes [n12] += 1;
  	        else if (_notes [n12] > 0) _notes [n12] -= 1;
	        break;
  	    }
	}
	i++;
    }
	
    _midimask = 0;
    for (i = 0, b = 1; i < 12; i++, b <<= 1) 
    {
	if (_notes [i]) _midimask |= b;
    }
}


int Jclient::jack_process (int nframes)
{
    float *inpp;
    float *outp;

    if (!_active) return 0;

    inpp = (float *) jack_port_get_buffer (_ainp_port, nframes);
    outp = (float *) jack_port_get_buffer (_aout_port, nframes);
    midi_process (nframes);
    _retuner->set_notemask (_midimask ? _midimask : _notemask);
    _retuner->process (nframes, inpp, outp);
 
    return 0;
}


}
