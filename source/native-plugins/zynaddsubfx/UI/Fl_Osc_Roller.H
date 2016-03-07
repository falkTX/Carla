/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Roller.H - OSC Powered Roller
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <FL/Fl_Roller.H>
#include "Fl_Osc_Widget.H"

class Fl_Osc_Roller:public Fl_Roller, Fl_Osc_Widget
{

    public:
        Fl_Osc_Roller(int X, int Y, int W, int H, const char *label = NULL);
        virtual ~Fl_Osc_Roller(void);
        void init(const char *path);
        //void OSC_value(float);
        void OSC_value(char) override;
        using Fl_Osc_Widget::OSC_value;

        //Refetch parameter information
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);

        void cb(void);
    private:
        std::string name;
        std::pair<Fl_Callback*, void*> cb_data;
};
