/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Pane.H - OSC Subwindow
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef FL_OSC_PANE_H
#define FL_OSC_PANE_H
#include <FL/Fl_Group.H>
#include <FL/Fl_Double_Window.H>
#include <string>
#include "Osc_DataModel.h"

class Fl_Osc_Pane
{
    public:
        Fl_Osc_Pane(void);
        class Fl_Osc_Interface *osc;
        std::string base;
        virtual std::string loc(void) const = 0;
        virtual void rebase(std::string new_base) = 0;
};

class Fl_Osc_Window:public Fl_Double_Window, public Fl_Osc_Pane
{
    Osc_DataModel *title_ext;
    std::string rewrite_rule;
    std::string title_orig;
    std::string title_new;
    public:
        Fl_Osc_Window(int w, int h, const char *L=0);
        ~Fl_Osc_Window(void);

        void init(Fl_Osc_Interface *osc_, std::string loc_);
        virtual std::string loc(void) const;
        virtual void rebase(std::string new_base);
        void update();
        void update_title();
};


class Fl_Osc_Group:public Fl_Osc_Pane, public Fl_Group
{
    public:
        Fl_Osc_Group(int x, int y, int w, int h, const char *L=0);

        std::string ext;

        virtual std::string loc(void) const;
        virtual void reext(std::string new_base);
        virtual void rebase(std::string new_ext);
};
#endif

