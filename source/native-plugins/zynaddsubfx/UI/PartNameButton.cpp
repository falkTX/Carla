/*
  ZynAddSubFX - a software synthesizer

  PartNameButton.cpp - OSC Renamable Button
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "PartNameButton.h"

PartNameButton::PartNameButton(int X, int Y, int W, int H, const char *label)
    :Fl_Button(X,Y,W,H,label), Fl_Osc_Widget(this)
{
}

void PartNameButton::OSC_value(const char *label_)
{
    the_string = label_;
    label(the_string.c_str());
    redraw();
}
