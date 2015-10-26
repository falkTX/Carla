#include <FL/Fl.H>
#include "Fl_Osc_Slider.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>
#include "../Misc/Util.h"

static double min__(double a, double b)
{
    return a<b?a:b;
}

Fl_Osc_Slider::Fl_Osc_Slider(int X, int Y, int W, int H, const char *label)
    :Fl_Slider(X,Y,W,H,label), Fl_Osc_Widget(this), reset_value(0),
     cb_data(NULL, NULL)
{
    //bounds(0.0f,1.0f);
    Fl_Slider::callback(Fl_Osc_Slider::_cb);
}

void Fl_Osc_Slider::init(std::string path_, char type_)
{
    osc_type = type_;
    ext = path_;
    oscRegister(ext.c_str());
}

Fl_Osc_Slider::~Fl_Osc_Slider(void)
{}

void Fl_Osc_Slider::OSC_value(int v)
{
    const float min_ = min__(minimum(), maximum());//flipped sliders
    Fl_Slider::value(v+min_+fmodf(value(),1.0));
}

void Fl_Osc_Slider::OSC_value(float v)
{
    const float min_ = min__(minimum(), maximum());//flipped sliders
    Fl_Slider::value(v+min_);
}

void Fl_Osc_Slider::OSC_value(char v)
{
    const float min_ = min__(minimum(), maximum());//flipped sliders
    Fl_Slider::value(v+min_+fmodf(value(),1.0));
}

void Fl_Osc_Slider::cb(void)
{
    const float min_ = min__(minimum(), maximum());//flipped sliders
    const float val = Fl_Slider::value();
    if(osc_type == 'f')
        oscWrite(ext, "f", val-min_);
    else if(osc_type == 'i')
        oscWrite(ext, "i", (int)(val-min_));
    else {
	fprintf(stderr, "invalid `c' from slider %s%s, using `i'\n", loc.c_str(), ext.c_str());
	oscWrite(ext, "i", (int)(val-min_));
    }
    //OSC_value(val);

    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}

void Fl_Osc_Slider::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

int Fl_Osc_Slider::handle(int ev, int X, int Y, int W, int H)
{
    bool middle_mouse = (ev == FL_PUSH && Fl::event_state(FL_BUTTON2) && !Fl::event_shift());
    bool ctl_click    = (ev == FL_PUSH && Fl::event_state(FL_BUTTON1) && Fl::event_ctrl());
    bool shift_middle = (ev == FL_PUSH && Fl::event_state(FL_BUTTON2) && Fl::event_shift());
    if(middle_mouse || ctl_click) {
        printf("Trying to learn...\n");
        osc->write("/learn", "s", (loc+ext).c_str());
        return 1;
    } else if(shift_middle) {
        osc->write("/unlearn", "s", (loc+ext).c_str());
        return 1;
    }

    int handled, rounded;
    bool reset_requested = false;
    switch (ev) {
        case FL_MOUSEWHEEL:
            if (this == Fl::belowmouse() && Fl::e_dy != 0) {
                int step = 1, divisor = 16;
                switch (Fl::event_state() & ( FL_CTRL | FL_SHIFT)) {
                    case FL_SHIFT:
                        step = 8;
                    case FL_SHIFT | FL_CTRL:
                        break;
                    case FL_CTRL:
                        divisor = 128;
                    default:
                        step = (fabs(maximum() - minimum()) + 1) / divisor;
                        if (step < 1)
                            step = 1;
                }
                int dy = minimum() <=  maximum() ? -Fl::e_dy : Fl::e_dy;
                handle_drag(clamp(value() + step * dy));
            }
            return 1;
        case FL_RELEASE:
            rounded = value() + 0.5;
            value(clamp((double)rounded));
            if (Fl::event_clicks() == 1) {
                Fl::event_clicks(0);
                reset_requested = true;
            }
    }
    
    if (!Fl::event_shift()) {
        handled = Fl_Slider::handle(ev, X, Y, W, H);
        if (reset_requested) {
            value(reset_value);
            value_damage();
            if (this->when() != 0)
                do_callback();
        }
        return handled;
    }

    // Slow down the drag.
    // Handy if the slider has a large delta bigger than a mouse quantum.
    // Somewhat tricky to use with OSC feedback.
    // To change direction of movement, one must reclick the handle.
    int old_value = value();
    handled = Fl_Slider::handle(ev, X, Y, W, H);
    int delta = value() - old_value;
    if (ev == FL_DRAG && (delta < -1 || delta > 1)) {
        value(clamp((old_value + (delta > 0 ? 1 : -1))));
        value_damage();
        do_callback();
    }
    return handled;
}

int Fl_Osc_Slider::handle(int ev) {
    return handle(ev,
                  x()+Fl::box_dx(box()),
                  y()+Fl::box_dy(box()),
                  w()-Fl::box_dw(box()),
                  h()-Fl::box_dh(box()));
}

void Fl_Osc_Slider::update(void)
{
    oscWrite(ext, "");
}

void Fl_Osc_Slider::_cb(Fl_Widget *w, void *)
{
    static_cast<Fl_Osc_Slider*>(w)->cb();
}
