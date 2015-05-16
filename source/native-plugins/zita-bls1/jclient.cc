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
#include "jclient.h"

namespace BLS1 {


Jclient::Jclient (jack_client_t *jclient) :
    A_thread ("Jclient"),
    _jack_client (jclient),
    _active (false),
    _inpbal0 (0),	
    _inpbal1 (0),	
    _ga (1.0f),
    _gb (1.0f),
    _da (0.0f),
    _db (0.0f)
{
    init_jack ();
}


Jclient::~Jclient (void)
{
    if (_jack_client) close_jack ();
}


void Jclient::init_jack (void)
{
    jack_set_process_callback (_jack_client, jack_static_process, (void *) this);
    jack_on_shutdown (_jack_client, jack_static_shutdown, (void *) this);
    jack_activate (_jack_client);

    _fsamp = jack_get_sample_rate (_jack_client);
    _psize = jack_get_buffer_size (_jack_client);
    if (_psize > 4096)
    {
	fprintf (stderr, "Period size can't be more than 4096.\n");
	return;
    }
    if (_psize & (_psize - 1))
    {
	fprintf (stderr, "Period size must be a power of 2.\n");
	return;
    }

    _inpports [0] = jack_port_register (_jack_client, "inp.L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    _inpports [1] = jack_port_register (_jack_client, "inp.R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    _outports [0] = jack_port_register (_jack_client, "out.L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    _outports [1] = jack_port_register (_jack_client, "out.R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    _hpfilt.setfsamp (_fsamp);
    _lshelf.setfsamp (_fsamp);
    _lshelf.bypass (false);
    _shuffl.init (_fsamp, _psize);


    for (int k = _fsamp, _fragm = 1024; k > 56000; k >>= 1, _fragm <<= 1);
    _nsamp = 0;
    _active = true;
}


void Jclient::close_jack ()
{
    jack_deactivate (_jack_client);
    jack_client_close (_jack_client);
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


int Jclient::jack_process (int frames)
{
    int   i, k, n;
    float a, b, t;
    float *inp [2];
    float *tmp [2];
    float *out [2];

    if (!_active) return 0;
    
    inp [0] = (float *) jack_port_get_buffer (_inpports [0], frames);
    inp [1] = (float *) jack_port_get_buffer (_inpports [1], frames);
    out [0] = tmp [0] = (float *) jack_port_get_buffer (_outports [0], frames);
    out [1] = tmp [1] = (float *) jack_port_get_buffer (_outports [1], frames);

    a = _ga;
    b = _gb;
    n = frames;
    while (n)
    {
	if (!_nsamp)
	{
	    if (fabsf (_inpbal0 -_inpbal1) > 0.01f)
	    {
		_inpbal1 = _inpbal0;
		t = powf (10.0f, 0.05f * _inpbal0);
		_db = (t - b) / _fragm;
 	        t = 1.0f / t;
		_da = (t - a) / _fragm;
	    }
	    else
	    {
                _da = 0.0f;
		_db = 0.0f;
	    }
	    _hpfilt.prepare (_fragm);
	    _lshelf.prepare (_fragm);
            _nsamp = _fragm;
	}

        k = (n < _nsamp) ? n: _nsamp;
	for (i = 0; i < k; i++)
	{
	    a += _da;
	    b += _db;
	    tmp [0][i] = a * inp [0][i];
	    tmp [1][i] = b * inp [1][i];
	}
        _hpfilt.process (k, 2, tmp);
        _lshelf.process (k, 2, tmp);
        inp [0] += k;
        inp [1] += k;
        tmp [0] += k;
        tmp [1] += k;
	_nsamp -= k;
	n -= k;
    }
    _ga = a;
    _gb = b;

    _shuffl.process (frames, out, out);

    return 0;
}


}
