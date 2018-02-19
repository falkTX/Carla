/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Check.cpp - OSC Powered Check Button
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "Fl_Osc_Check.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

Fl_Osc_Check::Fl_Osc_Check(int X, int Y, int W, int H, const char *label)
    :Fl_Check_Button(X,Y,W,H,label), Fl_Osc_Widget(this), is_osc(false), cb_data(NULL, NULL)
{
    Fl_Check_Button::callback(Fl_Osc_Check::_cb);
}

Fl_Osc_Check::~Fl_Osc_Check(void)
{}

void Fl_Osc_Check::OSC_value(bool v)
{
    value(v);

    is_osc = true;
    if(cb_data.first)
        cb_data.first(this, cb_data.second);
    is_osc = false;
}

void Fl_Osc_Check::init(std::string path, char type)
{
    this->ext = path;
    this->type = type;
    oscRegister(path.c_str());
}

void Fl_Osc_Check::cb(void)
{
    //Order is significant for takeback style callbacks
    if(cb_data.first)
        cb_data.first(this, cb_data.second);


    if(type == 'T')
        oscWrite(ext, value() ? "T" : "F");
    else {
	if(type=='c')
	    fprintf(stderr, "invalid `c' from checkbox %s%s, using `i'\n", loc.c_str(), ext.c_str());
	oscWrite(ext, "i", value());
    }
}

void Fl_Osc_Check::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

void Fl_Osc_Check::_cb(Fl_Widget *w, void *)
{
    static_cast<Fl_Osc_Check*>(w)->cb();
}
