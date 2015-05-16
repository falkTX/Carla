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


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "styles.h"
#include "global.h"
#include "mainwin.h"

namespace BLS1 {


Mainwin::Mainwin (X_rootwin *parent, X_resman *xres, int xp, int yp, ValueChangedCallback* valuecb) :
    A_thread ("Main"),
    X_window (parent, xp, yp, XSIZE, YSIZE, XftColors [C_MAIN_BG]->pixel),
    _stop (false),
    _xres (xres),
    _touch (false),
    _valuecb (valuecb)
{
    X_hints     H;
    int         i;

    _atom = XInternAtom (dpy (), "WM_DELETE_WINDOW", True);
    XSetWMProtocols (dpy (), win (), &_atom, 1);
    _atom = XInternAtom (dpy (), "WM_PROTOCOLS", True);

    H.position (xp, yp);
    H.minsize (XSIZE, YSIZE);
    H.maxsize (XSIZE, YSIZE);
    H.rname (xres->rname ());
    H.rclas (xres->rclas ());
    x_apply (&H); 

    RotaryCtl::init (disp ());
    _rotary [INPBAL] = new Rlinctl (this, this, &inpbal_img,  20, 0, 120, 4,  -3.0f,   3.0f,  0.0f, INPBAL);
    _rotary [HPFILT] = new Rlogctl (this, this, &hpfilt_img,  20, 0, 120, 4,  10.0f, 320.0f, 40.0f, HPFILT);
    _rotary [SHGAIN] = new Rlinctl (this, this, &shgain_img, 190, 0, 120, 5,   0.0f,  24.0f, 15.0f, SHGAIN);
    _rotary [SHFREQ] = new Rlogctl (this, this, &shfreq_img, 190, 0, 192, 8, 125.0f,   2e3f,  5e2f, SHFREQ);
    _rotary [LFFREQ] = new Rlogctl (this, this, &lffreq_img, 410, 0, 192, 8,  20.0f, 320.0f, 80.0f, LFFREQ);
    _rotary [LFGAIN] = new Rlinctl (this, this, &lfgain_img, 410, 0, 180, 5,  -9.0f,   9.0f,  0.0f, LFGAIN);
    for (i = 0; i < NROTARY; i++) _rotary [i]->x_map ();

    _numtext = new X_textip (this, 0, &tstyle1, 0, 0, 45, 15, 15);
    _numtext->set_align (0);
    _parmind = -1;
    _timeout = 0;

    x_add_events (ExposureMask);
    set_time (0);
    inc_time (250000);
}

 
Mainwin::~Mainwin (void)
{
    RotaryCtl::fini ();
}

 
int Mainwin::process (void)
{
    int e;

    if (_stop) handle_stop ();

    e = get_event_timed ();
    switch (e)
    {
    case EV_TIME:
        handle_time ();
	break;
    }
    return e;
}


void Mainwin::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case Expose:
	expose ((XExposeEvent *) E);
	break;  
 
    case ClientMessage:
        clmesg ((XClientMessageEvent *) E);
        break;
    }
}


void Mainwin::expose (XExposeEvent *E)
{
    if (E->count) return;
    redraw ();
}


void Mainwin::clmesg (XClientMessageEvent *E)
{
    if (E->message_type == _atom) _stop = true;
}


void Mainwin::handle_time (void)
{
    if (_timeout)
    {
	if (--_timeout == 0) numdisp (-1);
    }

    if (_touch) 
    {
        double v1 = _rotary [SHGAIN]->value (), v2 = _rotary [SHFREQ]->value ();
        _valuecb->valueChangedCallback (SHGAIN, v1);
        _valuecb->valueChangedCallback (SHFREQ, v2);
	_touch = 0;
    }

    inc_time (5000);
    XFlush (dpy ());
}


void Mainwin::handle_stop (void)
{
    put_event (EV_EXIT, 1);
}


void Mainwin::handle_callb (int type, X_window *W, XEvent *E)
{
    RotaryCtl  *R;
    int         k;
    double      v, v2;

    switch (type)
    {
    case RotaryCtl::PRESS:
	R = (RotaryCtl *) W;
	k = R->cbind ();
	switch (k)
	{
	default:
	    numdisp (-1);
	}
	break;

    case RotaryCtl::DELTA:
	R = (RotaryCtl *) W;
	k = R->cbind ();
	switch (k)
	{
	case INPBAL:
            v = _rotary [INPBAL]->value ();
            _valuecb->valueChangedCallback (INPBAL, v);
	    break;
	case HPFILT:
            v = _rotary [HPFILT]->value ();
            _valuecb->valueChangedCallback (HPFILT, v);
	    break;
	case SHGAIN:
	case SHFREQ:
	    _touch++;
	    break;
	case LFFREQ:
	case LFGAIN:
            v  = _rotary [LFGAIN]->value ();
            v2 = _rotary [LFFREQ]->value ();
            _valuecb->valueChangedCallback (LFGAIN, v);
            _valuecb->valueChangedCallback (LFFREQ, v2);
	    break;
	}
	break;
    }
}


void Mainwin::redraw (void)
{
    XPutImage (dpy (), win (), dgc (), inputsect, 0, 0,  20, 0, 130, 75);
    XPutImage (dpy (), win (), dgc (), shuffsect, 0, 0, 190, 0, 170, 75);
    XPutImage (dpy (), win (), dgc (), lfshfsect, 0, 0, 410, 0, 105, 75);
}


void Mainwin::numdisp (int k)
{
    int y = 0;
    
    _timeout = 10;
    if (k >= 0) fmtfreq (k);
    if (k == _parmind) return;
    if (k < 0) 
    {
	_numtext->x_unmap ();
	_parmind = -1;
    }
    else
    {
	switch (k)
	{
	default:
	    ;
	}
	_numtext->x_move (1, y + 3);
	_numtext->x_map ();
	_parmind = k;
    }
}


void Mainwin::fmtfreq (int k)
{
    double     v;
    char       t [16];
    const char *f;

    v = _rotary [k]->value ();
    if      (v <= 3e1) f = "%1.1lf";
    else if (v <= 1e3) f = "%1.0lf";
    else if (v <= 3e3)
    {
	f = "%1.2lfk";
	v *= 1e-3;
    }
    else
    {
	f = "%1.1lfk";
	v *= 1e-3;
    }
    sprintf (t, f, v);
    _numtext->set_text (t);
}


}
