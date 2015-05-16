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


#ifndef __MAINWIN_H
#define	__MAINWIN_H


#include <clxclient.h>
#include "guiclass.h"
#include "jclient.h"
#include "tmeter.h"
#include "global.h"

class ZitaPipeClient;

namespace AT1 {


class Mainwin : public A_thread, public X_window, public X_callback
{
public:

    struct ValueChangedCallback {
        virtual ~ValueChangedCallback() {}
        virtual void noteMaskChangedCallback(int) = 0;
        virtual void valueChangedCallback(uint, float) = 0;
    };


    enum { XSIZE = 580, YSIZE = 75 };

    Mainwin (X_rootwin *parent, X_resman *xres, int xp, int yp, ValueChangedCallback* valuecb);
    ~Mainwin (void);
    Mainwin (const Mainwin&);
    Mainwin& operator=(const Mainwin&);

    void stop (void) { _stop = true; }
    int process (void); 

    void setdata_ui (float error, int noteset, int midiset);
    void setchan_ui (int chan);
    void setmask_ui (int mask);

private:

    enum { B_MIDI = 12, B_CHAN = 13 };
    enum { R_TUNE, R_FILT, R_BIAS, R_CORR, R_OFFS, NROTARY };
 
    virtual void thr_main (void) {}

    void handle_time (void);
    void handle_stop (void);
    void handle_event (XEvent *);
    void handle_callb (int type, X_window *W, XEvent *E);
    void showval (int k);
    void expose (XExposeEvent *E);
    void clmesg (XClientMessageEvent *E);
    void redraw (void);
    void setchan (int d);

    Atom            _atom;
    bool            _stop;
    bool            _ambis;
    X_resman       *_xres;
    int             _notes;
    PushButton     *_bmidi;
    PushButton     *_bnote [12];
    RotaryCtl      *_rotary [NROTARY];
    Tmeter         *_tmeter;
    X_textip       *_textln;
    X_tbutton      *_bchan;
    int             _midich;
    int             _ttimer;

    float z_error;
    int z_noteset, z_midiset;

    ValueChangedCallback* _valuecb;

    friend class ::ZitaPipeClient;
};


}

#endif
