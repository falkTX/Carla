#ifndef TIPWIN_H
#include <string>
#include <FL/Fl_Menu_Window.H>
#include <FL/Fl_Tooltip.H>
#define TIPWIN_H
using namespace std;

class TipWin:public Fl_Menu_Window
{
    public:
        TipWin();
        void draw();
        void showValue(float f);
        void setText(const char *c);
        void showText();
        void setRounding(unsigned int digits = 0);
    private:
        void redraw();
        const char *getStr() const;
        string tip;
        string text;
        bool   textmode;
        char  format[6];
};
#endif
