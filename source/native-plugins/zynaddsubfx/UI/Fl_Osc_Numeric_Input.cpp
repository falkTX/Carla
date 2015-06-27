#include "Fl_Osc_Numeric_Input.H"

Fl_Osc_Numeric_Input::Fl_Osc_Numeric_Input(int X, int Y, int W, int H, const char *label)
    :Fl_Input(X,Y,W,H, label), Fl_Osc_Widget(this)
{
    callback(numeric_callback);
}

Fl_Osc_Numeric_Input::~Fl_Osc_Numeric_Input(void)
{}

void Fl_Osc_Numeric_Input::init(const char *path)
{
    ext = path;
    oscRegister(path);
}

void Fl_Osc_Numeric_Input::OSC_value(float f)
{
    OSC_value((int)f);
}

void Fl_Osc_Numeric_Input::OSC_value(int i)
{
    char buf[128];
    snprintf(buf, 128, "%d", i);
    value(buf);
}

void Fl_Osc_Numeric_Input::numeric_callback(Fl_Widget *w)
{
    auto &ww = *(Fl_Osc_Numeric_Input *)w;
    int x = atoi(ww.value());
    if(x)
        ww.oscWrite(ww.ext, "i", x);
}
