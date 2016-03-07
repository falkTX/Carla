/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Roller.cpp - OSC Powered Roller
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "Fl_Osc_Roller.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

static void callback_fn_roller(Fl_Widget *w, void *)
{
    ((Fl_Osc_Roller*)w)->cb();
}

Fl_Osc_Roller::Fl_Osc_Roller(int X, int Y, int W, int H, const char *label)
    :Fl_Roller(X,Y,W,H, label), Fl_Osc_Widget(this)
{
    Fl_Roller::callback(callback_fn_roller);
    bounds(0.0, 127.0f);
}


void Fl_Osc_Roller::init(const char *path)
{
    name = path;
    oscRegister(path);
};

Fl_Osc_Roller::~Fl_Osc_Roller(void)
{}

void Fl_Osc_Roller::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

void Fl_Osc_Roller::OSC_value(char v)
{
    value(v);
}
        
void Fl_Osc_Roller::update(void)
{
    oscWrite(name);
}

void Fl_Osc_Roller::cb(void)
{
    oscWrite(name, "i", (int)value());
    
    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}
