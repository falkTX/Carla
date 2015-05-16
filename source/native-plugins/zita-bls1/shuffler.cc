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


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "shuffler.h"

namespace BLS1 {


Shuffler::Shuffler (void) :
    _fsamp (0),
    _quant (0),
    _minpt (0),
    _iplen (0),
    _delay (0),
    _fft_time (0),
    _fft_freq (0),
    _fft_plan (0),
    _del_size (0),
    _del_wind (0),
    _del_data (0),
    _count (0),
    _touch0 (0),
    _touch1 (0)
{
}


Shuffler::~Shuffler (void)
{
    fini ();
}


void Shuffler::init (int fsamp, int quant)
{
    int k;

    _fsamp = fsamp;
    _quant = quant;
    _minpt = 256;
    for (k = _fsamp, _iplen = 1024; k > 56000; k >>= 1, _iplen <<= 1);
    if (_quant > _iplen) _quant = _iplen;
    _delay = _iplen / 2;
    if (_quant < _minpt) _delay += 2 * _minpt - _quant;
    else _minpt = _quant;

    _del_size = 3 * _iplen;
    _del_wind = 0;
    _del_data = new float [3 * _iplen];
    memset (_del_data, 0, 3 * _iplen * sizeof (float));

    _fft_time = (float *) fftwf_malloc (_iplen * sizeof (float));
    _fft_freq = (fftwf_complex *) fftwf_malloc ((_iplen / 2 + 1) * sizeof (fftwf_complex));
    _fft_plan = fftwf_plan_dft_c2r_1d (_iplen, _fft_freq, _fft_time, FFTW_ESTIMATE);
    memset (_fft_time, 0, _iplen * sizeof (float));
    _fft_time [_iplen / 2] = 1.0f;

    _convproc.configure (1, 1, _iplen, _quant, _minpt, _minpt);
    _convproc.impdata_create (0, 0, 1, _fft_time, 0, _iplen);
    _convproc.start_process (35, SCHED_FIFO);
}


void Shuffler::fini (void)
{
    fftwf_free (_fft_time);
    fftwf_free (_fft_freq);
    fftwf_destroy_plan (_fft_plan);
    delete[] _del_data;
    _convproc.stop_process ();
    _convproc.cleanup ();
}


void Shuffler::reset (void)
{
}


void Shuffler::prepare (float gain, float freq)
{
    int    i, h;
    float  f, g, t;

    g = powf (10.0f, 0.05f * gain);
    g = sqrtf (g * g - 1.0f);
    freq /= g;

    h = _iplen / 2;
    for (i = 0; i < h; i++)
    {
	f = i * _fsamp / _iplen;
	t = 1.0f / hypotf (1.0f, f / freq);
        _fft_freq [i][0] = 0;
        _fft_freq [i][1] = (i & 1) ? t : -t;
    }
    _fft_freq [h][0] = 0;
    _fft_freq [h][1] = 0;
    fftwf_execute(_fft_plan);

    g /= _iplen;
    for (i = 1; i < h; i++)
    {
        t = g * (0.6f + 0.4f * cosf (i * M_PI / h));
	_fft_time [h + i] *= t;
	_fft_time [h - i] *= t;
    }
    _fft_time [0] = 0;
    _fft_time [h] = 1;
    _convproc.impdata_update (0, 0, 1, _fft_time, 0, _iplen);

    _touch0++;
}


void Shuffler::process (int nsamp, float *inp [], float *out [])
{
    int   i, k, im, ri, wi;
    float a, b, *p0, *p1, *q;

    im = _del_size;
    wi = _del_wind;
    ri = _del_wind - _delay;
    if (ri < 0) ri += im;

    for (k = 0; k < nsamp; k += _quant)
    {
        if ((_count == 0) && (_touch0 != _touch1)) _count = 2 * _iplen;

	p0 = inp [0] + k;
	p1 = inp [1] + k;
	q = _convproc.inpdata (0);
	for (i = 0; i < _quant; i++)
	{
	    a = p0 [i] / 2;
	    b = p1 [i] / 2;
	    _del_data [wi++] = a + b;
	    if (wi == im) wi = 0;
	    *q++ = a - b;
	}
	_convproc.process ();
	p0 = out [0] + k;
	p1 = out [1] + k;
	q = _convproc.outdata (0);
	for (i = 0; i < _quant; i++)
	{
	    a = _del_data [ri++];
	    if (ri == im) ri = 0;
	    b = *q++;
	    p0 [i] = a + b;
            p1 [i] = a - b;
	}

        if (_count)
        {
            _count -= _quant;
	    if (_count == 0) _touch1 = _touch0;
        }
    }

    _del_wind = wi;
}


}
