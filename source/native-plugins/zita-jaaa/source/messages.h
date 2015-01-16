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


#ifndef __MESSAGES_H
#define __MESSAGES_H


#include <clthreads.h>


#define  EV_MESG   0
#define  EV_X11   16
#define  EV_JACK  29
#define  EV_TRIG  30
#define  EV_EXIT  31

#define  M_BUFFP  1
#define  M_INPUT  2
#define  M_JINFO  3
#define  M_GENPAR 4


class M_buffp : public ITC_mesg
{
public:

    M_buffp (float *data, int size, int step) : 
	ITC_mesg (M_BUFFP),
        _data (data),
        _size (size),
        _step (step) {}

    float    *_data;
    int       _size;
    int       _step;
};

class M_input : public ITC_mesg
{
public:

    M_input (int input) :
        ITC_mesg (M_INPUT),
        _input (input) {}

    int       _input;
};

class M_jinfo : public ITC_mesg
{
public:

    M_jinfo (unsigned long fsamp, unsigned long fsize, const char *jname) :
        ITC_mesg (M_JINFO),
        _fsamp (fsamp),
	_fsize (fsize),
        _jname (jname) {}

    unsigned long _fsamp;
    unsigned long _fsize;
    const char   *_jname;
};

class M_genpar : public ITC_mesg
{
public:

    enum { WNOISE = 256, SINE = 512 };

    M_genpar (void) :
        ITC_mesg (M_GENPAR) {}

    int         _g_bits;
    float       _a_noise;
    float       _a_sine;
    float       _f_sine;
};


#endif
