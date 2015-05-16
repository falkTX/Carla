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
#include "rotary.h"

namespace REV1 {


cairo_t         *RotaryCtl::_cairotype = 0;
cairo_surface_t *RotaryCtl::_cairosurf = 0;

int RotaryCtl::_wb_up = 4;
int RotaryCtl::_wb_dn = 5;
int RotaryCtl::_keymod = 0;
int RotaryCtl::_button = 0;
int RotaryCtl::_rcount = 0;
int RotaryCtl::_rx = 0;
int RotaryCtl::_ry = 0;


RotaryCtl::RotaryCtl (X_window     *parent,
                      X_callback   *cbobj,
                      RotaryImg    *image,
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
    _state (0),
    _count (0),
    _value (0),
    _angle (0)
{
    x_add_events (  ExposureMask
                  | Button1MotionMask | ButtonPressMask | ButtonReleaseMask);
} 


RotaryCtl::~RotaryCtl (void)
{
}


void RotaryCtl::init (X_display *disp)
{
    _cairosurf = cairo_xlib_surface_create (disp->dpy (), 0, disp->dvi (), 50, 50);
    _cairotype = cairo_create (_cairosurf);
}


void RotaryCtl::fini (void)
{
    cairo_destroy (_cairotype);
    cairo_surface_destroy (_cairosurf);
}


void RotaryCtl::handle_event (XEvent *E)
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

    case MotionNotify:
	motion ((XMotionEvent *) E);
	break;

    default: 
	fprintf (stderr, "RotaryCtl: event %d\n", E->type );
    }
}


void RotaryCtl::bpress (XButtonEvent *E)
{
    int    r = 0;
    double d;

    d = hypot (E->x - _image->_xref, E->y - _image->_yref);
    if (d > _image->_rad + 3) return;
    _keymod  = E->state;
    if (E->button < 4)
    {
	_rx = E->x;
	_ry = E->y;
	_button = E->button;
        r = handle_button ();
	_rcount = _count;
    }
    else if (_button) return;
    else if ((int)E->button == _wb_up)
    {
        r = handle_mwheel (1);
    } 
    else if ((int)E->button == _wb_dn) 
    {
        r = handle_mwheel (-1);
    } 
    if (r)
    {
        callback (r);
	render ();
    }
}


void RotaryCtl::brelse (XButtonEvent *E)
{
    if (_button == (int)E->button)
    {
	_button = 0;
	callback (RELSE);
    }
}


void RotaryCtl::motion (XMotionEvent *E)
{
    int dx, dy, r;

    if (_button)
    {
        _keymod = E->state;
        dx = E->x - _rx;
        dy = E->y - _ry;
        r = handle_motion (dx, dy);
	if (r)
	{
            callback (r);
	    render ();
	}
    }
}


void RotaryCtl::set_state (int s)
{
    _state = s;	
    render ();
}


void RotaryCtl::render (void)
{
    XImage  *I;
    double  a, c, r, x, y;

    I = _image->_image [_state];
    XPutImage (dpy (), win (), dgc (), I,
               _image->_x0, _image->_y0, 0, 0, _image->_dx, _image->_dy);
    cairo_xlib_surface_set_drawable (_cairosurf, win(),
                                     _image->_dx, _image->_dy);
    c = _image->_lncol [_state] ? 1.0 : 0.0;
    a = _angle * M_PI / 180;
    r = _image->_rad;
    x = _image->_xref;
    y = _image->_yref;
    cairo_new_path (_cairotype);
    cairo_move_to (_cairotype, x, y);
    x += r * sin (a);
    y -= r * cos (a);
    cairo_line_to (_cairotype, x, y);
    cairo_set_source_rgb (_cairotype, c, c, c);
    cairo_set_line_width (_cairotype, 1.8);
    cairo_stroke (_cairotype);
}


}
