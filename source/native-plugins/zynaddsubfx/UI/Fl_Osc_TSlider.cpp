#include <cmath>
#include "Fl_Osc_TSlider.H"
//Copyright (c) 2015 Christopher Oliver
//License: GNU GPL version 2 or later

static float identity_ts(float value)
{
    return value;
}

Fl_Osc_TSlider::Fl_Osc_TSlider(int x, int y, int w, int h, const char *label)
    :Fl_Osc_Slider(x, y, w, h, label), transform(identity_ts)
{
    Fl_Group *save = Fl_Group::current();
    tipwin = new TipWin();
    tipwin->hide();
    Fl_Group::current(save);
    tipwin->set_rounding();
}

Fl_Osc_TSlider::~Fl_Osc_TSlider()
{
    if (tipwin)
        delete tipwin;
}

void Fl_Osc_TSlider::set_rounding(unsigned int digits)
{
    tipwin->set_rounding(digits);
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

void Fl_Osc_TSlider::set_transform(float (*transformer)(float))
{
    transform = transformer;
}
