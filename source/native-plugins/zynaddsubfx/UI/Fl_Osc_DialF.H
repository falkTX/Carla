/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_DialF.H - OSC Powered Real Valued Dial
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <FL/Fl_Dial.H>
#include "WidgetPDial.h"
#include "Fl_Osc_Widget.H"
#include <string>

class Fl_Osc_DialF:public WidgetPDial, public Fl_Osc_Widget
{

    public:
        Fl_Osc_DialF(int X, int Y, int W, int H, const char *label = NULL);
        virtual ~Fl_Osc_DialF(void);
        void init(const char *path);
        void OSC_value(float);

        //Refetch parameter information
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);

        //Midi learn handler
        int handle(int);

        void cb(void);
    private:
        std::pair<Fl_Callback*, void*> cb_data;
};
