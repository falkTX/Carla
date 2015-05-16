// ----------------------------------------------------------------------
//
//  Copyright (C) 2010-2014 Fons Adriaensen <fons@linuxaudio.org>
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

namespace AT1 {


Mainwin::Mainwin (X_rootwin *parent, X_resman *xres, int xp, int yp, ValueChangedCallback* valuecb) :
    A_thread ("Main"),
    X_window (parent, xp, yp, XSIZE, YSIZE, XftColors [C_MAIN_BG]->pixel),
    _stop (false),
    _xres (xres),
    z_error (0.0f),
    z_noteset (0),
    z_midiset (0),
    _valuecb (valuecb)
{
    X_hints     H;
    int         i, j, x, y;

    _atom = XInternAtom (dpy (), "WM_DELETE_WINDOW", True);
    XSetWMProtocols (dpy (), win (), &_atom, 1);
    _atom = XInternAtom (dpy (), "WM_PROTOCOLS", True);

    H.position (xp, yp);
    H.minsize (XSIZE, YSIZE);
    H.maxsize (XSIZE, YSIZE);
    H.rname (xres->rname ());
    H.rclas (xres->rclas ());
    x_apply (&H); 

    x = 20;
    _bmidi = new Pbutt0 (this, this, &b_midi_img, x, 12, B_MIDI);
    _bmidi->x_map ();

    bstyle1.size.x = 50;
    bstyle1.size.y = 20;
    _bchan = new X_tbutton (this, this, &bstyle1, x - 5, 40, "Omni", 0, B_CHAN);
    _bchan->x_map ();
    _midich = 0;

    x = 100;
    y = 23;
    _tmeter = new Tmeter (this, x - 20, 53);
    _tmeter->x_map ();
    for (i = j = 0; i < 12; i++, j++)
    {
        _bnote [i] = new Pbutt1 (this, this, &b_note_img, x, y, i);
        _bnote [i]->set_state (1);
        _bnote [i]->x_map ();
	if (j == 4) 
	{
	    x += 20;
	    j++;
	}
	else
	{
	    x += 10;
            if (j & 1) y += 18;
	    else       y -= 18;
	} 
    }


    RotaryCtl::init (disp ());
    x = 270;
    _rotary [R_TUNE] = new Rlinctl (this, this, R_TUNE, &r_tune_geom, x, 0, 400,  5, 400.0, 480.0, 440.0);
    _rotary [R_BIAS] = new Rlinctl (this, this, R_BIAS, &r_bias_geom, x, 0, 270,  5,   0.0,   1.0,   0.5);
    _rotary [R_FILT] = new Rlogctl (this, this, R_FILT, &r_filt_geom, x, 0, 200,  5,   0.50,  0.02,  0.1);
    _rotary [R_CORR] = new Rlinctl (this, this, R_CORR, &r_corr_geom, x, 0, 270,  5,   0.0,   1.0,   1.0);
    _rotary [R_OFFS] = new Rlinctl (this, this, R_OFFS, &r_offs_geom, x, 0, 400, 10,  -2.0,   2.0,   0.0);
    for (i = 0; i < NROTARY; i++) _rotary [i]->x_map ();

    _textln = new X_textip (this, 0, &tstyle1, 0, 0, 50, 15, 15);
    _textln->set_align (0);
    _ttimer = 0;
    _notes = 0xFFF;

    x_add_events (ExposureMask);
    set_time (0);
    inc_time (500000);
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


void Mainwin::setdata_ui (float error, int noteset, int midiset)
{
    z_error = error;
    z_noteset = noteset;
    z_midiset = midiset;
}


void Mainwin::setchan_ui (int chan)
{
    char s [16];

    _midich = chan;

    if (_midich)
    {
        sprintf (s, "Ch %d\n", _midich);
        _bchan->set_text (s, 0);
    }
    else _bchan->set_text ("Omni", 0);
}


void Mainwin::setmask_ui (int mask)
{
    _notes = mask;

    for (int i = 0; i < 12; i++)
         _bnote [i]->set_state ( (_notes & (1 << i)) != 0 ? 1 : 0 );
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
    int   i, k, s;
    float v;

    v = z_error;
    _tmeter->update (v, v);
    k = z_noteset;
    for (i = 0; i < 12; i++)
    {
	s = _bnote [i]->state ();
	if (k & 1) s |= 2;
	else s &= ~2;
        _bnote [i]->set_state (s);
	k >>= 1;
    }
    k = z_midiset;
    if (k) _bmidi->set_state (_bmidi->state () | 1);
    else   _bmidi->set_state (_bmidi->state () & ~1);
    if (_ttimer)
    {
	if (--_ttimer == 0) _textln->x_unmap ();
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
    PushButton *B;
    RotaryCtl  *R;
    int         k;
    float       v;

    switch (type)
    {
    case X_callback::BUTTON | X_button::PRESS:
    {
        X_button     *Z = (X_button *) W;
	XButtonEvent *X = (XButtonEvent *) E;
	switch (Z->cbid ())
	{
	case B_CHAN:
	    switch (X->button)
	    {
	    case 1:
	    case 4:
		setchan (1);
	        break;
	    case 3:
	    case 5:
		setchan (-1);
	        break;
	    }
            break;
	}
	break;
    }
    case PushButton::PRESS:
	B = (PushButton *) W;
	k = B->cbind ();
	if (k < B_MIDI)
	{
	    k = 1 << k;
	    if (B->state () & 1) _notes |=  k;
	    else                 _notes &= ~k;
	    _valuecb->noteMaskChangedCallback (_notes);
	}
	else if (k == B_MIDI)
	{
	    _valuecb->noteMaskChangedCallback (-1);
	}
	break;

    case RotaryCtl::PRESS:
	R = (RotaryCtl *) W;
	k = R->cbind ();
	switch (k)
	{
        case R_TUNE:
        case R_OFFS:
	    showval (k);
	    break;
	}
	break;

    case RotaryCtl::DELTA:
	R = (RotaryCtl *) W;
	k = R->cbind ();
	switch (k)
	{
        case R_TUNE:
            v = _rotary [R_TUNE]->value ();
            _valuecb->valueChangedCallback (R_TUNE, v);
	    showval (k);
	    break;
	case R_BIAS:   
            v = _rotary [R_BIAS]->value ();
            _valuecb->valueChangedCallback (R_BIAS, v);
	    break;
	case R_FILT:   
            v = _rotary [R_FILT]->value ();
            _valuecb->valueChangedCallback (R_FILT, v);
	    break;
	case R_CORR:   
            v = _rotary [R_CORR]->value ();
            _valuecb->valueChangedCallback (R_CORR, v);
	    break;
	case R_OFFS:   
            v = _rotary [R_OFFS]->value ();
            _valuecb->valueChangedCallback (R_OFFS, v);
	    showval (k);
	    break;
	}
	break;
    }
}


void Mainwin::setchan (int d)
{
    char s [16];

    _midich += d;
    if (_midich <  0) _midich =  0;
    if (_midich > 16) _midich = 16;
    if (_midich)
    {
        sprintf (s, "Ch %d\n", _midich);
	_bchan->set_text (s, 0);
    }
    else _bchan->set_text ("Omni", 0);
    _valuecb->valueChangedCallback (NROTARY, _midich);
}   


void Mainwin::showval (int k)
{
    char s [16];

    switch (k)
    {
    case R_TUNE:
        sprintf (s, "%5.1lf", _rotary [R_TUNE]->value ());
	_textln->x_move (222, 58);
	break;
    case R_OFFS:
        sprintf (s, "%5.2lf", _rotary [R_OFFS]->value ());
	_textln->x_move (463, 58);
	break;
    }
    _textln->set_text (s);
    _textln->x_map ();
    _ttimer = 40;
}


void Mainwin::redraw (void)
{
    int x;

    x = 80;
    XPutImage (dpy (), win (), dgc (), notesect_img, 0, 0, x, 0, 190, 75);
    x += 190;
    XPutImage (dpy (), win (), dgc (), ctrlsect_img, 0, 0, x, 0, 315, 75);
}


}
