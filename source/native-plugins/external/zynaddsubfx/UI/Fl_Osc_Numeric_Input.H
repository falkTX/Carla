/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Numeric_Input.H - OSC Based Numeric Input
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <FL/Fl_Input.H>
#include "Fl_Osc_Widget.H"

class Fl_Osc_Numeric_Input: public Fl_Input, public Fl_Osc_Widget
{
    public:
        Fl_Osc_Numeric_Input(int X, int Y, int W, int H, const char *label = NULL);
        virtual ~Fl_Osc_Numeric_Input(void);
        
        //Normal Initialization
        void init(const char *path);

        virtual void OSC_value(int i) override;
        virtual void OSC_value(float f) override;
        static void  numeric_callback(Fl_Widget *w);
};
