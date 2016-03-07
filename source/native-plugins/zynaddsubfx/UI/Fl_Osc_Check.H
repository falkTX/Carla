/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Check.H - OSC Powered Check Button
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <FL/Fl_Check_Button.H>
#include "Fl_Osc_Widget.H"
#include <string>

using std::string; //yes this is bad form FIXME

class Fl_Osc_Check:public Fl_Check_Button, public Fl_Osc_Widget
{

    public:
        Fl_Osc_Check(int X, int Y, int W, int H, const char *label = NULL);
        //string name,
        //        const char *metadata);

        virtual ~Fl_Osc_Check(void);
        virtual void OSC_value(bool) override;
        using Fl_Osc_Widget::OSC_value;

        void init(std::string loc, char type = 'T');
        void callback(Fl_Callback *cb, void *p = NULL);
        void cb(void);
        static void _cb(Fl_Widget *w, void *);
        int is_osc;
    private:
        std::pair<Fl_Callback*, void*> cb_data;
        char type;
};
