/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_VSlider.cpp - OSC Based Vertical Slider
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include "Fl_Osc_VSlider.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

Fl_Osc_VSlider::Fl_Osc_VSlider(int X, int Y, int W, int H, const char *label)
    :Fl_Osc_Slider(X,Y,W,H,label)
{
    //bounds(0.0f,1.0f);
    Fl_Slider::callback(Fl_Osc_Slider::_cb);
    textfont_ = FL_HELVETICA;
    textsize_ = 10;
    textcolor_ = FL_FOREGROUND_COLOR;
}

Fl_Osc_VSlider::~Fl_Osc_VSlider(void)
{}

void Fl_Osc_VSlider::init(std::string path_, char type_)
{
    Fl_Osc_Slider::init(path_, type_);
}

void Fl_Osc_VSlider::draw() {
    int sxx = x(), syy = y(), sww = w(), shh = h();
    int bxx = x(), byy = y(), bww = w(), bhh = h();
    if (horizontal()) {
        bww = 35; sxx += 35; sww -= 35;
    } else {
        syy += 25; bhh = 25; shh -= 25;
    }
    if (damage()&FL_DAMAGE_ALL) draw_box(box(),sxx,syy,sww,shh,color());
    Fl_Osc_Slider::draw(sxx+Fl::box_dx(box()),
                        syy+Fl::box_dy(box()),
                        sww-Fl::box_dw(box()),
                        shh-Fl::box_dh(box()));
    draw_box(box(),bxx,byy,bww,bhh,color());
    char buf[128];
    format(buf);
    fl_font(textfont(), textsize());
    fl_color(active_r() ? textcolor() : fl_inactive(textcolor()));
    fl_draw(buf, bxx, byy, bww, bhh, FL_ALIGN_CLIP);
}

int Fl_Osc_VSlider::handle(int ev)
{
    if (ev == FL_PUSH && Fl::visible_focus()) {
        Fl::focus(this);
        redraw();
    }
    int sxx = x(), syy = y(), sww = w(), shh = h();
    if (horizontal()) {
        sxx += 35; sww -= 35;
    } else {
        syy += 25; shh -= 25;
    }

    return Fl_Osc_Slider::handle(ev,
                                 sxx+Fl::box_dx(box()),
                                 syy+Fl::box_dy(box()),
                                 sww-Fl::box_dw(box()),
                                 shh-Fl::box_dh(box()));
}
