/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Input.cpp - OSC Powered Input Field
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "Fl_Osc_Input.H"

Fl_Osc_Input::Fl_Osc_Input(int X, int Y, int W, int H, const char *label)
    :Fl_Input(X,Y,W,H, label), Fl_Osc_Widget(this)
{
}

Fl_Osc_Input::~Fl_Osc_Input(void)
{}

void Fl_Osc_Input::init(const char *path)
{
    ext = path;
    oscRegister(path);
}

void Fl_Osc_Input::OSC_value(const char *v)
{
    value(v);
}
