/*
    Copyright (C) 2009-2010 Fons Adriaensen <fons@linuxaudio.org>
    Modified by falkTX on Jan-Apr 2015 for inclusion in Carla
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifndef __TMETER_H
#define	__TMETER_H


#include <clxclient.h>

namespace AT1 {


class Tmeter : public X_window
{
public:

    Tmeter (X_window *parent, int xpos, int ypos);
    ~Tmeter (void);
    Tmeter (const Tmeter&);
    Tmeter& operator=(const Tmeter&);

    void update (float v0, float v1);

    static XImage  *_scale;
    static XImage  *_imag0;
    static XImage  *_imag1;

private:

    enum { XS = 173, YS = 17, XM = 0, YM = 0, Y1 = 7, Y2 = 10 };

    void handle_event (XEvent *E);
    void expose (XExposeEvent *E);

    int     _k0;
    int     _k1;
};


}

#endif
