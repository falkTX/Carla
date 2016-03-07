/*
  ZynAddSubFX - a software synthesizer

  PartNameButton.h - OSC Renameable Button
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

class PartNameButton:public Fl_Button, public Fl_Osc_Widget
{
    public:
        PartNameButton(int X, int Y, int W, int H, const char *label=NULL);

        virtual ~PartNameButton(void){};
        virtual void OSC_value(const char *);
        std::string the_string;

        //virtual void rebase(std::string) override;
};
