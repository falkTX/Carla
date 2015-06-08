#ifndef NIOUI_H
#define NIOUI_H

#include <FL/Fl.H>
#include <FL/Fl_Window.H>

class NioUI:public Fl_Window
{
    public:
        NioUI(class Fl_Osc_Interface*);
        ~NioUI();
        void refresh();
    private:
        class Fl_Osc_StrChoice * midi;
        class Fl_Osc_StrChoice * audio;
        class Osc_SimpleListModel * midi_opt;
        class Osc_SimpleListModel * audio_opt;
        static void midiCallback(Fl_Widget *c);
        static void audioCallback(Fl_Widget *c);
};

#endif
