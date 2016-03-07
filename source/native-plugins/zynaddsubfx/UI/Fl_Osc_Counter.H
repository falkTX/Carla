/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Counter.H - OSC Powered Counter
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <FL/Fl_Counter.H>
#include "Fl_Osc_Widget.H"
#include <string>

class Fl_Osc_Counter: public Fl_Counter, public Fl_Osc_Widget
{
    public:
        Fl_Osc_Counter(int x, int y, int w, int h, const char *label=0);
        void init(const char *path_, char type_ = 'i', int display_off = 0);
        void OSC_value(char);
        void OSC_value(int);
        using Fl_Osc_Widget::OSC_value;
        
        //Refetch parameters
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);

        void cb(void);
        char cb_type;
    private:
        int offset;
        std::pair<Fl_Callback*, void*> cb_data;
};
