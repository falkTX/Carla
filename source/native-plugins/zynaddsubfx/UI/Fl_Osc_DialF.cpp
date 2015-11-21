#include "Fl_Osc_DialF.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

template<typename A, typename B>
B string_cast(const A &a)
{
    std::stringstream s;
    s.precision(3);
    B b;
    s << " " << a << " ";
    s >> b;
    return b;
}

static void callback_fn_dialf(Fl_Widget *w, void *)
{
    ((Fl_Osc_DialF*)w)->cb();
}

Fl_Osc_DialF::Fl_Osc_DialF(int X, int Y, int W, int H, const char *label)
    :WidgetPDial(X,Y,W,H, label), Fl_Osc_Widget()
{
    //bounds(0.0, 127.0f);
    WidgetPDial::callback(callback_fn_dialf);
}


void Fl_Osc_DialF::init(const char *path)
{
    Fl_Osc_Pane *pane = fetch_osc_pane(this);
    assert(pane);
    osc = pane->osc;
    ext = path;
    loc = pane->base;
    oscRegister(path);
    integer_step = false;
};

Fl_Osc_DialF::~Fl_Osc_DialF(void)
{}

void Fl_Osc_DialF::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

int Fl_Osc_DialF::handle(int ev)
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
    return WidgetPDial::handle(ev);
}

void Fl_Osc_DialF::OSC_value(float v)
{
    value(v);
}

void Fl_Osc_DialF::update(void)
{
    oscWrite(ext);
}

void Fl_Osc_DialF::cb(void)
{
    assert(osc);

    oscWrite(ext, "f", (float)value());

    if(cb_data.first)
        cb_data.first(this, cb_data.second);
//    label_str = string_cast<float,string>(val);
//    label("                ");
//    label(label_str.c_str());
}
