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
     cb_data(NULL, NULL), just_pushed(true)
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
    Fl_Slider::value(v+min_+value()-floorf(value()));
}

void Fl_Osc_Slider::OSC_value(float v)
{
    const float min_ = min__(minimum(), maximum());//flipped sliders
    Fl_Slider::value(v+min_);
}

void Fl_Osc_Slider::OSC_value(char v)
{
    const float min_ = min__(minimum(), maximum());//flipped sliders
    Fl_Slider::value(v+min_+value()-floorf(value()));
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

#define MOD_MASK (FL_CTRL | FL_SHIFT)

int Fl_Osc_Slider::handle(int ev, int X, int Y, int W, int H)
{
    bool middle_mouse = (ev == FL_PUSH && Fl::event_state(FL_BUTTON2) && !Fl::event_shift());
    bool ctl_click    = (ev == FL_PUSH && Fl::event_state(FL_BUTTON3) && Fl::event_ctrl());
    bool shift_middle = (ev == FL_PUSH && Fl::event_state(FL_BUTTON2) && Fl::event_shift());
    if(middle_mouse || ctl_click) {
        printf("Trying to learn...\n");
        osc->write("/learn", "s", (loc+ext).c_str());
        return 1;
    } else if(shift_middle) {
        osc->write("/unlearn", "s", (loc+ext).c_str());
        return 1;
    }

    int handled;
    float rounded;

    const float range = maximum() - minimum();
    const float absrange = (range > 0 ? range : -range)+1;
    int old_mod_state;
    const float normal_step = range / W;

    switch (ev) {
        case FL_PUSH:
            just_pushed = true;
            mod_state = Fl::event_state() & MOD_MASK;
            slow_state = 0;
            handled = mod_state ? 1 : Fl_Slider::handle(ev, X, Y, W, H);
            break;
        case FL_MOUSEWHEEL:
            mod_state = Fl::event_state() & MOD_MASK;
            if (Fl::event_buttons())
                return 1;
            if (this == Fl::belowmouse() && Fl::e_dy != 0) {
                int step_ = 1, divisor = 16;

                switch (mod_state) {
                    case FL_SHIFT:
                        if (absrange > divisor * 8)
                            step_ = 8;
                    case FL_SHIFT | FL_CTRL:
                        break;
                    case FL_CTRL:
                        divisor = 128;
                    default:
                        step_ = absrange / divisor;
                        if (step_ < 1)
                            step_ = 1;
                }
                int dy = minimum() <=  maximum() ? -Fl::e_dy : Fl::e_dy;
                // Flip sense for vertical sliders.
                dy = this->horizontal() ? dy : -dy;
                handle_drag(clamp(value() + step_ * dy));
            }
            return 1;
        case FL_RELEASE:
            handled = Fl_Slider::handle(ev, X, Y, W, H);
            if (Fl::event_clicks() == 1) {
                Fl::event_clicks(0);
                value(reset_value);
            } else {
                rounded = floorf(value() + 0.5);
                value(clamp(rounded));
            }
            value_damage();
            do_callback();
            break;
        case FL_DRAG: {
            old_mod_state = mod_state;
            mod_state = Fl::event_state() & MOD_MASK;
            if (slow_state == 0 && mod_state == 0)
                return Fl_Slider::handle(ev, X, Y, W, H);

            if (mod_state != 0) {
                slow_state = 1;
            } else if (slow_state == 1)
                slow_state = 2;

            if (just_pushed || old_mod_state != mod_state) {
                just_pushed = false;
                old_value = value();
                start_pos = horizontal() ? Fl::event_x() : Fl::event_y();
                if (slow_state == 1) {
                    denominator = 2.0;
                    float step_ = step();
                    if (step_ == 0) step_ = 1;

                    if (absrange / W / step_ > 32)
                        switch (mod_state) {
                            case FL_CTRL:
                                denominator = 0.15;
                                break;
                            case FL_SHIFT:
                                denominator = 0.7;
                                break;
                            case MOD_MASK:
                                denominator = 3.0;
                                break;
                        }
                    else if (mod_state & FL_SHIFT)
                        denominator = 5.0;

                    if (range < 0)
                        denominator *= -1;
                }
            }

            int delta = (horizontal() ? Fl::event_x() : Fl::event_y())
                - start_pos;
            float new_value;
            if (slow_state == 1) {
                new_value = old_value + delta / denominator;
            } else {
                new_value = old_value + delta * normal_step;
            }
            const float clamped_value = clamp(new_value);
            rounded = floor(clamped_value + 0.5);
            if (new_value != clamped_value) {
                start_pos = horizontal() ? Fl::event_x() : Fl::event_y();
                old_value = rounded;
                if (slow_state == 2 &&
                    ((horizontal() &&
                      (Fl::event_x() < X || Fl::event_x() > X + W)) ||
                     (!horizontal() &&
                      (Fl::event_y() < Y || Fl::event_y() > Y + H))))
                    slow_state = 0;
            }
            value(rounded);
            value_damage();
            do_callback();

            handled = 1;
            break;
        }
        default:
            handled = Fl_Slider::handle(ev, X, Y, W, H);
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
