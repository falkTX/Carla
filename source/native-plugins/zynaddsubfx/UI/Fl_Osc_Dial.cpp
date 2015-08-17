#include <FL/Fl.H>
#include "Fl_Osc_Dial.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

static void callback_fn_dial(Fl_Widget *w, void *)
{
    ((Fl_Osc_Dial*)w)->cb();
}

Fl_Osc_Pane *fetch_osc_pane(Fl_Widget *w)
{
    if(!w)
        return NULL;

    Fl_Osc_Pane *pane = dynamic_cast<Fl_Osc_Pane*>(w->parent());
    if(pane)
        return pane;
    return fetch_osc_pane(w->parent());
}

Fl_Osc_Dial::Fl_Osc_Dial(int X, int Y, int W, int H, const char *label)
    :WidgetPDial(X,Y,W,H, label), Fl_Osc_Widget(this), alt_style(false), dead(false)
{
    bounds(0.0, 127.0f);
    WidgetPDial::callback(callback_fn_dial);
}


void Fl_Osc_Dial::init(std::string path_)
{
    assert(osc);
    ext = path_;
    oscRegister(path_.c_str());
};

void Fl_Osc_Dial::alt_init(std::string base, std::string path_)
{
    Fl_Osc_Pane *pane = fetch_osc_pane(this);
    assert(pane);
    osc = pane->osc;
    assert(osc);
    loc  = base;
    oscRegister(path_.c_str());
    ext = path_;
    alt_style = true;
}

Fl_Osc_Dial::~Fl_Osc_Dial(void)
{}

void Fl_Osc_Dial::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

int Fl_Osc_Dial::handle(int ev)
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
    return WidgetPDial::handle(ev);
}

void Fl_Osc_Dial::OSC_value(int v)
{
    if(64 != (int)minimum())
        value(v+minimum()+fmodf(value(), 1));
    else
        value(v+fmodf(value(), 1));
}

void Fl_Osc_Dial::OSC_value(char v)
{
    value(v+minimum()+fmodf(value(), 1));
}

void Fl_Osc_Dial::update(void)
{
    osc->requestValue(loc+ext);
}

void Fl_Osc_Dial::cb(void)
{
    assert(osc);

    if(64 != (int)minimum())
        oscWrite(ext, "i", (int)(value()-minimum()));
    else
        oscWrite(ext, "i", (int)(value()));

    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}

void Fl_Osc_Dial::mark_dead(void)
{
    dead = true;
}

void Fl_Osc_Dial::rebase(std::string new_base)
{
    if(dead || loc == "/")
        return;
    if(!alt_style) {
        Fl_Osc_Widget::rebase(new_base);
        return;
    }

    //ok, for a simple hack here lets just assume that there is one branch
    //missing
    int depth = 0;
    for(int i=0; i<(int)loc.size(); ++i)
        depth += loc[i] == '/';
    int match_depth = 0;
    int match_pos   = 0;
    for(int i=0; i<(int)new_base.size(); ++i) {
        match_depth += new_base[i] == '/';
        if(match_depth == depth) {
            match_pos = i;
            break;
        }
    }

    if(match_pos == 0) {
        //well, that didn't work
        assert(!"good enough hack");
    }

    std::string new_loc = new_base.substr(0, match_pos+1);
    printf("Moving '%s' to\n", (loc+ext).c_str());
    printf("       '%s'\n", (new_loc+ext).c_str());
    oscMove(loc+ext, new_loc+ext);
    loc = new_loc;
}
