#pragma once
#include <FL/Fl_Value_Slider.H>
#include "Fl_Osc_Slider.H"
#include <string>

class Fl_Osc_VSlider:public Fl_Osc_Slider
{

    public:
        Fl_Osc_VSlider(int X, int Y, int W, int H, const char *label = NULL);

        virtual ~Fl_Osc_VSlider(void);
        Fl_Font textfont_;
        Fl_Fontsize textsize_;
        Fl_Color textcolor_;

        void init(std::string, char type = 'i');
        //Refetch parameter information
        
        //MIDI Learn
        int handle(int);

        // Value Slider add-ins.
        Fl_Font textfont() const {return textfont_;}
        void textfont(Fl_Font s) {textfont_ = s;}
        Fl_Fontsize textsize() const {return textsize_;}
        void textsize(Fl_Fontsize s) {textsize_ = s;}
        Fl_Color textcolor() const {return textcolor_;}
        void textcolor(Fl_Color s) {textcolor_ = s;}

    protected:
        void draw(void);
};
