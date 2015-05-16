// ----------------------------------------------------------------------
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
// ----------------------------------------------------------------------


#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <math.h>
#include "button.h"

namespace AT1 {


int PushButton::_keymod = 0;
int PushButton::_button = 0;


PushButton::PushButton (X_window     *parent,
                        X_callback   *cbobj,
                        ButtonImg    *image,
		        int  xp,
                        int  yp,
                        int  cbind) :

    X_window (parent,
              image->_x0 + xp, image->_y0 + yp,
              image->_dx, image->_dy,
              image->_backg->pixel),
    _cbobj (cbobj),
    _cbind (cbind),
    _image (image),
    _state (0)
{
    x_add_events (ExposureMask | ButtonPressMask | ButtonReleaseMask);
} 


PushButton::~PushButton (void)
{
}


void PushButton::init (X_display *disp)
{
}


void PushButton::fini (void)
{
}


void PushButton::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case Expose:
	render ();
	break;  
 
    case ButtonPress:
	bpress ((XButtonEvent *) E);
	break;  

    case ButtonRelease:
	brelse ((XButtonEvent *) E);
	break;

    default: 
	fprintf (stderr, "PushButton: event %d\n", E->type );
    }
}


void PushButton::bpress (XButtonEvent *E)
{
    int r = 0;

    if (E->button < 4)
    {
        _keymod = E->state;
        _button = E->button;
        r = handle_press ();
    }
    render ();
    if (r) callback (r);
}


void PushButton::brelse (XButtonEvent *E)
{
    int r = 0;

    if (E->button < 4)
    {
        _keymod = E->state;
        _button = E->button;
        r = handle_relse ();
    }
    render ();
    if (r) callback (r);
}


void PushButton::set_state (int s)
{
    if (_state != s)
    {
        _state = s;	
        render ();
    }
}


void PushButton::render (void)
{
    XPutImage (dpy (), win (), dgc (), _image->_ximage,
               _image->_x0, _image->_y0 + _state * _image->_dy, 0, 0, _image->_dx, _image->_dy);
}


}
