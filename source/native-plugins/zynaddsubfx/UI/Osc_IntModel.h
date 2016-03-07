/*
  ZynAddSubFX - a software synthesizer

  Osc_IntModel.h - OSC Updated Integer
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include "Fl_Osc_Widget.H"
#include <functional>
#include <vector>
#include <rtosc/rtosc.h>

class Osc_IntModel:public Fl_Osc_Widget
{
    public:
        Osc_IntModel(Fl_Osc_Interface *osc_)
            :Fl_Osc_Widget("", osc_), value(0)
        {
            assert(osc);
        }

        typedef int value_t;
        value_t value;
        std::function<void(value_t)> callback;

        void updateVal(value_t v)
        {
            value = v;
            oscWrite(ext, "i", v);
        }

        void doUpdate(std::string url)
        {
            if(!ext.empty())
                osc->removeLink(this);
            ext = url;

            oscRegister(ext.c_str());
        }

        //Raw messages
        virtual void OSC_raw(const char *msg)
        {
            std::string args = rtosc_argument_string(msg);
            if(args == "i") {
                value = rtosc_argument(msg, 0).i;
                if(callback)
                    callback(value);
            } else if(args == "T") {
                value = 1;
                if(callback)
                    callback(value);
            }  else if(args == "F") {
                value = 0;
                if(callback)
                    callback(value);
            }
        }
};
