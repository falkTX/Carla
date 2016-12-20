/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Value.cpp - OSC Based Fl_Value
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "Fl_Osc_Value.H"

static void callback_fn_value(Fl_Widget *w, void *)
{
    ((Fl_Osc_Value*)w)->cb();
}

Fl_Osc_Value::Fl_Osc_Value(int X, int Y, int W, int H, const char *label)
    :Fl_Value_Input(X,Y,W,H, label), Fl_Osc_Widget(this)
{
    Fl_Value_Input::callback(callback_fn_value);
}

Fl_Osc_Value::~Fl_Osc_Value(void)
{}

void Fl_Osc_Value::init(const char *path_)
{
    ext = path_;
    oscRegister(path_);
}

void Fl_Osc_Value::OSC_value(float v)
{
    //static char value[1024];
    //snprintf(value, sizeof(value), "%f", v);
    this->value(v);
}

void Fl_Osc_Value::cb(void)
{
    assert(osc);
    oscWrite(ext, "f", (float)(value()));
}
