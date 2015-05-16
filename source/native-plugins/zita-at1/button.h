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


#ifndef __BUTTON_H
#define	__BUTTON_H


#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <clxclient.h>

namespace AT1 {


class ButtonImg
{
public:

    XftColor *_backg;
    XImage   *_ximage;
    int       _x0;
    int       _y0;
    int       _dx;
    int       _dy;
};



class PushButton : public X_window
{
public:

    PushButton (X_window    *parent,
                X_callback  *cbobj,
		ButtonImg   *image,
	        int xp,
                int yp,
                int cbind = 0);

    virtual ~PushButton (void);

    enum { NOP = 100, PRESS, RELSE };

    int    cbind (void) { return _cbind; }
    int    state (void) { return _state; }

    virtual void set_state (int s);

    static void init (X_display *disp);
    static void fini (void);

protected:

    X_callback  *_cbobj;
    int          _cbind;
    ButtonImg   *_image;
    int          _state;

    void render (void);
    void callback (int k) { _cbobj->handle_callb (k, this, 0); }

private:

    void handle_event (XEvent *E);
    void bpress (XButtonEvent *E);
    void brelse (XButtonEvent *E);

    virtual int handle_press (void) = 0;
    virtual int handle_relse (void) = 0;

    static int _keymod;
    static int _button;
};


}

#endif
