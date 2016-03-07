/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Slider.H - OSC Based Slider
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <FL/Fl_Slider.H>
#include "Fl_Osc_Widget.H"
#include <string>

class Fl_Osc_Slider:public Fl_Slider, public Fl_Osc_Widget
{
    public:
        Fl_Osc_Slider(int X, int Y, int W, int H, const char *label = NULL);
       // string name,
        //        const char *metadata);

        virtual ~Fl_Osc_Slider(void);
        void OSC_value(int) override;
        void OSC_value(float) override;
        void OSC_value(char) override;
        void init(std::string, char type = 'f');

        //Refetch parameter information
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);
        
        //MIDI Learn
        int handle(int ev, int X, int Y, int W, int H);
        int handle(int ev);

        virtual void cb(void);
        static void _cb(Fl_Widget *w, void *);
        float reset_value;
    protected:
        char osc_type;
        std::pair<Fl_Callback*, void*> cb_data;

    private:
        float old_value;
        int mod_state;
        int slow_state;
        int start_pos;
        bool just_pushed;
    float denominator;
};
