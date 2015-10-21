#include <cstdio>
#include <cmath>
#include <FL/Fl_Tooltip.H>
#include <FL/fl_draw.H>
#include "TipWin.h"

TipWin::TipWin(void):Fl_Menu_Window(1, 1)
{
    strcpy(format, "%0.2f");
    set_override();
    end();
}

void TipWin::setRounding(unsigned int digits)
{
    format[3] = "0123456789"[digits < 9 ? digits : 9];
}

void TipWin::draw()
{
    //setup window
    draw_box(FL_BORDER_BOX, 0, 0, w(), h(), Fl_Color(175));
    fl_color(Fl_Tooltip::textcolor());
    fl_font(labelfont(), labelsize());

    //Draw the current string
    fl_draw(getStr(), 3, 3, w() - 6, h() - 6,
            Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_WRAP));
}

void TipWin::showValue(float f)
{
    //convert the value to a string
    char tmp[10];
    snprintf(tmp, 9, format, f);
    tip = tmp;

    textmode = false;
    redraw();
    show();
}

void TipWin::setText(const char *c)
{
    text     = c;
    textmode = true;
    redraw();
}

void TipWin::showText()
{
    if(!text.empty()) {
        textmode = true;
        redraw();
        show();
    }
}

void TipWin::redraw()
{
    // Recalc size of window
    fl_font(labelfont(), labelsize());
    int W = 0, H = 0;
    fl_measure(getStr(), W, H, 0);
    //provide a bit of extra space
    W += 8;
    H += 4;
    size(W, H);
    Fl_Menu_Window::redraw();
}

const char *TipWin::getStr() const
{
    return (textmode ? text : tip).c_str();
}
