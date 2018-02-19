/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Value.H - OSC Based Fl_Value
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <FL/Fl_Value_Input.H>
#include "Fl_Osc_Widget.H"

class Fl_Osc_Value: public Fl_Value_Input, public Fl_Osc_Widget
{
    public:
        Fl_Osc_Value(int X, int Y, int W, int H, const char *label = NULL);
        virtual ~Fl_Osc_Value(void);
        
        //Normal Initialization
        void init(const char *path);
        void OSC_value(float v);

        void cb();
};
