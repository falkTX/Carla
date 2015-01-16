// -------------------------------------------------------------------------
//
//  Copyright (C) 2004-2013 Fons Adriaensen <fons@linuxaudio.org>
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
// -------------------------------------------------------------------------


#include <math.h>
#include "audio.h"
#include "messages.h"


Audio::Audio (ITC_ctrl *cmain, const char *name) :
    A_thread ("Audio"),
    _name (name),
    _cmain (cmain),
    _run_jack (0),
    _ncapt (8),
    _nplay (8),
    _input (-1),
    _data (0),
    _outs (0)
{
}


Audio::~Audio (void)
{
    if (_run_jack) close_jack ();
    delete[] _outs;
}


void Audio::init (void)
{
    int i;

    for (i = 0; i < LSINE; i++)
    {
        _sine [i] = sin (2 * M_PI * i / LSINE);
    }
    _sine [LSINE] = 0;
    _g_bits = 0;
    _a_noise = 0.0;
    _a_sine  = 0.0;
    _f_sine  = 0.0;
    _p_sine  = 0.0;
}


void Audio::thr_main (void)
{
    put_event (EV_EXIT);
}


void Audio::init_jack (const char *server)
{
    char           s [16];
    int             opts;
    jack_status_t  stat;

    opts = JackNoStartServer;
    if (server) opts |= JackServerName;
    if ((_jack_handle = jack_client_open (_name, (jack_options_t) opts, &stat, server)) == 0)
    {
        fprintf (stderr, "Can't connect to JACK\n");
        exit (1);
    }

    jack_set_process_callback (_jack_handle, jack_static_callback, (void *)this);
    jack_on_shutdown (_jack_handle, jack_static_shutdown, (void *)this);

    for (int i = 0; i < _nplay; i++)
    {
        sprintf(s, "out_%d", i + 1);
        _jack_out [i] = jack_port_register (_jack_handle, s, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    }
    for (int i = 0; i < _ncapt; i++)
    {
        sprintf(s, "in_%d", i + 1);
        _jack_in [i] = jack_port_register (_jack_handle, s, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    }

    _outs = new float [4096];
    init ();
    
    if (jack_activate (_jack_handle))
    {
        fprintf(stderr, "Can't activate JACK");
        exit (1);
    }

    _fsamp = jack_get_sample_rate (_jack_handle);
    _fsize = jack_get_buffer_size (_jack_handle);
    _name = jack_get_client_name (_jack_handle);
    _run_jack = true;
    _cmain->put_event (EV_MESG, new M_jinfo (_fsamp, _fsize, _name));
}


void Audio::close_jack ()
{
    jack_deactivate (_jack_handle);
    for (int i = 0; i < _nplay; i++) jack_port_unregister(_jack_handle, _jack_out[i]);
    for (int i = 0; i < _ncapt; i++) jack_port_unregister(_jack_handle, _jack_in[i]);
    jack_client_close (_jack_handle);
}


void Audio::jack_static_shutdown (void *arg)
{
    return ((Audio *) arg)->jack_shutdown ();
}


void Audio::jack_shutdown (void)
{
    _cmain->put_event (EV_JACK);
}


int Audio::jack_static_callback (jack_nframes_t nframes, void *arg)
{
    return ((Audio *) arg)->jack_callback (nframes);
}


int Audio::jack_callback (jack_nframes_t nframes)
{
    unsigned long  b, m, n;
    int     i;
    float  *p;
 
    if (_data && _input >= 0)
    {
        p = (float *)(jack_port_get_buffer (_jack_in [_input], nframes));
	m = nframes;
        n = _size - _dind;
        if (m >= n)
	{
            memcpy (_data + _dind, p, sizeof(jack_default_audio_sample_t) * n);
            _dind = 0;
            p += n;
            m -= n;
        }
        if (m)
	{
            memcpy (_data + _dind, p, sizeof(jack_default_audio_sample_t) * m);
            _dind += m;
	}
        _scnt += nframes;
    }

    generate (nframes);
    b = _g_bits & 255;   
    for (i = 0; i < _nplay; i++, b >>= 1)
    {    
          
        p = (float *)(jack_port_get_buffer (_jack_out [i], nframes));
    	if (b & 1) memcpy (p, _outs, sizeof(jack_default_audio_sample_t) * nframes);
        else       memset (p, 0,     sizeof(jack_default_audio_sample_t) * nframes);
    }

    process ();

    return 0;
}


void Audio::process (void) 
{
    int       k;
    ITC_mesg *M;

    if (_data)
    {
        k = _scnt / _step;
        if (k && _cmain->put_event_try (EV_TRIG, k) == ITC_ctrl::NO_ERROR) _scnt -= k * _step;
    }
   
    if (get_event_nowait (1 << EV_MESG) == EV_MESG)
    {
	M = get_message ();
	if (M->type () == M_BUFFP)
	{
	    M_buffp *Z = (M_buffp *) M; 
	    _data = Z->_data;
	    _size = Z->_size;
	    _step = Z->_step; 
	    _dind = 0;
	    _scnt = 0;
	}
	else if (M->type () == M_INPUT)
	{
	    M_input *Z = (M_input *) M; 
	    _input = Z->_input;
	    if (_input >= _ncapt) _input = -1;
	}
	else if (M->type () == M_GENPAR)
	{
	    M_genpar *Z = (M_genpar *) M; 
            _g_bits  = Z->_g_bits;  
            _a_noise = sqrt (0.5) * pow (10.0, 0.05 * Z->_a_noise);
            _a_sine  = pow (10.0, 0.05 * Z->_a_sine);
            _f_sine  = LSINE * Z->_f_sine / _fsamp; 
	}
	M->recover ();
    }
}


void Audio::generate (int size) 
{
    int   i, j;
    float a, p, r;

    if (size > 4096) size = 4096;

    memset (_outs, 0, size * sizeof (float));

    if (_g_bits & M_genpar::WNOISE)
    {
	for (i = 0; i < size; i++)
	{
	    _outs [i] += _a_noise * _rngen.grandf ();
	}
    }
    if (_g_bits & M_genpar::SINE)
    {  
        p = _p_sine;       
	for (i = 0; i < size; i++)
        {
	    j = (int) p;
            if (j == LSINE) a = 0;
	    else
	    {
   	        r = p - j;
                a = (1.0 - r) * _sine [j] + r * _sine [j + 1];
	    }
            _outs [i] += _a_sine * a;
            p += _f_sine;
            if (p >= LSINE) p -= LSINE;         
	}
        _p_sine = p;       
    }
}
