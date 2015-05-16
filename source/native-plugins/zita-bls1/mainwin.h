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


#ifndef __MAINWIN_H
#define	__MAINWIN_H


#include <clxclient.h>
#include "guiclass.h"
#include "jclient.h"
#include "global.h"

class ZitaPipeClient;

namespace BLS1 {


class Mainwin : public A_thread, public X_window, public X_callback
{
public:

    struct ValueChangedCallback {
        virtual ~ValueChangedCallback() {}
        virtual void valueChangedCallback(uint, double) = 0;
    };


    enum { XSIZE = 540, YSIZE = 75 };

    Mainwin (X_rootwin *parent, X_resman *xres, int xp, int yp, ValueChangedCallback* valuecb);
    ~Mainwin (void);
    Mainwin (const Mainwin&);
    Mainwin& operator=(const Mainwin&);

    void stop (void) { _stop = true; }
    int process (void); 

private:

    enum { INPBAL, HPFILT, SHGAIN, SHFREQ, LFFREQ, LFGAIN, NROTARY  };
 
    virtual void thr_main (void) {}

    void handle_time (void);
    void handle_stop (void);
    void handle_event (XEvent *);
    void handle_callb (int type, X_window *W, XEvent *E);
    void expose (XExposeEvent *E);
    void clmesg (XClientMessageEvent *E);
    void redraw (void);
    void numdisp (int ind);
    void fmtfreq (int ind);

    Atom            _atom;
    bool            _stop;
    X_resman       *_xres;
    RotaryCtl      *_rotary [NROTARY];
    X_textip       *_numtext;
    int             _parmind;
    int             _timeout;
    int             _touch;

    ValueChangedCallback* _valuecb;

    friend class ::ZitaPipeClient;
};


}

#endif
