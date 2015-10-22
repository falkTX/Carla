#ifndef FL_OSC_TSLIDER_H
#define FL_OSC_TSLIDER_H
#include "Fl_Osc_Slider.H"
#include "TipWin.h"


class Fl_Osc_TSlider:public Fl_Osc_Slider
{
    public:
        Fl_Osc_TSlider(int x, int y, int w, int h, const char *label = 0);
        ~Fl_Osc_TSlider();
        int handle(int event);
	void setTransform(float scale = 1.0, float offset = 0.0);
	float transform(float x);
        void setRounding(unsigned int digits = 0);
    private:
        class TipWin * tipwin;
        float value_offset;
        float value_scale;
};
#endif
