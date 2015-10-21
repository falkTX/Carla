#include <cmath>
#include "Fl_Osc_TSlider.H"
//Copyright (c) 2015 Christopher Oliver
//License: GNU GPL version 2 or later


Fl_Osc_TSlider::Fl_Osc_TSlider(int x, int y, int w, int h, const char *label)
    :Fl_Osc_Slider(x, y, w, h, label), value_offset(0.0), value_scale(1.0)
{
    Fl_Group *save = Fl_Group::current();
    tipwin = new TipWin();
    tipwin->hide();
    Fl_Group::current(save);
    tipwin->setRounding();
}

Fl_Osc_TSlider::~Fl_Osc_TSlider()
{
    if (tipwin)
        delete tipwin;
}

void Fl_Osc_TSlider::setRounding(unsigned int digits)
{
    tipwin->setRounding(digits);
}


int Fl_Osc_TSlider::handle(int event)
{
    int super = 1;

    super = Fl_Osc_Slider::handle(event);

    switch(event) {
        case FL_PUSH:
        case FL_MOUSEWHEEL:
            if (!Fl::event_inside(x(),y(),w(),h()))
                return(1);
            tipwin->position(Fl::event_x_root()-Fl::event_x()+x(),
                             Fl::event_y_root()-Fl::event_y()+h()+y()+5);
        case FL_DRAG:
            tipwin->showValue(transform(value()));
            break;
        case FL_RELEASE:
        case FL_HIDE:
        case FL_LEAVE:
            if (tipwin)
                tipwin->hide();
            return 1;
    }
    
    return super;
}

void Fl_Osc_TSlider::set_transform(float scale, float offset)
{
    value_offset = offset;
    value_scale = scale;
}

float Fl_Osc_TSlider::transform(float x)
{
    return value_scale * x + value_offset;
}
