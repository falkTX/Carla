/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Button.H - OSC Powered Button
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <FL/Fl_Button.H>
#include "Fl_Osc_Widget.H"
#include <string>

class Fl_Osc_Button:public Fl_Button, public Fl_Osc_Widget
{

    public:
        Fl_Osc_Button(int X, int Y, int W, int H, const char *label);

        virtual ~Fl_Osc_Button(void);
        virtual void OSC_value(bool);

        virtual void rebase(std::string) override;

        //void init(Fl_Osc_Interface *, std::string);
        //void init(std::string);

        //void cb(void);
        //static void _cb(Fl_Widget *w, void *);
};
